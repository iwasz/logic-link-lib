/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdlib>

namespace cli {

/**
 * Params speciffic to the client application.
 */
struct Params {
        size_t seconds{};
        size_t bytes{};
};

namespace priv {
        inline Params &params ()
        {
                static Params _;
                return _;
        }
} // namespace priv

inline Params const &params () { return priv::params (); }

} // namespace cli