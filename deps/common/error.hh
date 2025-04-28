/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <array>
#include <cstdint>
#include <string_view>

namespace logs {
enum class Code : uint8_t {
        none,
        unknownRequest,
        noClassOrVerb,
        malformedConfig,
        malformedRequestUint32,
        malformedRequestUint8,
        setUint32,
        setUint8,
        usbQueueFull,
        malformedConfigChan0,
        wrongBlockSize,
        smallBlockSize,
        bigBlockSize,
        tooManyBlocks,
        sampleRateTooHigh,
        adcCalibrationFailed,
        malformedConfigChanAdc,
        notEnoughResources,
        peripheralSampleRateTooHigh,
        clko1clkError,
        clko2clkError,
        sampleRateTooLow
};

constexpr std::array message = {
        std::string_view{"None"},
        std::string_view{"Unrecognized request from the host."},
        std::string_view{"Malformed request from the host. No class or verb."},
        std::string_view{"Malformed configuration request from the host."},
        std::string_view{"Malformed request from the host. Can't decode an uint32 value."},
        std::string_view{"Malformed request from the host. Can't decode an uint8 value."},
        std::string_view{"Can't set uint32 value."},
        std::string_view{"Can't set uint8 value."},
        std::string_view{"Main data queue overflow."},
        std::string_view{"Bank 1 channel number can be only 1, 2, 4 or 8."},
        std::string_view{"Transfer size must be must be divisible by block size."},
        std::string_view{"Block size must be greater than 512B."},
        std::string_view{"Block size cannot be greater than the DMA_BUFFER_SIZE_B."},
        std::string_view{"Too many blocks."},
        std::string_view{"Too high sample rate requested."},
        std::string_view{"ADC auto-calibration failed."},
        std::string_view{"Analog channel number has to be between 1 and 8."},
        std::string_view{"Wrong channel configuration. Not enough resources."},
        std::string_view{"Improper peripheral configuration. Max sample rate is too high."},
        std::string_view{"Improper CLKO1_CLK configguration."},
        std::string_view{"Improper CLKO2_CLK configguration."},
        std::string_view{"Too low sample rate requested."},
};
} // namespace logs
