/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "common/params.hh"

namespace acq {

namespace priv {
        inline common::acq::Params &params ()
        {
                static common::acq::Params _;
                return _;
        }
} // namespace priv

inline common::acq::Params const &params () { return priv::params (); }

} // namespace acq