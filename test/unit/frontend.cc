/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
import utils;
#include <catch2/catch_test_macros.hpp>

using namespace logic;

TEST_CASE ("empty range", "[frontend]")
{
        BlockBackend backend;
        backend.configureGroup (0, 240'000'000); // Digital channels
        backend.append (0, getChannelBlockData (0));
        backend.append (0, getChannelBlockData (1));
        backend.append (0, getChannelBlockData (2));

        Frontend frontend{&backend};
        // auto range = frontend.range (0, 3);
        // REQUIRE (range.groups.size () == 1);
        // auto digitalGroup = range.groups.front ();
        // REQUIRE (digitalGroup.channelsNo () == 4);
        // REQUIRE (digitalGroup.size () == 4);
}
