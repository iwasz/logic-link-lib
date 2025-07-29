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
#include <deque>
#include <gsl/gsl>
#include <iterator>
#include <libusb.h>
#include <mutex>
#include <print>
#include <string>
#include <unordered_set>
#include <vector>
module logic;
import :input.usb;

namespace logic {
using namespace std::string_literals;
using namespace common::usb;
using namespace std::chrono;

/****************************************************************************/

UsbAsync::UsbAsync (HotplugHooks const &hotplugHooks) : AbstractInput{hotplugHooks}
{
        if (int r = libusb_init_context (/*ctx=*/nullptr, /*options=*/nullptr, /*num_options=*/0); r < 0) {
                throw Exception ("Failed to init USB (libusb_init_context): "s + libusb_error_name (r));
        }

        initialized = true;

        libusb_device **devs{};
        if (auto cnt = libusb_get_device_list (nullptr, &devs); cnt < 0) {
                throw Exception ("Failed to get a device list. Cnt: "s + std::to_string (cnt));
        }

        // printDevs (devs);
        libusb_free_device_list (devs, 1);
}

/****************************************************************************/

void UsbAsync::open (DeviceHooks const &deviceHooks)
{
        AbstractInput::open (deviceHooks);

        if (dev = libusb_open_device_with_vid_pid (nullptr, VID, PID); dev == nullptr) {
                throw Exception ("Error finding USB device.");
        }

        // if (auto r = libusb_reset_device (dev); r < 0) {
        //         throw Exception ("Error resetting interface : "s + libusb_error_name (r));
        // }

        if (auto r = libusb_claim_interface (dev, 0); r < 0) {
                throw Exception ("Error claiming interface : "s + libusb_error_name (r));
        }

        if (auto r = libusb_set_interface_alt_setting (dev, 0, 0); r < 0) {
                throw Exception ("Error to libusb_set_interface_alt_setting: "s + libusb_error_name (r));
        }

        /*
         * TODO this is problematic, and IDK why. On python this works OK, though I didn't pass 0 or any
         * other argument. Whereas here I get warning in the dmesg, and LIBUSB_ERROR_BUSY in console.
         */
        // if (auto r = libusb_set_configuration (dev, 0); r < 0) {
        //         close();
        //         throw Exception ("Error to libusb_set_configuration: "s + libusb_error_name (r));
        // }
}

/****************************************************************************/

UsbAsync::~UsbAsync ()
{
        if (initialized) {
                libusb_hotplug_deregister_callback (nullptr, callback_handle);
                libusb_exit (nullptr);
                initialized = false;
        }
}

namespace {
        void sync_transfer_cb (struct libusb_transfer *transfer)
        {
                auto *completed = reinterpret_cast<int *> (transfer->user_data);
                *completed = 1;
        }

        int hotplug_callback (struct libusb_context *ctx, struct libusb_device *dev, libusb_hotplug_event event, void *user_data)
        {
                static libusb_device_handle *dev_handle = NULL;
                struct libusb_device_descriptor desc;
                int rc;

                (void)libusb_get_device_descriptor (dev, &desc);

                if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
                        rc = libusb_open (dev, &dev_handle);
                        if (LIBUSB_SUCCESS != rc) {
                                printf ("Could not open USB device\n");
                        }
                }
                else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                        if (dev_handle) {
                                libusb_close (dev_handle);
                                dev_handle = NULL;
                        }
                }
                else {
                        printf ("Unhandled event %d\n", event);
                }

                return 0;
        }
} // namespace

/**
 * Acquires ~~`wholeDataLenB` bytes of~~ data (blocking function) block by block (`singleTransferLenB`).
 * See src/feature/analyzer/flexio.cc in the firmware project for the other side of the connection.
 */
void UsbAsync::run ()
{
        libusb_transfer *transfer = libusb_alloc_transfer (0);

        if (transfer == nullptr) {
                throw Exception{"Libusb could not instantiate a new transfer using `libusb_alloc_transfer`"};
        }

        auto finally = gsl::finally ([this, transfer /* , session */] {
                libusb_free_transfer (transfer);
                running = false;
        });

        {
                /*
                 * For now we allocate the whole (rather big) memory buffer all at once. This is
                 * because we must avoid reallocations in the future.
                 */
                std::lock_guard lock{mutex};
                globalStart = high_resolution_clock::now ();
        }

        int completed{};
        size_t benchmarkB{};
        std::optional<high_resolution_clock::time_point> startPoint;
        timeval timeout{};
        bool started{};

        // TODO!
        {
                int rc = libusb_hotplug_register_callback (NULL, LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED | LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT, 0,
                                                           0x045a, 0x5005, LIBUSB_HOTPLUG_MATCH_ANY, hotplug_callback, NULL, &callback_handle);
                if (LIBUSB_SUCCESS != rc) {
                        throw Exception{"Error creating a hotplug callback: " + std::string{libusb_error_name (rc)}};
                }

                while (true) {
                        libusb_handle_events_completed (NULL, NULL);
                        // nanosleep (&(struct timespec){0, 10000000UL}, NULL);
                }
        }

        while (true) {
                if (!startPoint) {
                        startPoint = high_resolution_clock::now ();
                }

                Bytes singleTransfer (singleTransferLenB);
                libusb_fill_bulk_transfer (transfer, dev, common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (), sync_transfer_cb,
                                           &completed, common::usb::TIMEOUT_MS);

                completed = 0;

                if (auto r = libusb_submit_transfer (transfer); r < 0) {
                        throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::to_string (r)};
                }

                if (!started) {
                        start (/* params */);
                        started = true;
                }

                while (completed == 0) {
                        if (auto r = libusb_handle_events_timeout_completed (nullptr, &timeout, &completed); r < 0) {
                                if (r == LIBUSB_ERROR_INTERRUPTED) {
                                        std::println ("Interrupted");
                                        continue;
                                }

                                std::println ("libusb_handle_events failed: {}, cancelling transfer and retrying", libusb_error_name (r));
                                libusb_cancel_transfer (transfer);
                                continue;
                        }

                        if (transfer->dev_handle == nullptr) {
                                /* transfer completion after libusb_close() */
                                transfer->status = LIBUSB_TRANSFER_NO_DEVICE;
                                completed = 1;
                        }
                }

                if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                        throw Exception{"USB transfer status error Code: " + std::string{libusb_error_name (transfer->status)}};
                }

                if (size_t (transfer->actual_length) != singleTransferLenB) {
                        std::println ("Short: {}", transfer->actual_length);
                }

                singleTransfer.resize (transfer->actual_length);
                auto now = high_resolution_clock::now ();
                benchmarkB += singleTransfer.size ();

                {
                        std::lock_guard lock{mutex};
                        allTransferedB += singleTransfer.size ();
                }

                if (!singleTransfer.empty ()) {
                        auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *startPoint).count ())) * 8;
                        RawCompressedBlock rcd{mbps, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
                        deviceHooks ().queue->push (std::move (rcd));

                        {
                                std::lock_guard lock{mutex};
                                globalStop = high_resolution_clock::now (); // We want to be up to date, and skip possible time-out.
                        }
                }

                benchmarkB = 0;
                startPoint.reset ();

                if (request_ == Request::stop && singleTransfer.empty ()) {
                        break;
                }
        }
}

/****************************************************************************/

void UsbAsync::controlOut (std::vector<uint8_t> const &request)
{
        if (!initialized) {
                throw Exception{"`controlOut` called, but USB is not initialized."};
        }

        if (request.size () > MAX_CONTROL_PAYLOAD_SIZE) {
                throw Exception{"MAX_CONTROL_PAYLOAD_SIZE exceeded."};
        }

        // Configure sample rate and channels. This sends data.
        if (auto r = libusb_control_transfer (
                    dev, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_OUT),
                    VENDOR_CLASS_REQUEST, 1, 0, const_cast<uint8_t *> (std::data (request)), request.size (), TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }
}

/****************************************************************************/

std::vector<uint8_t> UsbAsync::controlIn (size_t len)
{
        if (!initialized) {
                throw Exception{"`controlIn` called, but USB is not initialized."};
        }

        std::vector<uint8_t> request (len);

        if (auto r = libusb_control_transfer (
                    dev, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_IN),
                    VENDOR_CLASS_REQUEST, 1, 0, request.data (), request.size (), TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }

        return request;
}

} // namespace logic
