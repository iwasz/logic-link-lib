/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "ordinal.hh"
#include <cstdio>
#include <gsl/gsl>
#include <print>
#include <vector>

namespace logic::an {
using namespace std::chrono;
using namespace std::string_literals;

/****************************************************************************/

data::AugumentedData OrdinalCheck::run (data::SampleBlockStream const &samples)
{
        // auto const &bytes = std::get<data::Bytes> (samples.at (channel ()).buffer);
        // auto dmaBlocksNum = bytes.size () / dmaBlockLenB (); // We assume % is 0.

        // for (size_t i = 0; i < dmaBlocksNum; ++i) {
        //         uint32_t bufferNo{};
        //         // uint32_t queueSize{};

        //         // Extract the meta data -> ordinal number from the beginning of the block.
        //         bufferNo = *reinterpret_cast<uint32_t const *> (bytes.data () + i * dmaBlockLenB ());
        //         // queueSize = *reinterpret_cast<uint32_t const *> (bytes.data () + i * dmaBlockLenB () + 4);

        //         if (lastBufferNo) {
        //                 if (auto diff = (int (bufferNo) - int (*lastBufferNo)); diff != 1) {
        //                         // std::println ("{}. bufferNo: {}, bps: {:.2f}, qSize: {}, txSize: {}, \u001b[31movr {}\u001b[0m",
        //                         devBufferNo,
        //                         //               bufferNo, 0U /* bmd.bps */, queueSize, bytes.size (), diff);

        //                         overrunsNo += (diff <= 0) ? (1) : (diff);
        //                 }
        //         }

        //         lastBufferNo = bufferNo;
        //         ++devBufferNo;
        // }

        return {};
}
/****************************************************************************/

void OrdinalCheck::stop ()
{
        if (overrunsNo > 0) {
                std::println ("\u001b[31mOverflows total: {}\u001b[0m", overrunsNo);
        }
        else {
                std::println ("Overflows total: 0");
        }
}

} // namespace logic::an
