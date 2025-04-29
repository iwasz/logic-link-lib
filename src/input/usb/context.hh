/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <atomic>
#include <libusb.h>

namespace logic::usb {
struct Context {
        libusb_device_handle *dev{};
        std::atomic<bool> initialized;
};

Context &ctx ();

} // namespace logic::usb