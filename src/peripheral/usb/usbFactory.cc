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
#include <string>
#include <vector>
module logic.peripheral;

namespace logic {

UsbFactory::UsbFactory (EventQueue *eventQueue, UsbAsyncInput *usb)
    : eventQueue_{eventQueue},
      usbEntries{
              UsbEntry{.name = "logicLink",
                       .vid = common::usb::VID,
                       .pid = common::usb::PID,
                       .create = [eventQueue] (libusb_device_handle *h) { return std::make_unique<LogicLink> (eventQueue, h); }},

              UsbEntry{.name = "logicLinkDummy",
                       .vid = common::usb::VID,
                       .pid = common::usb::PID - 1,
                       .create = [eventQueue] (libusb_device_handle *h) { return std::make_unique<LogicLinkDummy> (eventQueue, h); }},
      }
{
}

/****************************************************************************/

std::unique_ptr<UsbDevice> UsbFactory::create (int vid, int pid, libusb_device_handle *h) const
{
        auto const *en = find (vid, pid);

        if (en == nullptr) {
                return {};
        }

        return en->create (h);
}

/****************************************************************************/

bool UsbFactory::isSupported (int vid, int pid) const { return find (vid, pid) != nullptr; }

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

}; // namespace logic