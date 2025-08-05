/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <algorithm>
#include <memory>
#include <string>
module logic;

namespace logic {

Factory::Factory ()
    : usbEntries{{"logicLink", common::usb::VID, common::usb::PID, [this] { return std::make_unique<LogicLink> (&usb_); }}},
      demoEntries{{"demoLogicLink", [this] { return std::make_unique<LogicLink> (&demo_); }}}
{
}

/****************************************************************************/

std::unique_ptr<IDevice> Factory::doCreate (std::string const &name, bool wait)
{
        auto nameMatch = [&name] (auto const &e) { return e.name == name; };

        if (auto i = std::ranges::find_if (usbEntries, nameMatch); i != usbEntries.cend ()) {
                return i->create ();
        }

        if (auto i = std::ranges::find_if (demoEntries, nameMatch); i != demoEntries.cend ()) {
                return i->create ();
        }

        int dv{};
        int dp{};

        if (wait) {
                eventQueue ().waitAlarms<UsbConnectedAlarm> (
                        [&dv, &dp] (int v, int p) {
                                dv = v;
                                dp = p;
                        },
                        1);
        }
        else {
                eventQueue ().visitAlarms<UsbConnectedAlarm> (
                        [&dv, &dp] (int v, int p) {
                                dv = v;
                                dp = p;
                        },
                        1);
        }

        if (auto i = std::ranges::find_if (usbEntries, [&dv, &dp] (auto const &e) { return e.vid == dv && e.pid == dp; });
            i != usbEntries.cend ()) {
                return i->create ();
        }

        return {};
}

/****************************************************************************/

std::unique_ptr<IDevice> Factory::create (std::string const &name) { return doCreate (name, false); }
std::unique_ptr<IDevice> Factory::wait (std::string const &name) { return doCreate (name, true); }

}; // namespace logic