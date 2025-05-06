/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <atomic>
#include <chrono>
#include <condition_variable>
#include <deque>
#include <mutex>
#include <vector>

namespace logic::data {
using Buffer = std::vector<uint8_t>;

/**
 * The data as received from the device. Possibly compressed with LZ4.
 */
struct RawCompressedData {
        double bps{};
        size_t overrunsNo{};
        Buffer buffer;

        void clear () { buffer.clear (); }
};

/**
 * The same data but decompressed. In case of no compression on the device,
 * both RawCompressedData and RawData will contain the same data.
 */
using RawData = RawCompressedData;

/**
 * Data decoded from the RawData to the sample data organized by channels
 * (both digital and analog). This is the data that can be finally displayed
 * or consumed by the analyzers.
 */
struct SampleData {

        std::vector<Buffer> digital;
        std::vector<Buffer> analog;
};

/**
 * Analyzer output format. This data accompanies the sample data and provides
 * additional, decoded information like binary->hex etc.
 */
struct AugumentedData {};

struct Block {
        // Block could have a sampleNumber and time_point
        RawCompressedData rawCompressedData;
        SampleData sampleData;
        std::vector<std::unique_ptr<AugumentedData>> augumentedData;
        // std::mutex mutex; // sizeof == ~40B Do I really need this?
};

using BufferQueue = std::deque<Block>;

/**
 * Data as received directly from the device.
 */
struct Session {
        BufferQueue queue;
        std::mutex bufferMutex;

        std::chrono::high_resolution_clock::time_point globalStart;
        std::chrono::high_resolution_clock::time_point globalStop;
        std::condition_variable bufferCV;
        std::atomic_bool running;
        std::atomic_bool stop; /// Send stop request to the device.
        size_t allTransferedB{};

        /// Number of bytes received so far irrespective of the buffer size.
        size_t receivedB () const { return allTransferedB; }
};

} // namespace logic::data