/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
module logic.data;
import utils;

using namespace logic;

/**
 * One block with 4 channels. 4 samples long.
 */
TEST_CASE ("one block", "[backend]")
{
        ChannelBlockStream cbs;
        cbs.append (getChannelBlockData (0));

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
                REQUIRE (r.size () == 0);
        }

        SECTION ("4..8")
        {
                auto r = cbs.range (4, 8);
                REQUIRE (std::empty (r));
        }
}

/**
 * Three blocks with 4 channels. Each is 4 samples long -> stream is 12 samples long.
 */
TEST_CASE ("many blocks", "[backend]")
{
        ChannelBlockStream cbs;
        cbs.append (getChannelBlockData (0));
        cbs.append (getChannelBlockData (1));
        cbs.append (getChannelBlockData (2));

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
 *
 */
TEST_CASE ("data", "[backend]")
{
        BlockBackend backend;
        backend.configureGroup (0, 240'000'000);
        backend.append (0, getChannelBlockData (0));
        backend.append (0, getChannelBlockData (1));
        backend.append (0, getChannelBlockData (2));

        SECTION ("empty range")
        {
                auto stream = backend.range (0, 0, 0);
                REQUIRE (stream.channelSize () == 0);
        }

        SECTION ("first single byte")
        {
                auto stream = backend.range (0, 0, 1);
                REQUIRE (stream.channelSize () == 1);
                auto const &ch0 = stream.data.at (0);
                REQUIRE (ch0.size () == 1);
                REQUIRE (ch0.front () == 0x0);

                auto const &ch1 = stream.data.at (1);
                REQUIRE (ch1.front () == 0x1);

                auto const &ch2 = stream.data.at (2);
                REQUIRE (ch2.front () == 0x2);

                auto const &ch3 = stream.data.at (3);
                REQUIRE (ch3.front () == 0x3);
        }

        SECTION ("second single byte")
        {
                auto stream = backend.range (0, 1, 2);
                REQUIRE (stream.channelSize () == 1);
                REQUIRE (stream.data == std::vector<Bytes>{{0x1}, {0x2}, {0x3}, {0x4}});
        }

        SECTION ("single byte from second block")
        {
                auto stream = backend.range (0, 5, 6);
                REQUIRE (stream.channelSize () == 1);
                REQUIRE (stream.data == std::vector<Bytes>{{0x5}, {0x6}, {0x7}, {0x8}});
        }

        SECTION ("last single byte")
        {
                auto stream = backend.range (0, 11, 12);
                REQUIRE (stream.channelSize () == 1);
                REQUIRE (stream.data == std::vector<Bytes>{{0xb}, {0xc}, {0xd}, {0xe}});
        }

        SECTION ("outside range")
        {
                auto stream = backend.range (0, 12, 13);
                REQUIRE (stream.channelSize () == 0);
        }

        SECTION ("two bytes at the front")
        {
                auto stream = backend.range (0, 0, 2);
                REQUIRE (stream.channelSize () == 2);
                REQUIRE (stream.data == std::vector<Bytes>{{0x0, 0x1}, {0x1, 0x2}, {0x2, 0x3}, {0x3, 0x4}});
        }

        SECTION ("two bytes spanning two blocks")
        {
                auto stream = backend.range (0, 3, 5);
                REQUIRE (stream.channelSize () == 2);
                REQUIRE (stream.data == std::vector<Bytes>{{0x3, 0x4}, {0x4, 0x5}, {0x5, 0x6}, {0x6, 0x7}});
        }

        SECTION ("last two bytes")
        {
                auto stream = backend.range (0, 10, 12);
                REQUIRE (stream.channelSize () == 2);
                REQUIRE (stream.data == std::vector<Bytes>{{0xa, 0xb}, {0xb, 0xc}, {0xc, 0xd}, {0xd, 0xe}});
        }

        SECTION ("two bytes requested, only one returned from the back")
        {
                auto stream = backend.range (0, 11, 13);
                REQUIRE (stream.channelSize () == 1);
                REQUIRE (stream.data == std::vector<Bytes>{{0xb}, {0xc}, {0xd}, {0xe}});
        }

        SECTION ("more than a block")
        {
                auto stream = backend.range (0, 0, 6);
                REQUIRE (stream.channelSize () == 6);
                REQUIRE (stream.data
                         == std::vector<Bytes>{{0x0, 0x1, 0x2, 0x3, 0x4, 0x5},
                                               {0x1, 0x2, 0x3, 0x4, 0x5, 0x6},
                                               {0x2, 0x3, 0x4, 0x5, 0x6, 0x7},
                                               {0x3, 0x4, 0x5, 0x6, 0x7, 0x8}});
        }

        SECTION ("more than a block")
        {
                auto stream = backend.range (0, 6, 12);
                REQUIRE (stream.channelSize () == 6);
                REQUIRE (stream.data
                         == std::vector<Bytes>{{0x6, 0x7, 0x8, 0x9, 0xa, 0xb},
                                               {0x7, 0x8, 0x9, 0xa, 0xb, 0xc},
                                               {0x8, 0x9, 0xa, 0xb, 0xc, 0xd},
                                               {0x9, 0xa, 0xb, 0xc, 0xd, 0xe}});
        }

        SECTION ("full range")
        {
                auto stream = backend.range (0, 0, 100);
                REQUIRE (stream.channelSize () == 12);
                REQUIRE (stream.data
                         == std::vector<Bytes>{{0x0, 0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb},
                                               {0x1, 0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc},
                                               {0x2, 0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd},
                                               {0x3, 0x4, 0x5, 0x6, 0x7, 0x8, 0x9, 0xa, 0xb, 0xc, 0xd, 0xe}});
        }
}
