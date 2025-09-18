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
#include <climits>
#include <mutex>
#include <ranges>
#include <vector>
module logic.data;
import logic.core;

namespace logic {

Block::Block (uint8_t bitsPerSample, size_t channels, size_t numberOfSampl) : bitsPerSample_{bitsPerSample}, data_ (channels)
{
        for (auto &ch : data_) {
                ch.resize (numberOfSampl);
        }
}

/*--------------------------------------------------------------------------*/

SampleNum Block::channelSize () const
{
        if (data_.empty ()) {
                return 0;
        }

        auto const SAMPLES_PER_BYTE = CHAR_BIT / bitsPerSample_;
        return SampleNum (data_.front ().size ()) * SAMPLES_PER_BYTE;
}

/****************************************************************************/

void BlockArray::append (uint8_t bitsPerSample, std::vector<Bytes> &&channels)
{
        auto cb = Block{bitsPerSample, std::move (channels)};
        cb.firstSampleNo () = currentSampleNo;
        currentSampleNo = currentSampleNo + cb.channelSize ();
        data_.push_back (std::move (cb));
}

/*--------------------------------------------------------------------------*/

BlockArray::SubRange BlockArray::range (SampleIdx begin, SampleIdx end)
{
        if (begin == end) {
                return {};
        }

        auto b = std::find_if (data_.cbegin (), data_.cend (), [&begin] (Block const &b) {
                return b.channelSize () > 0 && b.firstSampleNo () <= begin && b.lastSampleNo () >= begin;
        });

        auto e = std::find_if (b, data_.cend (), [&end] (Block const &b) {
                return b.channelSize () > 0 && b.firstSampleNo () <= end && b.lastSampleNo () >= end;
        });

        if (e != data_.cend ()) {
                std::advance (e, 1);
        }

        return {b, e};
}

/*--------------------------------------------------------------------------*/

Stream BlockArray::clipBytes (ByteIdx begin, ByteIdx end)
{
        /*
         * This method was implemented only for 8 bit samples which makes it
         * less universal and useful. It is not used anywhere (?).
         */
        auto subRangeBlocks = range (begin, end);

        if (subRangeBlocks.empty ()) {
                return {};
        }

        SampleNum totalLenToCopy = std::min<SampleNum> (end, subRangeBlocks.back ().lastSampleNo () + 1) - begin;
        Stream trimmedSubrange{subRangeBlocks.front ().bitsPerSample (), channelsNumber (), size_t (totalLenToCopy)};

        size_t cnt{};
        SampleNum outputStartOffset{};

        for (auto const &subRangeBlock : subRangeBlocks) {
                size_t channelNo{};
                // If first block, then offset != 0. For every other block offset == 0.
                auto inputStartOffset = (cnt++ == 0) ? (begin - subRangeBlock.firstSampleNo ()) : (0);

                auto inputLenToCopy = std::min<SampleNum> (totalLenToCopy, subRangeBlock.data().front ().size () - inputStartOffset);
                totalLenToCopy -= inputLenToCopy;

                for (auto const &channelInBlock : subRangeBlock.data()) {
                        auto begin = std::next (channelInBlock.cbegin (), inputStartOffset);
                        auto end = std::next (begin, inputLenToCopy);
                        std::copy (begin, end, std::next (trimmedSubrange.data_.at (channelNo++).begin(), outputStartOffset));
                }

                outputStartOffset += inputLenToCopy;
        }

        if (totalLenToCopy != 0) {
                throw Exception{"BlockBackend::range internal problem."};
        }

        return trimmedSubrange;
}

/****************************************************************************/

void Backend::append (size_t groupIdx, uint8_t bitsPerSample, std::vector<Bytes> &&s)
{
        std::lock_guard lock{mutex};
        groups.at (groupIdx).append (bitsPerSample, std::move (s));
}

/*--------------------------------------------------------------------------*/

Stream Backend::range (size_t groupIdx, SampleIdx begin, SampleIdx end)
{
        std::lock_guard lock{mutex};
        Block copy{groups.at (groupIdx).range (begin, end)};
        return copy;
}

/*--------------------------------------------------------------------------*/

void Backend::configureGroup (size_t groupIdx, SampleRate sampleRate)
{
        if (groups.size () <= groupIdx) {
                groups.resize (groupIdx + 1);
        }

        groups.at (groupIdx).setSampleRate (sampleRate);
}

} // namespace logic