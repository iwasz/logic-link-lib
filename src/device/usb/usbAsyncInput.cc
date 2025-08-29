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
module logic;

namespace logic {
using namespace std::string_literals;
using namespace common::usb;
using namespace std::chrono;

/****************************************************************************/

UsbAsyncInput::UsbAsyncInput (EventQueue *eventQueue) : UsbInput (eventQueue)
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
        libusb_hotplug_deregister_callback (nullptr, hotplugCallbackHandle);
        libusb_exit (nullptr);
}

/****************************************************************************/

int UsbAsyncInput::hotplugCallback (libusb_context * /* ctx */, libusb_device *dev, libusb_hotplug_event event, void *userData)
{
        libusb_device_handle *devHandle{};
        libusb_device_descriptor desc{};
        UsbAsyncInput *that = reinterpret_cast<UsbAsyncInput *> (userData);

        if (int rc = libusb_get_device_descriptor (dev, &desc); rc < 0) {
                that->eventQueue ()->addEvent<ErrorEvent> ("Could not get device's VID and PID."s);
                return 0;
        }

        if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
                if (int rc = libusb_open (dev, &devHandle); LIBUSB_SUCCESS != rc) {
                        that->eventQueue ()->addEvent<ErrorEvent> ("Could not open USB device"s);
                }
                else {
                        // that->state_.store (State::connectedIdle);
                        that->eventQueue ()->setAlarm<UsbConnectedAlarm> (devHandle, &desc);
                }
        }
        else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                if (devHandle != nullptr) {
                        libusb_close (devHandle);
                        devHandle = NULL;
                        // that->state_.store (State::disconnected);
                        that->eventQueue ()->clearAlarm<UsbConnectedAlarm> (devHandle, &desc);
                        // TODO remove from the handles map.
                }
        }

        return 0;
}

/****************************************************************************/

void UsbAsyncInput::transferCallback (libusb_transfer *transfer)
{
        auto *h = reinterpret_cast<UsbHandleInternal *> (transfer->user_data);

        auto removeCurrentHandle = [h, transfer] {
                std::lock_guard lock{h->input->mutex};
                h->input->handles.erase (transfer->dev_handle);
        };

        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                /*
                 * We're in the interal lubusb thread now, we can't throw, thus I
                 */
                auto msg = std::format ("USB transfer status error Code: {}", libusb_error_name (transfer->status));
                h->input->eventQueue ()->addEvent<ErrorEvent> (msg);
                removeCurrentHandle ();
                return;
        }

        if (h->stopRequest) {
                removeCurrentHandle ();
                return;
        }

        if (auto rc = libusb_submit_transfer (transfer); rc < 0) {
                auto msg = std::format ("libusb_submit_transfer status error Code: {}", libusb_error_name (rc));
                h->input->eventQueue ()->addEvent<ErrorEvent> (msg);
                removeCurrentHandle ();
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
        }
}

/*--------------------------------------------------------------------------*/

UsbHandleInternal::UsbHandleInternal (UsbHandle const &h, UsbAsyncInput *input) : UsbHandle (h), input{input}
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
}

/*--------------------------------------------------------------------------*/

void UsbAsyncInput::start (UsbHandle const &handle)
{
        std::lock_guard lock{mutex};
        auto const *rawDevice = handle.device->device ();
        handles[rawDevice] = std::make_unique<UsbHandleInternal> (handle, this);
        handle.device->onStart ();
}

/*--------------------------------------------------------------------------*/

UsbHandleInternal::~UsbHandleInternal () { libusb_free_transfer (transfer); }

/*--------------------------------------------------------------------------*/

void UsbAsyncInput::stop (UsbDevice *device)
{
        device->onStop ();
        auto &handle = handles[device->device ()];
        handle->stopRequest = true;
}

/****************************************************************************/

/**
 * Acquires ~~`wholeDataLenB` bytes of~~ data (blocking function) block by block (`singleTransferLenB`).
 * See src/feature/analyzer/flexio.cc in the firmware project for the other side of the connection.
 */
void UsbAsyncInput::run ()
{
        {
                /*
                 * For now we allocate the whole (rather big) memory buffer all at once. This is
                 * because we must avoid reallocations in the future.
                 */
                // std::lock_guard lock{mutex};
                // globalStart = high_resolution_clock::now ();
        }

        // int completed{};
        // size_t benchmarkB{};
        std::optional<high_resolution_clock::time_point> startPoint;
        // bool started{};

        while (true) {
                if (request_.load () == Request::kill) {
                        break;
                }

                // if (state_.load () == State::disconnected) {
                //         handleUsbEventsTimeout (1000, nullptr);
                // }

                // if (state_.load () == State::connectedIdle) {
                //         handleUsbEventsTimeout (10, nullptr); // Still LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT can be reporterd.

                //         if (request_ == Request::start) {
                //                 request_ = Request::none;
                //                 state_ = State::transferring;
                //                 started = false;
                //         }
                // }

                // if (state_.load () == State::transferring) {
                // Bytes singleTransfer (device->transferLen ());

                if (!startPoint) { // TODO move to a benchmarking class
                        startPoint = high_resolution_clock::now ();
                }

                // libusb_fill_bulk_transfer (transfer, dev, common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (),
                // transferCallback,
                //                            &completed, common::usb::TIMEOUT_MS);

                // completed = 0;

                // if (auto r = libusb_submit_transfer (transfer); r < 0) {
                //         throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::string{libusb_error_name (r)}};
                // }

                // std::print (".");

                // if (!started) {
                //         device->onStart ();
                //         std::println ("onStart");
                //         started = true;
                // }

                if (auto r = libusb_handle_events (nullptr); r < 0) {
                        std::println ("libusb error: {}", r);
                }

                // if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                //         throw Exception{"USB transfer status error Code: " + std::string{libusb_error_name (transfer->status)}};
                // }

                // if (size_t (transfer->actual_length) != device->transferLen ()) {
                //         std::println ("Short: {}", transfer->actual_length);
                // }

                // singleTransfer.resize (transfer->actual_length);
                // auto now = high_resolution_clock::now ();
                // benchmarkB += singleTransfer.size ();

                // {
                //         // std::lock_guard lock{mutex};
                //         // allTransferedB += singleTransfer.size ();
                // }

                // if (!singleTransfer.empty ()) {
                //         auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *startPoint).count ())) * 8;
                //         RawCompressedBlock rcd{mbps, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
                //         session->rawQueue.push (std::move (rcd));

                //         {
                //                 // std::lock_guard lock{mutex};
                //                 // globalStop = high_resolution_clock::now (); // We want to be up to date, and skip possible time-out.
                //         }
                // }

                // benchmarkB = 0;
                startPoint.reset ();

                // if (request_ == Request::stop && singleTransfer.empty ()) {
                //         device->onStop ();
                //         request_ = Request::none;
                //         state_ = State::connectedIdle;
                //         session = nullptr;
                // }
        }
}

} // namespace logic
