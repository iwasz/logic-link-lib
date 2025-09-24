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
// import logic.data;
import utils;
import :range.span; // Hacks to speed up UT compilation (thus development).

using namespace logic;

/*
 * To test and figure out what actually should happen:
 * - Iterator from BlockArray{}
 * - Iterator from BlockArray{ Block {} }
 * - Iterator from BlockArray{ Block { {}, {} } }
 */

TEST_CASE ("Basic", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        auto r = blockArray.range (0, 1);

        using Container = std::deque<Block>;
        RangeBitSpan<uint8_t const, Container> rbs{r, 1, 0, 8};
        REQUIRE (vectorize (rbs) == std::vector<bool>{0, 0, 0, 0, 0, 0, 0, 1});
}
