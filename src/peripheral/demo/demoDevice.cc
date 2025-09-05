/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include "common/error.hh"
#include "common/params.hh"
#include "common/stats.hh"
#include <cstdint>
#include <cstdlib>
#include <libusb.h>
#include <print>
#include <string>
#include <vector>
module logic.peripheral;

namespace logic {

common::acq::Params DemoDevice::configureAcquisition (common::acq::Params const &params, bool /* legacy */)
{
        params_ = params;
        return params;
}

/****************************************************************************/

void DemoDevice::start (Queue<RawCompressedBlock> *queue) { this->queue = queue; }

/****************************************************************************/

void DemoDevice::stop ()
{
        stopRequest = true;
        queue = nullptr;
}

} // namespace logic