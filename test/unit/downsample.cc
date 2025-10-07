/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
#include <catch2/catch_test_macros.hpp>

using namespace logic;

TEST_CASE ("64 -> 8", "[downsample]")
{
        SECTION ("0 bits")
        {
                Bytes input{0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0x00});
        }

        SECTION ("1 bit")
        {
                Bytes input{0x01, 0x02, 0x04, 0x08, 0x10, 0x20, 0x40, 0x80};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0x00});
        }

        SECTION ("2 bits")
        {
                Bytes input{0x03, 0x30, 0x03, 0x30, 0x03, 0x30, 0x03, 0x30};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0x00});
        }

        SECTION ("3 and 5 bits")
        {
                Bytes input{0x03, 0x30, 0x03, 0x30, 0x1f, 0xf1, 0x1f, 0xf1};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0x0f});
        }

        SECTION ("6 bits")
        {
                Bytes input{0xf3, 0x3f, 0xf3, 0x3f, 0x3f, 0xf3, 0x3f, 0xf3};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0xff});
        }

        SECTION ("Half bits set 0xf0")
        {
                Bytes input{0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0, 0xF0};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0xaa});
        }

        SECTION ("Half bits set 0xaa")
        {
                Bytes input{0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa, 0xaa};
                State state;
                auto result = downsample (input, 0, 64, 8, &state);
                REQUIRE (result == Bytes{0xaa});
        }
}

TEST_CASE ("3 -> 2", "[downsample]")
{
        SECTION ("0 bits")
        {
                Bytes input{0b10010010, 0b01001001, 0b00100100, 0b10010011};
                State state;
                auto result = downsample (input, 0, 30, 20, &state);
                REQUIRE (result == Bytes{0b10001000, 0b10001000});
        }

        SECTION ("1 bits")
        {
                Bytes input{0b11011011, 0b01101101, 0b10110110, 0b11011000};
                State state;
                auto result = downsample (input, 0, 30, 20, &state);
                REQUIRE (result == Bytes{0xaa, 0xaa});
        }
}

TEST_CASE ("4 -> 2 offset", "[downsample]")
{
        SECTION ("offset 0")
        {
                Bytes input{0b11111100, 0b11001100, 0b11001100, 0b11001100};
                State state;
                auto result = downsample (input, 0, 32, 16, &state);
                REQUIRE (result == Bytes{0b11101010, 0b10101010});
        }

        SECTION ("offset 4")
        {
                Bytes input{
                        0b11111100, 0b11001100, 0b11001100, 0b11001100, 0b11001111,
                };
                State state;
                auto result = downsample (input, 4, 32, 16, &state);
                REQUIRE (result == Bytes{0b10101010, 0b10101010});
        }

        SECTION ("offset 8")
        {
                Bytes input{
                        0b11111100, 0b11001100, 0b11001100, 0b11001100, 0b11001111,
                };
                State state;
                auto result = downsample (input, 8, 32, 16, &state);
                REQUIRE (result == Bytes{0b10101010, 0b10101011});
        }
}

TEST_CASE ("BlockArray", "[downsample]")
{
        SECTION ("offset 0")
        {
                constexpr auto BITS_PER_SAMPLE = 1U;
                constexpr size_t CHANNEL = 0;
                constexpr size_t INPUT_SIZE_BYTES = 6;
                constexpr size_t INPUT_SIZE_BITS = INPUT_SIZE_BYTES * 8;
                constexpr size_t INPUT_OFFSET_BITS = 0;
                constexpr size_t OUTPUT_SIZE_BITS = 8 * 3;

                BlockArray blockArray;
                blockArray.append (BITS_PER_SAMPLE, std::vector<Bytes>{Bytes{0b11111100, 0b11001100}});
                blockArray.append (BITS_PER_SAMPLE, std::vector<Bytes>{Bytes{0b11001100, 0b11001100}});
                blockArray.append (BITS_PER_SAMPLE, std::vector<Bytes>{Bytes{0b11001100, 0b11001111}});

                auto r = blockArray.range (0, INPUT_SIZE_BITS);
                REQUIRE (std::ranges::distance (r) == 3);

                State state;
                BlockRangeWordSpan<uint8_t const, BlockArray::Container> rbs{r, CHANNEL, 0, INPUT_SIZE_BYTES};
                auto result = downsample (rbs, INPUT_OFFSET_BITS, INPUT_SIZE_BITS, OUTPUT_SIZE_BITS, &state);

                REQUIRE (result == Bytes{0b11101010, 0b10101010, 0b10101011});
        }
}