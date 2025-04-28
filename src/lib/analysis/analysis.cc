/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "analysis.hh"
#include "decompress.hh"
#include "rearrange.hh"
#include <chrono>
#include <print>

namespace an {
using namespace std::chrono;
using namespace std::string_literals;

void analyze (common::acq::Params const &params, data::Session *session, an::ICheck *strategy, bool discard, bool decompress)
{
        if (strategy != nullptr) {
                strategy->start ();
        }

        int currentBlockIndex{}; // Block to be currently decompressed (if available).

        while (session->running) {
                data::Block *currentBlock{};
                data::RawCompressedData rcd;

                {
                        std::unique_lock lock{session->bufferMutex};

                        if (discard) {
                                session->bufferCV.wait (lock, [session] { return !session->running || !session->queue.empty (); });
                        }
                        else {
                                session->bufferCV.wait (lock, [session, &currentBlockIndex] {
                                        return !session->running || session->queue.size () >= currentBlockIndex + 1;
                                });
                        }

                        if (!session->running) {
                                break;
                        }

                        currentBlock = &session->queue.at (currentBlockIndex++);
                        rcd = std::move (currentBlock->rawCompressedData);
                }

                data::RawData rd;

                if (decompress) {
                        rd = an::decompress (rcd);
                        rcd.clear ();
                }
                else {
                        rd = std::move (rcd);
                }

                if (strategy != nullptr) {
                        strategy->run (rd);
                }

                data::SampleData sd = rearrange (rd, params);

                {
                        /*
                         * Consider locking granularity. But even if it is too coarse, the move operation
                         * below is so fast, that we aren't locked for too long.
                         */
                        std::lock_guard lock{session->bufferMutex};
                        currentBlock->sampleData = std::move (sd);
                }

                if (strategy != nullptr) {
                        strategy->run (currentBlock->sampleData);
                }
        }

        double globalBps{};
        {
                std::lock_guard lock{session->bufferMutex};
                globalBps = double (session->receivedB ())
                        / double (duration_cast<microseconds> (session->globalStop - session->globalStart).count ()) * CHAR_BIT;
        }

        std::println ("Overall: {:.2f} Mbps, ", globalBps);

        if (strategy != nullptr) {
                strategy->stop ();
        }
}

} // namespace an