/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "flexioSynchro.hh"
#include <cstdio>
#include <gsl/gsl>
#include <print>

namespace logic::an {

/****************************************************************************/

data::AugumentedData FlexioSynchronizationCheck::run (data::SampleBlockStream const &samples)
{
        // auto const &bytes = std::get<data::Bytes> (samples.at (channel ()).buffer);
        // auto dmaBlocksNum = bytes.size () / dmaBlockLenB (); // We assume % is 0.

        // for (size_t i = 0; i < dmaBlocksNum; ++i) {
        //         std::span<const uint32_t> block{reinterpret_cast<uint32_t const *> (bytes.data () + i * dmaBlockLenB ()), dmaBlockLenB () /
        //         4}; analyzeDataIntegrityBlock (block);
        //         ++devBlockNo;
        // }

        return {};
}

/****************************************************************************/

size_t FlexioSynchronizationCheck::analyzeDataIntegrityBlock (std::span<const uint32_t> const &block)
{
        size_t errorNum{};

        for (size_t i = 0; i < block.size (); i += 4) {
                uint32_t w0 = block[i];
                uint32_t w1 = block[i + 1];
                uint32_t w2 = block[i + 2];
                uint32_t w3 = block[i + 3];

                if (w0 != w1 || w1 != w2 || w2 != w3) {
                        ++errorNum;
                        std::println ("{}:{}. f0: {:032b}, f1: {:032b}, f2: {:032b}, f3: {:032b}", devBlockNo, i / 4, w0, w1, w2, w3);
                }
        }

        return errorNum;
}

/****************************************************************************/

void FlexioSynchronizationCheck::stop () {}

} // namespace logic::an
