/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <atomic>
#include <chrono>
#include <climits>
#include <mutex>
#include <print>
module logic.processing;
import logic.analysis;

namespace logic {
using namespace std::chrono;
using namespace std::string_literals;

/**
 * @brief Perform the (debug) analysis using a strategy.
 * TODO consider removing.
 * @param rawData Data to analyze.
 * @param strategy Describes what to do.
 * @param discard Whether to discard the data or sotre it indefinitely in the RAM.
 */
struct Analyze {
public:
        void operator() (Queue<RawCompressedBlock> *queue, IBackend *backend, IAnalyzer *strategy, bool discard = false,
                         bool decompress = false);

        void kill () { running = false; }

private:
        std::atomic_bool running = true;
};

void Analyze::operator() (Queue<RawCompressedBlock> *queue, IBackend *backend, IAnalyzer *strategy, bool discard, bool decompress)
{
        // if (strategy != nullptr) {
        //         strategy->start ();
        // }

        // if (!discard) {
        //         queue->start ();
        // }

        // while (running) {
        //         std::unique_ptr<RawCompressedBlock> rcd = (discard) ? (queue->pop ()) : (queue->next ());

        //         if (!rcd) {
        //                 break;
        //         }

        //         RawData rd;

        //         if (decompress) {
        //                 rd = logic::decompress (*rcd);
        //                 rcd->clear ();
        //         }
        //         else {
        //                 rd = std::move (*rcd);
        //         }

        //         if (strategy != nullptr) {
        //                 strategy->runRaw (rd);
        //         }

        //         // TODO for now only digital data gets rearranged
        //         // TODO agnostic! vector of vector of bytes
        //         // std::vector<SampleBlock>  digitalChannels = rearrange (rd, params);
        //         // ChannelBlock digitalChannels{0, {}};
        //         std::vector<Bytes> digitalChannels;

        //         /*
        //          * Consider locking granularity. But even if it is too coarse, the move operation
        //          * below is so fast, that we aren't locked for too long.
        //          */
        //         backend->append (0, std::move (digitalChannels));

        //         // if (strategy != nullptr) {
        //         //         strategy->run (currentBlock->sampleData);
        //         // }
        // }

        // // TODO statictics
        // // double globalBps{};
        // // {
        // //         std::lock_guard lock{session->rawQueueMutex};
        // //         globalBps = double (session->receivedB ())
        // //                 / double (duration_cast<microseconds> (session->globalStop - session->globalStart).count ()) * CHAR_BIT;
        // // }

        // // std::println ("Overall: {:.2f} Mbps, ", globalBps);

        // if (strategy != nullptr) {
        //         strategy->stop ();
        // }
}

} // namespace logic