/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "analyzer/lowLevel/uart.hh"
#include <catch2/catch_test_macros.hpp>

using namespace logic;

TEST_CASE ("Valid", "[uart]")
{
        data::SampleBlockStream in;
        // in.digital.push_back (data::Bytes{1, 2, 3});

        an::uart::UartAnalyzer a{0};
        /* auto out =  */ a.run (in);

        // REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 0);
        // REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 0);
}
