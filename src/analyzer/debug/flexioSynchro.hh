/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "types.hh"

namespace logic::an {

/**
 * Analyze the stream, assume there are 4 flexio streams, 32bit each like this:
 *
 * 0x00 | flexio0-32bits | flexio1-32bits | flexio2-32bits | flexio3-32bits |
 * 0x10 | flexio0-32bits | flexio1-32bits | flexio2-32bits | flexio3-32bits |
 * ...
 *
 * Assume that all 4 flexio inputs are connected (physically by wire) together,
 * thus all 4 blocks staring every 16 bytes shall have the same value. Print or
 * store errors if they don't.
 */
class FlexioSynchronizationCheck : public AbstractCheck {
public:
        using AbstractCheck::AbstractCheck;

        void start () override {}

        void run (data::SampleData const &samples) override;
        void stop () override;

private:
        size_t analyzeDataIntegrityBlock (std::span<const uint32_t> const &block);

        size_t devBlockNo{};
};

} // namespace logic::an