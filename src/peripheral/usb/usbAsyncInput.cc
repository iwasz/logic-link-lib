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

UsbAsyncInput::UsbAsyncInput (EventQueue *eventQueue) : AbstractInput (eventQueue), usbFactory{eventQueue}
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
                                eventQueue->addEvent<ErrorEvent> (std::format ("Could not open USB device: {}, vid: {:x}, pid: {:x}",
                                                                               libusb_error_name (rc), desc.idVendor, desc.idProduct));
                        }
                        else {
                                std::shared_ptr<UsbDevice> device = input->usbFactory.create (desc.idVendor, desc.idProduct, devHandle);
                                {
                                        std::lock_guard lock{input->mutex};
                                        input->handles[dev] = device;
                                }
                                eventQueue->setAlarm<DeviceAlarm> (std::move (device));
                        }
                }
                else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                        auto &intDevice = input->handles.at (dev);
                        eventQueue->clearAlarm<DeviceAlarm> (intDevice);

                        {
                                /*
                                 * Remove from the `handles` set. This releases (or prepares to release)
                                 * all the resources. Removes the InternalHandle, deletes the shared_ptr
                                 * device (if upper layers don't use it), which in turn calls libusb_close.
                                 * If upper layers till hold a shared_pointer to the device, it gets
                                 * destroyed when they release it.
                                 */
                                std::lock_guard lock{input->mutex};
                                input->handles.erase (dev);
                        }
                }
        }
        catch (std::exception const &e) {
                eventQueue->addEvent<ErrorEvent> (std::format ("Exception caught in `UsbAsyncInput::hotplugCallback`: {}, vid: {:x}, pid: {:x}",
                                                               e.what (), desc.idVendor, desc.idProduct));
        }
        catch (...) {
                eventQueue->addEvent<ErrorEvent> (
                        std::format ("Unknown (...) exception caught in `UsbAsyncInput::hotplugCallback`, vid: {:x}, pid: {:x}", desc.idVendor,
                                     desc.idProduct));
        }

        return 0;
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
                if (kill_.load ()) {
                        break;
                }

                if (!startPoint) { // TODO move to a benchmarking class
                        startPoint = high_resolution_clock::now ();
                }

                if (auto r = libusb_handle_events_timeout (nullptr, &tv); r < 0) {
                        /*
                         * After libusb source code analysis I believe that (at least) transfer timeout
                         * erorrs will be directed to the transfer callback and there I can assing the
                         * error to a transfer and device. This way I can inform particular device about
                         * the error.
                         */
                        eventQueue ()->addEvent<ErrorEvent> (
                                std::format ("Error in the main USB loop (libusb_handle_events_timeout): {}", libusb_error_name (r)));
                }

                startPoint.reset ();
        }
}

} // namespace logic
