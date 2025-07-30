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
#include <ranges>
#include <unordered_set>
#include <vector>
module logic;
import :device.link;

namespace logic {
using namespace common::usb;

LogicLink::LogicLink (IInput *input) : AbstractDevice{input}
{
        DeviceInfo info = {
                .vid = VID,
                .pid = PID,
                .claimInterface = 0,
                .interfaceNumber = 0,
                .alternateSetting = 0,
        };

        DeviceHooks hooks;
        hooks.startHook = [this] { start (); };
        hooks.stopHook = [this] { stop (); };

        input->open (hooks, info);
}

/****************************************************************************/

common::acq::Params LogicLink::configureAcquisition (common::acq::Params const &params, bool legacy)
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
                auto request = input ()->controlIn (9);
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

        auto request = input ()->controlIn (13);

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
void LogicLink::start () { controlOut (Request{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_START_CAPTURE)); }

/****************************************************************************/

void LogicLink::stop ()
{
        // stop_ = true;
        controlOut (Request{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_STOP_CAPTURE));
}

/****************************************************************************/

common::usb::Stats LogicLink::getStats ()
{
        controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_STATS));
        auto rawStats = input ()->controlIn (sizeof (common::usb::Stats));
        common::usb::Stats stats{};
        memcpy (&stats, rawStats.data (), rawStats.size ());
        return stats;
}

/****************************************************************************/

std::unordered_set<logs::Code> LogicLink::getErrors ()
{
        controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS));
        auto rawStats = input ()->controlIn (MAX_CONTROL_PAYLOAD_SIZE);

        if (rawStats.empty ()) { // Should not happen.
                return {};
        }

        size_t errorNum = rawStats.back ();

        return rawStats | std::views::take (errorNum) | std::views::transform ([] (uint8_t b) { return static_cast<logs::Code> (b); })
                | std::ranges::to<std::unordered_set<logs::Code>> ();
}

/****************************************************************************/

void LogicLink::clearErrors () { controlOut (Request{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_ERRORS)); }

/****************************************************************************/

void LogicLink::configureTransmission (TransmissionParams const &params)
{AbstractDevice::configureTransmission (params);
        controlOut (Request{}
                            .clazz (LOGIC_LINK_CLASS_LA)
                            .verb (LL_VERB_SET_USB_TRANSFER_PARAMS)
                            .integer (params.usbTransfer)
                            .integer (params.usbBlock)
                    /* .integer (dmaBlock) */);
}

} // namespace logic