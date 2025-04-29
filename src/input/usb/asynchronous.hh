/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "common/params.hh"
#include "types.hh"

namespace logic::usb::async {

/**
 * Blocking acquisition.
 */
void acquire (common::acq::Params const &params, data::Session *session, size_t singleTransferLenB);

} // namespace logic::usb::async