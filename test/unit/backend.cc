/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "common/constants.hh"
#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <ranges>
#include <vector>
import logic;
import utils;

using namespace logic;
static constexpr auto BITS_PER_SAMPLE = 1U;
static constexpr auto DEFAULT_BLOKC_SIZE = 16U;

TEST_CASE ("size", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto GROUP = 0U;

        Backend backend;
        backend.addGroup ({.channelsNumber = 4, .maxZoomOutLevels = 1, .zoomOutPerLevel = 1, .blockSizeB = 16});

        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (0));
        REQUIRE (backend.channelLength (0) == 32_Sn);

        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (1));
        REQUIRE (backend.channelLength (0) == 64_Sn);

        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (2));
        REQUIRE (backend.channelLength (0) == 96_Sn);
}

TEST_CASE ("maxSampleRate", "[backend]")
{
        Backend backend;
        backend.addGroup ({.channelsNumber = 4, .sampleRate = 10_Sps});
        backend.addGroup ({.channelsNumber = 4, .sampleRate = 40_Sps});
        backend.addGroup ({.channelsNumber = 4, .sampleRate = 20_Sps});
        backend.addGroup ({.channelsNumber = 4, .sampleRate = 30_Sps});

        REQUIRE (backend.sampleRate () == 40_Sps);
}

TEST_CASE ("universal SampleNum and IDX", "[backend]")
{
        Backend backend;

        SECTION ("slower group added first")
        {
                auto const analogGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 10_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});
                REQUIRE (backend.sampleRate () == 10_Sps);

                for (int i = 0; i < 10; ++i) {
                        backend.append (analogGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (0UZ, DEFAULT_BLOKC_SIZE / 4) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());
                }

                auto l1 = backend.channelLength ();
                REQUIRE (l1 == SampleNum (40 * CHAR_BIT, 10_Sps));
                REQUIRE (l1.sampleRate () == 10_Sps);

                /*--------------------------------------------------------------------------*/

                auto const digitalGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 100_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});
                REQUIRE (backend.sampleRate () == 100_Sps);

                for (int i = 0; i < 100; ++i) {
                        backend.append (digitalGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (0UZ, DEFAULT_BLOKC_SIZE / 4) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());
                }

                auto l2 = backend.channelLength ();
                REQUIRE (l2 == SampleNum (400 * CHAR_BIT, 100_Sps));
                REQUIRE (l2.sampleRate () == 100_Sps);
        }

        SECTION ("faster group added first")
        {
                auto const digitalGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 100_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});
                REQUIRE (backend.sampleRate () == 100_Sps);

                for (int i = 0; i < 100; ++i) {
                        backend.append (digitalGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (0UZ, DEFAULT_BLOKC_SIZE / 4) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());
                }

                auto l2 = backend.channelLength ();
                REQUIRE (l2 == SampleNum (400 * CHAR_BIT, 100_Sps));
                REQUIRE (l2.sampleRate () == 100_Sps);

                /*--------------------------------------------------------------------------*/

                auto const analogGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 10_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});
                REQUIRE (backend.sampleRate () == 100_Sps);

                for (int i = 0; i < 10; ++i) {
                        backend.append (analogGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (0UZ, DEFAULT_BLOKC_SIZE / 4) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());
                }

                auto l1 = backend.channelLength ();
                REQUIRE (l1 == SampleNum (400 * CHAR_BIT, 100_Sps));
                REQUIRE (l1.sampleRate () == 100_Sps);

                auto l3 = backend.channelLength (analogGroup);
                REQUIRE (l3 == SampleNum (40 * CHAR_BIT, 10_Sps));
                // REQUIRE (l3.sampleRate () == 10);
        }
}

/**
 * Shows how to incremtally get data from all the groups simulataneously.
 */
TEST_CASE ("iterate the data", "[backend]")
{
        Backend backend;
        size_t aState{};
        size_t dState{};

        /*
         * ch0: 1,2,3,4
         * ch1: 1,2,3,4
         * ch2: 1,2,3,4
         * ch3: 1,2,3,4
         */
        auto acq = [&backend, &aState, &dState] (size_t analogGroup, size_t digitalGroup, size_t anum, size_t bnum) {
                for (int i = 0; i < anum; ++i) {
                        auto bytesPerCh = DEFAULT_BLOKC_SIZE / 4;

                        backend.append (analogGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (aState, aState + bytesPerCh) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());

                        aState += bytesPerCh;
                }

                for (int i = 0; i < bnum; ++i) {
                        auto bytesPerCh = DEFAULT_BLOKC_SIZE / 4;

                        backend.append (digitalGroup, BITS_PER_SAMPLE,
                                        std::views::repeat (std::views::iota (dState, dState + bytesPerCh) | std::ranges::to<Bytes> (), 4)
                                                | std::ranges::to<std::vector<Bytes>> ());

                        dState += bytesPerCh;
                }
        };

        auto ch0 = [] (size_t channelIdx, SampleIdx rangeBegin, SampleNum rangeLen) {
                auto beg = rangeBegin.get () / CHAR_BIT;
                auto len = rangeLen.get () / CHAR_BIT;

                return std::views::transform ([channelIdx] (auto &block) { return block.channel (channelIdx); }) | std::views::join
                        | std::views::drop (beg) | std::views::take (len) | std::ranges::to<Bytes> ();
        };

        SECTION ("first")
        {
                auto const analogGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 10_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});
                auto const digitalGroup = backend.addGroup ({.channelsNumber = 4, .sampleRate = 100_Sps, .blockSizeB = DEFAULT_BLOKC_SIZE});

                REQUIRE (backend.channelLength () == 0_Sn);

                SampleIdx rangeBegin{0, backend.sampleRate ()}; // Start from 0

                acq (analogGroup, digitalGroup, 1, 10);
                auto rangeLen = backend.channelLength () - rangeBegin;
                auto analogData = backend.range (analogGroup, rangeBegin, rangeLen);
                auto digitalData = backend.range (digitalGroup, rangeBegin, rangeLen);

                // Evaluate what has been returned.
                REQUIRE (firstSampleNo (analogData).get () == 0);
                REQUIRE (firstSampleNo (analogData).sampleRate () == 10_Sps);
                REQUIRE (lastSampleNo (analogData).get () == 4 * CHAR_BIT - 1);
                REQUIRE (lastSampleNo (analogData).sampleRate () == 10_Sps);

                REQUIRE (firstSampleNo (digitalData).get () == 0);
                REQUIRE (firstSampleNo (digitalData).sampleRate () == 100_Sps);
                REQUIRE (lastSampleNo (digitalData).get () == 40 * CHAR_BIT - 1); // 10 times more acquired
                REQUIRE (lastSampleNo (digitalData).sampleRate () == 100_Sps);

                auto asr = backend.sampleRate (analogGroup);
                auto dsr = backend.sampleRate (digitalGroup);

                // First 4 analog samples and 40 digital samples
                REQUIRE (std::ranges::equal (analogData | ch0 (0, resample (rangeBegin, asr), resample (rangeLen, asr)),
                                             std::views::iota (0, 4) | std::ranges::to<Bytes> ()));

                REQUIRE (std::ranges::equal (digitalData | ch0 (0, resample (rangeBegin, dsr), resample (rangeLen, dsr)),
                                             std::views::iota (0, 40) | std::ranges::to<Bytes> ()));

                rangeBegin += rangeLen;

                /*--------------------------------------------------------------------------*/

                acq (analogGroup, digitalGroup, 1, 10);
                rangeLen = backend.channelLength () - rangeBegin;
                analogData = backend.range (analogGroup, rangeBegin, rangeLen);
                digitalData = backend.range (digitalGroup, rangeBegin, rangeLen);

                REQUIRE (firstSampleNo (analogData).get () == 4 * CHAR_BIT);
                REQUIRE (firstSampleNo (analogData).sampleRate () == 10_Sps);
                REQUIRE (lastSampleNo (analogData).get () == 8 * CHAR_BIT - 1);
                REQUIRE (lastSampleNo (analogData).sampleRate () == 10_Sps);

                REQUIRE (firstSampleNo (digitalData).get () == 40 * CHAR_BIT);
                REQUIRE (firstSampleNo (digitalData).sampleRate () == 100_Sps);
                REQUIRE (lastSampleNo (digitalData).get () == 80 * CHAR_BIT - 1);
                REQUIRE (lastSampleNo (digitalData).sampleRate () == 100_Sps);

                // auto wholeCh0 = analogData | std::views::transform ([] (auto const &block) { return block.channel (0); }) | std::views::join
                //         | std::ranges::to<Bytes> ();

                // Next 4 analog samples and 40 digital samples
                REQUIRE ((analogData | ch0 (0, resample (rangeBegin, asr) - firstSampleNo (analogData), resample (rangeLen, asr))
                          | std::ranges::to<Bytes> ())
                         == (std::views::iota (4, 8) | std::ranges::to<Bytes> ()));

                REQUIRE (std::ranges::equal (
                        digitalData | ch0 (0, resample (rangeBegin, dsr) - firstSampleNo (digitalData), resample (rangeLen, dsr)),
                        std::views::iota (40, 80) | std::ranges::to<Bytes> ()));

                rangeBegin += rangeLen;
        }
}
