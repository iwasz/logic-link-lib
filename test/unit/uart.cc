/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
#include <catch2/catch_test_macros.hpp>

using namespace logic;

#if 0


namespace public_interface /* as opposed to impl */ {


} // namespace public_interface

namespace private_aka_data_etc {
struct IBacked {};
class MockBackend : public IBacked {}; // for testingf

/**
 * This is where the real data is stored.
 * It may be encoded, compressed, whatever, the important thing is, that we copy portions
 * of it upon users' requests and return it to the user.
 *
 */
class RealBackend : public IBacked {}; // for testingf
} // namespace private_aka_data_etc
#endif

TEST_CASE ("Valid", "[uart]")
{
        data::SampleBlockStream in;
        // in.digital.push_back (data::Bytes{1, 2, 3});

        an::uart::UartAnalyzer a{0};
        /* auto out =  */ a.run (in);

        // REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 0);
        // REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 0);
}
