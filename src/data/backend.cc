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
#include <mutex>
#include <vector>
module logic.data;
import logic.core;
import logic.processing;

namespace logic {

void Backend::append (size_t groupIdx, uint8_t bitsPerSample, std::vector<Bytes> &&s)
{
        {
                std::lock_guard lock{mutex};
                groups_.at (groupIdx).append (bitsPerSample, std::move (s));
        }

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
        return groups_.at (groupIdx).range (begin, end, zoomOut, peek);
}

/*--------------------------------------------------------------------------*/

size_t Backend::addGroup (Config const &config)
{
        std::lock_guard lock{mutex};
        groups_.emplace_back (config.channelsNumber, config.maxZoomOutLevels, config.zoomOutPerLevel);
        auto &g = groups_.back ();
        g.setBlockSizeB (config.blockSizeB);
        g.setBlockSizeMultiplier (config.blockSizeMultiplier);
        return groups_.size () - 1;
}

/*--------------------------------------------------------------------------*/

SampleNum Backend::channelLength (size_t groupIdx) const
{
        std::lock_guard lock{mutex};
        return groups_.at (groupIdx).channelLength ();
}

/*--------------------------------------------------------------------------*/

void Backend::notifyObservers ()
{
        for (auto *o : observers) {
                o->onNewData ();
        }
}

} // namespace logic