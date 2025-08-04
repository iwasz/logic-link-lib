/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <memory>
#include <string>
module logic;

namespace logic {

// Factory::Factory ()
//     : usbDeviceRecipes{
//               {"logicLink",
//                {UsbDeviceInfo{
//                         .vid = common::usb::VID,
//                         .pid = common::usb::PID,
//                         .claimInterface = 0,
//                         .interfaceNumber = 0,
//                         .alternateSetting = 0,
//                 },
//                 [this] { return std::make_unique<LogicLink> (&usb_); }}},
//       }
// {
// }

std::unique_ptr<IDevice> Factory::create (std::string const &name)
{
        if (name == "logicLink") {
                return std::make_unique<LogicLink> (&usb_);
        }

        if (name == "demoLogicLink") {
                return std::make_unique<LogicLink> (&demo_);
        }

        throw Exception{"Factory::create is unable to create a device of name " + name};
}

}; // namespace logic