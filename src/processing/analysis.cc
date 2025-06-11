/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <chrono>
#include <climits>
#include <mutex>
#include <print>
module logic;
import :processing;
import :data.backend;

namespace logic::an {
using namespace std::chrono;
using namespace std::string_literals;

void analyze (common::acq::Params const &params, data::Session *session, an::IAnalyzer *strategy, bool discard, bool decompress)
{
        if (strategy != nullptr) {
                strategy->start ();
        }

        size_t currentBlockIndex{}; // Block to be currently decompressed (if available).

        while (session->running) {
                data::RawCompressedBlock rcd;

                {
                        std::unique_lock lock{session->rawQueueMutex};

                        if (discard) {
                                session->bufferCV.wait (lock, [session] { return !session->running || !session->rawQueue.empty (); });
                        }
                        else {
                                session->bufferCV.wait (lock, [session, &currentBlockIndex] {
                                        return !session->running || session->rawQueue.size () >= currentBlockIndex + 1;
                                });
                        }

                        if (!session->running) {
                                break;
                        }

                        rcd = std::move (session->rawQueue.at (currentBlockIndex++));
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
                        strategy->runRaw (rd);
                }

                // TODO for now only digital data gets rearranged
                // TODO agnostic! vector of vector of bytes
                // std::vector<data::SampleBlock>  digitalChannels = rearrange (rd, params);
                // data::ChannelBlock digitalChannels{0, {}};
                std::vector<data::Bytes> digitalChannels;

                {
                        /*
                         * Consider locking granularity. But even if it is too coarse, the move operation
                         * below is so fast, that we aren't locked for too long.
                         */
                        std::lock_guard lock{session->sampleMutex};
                        // currentBlock->sampleData = std::move (digitalChannels);
                        // Group 0 means digital channels
                        // session->groups.at (0).appendChannels (std::move (digitalChannels));
                        session->backend->append (0, std::move (digitalChannels));
                }

                // if (strategy != nullptr) {
                //         strategy->run (currentBlock->sampleData);
                // }
        }

        double globalBps{};
        {
                std::lock_guard lock{session->rawQueueMutex};
                globalBps = double (session->receivedB ())
                        / double (duration_cast<microseconds> (session->globalStop - session->globalStart).count ()) * CHAR_BIT;
        }

        std::println ("Overall: {:.2f} Mbps, ", globalBps);

        if (strategy != nullptr) {
                strategy->stop ();
        }
}

} // namespace logic::an