/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "clockSignal.hh"
#include <cstdio>
#include <gsl/gsl>
#include <print>
#include <vector>

namespace logic::an {
using namespace std::chrono;
using namespace std::string_literals;

/****************************************************************************/

void ClockSignalCheck::run (data::SampleData const &samples)
{
        auto dmaBlocksNum = samples.digital.size () / dmaBlockLenB (); // We assume % is 0.

        for (size_t i = 0; i < dmaBlocksNum; ++i) {
                analyzeDataIntegrity (reinterpret_cast<uint32_t const *> (samples.digital.data () + i * dmaBlockLenB ()), dmaBlockLenB () / 4);
                ++devBlockNo;
        }
}

/****************************************************************************/

size_t ClockSignalCheck::analyzeDataIntegrity (uint32_t const *data, size_t sizeW)
{
        uint32_t const *data32 = reinterpret_cast<uint32_t const *> (data);
        wordInBlockNo = 0;

        for (size_t i = 0; i < sizeW; ++i) {
                uint32_t const &w = *(data32 + i);

                if (!decimationDiv || (i % *decimationDiv == decimationMod)) {
                        analyzeDataIntegrity (w);
                }

                ++wordInBlockNo;
        }

        return errorNum;
}

/****************************************************************************/

void ClockSignalCheck::analyzeDataIntegrity (uint32_t w)
{
        if (skip) { // TODO Assumes there's an edge in the first 32 bits. Fix.
                prevBit = bool (w & 1);
        }

        for (size_t j = 0; j < CHAR_BIT * sizeof (uint32_t); ++j) {
                bool bit = bool ((w >> j) & 1);

                // Skip initial stream of same values (first level). Most likely it's not complete.
                if (skip) {
                        if (bit != prevBit) {
                                skip = false;
                                ++period;
                        }
                }
                else {
                        if (bit != prevBit) {
                                if (!secondLevel) {
                                        secondLevel = true;
                                        ++period;
                                }
                                else {
                                        secondLevel = false;

                                        if (prevPeriod != std::nullopt && *prevPeriod != period) {
                                                if (!lastPeriodWasError) {
                                                        ++errorNum;
                                                        std::println ("{}:{}. {:032b} {:032b} {}:{}", devBlockNo, wordInBlockNo, w, prevWord,
                                                                      *prevPeriod, period);
                                                        lastPeriodWasError = true;
                                                }
                                        }
                                        else {
                                                prevPeriod = period;
                                                lastPeriodWasError = false;
                                        }

                                        period = 1;
                                }
                        }
                        else {
                                ++period;
                        }
                }

                prevBit = bit;
        }

        prevWord = w;
}

/****************************************************************************/

void ClockSignalCheck::stop () {}

} // namespace logic::an
