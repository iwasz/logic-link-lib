/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
#include <catch2/catch_test_macros.hpp>

import logic.core;

using namespace logic;

TEST_CASE ("popcount", "[popcount]")
{
        REQUIRE (dwnsample<8> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}) == Bytes{0x00});
        REQUIRE (dwnsample<8> (Bytes{0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x00}) == Bytes{0xaa});

        REQUIRE (dwnsample<16> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
                 == Bytes{0x00});
        REQUIRE (dwnsample<16> (Bytes{0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00, 0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00})
                 == Bytes{0xaa});

        REQUIRE (dwnsample<2> (Bytes{0x00, 0x00}) == Bytes{0x00});
        REQUIRE (dwnsample<2> (Bytes{0x00, 0x00, 0x00, 0x00}) == Bytes{0x00, 0x00});
        REQUIRE (dwnsample<2> (Bytes{0b11111111, 0b00000000}) == Bytes{0xf0});
        REQUIRE (dwnsample<2> (Bytes{0b11001100, 0b11001100}) == Bytes{0xaa});
        REQUIRE (dwnsample<2> (Bytes{0b11001100, 0b11001100, 0b11111111, 0b00000000}) == Bytes{0xaa, 0xf0});

        REQUIRE (dwnsample<4> (Bytes{0x00, 0x00, 0x00, 0x00}) == Bytes{0x00});
        REQUIRE (dwnsample<4> (Bytes{0b11111111, 0b11111111, 0b00000000, 0b00000000}) == Bytes{0xf0});
        REQUIRE (dwnsample<4> (Bytes{0b11110000, 0b11110000, 0b11110000, 0b11110000}) == Bytes{0xaa});
}
