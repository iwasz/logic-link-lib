/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include "exception.hh"
#include <algorithm>
#include <chrono>
#include <climits>
#include <cstdint>
#include <span>
#include <variant>
#include <vector>
module logic;

namespace logic::an {
using namespace data;

namespace {

        /**
         * This is (early-stage) rearrange algorith for 1 channel that uses 4 shifters.
         * It has only 2 nested loops instead of 5 as the generic one has. It was left
         * in this file to experiment with the optimization in the future. Below I present
         * a helpful description that helps to understand the for loop counters:
         * - i counts the bytes. Single iteration of the outermost loop (controlled by i)
         *   concerns one FLEXIO trogger event, that gives us (in the fastest setting which
         *   is for CH0 with 4 shifters) 4x 32 bits == 16B.
         *
         *                          k==3        k==2        k==1        k==0
         *
         *                       0b00111001, 0b11001110, 0b01110011, 0b10011100, // CH0 SHIFTBUF0
         * i == 0                0b00111001, 0b11001110, 0b01110011, 0b10011100, // CH0 SHIFTBUF1
         *                       0b00110001, 0b10001100, 0b01100011, 0b00011000, // CH0 SHIFTBUF2
         *                       0b00110001, 0b10001100, 0b01100011, 0b00011000, // CH0 SHIFTBUF3
         *
         *                       0b11001110, 0b01110011, 0b10011100, 0b11100111, // CH1 SHIFTBUF0
         * i == BYTES_PER_BATCH  0b11001110, 0b01110011, 0b10011100, 0b11100111, // CH1 SHIFTBUF1
         * (i == 16)             0b10001100, 0b01100011, 0b00011000, 0b11000110, // CH1 SHIFTBUF2
         *                       0b10001100, 0b01100011, 0b00011000, 0b11000110, // CH1 SHIFTBUF3
         *
         *                                                              C C C C
         *                                                              o o o o
         *                                                              l l l l
         *                                                              u u u u
         *                                                              m m m m
         *                                                              n n n n
         *
         *                                                              D C B A
         */
        std::vector<data::SampleBlock> rearrangeFlexio1a (RawData const &rd)
        {
                constexpr size_t CHANNELS_NUM = 1;
                constexpr size_t SHIFTBUFS_PER_CH_NUM = 4;
                constexpr size_t BYTES_PER_BATCH = sizeof (uint32_t) * CHANNELS_NUM * SHIFTBUFS_PER_CH_NUM;

                auto digital = prepareDigitalBlocks (rd, CHANNELS_NUM);
                auto outI = std::get<data::Bytes> (digital.at (0).buffer).begin ();

                for (size_t i = 0; i < rd.buffer.size (); i += BYTES_PER_BATCH) {
                        auto in = std::span{std::next (rd.buffer.begin (), int (i)), BYTES_PER_BATCH};
                        auto out = std::span{outI, BYTES_PER_BATCH};

                        for (size_t k = 0; k < (CHAR_BIT / 2); ++k) {
                                size_t n = (CHAR_BIT / 2) - 1 - k;
                                uint8_t mask1 = 1 << 0;
                                uint8_t mask2 = 1 << 1;

                                // Column A
                                out[k * SHIFTBUFS_PER_CH_NUM + 0] = (in[12 + n] & mask1) << 7 | (in[8 + n] & mask1) << 6
                                        | (in[4 + n] & mask1) << 5 | (in[0 + n] & mask1) << 4 | (in[12 + n] & mask2) << 2
                                        | (in[8 + n] & mask2) << 1 | (in[4 + n] & mask2) << 0 | (in[0 + n] & mask2) >> 1;

                                // Column B
                                mask1 = 1 << 2;
                                mask2 = 1 << 3;
                                out[k * SHIFTBUFS_PER_CH_NUM + 1] = (in[12 + n] & mask1) << 5 | (in[8 + n] & mask1) << 4
                                        | (in[4 + n] & mask1) << 3 | (in[0 + n] & mask1) << 2 | (in[12 + n] & mask2) | (in[8 + n] & mask2) >> 1
                                        | (in[4 + n] & mask2) >> 2 | (in[0 + n] & mask2) >> 3;

                                // Column C
                                mask1 = 1 << 4;
                                mask2 = 1 << 5;
                                out[k * SHIFTBUFS_PER_CH_NUM + 2] = (in[12 + n] & mask1) << 3 | (in[8 + n] & mask1) << 2
                                        | (in[4 + n] & mask1) << 1 | (in[0 + n] & mask1) << 0 | (in[12 + n] & mask2) >> 2
                                        | (in[8 + n] & mask2) >> 3 | (in[4 + n] & mask2) >> 4 | (in[0 + n] & mask2) >> 5;

                                // Column D
                                mask1 = 1 << 6;
                                mask2 = 1 << 7;
                                out[k * SHIFTBUFS_PER_CH_NUM + 3] = (in[12 + n] & mask1) << 1 | (in[8 + n] & mask1) >> 0
                                        | (in[4 + n] & mask1) >> 1 | (in[0 + n] & mask1) >> 2 | (in[12 + n] & mask2) >> 4
                                        | (in[8 + n] & mask2) >> 5 | (in[4 + n] & mask2) >> 6 | (in[0 + n] & mask2) >> 7;
                        }

                        std::advance (outI, int (BYTES_PER_BATCH));
                }

                return digital;
        }

        /**
         * The same algorithm as the above, but "rolled" to 3 nested loops instead of 2.
         */
        std::vector<data::SampleBlock> rearrangeFlexio1b (RawData const &rd)
        {
                constexpr size_t CHANNELS_NUM = 1;
                constexpr size_t SHIFTBUFS_PER_CH_NUM = 4;
                constexpr size_t BYTES_PER_BATCH = sizeof (uint32_t) * CHANNELS_NUM * SHIFTBUFS_PER_CH_NUM;

                auto digital = prepareDigitalBlocks (rd, CHANNELS_NUM);
                auto outI = std::get<data::Bytes> (digital.at (0).buffer).begin ();

                using Batch = std::array<uint8_t, SHIFTBUFS_PER_CH_NUM>;

                for (size_t i = 0; i < rd.buffer.size (); i += BYTES_PER_BATCH) {
                        auto in = std::span{std::next (rd.buffer.begin (), int (i)), BYTES_PER_BATCH};
                        auto out = std::span{outI, BYTES_PER_BATCH};

                        for (size_t k = 0; k < (CHAR_BIT / 2); ++k) {
                                for (size_t j = 0; j < CHAR_BIT; j += 2) {
                                        constexpr uint8_t mask = 0b1000'0000;
                                        size_t sh = CHAR_BIT - j - 1;
                                        size_t n = (CHAR_BIT / 2) - 1 - k;

                                        // Normalize to 0bX000'0000 using only shift left
                                        Batch n1{uint8_t (in[12 + n] << sh), uint8_t (in[8 + n] << sh), uint8_t (in[4 + n] << sh),
                                                 uint8_t (in[0 + n] << sh)};

                                        Batch n2{uint8_t (in[12 + n] << (sh - 1)), uint8_t (in[8 + n] << (sh - 1)),
                                                 uint8_t (in[4 + n] << (sh - 1)), uint8_t (in[0 + n] << (sh - 1))};

                                        // Check the bit comparing with the mask, and place on the right spot in the byte.
                                        out[k * SHIFTBUFS_PER_CH_NUM + j / 2] = (n1[0] & mask) >> 0 | (n1[1] & mask) >> 1 | (n1[2] & mask) >> 2
                                                | (n1[3] & mask) >> 3 | (n2[0] & mask) >> 4 | (n2[1] & mask) >> 5 | (n2[2] & mask) >> 6
                                                | (n2[3] & mask) >> 7;
                                }
                        }

                        std::advance (outI, int (BYTES_PER_BATCH));
                }

                return digital;
        }
} // namespace

/****************************************************************************/

inline std::vector<SampleBlock> rearrangeFlexio (RawData const &rd, common::acq::Params const &params)
{
        switch (params.digitalChannels) {
        case 1:
                return rearrangeFlexio<1, 4> (rd);
                // return rearrangeFlexio1a (rd);
                // return rearrangeFlexio1b (rd);
        case 2:
                return rearrangeFlexio<2, 2> (rd);
        case 4:
                return rearrangeFlexio<4> (rd);
        case 8:
                return rearrangeFlexio<8> (rd);
        default:
                throw Exception{"Wrong channel number for flexio rearrange."};
        }
}

std::vector<SampleBlock> rearrangeGpio1_2 (RawData const &rd, common::acq::Params const &params) { return {}; }

/**
 * Rearrange the device speciffic data format into SampleData
 */
std::vector<SampleBlock> rearrange (RawData const &rd, common::acq::Params const &params)
{
        using enum common::acq::DigitalChannelEncoding;
        using enum common::acq::AnalogChannelEncoding;

        if (params.digitalChannels > 0) {
                if (params.digitalEncoding == flexio) {
                        return rearrangeFlexio (rd, params);
                }
                if (params.digitalEncoding == gpio1_2) {
                        return rearrangeGpio1_2 (rd, params);
                }
                throw Exception{"Unknown digital channel data encoding."};
        }
        if (params.analogChannels > 0) {
                if (params.analogEncoding == analog8bit) {
                        return {};
                }

                throw Exception{"Unknown analog channel data encoding."};
        }

        return {};
}

/****************************************************************************/

std::vector<data::SampleBlock> prepareDigitalBlocks (data::RawData const &rd, size_t channelsNum, bool resize)
{
        std::vector<data::SampleBlock> digital (channelsNum);

        data::SampleBlock sb = {.type = data::StreamType::digital,
                                .sampleRate = 0,                                     // TODO
                                .begin = std::chrono::high_resolution_clock::now (), // TODO
                                .end = std::chrono::high_resolution_clock::now (),   // TODO
                                .buffer = data::Bytes{}};

        std::ranges::fill (digital, sb);

        for (auto &ss : digital) {
                auto &bb = std::get<data::Bytes> (ss.buffer);
                auto bbsiz = rd.buffer.size () / channelsNum;

                if (resize) {
                        bb.resize (bbsiz);
                }
                else {
                        bb.reserve (bbsiz);
                }
        }

        return digital;
}

} // namespace logic::an