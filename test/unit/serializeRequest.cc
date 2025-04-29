/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "common/serialize.hh"
#include <catch2/catch_test_macros.hpp>

// TODO adapt to the Request class

// TEST_CASE ("Serialize to internal vector", "[common]")
// {
//         SECTION ("simple")
//         {
//                 auto s = common::serialize ().integer (uint32_t (0x12345678));
//                 REQUIRE (s.data () == std::vector<uint8_t>{0x78, 0x56, 0x34, 0x12});
//         }

//         SECTION ("mixed")
//         {
//                 auto s = common::serialize ()
//                                  .integer (uint32_t (0x12345678))
//                                  .integer (uint8_t (0x67))
//                                  .integer (uint32_t (0x87654321))
//                                  .integer (uint8_t (0x89));

//                 REQUIRE (s.data () == std::vector<uint8_t>{0x78, 0x56, 0x34, 0x12, 0x67, 0x21, 0x43, 0x65, 0x87, 0x89});
//         }
// }
