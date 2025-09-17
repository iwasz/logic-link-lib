/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <optional>
module logic.peripheral;

namespace logic {

/****************************************************************************/

void AbstractDevice::notify (std::optional<bool> running, std::optional<State> state)
{
        if (running != std::nullopt) {
                running_ = *running;
        }

        if (state != std::nullopt) {
                state_ = *state;
        }

        eventQueue ()->addEvent<DeviceStatusAlarm> (this, running_, state_);
}

/****************************************************************************/

void AbstractDevice::writeAcquisitionParams (common::acq::Params const &params, bool /* legacy */)
{
        if (running ()) {
                throw Exception{"UsbDevice::writeAcquisitionParams called on a running device."};
        }

        acquisitionParams = params;
}

} // namespace logic