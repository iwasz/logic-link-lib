/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "context.hh"
#include "deps/constants.hh"
#include "exception.hh"
#include "types.hh"
#include <chrono>
#include <cstdio>
#include <gsl/gsl>
#include <libusb.h>
#include <print>
#include <vector>

namespace logic::usb::sync {
using namespace std::chrono;
using namespace std::string_literals;

/****************************************************************************/

/**
 * Acquires a single transfer amount of data, which is more closely explained in
 * src/feature/analyzer/flexio.cc file in the firmware.
 */
data::Buffer acquireSingleTransfer (data::Session *rawData, size_t bytes)
{
        data::Buffer singleTransfer (bytes);

        int transferredB{};

        if (auto r = libusb_bulk_transfer (ctx ().dev, IN_EP, singleTransfer.data (), int (singleTransfer.size ()), &transferredB, TIMEOUT_MS);
            r < 0) {
                if (!rawData->stop) {
                        throw Exception ("Bulk transfer error: "s + libusb_error_name (r));
                }

                return {};
        }

        singleTransfer.resize (transferredB);
        return singleTransfer;
}

/**
 * Acquires ~~`wholeDataLenB` bytes of~~ data (blocking function) block by block (`singleTransferLenB`).
 * See src/feature/analyzer/flexio.cc in the firmware project for the other side of the connection.
 */
void acquire (data::Session *rawData, size_t singleTransferLenB)
{
        rawData->running = true;

        auto finally = gsl::finally ([rawData] {
                rawData->running = false;
                rawData->bufferCV.notify_one ();

                {
                        std::lock_guard lock{rawData->bufferMutex}; // TODO  ???? I must have removed something below, what was it?
                }
        });

        {
                /*
                 * For now we allocate the whole (rather big) memory buffer all at once. This is
                 * because we must avoid reallocations in the future.
                 */
                std::lock_guard lock{rawData->bufferMutex};
                rawData->globalStart = high_resolution_clock::now ();
        }

        size_t benchmarkB{};
        size_t deviceBufferStatusB{};
        std::optional<high_resolution_clock::time_point> start;
        size_t overrunsNo{}; // TODO is this used???

        while (true) {
                if (!start) {
                        start = high_resolution_clock::now ();
                }

                auto singleTransfer = acquireSingleTransfer (rawData, singleTransferLenB);
                auto now = high_resolution_clock::now ();
                benchmarkB += singleTransfer.size ();
                bool notify{};

                {
                        std::lock_guard lock{rawData->bufferMutex};
                        rawData->allTransferedB += singleTransfer.size ();

                        if (!singleTransfer.empty ()) {
                                auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *start).count ())) * 8;
                                rawData->queue.emplace_back (mbps, overrunsNo, std::make_unique<data::Buffer> (std::move (singleTransfer)));
                                notify = true;
                                rawData->globalStop = high_resolution_clock::now (); // We want to be up to date, and skip possible time-out.
                        }

                        overrunsNo = 0;
                        benchmarkB = 0;
                        start.reset ();
                }

                if (notify) {
                        rawData->bufferCV.notify_one ();
                }

                if (rawData->stop && singleTransfer.empty ()) {
                        break;
                }
        }
}

} // namespace logic::usb::sync