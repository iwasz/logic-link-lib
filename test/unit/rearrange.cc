/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <catch2/catch_test_macros.hpp>
#include <variant>
module logic; // This is a HACK. By becoming a part of the module I gain access to its partitions I want to test.
import :processing;

using namespace logic;

TEST_CASE ("flexio", "[rearrange]")
{
        SECTION ("8 channels 1 shifter per channel")
        {
                RawData raw;
                raw.buffer = {
                        0x00, 0x01, 0x02, 0x03, // CH0 word 0
                        0x10, 0x11, 0x12, 0x13, // CH1 word 0
                        0x20, 0x21, 0x22, 0x23, // CH2 word 0
                        0x30, 0x31, 0x32, 0x33, // CH3 word 0
                        0x40, 0x41, 0x42, 0x43, // CH4 word 0
                        0x50, 0x51, 0x52, 0x53, // CH5 word 0
                        0x60, 0x61, 0x62, 0x63, // CH6 word 0
                        0x70, 0x71, 0x72, 0x73, // CH7 word 0

                        0x04, 0x05, 0x06, 0x07, // CH0 word 1
                        0x14, 0x15, 0x16, 0x17, // CH1 word 1
                        0x24, 0x25, 0x26, 0x27, // CH2 word 1
                        0x34, 0x35, 0x36, 0x37, // CH3 word 1
                        0x44, 0x45, 0x46, 0x47, // CH4 word 1
                        0x54, 0x55, 0x56, 0x57, // CH5 word 1
                        0x64, 0x65, 0x66, 0x67, // CH6 word 1
                        0x74, 0x75, 0x76, 0x77, // CH7 word 1
                };

                common::acq::Params params;
                params.digitalChannels = 8;
                params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
                auto sd = rearrange (raw, params);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer) == Bytes{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07});
                REQUIRE (std::get<Bytes> (sd.at (1).buffer) == Bytes{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17});
                REQUIRE (std::get<Bytes> (sd.at (2).buffer) == Bytes{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27});
                REQUIRE (std::get<Bytes> (sd.at (3).buffer) == Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37});
                REQUIRE (std::get<Bytes> (sd.at (4).buffer) == Bytes{0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47});
                REQUIRE (std::get<Bytes> (sd.at (5).buffer) == Bytes{0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57});
                REQUIRE (std::get<Bytes> (sd.at (6).buffer) == Bytes{0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67});
                REQUIRE (std::get<Bytes> (sd.at (7).buffer) == Bytes{0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77});
        }

        SECTION ("8 channels 1 shifter per channel")
        {
                RawData raw;
                raw.buffer = {
                        0x00, 0x01, 0x02, 0x03, // CH0 word 0
                        0x10, 0x11, 0x12, 0x13, // CH1 word 0
                        0x20, 0x21, 0x22, 0x23, // CH2 word 0
                        0x30, 0x31, 0x32, 0x33, // CH3 word 0

                        0x04, 0x05, 0x06, 0x07, // CH0 word 1
                        0x14, 0x15, 0x16, 0x17, // CH1 word 1
                        0x24, 0x25, 0x26, 0x27, // CH2 word 1
                        0x34, 0x35, 0x36, 0x37, // CH3 word 1

                        0x08, 0x09, 0x0a, 0x0b, // CH0 word 2
                        0x18, 0x19, 0x1a, 0x1b, // CH1 word 2
                        0x28, 0x29, 0x2a, 0x2b, // CH2 word 2
                        0x38, 0x39, 0x3a, 0x3b, // CH3 word 2

                        0x0c, 0x0d, 0x0e, 0x0f, // CH0 word 3
                        0x1c, 0x1d, 0x1e, 0x1f, // CH1 word 3
                        0x2c, 0x2d, 0x2e, 0x2f, // CH2 word 3
                        0x3c, 0x3d, 0x3e, 0x3f, // CH3 word 3
                };

                common::acq::Params params;
                params.digitalChannels = 4;
                params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
                auto sd = rearrange (raw, params);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f});
                REQUIRE (std::get<Bytes> (sd.at (1).buffer)
                         == Bytes{0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f});
                REQUIRE (std::get<Bytes> (sd.at (2).buffer)
                         == Bytes{0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27, 0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f});
                REQUIRE (std::get<Bytes> (sd.at (3).buffer)
                         == Bytes{0x30, 0x31, 0x32, 0x33, 0x34, 0x35, 0x36, 0x37, 0x38, 0x39, 0x3a, 0x3b, 0x3c, 0x3d, 0x3e, 0x3f});
        }

        /*
         * This configuration is not used in the firmware.
         */
        SECTION ("1 channel 2 shifters")
        {
                RawData raw;
                raw.buffer = {
                        0b11000100, 0b11000100, 0b10000000, 0b10000000, // CH0 SHIFTBUF0
                        0b10001000, 0b00000000, 0b10001000, 0b00000000, // CH0 SHIFTBUF1

                        0b11000100, 0b11000100, 0b10000000, 0b10000000, // CH0 SHIFTBUF0
                        0b11001100, 0b01000100, 0b11001100, 0b01000100, // CH0 SHIFTBUF1

                        0b11100110, 0b11100110, 0b10100010, 0b10100010, // CH0 SHIFTBUF0
                        0b10001000, 0b00000000, 0b10001000, 0b00000000, // CH0 SHIFTBUF1

                        0b11100110, 0b11100110, 0b10100010, 0b10100010, // CH0 SHIFTBUF0
                        0b11001100, 0b01000100, 0b11001100, 0b01000100, // CH0 SHIFTBUF1
                };

                auto sd = rearrangeFlexio<1, 2> (raw);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{
                                 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, //
                                 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, //
                                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //
                                 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, //
                         });
        }

        SECTION ("1 channel 4 shifters")
        {
                RawData raw;
                raw.buffer = {
                        0b10001000, 0b10001000, 0b10001000, 0b10001000, // CH0 SHIFTBUF0
                        0b10100000, 0b10100000, 0b10100000, 0b10100000, // CH0 SHIFTBUF1
                        0b10101010, 0b00000000, 0b10101010, 0b00000000, // CH0 SHIFTBUF2
                        0b10101010, 0b10101010, 0b00000000, 0b00000000, // CH0 SHIFTBUF3

                        0b11011101, 0b11011101, 0b11011101, 0b11011101, // CH0 SHIFTBUF0
                        0b10100000, 0b10100000, 0b10100000, 0b10100000, // CH0 SHIFTBUF1
                        0b10101010, 0b00000000, 0b10101010, 0b00000000, // CH0 SHIFTBUF2
                        0b10101010, 0b10101010, 0b00000000, 0b00000000, // CH0 SHIFTBUF3
                };

                common::acq::Params params;
                params.digitalChannels = 1;
                params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
                auto sd = rearrange (raw, params);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{
                                 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, //
                                 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, //
                                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //
                                 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, //
                         });
        }

        // SECTION ("1 channel 4 shifters - 2")
        // {
        //         RawData raw;
        //         raw.buffer = {
        //                 0b10'10'10'10, 0b10'10'10'10, 0b00'00'00'00, 0b00'00'00'00, // CH0 SHIFTBUF0
        //                 0b10'10'10'10, 0b00'00'00'00, 0b10'10'10'10, 0b00'00'00'00, // CH0 SHIFTBUF1
        //                 0b10'10'00'00, 0b10'10'00'00, 0b10'10'00'00, 0b10'10'00'00, // CH0 SHIFTBUF2
        //                 0b10'00'10'00, 0b10'00'10'00, 0b10'00'10'00, 0b10'00'10'00, // CH0 SHIFTBUF3

        //                 0b10'10'10'10, 0b10'10'10'10, 0b00'00'00'00, 0b00'00'00'00, // CH1 SHIFTBUF0
        //                 0b10'10'10'10, 0b00'00'00'00, 0b10'10'10'10, 0b00'00'00'00, // CH1 SHIFTBUF1
        //                 0b10'10'00'00, 0b10'10'00'00, 0b10'10'00'00, 0b10'10'00'00, // CH1 SHIFTBUF2
        //                 0b11'01'11'01, 0b11'01'11'01, 0b11'01'11'01, 0b11'01'11'01, // CH1 SHIFTBUF3
        //         };

        //         common::acq::Params params;
        //         params.digitalChannels = 1;
        //         params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
        //         SampleData sd = rearrange (raw, params);

        //         REQUIRE (std::get<Bytes>(sd.at (0).buffer)
        //                  == Buffer{
        //                          0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, //
        //                          0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, //
        //                          0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //
        //                          0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, //
        //                  });
        // }

        SECTION ("1 channel 4 shifters - real use case")
        {
                RawData raw;
                raw.buffer = {
                        0b00111001, 0b11001110, 0b01110011, 0b10011100, // CH0 SHIFTBUF0
                        0b00111001, 0b11001110, 0b01110011, 0b10011100, // CH0 SHIFTBUF1
                        0b00110001, 0b10001100, 0b01100011, 0b00011000, // CH0 SHIFTBUF2
                        0b00110001, 0b10001100, 0b01100011, 0b00011000, // CH0 SHIFTBUF3

                        0b11001110, 0b01110011, 0b10011100, 0b11100111, // CH1 SHIFTBUF0
                        0b11001110, 0b01110011, 0b10011100, 0b11100111, // CH1 SHIFTBUF1
                        0b10001100, 0b01100011, 0b00011000, 0b11000110, // CH1 SHIFTBUF2
                        0b10001100, 0b01100011, 0b00011000, 0b11000110, // CH1 SHIFTBUF3
                };

                common::acq::Params params;
                params.digitalChannels = 1;
                params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
                auto sd = rearrange (raw, params);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{
                                 0b00000000, 0b00111111, 0b11110000, 0b00000011, 0b11111111, 0b00000000, 0b00111111, 0b11110000,
                                 0b00000011, 0b11111111, 0b00000000, 0b00111111, 0b11110000, 0b00000011, 0b11111111, 0b00000000,
                                 0b00111111, 0b11110000, 0b00000011, 0b11111111, 0b00000000, 0b00111111, 0b11110000, 0b00000011,
                                 0b11111111, 0b00000000, 0b00111111, 0b11110000, 0b00000011, 0b11111111, 0b00000000, 0b00111111,
                         });
        }

        /*
         * This configuration is not used in the firmware.
         */
        SECTION ("1 channel 8 shifters")
        {
                RawData raw;
                raw.buffer = {
                        0b10101010, 0b10101010, 0b10101010, 0b10101010, // CH0 SHIFTBUF0
                        0b11001100, 0b11001100, 0b11001100, 0b11001100, // CH0 SHIFTBUF1
                        0b11110000, 0b11110000, 0b11110000, 0b11110000, // CH0 SHIFTBUF2
                        0b11111111, 0b00000000, 0b11111111, 0b00000000, // CH0 SHIFTBUF3
                        0b11111111, 0b11111111, 0b00000000, 0b00000000, // CH0 SHIFTBUF4
                        0b00000000, 0b00000000, 0b00000000, 0b00000000, // CH0 SHIFTBUF5
                        0b00000000, 0b00000000, 0b00000000, 0b00000000, // CH0 SHIFTBUF6
                        0b00000000, 0b00000000, 0b00000000, 0b00000000, // CH0 SHIFTBUF7
                };

                auto sd = rearrangeFlexio<1, 8> (raw);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{
                                 0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, //
                                 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f, //
                                 0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17, //
                                 0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f, //
                         });
        }

        SECTION ("2 channels 2 shifters")
        {
                RawData raw;
                raw.buffer = {
                        0b11000100, 0b11000100, 0b10000000, 0b10000000, // CH0 SHIFTBUF0
                        0b10001000, 0b00000000, 0b10001000, 0b00000000, // CH0 SHIFTBUF1

                        0b11100110, 0b11100110, 0b10100010, 0b10100010, // CH1 SHIFTBUF0
                        0b10001000, 0b00000000, 0b10001000, 0b00000000, // CH1 SHIFTBUF1

                        0b11000100, 0b11000100, 0b10000000, 0b10000000, // CH0 SHIFTBUF0
                        0b11001100, 0b01000100, 0b11001100, 0b01000100, // CH0 SHIFTBUF1

                        0b11100110, 0b11100110, 0b10100010, 0b10100010, // CH1 SHIFTBUF0
                        0b11001100, 0b01000100, 0b11001100, 0b01000100, // CH1 SHIFTBUF1
                };

                common::acq::Params params;
                params.digitalChannels = 2;
                params.digitalEncoding = common::acq::DigitalChannelEncoding::flexio;
                auto sd = rearrange (raw, params);

                REQUIRE (std::get<Bytes> (sd.at (0).buffer)
                         == Bytes{
                                 0x00,
                                 0x01,
                                 0x02,
                                 0x03,
                                 0x04,
                                 0x05,
                                 0x06,
                                 0x07, //
                                 0x08,
                                 0x09,
                                 0x0a,
                                 0x0b,
                                 0x0c,
                                 0x0d,
                                 0x0e,
                                 0x0f, //
                         });

                REQUIRE (std::get<Bytes> (sd.at (1).buffer)
                         == Bytes{
                                 0x10,
                                 0x11,
                                 0x12,
                                 0x13,
                                 0x14,
                                 0x15,
                                 0x16,
                                 0x17, //
                                 0x18,
                                 0x19,
                                 0x1a,
                                 0x1b,
                                 0x1c,
                                 0x1d,
                                 0x1e,
                                 0x1f, //
                         });
        }
}
