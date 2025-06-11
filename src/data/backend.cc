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
#include <span>
#include <vector>
module logic;

namespace logic::data {

void ChannelBlockStream::append (std::vector<Bytes> &&channels)
{
        auto cb = ChannelBlock{currentSampleNo, std::move (channels)};
        currentSampleNo = currentSampleNo + cb.size ();
        data_.push_back (std::move (cb));
}

/****************************************************************************/

ChannelBlockStream::SubRange ChannelBlockStream::range (SampleNo const &begin, SampleNo const &end)
{
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

BlockBackend::SubRange BlockBackend::range (size_t groupIdx, SampleNo const &begin, SampleNo const &end)
{
        return groups.at (groupIdx).range (begin, end);
}

/****************************************************************************/

void BlockBackend::configureGroup (size_t groupIdx, SampleRate sampleRate)
{

        if (groups.size () <= groupIdx) {
                groups.resize (groupIdx + 1);
        }

        groups.at (groupIdx).sampleRate = sampleRate;
}

} // namespace logic::data