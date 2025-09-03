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
#include <libusb.h>
#include <string>
#include <vector>
module logic.peripheral;

namespace logic {

void UsbDevice::open (UsbInterface const &info)
{
        using namespace std::string_literals;
        // UsbDeviceInfo const &info = std::any_cast<UsbDeviceInfo const &> (in);
        // libusb_device_handle *dev{};

        // if (dev = libusb_open_device_with_vid_pid (nullptr, info.vid, info.pid); dev == nullptr) {
        //         throw Exception ("Error finding USB device. VID: " + std::to_string (info.vid) + ", PID: " + std::to_string (info.pid));
        // }

        // if (auto r = libusb_reset_device (dev); r < 0) {
        //         throw Exception ("Error resetting interface : "s + libusb_error_name (r));
        // }

        if (auto r = libusb_claim_interface (device (), info.claimInterface); r < 0) {
                throw Exception ("Error claiming interface: "s + std::to_string (info.claimInterface) + ", msg: "s + libusb_error_name (r));
        }

        if (auto r = libusb_set_interface_alt_setting (device (), info.interfaceNumber, info.alternateSetting); r < 0) {
                throw Exception ("Error libusb_set_interface_alt_setting: "s + libusb_error_name (r));
        }

        /*
         * TODO this is problematic, and IDK why. On python this works OK, though I didn't pass 0 or any
         * other argument. Whereas here I get warning in the dmesg, and LIBUSB_ERROR_BUSY in console.
         */
        // if (auto r = libusb_set_configuration (dev, 0); r < 0) {
        //         close();
        //         throw Exception ("Error to libusb_set_configuration: "s + libusb_error_name (r));
        // }
}

/****************************************************************************/

void UsbDevice::controlOut (std::vector<uint8_t> const &request)
{
        if (request.size () > common::usb::MAX_CONTROL_PAYLOAD_SIZE) {
                throw Exception{"MAX_CONTROL_PAYLOAD_SIZE exceeded."};
        }

        // Configure sample rate and channels. This sends data.
        if (auto r = libusb_control_transfer (
                    device (), uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_OUT),
                    common::usb::VENDOR_CLASS_REQUEST, 1, 0, const_cast<uint8_t *> (std::data (request)), request.size (),
                    common::usb::TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }
}

/****************************************************************************/

std::vector<uint8_t> UsbDevice::controlIn (size_t len)
{
        std::vector<uint8_t> request (len);
        /*
         * TODO VENDOR_CLASS_REQUEST is too speciffic to the logic-link device to be used in
         * abstract class like this. Leving this for now as is.
         */
        if (auto r = libusb_control_transfer (
                    device (), uint8_t (LIBUSB_RECIPIENT_ENDPOINT) | uint8_t (LIBUSB_REQUEST_TYPE_VENDOR) | uint8_t (LIBUSB_ENDPOINT_IN),
                    common::usb::VENDOR_CLASS_REQUEST, 1, 0, request.data (), request.size (), common::usb::TIMEOUT_MS);
            r < 0) {
                throw Exception (libusb_error_name (r));
        }

        return request;
}

/****************************************************************************/

std::string UsbDevice::getString (uint32_t clazz, uint32_t verb, size_t len)
{
        controlOut (UsbRequest{}.clazz (clazz).verb (verb));
        auto data = controlIn (len);
        auto zero = std::ranges::find (data, '\0');
        return std::string (reinterpret_cast<char const *> (data.data ()), std::distance (data.begin (), zero));
}

} // namespace logic