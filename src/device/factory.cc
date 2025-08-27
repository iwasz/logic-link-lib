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
#include <ranges>
#include <set>
#include <string>
#include <vector>
module logic;

namespace logic {

Factory::Factory (EventQueue *eventQueue)
    : eventQueue_{eventQueue},
      usb_{eventQueue},
      demo_{eventQueue},
      usbEntries{{"logicLink", common::usb::VID, common::usb::PID, [this] { return std::make_unique<LogicLink> (&usb_); }},
                 {"saleaeClone", 0x0925, 0x3881, [this] { return std::make_unique<LogicLink> (&usb_); }}}, // TODO remove

      demoEntries{{"demoLogicLink", [this] { return std::make_unique<LogicLink> (&demo_); }}}
{
}

/****************************************************************************/

std::unique_ptr<IDevice> Factory::doCreate (std::string const &name, bool wait)
{
        // TODO It creates first device with a name match even if it was not connected.
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
                eventQueue_->waitAlarms<UsbConnectedAlarm> (
                        [&dv, &dp] (int v, int p) {
                                dv = v;
                                dp = p;
                        },
                        1);
        }
        else {
                eventQueue_->visitAlarms<UsbConnectedAlarm> (
                        [&dv, &dp] (int v, int p) {
                                dv = v;
                                dp = p;
                        },
                        1);
        }

        return createUsb (dv, dp);
}

/****************************************************************************/

std::unique_ptr<IDevice> Factory::create (std::string const &name) { return doCreate (name, false); }
std::unique_ptr<IDevice> Factory::wait (std::string const &name) { return doCreate (name, true); }

std::unique_ptr<IDevice> Factory::create (UsbConnectedAlarm const *alarm) { return createUsb (alarm->vid (), alarm->pid ()); }
std::unique_ptr<IDevice> Factory::createUsb (int vid, int pid)
{
        if (auto i = std::ranges::find_if (usbEntries, [vid, pid] (auto const &e) { return e.vid == vid && e.pid == pid; });
            i != usbEntries.cend ()) {
                return i->create ();
        }

        return {};
}

/****************************************************************************/

std::string Factory::vidPidToName (std::pair<int, int> const &vp) const
{
        if (auto i = std::ranges::find_if (usbEntries, [&vp] (auto const &e) { return e.vid == vp.first && e.pid == vp.second; });
            i != usbEntries.cend ()) {
                return i->name;
        }

        return "";
};

/****************************************************************************/

std::set<std::string> Factory::getConnectedDevices () const
{
        std::vector<std::pair<int, int>> vps;
        eventQueue_->visitAlarms<UsbConnectedAlarm> ([&vps] (int v, int p) { vps.emplace_back (v, p); });
        auto notEmptyString = [] (auto const &s) { return !s.empty (); };
        return vps | std::views::transform ([this] (auto const &p) { return vidPidToName (p); }) | std::views::filter (notEmptyString)
                | std::ranges::to<std::set<std::string>> ();
}

}; // namespace logic