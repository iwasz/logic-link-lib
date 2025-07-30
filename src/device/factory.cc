/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <memory>
#include <string>
module logic;

namespace logic {

std::unique_ptr<IDevice> Factory::create (std::string const &name)
{
        if (name == "auto" || name.empty ()) {
                auto autoDev = std::make_unique<logic::AutoDevice> (&usb, this);

                logic::HotplugHooks htp;
                htp.connected = [&autoDev] (std::string const &name) { autoDev->onConnected (name); };
                htp.disconnected = [&autoDev] { autoDev->onDisconnected (); };
                usb.hotplug (htp);
                return autoDev;
        }

        if (name == "logicLink") {
                return std::make_unique<LogicLink> (&usb);
        }

        if (name == "demoLogicLink") {
                return std::make_unique<LogicLink> (&demo);
        }

        throw Exception{"Factory::create is unable to create a device of name " + name};
}

}; // namespace logic