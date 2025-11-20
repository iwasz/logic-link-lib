/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
import logic;
import utils;

using namespace logic;

TEST_CASE ("Block size", "[block]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        Block block{100_Sps, BITS_PER_SAMPLE, generateDemoDeviceBlock ()};
        REQUIRE (block.channelLength () == 8192_Sn * 100_Sps);
        REQUIRE (block.channelsNumber () == 16);

        block.clear ();
        REQUIRE (block.channelLength () == 0_Sn * 100_Sps);
        REQUIRE (block.channelsNumber () == 16);
}
