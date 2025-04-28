/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "data/rawData.hh"

namespace usb::sync {

/**
 * Blocking acquisition.
 */
void acquire (data::RawData *rawData, size_t singleTransferLenB);

} // namespace usb::sync