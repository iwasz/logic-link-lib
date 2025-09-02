/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include <libusb.h>
#include <memory>
module logic.peripheral;

namespace logic {

std::shared_ptr<IDevice> Factory::create (std::string const &name) const
{
        if (name.empty ()) {
                return create ();
        }

        std::shared_ptr<IDevice> ret;
        eventQueue_->waitAlarms<DeviceAlarm> ([&ret, &name] (std::shared_ptr<IDevice> const &dev) {
                if (dev->name () == name) {
                        ret = dev;
                }
        });

        return ret;
}

/****************************************************************************/

std::shared_ptr<IDevice> Factory::create () const
{
        std::shared_ptr<IDevice> ret;
        eventQueue_->waitAlarms<DeviceAlarm> ([&ret] (std::shared_ptr<IDevice> const &dev) { ret = dev; }, 1);
        return ret;
}

} // namespace logic