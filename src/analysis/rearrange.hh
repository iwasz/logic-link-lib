/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "common/params.hh"
#include "types.hh"

namespace logic::an {

/**
 * Rearrange the byte and/or bit order from raw device data format to per-channel
 * order (chanel -> stream of bits). Due to hardware and signal itegrity reasons,
 * the data coming from the device is "encoded" in a certain way depending on the
 * speed and numebr of chhannels. For some settings only byte reordering is needed,
 * for some (the fastest transfers) also bits in the input bytes have to be reordered
 * as well.
 */
data::SampleData rearrange (data::RawData const &rd, common::acq::Params const &params);

/**
 * Configurable rearrange algorithm. Moved to a header file to simplify unit testing.
 * This algorithm works on the per-bit basis meaning it moves/shuifts bits in every byte
 * to their respective locations.
 * TIP: see the unit test.
 * TIP2: see the "loop-unrolled" versions in the cc file. They are easier to understand.
 *
 * Lets label a byte acquired by CH0 as 'B0', CH1 -> B1 etc. 32bit word from
 * a channel will be named W0, W1 etc. Bit from a flexio PIN for channel 0 will be denoted
 * as b0[0], b1[0] etc. For channel 1 as: b0[1], b1[2] ... etc
 *
 * For single channel (CH0 only) encoded in the flexio fashion the data comes
 * in the following format:
 *
 * b0[0] b0[1] b0[2] ... b0[31]
 * b1[0] b1[1] b1[2] ... b1[31]
 * b2[0] b2[1] b2[2] ... b2[31]
 * b3[0] b3[1] b3[2] ... b3[31]
 *
 * We want to rearrange it to:
 *
 * b0[0]  b1[0]  b2[0]  b3[0]  b0[1]  b1[1]  b2[1]  b3[1]
 * b0[2]  b1[2]  b2[2]  b3[2]  b0[3]  b1[3]  b2[3]  b3[3]
 * ...
 * b0[30] b1[30] b2[30] b3[30] b0[31] b1[31] b2[31] b3[31]
 */
template <size_t CHANNELS_NUM, size_t SHIFTBUFS_PER_CH_NUM> data::SampleData rearrangeFlexio (data::RawData const &rd)
{
        data::SampleData sd;
        sd.digital.resize (CHANNELS_NUM);
        std::array<data::Buffer::iterator, SHIFTBUFS_PER_CH_NUM> outI;

        size_t i{};
        for (auto &ch : sd.digital) {
                ch.resize (rd.buffer.size () / CHANNELS_NUM);
                outI[i] = ch.begin ();
                i = (i + 1) % CHANNELS_NUM;
        }

        auto inI = rd.buffer.begin ();

        constexpr size_t BYTES_PER_BATCH = sizeof (uint32_t) * SHIFTBUFS_PER_CH_NUM;
        constexpr uint8_t mask = 0b1000'0000;

        while (std::distance (inI, rd.buffer.cend ()) > 0) {
                for (size_t m = 0; m < CHANNELS_NUM; ++m) {
                        auto in = std::span{inI, BYTES_PER_BATCH};
                        auto out = std::span{outI[m], BYTES_PER_BATCH};
                        std::ranges::fill (out, 0x00);

                        for (size_t k = 0; k < (CHAR_BIT / 2); ++k) {
                                for (size_t j = 0; j < CHAR_BIT; ++j) {

                                        // Normalize to 0bX000'0000 using only shift left
                                        for (size_t l = 0; l < SHIFTBUFS_PER_CH_NUM; ++l) {
                                                auto inIdx = (4 * (SHIFTBUFS_PER_CH_NUM - l - 1)) + (CHAR_BIT / 2) - 1 - k;
                                                auto sh = (CHAR_BIT - j - 1);
                                                uint8_t n = in[inIdx] << sh;
                                                uint8_t nibble = (j % (CHAR_BIT / SHIFTBUFS_PER_CH_NUM)) * SHIFTBUFS_PER_CH_NUM;

                                                // Check the bit comparing with the mask, and place on the right spot in the byte.
                                                auto outIdx = (k * SHIFTBUFS_PER_CH_NUM) + (j / (CHAR_BIT / SHIFTBUFS_PER_CH_NUM));
                                                out[outIdx] |= (n & mask) >> (l + nibble);
                                        }
                                }
                        }

                        std::advance (outI[m], int (BYTES_PER_BATCH));
                        std::advance (inI, int (BYTES_PER_BATCH));
                }
        }

        return sd;
}

/*
 * Rearrange algorithm that doesn't reorder bits in a byte. Only bytes are moved to
 * respective channel collections (one vector per channel). Input data looks like this:
 * CH0 4B, CH1 4B, ..., CH7 4B.
 */
template <size_t CHANNELS_NUM> data::SampleData rearrangeFlexio (data::RawData const &rd)
{
        data::SampleData sd;
        sd.digital.resize (CHANNELS_NUM);

        for (auto &ch : sd.digital) {
                ch.reserve (rd.buffer.size () / CHANNELS_NUM);
        }

        size_t byteCounter{};
        size_t channelCounter{};
        for (uint8_t b : rd.buffer) {
                data::Buffer &buf = sd.digital[channelCounter];

                buf.push_back (b);

                if (++byteCounter >= sizeof (uint32_t)) {
                        byteCounter = 0;
                        ++channelCounter;

                        if (channelCounter >= CHANNELS_NUM) {
                                channelCounter = 0;
                        }
                }
        }

        return sd;
}

} // namespace logic::an