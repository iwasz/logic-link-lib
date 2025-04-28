/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "analysis/debug/clockSignal.hh"
#include "analysis/debug/flexioSynchro.hh"
#include "analysis/debug/ordinal.hh"
#include "analysis/debug/simplePrint.hh"
#include "cliParams.hh"
#include "commandLine.hh"
#include "data/acqParams.hh"
#include "data/rawData.hh"
#include "input/usb/asynchronous.hh"
#include "input/usb/request.hh"
#include "input/usb/usbParams.hh"
#include "lib/system/system.hh"
#include <argparse/argparse.hpp>
#include <future>
#include <gsl/gsl>
#include <print>
#include <ranges>

using namespace std::chrono_literals;
using namespace std::string_literals;

/****************************************************************************/

int main (int argc, char **argv)
{
        auto finally = gsl::finally ([] { usb::close (); });

        /*
         * For cli application there is only 1 session. For GUI app there may be
         * more then one.
         */
        data::Session session;

        try {
                sys::init ();
                cli::parse (argc, argv);
                usb::init ();

                auto const &cliParams = cli::params ();
                auto const &usbParams = usb::params ();
                usb::setTransmissionParams (usbParams.usbTransfer, usbParams.usbBlock);
                /*
                 * Here we should do usb::configure, and obtain the set of acquisitionParams accepted by the
                 * device itself. Input params and the final ones can differ.
                 */
                auto acquisitionParams = acq::params ();
                acquisitionParams = usb::configure (acquisitionParams);
                auto start = std::chrono::high_resolution_clock::now ();

                try {
                        // Algorithm used
                        // an::OrdinalCheck ordinal{usbParams.usbBlock};
                        // an::ClockSignalCheck clock{usbParams.usbBlock};
                        // clock.setDecimation (4, 1);
                        // an::FlexioSynchronizationCheck flexioSynchro{usbParams.usbBlock};
                        // an::SimplePrint check{usbParams.usbBlock};
                        an::RawPrint check{usbParams.usbBlock};

                        // true: discard the data, false: keep data forever in the RAM
                        // auto clockAnalyzeThread = std::async (std::launch::async, an::analyze, &rawData, &clock, false);
                        // auto flexioAnalyzeThread = std::async (std::launch::async, an::analyze, &rawData, &flexioSynchro, false);
                        session.running = true;

                        // Futures are created for easy exception handling.
                        auto simplePrintThread = std::async (std::launch::async, an::analyze, acquisitionParams, &session, &check, false, false);
                        auto acquire = std::async (std::launch::async, usb::async::acquire, acquisitionParams, &session, usbParams.usbTransfer);

                        while (true) {
                                if (auto now = std::chrono::high_resolution_clock::now ();
                                    cliParams.seconds > 0 && now - start >= std::chrono::seconds{cliParams.seconds}) {
                                        usb::stop (&session);
                                        break;
                                }

                                if (cliParams.bytes > 0 && session.receivedB () >= cliParams.bytes) {
                                        usb::stop (&session);
                                        break;
                                }

                                if (sys::isTermRequested ()) {
                                        std::println ("Interrupted");
                                        usb::stop (&session);
                                        break;
                                }

                                if (acquire.wait_for (1000ms) == std::future_status::ready) {
                                        acquire.get ();
                                        // clockAnalyzeThread.get ();
                                        // flexioAnalyzeThread.get ();
                                        simplePrintThread.get ();
                                }
                        }
                } // Both threads joined. We catch so early here, because we'll try to download error code from the device.
                catch (std::exception const &e) {
                        std::println ("Acquisition thread exception: {}", e.what ());
                }
                catch (...) {
                        std::println ("Acquisition thread unknown exception.");
                        std::exit (1);
                }

                auto s = usb::getStats ();
                std::println ("Queue max: {}, addErr: {}, s1F: {}, s1B: {}, s1E: {}, s2F: {}, s2F: {}, s2E: {}", s.queue.maxSize, s.addErrors,
                              s.send1Fatal, s.send1Busy, s.send1Empty, s.send2Fatal, s.send2Busy, s.send2Empty);

                if (auto e = usb::getErrors (); !e.empty ()) {
                        std::println ("Errors reported by the device:");

                        std::ranges::copy (
                                e | std::views::transform ([] (logs::Code c) { return "* "s + logs::message.at (size_t (c)).data (); }),
                                std::ostream_iterator<std::string> (std::cerr, "\n"));
                }
        }
        catch (std::exception const &e) {
                std::println ("Main thread exception: {}", e.what ());
                std::exit (1);
        }
        catch (...) {
                std::println ("Main thread unknown exception.");
                std::exit (1);
        }
}
