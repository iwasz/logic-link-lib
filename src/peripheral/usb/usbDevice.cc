/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include "common/error.hh"
#include "common/params.hh"
#include "common/stats.hh"
#include <cstdint>
#include <cstdlib>
#include <format>
#include <libusb.h>
#include <memory>
#include <string>
#include <vector>
module logic.peripheral;
import logic.processing;

namespace logic {

UsbDevice::~UsbDevice ()
{
        libusb_free_transfer (transfer);
        libusb_close (deviceHandle_);
}

/****************************************************************************/

void UsbDevice::start (IBackend *backend)
{
        if (running ()) {
                throw Exception{"Start called, but the device has been already started."};
        }

        {
                // std::lock_guard lock{mutex};
                this->backend = backend;

                if (!transmissionParams.discardRaw) {
                        queue.start ();
                }

                if (transmissionParams.singleTransferLenB == 0) {
                        notify (false, State::error);
                        throw Exception{"Can't send an USB transfer of length 0."};
                }

                singleTransfer.resize (transmissionParams.singleTransferLenB);
        }

        stopRequest = false;

        if (transfer = libusb_alloc_transfer (0); transfer == nullptr) {
                /*
                 * This is called from an user thread (via UsbAsyncInput::start) so we are
                 * safe to throw an exception.
                 */
                notify (false, State::error);
                throw Exception{"Libusb could not instantiate a new transfer using `libusb_alloc_transfer`"};
        }

        libusb_fill_bulk_transfer (transfer, deviceHandle (), common::usb::IN_EP, singleTransfer.data (), singleTransfer.size (),
                                   &UsbDevice::transferCallback, this, common::usb::TIMEOUT_MS);

        if (auto r = libusb_submit_transfer (transfer); r < 0) {
                notify (false, State::error);
                throw Exception{"`libusb_submit_transfer` has failed. Code: " + std::string{libusb_error_name (r)}};
        }

        // Clear the error state since it seems we started successfuly.
        notify (true, State::ok);
}

/****************************************************************************/

void UsbDevice::run ()
{
        if (!running ()) {
                return;
        }

        /*
         * Protecting transmissionParams with mutex is not crucial as it
         * changes only upon a call to writeTransmissionParams. That method
         * in turn checks for atomic running_ flag and throws if running_ is
         * true. UsbDevice::run at the other hand, runs only when running_ is
         * true, so they are mutually exclusive.
         *
         * queue->start () called in UsbDevice::start
         */
        auto rcd = (transmissionParams.discardRaw) ? (queue.pop ()) : (queue.next ());

        if (!rcd) {
                return;
        }

        RawData rd;

        if (transmissionParams.decompress) {
                rd = decompress (*rcd);
                rcd->clear ();
        }
        else {
                rd = std::move (*rcd);
        }

        // if (strategy != nullptr) {
        //         strategy->runRaw (rd);
        // }

        // TODO for now only digital data gets rearranged
        // TODO agnostic! vector of vector of bytes
        std::vector<Bytes> digitalChannels = rearrange (rd, acquisitionParams);
        // ChannelBlock digitalChannels{0, {}};
        // std::vector<Bytes> digitalChannels;

        /*
         * Consider locking granularity. But even if it is too coarse, the move operation
         * below is so fast, that we aren't locked for too long.
         */
        static constexpr auto BITS_PER_SAMPLE = 1U;
        static constexpr auto GROUP = 0U;
        backend->append (GROUP, BITS_PER_SAMPLE, std::move (digitalChannels));

        // if (strategy != nullptr) {
        //         strategy->run (currentBlock->sampleData);
        // }

        // TODO statictics
        // double globalBps{};
        // {
        //         std::lock_guard lock{session->rawQueueMutex};
        //         globalBps = double (session->receivedB ())
        //                 / double (duration_cast<microseconds> (session->globalStop - session->globalStart).count ()) * CHAR_BIT;
        // }

        // std::println ("Overall: {:.2f} Mbps, ", globalBps);

        // if (strategy != nullptr) {
        //         strategy->stop ();
        // }
}

/****************************************************************************/

void UsbDevice::stop () { stopRequest = true; }

/****************************************************************************/

void UsbDevice::open (UsbInterface const &info)
{
        using namespace std::string_literals;

        if (auto r = libusb_claim_interface (deviceHandle (), info.claimInterface); r < 0) {
                notify (false, State::error);
                throw Exception ("Error claiming interface: "s + std::to_string (info.claimInterface) + ", msg: "s + libusb_error_name (r));
        }

        if (auto r = libusb_set_interface_alt_setting (deviceHandle (), info.interfaceNumber, info.alternateSetting); r < 0) {
                notify (false, State::error);
                throw Exception ("Error libusb_set_interface_alt_setting: "s + libusb_error_name (r));
        }

        /*
         * TODO this is problematic, and IDK why. On python this works OK, though I didn't pass 0 or any
         * other argument. Whereas here I get warning in the dmesg, and LIBUSB_ERROR_BUSY in console.
         */
        // if (auto r = libusb_set_configuration (dev, 0); r < 0) {
        //         setState (State::error);
        //         close();
        //         throw Exception ("Error to libusb_set_configuration: "s + libusb_error_name (r));
        // }

        notify (false, State::ok);
}

/****************************************************************************/

void UsbDevice::transferCallback (libusb_transfer *transfer)
{
        auto *h = reinterpret_cast<UsbDevice *> (transfer->user_data);
        // We assume transmisionParams are already set.
        auto transferLen = h->transmissionParams.singleTransferLenB;

        if (transfer->status != LIBUSB_TRANSFER_COMPLETED) {
                /*
                 * We're in the interal lubusb thread now, we can't throw, thus I
                 */
                h->eventQueue ()->addEvent<ErrorEvent> (
                        std::format ("USB transfer status error Code: {}", libusb_error_name (transfer->status)));
                h->notify (false, State::error);
                return;
        }

        if (h->stopRequest) {
                h->notify (false, State::ok);
                return;
        }

        if (size_t (transfer->actual_length) < transferLen) {
                // Shrink. Standard guarantees, there's no re-allocation here.
                h->singleTransfer.resize (transfer->actual_length);
        }
        else if (size_t (transfer->actual_length) > transferLen) {
                // This should never happen I think.
                h->eventQueue ()->addEvent<ErrorEvent> ("There's more data received than requested.");
                h->notify (false, State::error);
                return;
        }

        // auto now = high_resolution_clock::now ();
        // benchmarkB += singleTransfer.size ();

        {
                // std::lock_guard lock{mutex};
                // allTransferedB += singleTransfer.size ();
        }

        if (!h->singleTransfer.empty ()) {
                // auto mbps = (double (benchmarkB) / double (duration_cast<microseconds> (now - *startPoint).count ())) * 8;
                double mbps = 0;

                /*
                 * Warning! I made a terible mistake here, the one which takes
                 * hours to debug and drives you crazy. In my case it took me 12
                 * hours over 4 days to figure it out. The lines you see below
                 * originally read:
                 *
                 * RawCompressedBlock rcd{mbps, 0, std::move (h->singleTransfer)};
                 * ...
                 * h->singleTransfer = Bytes (transferLen);
                 *
                 * Which first destroyed the h->singleTransfer and then immediately
                 * re-created it. But I ignored the fact that in the start method the
                 * libusb_fill_bulk_transfer is given the original h->singleTransfer
                 * memory address and stores it indefinitely. So the next time the
                 * libusb_submit_transfer was called it tried to fill that original,
                 * not existing memory buffer.
                 *
                 * Another problem here, was that libusb_submit_transfer was called
                 * BEFORE the h->singleTransfer was copied (actually moved) to the
                 * final queue.
                 */
                RawCompressedBlock rcd{mbps, 0, h->singleTransfer}; // TODO resize blocks (if needed)
                h->queue.push (std::move (rcd));                    // Lock protected
                h->singleTransfer.resize (transferLen);
        }

        // Only after finishing the data gathering may we re-start the transfer.
        if (auto rc = libusb_submit_transfer (transfer); rc < 0) {
                auto msg = std::format ("libusb_submit_transfer status error Code: {}", libusb_error_name (rc));
                h->notify (false, State::error);
                h->eventQueue ()->addEvent<ErrorEvent> (msg);
                return;
        }
}

/****************************************************************************/

void UsbDevice::controlOut (std::vector<uint8_t> const &request) const
{
        if (request.size () > common::usb::MAX_CONTROL_PAYLOAD_SIZE) {
                // We don't know if it's running or not, so we don't touch the `running` variable.
                const_cast<UsbDevice *> (this)->notify ({}, State::error);
                throw Exception{"MAX_CONTROL_PAYLOAD_SIZE exceeded."};
        }

        // Configure sample rate and channels. This sends data.
        if (auto r = libusb_control_transfer (
                    deviceHandle_, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_OUT),
                    common::usb::VENDOR_CLASS_REQUEST, 1, 0, const_cast<uint8_t *> (std::data (request)), request.size (),
                    common::usb::TIMEOUT_MS);
            r < 0) {
                const_cast<UsbDevice *> (this)->notify ({}, State::error);
                throw Exception (libusb_error_name (r));
        }
}

/****************************************************************************/

std::vector<uint8_t> UsbDevice::controlIn (size_t len) const
{
        std::vector<uint8_t> request (len);
        /*
         * TODO VENDOR_CLASS_REQUEST is too speciffic to the logic-link device to be used in
         * abstract class like this. Leving this for now as is.
         */
        if (auto r = libusb_control_transfer (
                    deviceHandle_, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_IN),
                    common::usb::VENDOR_CLASS_REQUEST, 1, 0, request.data (), request.size (), common::usb::TIMEOUT_MS);
            r < 0) {
                const_cast<UsbDevice *> (this)->notify ({}, State::error);
                throw Exception (libusb_error_name (r));
        }

        return request;
}

/****************************************************************************/

std::string UsbDevice::getString (uint32_t clazz, uint32_t verb, size_t len) const
{
        controlOut (UsbRequest{}.clazz (clazz).verb (verb));
        auto data = controlIn (len);
        auto zero = std::ranges::find (data, '\0');
        return std::string (reinterpret_cast<char const *> (data.data ()), std::distance (data.begin (), zero));
}

/****************************************************************************/

void UsbDevice::writeTransmissionParams (UsbTransmissionParams const &params)
{
        if (running ()) {
                throw Exception{"UsbDevice::writeTransmissionParams called on a running device."};
        }

        // std::lock_guard lock{mutex};
        transmissionParams = params;
}

} // namespace logic