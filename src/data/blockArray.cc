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
                throw Exception{"`downsample` with `zoomOut == 1` is not supported"};
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
        if (channels.empty () || channels.front ().empty ()) {
                return;
        }

        if (auto currentSize = channels.size () * channels.front ().size (); currentSize != blockSizeB_) {
                throw Exception{std::format ("Block size mismatch. Allowed: {} != provided: {}", blockSizeB_, currentSize)};
        }

        // Number of bytes that can be safely digested by a downsample algorithm.
        auto const multiBlockSizeB = blockSizeB_ * blockSizeMultiplier_;
        static auto blockB = [] (Block const &blk) { return blk.channelBytes () * blk.channelsNumber (); };

        auto doAppend = [multiBlockSizeB, this] (Block block, size_t levNo, auto &that) -> void {
                block.zoomOut_ = size_t (std::pow (zoomOutPerLevel_, levNo));
                auto &level = levels.at (levNo);
                auto &data = level.data_;

                if (levels.size () - 1 > levNo) {
                        Block zoomed = downsample (block, zoomOutPerLevel_, level.downSamplers);
                        that (std::move (zoomed), levNo + 1, that);
                }
                // We start fresh, OR last block in this level is `multiBlockSizeB` bytes.
                if (data.empty () || blockB (data.back ()) >= multiBlockSizeB) {
                        data.emplace_back (std::move (block));
                        data.back ().firstSampleNo () = channelLength_;
                }
                else {
                        data.back ().append (std::move (block));
                }

                auto p = BlockIndex::value_type{data.back ().lastSampleNo (), std::next (data.cend (), -1)};

                /*
                 * index_ size and data_ sizes have to be the same. So if index_ already
                 * contains the same number of entries, it means we have to modify it.
                 */
                if (level.index_.size () >= data.size ()) {
                        level.index_.erase (std::next (level.index_.end (), -1)); // Last element has the highest key (lastSampleNo)
                }

                level.index_.insert (p);
        };

        // Collect multiBlockBytes (blockSizeB_ * blockSizeMultiplier_) bytes of data, so the downsampling algorithms hev enough data to work on.
        pendingBlock.append (Block{bitsPerSample, std::move (channels)});

        if (blockB (pendingBlock) >= multiBlockSizeB) {
                auto tmp = pendingBlock.channelLength ();
                doAppend (std::move (pendingBlock), 0, doAppend);
                channelLength_ += tmp;
                pendingBlock = Block{};
        }
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