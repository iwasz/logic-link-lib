/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "types.hh"

namespace logic::data {

void Group::appendChannels (std::vector<SampleBlock> &&s)
{
        for (size_t i = 0; i < s.size (); ++i) {
                channels.at (i).stream.push_back (std::move (s.at (i)));
        }
}

} // namespace logic::data