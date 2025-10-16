/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
import logic.data;
using namespace logic;

TEST_CASE ("relativeIndex", "[types]")
{
        REQUIRE (relativeBegin (0, 0, 1) == 0);
        REQUIRE (relativeBegin (2, 0, 1) == 2);
        REQUIRE (relativeBegin (1000, 0, 1) == 1000);

        REQUIRE (relativeBegin (20, 16, 1) == 4);
        REQUIRE (relativeBegin (25, 16, 1) == 9);

        REQUIRE (relativeBegin (20, 16, 2) == 2);
        REQUIRE (relativeBegin (25, 16, 2) == 4);

        REQUIRE (relativeEnd (20, 16, 1) == 4);
        REQUIRE (relativeEnd (25, 16, 1) == 9);

        REQUIRE (relativeEnd (20, 16, 2) == 1);
        REQUIRE (relativeEnd (25, 16, 2) == 4);
}
