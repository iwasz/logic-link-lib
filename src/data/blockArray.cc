/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <Tracy.hpp>
#include <algorithm>
#include <climits>
#include <cmath>
#include <format>
#include <memory>
#include <ranges>
#include <vector>
module logic.data;
import logic.core;
import logic.processing;

namespace logic {

BlockArray::BlockArray (size_t channelsNumber, size_t maxZoomOutLevels, size_t zoomOutPerLevel)
    : levels (std::max (maxZoomOutLevels, 1uz)), zoomOutPerLevel_{zoomOutPerLevel}
{
        for (size_t curZoomOut = 1; auto &lev : levels) {
                lev.zoomOut = curZoomOut;
                curZoomOut *= zoomOutPerLevel_;
                lev.downSamplers.resize (channelsNumber);

                for (std::unique_ptr<IDownSampler> &ds : lev.downSamplers) {
                        ds = std::make_unique<DigitalDownSampler> ();
                }
        }
}

/****************************************************************************/

Block BlockArray::downsample (Block const &block, size_t zoomOut, DownSamplers const &downSamplers) const
{
        if (zoomOut == 1) {
                return block;
        }

        return {block.bitsPerSample (),
                std::views::zip_transform ([zoomOut] (Bytes const &channel, std::unique_ptr<IDownSampler> const &downSampler)
                                                   -> Bytes { return (*downSampler) (channel, zoomOut); },
                                           block.data (), downSamplers)
                        | std::ranges::to<Block::Container> ()};
}

/****************************************************************************/

void BlockArray::append (uint8_t bitsPerSample, std::vector<Bytes> &&channels)
{
        if (channels.empty ()) {
                return;
        }

        if (auto currentSize = channels.size () * channels.front ().size (); currentSize != blockSizeB_) {
                throw Exception{std::format ("Block size mismatch. Allowed: {} != provided: {}", blockSizeB_, currentSize)};
        }

        auto const multiBlockBytes = blockSizeB_ * blockSizeMultiplier_;
        auto const SAMPLES_PER_BYTE = CHAR_BIT / bitsPerSample;
        auto const chLenBits = channels.front ().size () * SAMPLES_PER_BYTE;

        if (chLenBits == 0) {
                return;
        }

        auto doAppend = [multiBlockBytes, this] (Block block, size_t levNo, auto &that) -> void {
                block.zoomOut_ = std::pow (zoomOutPerLevel_, levNo);

                auto &level = levels.at (levNo);

                if (levels.size () - 1 > levNo) {
                        Block zoomed = downsample (block, zoomOutPerLevel_, level.downSamplers);
                        that (std::move (zoomed), levNo + 1, that);
                }

                if (level.data_.empty () || level.data_.back ().channelBytes () * level.data_.back ().channelsNumber () >= multiBlockBytes) {
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

        // Collect multiBlockBytes (blockSizeB_ * blockSizeMultiplier_) bytes of data, so the downsampling algorithms hev enough data to work on.
        pendingBlock.append (Block{bitsPerSample, std::move (channels)});

        if (pendingBlock.channelBytes () * pendingBlock.channelsNumber () >= multiBlockBytes) {
                auto tmp = pendingBlock.channelLength ();
                doAppend (std::move (pendingBlock), 0, doAppend);
                // channelLength_ += SampleNum (chLenBits);
                channelLength_ += tmp;
                pendingBlock = Block{};
        }

        // channelLength_ += SampleNum (chLenBits);
}

/****************************************************************************/

// TODO I confused end with length at least once. Maybe it's time to make SampleIdx "strongly typed" with explicit ctors?
// TODO like here: https://www.fluentcpp.com/2016/12/08/strong-types-for-strong-interfaces/
BlockArray::SubRange BlockArray::range (SampleIdx begin, SampleIdx end, size_t zoomOut, bool peek) const
{
        if (begin == end) {
                return {};
        }

        auto lll = levels | std::views::reverse | std::views::filter ([zoomOut] (auto &lev) { return lev.zoomOut <= zoomOut; });
        auto const &level = (std::ranges::empty (lll)) ? (levels.front ()) : (lll.front ());

        if (peek) {
                begin -= level.zoomOut;
        }

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

/****************************************************************************/

void BlockArray::clear ()
{
        for (auto &levels : levels) {
                for (auto &blck : levels.data_) {
                        blck.clear ();
                }
        }

        channelLength_ = 0;
}

} // namespace logic