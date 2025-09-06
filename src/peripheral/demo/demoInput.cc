/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <algorithm>
#include <atomic>
#include <cstdlib>
#include <libusb.h>
#include <memory>
module logic.peripheral;
import :input.demo;

namespace logic {

DemoInput::DemoInput (EventQueue *eventQueue) : AbstractInput{eventQueue}
{
        eventQueue->setAlarm<DemoDeviceAlarm> (std::make_shared<DemoDevice> (eventQueue));
}

/****************************************************************************/

void DemoInput::run ()
{
        // Bytes singleTransfer;

        // while (true) {
        //         if (running_) {
        //                 singleTransfer.resize (blockSize);
        //                 std::ranges::generate (singleTransfer, [] { return std::rand () % 256; });
        //                 RawCompressedBlock rcd{0, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
        //                 session->rawQueue.push (std::move (rcd));
        //         }
        //         else {
        //                 session = nullptr;
        //         }

        //         if (stop_ && singleTransfer.empty ()) {
        //                 break;
        //         }
        // }
}

/****************************************************************************/

// void DemoInput::start (UsbHandle const &handle)
// {
//         std::lock_guard lock{mutex};
//         // this->session = session;
//         running_ = true;
// }

// /****************************************************************************/

// void DemoInput::stop (UsbDevice * /* dev */) { running_ = false; }

} // namespace logic