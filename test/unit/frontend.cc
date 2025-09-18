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

TEST_CASE ("square wave integration test usecase", "[frontend]")
{
        static constexpr auto BITS_PER_SAMPLE = 8U;
        static constexpr auto GROUP = 0U;

        Backend backend;
        backend.configureGroup (0, 240'000'000); // Digital channels

        // Prepare generators suitable for continuous work. DemoDevice does the same.
        // auto generators = std::views::iota (0, 16) | std::views::transform ([] (int i) { return Square (i); }) | std::ranges::to<std::vector>
        // ();
        std::vector<Square> generators (16);

        // Simulate a block from DemoDevice (16 channels square signals).
        auto generateDemoDeviceBlock = [&generators] {
                std::vector<Bytes> channels (16);

                size_t cnt{};
                for (auto &gen : generators) {
                        channels.at (cnt++) = gen (cnt + 1, cnt + 1, 8192);
                }

                return channels;
        };

        // Add some of these blocks.
        for (int i = 0; i < 10; ++i) {
                backend.append (GROUP, BITS_PER_SAMPLE, generateDemoDeviceBlock ());
        }

        Frontend frontend{&backend};

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
