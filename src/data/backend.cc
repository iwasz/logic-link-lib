/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <algorithm>
#include <mutex>
#include <ranges>
#include <vector>
module logic.data;
import logic.core;

namespace logic {

ChannelBlock::ChannelBlock (size_t channels, size_t numberOfSampl) : data (channels)
{
        for (auto &ch : data) {
                ch.resize (numberOfSampl);
        }
}

/****************************************************************************/

void ChannelBlockStream::append (std::vector<Bytes> &&channels)
{
        auto cb = ChannelBlock{currentSampleNo, std::move (channels)};
        currentSampleNo = currentSampleNo + cb.channelSize ();
        data_.push_back (std::move (cb));
}

/*--------------------------------------------------------------------------*/

ChannelBlockStream::SubRange ChannelBlockStream::range (SampleIdx const &begin, SampleIdx const &end)
{
        if (begin == end) {
                return {};
        }

        auto b = std::find_if (data_.cbegin (), data_.cend (),
                               [&begin] (ChannelBlock const &b) { return b.firstSampleNo () <= begin && b.lastSampleNo () >= begin; });

        auto e = std::find_if (b, data_.cend (),
                               [&end] (ChannelBlock const &b) { return b.firstSampleNo () <= end && b.lastSampleNo () >= end; });

        if (e != data_.cend ()) {
                std::advance (e, 1);
        }

        return {b, e};
}

/****************************************************************************/

void BlockBackend::append (size_t groupIdx, std::vector<Bytes> &&s)
{
        std::lock_guard lock{mutex};
        groups.at (groupIdx).append (std::move (s));
}

/*--------------------------------------------------------------------------*/

ChannelStream BlockBackend::range (size_t groupIdx, SampleIdx const &begin, SampleIdx const &end)
{
        std::lock_guard lock{mutex};
        auto &group = groups.at (groupIdx);
        auto subRangeBlocks = group.range (begin, end);

        if (subRangeBlocks.empty ()) {
                return {};
        }

        SampleNum totalLenToCopy = std::min<SampleNum> (end, subRangeBlocks.back ().lastSampleNo () + 1) - begin;
        ChannelStream trimmedSubrange{group.channelsNumber (), size_t (totalLenToCopy)};

        size_t cnt{};
        SampleNum outputStartOffset{};

        for (auto const &subRangeBlock : subRangeBlocks) {
                size_t channelNo{};
                // If first block, then offset != 0. For every other block offset == 0.
                auto inputStartOffset = (cnt++ == 0) ? (begin - subRangeBlock.firstSampleNo ()) : (0);

                auto inputLenToCopy = std::min<SampleNum> (totalLenToCopy, subRangeBlock.data.front ().size () - inputStartOffset);
                totalLenToCopy -= inputLenToCopy;

                for (auto const &channelInBlock : subRangeBlock.data) {
                        auto begin = std::next (channelInBlock.cbegin (), inputStartOffset);
                        auto end = std::next (begin, inputLenToCopy);
                        std::copy (begin, end, std::next (trimmedSubrange.data.at (channelNo++).begin (), outputStartOffset));
                }

                outputStartOffset += inputLenToCopy;
        }

        if (totalLenToCopy != 0) {
                throw Exception{"BlockBackend::range internal problem."};
        }

        return trimmedSubrange;
}

/*--------------------------------------------------------------------------*/

void BlockBackend::configureGroup (size_t groupIdx, SampleRate sampleRate)
{

        if (groups.size () <= groupIdx) {
                groups.resize (groupIdx + 1);
        }

        groups.at (groupIdx).setSampleRate (sampleRate);
}

} // namespace logic