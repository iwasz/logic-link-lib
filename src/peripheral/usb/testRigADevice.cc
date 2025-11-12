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
#include <chrono>
#include <cstdint>
#include <libusb.h>
#include <random>
#include <vector>
module logic.peripheral;
import :device.link;

namespace logic {
using namespace common::usb;

void TestRigA::start (IBackend *backend)
{
        if (acquiring ()) {
                throw Exception{"Start called, but the device has been already started."};
        }

        setBackend (backend);
        controlOut (UsbRequest{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_START_CAPTURE));
        notify (true, Health::ok);

        totalSizePerChan = 0;
        lce.seed (1);
        nextTick = std::chrono::steady_clock::now ();
}

/****************************************************************************/

void TestRigA::stop ()
{
        LogicLink::stop ();
        notify (false, Health::ok);
}

/****************************************************************************/

void TestRigA::run ()
{
        if (!acquiring ()) {
                return;
        }

        auto dc = acquisitionParams.digitalChannels;

        if (auto now = std::chrono::steady_clock::now (); now < nextTick) {
                return;
        }
        else {
                nextTick = now
                        + std::chrono::round<std::chrono::microseconds> (std::chrono::duration<double> (
                                double (DEFAULT_USB_TRANSFER_SIZE_B) * CHAR_BIT / (acquisitionParams.digitalSampleRate * dc)));
        }

        std::vector<Bytes> channels (dc);

        for (auto &ch : channels) {
                ch.reserve (DEFAULT_USB_TRANSFER_SIZE_B / dc);
        }

        auto sizePerChanWords = DEFAULT_USB_TRANSFER_SIZE_B / (dc * sizeof (uint32_t));

        for (auto j = 0U; j < sizePerChanWords; ++j) {
                for (auto &ch : channels) {
                        uint32_t u = lce ();
                        std::span<uint8_t> spn{std::bit_cast<uint8_t *> (&u), sizeof (uint32_t)};
                        std::ranges::copy (spn, std::back_inserter (ch));
                }
        }
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto GROUP = 0U;
        backend ()->append (GROUP, BITS_PER_SAMPLE, std::move (channels));

        totalSizePerChan += sizePerChanWords * sizeof (uint32_t) * CHAR_BIT;
        if (acquisitionParams.digitalSamplesPerChannelLimit > 0 && totalSizePerChan >= acquisitionParams.digitalSamplesPerChannelLimit) {
                notify (false, Health::ok);
        }
}

} // namespace logic