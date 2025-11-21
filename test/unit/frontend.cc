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
        backend.addGroup ({.channelsNumber = 4, .blockSizeB = 16384});

        DigitalFrontend frontend1{&backend};
        DigitalFrontend frontend2{&backend};

        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == false);

        backend.append (0, generateDemoDeviceBlock ());

        REQUIRE (frontend1.isNewData () == true);
        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == true);
        REQUIRE (frontend2.isNewData () == false);

        backend.append (0, generateDemoDeviceBlock ());

        REQUIRE (frontend1.isNewData () == true);
        REQUIRE (frontend1.isNewData () == false);
        REQUIRE (frontend2.isNewData () == true);
        REQUIRE (frontend2.isNewData () == false);
}

TEST_CASE ("size", "[frontend]")
{
        Backend backend;
        backend.addGroup ({.channelsNumber = 4, .blockSizeB = 16384});
        DigitalFrontend frontend{&backend};

        REQUIRE (frontend.size (0) == 0_Sn);

        backend.append (0, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == 8192_Sn);

        backend.append (0, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == SampleNum (2 * 8192));

        backend.append (0, generateDemoDeviceBlock ());
        REQUIRE (frontend.size (0) == SampleNum (3 * 8192));

        backend.clear ();
        REQUIRE (frontend.size (0) == 0_Sn);
}

/**
 *
 */
TEST_CASE ("square wave integration test usecase", "[frontend]")
{
        static constexpr auto GROUP = 0U;

        Backend backend;
        backend.addGroup ({.channelsNumber = 4, .blockSizeB = 16384});

        // Add some of these blocks.
        for (int i = 0; i < 10; ++i) {
                backend.append (GROUP, generateDemoDeviceBlock ());
        }

        DigitalFrontend frontend{&backend};

        auto extractChannel
                = std::views::transform ([] (auto const &block) { return block.channel (15); }) | std::views::join | std::ranges::to<Bytes> ();

        // Simulate how the GUI gets the data to render first 3 tiles of channel 15
        // Offsets and lengths taken from the GUI app itself
        auto tileAData = frontend.range (0, 0_SI, 750_Sn, 1, false) | extractChannel;
        auto tileBData = frontend.range (0, 750_SI, 750_Sn, 1, false) | extractChannel;
        auto tileCData = frontend.range (0, 1500_SI, 750_Sn, 1, false) | extractChannel;

        /// TODO! cross the blocks! i.e. past 8192 like:
        // auto tileCData = frontend.channel (0, 15, 8000, 750);
        // There was a problem with that. Fixed wo. UT.

        auto expectedData = std::views::repeat (std::vector<int>{1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
                                                                 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0})
                | std::views::join | std::views::take (3 * 750);

        // In C++26's STL we could concat tileA, B and C.
        for (auto [expected, actual] : std::views::zip (expectedData, OwningBitSpan{tileAData, 0, 750})) {
                REQUIRE (expected == actual);
        }

        for (auto [expected, actual] : std::views::zip (expectedData | std::views::drop (750), OwningBitSpan{tileBData, 750, 750})) {
                REQUIRE (expected == actual);
        }

        // TODO make OwningBitSpan a view and simply pipe the data to it as all other views do.
        for (auto [expected, actual] : std::views::zip (expectedData | std::views::drop (1500), OwningBitSpan{tileCData, 1500, 750})) {
                REQUIRE (expected == actual);
        }
}
