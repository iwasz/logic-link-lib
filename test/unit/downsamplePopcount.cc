/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
#include <bit>
#include <catch2/catch_test_macros.hpp>
#include <format>
#include <ranges>

import logic.core;

using namespace logic;

#if 0
namespace test {
template <std::integral auto factor, byte_collection Collection> struct Downsample {};

template <byte_collection Collection> struct Downsample<2, Collection> {
        static constexpr size_t BITS = 2;
        static Collection operator() (Collection const &in);
};

#if 0
template <byte_collection Collection> Collection Downsample<2, Collection>::operator() (Collection const &in)
{
        if (in.size () % BITS) {
                throw logic::Exception{"in.size () % BITS"};
        }

        Collection out;
        out.reserve (in.size () / BITS);

        size_t cntOut = BITS - 1;
        uint8_t outB{};

        auto last = [state = false] (int bits) mutable -> bool {
                if (bits == 1) {
                        bits = !state;
                        state = bits;
                }

                return bool (bits);
        };

        for (uint8_t b : in) {
                int bit7 = last (std::popcount (uint8_t (b & 0b11000000)));
                int bit6 = last (std::popcount (uint8_t (b & 0b00110000)));
                int bit5 = last (std::popcount (uint8_t (b & 0b00001100)));
                int bit4 = last (std::popcount (uint8_t (b & 0b00000011)));

                if (cntOut == 1) {
                        outB |= (bit7 << 7 | bit6 << 6 | bit5 << 5 | bit4 << 4);
                        --cntOut;
                }
                else {
                        outB |= (bit7 << 3 | bit6 << 2 | bit5 << 1 | bit4 << 0);
                        out.push_back (outB);
                        outB = 0;
                        cntOut = BITS - 1;
                }
        }

        return out;
}
#endif

template <byte_collection Collection> Collection Downsample<2, Collection>::operator() (Collection const &in)
{
        if (in.size () % BITS) {
                throw logic::Exception{std::format ("in.size () % {} != 0", BITS)};
        }

        Collection out;
        out.reserve (in.size () / BITS);

        auto last = [state = false] (int bits) mutable -> bool {
                if (bits == 1) {
                        bits = !state;
                        state = bits;
                }

                return bool (bits);
        };

        for (auto [a, b] : in | std::views::adjacent<2> | std::views::stride (2)) {
                int bit7 = last (std::popcount (uint8_t (a & 0b11000000)));
                int bit6 = last (std::popcount (uint8_t (a & 0b00110000)));
                int bit5 = last (std::popcount (uint8_t (a & 0b00001100)));
                int bit4 = last (std::popcount (uint8_t (a & 0b00000011)));

                int bit3 = last (std::popcount (uint8_t (b & 0b11000000)));
                int bit2 = last (std::popcount (uint8_t (b & 0b00110000)));
                int bit1 = last (std::popcount (uint8_t (b & 0b00001100)));
                int bit0 = last (std::popcount (uint8_t (b & 0b00000011)));

                out.push_back (bit7 << 7 | bit6 << 6 | bit5 << 5 | bit4 << 4 | bit3 << 3 | bit2 << 2 | bit1 << 1 | bit0 << 0);
        }

        return out;
}
} // namespace test
#endif

TEST_CASE ("popcount", "[popcount]")
{
        REQUIRE (downsample<8> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}) == Bytes{0x00});
        REQUIRE (downsample<8> (Bytes{0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x00}) == Bytes{0xaa});

        REQUIRE (downsample<16> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00})
                 == Bytes{0x00});
        REQUIRE (downsample<16> (Bytes{0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00, 0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00})
                 == Bytes{0xaa});

        REQUIRE (Downsample<2, Bytes>{}(Bytes{0x00, 0x00}) == Bytes{0x00});
        REQUIRE (Downsample<2, Bytes>{}(Bytes{0x00, 0x00, 0x00, 0x00}) == Bytes{0x00, 0x00});
        REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11111111, 0b00000000}) == Bytes{0xf0});
        REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11001100, 0b11001100}) == Bytes{0xaa});
        REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11001100, 0b11001100, 0b11111111, 0b00000000}) == Bytes{0xaa, 0xf0});
        REQUIRE (Downsample<2, Bytes>{}(Bytes{0b10101010, 0b10101010}) == Bytes{0b10101010});
        // REQUIRE (test::Downsample<2, Bytes>{}(Bytes{0b10101010, 0b10101010, 0b10101010}) == Bytes{0b10101010, 0b1010'0000});

        REQUIRE (downsample<4> (Bytes{0x00, 0x00, 0x00, 0x00}) == Bytes{0x00});
        REQUIRE (downsample<4> (Bytes{0b11111111, 0b11111111, 0b00000000, 0b00000000}) == Bytes{0xf0});
        REQUIRE (downsample<4> (Bytes{0b11110000, 0b11110000, 0b11110000, 0b11110000}) == Bytes{0xaa});
}
