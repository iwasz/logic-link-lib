/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
#include <deque>
module logic.data;
import utils;

using namespace logic;

TEST_CASE ("Block size", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        Block block{BITS_PER_SAMPLE, generateDemoDeviceBlock ()};
        REQUIRE (block.channelLength () == 8192);
        REQUIRE (block.channelsNumber () == 16);

        block.clear ();
        REQUIRE (block.channelLength () == 0);
        REQUIRE (block.channelsNumber () == 16);
}

TEST_CASE ("BlockArray size", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray cbs;
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

TEST_CASE ("Block from range", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        SECTION ("full copy")
        {
                auto r = cbs.range (0, 96);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                Block copy{r};

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 0);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channel (0) == Bytes{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("part copy")
        {
                auto r = cbs.range (32, 96);
                REQUIRE (r.begin () == std::next (cbs.data ().begin (), 1));
                REQUIRE (r.end () == cbs.data ().end ());

                Block copy{r};

                REQUIRE (copy.bitsPerSample () == 1);
                REQUIRE (copy.firstSampleNo () == 32);
                REQUIRE (copy.lastSampleNo () == 95);
                REQUIRE (copy.channel (0) == Bytes{0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb});
        }

        SECTION ("empty block added")
        {
                static constexpr auto BITS_PER_SAMPLE = 1U;
                BlockArray cbs;
                cbs.append (BITS_PER_SAMPLE, std::vector<Bytes>{{}, {}});

                auto r = cbs.range (32, 96);
                REQUIRE (r.empty ());

                r = cbs.range (0, 0);
                REQUIRE (r.empty ());

                r = cbs.range (0, 1);
                REQUIRE (r.empty ());
        }
}

/**
 * One block with 4 channels. 4 samples long, 8 bits per sample.
 */
TEST_CASE ("one block 8bit", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));

        SECTION ("0..4")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("0..3")
        {
                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("0..2")
        {
                auto r = cbs.range (0, 2);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
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
TEST_CASE ("many blocks 8bit", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        SECTION ("1 block")
        {
                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 1));
        }

        SECTION ("2 blocks")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 2));

                r = cbs.range (0, 7);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 2));
        }

        SECTION ("all blocks")
        {
                auto r = cbs.range (0, 8);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (0, 11);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (0, 12);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("last block")
        {
                auto r = cbs.range (8, 100);
                REQUIRE (r.begin () == std::next (cbs.data ().begin (), 2));
                REQUIRE (r.end () == cbs.data ().end ());
        }
}

/**
 * One block with 4 channels. 4 samples long, 1 bits per sample.
 */
TEST_CASE ("one block 1bit", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));

        SECTION ("in range")
        {
                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (8, 12);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (12, 31);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0, 32);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (8, 100);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (0, 100);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
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
TEST_CASE ("many blocks 1bit", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        SECTION ("in range")
        {
                auto r = cbs.range (0, 7);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 1));

                r = cbs.range (0, 40);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 2));

                r = cbs.range (0, 65);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("partially in range")
        {
                auto r = cbs.range (0, 100);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (32, 100);
                REQUIRE (r.begin () == std::next (cbs.data ().begin (), 1));
                REQUIRE (r.end () == cbs.data ().end ());

                r = cbs.range (64, 100);
                REQUIRE (r.begin () == std::next (cbs.data ().begin (), 2));
                REQUIRE (r.end () == cbs.data ().end ());
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
TEST_CASE ("huge blocks 1bit", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto SAMPLES_PER_BLOCK = 16384;
        BlockArray cbs;

        // Simulate hardware logic-link when set to 1 channel.
        for (int i = 0; i < 100; ++i) {
                cbs.append (BITS_PER_SAMPLE, generateDemoDeviceBlock (1, SAMPLES_PER_BLOCK));
                auto const &lastAdded = cbs.data ().back ();
                REQUIRE (lastAdded.channelLength () == SAMPLES_PER_BLOCK);
                REQUIRE (lastAdded.firstSampleNo () == SAMPLES_PER_BLOCK * i);
                REQUIRE (lastAdded.lastSampleNo () == SAMPLES_PER_BLOCK * i + SAMPLES_PER_BLOCK - 1);
        }

        // Get me a block with last 100 samples.
        auto r = cbs.range (cbs.channelLength () - 100, cbs.channelLength () - 1);

        // This should be the last block right?
        REQUIRE (std::ranges::distance (r) == 1);
        REQUIRE (r.cbegin () == std::next (cbs.data ().cbegin (), 99));
        REQUIRE (r.cbegin () == std::next (cbs.data ().cend (), -1));
        REQUIRE (r.front ().firstSampleNo () == SAMPLES_PER_BLOCK * 99);
        REQUIRE (r.front ().lastSampleNo () == SAMPLES_PER_BLOCK * 100 - 1);
}

/**
 *
 */
TEST_CASE ("clipBytes", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        SECTION ("empty backend")
        {
                BlockArray empty;
                auto stream = empty.clipBytes (0, 0);
                REQUIRE (stream.channelLength () == 0);

                stream = empty.clipBytes (0, 1000);
                REQUIRE (stream.channelLength () == 0);
        }

        SECTION ("empty range")
        {
                auto stream = blockArray.clipBytes (0, 0);
                REQUIRE (stream.channelLength () == 0);
        }

        SECTION ("first single byte")
        {
                auto stream = blockArray.clipBytes (0, 1);
                REQUIRE (stream.channelLength () == 1);
                auto const &ch0 = stream.channel (0);
                REQUIRE (ch0.size () == 1);
                REQUIRE (ch0.front () == 0x0);

                auto const &ch1 = stream.channel (1);
                REQUIRE (ch1.front () == 0x1);

                auto const &ch2 = stream.channel (2);
                REQUIRE (ch2.front () == 0x2);

                auto const &ch3 = stream.channel (3);
                REQUIRE (ch3.front () == 0x3);
        }

        SECTION ("second single byte")
        {
                auto stream = blockArray.clipBytes (1, 2);
                REQUIRE (stream.channelLength () == 1);
                REQUIRE (stream.data () == std::vector<Bytes>{{0x1}, {0x2}, {0x3}, {0x4}});
        }

        SECTION ("single byte from second block")
        {
                auto stream = blockArray.clipBytes (5, 6);
                REQUIRE (stream.channelLength () == 1);
                REQUIRE (stream.data () == std::vector<Bytes>{{0x5}, {0x6}, {0x7}, {0x8}});
        }

        SECTION ("last single byte")
        {
                auto stream = blockArray.clipBytes (11, 12);
                REQUIRE (stream.channelLength () == 1);
                REQUIRE (stream.data () == std::vector<Bytes>{{0xb}, {0xc}, {0xd}, {0xe}});
        }

        SECTION ("outside range")
        {
                auto stream = blockArray.clipBytes (12, 13);
                REQUIRE (stream.channelLength () == 0);
        }

        SECTION ("two bytes at the front")
        {
                auto stream = blockArray.clipBytes (0, 2);
                REQUIRE (stream.channelLength () == 2);
                REQUIRE (stream.data () == std::vector<Bytes>{{0x0, 0x1}, {0x1, 0x2}, {0x2, 0x3}, {0x3, 0x4}});
        }

        SECTION ("two bytes spanning two blocks")
        {
                auto stream = blockArray.clipBytes (3, 5);
                REQUIRE (stream.channelLength () == 2);
                REQUIRE (stream.data () == std::vector<Bytes>{{0x3, 0x4}, {0x4, 0x5}, {0x5, 0x6}, {0x6, 0x7}});
        }

        SECTION ("last two bytes")
        {
                auto stream = blockArray.clipBytes (10, 12);
                REQUIRE (stream.channelLength () == 2);
                REQUIRE (stream.data () == std::vector<Bytes>{{0xa, 0xb}, {0xb, 0xc}, {0xc, 0xd}, {0xd, 0xe}});
        }

        SECTION ("two bytes requested, only one returned from the back")
        {
                auto stream = blockArray.clipBytes (11, 13);
                REQUIRE (stream.channelLength () == 1);
                REQUIRE (stream.data () == std::vector<Bytes>{{0xb}, {0xc}, {0xd}, {0xe}});
        }

        SECTION ("more than a block")
        {
                auto stream = blockArray.clipBytes (0, 6);
                REQUIRE (stream.channelLength () == 6);
                REQUIRE (stream.data ()
                         == std::vector<Bytes>{{0x0, 0x1, 0x2, 0x3, 0x4, 0x5},
                                               {0x1, 0x2, 0x3, 0x4, 0x5, 0x6},
                                               {0x2, 0x3, 0x4, 0x5, 0x6, 0x7},
                                               {0x3, 0x4, 0x5, 0x6, 0x7, 0x8}});
        }

        SECTION ("more than a block")
        {
                auto stream = blockArray.clipBytes (6, 12);
                REQUIRE (stream.channelLength () == 6);
                REQUIRE (stream.data ()
                         == std::vector<Bytes>{{0x6, 0x7, 0x8, 0x9, 0xa, 0xb},
                                               {0x7, 0x8, 0x9, 0xa, 0xb, 0xc},
                                               {0x8, 0x9, 0xa, 0xb, 0xc, 0xd},
                                               {0x9, 0xa, 0xb, 0xc, 0xd, 0xe}});
        }

        SECTION ("full range")
        {
                auto stream = blockArray.clipBytes (0, 100);
                REQUIRE (stream.channelLength () == 12);
                REQUIRE (stream.data ()
                         == std::vector<Bytes>{{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb},
                                               {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc},
                                               {0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd},
                                               {0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe}});
        }
}

TEST_CASE ("size", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto GROUP = 0U;

        Backend backend;
        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (0));
        REQUIRE (backend.channelLength (0) == 32);

        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (1));
        REQUIRE (backend.channelLength (0) == 64);

        backend.append (GROUP, BITS_PER_SAMPLE, getChannelBlockData (2));
        REQUIRE (backend.channelLength (0) == 96);
}