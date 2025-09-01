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
#include <libusb.h>
#include <memory>
#include <ranges>
#include <set>
#include <string>
#include <vector>
module logic;

namespace logic {

UsbFactory::UsbFactory (EventQueue *eventQueue)
    : eventQueue_{eventQueue},
      usb_{eventQueue},
      //       demo_{eventQueue},
      usbEntries{
              UsbEntry{.name = "logicLink",
                       .vid = common::usb::VID,
                       .pid = common::usb::PID,
                       .create = [this] (libusb_device_handle *h) { return std::make_unique<LogicLink> (&usb_, h); }},

              UsbEntry{.name = "saleaeClone",
                       .vid = 0x0925,
                       .pid = 0x3881,
                       .create = [this] (libusb_device_handle *h) { return std::make_unique<LogicLink> (&usb_, h); }},
      }
{
}

/****************************************************************************/

std::unique_ptr<IDevice> UsbFactory::create () const
{
        std::unique_ptr<IDevice> ret;

        eventQueue_->waitAlarms<UsbConnectedAlarm> (
                [this, &ret] (UsbConnectedAlarm const *currentAlarm) {
                        ret = find (currentAlarm->vid (), currentAlarm->pid ())->create (currentAlarm->devHandle ());
                },
                1);

        return ret;
}

/****************************************************************************/

std::unique_ptr<IDevice> UsbFactory::create (std::string const &name) const
{
        if (name.empty ()) {
                return create ();
        }

        auto const *en = find (name);

        if (en == nullptr) {
                return {}; // No configuration for this name.
        }

        return create (en->vid, en->pid);
}

/****************************************************************************/

std::unique_ptr<IDevice> UsbFactory::create (UsbConnectedAlarm const *alarm) const { return create (alarm->vid (), alarm->pid ()); }

/****************************************************************************/

std::unique_ptr<IDevice> UsbFactory::create (int vid, int pid) const
{
        std::unique_ptr<IDevice> ret;

        /*
         * First lock the event queue and check that the alarm is still there.
         * This is done by comparing the vid and pid only.
         */
        eventQueue_->waitAlarms<UsbConnectedAlarm> (
                [vid, pid, this, &ret] (UsbConnectedAlarm const *currentAlarm) {
                        if (vid == currentAlarm->vid () && pid == currentAlarm->pid ()) {
                                ret = find (currentAlarm->vid (), currentAlarm->pid ())->create (currentAlarm->devHandle ());
                        }
                },
                1);

        return ret;
}

/****************************************************************************/

std::string UsbFactory::vidPidToName (std::pair<int, int> const &vp) const
{
        auto const *en = find (vp.first, vp.second);
        return (en != nullptr) ? (en->name) : ("");
}

/****************************************************************************/

UsbFactory::UsbEntry const *UsbFactory::find (int vid, int pid) const
{
        if (auto i = std::ranges::find_if (usbEntries, [vid, pid] (auto const &e) { return e.vid == vid && e.pid == pid; });
            i != usbEntries.cend ()) {
                return &*i;
        }

        return {};
}

/****************************************************************************/

UsbFactory::UsbEntry const *UsbFactory::find (std::string const &name) const
{
        if (auto i = std::ranges::find_if (usbEntries, [&name] (auto const &e) { return e.name == name; }); i != usbEntries.cend ()) {
                return &*i;
        }

        return {};
}

/****************************************************************************/

std::set<std::string> UsbFactory::getConnectedDevices () const
{
        std::vector<std::pair<int, int>> vps;
        eventQueue_->visitAlarms<UsbConnectedAlarm> ([&vps] (int v, int p) { vps.emplace_back (v, p); });
        auto notEmptyString = [] (auto const &s) { return !s.empty (); };
        return vps | std::views::transform ([this] (auto const &p) { return vidPidToName (p); }) | std::views::filter (notEmptyString)
                | std::ranges::to<std::set<std::string>> ();
}

}; // namespace logic