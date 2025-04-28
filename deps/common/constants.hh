/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "constants.h"
#include <cstdint>
#include <cstdlib>

using SampleRate = uint32_t;

namespace common {
namespace usb {
        constexpr uint8_t VENDOR_CLASS_REQUEST = 0x65;
        constexpr size_t TIMEOUT_MS = 1000;
        constexpr uint8_t IN_EP = 0x81;
        constexpr size_t MB_1_B = 0x100000; // 1 MiB
        constexpr uint16_t VID = 0x20a0;
        constexpr uint16_t PID = 0x41ff;

        // Maximum expected payload length (of control transfer) from PC to us.
        constexpr size_t MAX_CONTROL_PAYLOAD_SIZE = MAX_CONTROL_PAYLOAD_SIZE_DEF;

        static constexpr uint32_t GREATFET_CLASS_CORE = 0x000;
        static constexpr uint32_t CORE_VERB_READ_VERSION = 0x1;
        static constexpr uint32_t CORE_VERB_READ_SERIAL = 0x3;

        // Requestes compatible with GreatFet one and Sigrok.
        static constexpr uint32_t GREATFET_CLASS_LA = 0x10d;
        static constexpr uint32_t LA_VERB_CONFIGURE = 0x0;
        static constexpr uint32_t LA_VERB_FIRST_PIN = 0x1;
        static constexpr uint32_t LA_VERB_ALT_PIN_MAP = 0x2;
        static constexpr uint32_t LA_VERB_START_CAPTURE = 0x3;
        static constexpr uint32_t LA_VERB_STOP_CAPTURE = 0x4;

        // Logic-Link speciffic requests.
        static constexpr uint32_t LOGIC_LINK_CLASS_LA = 0x123;
        static constexpr uint32_t LL_VERB_DECIMATE = 0x0;
        static constexpr uint32_t LL_VERB_FIXED_BUFFER = 0x1;
        static constexpr uint32_t LL_VERB_STATS = 0x2;
        static constexpr uint32_t LL_VERB_ERRORS = 0x3;
        static constexpr uint32_t LL_VERB_CLEAR_ERRORS = 0x4;
        static constexpr uint32_t LL_VERB_SET_USB_TRANSFER_PARAMS = 0x5;
        static constexpr uint32_t LL_VERB_CONFIGURE = 0x6;
} // namespace usb

namespace acq {
        enum class DigitalChannelEncoding : uint8_t {
                flexio, // 1-8 channels of interleaved: 32bits of D0, 32bits of D1 ....
                gpio1_2 // Bytes 1 and 2 of every 32 bit word. Bit 8 -> D0 sample, bit 9 -> D1 sample etc...
        };

        enum class AnalogChannelEncoding : uint8_t {
                analog8bit, // 8 bit sample data
        };

        enum class Mode : uint8_t {
                acquisition,
                fixedBuffer,
        };
} // namespace acq
} // namespace common