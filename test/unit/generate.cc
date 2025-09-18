/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "common/params.hh"
#include <catch2/catch_test_macros.hpp>
#include <climits>
#include <ranges>
#include <vector>

import logic.processing;
import logic.data;

using namespace logic;

TEST_CASE ("square", "[generate]")
{
        SECTION ("1 bit")
        {
                Bytes buf = Square{}(1, 1, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xaa);

                buf = Square{false}(1, 1, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0x55);

                buf = Square{}(1, 1, 16);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xaa, 0xaa});

                buf = Square{}(1, 1, 17);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xaa, 0xaa});

                buf = Square{}(1, 1, 10 * CHAR_BIT);
                REQUIRE (buf.size () == 10);
                REQUIRE (buf == Bytes{0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa});
        }

        SECTION ("2 bit")
        {
                Bytes buf = Square{}(2, 2, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xcc);

                buf = Square{}(2, 2, 24);
                REQUIRE (buf.size () == 3);
                REQUIRE (buf == Bytes{0xcc, 0xcc, 0xcc});
        }

        SECTION ("3 bit")
        {
                Bytes buf = Square{}(3, 3, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xe3);

                buf = Square{}(3, 3, 16);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xe3, 0x8e});
        }

        SECTION ("13 bit")
        {
                Bytes buf = Square{}(13, 13, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xff);

                buf = Square{}(13, 13, 64);
                REQUIRE (buf.size () == 8);
                REQUIRE (buf == Bytes{0xFF, 0xF8, 0x00, 0x3F, 0xFE, 0x00, 0x0F, 0xFF});
        }

        SECTION ("not symmetric")
        {
                Bytes buf = Square{}(1, 2, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0x92);
        }

        SECTION ("3 bit continuation")
        {
                auto square = Square{};
                Bytes buf = square (3, 3, 8);
                REQUIRE (buf.front () == 0xe3);

                buf = square (3, 3, 8);
                REQUIRE (buf.front () == 0x8e);
        }

        SECTION ("13 bit continuation")
        {
                auto square = Square{};
                Bytes buf;

                std::ranges::copy (square (13, 13, 8), std::back_inserter (buf));
                std::ranges::copy (square (13, 13, 16), std::back_inserter (buf));
                std::ranges::copy (square (13, 13, 32), std::back_inserter (buf));
                std::ranges::copy (square (13, 13, 8), std::back_inserter (buf));

                REQUIRE (buf == Bytes{0xFF, 0xF8, 0x00, 0x3F, 0xFE, 0x00, 0x0F, 0xFF});
        }

        auto gen = [] (size_t i) {
                auto square = Square{};
                Bytes buf;

                std::ranges::copy (square (i, i, 8192), std::back_inserter (buf));
                std::ranges::copy (square (i, i, 8192), std::back_inserter (buf));
                std::ranges::copy (square (i, i, 8192), std::back_inserter (buf));

                REQUIRE (buf.size () == 3 * 1024);
                return buf;
        };

        SECTION ("demo device 1 bit")
        {
                auto buf = gen (1);
                Bytes cmp (buf.size ());
                std::ranges::fill (cmp, 0xaa);
                REQUIRE (cmp == buf);
        }

        SECTION ("demo device 2 bit")
        {
                auto buf = gen (2);
                Bytes cmp (buf.size ());
                std::ranges::fill (cmp, 0xcc);
                REQUIRE (cmp == buf);
        }

        SECTION ("demo device other bits")
        {
                REQUIRE (gen (3)
                         == (std::views::repeat (Bytes{0xe3, 0x8e, 0x38}) | std::views::take (1024) | std::views::join
                             | std::ranges::to<Bytes> ()));

                REQUIRE (gen (4) == (std::views::repeat (0xf0, 3 * 1024) | std::ranges::to<Bytes> ()));

                REQUIRE (gen (5)
                         == (std::views::repeat (Bytes{0xF8, 0x3E, 0x0F, 0x83, 0xE0}) | std::views::join | std::views::take (3 * 1024)
                             | std::ranges::to<Bytes> ()));

                REQUIRE (gen (6)
                         == (std::views::repeat (Bytes{0xfc, 0x0f, 0xc0}) | std::views::join | std::views::take (3 * 1024)
                             | std::ranges::to<Bytes> ()));
        }
}
