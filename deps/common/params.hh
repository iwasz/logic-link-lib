/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "constants.hh"
#include <cstdlib>

namespace common::acq {

struct Params {
        SampleRate digitalSampleRate{};
        int digitalChannels{}; // Int due to arg_parse limitation
        DigitalChannelEncoding digitalEncoding{};

        SampleRate analogSampleRate{};
        int analogChannels{};
        AnalogChannelEncoding analogEncoding{};

        Mode mode{};
};

} // namespace common::acq