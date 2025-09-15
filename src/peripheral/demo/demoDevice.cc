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

void DemoDevice::writeAcquisitionParams (common::acq::Params const &params, bool /* legacy */) { params_ = params; }

/****************************************************************************/

void DemoDevice::start (IBackend * /* backend */) { /* this->queue = queue; */ }

void DemoDevice::run () {}

/****************************************************************************/

void DemoDevice::stop ()
{
        stopRequest = true;
        // queue = nullptr;
}

} // namespace logic