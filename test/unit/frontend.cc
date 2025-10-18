/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <ranges>
#include <vector>

import logic;
import utils;

using namespace logic;

TEST_CASE ("new data", "[frontend]")
{
        Backend backend;
        backend.addGroup ({.channelsNumber = 4});

        DigitalFrontend frontend1{&backend};
        DigitalFrontend frontend2{&backend};

        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == false);

        backend.append (0, 1, generateDemoDeviceBlock ());

        REQUIRE (frontend1.isNewData () == true);
        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == true);
        REQUIRE (frontend2.isNewData () == false);

        backend.append (0, 1, generateDemoDeviceBlock ());

        REQUIRE (frontend1.isNewData () == true);
        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == true);
        REQUIRE (frontend2.isNewData () == false);
}

TEST_CASE ("size", "[frontend]")
{
        Backend backend;
        backend.addGroup ({.channelsNumber = 4});
        DigitalFrontend frontend{&backend};

        REQUIRE (frontend.size (0) == 0);

        backend.append (0, 1, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == 8192);

        backend.append (0, 1, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == 2 * 8192);

        backend.append (0, 1, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == 3 * 8192);

        backend.clear ();
        REQUIRE (frontend.size (0) == 0);
}

/**
 *
 */
TEST_CASE ("square wave integration test usecase", "[frontend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto GROUP = 0U;

        Backend backend;
        backend.addGroup ({.channelsNumber = 4});

        // Add some of these blocks.
        for (int i = 0; i < 10; ++i) {
                backend.append (GROUP, BITS_PER_SAMPLE, generateDemoDeviceBlock ());
        }

        DigitalFrontend frontend{&backend};

        // Simulate how the GUI gets the data to render first 3 tiles of channel 15
        // Offsets and lengths taken from the GUI app itself
        auto tileAData = frontend.channel (0, 15, 0, 750);
        auto tileBData = frontend.channel (0, 15, 750, 750);
        auto tileCData = frontend.channel (0, 15, 1500, 750);

        /// TODO! cross the blocks! i.e. past 8192 like:
        // auto tileCData = frontend.channel (0, 15, 8000, 750);
        // There was a problem with that. Fixed wo. UT.

        auto expectedData = std::views::repeat (std::vector<int>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
                | std::views::join | std::views::take (3 * 750);

        // In C++26's STL we could concat tileA, B and C.
        for (auto [expected, actual] : std::views::zip (expectedData, tileAData)) {
                REQUIRE (expected == actual);
        }

        for (auto [expected, actual] : std::views::zip (expectedData | std::views::drop (750), tileBData)) {
                REQUIRE (expected == actual);
        }

        for (auto [expected, actual] : std::views::zip (expectedData | std::views::drop (1500), tileCData)) {
                REQUIRE (expected == actual);
        }
}
