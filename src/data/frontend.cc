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
#include <vector>

module logic;
import :data;
import :data.frontend;
import :data.backend;

namespace logic::data {

auto binaryToGray (auto num) { return num ^ (num >> 1); };

Frontend::Frontend (IBackend *backend) : backend{backend}
{
        ChannelStream group;
        group.data.resize (16);

        for (auto &channel : group.data) {
                channel.resize (10000 / CHAR_BIT);
                std::ranges::generate (channel, [] { return uint8_t (std::rand () % 256); });
                // std::ranges::copy (std::views::iota (0, 4192), bytes.begin ());
                // std::ranges::fill (channel, 0xe0);
                // std::ranges::fill (channel, 0xaa);
                // uint8_t i = 0;
                // std::ranges::generate (bytes, [&i] { return binaryToGray (i++); });
        }

        current.push_back (std::move (group));
}
} // namespace logic::data