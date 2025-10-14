/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <Tracy.hpp>
#include <algorithm>
#include <climits>
#include <cmath>
#include <mutex>
#include <ranges>
#include <vector>

module logic.data;
import logic.core;
import logic.processing;

namespace logic {

void Block::append (Block &&d)
{
        bitsPerSample_ = d.bitsPerSample_;
        append (std::move (d).data_);
}

/*--------------------------------------------------------------------------*/

void Block::append (Container &&d)
{
        if (d.size () != channelsNumber ()) {
                throw Exception{"Block::append: d.size () != channelsNumber ()"};
        }

        Container copy = std::move (d);
        for (std::tuple<Bytes &, Bytes const &> t : std::views::zip (data_, copy)) {
                Bytes &dest = std::get<0> (t);
                Bytes const &src = std::get<1> (t);

                dest.reserve (dest.size () + src.size ());
                std::ranges::copy (src, std::back_inserter (dest));
        }
}

/*--------------------------------------------------------------------------*/

void Block::reserve (size_t channels, SampleNum numberOfSampl)
{
        data_.resize (std::max (channels, data_.size ()));

        for (auto &ch : data_) {
                ch.reserve (numberOfSampl);
        }
}

/*--------------------------------------------------------------------------*/

SampleNum Block::channelLength () const
{
        if (data_.empty ()) {
                return 0;
        }

        auto const SAMPLES_PER_BYTE = CHAR_BIT / bitsPerSample_;
        return SampleNum (bytesUsed () * SAMPLES_PER_BYTE * zoomOut_);
}

/*--------------------------------------------------------------------------*/

size_t Block::bytesUsed () const
{
        if (data_.empty ()) {
                return 0;
        }

        return data_.front ().size ();
}

/*--------------------------------------------------------------------------*/

void Block::clear ()
{
        for (auto &ch : data_) {
                ch.clear ();
        }
}

/****************************************************************************/

template <std::integral auto factor> Block downsample (Block const &block)
{
        return {block.bitsPerSample (),
                block.data () | std::views::transform ([] (Bytes const &bts) -> Bytes { return downsample<factor> (bts); })
                        | std::ranges::to<Block::Container> ()};
}

/*--------------------------------------------------------------------------*/

Block downsample (Block const &block, size_t zoomOut)
{
        if (zoomOut == 1) {
                return block;
        }

        return {block.bitsPerSample (),
                block.data () | std::views::transform ([zoomOut] (Bytes const &bts) -> Bytes { return downsample (bts, zoomOut); })
                        | std::ranges::to<Block::Container> ()};
}

/****************************************************************************/

void BlockArray::append (uint8_t bitsPerSample, std::vector<Bytes> &&channels)
{
        if (channels.empty ()) {
                return;
        }

        size_t levNo = 0;
        auto const chLenBytes = channels.front ().size ();
        auto const SAMPLES_PER_BYTE = CHAR_BIT / bitsPerSample;
        auto const chLenBits = chLenBytes * SAMPLES_PER_BYTE;

        if (chLenBits == 0) {
                return;
        }

        auto doAppend = [chLenBytes, this] (Block block, size_t levNo, auto &that) -> void {
                block.zoomOut_ = std::pow (zoomOutPerLevel, levNo);

                if (levels.size () - 1 > levNo) {
                        Block zoomed = downsample (block, zoomOutPerLevel);
                        that (zoomed, levNo + 1, that);
                }

                auto &level = levels.at (levNo);

                if (level.data_.empty () || level.data_.back ().bytesUsed () >= chLenBytes) {
                        level.data_.emplace_back (std::move (block));
                        level.data_.back ().firstSampleNo () = channelLength_;
                }
                else {
                        level.data_.back ().append (std::move (block));
                }

                auto p = BlockIndex::value_type{level.data_.back ().lastSampleNo (), std::next (level.data_.cend (), -1)};

                /*
                 * index_ size and deta_ sizes have to be the same. So if index_ already
                 * contains the same number of entries, it means we have to modify it.
                 */
                if (level.index_.size () >= level.data_.size ()) {
                        level.index_.erase (std::next (level.index_.end (), -1)); // Last element has the highest key (lastSampleNo)
                }

                level.index_.insert (p);
        };

        Block block{bitsPerSample, std::move (channels), 1};
        doAppend (std::move (block), levNo, doAppend);

        channelLength_ += SampleNum (chLenBits);
}

/*--------------------------------------------------------------------------*/

BlockArray::SubRange BlockArray::range (SampleIdx begin, SampleIdx end, SampleNum maxDiscernibleSamples) const
{
        if (begin == end) {
                return {};
        }

        size_t len = end - begin;

        if (maxDiscernibleSamples < 0) {
                maxDiscernibleSamples = len;
        }
        else {
                maxDiscernibleSamples = std::min<SampleNum> (maxDiscernibleSamples, len);
        }

        auto zoomOut = std::bit_floor (len / maxDiscernibleSamples);

        auto lll = levels | std::views::filter ([zoomOut] (auto &lev) { return lev.zoomOut >= zoomOut; });
        auto &level = (std::ranges::empty (lll)) ? (levels.front ()) : (lll.front ());

        ZoneScoped;
        auto bi = level.index_.lower_bound (begin);

        if (bi == level.index_.cend ()) {
                return {};
        }

        ZoneNamedN (z1, "end", true);
        auto ei = level.index_.lower_bound (end);

        if (ei == level.index_.cend ()) {
                std::advance (ei, -1);
        }

        auto b = bi->second;
        auto e = ei->second;

        // Maintain "past-the-end" semantics.
        if (e != level.data_.cend ()) {
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
        // Stream trimmedSubrange{subRangeBlocks.front ().bitsPerSample (), channelsNumber (), totalLenToCopy};
        Block const &f = subRangeBlocks.front ();

        std::vector<Bytes> tmp (f.channelsNumber ());
        for (auto &ch : tmp) {
                ch.resize (totalLenToCopy);
        }

        Stream trimmedSubrange{f.bitsPerSample (), std::move (tmp)};

        size_t cnt{};
        SampleNum outputStartOffset{};

        for (auto const &subRangeBlock : subRangeBlocks) {
                size_t channelNo{};
                // If first block, then offset != 0. For every other block offset == 0.
                auto inputStartOffset = (cnt++ == 0) ? (begin - subRangeBlock.firstSampleNo ()) : (0);

                auto inputLenToCopy = std::min<SampleNum> (totalLenToCopy, subRangeBlock.data ().front ().size () - inputStartOffset);
                totalLenToCopy -= inputLenToCopy;

                for (auto const &channelInBlock : subRangeBlock.data ()) {
                        auto begin = std::next (channelInBlock.cbegin (), inputStartOffset);
                        auto end = std::next (begin, inputLenToCopy);
                        std::copy (begin, end, std::next (trimmedSubrange.data_.at (channelNo++).begin (), outputStartOffset));
                }

                outputStartOffset += inputLenToCopy;
        }

        if (totalLenToCopy != 0) {
                throw Exception{"BlockBackend::range internal problem."};
        }

        return trimmedSubrange;
}

/*--------------------------------------------------------------------------*/

void BlockArray::clear ()
{
        for (auto &levels : levels) {
                for (auto &blck : levels.data_) {
                        blck.clear ();
                }
        }

        channelLength_ = 0;
}

/****************************************************************************/

void Backend::append (size_t groupIdx, uint8_t bitsPerSample, std::vector<Bytes> &&s)
{
        {
                std::lock_guard lock{mutex};
                groups.at (groupIdx).append (bitsPerSample, std::move (s));
        }

        notifyObservers ();
}
/*--------------------------------------------------------------------------*/

void Backend::clear ()
{
        {
                std::lock_guard lock{mutex};

                for (auto &g : groups) {
                        g.clear ();
                }
        }

        notifyObservers ();
}

/*--------------------------------------------------------------------------*/

Backend::SubRange Backend::range (size_t groupIdx, SampleIdx begin, SampleIdx end, SampleNum maxDiscernibleSamples) const
{
        ZoneScopedN ("BackendRange");
        /*
         * Only groups and a paricular group (BlockArray) is protected. But
         * the blocks thselves are not. They are returned by const_iterators
         * though, so won't get modified by this thread. Any othe thread
         * won't modify them as well, because once tha data is read from a
         * device, there's no need to modifu it.
         */
        std::lock_guard lock{mutex};
        return groups.at (groupIdx).range (begin, end, maxDiscernibleSamples);
}

/*--------------------------------------------------------------------------*/

void Backend::configureGroup (size_t groupIdx, SampleRate /* sampleRate */)
{
        std::lock_guard lock{mutex};

        if (groups.size () <= groupIdx) {
                groups.resize (groupIdx + 1);
        }

        // groups.at (groupIdx).setSampleRate (sampleRate);
}

/*--------------------------------------------------------------------------*/

SampleNum Backend::channelLength (size_t groupIdx) const
{
        std::lock_guard lock{mutex};
        return groups.at (groupIdx).channelLength ();
}

/*--------------------------------------------------------------------------*/

void Backend::notifyObservers ()
{
        for (auto *o : observers) {
                o->onNewData ();
        }
}

} // namespace logic