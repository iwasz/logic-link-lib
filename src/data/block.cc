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
#include <climits>
#include <format>
#include <ranges>
#include <vector>
module logic.data;
import logic.core;
import logic.processing;

namespace logic {

void Block::append (Block &&d)
{
        bitsPerSample_ = d.bitsPerSample_;
        append (std::move (d).data_);
}

/****************************************************************************/

void Block::append (Container &&d)
{
        if (channelsNumber () == 0) { // This lets us append to an empty block
                data_.resize (d.size ());
        }

        if (d.size () != channelsNumber ()) {
                throw Exception{std::format ("Block::append: d.size ():{} != channelsNumber ():{}", d.size (), channelsNumber ())};
        }

        Container copy = std::move (d);
        for (std::tuple<Bytes &, Bytes const &> t : std::views::zip (data_, copy)) {
                Bytes &dest = std::get<0> (t);
                Bytes const &src = std::get<1> (t);

                dest.reserve (dest.size () + src.size ());
                std::ranges::copy (src, std::back_inserter (dest));
        }
}

/****************************************************************************/

void Block::reserve (size_t channels, SampleNum numberOfSampl)
{
        data_.resize (std::max (channels, data_.size ()));

        for (auto &ch : data_) {
                ch.reserve (numberOfSampl);
        }
}

/****************************************************************************/

SampleNum Block::channelLength () const
{
        if (data_.empty ()) {
                return 0;
        }

        auto const SAMPLES_PER_BYTE = CHAR_BIT / bitsPerSample_;
        return SampleNum (channelBytes () * SAMPLES_PER_BYTE * zoomOut_);
}

/****************************************************************************/

size_t Block::channelBytes () const
{
        if (data_.empty ()) {
                return 0;
        }

        return data_.front ().size ();
}

/****************************************************************************/

void Block::clear ()
{
        for (auto &ch : data_) {
                ch.clear ();
        }
}

} // namespace logic