/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdint>
#include <cstdlib>

namespace logic::usb {

struct Params {
        uint32_t usbTransfer{}; // TODO validate
        uint32_t usbBlock{};
};

namespace priv {
        inline Params &params ()
        {
                static Params _;
                return _;
        }
} // namespace priv

inline Params const &params () { return priv::params (); }

} // namespace logic::usb