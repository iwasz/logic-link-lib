/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <vector>

import logic.util;
import logic.data;

using namespace logic;
using namespace logic::util;

TEST_CASE ("Basic", "[bitSpan]")
{
        Bytes bytes = {
                0b1111'0000,
                0b0101'1010,
                0b1100'1100,
                0b1110'0011,
        };

        auto vectorize = [] (auto const &sp) {
                std::vector<bool> dreadedVector;

                for (bool b : sp) {
                        dreadedVector.push_back (b);
                }
                return dreadedVector;
        };

        SECTION ("Span")
        {
                REQUIRE (vectorize (BitSpan{bytes.data (), 12U, 8U}) == std::vector<bool>{1, 0, 1, 0, 1, 1, 0, 0});
                REQUIRE (vectorize (BitSpan{bytes.data (), 0U, 8U}) == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0});
                REQUIRE (vectorize (BitSpan{bytes.data (), 0U, 32U})
                         == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1});
                REQUIRE (vectorize (BitSpan{bytes.data (), 31U, 1U}) == std::vector<bool>{1});
        }

        SECTION ("Iterator increment decrement")
        {
                auto span = BitSpan{bytes.data (), 4, 4};
                auto i = span.begin ();
                REQUIRE (i == span.begin ());
                REQUIRE (i != span.end ());
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (i == span.end ());

                i = span.begin ();
                REQUIRE (*i-- == 0);
                REQUIRE (*i-- == 1);
                REQUIRE (*i-- == 1);
                REQUIRE (*i-- == 1);
                REQUIRE (*i-- == 1);
        }
}
