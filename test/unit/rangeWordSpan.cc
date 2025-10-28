/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <catch2/catch_test_macros.hpp>
module logic.data;
import utils;
import :range.span.byte; // Hack to speed up UT compilation (thus development).

using namespace logic;

TEST_CASE ("Size, empty B", "[rangeWordSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        BlockArray blockArray;
        blockArray.setBlockSizeB (16);
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
        blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
        auto r = blockArray.range (0, 96);
        REQUIRE (std::ranges::distance (r) == 3);

        SECTION ("size 1")
        {
                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 1};
                REQUIRE (rbs.size () == 1);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size 1a")
        {
                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1};
                REQUIRE (rbs.size () == 1);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size 6 max")
        {
                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 6};
                REQUIRE (rbs.size () == 6);
                REQUIRE (!rbs.empty ());
        }

        SECTION ("size past the max")
        {
                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 32};
                REQUIRE (rbs.size () == 6); // only 12 B (length) in the range
                REQUIRE (!rbs.empty ());
        }

        SECTION ("empty range 1")
        {
                BlockArray blockArray;
                blockArray.append (BITS_PER_SAMPLE, {});
                auto r = blockArray.range (0, 96);

                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                REQUIRE (rbs.size () == 0);
                REQUIRE (rbs.empty ());
        }

        SECTION ("empty range 2")
        {
                BlockArray blockArray;
                blockArray.append (BITS_PER_SAMPLE, {{}, {}});
                auto r = blockArray.range (0, 96);

                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 6, 1000};
                REQUIRE (rbs.size () == 0);
                REQUIRE (rbs.empty ());
        }
}

/****************************************************************************/

TEST_CASE ("Advance 1 block B", "[rangeWordSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        SECTION ("One block")
        {
                BlockArray blockArray;
                blockArray.setBlockSizeB (16);
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
                auto r = blockArray.range (0, 4);

                SECTION ("first byte")
                {
                        BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 1};
                        auto iter = rbs.begin ();
                        REQUIRE (*iter == 0xaa);

                        iter.advance (0);
                        REQUIRE (*iter == 0xaa);
                }

                SECTION ("advance by 1")
                {
                        BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 4};
                        auto iter = rbs.begin ();
                        REQUIRE (*iter == 0xaa);

                        iter.advance (1);
                        REQUIRE (*iter == 0xef);
                        REQUIRE (iter != rbs.end ());

                        iter.advance (1);
                        REQUIRE (*iter == 0xdf);

                        iter.advance (1);
                        REQUIRE (*iter == 0xcf);

                        iter.advance (1);
                        REQUIRE (iter == rbs.cend ());
                }

                SECTION ("advance by 2")
                {
                        auto r = blockArray.range (0, 32);

                        BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 1, 3};
                        auto iter = rbs.begin ();
                        REQUIRE (*iter == 0xef);

                        iter.advance (2);
                        REQUIRE (*iter == 0xcf);

                        iter.advance (1);
                        REQUIRE (iter == rbs.end ());
                }
        }

        SECTION ("4 blocks")
        {
                BlockArray blockArray;
                blockArray.setBlockSizeB (16);
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                auto r = blockArray.range (0, 128);

                SECTION ("advance by 5")
                {
                        BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 2, 1000};
                        auto iter = rbs.begin ();
                        REQUIRE (iter != rbs.end ());
                        REQUIRE (*iter == 0xdf);

                        iter.advance (5);
                        REQUIRE (iter != rbs.end ());
                        REQUIRE (*iter == 0x0b);

                        iter.advance (5);
                        REQUIRE (iter != rbs.end ());
                        REQUIRE (*iter == 0x00);

                        iter.advance (4);
                        REQUIRE (iter == rbs.end ());
                }

                SECTION ("max advance")
                {
                        BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 2, 1000};
                        auto iter = rbs.begin ();
                        REQUIRE (*iter == 0xdf);

                        iter.advance (12);
                        REQUIRE (iter != rbs.end ());
                        REQUIRE (*iter == 0x02);

                        iter.advance (2);
                        REQUIRE (iter == rbs.end ());
                }
        }
}

TEST_CASE ("Increment++", "[rangeWordSpan]")
{
        static constexpr auto BITS_PER_SAMPLE = 1U;

        SECTION ("1 block")
        {
                BlockArray blockArray;
                blockArray.setBlockSizeB (16);
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
                auto r = blockArray.range (0, 32);

                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 4};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 0xaa);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xef);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xdf);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xcf);

                iter++;
                REQUIRE (iter == rbs.end ());
        }

        SECTION ("4 blocks")
        {
                BlockArray blockArray;
                blockArray.setBlockSizeB (16);
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (3));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (2));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (1));
                blockArray.append (BITS_PER_SAMPLE, getChannelBlockData (0));
                auto r = blockArray.range (0, 128);

                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, 0, 0, 16};
                auto iter = rbs.begin ();
                REQUIRE (*iter == 0xaa);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xef);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xdf);

                iter++;
                REQUIRE (iter != rbs.end ());
                REQUIRE (*iter == 0xcf);

                iter++;
                REQUIRE (*iter == 0x08);

                iter++;
                iter++;
                REQUIRE (*iter == 0x0a);

                iter++;
                iter++;
                iter++;
                REQUIRE (*iter == 0x05);

                iter++;
                iter++;
                iter++;
                REQUIRE (*iter == 0x00);

                iter++;
                iter++;
                iter++;
                REQUIRE (*iter == 0x03);

                iter++;
                REQUIRE (iter == rbs.end ());
        }
}