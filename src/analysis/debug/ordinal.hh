/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "data/types.hh"

namespace logic {

/**
 * Checks for the consecutive integer number at the start of every
 * block that comes from the device. Numbers have to be consecutive,
 * one by one. Every deviation from this pattern is reported. Also It
 * expects the queue size in the second word (32bit) of every block,
 * and outputs it as well.
 */
class OrdinalCheck : public SingleChannelAnalyzer {
public:
        using SingleChannelAnalyzer::SingleChannelAnalyzer;

        void start () override
        {
                lastBufferNo = {};
                devBufferNo = 0;
                overrunsNo = 0;
        }

        /**
         * Basic data integrity (overflow) check. Buffers received from the device shall
         * contain some meta-data every deviceBufferLen bytes. We check if the counter
         * in this data increases one by one in every received buffer. If the increase is
         * more than 1m then we know we've lost some buffers. It is assumed, that
         * buffer.size () % deviceBufferLenB == 0. buffer has to be a std::vector, array etc;
         * A continious memory area.
         *
         * @param deviceBufferLenB DMA blocks on the device (IRQ after each block -> metadata
         * for each block).
         */
        AugumentedData run (ChannelBlockStream const &bmd) override;
        void stop () override;

private:
        std::optional<size_t> lastBufferNo;
        size_t devBufferNo{};
        size_t overrunsNo{};
};

} // namespace logic