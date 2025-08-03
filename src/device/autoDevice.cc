// /****************************************************************************
//  *                                                                          *
//  *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
//  *  ~~~~~~~~                                                                *
//  *  License : see COPYING file for details.                                 *
//  *  ~~~~~~~~~                                                               *
//  ****************************************************************************/

// module;
// #include "common/error.hh"
// #include "common/params.hh"
// #include "common/stats.hh"
// #include <cstdlib>
// #include <mutex>
// #include <unordered_set>
// module logic;

// namespace logic {
// using namespace common::usb;

// void asrt (auto const &ptr)
// {
//         if (!ptr) {
//                 throw Exception{"AutoDevice::configureAcquisition no delegate."};
//         }
// }

// /****************************************************************************/

// void AutoDevice::run (Queue<RawCompressedBlock> *queue)
// {
//         // // Wait for the device to become connected.
//         // if (!delegate) {
//         //         input ()->detect ();
//         // }

//         AbstractDevice::run (queue);
// }

// /****************************************************************************/

// common::acq::Params AutoDevice::configureAcquisition (common::acq::Params const &params, bool legacy)
// {
//         asrt (delegate);
//         return delegate->configureAcquisition (params, legacy);
// }

// /****************************************************************************/

// common::usb::Stats AutoDevice::getStats ()
// {
//         asrt (delegate);
//         return delegate->getStats ();
// }

// /****************************************************************************/

// std::unordered_set<logs::Code> AutoDevice::getErrors ()
// {
//         asrt (delegate);
//         return delegate->getErrors ();
// }

// /****************************************************************************/

// void AutoDevice::clearErrors ()
// {
//         asrt (delegate);
//         delegate->clearErrors ();
// }

// /****************************************************************************/

// void AutoDevice::configureTransmission (TransmissionParams const &params)
// {
//         asrt (delegate);
//         delegate->configureTransmission (params);
// }

// /****************************************************************************/

// void AutoDevice::onConnected (std::string const &name)
// {
//         {
//                 std::lock_guard lock{mutex};
//                 delegate = factory_->create (name);
//         }

//         cv.notify_all ();
// }

// /****************************************************************************/

// void AutoDevice::onDisconnected ()
// {
//         std::lock_guard lock{mutex};
//         delegate.reset ();
// }

// /****************************************************************************/

// void AutoDevice::onStart ()
// {
//         asrt (delegate);
//         delegate->onStart ();
// }

// /****************************************************************************/

// void AutoDevice::onStop ()
// {
//         asrt (delegate);
//         delegate->onStop ();
// }

// /****************************************************************************/

// size_t AutoDevice::transferLen () const
// {
//         asrt (delegate);
//         return delegate->transferLen ();
// }

// /****************************************************************************/

// bool AutoDevice::isReady () const
// {
//         std::lock_guard lock{mutex};
//         return delegate != nullptr && delegate->isReady ();
// }

// /****************************************************************************/

// void AutoDevice::waitReady () const
// {
//         std::unique_lock lock{mutex};
//         cv.wait (lock, [this] { return delegate != nullptr && delegate->isReady (); });
// }

// } // namespace logic