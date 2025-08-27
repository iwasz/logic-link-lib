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
#include <any>
#include <chrono>
#include <cstdio>
#include <cstring>
#include <deque>
#include <gsl/gsl>
#include <iterator>
#include <libusb.h>
#include <print>
#include <string>
#include <unordered_set>
#include <vector>
module logic;

namespace logic {
using namespace std::string_literals;
using namespace common::usb;
using namespace std::chrono;

/****************************************************************************/

UsbAsync::UsbAsync (EventQueue *eventQueue) : AbstractInput (eventQueue)
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
                                                   LIBUSB_HOTPLUG_MATCH_ANY, hotplugCallback, this, &callback_handle);

        if (LIBUSB_SUCCESS != rc) {
                throw Exception{"Error creating a hotplug callback: " + std::string{libusb_error_name (rc)}};
        }
}

/****************************************************************************/

std::any UsbAsync::open (std::any const &in)
{
        UsbDeviceInfo const &info = std::any_cast<UsbDeviceInfo const &> (in);
        // libusb_device_handle *dev{};

        if (dev = libusb_open_device_with_vid_pid (nullptr, info.vid, info.pid); dev == nullptr) {
                throw Exception ("Error finding USB device. VID: " + std::to_string (info.vid) + ", PID: " + std::to_string (info.pid));
        }

        // if (auto r = libusb_reset_device (dev); r < 0) {
        //         throw Exception ("Error resetting interface : "s + libusb_error_name (r));
        // }

        if (auto r = libusb_claim_interface (dev, info.claimInterface); r < 0) {
                throw Exception ("Error claiming interface: "s + std::to_string (info.claimInterface) + ", msg: "s + libusb_error_name (r));
        }

        if (auto r = libusb_set_interface_alt_setting (dev, info.interfaceNumber, info.alternateSetting); r < 0) {
                throw Exception ("Error libusb_set_interface_alt_setting: "s + libusb_error_name (r));
        }

        /*
         * TODO this is problematic, and IDK why. On python this works OK, though I didn't pass 0 or any
         * other argument. Whereas here I get warning in the dmesg, and LIBUSB_ERROR_BUSY in console.
         */
        // if (auto r = libusb_set_configuration (dev, 0); r < 0) {
        //         close();
        //         throw Exception ("Error to libusb_set_configuration: "s + libusb_error_name (r));
        // }

        return dev;
}

/****************************************************************************/

UsbAsync::~UsbAsync ()
{
        libusb_hotplug_deregister_callback (nullptr, callback_handle);
        libusb_exit (nullptr);
}

/****************************************************************************/

void UsbAsync::start (Session *session)
{
        // std::lock_guard lock{mutex};
        this->session = session;
        this->device = dynamic_cast<AbstractUsbDevice *> (session->device);

        if (this->device == nullptr) {
                throw Exception{"Use AbstractUsbDevice with UsbAsync."};
        }

        request_.store (Request::start);
}

/****************************************************************************/

void UsbAsync::transferCallback (struct libusb_transfer *transfer)
{
        auto *completed = reinterpret_cast<int *> (transfer->user_data);
        *completed = 1;
}

/*--------------------------------------------------------------------------*/

int UsbAsync::hotplugCallback (libusb_context * /* ctx */, libusb_device *dev, libusb_hotplug_event event, void *userData)
{
        libusb_device_handle *devHandle{};
        libusb_device_descriptor desc{};
        UsbAsync *that = reinterpret_cast<UsbAsync *> (userData);

        if (int rc = libusb_get_device_descriptor (dev, &desc); rc < 0) {
                that->eventQueue ()->addEvent<ErrorEvent> ("Could not get device's VID and PID."s);
                return 0;
        }

        if (LIBUSB_HOTPLUG_EVENT_DEVICE_ARRIVED == event) {
                if (int rc = libusb_open (dev, &devHandle); LIBUSB_SUCCESS != rc) {
                        that->eventQueue ()->addEvent<ErrorEvent> ("Could not open USB device"s);
                }
                else {
                        that->state_.store (State::connectedIdle);
                        that->eventQueue ()->setAlarm<UsbConnectedAlarm> (devHandle, &desc);
                }
        }
        else if (LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT == event) {
                if (devHandle != nullptr) {
                        libusb_close (devHandle);
                        devHandle = NULL;
                        that->state_.store (State::disconnected);
                        that->eventQueue ()->clearAlarm<UsbConnectedAlarm> (devHandle, &desc);
                }
        }

        return 0;
}

/*--------------------------------------------------------------------------*/

void UsbAsync::handleUsbEvents (int *completed, int timeoutMs, libusb_transfer *transfer)
{
        timeval timeout{};
        timeout.tv_sec = timeoutMs / 1000;
        timeout.tv_usec = (timeoutMs % 1000) * 1000;

        do {
                if (auto r = libusb_handle_events_timeout_completed (nullptr, &timeout, completed); r < 0) {
                        if (r == LIBUSB_ERROR_INTERRUPTED) {
                                // std::println ("USB Interrupted");
                                continue;
                        }

                        if (transfer != nullptr) {
                                // std::println ("libusb_handle_events failed: {}, cancelling transfer and retrying", libusb_error_name (r));
                                libusb_cancel_transfer (transfer);
                        }

                        continue;
                }

                if (transfer != nullptr && transfer->dev_handle == nullptr) {
                        /* transfer completion after libusb_close() */
                        transfer->status = LIBUSB_TRANSFER_NO_DEVICE;
                        *completed = 1;
                }
        } while (timeoutMs == 0 && *completed == 0);
}

/*--------------------------------------------------------------------------*/

void UsbAsync::handleUsbEventsCompleted (int *completed, libusb_transfer *transfer) { handleUsbEvents (completed, 0, transfer); }

/*--------------------------------------------------------------------------*/

void UsbAsync::handleUsbEventsTimeout (int timeoutMs, libusb_transfer *transfer)
{
        int completed = 0;
        handleUsbEvents (&completed, timeoutMs, transfer);
}

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

        auto finally = gsl::finally ([transfer] {
                libusb_free_transfer (transfer);
                // running = false;
        });

        {
                /*
                 * For now we allocate the whole (rather big) memory buffer all at once. This is
                 * because we must avoid reallocations in the future.
                 */
                // std::lock_guard lock{mutex};
                // globalStart = high_resolution_clock::now ();
        }

        int completed{};
        size_t benchmarkB{};
        std::optional<high_resolution_clock::time_point> startPoint;
        bool started{};

        while (true) {
                if (request_.load () == Request::kill) {
                        break;
                }

                if (state_.load () == State::disconnected) {
                        handleUsbEventsTimeout (1000, nullptr);
                }

                if (state_.load () == State::connectedIdle) {
                        handleUsbEventsTimeout (10, nullptr); // Still LIBUSB_HOTPLUG_EVENT_DEVICE_LEFT can be reporterd.

                        if (request_ == Request::start) {
                                request_ = Request::none;
                                state_ = State::transferring;
                                started = false;
                        }
                }

                if (state_.load () == State::transferring) {
                        Bytes singleTransfer (device->transferLen ());

                        if (!startPoint) { // TODO move to a benchmarking class
                                startPoint = high_resolution_clock::now ();
                        }

                        libusb_fill_bulk_transfer (transfer, dev, common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (),
                                                   transferCallback, &completed, common::usb::TIMEOUT_MS);

                        completed = 0;

                        if (auto r = libusb_submit_transfer (transfer); r < 0) {
                                throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::string{libusb_error_name (r)}};
                        }

                        std::print (".");

                        if (!started) {
                                device->onStart ();
                                std::println ("onStart");
                                started = true;
                        }

                        handleUsbEventsCompleted (&completed, transfer);

                        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                                throw Exception{"USB transfer status error Code: " + std::string{libusb_error_name (transfer->status)}};
                        }

                        if (size_t (transfer->actual_length) != device->transferLen ()) {
                                std::println ("Short: {}", transfer->actual_length);
                        }

                        singleTransfer.resize (transfer->actual_length);
                        auto now = high_resolution_clock::now ();
                        benchmarkB += singleTransfer.size ();

                        {
                                // std::lock_guard lock{mutex};
                                // allTransferedB += singleTransfer.size ();
                        }

                        if (!singleTransfer.empty ()) {
                                auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *startPoint).count ())) * 8;
                                RawCompressedBlock rcd{mbps, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
                                session->rawQueue.push (std::move (rcd));

                                {
                                        // std::lock_guard lock{mutex};
                                        // globalStop = high_resolution_clock::now (); // We want to be up to date, and skip possible time-out.
                                }
                        }

                        benchmarkB = 0;
                        startPoint.reset ();

                        if (request_ == Request::stop && singleTransfer.empty ()) {
                                device->onStop ();
                                request_ = Request::none;
                                state_ = State::connectedIdle;
                                session = nullptr;
                        }
                }
        }
}

/****************************************************************************/

void UsbAsync::controlOut (std::vector<uint8_t> const &request)
{
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
