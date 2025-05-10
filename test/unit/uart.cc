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

/**
 * Sample number with corresponding sample rate.
 */
class SampleNo {
public:
        SampleNo (uint64_t s) : sampleNumber_{s} {}
        operator uint64_t () const { return sampleNumber_; }

        SampleRate &sampleRate () { return sampleRate_; }
        SampleRate const &sampleRate () const { return sampleRate_; }

        // operators =, >, < will take sample rate into account.

private:
        SampleRate sampleRate_{};
        uint64_t sampleNumber_;
};

namespace public_interface /* as opposed to impl */ {

class Range {
        // group[0, 1, 2];
        //  Block Attahced block Attached hints (by reference)
        //                 Attached blocks by value ;
        //                 Attached hints by value;
        //         < -all of this is copied form the "database"
};

class Frontend {
public:
        /**
         * All available new data, that was not read before. This is intrusive operation,
         * meaning that you get the data only once. It works like Java Iterator, but if there's
         * no new data, it will wait on a conditional_variable.
         */
        Range next ();

        /**
         * Returns all available data from a range of sample numbers.
         */
        Range range (SampleNo const &begin, SampleNo const &end);
        Range range (data::TimePoint const &begin, data::TimePoint const &end);
};

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

TEST_CASE ("Valid", "[uart]")
{
        data::SampleBlockStream in;
        // in.digital.push_back (data::Bytes{1, 2, 3});

        an::uart::UartAnalyzer a{0};
        /* auto out =  */ a.run (in);

        // REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 0);
        // REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 0);
}
