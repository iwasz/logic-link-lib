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

import logic.processing;
import logic.data;

using namespace logic;

TEST_CASE ("square", "[generate]")
{
        SECTION ("1 bit")
        {
                Bytes buf = square (1, 1, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xaa);

                buf = square (1, 1, 8, false);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0x55);

                buf = square (1, 1, 16);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xaa, 0xaa});

                buf = square (1, 1, 17);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xaa, 0xaa});

                buf = square (1, 1, 10 * CHAR_BIT);
                REQUIRE (buf.size () == 10);
                REQUIRE (buf == Bytes{0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa});
        }

        SECTION ("2 bit")
        {
                Bytes buf = square (2, 2, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xcc);

                buf = square (2, 2, 24);
                REQUIRE (buf.size () == 3);
                REQUIRE (buf == Bytes{0xcc, 0xcc, 0xcc});
        }

        SECTION ("3 bit")
        {
                Bytes buf = square (3, 3, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xe3);

                buf = square (3, 3, 16);
                REQUIRE (buf.size () == 2);
                REQUIRE (buf == Bytes{0xe3, 0x8e});
        }

        SECTION ("13 bit")
        {
                Bytes buf = square (13, 13, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0xff);

                buf = square (13, 13, 64);
                REQUIRE (buf.size () == 8);
                REQUIRE (buf == Bytes{0xFF, 0xF8, 0x00, 0x3F, 0xFE, 0x00, 0x0F, 0xFF});
        }

        SECTION ("not symmetric")
        {
                Bytes buf = square (1, 2, 8);
                REQUIRE (buf.size () == 1);
                REQUIRE (buf.front () == 0x92);
        }
}
