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
#include <vector>
module logic.data;

namespace logic {

auto binaryToGray (auto num) { return num ^ (num >> 1); };

Frontend::Frontend (IBackend *backend) : backend{backend}, cache (1)
{
        // Stream group;
        // group.data.resize (16);

        // for (auto &channel : group.data) {
        //         channel.resize (256); // 2048 bits
        //         std::ranges::generate (channel, [] { return uint8_t (std::rand () % 256); });
        //         // std::ranges::copy (std::views::iota (0, 4192), bytes.begin ());
        //         // std::ranges::fill (channel, 0xe0);
        //         // std::ranges::fill (channel, 0xaa);
        //         // uint8_t i = 0;
        //         // std::ranges::generate (bytes, [&i] { return binaryToGray (i++); });
        // }

        // current.push_back (std::move (group));
}

Stream const &Frontend::group (size_t groupIdx, SampleIdx const &begin, SampleIdx const &end)
{
        /*
         * TODO Better caching mechanism would be beneficial here. It would get more data
         * from the backend that was requested, and would return only a subrange / span.
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

        Bytes const &currentData = grp.data.at (channelIdx);
        auto beginByteOffset = begin / CHAR_BIT;

        // Basic validation
        if (length == 0 || beginByteOffset >= currentData.size ()) {
                return {};
        }

        if (auto byteOffset = (begin + length - 1) / CHAR_BIT; byteOffset >= currentData.size ()) {
                return {currentData.data (), size_t (begin), (currentData.size () - beginByteOffset) * CHAR_BIT};
        }

        return {currentData.data (), size_t (begin), size_t (length)};
}

} // namespace logic