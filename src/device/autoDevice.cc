/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/error.hh"
#include "common/params.hh"
#include "common/stats.hh"
#include <cstdlib>
#include <unordered_set>
module logic;

namespace logic {
using namespace common::usb;

void asrt (auto const &ptr)
{
        if (!ptr) {
                throw Exception{"AutoDevice::configureAcquisition no delegate."};
        }
}

/****************************************************************************/

common::acq::Params AutoDevice::configureAcquisition (common::acq::Params const &params, bool legacy)
{
        asrt (delegate);
        return delegate->configureAcquisition (params, legacy);
}

/****************************************************************************/

void AutoDevice::start ()
{
        asrt (delegate);
        delegate->start ();
}

/****************************************************************************/

void AutoDevice::stop ()
{
        asrt (delegate);
        delegate->stop ();
}

/****************************************************************************/

common::usb::Stats AutoDevice::getStats ()
{
        asrt (delegate);
        return delegate->getStats ();
}

/****************************************************************************/

std::unordered_set<logs::Code> AutoDevice::getErrors ()
{
        asrt (delegate);
        return delegate->getErrors ();
}

/****************************************************************************/

void AutoDevice::clearErrors ()
{
        asrt (delegate);
        delegate->clearErrors ();
}

/****************************************************************************/

void AutoDevice::configureTransmission (TransmissionParams const &params)
{
        asrt (delegate);
        delegate->configureTransmission (params);
}

/****************************************************************************/

void AutoDevice::onConnected (std::string const &name) { delegate = factory_->create (name); }
void AutoDevice::onDisconnected () { delegate.reset (); }

} // namespace logic