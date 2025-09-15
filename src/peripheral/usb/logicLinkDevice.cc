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
#include <format>
#include <libusb.h>
#include <ranges>
#include <unordered_set>
#include <vector>
module logic.peripheral;
import :device.link;

namespace logic {
using namespace common::usb;

LogicLink::LogicLink (EventQueue *eventQueue, libusb_device_handle *dev) : UsbDevice{eventQueue, dev}
{
        UsbInterface info = {
                .claimInterface = 0,
                .interfaceNumber = 0,
                .alternateSetting = 0,
        };

        open (info);
}

/****************************************************************************/

/*
 * TODO legacy makes little sense doesn't it? Current device as it is implemented now is not able
 * to provide the data in format digestable for the sigrok library. Consider removing the legacy
 * param and the implementation.
 */
void LogicLink::start (IBackend *backend)
{
        // Prepare the transfer and send it.
        UsbDevice::start (backend);

        // Send the start USB control request.
        controlOut (UsbRequest{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_START_CAPTURE));
}

/****************************************************************************/

void LogicLink::stop ()
{
        controlOut (UsbRequest{}.clazz (GREATFET_CLASS_LA).verb (LA_VERB_STOP_CAPTURE));
        UsbDevice::stop ();
}

/****************************************************************************/

void LogicLink::writeAcquisitionParams (common::acq::Params const &params, bool legacy)
{
        UsbDevice::writeAcquisitionParams (params, legacy);

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
                auto request = controlIn (LA_VERB_CONFIGURE_MAX_SIZE);
                // return params;
        }

        controlOut (UsbRequest{}
                            .clazz (LOGIC_LINK_CLASS_LA)
                            .verb (LL_VERB_CONFIGURE_WRITE)
                            .sampleRate (params.digitalSampleRate)
                            .channels (params.digitalChannels)
                            .sampleRate (params.analogSampleRate)
                            .channels (params.analogChannels)
                            .integer (uint8_t (params.mode)));
}

/****************************************************************************/

common::acq::Params LogicLink::readAcquisitionParams () const
{
        controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_CONFIGURE_READ));
        auto request = controlIn (LL_CONFIGURE_MAX_SIZE);

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

void LogicLink::writeTransmissionParams (UsbTransmissionParams const &params)
{
        UsbDevice::writeTransmissionParams (params);
        controlOut (UsbRequest{}
                            .clazz (LOGIC_LINK_CLASS_LA)
                            .verb (LL_VERB_SET_USB_TRANSFER_PARAMS)
                            //     .integer (params.usbTransfer)
                            .integer (params.usbBlock)
                    /* .integer (dmaBlock) */);
}

/****************************************************************************/

UsbTransmissionParams LogicLink::readTransmissionParams () const
{
        controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_GET_USB_TRANSFER_PARAMS));
        auto request = controlIn (LL_TRANSFER_PARAMS_MAX_SIZE);
        UsbTransmissionParams out{};
        out.singleTransferLenB = singleTransferLenB ();
        common::serialize (&request) /* .get (&out.usbTransfer) */.get (&out.usbBlock);
        return out;
}

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

std::string LogicLink::hwVersion () const { return getString (LOGIC_LINK_CLASS_LA, LL_VERB_HW_VERSION, LL_VERSION_SERIAL_MAX_SIZE); }
std::string LogicLink::fwVersion () const { return getString (LOGIC_LINK_CLASS_LA, LL_VERB_FW_VERSION, LL_VERSION_SERIAL_MAX_SIZE); }
std::string LogicLink::deviceSerial () const { return getString (LOGIC_LINK_CLASS_LA, LL_VERB_DEVICE_SERIAL, LL_VERSION_SERIAL_MAX_SIZE); }
std::string LogicLink::mcuSerial () const
{
        controlOut (UsbRequest{}.clazz (LOGIC_LINK_CLASS_LA).verb (LL_VERB_MCU_SERIAL));
        auto buf = controlIn (8);
        int ID_0{};
        int ID_1{};
        memcpy (&ID_0, buf.data (), 4);
        memcpy (&ID_1, buf.data () + 4, 4);
        return std::format ("{:x}{:x}", ID_0, ID_1);
}

} // namespace logic