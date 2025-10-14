/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <algorithm>
#include <catch2/catch_test_macros.hpp>
#include <ranges>
#include <type_traits>
#include <vector>
module logic.data;

import logic.util;
import logic.processing;
import utils;
import :span.owning;

using namespace logic;

TEST_CASE ("Basic", "[bitSpan]")
{
        Bytes bytes = {
                0b1111'0000,
                0b0101'1010,
                0b1100'1100,
                0b1110'0011,
        };

        SECTION ("Span")
        {
                REQUIRE (utl::vectorize (BitSpan{bytes.data (), 12U, 8U}) == std::vector<bool>{1, 0, 1, 0, 1, 1, 0, 0});
                REQUIRE (utl::vectorize (BitSpan{bytes.data (), 0U, 8U}) == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0});
                REQUIRE (utl::vectorize (BitSpan{bytes.data (), 0U, 32U})
                         == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1});
                REQUIRE (utl::vectorize (BitSpan{bytes.data (), 31U, 1U}) == std::vector<bool>{1});
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

/****************************************************************************/

TEST_CASE ("Owning Basic", "[bitSpan]")
{
        Bytes bytes = {
                0b1111'0000,
                0b0101'1010,
                0b1100'1100,
                0b1110'0011,
        };

        SECTION ("Iterator increment")
        {
                auto span = OwningBitSpan{bytes, 4, 4};
                auto i = span.begin ();
                REQUIRE (i == span.begin ());
                REQUIRE (i != span.end ());
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (*i++ == 0);
                REQUIRE (i == span.end ());
        }

        SECTION ("Span")
        {
                auto a = utl::vectorize (OwningBitSpan{bytes, 12U, 8U});
                REQUIRE (a == std::vector<bool>{1, 0, 1, 0, 1, 1, 0, 0});
                REQUIRE (utl::vectorize (OwningBitSpan{bytes, 0U, 8U}) == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0});
                REQUIRE (utl::vectorize (OwningBitSpan{bytes, 0U, 32U})
                         == std::vector<bool>{1, 1, 1, 1, 0, 0, 0, 0, 0, 1, 0, 1, 1, 0, 1, 0, 1, 1, 0, 0, 1, 1, 0, 0, 1, 1, 1, 0, 0, 0, 1, 1});
                REQUIRE (utl::vectorize (OwningBitSpan{bytes, 31U, 1U}) == std::vector<bool>{1});
        }
}

TEST_CASE ("Join", "[backend]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;
        BlockArray cbs;
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (0));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        cbs.append (BITS_PER_SAMPLE, getChannelBlockData (2));

        SECTION ("copy")
        {
                auto r = cbs.range (0, 96);

                auto x = r | std::views::transform ([] (Block const &b) { return b.channel (0); }) | std::views::join
                        | std::ranges::to<Bytes> ();

                static_assert (std::ranges::range<decltype (x)>);
                static_assert (std::ranges::input_range<decltype (x)>);
                static_assert (std::ranges::forward_range<decltype (x)>);
                static_assert (std::ranges::bidirectional_range<decltype (x)>);
                static_assert (std::ranges::random_access_range<decltype (x)>);
                static_assert (std::ranges::contiguous_range<decltype (x)>);
                static_assert (std::ranges::common_range<decltype (x)>);
                static_assert (std::ranges::viewable_range<decltype (x)>);

                REQUIRE (x == Bytes{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb});
        }

        SECTION ("lazy")
        {
                auto r = cbs.range (0, 96);
                REQUIRE (std::ranges::distance (r) == 3);

                auto x = r | std::views::transform ([] (Block const &b) { return b.channel (0); }) | std::views::join;

                static_assert (std::ranges::range<decltype (x)>);
                static_assert (std::ranges::input_range<decltype (x)>);
                static_assert (!std::ranges::forward_range<decltype (x)>);
                static_assert (!std::ranges::bidirectional_range<decltype (x)>);
                static_assert (!std::ranges::random_access_range<decltype (x)>);
                static_assert (!std::ranges::contiguous_range<decltype (x)>);
                static_assert (!std::ranges::common_range<decltype (x)>);
                static_assert (std::ranges::viewable_range<decltype (x)>);

                REQUIRE (std::ranges::equal (x, Bytes{0, 1, 2, 3, 4, 5, 6, 7, 8, 9, 0xa, 0xb}));
        }

        SECTION ("bit-wise all")
        {
                auto x = cbs.range (0, 96) | std::views::transform ([] (Block const &b) { return b.channel (0); }) | std::views::join;
                auto span = OwningBitSpan{x, 0, 96};

                static_assert (std::is_same_v<decltype (span)::BitCarrierT, uint8_t>);

                bool b = std::ranges::equal (span,
                                             std::vector<bool>{
                                                     0, 0, 0, 0, 0, 0, 0, 0, //
                                                     0, 0, 0, 0, 0, 0, 0, 1, //
                                                     0, 0, 0, 0, 0, 0, 1, 0, //
                                                     0, 0, 0, 0, 0, 0, 1, 1, //
                                                     0, 0, 0, 0, 0, 1, 0, 0, //
                                                     0, 0, 0, 0, 0, 1, 0, 1, //
                                                     0, 0, 0, 0, 0, 1, 1, 0, //
                                                     0, 0, 0, 0, 0, 1, 1, 1, //
                                                     0, 0, 0, 0, 1, 0, 0, 0, //
                                                     0, 0, 0, 0, 1, 0, 0, 1, //
                                                     0, 0, 0, 0, 1, 0, 1, 0, //
                                                     0, 0, 0, 0, 1, 0, 1, 1, //
                                             });
                REQUIRE (b);
        }

        SECTION ("bit-wise 2nd and 3rd block")
        {
                auto x = cbs.range (32, 64) | std::views::transform ([] (Block const &b) { return b.channel (0); }) | std::views::join;
                auto span = OwningBitSpan{x, 32, 64};

                static_assert (std::is_same_v<decltype (span)::BitCarrierT, uint8_t>);

                bool b = std::ranges::equal (span,
                                             std::vector<bool>{
                                                     0, 0, 0, 0, 0, 1, 0, 0, //
                                                     0, 0, 0, 0, 0, 1, 0, 1, //
                                                     0, 0, 0, 0, 0, 1, 1, 0, //
                                                     0, 0, 0, 0, 0, 1, 1, 1, //
                                                     0, 0, 0, 0, 1, 0, 0, 0, //
                                                     0, 0, 0, 0, 1, 0, 0, 1, //
                                                     0, 0, 0, 0, 1, 0, 1, 0, //
                                                     0, 0, 0, 0, 1, 0, 1, 1, //
                                             });
                REQUIRE (b);
        }

        SECTION ("bit-wise 9 bits")
        {
                auto x = cbs.range (31, 9) | std::views::transform ([] (Block const &b) { return b.channel (0); }) | std::views::join;
                auto span = OwningBitSpan{x, 31, 9};
                REQUIRE (std::ranges::equal (span, std::vector<bool>{1, 0, 0, 0, 0, 0, 1, 0, 1}));
        }
}