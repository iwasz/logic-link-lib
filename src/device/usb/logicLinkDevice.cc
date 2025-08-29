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
#include "common/serialize.hh"
#include "common/stats.hh"
#include <cstdint>
#include <cstdlib>
#include <cstring>
#include <libusb.h>
#include <ranges>
#include <unordered_set>
#include <vector>
module logic;
import :device.link;

namespace logic {
using namespace common::usb;

LogicLink::LogicLink (IInput *inp, libusb_device_handle *dev) : UsbDevice{inp, dev}
{
        UsbInterface info = {
                .claimInterface = 0,
                .interfaceNumber = 0,
                .alternateSetting = 0,
        };

        open (info);
}

/****************************************************************************/

common::acq::Params LogicLink::configureAcquisition (common::acq::Params const &params, bool legacy)
{
        using enum common::acq::Mode;

        if (legacy) {
                controlOut (UsbRequest{}
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

        controlOut (UsbRequest{}
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
void LogicLink::onStart () { controlOut (UsbRequest{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_START_CAPTURE)); }

/****************************************************************************/

void LogicLink::onStop () { controlOut (UsbRequest{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_STOP_CAPTURE)); }

/****************************************************************************/

common::usb::Stats LogicLink::getStats ()
{
        controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_STATS));
        auto rawStats = controlIn (sizeof (common::usb::Stats));
        common::usb::Stats stats{};
        memcpy (&stats, rawStats.data (), rawStats.size ());
        return stats;
}

/****************************************************************************/

std::unordered_set<logs::Code> LogicLink::getErrors ()
{
        controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS));
        auto rawStats = controlIn (MAX_CONTROL_PAYLOAD_SIZE);

        if (rawStats.empty ()) { // Should not happen.
                return {};
        }

        size_t errorNum = rawStats.back ();

        return rawStats | std::views::take (errorNum) | std::views::transform ([] (uint8_t b) { return static_cast<logs::Code> (b); })
                | std::ranges::to<std::unordered_set<logs::Code>> ();
}

/****************************************************************************/

void LogicLink::clearErrors () { controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS)); }

/****************************************************************************/

void LogicLink::configureTransmission (UsbTransmissionParams const &params)
{
        UsbDevice::configureTransmission (params);
        controlOut (UsbRequest{}
                            .clazz (LOGIC_LINK_CLASS_LA)
                            .verb (LL_VERB_SET_USB_TRANSFER_PARAMS)
                            .integer (params.usbTransfer)
                            .integer (params.usbBlock)
                    /* .integer (dmaBlock) */);
}

} // namespace logic