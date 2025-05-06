/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "types.hh"

namespace logic::usb::sync {

/**
 * Blocking acquisition.
 */
void acquire (data::RawData *rawData, size_t singleTransferLenB);

} // namespace logic::usb::sync
