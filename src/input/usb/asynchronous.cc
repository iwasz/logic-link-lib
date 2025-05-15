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
#include "exception.hh"
#include <chrono>
#include <cstdio>
#include <gsl/gsl>
#include <libusb.h>
#include <print>

module logic.input.usb;
import logic.data;

namespace logic::usb::async {
using namespace std::chrono;
using namespace std::string_literals;

/**
 * Acquires a single transfer amount of data, which is more closely explained in
 * src/feature/analyzer/flexio.cc file in the firmware.
 */
data::Bytes acquireSingleTransfer (data::Session *rawData, size_t bytes)
{
        data::Bytes singleTransfer (bytes);

        int transferredB{};

        if (auto r = libusb_bulk_transfer (ctx ().dev, common::usb::IN_EP, singleTransfer.data (), int (singleTransfer.size ()), &transferredB,
                                           common::usb::TIMEOUT_MS);
            r < 0) {
                if (!rawData->stop) {
                        throw Exception ("Bulk transfer error: "s + libusb_error_name (r));
                }

                return {};
        }

        singleTransfer.resize (transferredB);
        return singleTransfer;
}

/****************************************************************************/

namespace {
        void sync_transfer_cb (struct libusb_transfer *transfer)
        {
                auto *completed = reinterpret_cast<int *> (transfer->user_data);
                *completed = 1;
        }
} // namespace

/**
 * Acquires ~~`wholeDataLenB` bytes of~~ data (blocking function) block by block (`singleTransferLenB`).
 * See src/feature/analyzer/flexio.cc in the firmware project for the other side of the connection.
 */
void acquire (common::acq::Params const & /* params */, data::Session *session, size_t singleTransferLenB)
{
        // ::getMeaning (0);
        libusb_transfer *transfer = libusb_alloc_transfer (0);

        if (transfer == nullptr) {
                throw Exception{"Libusb could not instantiate a new transfer using `libusb_alloc_transfer`"};
        }

        auto finally = gsl::finally ([transfer, session] {
                libusb_free_transfer (transfer);
                session->running = false;
                session->bufferCV.notify_all ();
        });

        {
                /*
                 * For now we allocate the whole (rather big) memory buffer all at once. This is
                 * because we must avoid reallocations in the future.
                 */
                std::lock_guard lock{session->rawQueueMutex};
                session->globalStart = high_resolution_clock::now ();
        }

        int completed{};
        size_t benchmarkB{};
        std::optional<high_resolution_clock::time_point> start;
        timeval timeout{};
        bool started{};

        while (true) {
                if (!start) {
                        start = high_resolution_clock::now ();
                }

                data::Bytes singleTransfer (singleTransferLenB);
                libusb_fill_bulk_transfer (transfer, ctx ().dev, common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (),
                                           sync_transfer_cb, &completed, common::usb::TIMEOUT_MS);

                completed = 0;

                if (auto r = libusb_submit_transfer (transfer); r < 0) {
                        throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::to_string (r)};
                }

                if (!started) {
                        usb::start (/* params */);
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
                bool notify{};

                {
                        std::lock_guard lock{session->rawQueueMutex};
                        session->allTransferedB += singleTransfer.size ();

                        if (!singleTransfer.empty ()) {
                                auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *start).count ())) * 8;
                                // TODO resize blocks (if needed)
                                data::RawCompressedBlock rcd{mbps, 0, std::move (singleTransfer)};
                                session->rawQueue.emplace_back (std::move (rcd));
                                notify = true;
                                session->globalStop = high_resolution_clock::now (); // We want to be up to date, and skip possible time-out.
                        }

                        benchmarkB = 0;
                        start.reset ();
                }

                if (notify) {
                        session->bufferCV.notify_all ();
                }

                if (session->stop && singleTransfer.empty ()) {
                        break;
                }
        }
}

} // namespace logic::usb::async
