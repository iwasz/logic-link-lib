/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
#include <catch2/catch_test_macros.hpp>
#include <print>
#include <random>
#include <ranges>
using namespace logic;

void lutD2_f ();

TEST_CASE ("popcount", "[popcount]")
{
        SECTION ("Downsample 2,4,8,16")
        {
                bool s{};
                REQUIRE (downsample<8> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, &s) == Bytes{0x00});
                REQUIRE (downsample<8> (Bytes{0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x11, 0xaa, 0x00}, &s) == Bytes{0xaa});

                REQUIRE (downsample<16> (Bytes{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00},
                                         &s)
                         == Bytes{0x00});
                REQUIRE (downsample<16> (Bytes{0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00, 0xaa, 0xaa, 0x11, 0x11, 0xaa, 0xaa, 0x11, 0x00},
                                         &s)
                         == Bytes{0xaa});

                REQUIRE (Downsample<2, Bytes>{}(Bytes{0x00, 0x00}, &s) == Bytes{0x00});
                REQUIRE (Downsample<2, Bytes>{}(Bytes{0x00, 0x00, 0x00, 0x00}, &s) == Bytes{0x00, 0x00});
                REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11111111, 0b00000000}, &s) == Bytes{0xf0});
                REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11001100, 0b11001100}, &s) == Bytes{0xaa});
                REQUIRE (Downsample<2, Bytes>{}(Bytes{0b11001100, 0b11001100, 0b11111111, 0b00000000}, &s) == Bytes{0xaa, 0xf0});
                REQUIRE (Downsample<2, Bytes>{}(Bytes{0b10101010, 0b10101010}, &s) == Bytes{0b10101010});
                // REQUIRE (test::Downsample<2, Bytes>{}(Bytes{0b10101010, 0b10101010, 0b10101010}, &s) == Bytes{0b10101010, 0b1010'0000});

                REQUIRE (downsample<4> (Bytes{0x00, 0x00, 0x00, 0x00}, &s) == Bytes{0x00});
                REQUIRE (downsample<4> (Bytes{0b11111111, 0b11111111, 0b00000000, 0b00000000}, &s) == Bytes{0xf0});
                REQUIRE (downsample<4> (Bytes{0b11110000, 0b11110000, 0b11110000, 0b11110000}, &s) == Bytes{0xaa});

                SECTION ("16384") // Found by chance
                {
                        auto data = std::views::repeat (0xcc, 16384) | std::ranges::to<Bytes> ();
                        bool s{};
                        auto zoomedOut = downsample<8> (data, &s);
                        REQUIRE (zoomedOut.size () == 2048);
                }
        }

        SECTION ("Downsample look-up table")
        {
                bool s{};

                REQUIRE (downsample3 (Bytes{0x00, 0x00}, &s) == Bytes{0x00});
                REQUIRE (downsample3 (Bytes{0x00, 0x00, 0x00, 0x00}, &s) == Bytes{0x00, 0x00});
                REQUIRE (downsample3 (Bytes{0b11111111, 0b00000000}, &s) == Bytes{0xf0});
                REQUIRE (downsample3 (Bytes{0b11001100, 0b11001100}, &s) == Bytes{0xaa});
                REQUIRE (downsample3 (Bytes{0b11001100, 0b11001100, 0b11111111, 0b00000000}, &s) == Bytes{0xaa, 0xf0});
                REQUIRE (downsample3 (Bytes{0b10101010, 0b10101010}, &s) == Bytes{0b10101010});
        }
}

/****************************************************************************/

TEST_CASE ("comparison", "[popcount]")
{
        std::random_device rd;
        std::uniform_int_distribution uni (0, 255);
        auto data = std::views::iota (0, 16384) | std::views::transform ([&uni, &rd] (auto) { return uni (rd); }) | std::ranges::to<Bytes> ();

        SECTION ("/2 state=0")
        {
                bool s1{};
                auto a = downsample<2> (data, &s1);

                bool s2{};
                auto b = downsample3 (data, &s2);

                REQUIRE (a.size () == b.size ());

                auto words = data | std::views::adjacent_transform<2> ([] (uint8_t a, uint8_t b) -> uint16_t { return a << 8 | b; })
                        | std::views::stride (2) | std::ranges::to<std::vector> ();

                REQUIRE (words.size () == a.size ());

                // for (auto const &[src, a, b] : std::views::zip (words, a, b)) {
                //         std::println ("src: {:016b}, d<2>: {:08b}, d3: {:08b} {}", src, a, b, (a != b) ? ("*") : (""));
                // }

                REQUIRE (a == b);
        }

        SECTION ("/2 state=1")
        {
                bool s1 = true;
                auto a = downsample<2> (data, &s1);

                bool s2 = true;
                auto b = downsample3 (data, &s2);

                REQUIRE (a == b);
        }

        // SECTION ("/4 state=0")
        // {
        //         bool s1{};
        //         bool s2{};
        //         auto a = downsample<2> (downsample<2> (data, &s1), &s2);

        //         bool s3{};
        //         auto b = downsample3 (data, &s3);

        //         REQUIRE (a == b);
        // }

        // SECTION ("print") { lutD2_f (); }
}

/****************************************************************************/

/**
 * This is the function that perpared the LUT for downsample /2 function.
 */
void lutD2_f ()
{
        bool state{};

        auto last = [&state] (int bits) mutable -> bool {
                if (bits == 1) {
                        bits = !state;
                        state = bits; // Flipping back and forth
                }

                return bool (bits);
        };

        for (uint8_t ch : std::views::iota (0, 256)) {
                state = false; // Change this from true to false in order to generate both tables.
                int bit3 = last (std::popcount (uint8_t (ch & 0b11000000)));
                int bit2 = last (std::popcount (uint8_t (ch & 0b00110000)));
                int bit1 = last (std::popcount (uint8_t (ch & 0b00001100)));
                int bit0 = last (std::popcount (uint8_t (ch & 0b00000011)));

                uint8_t o = bit3 << 3 | bit2 << 2 | bit1 << 1 | bit0 << 0;
                (void)o;
                // std::println ("ch: {}, o: {}", ch, o);

                // std::println ("{}, {:08b}. {:#x}, {:04b}, s: {}", ch, ch, o, o, int (state));
                std::print ("{},", int (state));
                // std::print ("{},", int (o));
        }
}
