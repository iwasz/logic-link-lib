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
#include <condition_variable>
#include <mutex>
#include <ranges>
#include <vector>
module logic.data;
import logic.core;
import logic.processing;

namespace logic {

void Backend::append (size_t groupIdx, std::vector<Bytes> &&s)
{
        {
                std::lock_guard lock{mutex};
                groups_.at (groupIdx).append (std::move (s));
        }

        cvVar.notify_all ();
        notifyObservers ();
}

/*--------------------------------------------------------------------------*/

void Backend::clear ()
{
        {
                std::lock_guard lock{mutex};

                for (auto &g : groups_) {
                        g.clear ();
                }
        }

        notifyObservers ();
}

/*--------------------------------------------------------------------------*/

Backend::SubRange Backend::range (size_t groupIdx, SampleIdx begin, SampleIdx end, size_t zoomOut, bool peek) const
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
        auto mysr = sampleRate (groupIdx);
        return groups_.at (groupIdx).range (resample (begin, mysr), resample (end, mysr), zoomOut, peek);
}

/*--------------------------------------------------------------------------*/

Backend::SubRange Backend::range (size_t groupIdx, SampleIdx begin, SampleNum len, size_t zoomOut, bool peek) const
{
        ZoneScopedN ("BackendRange");
        std::lock_guard lock{mutex};
        auto mysr = sampleRate (groupIdx);
        return groups_.at (groupIdx).range (resample (begin, mysr), resample (begin + len, mysr), zoomOut, peek);
}

/*--------------------------------------------------------------------------*/

size_t Backend::addGroup (Group const &config)
{
        std::lock_guard lock{mutex};
        groups_.emplace_back (config.channelsNumber, config.sampleRate, config.bitsPerSample, config.maxZoomOutLevels, config.zoomOutPerLevel);
        auto &g = groups_.back ();
        g.setBlockSizeB (config.blockSizeB);
        g.setBlockSizeMultiplier (config.blockSizeMultiplier);

        auto res = std::ranges::max (groups_ | std::views::transform ([] (auto const &blockArray) { return blockArray.sampleRate ().get (); })
                                             | std::views::enumerate,
                                     [] (auto const &a, auto const &b) { return std::get<1> (a) < std::get<1> (b); });

        fastestGroup_ = std::get<0> (res);
        // maxSampleRate_ = SampleRate{std::get<1> (res)};

        return groups_.size () - 1;
}

/*--------------------------------------------------------------------------*/

SampleNum Backend::channelLength (size_t groupIdx) const
{
        std::lock_guard lock{mutex};
        return groups_.at (groupIdx).channelLength ();
}

/*--------------------------------------------------------------------------*/

SampleNum Backend::waitLength (size_t groupIdx, SampleNum const &len) const
{
        std::unique_lock lock{mutex};

        cvVar.wait (lock, [this, len, groupIdx] {
                auto actl = groups_.at (groupIdx).channelLength ();
                srcheck (actl, len);
                return actl.get () > len.get ();
        });

        return groups_.at (groupIdx).channelLength () - len;
}

/*--------------------------------------------------------------------------*/

void Backend::notifyObservers ()
{
        for (auto *o : observers) {
                o->onNewData ();
        }
}

} // namespace logic