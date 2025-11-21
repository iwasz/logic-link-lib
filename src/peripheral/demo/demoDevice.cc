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

        thread = std::thread{[backend, this, eventQueue = eventQueue ()] {
                try {
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

                                ZoneNamedN (pushBack, "generate", false);
                                for (auto i = 0U; i < dc; ++i) {
                                        /*
                                         * Square wave which frequency increases proportionally with the
                                         * channel number and length (number of bits) decreases.
                                         */
                                        channels.push_back (generators.at (i) (i + 1, i + 1, sizePerChanBits));
                                }

                                ZoneNamedN (append, "append", false);
                                backend->append (acquisitionParams.groups[0], std::move (channels));

                                totalSizePerChan += sizePerChanBits;
                                if (acquisitionParams.digitalSamplesPerChannelLimit > 0
                                    && totalSizePerChan >= acquisitionParams.digitalSamplesPerChannelLimit) {
                                        notify (false, Health::ok);
                                }

                                ZoneNamedN (sleep, "sleep", false);
                                std::this_thread::sleep_until (now + chDelay);
                        }
                }
                catch (std::exception const &e) {
                        eventQueue->addEvent<ErrorEvent> (std::format ("Exception caught in `DemoDevice thread`: {}", e.what ()));
                }
                catch (...) {
                        eventQueue->addEvent<ErrorEvent> (std::format ("Unknown (...) exception caught in `DemoDevice thread`"));
                }
        }};
}

/****************************************************************************/

void DemoDevice::stop ()
{
        try {
                // std::println ("DemoDevice stopped. Samples generated per channel: {}", totalSizePerChan);
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