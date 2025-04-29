/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "request.hh"
#include "common/constants.hh"
#include "common/serialize.hh"
#include "context.hh"
#include "exception.hh"
#include <algorithm>
#include <cstdio>
#include <cstring>
#include <deque>
#include <gsl/gsl>
#include <iterator>
#include <libusb.h>
#include <print>
#include <ranges>
#include <string>
#include <vector>

namespace logic::usb {
using namespace std::string_literals;
using namespace common::usb;

// constexpr size_t DEFAULT_DEVICE_BUFFER_B = 8192;

/****************************************************************************/

Context &ctx ()
{
        static Context _;
        return _;
}

/****************************************************************************/

void init ()
{
        if (int r = libusb_init_context (/*ctx=*/nullptr, /*options=*/nullptr, /*num_options=*/0); r < 0) {
                throw Exception ("Failed to init USB (libusb_init_context): "s + libusb_error_name (r));
        }

        ctx ().initialized = true;

        libusb_device **devs{};
        if (auto cnt = libusb_get_device_list (nullptr, &devs); cnt < 0) {
                throw Exception ("Failed to get a device list. Cnt: "s + std::to_string (cnt));
        }

        // printDevs (devs);
        libusb_free_device_list (devs, 1);

        if (ctx ().dev = libusb_open_device_with_vid_pid (nullptr, VID, PID); ctx ().dev == nullptr) {
                throw Exception ("Error finding USB device.");
        }

        // if (auto r = libusb_reset_device (ctx ().dev); r < 0) {
        //         throw Exception ("Error resetting interface : "s + libusb_error_name (r));
        // }

        if (auto r = libusb_claim_interface (ctx ().dev, 0); r < 0) {
                throw Exception ("Error claiming interface : "s + libusb_error_name (r));
        }

        if (auto r = libusb_set_interface_alt_setting (ctx ().dev, 0, 0); r < 0) {
                throw Exception ("Error to libusb_set_interface_alt_setting: "s + libusb_error_name (r));
        }

        /*
         * TODO this is problematic, and IDK why. On python this works OK, though I didn't pass 0 or any
         * other argument. Whereas here I get warning in the dmesg, and LIBUSB_ERROR_BUSY in console.
         */
        // if (auto r = libusb_set_configuration (ctx ().dev, 0); r < 0) {
        //         close();
        //         throw Exception ("Error to libusb_set_configuration: "s + libusb_error_name (r));
        // }
}

/****************************************************************************/

void controlOut (std::vector<uint8_t> request)
{
        if (!ctx ().initialized) {
                throw Exception{"`controlOut` called, but USB is not initialized."};
        }

        if (request.size () > MAX_CONTROL_PAYLOAD_SIZE) {
                throw Exception{"MAX_CONTROL_PAYLOAD_SIZE exceeded."};
        }

        // Configure sample rate and channels. This sends data.
        if (auto r = libusb_control_transfer (
                    ctx ().dev, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_OUT),
                    VENDOR_CLASS_REQUEST, 1, 0, const_cast<uint8_t *> (std::data (request)), request.size (), TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }
}

/****************************************************************************/

class Request {
public:
        Request &clazz (uint32_t c = GREATFET_CLASS_LA) { return integer (c); }
        Request &verb (uint32_t v) { return integer (v); }
        Request &sampleRate (uint32_t v) { return integer (v); }
        Request &channels (uint8_t v) { return integer (v); }
        Request &size (uint32_t v) { return integer (v); }

        template <std::integral T> Request &integer (T u)
        {
                data_.resize (data_.size () + sizeof (u));
                uint8_t const *p = reinterpret_cast<uint8_t *> (&u);
                std::copy (p, std::next (p, sizeof (u)), std::prev (data_.end (), sizeof (u)));
                return *this;
        }

        std::vector<uint8_t> const &data () const { return data_; }

private:
        std::vector<uint8_t> data_;
};

/****************************************************************************/

void controlOut (Request const &request) { controlOut (request.data ()); }

/****************************************************************************/

auto controlIn (size_t len)
{
        if (!ctx ().initialized) {
                throw Exception{"`controlIn` called, but USB is not initialized."};
        }

        std::vector<uint8_t> request (len);

        if (auto r = libusb_control_transfer (
                    ctx ().dev, uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_IN),
                    VENDOR_CLASS_REQUEST, 1, 0, request.data (), request.size (), TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }

        return request;
}

/****************************************************************************/

common::acq::Params configure (common::acq::Params const &params, bool legacy)
{
        using enum common::acq::Mode;

        if (legacy) {
                controlOut (Request{}
                                    .clazz (GREATFET_CLASS_LA)
                                    .verb (LA_VERB_CONFIGURE)
                                    .sampleRate (params.digitalSampleRate)
                                    .channels (params.digitalChannels));

                /*
                 * This one requests data from the control ep. It wants 9B of response from
                 * the previous transfer (configure sample rate and chgannel)
                 */
                auto request = controlIn (9);
                return params;
        }

        controlOut (Request{}
                            .clazz (LOGIC_LINK_CLASS_LA)
                            .verb (LL_VERB_CONFIGURE)
                            .sampleRate (params.digitalSampleRate)
                            .channels (params.digitalChannels)
                            .sampleRate (params.analogSampleRate)
                            .channels (params.analogChannels)
                            .integer (uint8_t (params.mode)));

        auto request = controlIn (13);

        common::acq::Params paramsOut{};
        common::serialize (&request)
                .get (&paramsOut.digitalSampleRate)
                .get (&paramsOut.digitalChannels)
                .get (&paramsOut.digitalEncoding)
                .get (&paramsOut.analogSampleRate)
                .get (&paramsOut.analogChannels)
                .get (&paramsOut.analogEncoding)
                .get (&paramsOut.mode);

        return paramsOut;
}

/****************************************************************************/

/*
 * TODO legacy makes little sense doesn't it? Current device as it is implemented now is not able
 * to provide the data in format digestable for the sigrok library. Consider removing the legacy
 * param and the implementation.
 */
void start () { controlOut (Request{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_START_CAPTURE)); }

/****************************************************************************/

void stop (data::Session *rawData)
{
        rawData->stop = true;
        controlOut (Request{}.clazz ().verb (LA_VERB_STOP_CAPTURE));
}

/****************************************************************************/

void close ()
{
        if (ctx ().initialized) {
                libusb_exit (nullptr);
                ctx ().initialized = false;
        }
}

/****************************************************************************/

common::usb::Stats getStats ()
{
        controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_STATS));
        auto rawStats = controlIn (sizeof (common::usb::Stats));
        common::usb::Stats stats{};
        memcpy (&stats, rawStats.data (), rawStats.size ());
        return stats;
}

/****************************************************************************/

std::unordered_set<logs::Code> getErrors ()
{
        controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS));
        auto rawStats = controlIn (MAX_CONTROL_PAYLOAD_SIZE);

        if (rawStats.empty ()) { // Should not happen.
                return {};
        }

        size_t errorNum = rawStats.back ();

        return rawStats | std::views::take (errorNum) | std::views::transform ([] (uint8_t b) { return static_cast<logs::Code> (b); })
                | std::ranges::to<std::unordered_set<logs::Code>> ();
}

/****************************************************************************/

void clearErrors () { controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS)); }

/****************************************************************************/

void setTransmissionParams (uint32_t usbTransfer, uint32_t usbBlock /* , uint32_t dmaBlock */)
{
        controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_SET_USB_TRANSFER_PARAMS).integer (usbTransfer).integer (usbBlock)
                    /* .integer (dmaBlock) */);
}

} // namespace logic::usb
