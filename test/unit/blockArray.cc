/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <deque>
#include <ranges>
module logic.data;
import utils;

namespace logic {
struct BlockArrayUtHelper {
        static BlockArray::Container const &data (BlockArray const &ba) { return ba.levels[0].data_; }
        template <block_range Range> static Block makeBlock (Range const &range);
};

/**
 * Helper function used in the UTs.
 * Construct from range of blocks to effectively join them into one
 * and copy them into this.
 */
template <block_range Range> Block BlockArrayUtHelper::makeBlock (Range const &range)
{
        Block ret{sampleRate (range), bitsPerSample (range), {}, zoomOut (range)};

        if (range.empty ()) {
                return ret;
        }

        size_t byteSizeOfRange{};

        for (auto const &block : range) {
                // TODO works only (and tested only) for bitsPerSample == 1 or 8.
                byteSizeOfRange += block.channelLength ().get () / (CHAR_BIT / block.bitsPerSample ());
        }

        Block const &front = range.front ();
        ret.setFirstSampleNo (front.firstSampleNo ());
        ret.bitsPerSample_ = front.bitsPerSample ();

        ret.data_.resize (front.channelsNumber ());

        int chNo{};
        for (auto &ch : ret.data_) {
                ch.reserve (byteSizeOfRange);

                for (auto const &sourceBlock : range) {
                        std::ranges::copy (sourceBlock.data_.at (chNo), std::back_inserter (ch));
                }

                ++chNo;
        }

        return ret;
}

} // namespace logic

using namespace logic;

TEST_CASE ("BlockArray size", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        SECTION ("no multiply")
        {
                BlockArray cbs{16, 1_Sps, BITS_PER_SAMPLE};
                cbs.setBlockSizeB (2 * 8192);
                cbs.append (generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == 8192_Sn);
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.append (generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == SampleNum (2 * 8192));
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.append (generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == SampleNum (3 * 8192));
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.clear ();
                REQUIRE (cbs.channelLength () == 0_Sn);
                REQUIRE (cbs.channelsNumber () == 16);
        }

        SECTION ("multiply by 2")
        {
                BlockArray cbs{16, 1_Sps, BITS_PER_SAMPLE};
                cbs.setBlockSizeB (16);
                cbs.setBlockSizeMultiplier (2);
                cbs.append (getChannelBlockData (0));
                // REQUIRE (cbs.channelLength () == 32); TODO - should work

                cbs.append (getChannelBlockData (0));
                REQUIRE (cbs.channelLength () == SampleNum (2 * 32));

                cbs.append (getChannelBlockData (0));
                // REQUIRE (cbs.channelLength () == 3 * 32); // TODO should work!

                cbs.append (getChannelBlockData (0));
                REQUIRE (cbs.channelLength () == SampleNum (4 * 32));

                cbs.clear ();
                REQUIRE (cbs.channelLength () == 0_Sn);
                REQUIRE (cbs.channelsNumber () == 4);
        }
}

TEST_CASE ("Block from range", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (16);
        cbs.append (getChannelBlockData (0));
        cbs.append (getChannelBlockData (1));
        cbs.append (getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("full copy")
        {
                auto r = cbs.range (0_SI, 96_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0_SI);
                REQUIRE (copy.lastSampleNo () == 95_SI);
                REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("part copy")
        {
                auto r = cbs.range (32_SI, 96_SI);
                REQUIRE (r.begin () == std::next (data.begin (), 1));
                REQUIRE (r.end () == data.end ());

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 32_SI);
                REQUIRE (copy.lastSampleNo () == 95_SI);
                REQUIRE (copy.channel (0) == Bytes{0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("empty block added")
        {
                static constexpr auto BITS_PER_SAMPLE = 1U;
                BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
                cbs.append (std::vector<Bytes>{{}, {}});

                auto r = cbs.range (32_SI, 96_SI);
                REQUIRE (r.empty ());

                r = cbs.range (0_SI, 0_SI);
                REQUIRE (r.empty ());

                r = cbs.range (0_SI, 1_SI);
                REQUIRE (r.empty ());
        }

        SECTION ("with multiplier")
        {
                static constexpr auto BITS_PER_SAMPLE = 1U;
                BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
                cbs.setBlockSizeB (16);
                cbs.setBlockSizeMultiplier (2);
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));
                cbs.append (getChannelBlockData (3));
                auto const &data = BlockArrayUtHelper::data (cbs);

                auto r = cbs.range (0_SI, 128_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
                REQUIRE (std::ranges::distance (r) == 2);
                REQUIRE (bitsPerSample (r) == 1);
                REQUIRE (firstSampleNo (r) == 0_SI);
                REQUIRE (lastSampleNo (r) == 127_SI);

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0_SI);
                REQUIRE (copy.lastSampleNo () == 127_SI);
                REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xaa, 0xef, 0xdf, 0xcf});
        }
}

/**
 * One block with 4 channels. 4 samples long, 8 bits per sample.
 */
TEST_CASE ("one block 8bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (16);
        cbs.append (getChannelBlockData (0));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("0..4")
        {
                auto r = cbs.range (0_SI, 4_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..3")
        {
                auto r = cbs.range (0_SI, 3_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..2")
        {
                auto r = cbs.range (0_SI, 2_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..0")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0_SI, 0_SI);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("4..8")
        {
                auto r = cbs.range (4_SI, 8_SI);
                REQUIRE (std::empty (r));
        }
}

/**
 * Three blocks with 4 channels. Each is 4 samples long -> stream is 12 samples long.
 * 8 bits per sample.
 */
TEST_CASE ("many blocks 8bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (16);
        cbs.append (getChannelBlockData (0));
        cbs.append (getChannelBlockData (1));
        cbs.append (getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("1 block")
        {
                auto r = cbs.range (0_SI, 3_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 1));
        }

        SECTION ("2 blocks")
        {
                auto r = cbs.range (0_SI, 4_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));

                r = cbs.range (0_SI, 7_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));
        }

        SECTION ("all blocks")
        {
                auto r = cbs.range (0_SI, 8_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0_SI, 11_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0_SI, 12_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("last block")
        {
                auto r = cbs.range (8_SI, 100_SI);
                REQUIRE (r.begin () == std::next (data.begin (), 2));
                REQUIRE (r.end () == data.end ());
        }
}

/**
 * One block with 4 channels. 4 samples long, 1 bits per sample.
 */
TEST_CASE ("one block 1bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (16);
        cbs.append (getChannelBlockData (0));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("in range")
        {
                auto r = cbs.range (0_SI, 4_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (8_SI, 12_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (12_SI, 31_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0_SI, 32_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (8_SI, 100_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0_SI, 100_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("empty range")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0_SI, 0_SI);
                REQUIRE (std::ranges::distance (r) == 0);

                r = cbs.range (10_SI, 10_SI);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("out of range")
        {
                auto r = cbs.range (32_SI, 31_SI);
                REQUIRE (std::empty (r));
        }
}

/**
 * Three blocks with 4 channels. Each is 4 samples long -> stream is 12 samples long.
 * 1 bits per sample.
 */
TEST_CASE ("many blocks 1bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (16);
        cbs.append (getChannelBlockData (0));
        cbs.append (getChannelBlockData (1));
        cbs.append (getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("in range")
        {
                auto r = cbs.range (0_SI, 7_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 1));

                r = cbs.range (0_SI, 40_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));

                r = cbs.range (0_SI, 65_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0_SI, 100_SI);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (32_SI, 100_SI);
                REQUIRE (r.begin () == std::next (data.begin (), 1));
                REQUIRE (r.end () == data.end ());

                r = cbs.range (64_SI, 100_SI);
                REQUIRE (r.begin () == std::next (data.begin (), 2));
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("empty range")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0_SI, 0_SI);
                REQUIRE (std::ranges::distance (r) == 0);

                r = cbs.range (10_SI, 10_SI);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("out of range")
        {
                auto r = cbs.range (96_SI, 100_SI);
                REQUIRE (std::empty (r));
        }
}

/**
 * Simulates what logicLink does.
 */
TEST_CASE ("huge blocks 1bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto SAMPLES_PER_BLOCK = 16384;
        BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE};
        cbs.setBlockSizeB (2048);
        auto const &data = BlockArrayUtHelper::data (cbs);

        // Simulate hardware logic-link when set to 1 channel.
        for (int i = 0; i < 100; ++i) {
                cbs.append (generateDemoDeviceBlock (1, SAMPLES_PER_BLOCK));
                auto const &lastAdded = data.back ();
                REQUIRE (lastAdded.channelLength () == SampleNum (SAMPLES_PER_BLOCK));
                REQUIRE (lastAdded.firstSampleNo () == SampleIdx (SAMPLES_PER_BLOCK * i));
                REQUIRE (lastAdded.lastSampleNo () == SampleIdx (SAMPLES_PER_BLOCK * i + SAMPLES_PER_BLOCK - 1));
        }

        // Get me a block with last 100 samples.
        auto r = cbs.range (SampleIdx (cbs.channelLength ().get () - 100), SampleIdx (cbs.channelLength ().get () - 1));

        // This should be the last block right?
        REQUIRE (std::ranges::distance (r) == 1);
        REQUIRE (r.cbegin () == std::next (data.cbegin (), 99));
        REQUIRE (r.cbegin () == std::next (data.cend (), -1));
        REQUIRE (r.front ().firstSampleNo () == SampleIdx (SAMPLES_PER_BLOCK * 99));
        REQUIRE (r.front ().lastSampleNo () == SampleIdx (SAMPLES_PER_BLOCK * 100 - 1));
}

TEST_CASE ("ZoomOut", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        SECTION ("zoomOut 0")
        {
                BlockArray cbs{1, 1_Sps, BITS_PER_SAMPLE}; // Only 1 zoomLevel, so no zooming is possible.
                cbs.setBlockSizeB (16);
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                // However we try our luck. We should get 96 samples.
                auto r = cbs.range (0_SI, 96_SI, 48);
                REQUIRE (!r.empty ());

                Block copy = BlockArrayUtHelper::makeBlock (r);
                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0_SI);
                REQUIRE (copy.lastSampleNo () == 95_SI);
                REQUIRE (copy.channelLength () == 96_Sn);
                REQUIRE (copy.channelsNumber () == 4);
                REQUIRE (copy.channel (0).size () == 12);
        }

        SECTION ("zoomOut 2")
        {
                BlockArray cbs{4, 1_Sps, BITS_PER_SAMPLE, 2, 2}; // 4 channels, 1sps, 2 zoom levels : 1 and 0.5 (zoomOut==2)
                cbs.setBlockSizeB (16);
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                auto r = cbs.range (0_SI, 96_SI, 48);
                REQUIRE (!r.empty ());

                Block copy = BlockArrayUtHelper::makeBlock (r);
                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0_SI);
                REQUIRE (copy.lastSampleNo () == 95_SI);
                REQUIRE (copy.channelLength () == 96_Sn);
                REQUIRE (copy.channelsNumber () == 4);

                /*
                 * Even though channelLength == 96 which is 12 bytes, we get only 6 here.
                 *
                 */
                REQUIRE (copy.channel (0).size () == 6);
                // Do not check for channel equality. Tested "visually".
                // REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("zoomOut 4")
        {
                BlockArray cbs (4, 1_Sps, BITS_PER_SAMPLE, 3, 2); // 3 zoom levels : 1, 0.5 (zoomOut==2) and 0.25 (zoomOut==4)
                cbs.setBlockSizeB (16);
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                SECTION ("zoomOut 2, 3 levels available")
                {
                        auto r = cbs.range (0_SI, 96_SI, 2);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);

                        REQUIRE (bitsPerSample (r) == 1);
                        REQUIRE (copy.bitsPerSample () == 1);

                        REQUIRE (firstSampleNo (r) == 0_SI);
                        REQUIRE (copy.firstSampleNo () == 0_SI);

                        REQUIRE (lastSampleNo (r) == 95_SI);
                        REQUIRE (copy.lastSampleNo () == 95_SI);

                        REQUIRE (copy.channelLength () == 96_Sn);
                        REQUIRE (channelLength (r) == 96_Sn);

                        REQUIRE (channelsNumber (r) == 4);
                        REQUIRE (copy.channelsNumber () == 4);

                        REQUIRE (copy.channel (0).size () == 6);
                }

                SECTION ("zoomOut 4, 3 levels available")
                {
                        auto r = cbs.range (0_SI, 96_SI, 4);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.bitsPerSample () == 1);
                        REQUIRE (copy.firstSampleNo () == 0_SI);
                        REQUIRE (lastSampleNo (r) == 95_SI);
                        REQUIRE (copy.lastSampleNo () == 95_SI);
                        REQUIRE (copy.channelLength () == 96_Sn);
                        REQUIRE (copy.channelsNumber () == 4);
                        REQUIRE (copy.channel (0).size () == 3);
                }
        }

        SECTION ("zoomOut by 4")
        {
                BlockArray cbs (4, 1_Sps, BITS_PER_SAMPLE, 2, 4); // 2 zoom levels : 1 and 0.5 (zoomOut==2)
                cbs.setBlockSizeB (16);
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                SECTION ("lev 0")
                {
                        // First request a range from the level 0 (level 1 is too compressed and we would loose details).
                        auto r = cbs.range (0_SI, 96_SI, 2);
                        REQUIRE (!r.empty ());
                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.channel (0).size () == 12);
                }

                SECTION ("lev 1")
                {
                        auto r = cbs.range (0_SI, 96_SI, 4);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.bitsPerSample () == 1);
                        REQUIRE (copy.firstSampleNo () == 0_SI);
                        REQUIRE (copy.lastSampleNo () == 95_SI);
                        REQUIRE (copy.channelLength () == 96_Sn);
                        REQUIRE (copy.channelsNumber () == 4);
                        REQUIRE (copy.channel (0).size () == 3);
                }
        }
}
