/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
module logic;
import utils;

using namespace logic;

TEST_CASE ("one block", "[backend]")
{
        SECTION ("0..4")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));

                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("0..3")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));

                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("0..2")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));

                auto r = cbs.range (0, 2);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("0..0")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));

                auto r = cbs.range (0, 0);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == cbs.data ().end ());
        }

        SECTION ("4..8")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));

                auto r = cbs.range (4, 8);
                REQUIRE (std::empty (r));
        }
}

TEST_CASE ("many blocks", "[backend]")
{
        SECTION ("1 block")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                auto r = cbs.range (0, 3);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 1));
        }

        SECTION ("2 blocks")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

                auto r = cbs.range (0, 4);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 2));

                r = cbs.range (0, 7);
                REQUIRE (r.begin () == cbs.data ().begin ());
                REQUIRE (r.end () == std::next (cbs.data ().begin (), 2));
        }

        SECTION ("all blocks")
        {
                ChannelBlockStream cbs;
                cbs.append (getChannelBlockData (0));
                cbs.append (getChannelBlockData (1));
                cbs.append (getChannelBlockData (2));

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
}

TEST_CASE ("data", "[backend]")
{
        BlockBackend backend;
        backend.configureGroup (0, 240'000'000);
        backend.append (0, getChannelBlockData (0));

        // auto chBlockSpan = backend.range (0, 0, 4);
        // REQUIRE (chBlockSpan.size () == 1);
}
