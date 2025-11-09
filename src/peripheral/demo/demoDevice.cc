/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include "common/params.hh"
#include <Tracy.hpp>
#include <spdlog/spdlog.h>
#include <thread>
module logic.peripheral;
import logic.processing;
import logic.util;

namespace logic {
using namespace std::chrono_literals;

DemoDevice::DemoDevice (EventQueue *eventQueue) : eventQueue_{eventQueue}
{
        // We have to have valid defaults.
        acquisitionParams.digitalChannels = 16;
        acquisitionParams.digitalSampleRate = 3000'000;
}

/****************************************************************************/

void DemoDevice::start (IBackend *backend)
{
        notify (true, Health::ok);
        totalSizePerChan = 0;

        if (thread.joinable ()) {
                thread.join ();
        }

        thread = std::thread{[backend, this] {
                auto dc = acquisitionParams.digitalChannels;
                double delay = double (transferSize) * CHAR_BIT / (acquisitionParams.digitalSampleRate * dc);
                auto chDelay = std::chrono::round<std::chrono::microseconds> (std::chrono::duration<double> (delay));
                setThreadName ("DemoDev");

                while (acquiring ()) {
                        ZoneNamedN (demoDev, "demoDev", true);

                        auto now = std::chrono::steady_clock::now ();
                        std::vector<Bytes> channels;
                        channels.reserve (dc);

                        auto sizePerChanBits = transferSize * CHAR_BIT / dc;
                        totalSizePerChan += sizePerChanBits;

                        ZoneNamedN (pushBack, "generate", false);
                        for (auto i = 0U; i < dc; ++i) {
                                /*
                                 * Square wave which frequency increases proportionally with the
                                 * channel number and length (number of bits) decreases.
                                 */
                                channels.push_back (generators.at (i) (i + 1, i + 1, sizePerChanBits));
                        }

                        static constexpr auto BITS_PER_SAMPLE = 1U;
                        static constexpr auto GROUP = 0U;
                        ZoneNamedN (append, "append", false);
                        backend->append (GROUP, BITS_PER_SAMPLE, std::move (channels));

                        // if (totalSizePerChan >= 1000000) {
                        //         notify (false, State::ok);
                        // }

                        ZoneNamedN (sleep, "sleep", false);
                        std::this_thread::sleep_until (now + chDelay);
                }
        }};
}

/****************************************************************************/

void DemoDevice::stop ()
{
        try {
                spdlog::info ("DemoDevice stopped. Samples generated per channel: {}", totalSizePerChan);
                notify (false, Health::ok);

                if (thread.joinable ()) {
                        thread.join ();
                }
        }
        catch (std::exception const &e) {
                eventQueue ()->addEvent<ErrorEvent> (std::format ("DemoDevice stop exception: {}", e.what ()));
        }
}

/****************************************************************************/

void DemoDevice::writeAcquisitionParams (common::acq::Params const &params, bool legacy)
{
        AbstractDevice::writeAcquisitionParams (params, legacy);
        generators.resize (params.digitalChannels);
}

} // namespace logic