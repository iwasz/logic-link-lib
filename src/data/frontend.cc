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
#include <climits>

#include <print>
module logic.data;
import logic.core;
import logic.processing;

namespace logic {

// auto binaryToGray (auto num) { return num ^ (num >> 1); };

DigitalFrontend::DigitalFrontend (IBackend *backend) : backend{backend}, cache (1) { backend->addObserver (this); }

/****************************************************************************/

DigitalFrontend::~DigitalFrontend () { backend->removeObserver (this); }

/****************************************************************************/

// Stream const &DigitalFrontend::group (size_t groupIdx, SampleIdx const &begin, SampleIdx const &end)
// {
//         ZoneScoped;

//         /*
//          * TODO Better caching mechanism would be beneficial here. It would get more data
//          * from the backend that was requested, and would return only a subrange / span.
//          * - retrieve more (like 3x more than end-begin), store in cache
//          * - remove cachedBegin and cachedEnd
//          * - add a query method to teh `Stream` class that would check if provided range is
//          *   covered by this stream.
//          * - and resize cache automatically (if multiple groups per frontend is possible).
//          */
//         if (cachedBegin != begin || cachedEnd != end) {
//                 cache.at (groupIdx) = backend->range (groupIdx, begin, end);
//         }

//         return cache.at (groupIdx);
// }

/****************************************************************************/

BlockRangeBitSpan<uint8_t const, BlockArray::Container> DigitalFrontend::channel (size_t groupIdx, size_t channelIdx, SampleIdx begin,
                                                                                  SampleNum length)
{
        ZoneScoped;
        if (int (channelIdx) > int (backend->channelsNumber (groupIdx)) - 1) {
                return {};
        }

        // Stream const &grp = group (groupIdx, begin, begin + length);
        auto grp = backend->range (groupIdx, begin, begin + length);

        if (grp.empty ()) {
                return {};
        }

        // auto joined = grp |

        ZoneNamedN (z1, "bitSpanPrepare", true);
        auto grpStart = grp.front ().firstSampleNo ();
        auto off = begin - grpStart;

        if (off < 0) {
                throw Exception{"Frontend internal error. Offset < 0."};
        }

        return {grp, channelIdx, size_t (off), size_t (length)};
}

/****************************************************************************/

BlockArray::SubRange DigitalFrontend::range (size_t groupIdx, SampleIdx offset, SampleNum length, SampleNum maxDiscernibleSamples)
{
        ZoneScoped;
        return backend->range (groupIdx, offset, offset + length, maxDiscernibleSamples);
}

/****************************************************************************/

bool DigitalFrontend::isNewData ()
{
        if (newData) {
                newData = false;
                return true;
        }

        return false;
}

} // namespace logic