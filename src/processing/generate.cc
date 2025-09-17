/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <algorithm>
#include <climits>
#include <cmath>
module logic.processing;

namespace logic {

Bytes square (size_t hiBits, size_t loBits, size_t bitsTotal, bool state)
{
        auto bytesTotal = size_t (std::round (bitsTotal / CHAR_BIT));
        Bytes buf (bytesTotal);
        std::ranges::fill (buf, 0);

        size_t scnt{};

        for (uint8_t &b : buf) {
                for (int i = CHAR_BIT - 1; i >= 0; --i) {
                        if (state) {
                                b |= (1 << i);

                                if (++scnt >= hiBits) {
                                        state = !state;
                                        scnt = 0;
                                }
                        }
                        else {
                                if (++scnt >= loBits) {
                                        state = !state;
                                        scnt = 0;
                                }
                        }
                }
        }

        return buf;
}

} // namespace logic