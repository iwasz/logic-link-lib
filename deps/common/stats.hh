/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <cstdint>
#include <cstdlib>

namespace usb {

struct Stats {
        uint16_t addErrors = 0;

        uint16_t send1Fatal = 0;
        uint16_t send1Busy = 0;
        uint16_t send1Empty = 0;

        uint16_t send2Fatal = 0;
        uint16_t send2Busy = 0;
        uint16_t send2Empty = 0;

        struct Queue {
                /**
                 * Average size (average usage at the moment of adding a new element). Warning
                 * this statistic is very limited, because its elements are stored only in the
                 * `add` method.
                 */
                uint16_t maxSize{};
        } queue;

        void clear () { *this = {}; }
}; // namespace usb

// Stats &stats ();

inline Stats &stats ()
{
        static Stats _;
        return _;
}

} // namespace usb