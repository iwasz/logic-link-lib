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

void AbstractDevice::notify (std::optional<bool> running, std::optional<Health> state)
{
        if (running != std::nullopt) {
                acquiring_ = *running;
        }

        if (state != std::nullopt) {
                health_ = *state;
        }

        eventQueue ()->addEvent<DeviceStatusAlarm> (this, acquiring_, health_);
}

/****************************************************************************/

void AbstractDevice::writeAcquisitionParams (common::acq::Params const &params, bool /* legacy */)
{
        if (acquiring ()) {
                throw Exception{"UsbDevice::writeAcquisitionParams called on a running device."};
        }

        acquisitionParams = params;
}

} // namespace logic