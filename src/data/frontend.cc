/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <climits>
module logic.data;

import logic.core;

namespace logic {

// auto binaryToGray (auto num) { return num ^ (num >> 1); };

Frontend::Frontend (IBackend *backend) : backend{backend}, cache (1) {}

Stream const &Frontend::group (size_t groupIdx, SampleIdx const &begin, SampleIdx const &end)
{
        /*
         * TODO Better caching mechanism would be beneficial here. It would get more data
         * from the backend that was requested, and would return only a subrange / span.
         * - retrieve more (like 3x more than end-begin), store in cache
         * - remove cachedBegin and cachedEnd
         * - add a query method to teh `Stream` class that would check if provided range is
         *   covered by this stream.
         * - and resize cache automatically (if multiple groups per frontend is possible).
         */
        if (cachedBegin != begin || cachedEnd != end) {
                cache.at (groupIdx) = backend->range (groupIdx, begin, end);
        }

        return cache.at (groupIdx);
}

/****************************************************************************/

util::BitSpan<uint8_t const> Frontend::channel (size_t groupIdx, size_t channelIdx, SampleIdx begin, SampleNum length)
{
        Stream const &grp = group (groupIdx, begin, begin + length);

        if (int (channelIdx) > int (grp.channelsNumber ()) - 1) {
                return {};
        }

        Bytes const &currentData = grp.channel (channelIdx);
        auto grpStart = grp.firstSampleNo ();
        auto off = begin - grpStart;

        if (off < 0) {
                throw Exception{"Frontend internal error. Offset < 0."};
        }

        return {currentData.data (), size_t (off), size_t (length)};
}

} // namespace logic