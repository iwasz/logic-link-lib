/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "common/params.hh"
#include "data/rawData.hh"

namespace usb::async {

/**
 * Blocking acquisition.
 */
void acquire (common::acq::Params const &params, data::Session *session, size_t singleTransferLenB);

} // namespace usb::async