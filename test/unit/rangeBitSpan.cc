/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
#include <deque>
module logic.data;
import utils;
import :range.span; // Hacks to speed up UT compilation (thus development).

using namespace logic;

TEST_CASE ("Size, empty", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        auto r = blockArray.range (0, 96);
        REQUIRE (r.size () == 3);

        SECTION ("size 1")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 1};
                REQUIRE (rbs.size () == 1);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size 1a")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1};
                REQUIRE (rbs.size () == 1);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size 6")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 6};
                REQUIRE (rbs.size () == 6);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size 32")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 32};
                REQUIRE (rbs.size () == 32);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size max")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 90};
                REQUIRE (rbs.size () == 90);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size past the max")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                REQUIRE (rbs.size () == 90);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("empty range 1")
        {
                BlockArray blockArray;
                blockArray.append (BITS_PER_SAMPLE, {});
                auto r = blockArray.range (0, 96);

                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                REQUIRE (rbs.size () == 0);
                REQUIRE (rbs.empty ());
        }

        SECTION ("empty range 2")
        {
                BlockArray blockArray;
                blockArray.append (BITS_PER_SAMPLE, {{}, {}});
                auto r = blockArray.range (0, 96);

                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                REQUIRE (rbs.size () == 0);
                REQUIRE (rbs.empty ());
        }
}

/*
 * To test and figure out what actually should happen:
 * - Iterator from BlockArray{}
 * - Iterator from BlockArray{ Block {} }
 * - Iterator from BlockArray{ Block { {}, {} } }
 */
TEST_CASE ("Advance 1 block", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        auto r = blockArray.range (0, 1);

        SECTION ("first bit")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 8};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (0);
                REQUIRE (*iter == 1);
        }

        SECTION ("advance by 1")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 8};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (*iter == 0);
                REQUIRE (iter != rbs.end ());

                iter.advance (1);
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (*iter == 0);

                iter.advance (1);
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (*iter == 0);

                iter.advance (1);
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (*iter == 0);

                iter.advance (1);
                REQUIRE (iter == rbs.end ());
        }

        SECTION ("advance by 2")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 1, 7};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 0);

                iter.advance (2);
                REQUIRE (*iter == 0);
                REQUIRE (iter != rbs.end ());

                iter.advance (2);
                REQUIRE (*iter == 0);
                REQUIRE (iter != rbs.end ());

                iter.advance (2);
                REQUIRE (*iter == 0);
                REQUIRE (iter != rbs.end ());

                iter.advance (1);
                REQUIRE (iter == rbs.end ());
        }

        SECTION ("advance by 5")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 2, 1000}; // len == 30 bits
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (5);
                REQUIRE (*iter == 0);

                iter.advance (5);
                REQUIRE (*iter == 1);

                iter.advance (5);
                REQUIRE (*iter == 1);

                iter.advance (5);
                REQUIRE (*iter == 1);

                iter.advance (5);
                REQUIRE (*iter == 0);

                iter.advance (5);
                REQUIRE (iter == rbs.end ());
        }

        SECTION ("max advance")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 2, 1000};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (29);
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (iter == rbs.end ());
        }
}

TEST_CASE ("Advance 3 blocks", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        auto r = blockArray.range (0, 96);
        REQUIRE (r.size () == 3);

        SECTION ("advance A")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (1);
                REQUIRE (*iter == 0);

                iter.advance (1);
                REQUIRE (*iter == 1);

                iter.advance (1); // 0
                iter.advance (1); // 1
                iter.advance (1); // 0
                iter.advance (1); // 1
                iter.advance (1); // 0

                iter.advance (1); // 1 (0xef)
                REQUIRE (*iter == 1);
                iter.advance (1); // 1 (0xef)
                REQUIRE (*iter == 1);
                iter.advance (1); // 1 (0xef)
                REQUIRE (*iter == 1);
                iter.advance (1); // 1 (0xef)
                REQUIRE (*iter == 1);

                iter.advance (4);
                REQUIRE (*iter == 1); // Last bit (LSb) of 0xef

                iter.advance (8);
                REQUIRE (*iter == 1);
        }

        SECTION ("advance by 30")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter.advance (30);
                REQUIRE (*iter == 1);
                REQUIRE (iter != rbs.end ());
        }
}

TEST_CASE ("Increment++ 1 block", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        auto r = blockArray.range (0, 1);

        SECTION ("advance by 1")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 8};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter++;
                REQUIRE (*iter == 0);
                REQUIRE (iter != rbs.end ());

                iter++;
                REQUIRE (*iter == 1);

                iter++;
                REQUIRE (*iter == 0);

                iter++;
                REQUIRE (*iter == 1);

                iter++;
                REQUIRE (*iter == 0);

                iter++;
                REQUIRE (*iter == 1);

                iter++;
                REQUIRE (*iter == 0);

                iter++;
                REQUIRE (iter == rbs.end ());
        }
}

TEST_CASE ("Increment++ 3 blocks", "[rangeBitSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        auto r = blockArray.range (0, 96);
        REQUIRE (r.size () == 3);

        SECTION ("advance")
        {
                RangeBitSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 1);

                iter++;
                REQUIRE (*iter == 0);

                iter++;
                REQUIRE (*iter == 1);

                iter++; // 0
                iter++; // 1
                iter++; // 0
                iter++; // 1
                iter++; // 0

                iter++; // 1 (0xef)
                REQUIRE (*iter == 1);
                iter++; // 1 (0xef)
                REQUIRE (*iter == 1);
                iter++; // 1 (0xef)
                REQUIRE (*iter == 1);
                iter++; // 1 (0xef)
                REQUIRE (*iter == 1);

                iter++;
                iter++;
                iter++;
                iter++;
                REQUIRE (*iter == 1); // Last bit (LSb) of 0xef

                iter++;
                iter++;
                iter++;
                iter++;
                iter++;
                iter++;
                iter++;
                iter++;
                REQUIRE (*iter == 1);

                iter++;
                iter++;
                iter++;
                REQUIRE (iter == rbs.end ());
        }
}
