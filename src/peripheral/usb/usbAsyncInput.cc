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
#include <Tracy.hpp>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <future>
#include <gsl/gsl>
#include <libusb.h>
#include <string>
module logic.peripheral;
import logic.util;

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

        // TODO for test. If it works, remove run and kill from the IInput interface - and the interface itself...
        run ();
}

/****************************************************************************/

UsbAsyncInput::~UsbAsyncInput () noexcept
{
        kill ();

        /*
         * Will terminate if exception is thrown from the future.
         * ... which makes using futures pointless if we don't catch
         * exceptions at all. Futures were chosen to catch the exceptions
         * in the first place.
         */
        analyzeFuture.get ();
        acquireFuture.get ();

        {
                std::lock_guard lock{mutex};

                for (auto &[p, ent] : handles) {
                        auto &[ph, dev] = ent;
                        dev->resetDeviceHandle ();
                        libusb_close (ph);
                }

                handles.clear ();
        }

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
                                        auto &entry = input->handles[dev];
                                        entry.first = devHandle;
                                        entry.second = device;
                                }

                                input->handlesCVar.notify_all ();
                                eventQueue->setAlarm<DeviceAlarm> (std::move (device));
                        }
                }
                else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                        {
                                std::lock_guard lock{input->mutex};
                                auto &entry = input->handles.at (dev);

                                auto &intDevice = entry.second;
                                intDevice->resetDeviceHandle ();
                                eventQueue->clearAlarm<DeviceAlarm> (intDevice);

                                libusb_close (entry.first);
                                /*
                                 * Remove from the `handles` set. This releases (or prepares to release)
                                 * all the resources. Removes the InternalHandle, deletes the shared_ptr
                                 * device (if upper layers don't use it), which in turn calls libusb_close.
                                 * If upper layers till hold a shared_pointer to the device, it gets
                                 * destroyed when they release it.
                                 */
                                input->handles.erase (dev);
                        }

                        input->handlesCVar.notify_all ();
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
void UsbAsyncInput::acquireLoop ()
{
        static timeval tv = {.tv_sec = 0, .tv_usec = 10000};
        std::optional<high_resolution_clock::time_point> startPoint;
        setThreadName ("Acquire");

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

/****************************************************************************/

void UsbAsyncInput::analyzeLoop (/* Queue<RawCompressedBlock> *rawQueue, IBackend *backend */)
{
        setThreadName ("Analyze");

        while (true) {
                if (kill_.load ()) {
                        break;
                }

                {
                        /*
                         * Locking for the whole time prevents `handles` collection modification,
                         * specifficaly prevents element removal which can happen when you disconnect
                         * a device.
                         *
                         * TODO But this is inefficient because this would block the whole USB thread
                         * when a device (possibly other than the currently running one) gets deisconnected.
                         * And that would result in a timeout, since we are very tight on bandwidth.
                         *
                         * TODO Though... I could un-register the hot-plug callback if at least one device
                         * is acquiring.
                         */
                        std::unique_lock lock{mutex};
                        handlesCVar.wait (lock, [this] { return !handles.empty () || kill_; });

                        for (auto &[deviceP, entryPair] : handles) {
                                entryPair.second->run ();
                        }
                }
        }
}

/****************************************************************************/

void UsbAsyncInput::run (/* Queue<RawCompressedBlock> *rawQueue, IBackend *backend */)
{
        // Futures are created for exception handling.
        analyzeFuture = std::async (std::launch::async, &UsbAsyncInput::analyzeLoop, this /* , rawQueue, backend */);
        acquireFuture = std::async (std::launch::async, &UsbAsyncInput::acquireLoop, this);
        // TODO start all inputs.
        // auto acquireFuture = std::async (std::launch::async, &logic::IInput::run, &factory.demo ());

        // while (true) {
        //         if (auto now = std::chrono::high_resolution_clock::now ();
        //             config.app.seconds > 0 && now - start >= std::chrono::seconds{config.app.seconds}) {
        //                 std::println ("Time limit reached.");
        //                 break;
        //         }

        //         //  TODO
        //         // if (config.app.bytes > 0 && session.receivedB () >= config.app.bytes) {
        //         //         device->stop ();
        //         //         break;
        //         // }

        //         if (cli::sys::isTermRequested ()) {
        //                 std::println ("Interrupted by user.");
        //                 break;
        //         }

        //         // They get ready only on exception.
        //         if (acquireFuture.wait_for (100ms) == std::future_status::ready) {
        //                 std::println ("acquireFuture has finished.");
        //                 break;
        //         }

        //         if (analyzeFuture.wait_for (100ms) == std::future_status::ready) {
        //                 std::println ("analyzeFuture has finished.");
        //                 break;
        //         }

        //         if (printErrorEvents (eventQueue)) {
        //                 break;
        //         }

        //         // std::this_thread::sleep_for (100ms);
        // }
}

} // namespace logic
