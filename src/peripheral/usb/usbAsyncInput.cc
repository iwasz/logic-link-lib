/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include "common/params.hh"
#include <chrono>
#include <cstdio>
#include <cstring>
#include <gsl/gsl>
#include <libusb.h>
#include <print>
#include <string>
module logic.peripheral;

namespace logic {
using namespace std::string_literals;
using namespace common::usb;
using namespace std::chrono;

/****************************************************************************/

UsbAsyncInput::UsbAsyncInput (EventQueue *eventQueue) : UsbInput (eventQueue), usbFactory{eventQueue, this}
{
        if (int r = libusb_init_context (/*ctx=*/nullptr, /*options=*/nullptr, /*num_options=*/0); r < 0) {
                throw Exception ("Failed to init USB (libusb_init_context): "s + libusb_error_name (r));
        }

        libusb_device **devs{};
        if (auto cnt = libusb_get_device_list (nullptr, &devs); cnt < 0) {
                throw Exception ("Failed to get a device list. Cnt: "s + std::to_string (cnt));
        }

        // printDevs (devs);
        libusb_free_device_list (devs, 1);

        int rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT,
                                                   LIBUSB_HOTPLUG_ENUMERATE, LIBUSB_HOTPLUG_MATCH_ANY, LIBUSB_HOTPLUG_MATCH_ANY,
                                                   LIBUSB_HOTPLUG_MATCH_ANY, hotplugCallback, this, &hotplugCallbackHandle);

        if (LIBUSB_SUCCESS != rc) {
                throw Exception{"Error creating a hotplug callback: " + std::string{libusb_error_name (rc)}};
        }
}

/****************************************************************************/

UsbAsyncInput::~UsbAsyncInput ()
{
        handles.clear ();
        libusb_hotplug_deregister_callback (nullptr, hotplugCallbackHandle);
        libusb_exit (nullptr);
}

/****************************************************************************/

int UsbAsyncInput::hotplugCallback (libusb_context * /* ctx */, libusb_device *dev, libusb_hotplug_event event, void *userData)
{
        libusb_device_handle *devHandle{};
        libusb_device_descriptor desc{};
        UsbAsyncInput *input = reinterpret_cast<UsbAsyncInput *> (userData);
        EventQueue *eventQueue = input->eventQueue ();

        if (int rc = libusb_get_device_descriptor (dev, &desc); rc < 0) {
                eventQueue->addEvent<ErrorEvent> (std::format ("Could not get device's VID and PID: {}", libusb_error_name (rc)));
                return 0;
        }

        if (!input->usbFactory.isSupported (desc.idVendor, desc.idProduct)) {
                return 0;
        }

        // Catch early, because we're in the libusb thread here.
        try {

                if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
                        if (int rc = libusb_open (dev, &devHandle); LIBUSB_SUCCESS != rc) {
                                eventQueue->addEvent<ErrorEvent> (std::format ("Could not open USB device: {}, vid: {}, pid: {}",
                                                                               libusb_error_name (rc), desc.idVendor, desc.idProduct));
                        }
                        else {
                                std::shared_ptr<UsbDevice> device = input->usbFactory.create (desc.idVendor, desc.idProduct, devHandle);
                                {
                                        std::lock_guard lock{input->mutex};
                                        input->handles[devHandle] = std::make_unique<UsbHandleInternal> (device, input);
                                }
                                eventQueue->setAlarm<DeviceAlarm> (device);
                        }
                }
                else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                        if (devHandle != nullptr) {
                                auto &intHandle = input->handles.at (devHandle);
                                eventQueue->clearAlarm<DeviceAlarm> (intHandle->device);

                                {
                                        /*
                                         * Remove from the handles set. This releases (or prepares to release)
                                         * all the resources. Removes the InternalHandle, deletes the shared_ptr
                                         * device (if upper layers don't use it), which in turn calls libusb_close.
                                         * If upper layers till hold a shared_pointer to the device, it gets
                                         * destroyed when they release it.
                                         */
                                        std::lock_guard lock{input->mutex};
                                        input->handles.erase (devHandle);
                                }
                        }
                }
        }
        catch (std::exception const &e) {
                eventQueue->addEvent<ErrorEvent> (std::format ("Exception caught in `UsbAsyncInput::hotplugCallback`: {}, vid: {}, pid: {}",
                                                               e.what (), desc.idVendor, desc.idProduct));
        }
        catch (...) {
                eventQueue->addEvent<ErrorEvent> (std::format (
                        "Unknown (...) exception caught in `UsbAsyncInput::hotplugCallback`, vid: {}, pid: {}", desc.idVendor, desc.idProduct));
        }

        return 0;
}

/****************************************************************************/

void UsbAsyncInput::transferCallback (libusb_transfer *transfer)
{
        auto *h = reinterpret_cast<UsbHandleInternal *> (transfer->user_data);

        // auto removeCurrentHandle = [h, transfer] {
        //         std::lock_guard lock{h->input->mutex};
        //         h->input->handles.erase (transfer->dev_handle);
        // };

        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                /*
                 * We're in the interal lubusb thread now, we can't throw, thus I
                 */
                auto msg = std::format ("USB transfer status error Code: {}", libusb_error_name (transfer->status));
                h->input->eventQueue ()->addEvent<ErrorEvent> (msg);
                // removeCurrentHandle ();
                return;
        }

        if (h->stopRequest) {
                // removeCurrentHandle ();
                return;
        }

        if (auto rc = libusb_submit_transfer (transfer); rc < 0) {
                auto msg = std::format ("libusb_submit_transfer status error Code: {}", libusb_error_name (rc));
                h->input->eventQueue ()->addEvent<ErrorEvent> (msg);
                // removeCurrentHandle ();
                return;
        }

        if (size_t (transfer->actual_length) != h->transferLen) {
                // TODO this should be removed
                std::println ("Short: {}", transfer->actual_length);
        }

        h->singleTransfer.resize (transfer->actual_length);
        // auto now = high_resolution_clock::now ();
        // benchmarkB += singleTransfer.size ();

        {
                // std::lock_guard lock{mutex};
                // allTransferedB += singleTransfer.size ();
        }

        if (!h->singleTransfer.empty ()) {
                // auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *startPoint).count ())) * 8;
                double mbps = 0;
                RawCompressedBlock rcd{mbps, 0, std::move (h->singleTransfer)}; // TODO resize blocks (if needed)
                h->queue->push (std::move (rcd));
                h->singleTransfer = Bytes (h->transferLen);
                // std::println (".");
        }
}

/*--------------------------------------------------------------------------*/

void UsbHandleInternal::start ()
{
        singleTransfer.resize (transferLen);

        if (transfer = libusb_alloc_transfer (0); transfer == nullptr) {
                /*
                 * This is called from an user thread (via UsbAsyncInput::start) so we are
                 * safe to throw an exception.
                 */
                throw Exception{"Libusb could not instantiate a new transfer using `libusb_alloc_transfer`"};
        }

        libusb_fill_bulk_transfer (transfer, device->device (), common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (),
                                   &UsbAsyncInput::transferCallback, this, common::usb::TIMEOUT_MS);

        if (auto r = libusb_submit_transfer (transfer); r < 0) {
                throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::string{libusb_error_name (r)}};
        }

        device->onStart ();
}

/*--------------------------------------------------------------------------*/

void UsbAsyncInput::start (UsbHandle const &handle)
{
        std::lock_guard lock{mutex};
        auto const *rawDevice = handle.device->device ();
        auto &intHandle = handles.at (rawDevice);
        intHandle->queue = handle.queue;
        intHandle->transferLen = handle.transferLen;
        intHandle->stopRequest = false;
        intHandle->start ();
}

/*--------------------------------------------------------------------------*/

UsbHandleInternal::~UsbHandleInternal () { libusb_free_transfer (transfer); }

/*--------------------------------------------------------------------------*/

void UsbAsyncInput::stop (UsbDevice *device)
{
        device->onStop ();
        auto &handle = handles[device->device ()];
        // Gets removed from the list in the hot-swap callback.
        handle->stopRequest = true;
}

/****************************************************************************/

/**
 * Acquires ~~`wholeDataLenB` bytes of~~ data (blocking function) block by block (`singleTransferLenB`).
 * See src/feature/analyzer/flexio.cc in the firmware project for the other side of the connection.
 */
void UsbAsyncInput::run ()
{
        static timeval tv = {.tv_sec = 0, .tv_usec = 10000};
        std::optional<high_resolution_clock::time_point> startPoint;

        while (true) {
                if (request_.load () == Request::kill) {
                        break;
                }

                if (!startPoint) { // TODO move to a benchmarking class
                        startPoint = high_resolution_clock::now ();
                }

                if (auto r = libusb_handle_events_timeout (nullptr, &tv); r < 0) {
                        std::println ("libusb error: {}", r);
                }

                startPoint.reset ();
        }
}

} // namespace logic
