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
#include <spdlog/spdlog.h>
#include <thread>
module logic.peripheral;
import logic.processing;

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
        notify (true, State::ok);
        totalSizePerChan = 0;

        thread = std::thread{[backend, this] {
                double delay = double (transferSize) * CHAR_BIT / acquisitionParams.digitalSampleRate;
                auto chDelay = std::chrono::round<std::chrono::microseconds> (std::chrono::duration<double> (delay));

                while (running ()) {
                        auto now = std::chrono::steady_clock::now ();

                        auto dc = acquisitionParams.digitalChannels;
                        std::vector<Bytes> channels;
                        channels.reserve (dc);

                        auto sizePerChan = transferSize * CHAR_BIT / dc;
                        totalSizePerChan += sizePerChan;

                        for (auto i = 0U; i < dc; ++i) {
                                /*
                                 * Square wave which frequency increases proportionally with the
                                 * channel number and length (number of bits) decreases.
                                 */
                                channels.push_back (generators.at (i) (i + 1, i + 1, sizePerChan));
                        }

                        static constexpr auto BITS_PER_SAMPLE = 1U;
                        static constexpr auto GROUP = 0U;
                        backend->append (GROUP, BITS_PER_SAMPLE, std::move (channels));

                        std::this_thread::sleep_until (now + chDelay);
                }
        }};
}

/****************************************************************************/

void DemoDevice::stop ()
{
        try {
                spdlog::info ("DemoDevice stopped. Samples generated per channel: {}", totalSizePerChan);
                notify (false, State::ok);

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