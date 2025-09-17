/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
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

        thread = std::thread{[backend, this] {
                while (running ()) {
                        auto now = std::chrono::steady_clock::now ();

                        auto dc = acquisitionParams.digitalChannels;
                        std::vector<Bytes> channels;
                        channels.reserve (dc);

                        for (auto i = 0U; i < dc; ++i) {
                                /*
                                 * Square wave which frequency increases proportionally with the
                                 * channel number and length (number of bits) decreases.
                                 */
                                channels.push_back (square (i + 1, i + 1, transferSize * CHAR_BIT / dc));
                        }

                        backend->append (0, std::move (channels));

                        auto timeSpent = std::chrono::seconds (transferSize / acquisitionParams.digitalSampleRate);
                        std::this_thread::sleep_until (now + timeSpent);
                }
        }};
}

/****************************************************************************/

void DemoDevice::stop ()
{
        try {
                notify (false, State::ok);
                if (thread.joinable ()) {
                        thread.join ();
                }
        }
        catch (std::exception const &e) {
                eventQueue ()->addEvent<ErrorEvent> (std::format ("DemoDevice stop exception: {}", e.what ()));
        }
}

} // namespace logic