/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "types.hh"

namespace logic::an {

/**
 * Analyze a (perfect) square signal down to single sample.
 * Check data integrity in a continuous data stream (may be invoked many times).
 * Returns number of errors detected. It looks for a pattern of 0es and the 1s
 * (or other way around) and determines its period. Then looks for another
 * stream of 0s and then 1s (or vice versa) and again calculates the period. If
 * these periods do not match an error counter is increased. So any square signal
 * will work provided that has steady (down to a single sample) frequency.
 * @arg size bytes
 */
class ClockSignalCheck : public AbstractCheck {
public:
        using AbstractCheck::AbstractCheck;

        void start () override
        {
                // lastBufferNo = {};
                devBlockNo = 0;
                // overrunsNo = 0;
        }

        void runRaw (data::RawData const & /* rd */) override {} // TODO this produces errors in UTs when is removed.
        void run (data::SampleData const &samples) override;
        void stop () override;

        /// Analyze only `mod`-th 32 bit word out of every `div` words in the stream.
        void setDecimation (int div, int mod)
        {
                decimationDiv = div;
                decimationMod = mod;
        }

// TODO refactor this. Maybe use the gmock and friends or some other mocks.
#ifndef UNIT_TEST
private:
#endif

        size_t analyzeDataIntegrity (uint32_t const *data, size_t sizeW);
        void analyzeDataIntegrity (uint32_t w);

        size_t devBlockNo{};
        size_t wordInBlockNo{};
        bool skip = true;   // Skip first (incomplete) level.
        bool secondLevel{}; // Every periodic square signal has first level and second level.
        bool prevBit{};
        std::optional<uint32_t> prevPeriod;
        uint32_t period{};
        uint32_t prevWord{};
        size_t errorNum{};
        bool lastPeriodWasError{}; // For skipping 2 consecutive errors
        std::optional<int> decimationDiv;
        std::optional<int> decimationMod;
        // std::vector<uint32_t> copy;
};

} // namespace logic::an
