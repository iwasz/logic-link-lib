/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/
#if 0
#include "simplePrint.hh"
#include <cstdio>
#include <print>
#include <span>

namespace logic::an {

/****************************************************************************/

data::AugumentedData SimplePrint::run (data::SampleBuffers const &samples)
{
        // if (cnt++ > 0) {
        //         return;
        // }

        std::vector<data::Bytes::const_iterator> diters (samples.digital.size ());

        for (size_t chan = 0; chan < samples.digital.size (); ++chan) {
                diters.at (chan) = samples.digital.at (chan).begin ();
        }

        size_t cnt{};

        while (true) {
                if (++cnt > samples.digital.front ().size ()) {
                        break;
                }

                for (auto &iter : diters) {
                        std::print ("{:08b} ", *iter++);
                }

                std::println ("");
        }

        return {};
}

/****************************************************************************/

data::AugumentedData RawPrint::runRaw (data::RawData const &raw)
{
        size_t row{};
        size_t column{};
        for (uint8_t ch : raw.buffer) {
                std::print ("{:08b}", ch);

                if (++column >= 4) {
                        column = 0;
                        std::println ();

                        if (++row % 4 == 0) {
                                std::println ();
                        }
                }
        }

        return {};
}

} // namespace logic::an
#endif