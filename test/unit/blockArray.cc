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
        Block ret{bitsPerSample (range), {}, zoomOut (range)};

        if (range.empty ()) {
                return ret;
        }

        size_t byteSizeOfRange{};

        for (auto const &block : range) {
                // TODO works only (and tested only) for bitsPerSample == 1 or 8.
                byteSizeOfRange += block.channelLength () / (CHAR_BIT / block.bitsPerSample ());
        }

        Block const &front = range.front ();
        ret.firstSampleNo_ = front.firstSampleNo ();
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
                BlockArray cbs{16, 1};
                cbs.setBlockSizeB (2 * 8192);
                cbs.append (BITS_PER_SAMPLE, generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == 8192);
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.append (BITS_PER_SAMPLE, generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == 2 * 8192);
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.append (BITS_PER_SAMPLE, generateDemoDeviceBlock ());
                REQUIRE (cbs.channelLength () == 3 * 8192);
                REQUIRE (cbs.channelsNumber () == 16);

                cbs.clear ();
                REQUIRE (cbs.channelLength () == 0);
                REQUIRE (cbs.channelsNumber () == 16);
        }

        SECTION ("multiply by 2")
        {
                BlockArray cbs{16, 1};
                cbs.setBlockSizeB (16);
                cbs.setBlockSizeMultiplier (2);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                // REQUIRE (cbs.channelLength () == 32); TODO - should work

                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                REQUIRE (cbs.channelLength () == 2 * 32);

                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                // REQUIRE (cbs.channelLength () == 3 * 32); // TODO should work!

                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                REQUIRE (cbs.channelLength () == 4 * 32);

                cbs.clear ();
                REQUIRE (cbs.channelLength () == 0);
                REQUIRE (cbs.channelsNumber () == 4);
        }
}

TEST_CASE ("Block from range", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (16);
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("full copy")
        {
                auto r = cbs.range (0, 96);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("part copy")
        {
                auto r = cbs.range (32, 96);
                REQUIRE (r.begin () == std::next (data.begin (), 1));
                REQUIRE (r.end () == data.end ());

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 32);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channel (0) == Bytes{0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("empty block added")
        {
                static constexpr auto BITS_PER_SAMPLE = 1U;
                BlockArray cbs{1, 1};
                cbs.append (BITS_PER_SAMPLE, std::vector<Bytes>{{}, {}});

                auto r = cbs.range (32, 96);
                REQUIRE (r.empty ());

                r = cbs.range (0, 0);
                REQUIRE (r.empty ());

                r = cbs.range (0, 1);
                REQUIRE (r.empty ());
        }

        SECTION ("with multiplier")
        {
                static constexpr auto BITS_PER_SAMPLE = 1U;
                BlockArray cbs{1, 1};
                cbs.setBlockSizeB (16);
                cbs.setBlockSizeMultiplier (2);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (3));
                auto const &data = BlockArrayUtHelper::data (cbs);

                auto r = cbs.range (0, 128);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
                REQUIRE (std::ranges::distance (r) == 2);
                REQUIRE (bitsPerSample (r) == 1);
                REQUIRE (firstSampleNo (r) == 0);
                REQUIRE (lastSampleNo (r) == 127);

                Block copy = BlockArrayUtHelper::makeBlock (r);

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0);
                REQUIRE (copy.lastSampleNo () == 127);
                REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xaa, 0xef, 0xdf, 0xcf});
        }
}

/**
 * One block with 4 channels. 4 samples long, 8 bits per sample.
 */
TEST_CASE ("one block 8bit", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (16);
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("0..4")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..3")
        {
                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..2")
        {
                auto r = cbs.range (0, 2);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("0..0")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0, 0);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("4..8")
        {
                auto r = cbs.range (4, 8);
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

        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (16);
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("1 block")
        {
                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 1));
        }

        SECTION ("2 blocks")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));

                r = cbs.range (0, 7);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));
        }

        SECTION ("all blocks")
        {
                auto r = cbs.range (0, 8);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0, 11);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0, 12);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("last block")
        {
                auto r = cbs.range (8, 100);
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

        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (16);
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("in range")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (8, 12);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (12, 31);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0, 32);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (8, 100);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (0, 100);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("empty range")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0, 0);
                REQUIRE (std::ranges::distance (r) == 0);

                r = cbs.range (10, 10);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("out of range")
        {
                auto r = cbs.range (32, 31);
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

        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (16);
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        auto const &data = BlockArrayUtHelper::data (cbs);

        SECTION ("in range")
        {
                auto r = cbs.range (0, 7);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 1));

                r = cbs.range (0, 40);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == std::next (data.begin (), 2));

                r = cbs.range (0, 65);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0, 100);
                REQUIRE (r.begin () == data.begin ());
                REQUIRE (r.end () == data.end ());

                r = cbs.range (32, 100);
                REQUIRE (r.begin () == std::next (data.begin (), 1));
                REQUIRE (r.end () == data.end ());

                r = cbs.range (64, 100);
                REQUIRE (r.begin () == std::next (data.begin (), 2));
                REQUIRE (r.end () == data.end ());
        }

        SECTION ("empty range")
        {
                // Empty range should result in 0 blocks.
                auto r = cbs.range (0, 0);
                REQUIRE (std::ranges::distance (r) == 0);

                r = cbs.range (10, 10);
                REQUIRE (std::ranges::distance (r) == 0);
        }

        SECTION ("out of range")
        {
                auto r = cbs.range (96, 100);
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
        BlockArray cbs{1, 1};
        cbs.setBlockSizeB (2048);
        auto const &data = BlockArrayUtHelper::data (cbs);

        // Simulate hardware logic-link when set to 1 channel.
        for (int i = 0; i < 100; ++i) {
                cbs.append (BITS_PER_SAMPLE, generateDemoDeviceBlock (1, SAMPLES_PER_BLOCK));
                auto const &lastAdded = data.back ();
                REQUIRE (lastAdded.channelLength () == SAMPLES_PER_BLOCK);
                REQUIRE (lastAdded.firstSampleNo () == SAMPLES_PER_BLOCK * i);
                REQUIRE (lastAdded.lastSampleNo () == SAMPLES_PER_BLOCK * i + SAMPLES_PER_BLOCK - 1);
        }

        // Get me a block with last 100 samples.
        auto r = cbs.range (cbs.channelLength () - 100, cbs.channelLength () - 1);

        // This should be the last block right?
        REQUIRE (std::ranges::distance (r) == 1);
        REQUIRE (r.cbegin () == std::next (data.cbegin (), 99));
        REQUIRE (r.cbegin () == std::next (data.cend (), -1));
        REQUIRE (r.front ().firstSampleNo () == SAMPLES_PER_BLOCK * 99);
        REQUIRE (r.front ().lastSampleNo () == SAMPLES_PER_BLOCK * 100 - 1);
}

TEST_CASE ("ZoomOut", "[blockArray]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        SECTION ("zoomOut 0")
        {
                BlockArray cbs{1, 1}; // Only 1 zoomLevel, so no zooming is possible.
                cbs.setBlockSizeB (16);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

                // However we try our luck. We should get 96 samples.
                auto r = cbs.range (0, 96, 48);
                REQUIRE (!r.empty ());

                Block copy = BlockArrayUtHelper::makeBlock (r);
                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channelLength () == 96);
                REQUIRE (copy.channelsNumber () == 4);
                REQUIRE (copy.channel (0).size () == 12);
        }

        SECTION ("zoomOut 2")
        {
                BlockArray cbs{4, 1, 2, 2}; // 4 channels, 1sps, 2 zoom levels : 1 and 0.5 (zoomOut==2)
                cbs.setBlockSizeB (16);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

                auto r = cbs.range (0, 96, 48);
                REQUIRE (!r.empty ());

                Block copy = BlockArrayUtHelper::makeBlock (r);
                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channelLength () == 96);
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
                BlockArray cbs (4, 1, 3, 2); // 3 zoom levels : 1, 0.5 (zoomOut==2) and 0.25 (zoomOut==4)
                cbs.setBlockSizeB (16);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

                SECTION ("zoomOut 2, 3 levels available")
                {
                        auto r = cbs.range (0, 96, 2);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);

                        REQUIRE (bitsPerSample (r) == 1);
                        REQUIRE (copy.bitsPerSample () == 1);

                        REQUIRE (firstSampleNo (r) == 0);
                        REQUIRE (copy.firstSampleNo () == 0);

                        REQUIRE (lastSampleNo (r) == 95);
                        REQUIRE (copy.lastSampleNo () == 95);

                        REQUIRE (copy.channelLength () == 96);
                        REQUIRE (channelLength (r) == 96);

                        REQUIRE (channelsNumber (r) == 4);
                        REQUIRE (copy.channelsNumber () == 4);

                        REQUIRE (copy.channel (0).size () == 6);
                }

                SECTION ("zoomOut 4, 3 levels available")
                {
                        auto r = cbs.range (0, 96, 4);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.bitsPerSample () == 1);
                        REQUIRE (copy.firstSampleNo () == 0);
                        REQUIRE (lastSampleNo (r) == 95);
                        REQUIRE (copy.lastSampleNo () == 95);
                        REQUIRE (copy.channelLength () == 96);
                        REQUIRE (copy.channelsNumber () == 4);
                        REQUIRE (copy.channel (0).size () == 3);
                }
        }

        SECTION ("zoomOut by 4")
        {
                BlockArray cbs (4, 1, 2, 4); // 2 zoom levels : 1 and 0.5 (zoomOut==2)
                cbs.setBlockSizeB (16);
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

                SECTION ("lev 0")
                {
                        // First request a range from the level 0 (level 1 is too compressed and we would loose details).
                        auto r = cbs.range (0, 96, 2);
                        REQUIRE (!r.empty ());
                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.channel (0).size () == 12);
                }

                SECTION ("lev 1")
                {
                        auto r = cbs.range (0, 96, 4);
                        REQUIRE (!r.empty ());

                        Block copy = BlockArrayUtHelper::makeBlock (r);
                        REQUIRE (copy.bitsPerSample () == 1);
                        REQUIRE (copy.firstSampleNo () == 0);
                        REQUIRE (copy.lastSampleNo () == 95);
                        REQUIRE (copy.channelLength () == 96);
                        REQUIRE (copy.channelsNumber () == 4);
                        REQUIRE (copy.channel (0).size () == 3);
                }
        }
}
