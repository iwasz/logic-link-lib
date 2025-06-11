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

TEST_CASE ("Valid", "[integrity]")
{
        std::array a = {
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
                0b00000000000000111000000000000000U, //
                0b00000000001110000000000000000000U, //
                0b00000011100000000000000000000000U, //
        };

        std::array b = {
                0b00111000000000000000000000000000U, //
                0b10000000000000000000000000000000U, //
                0b00000000000000000000000000000011U, //
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
        };

        an::ClockSignalCheck csc; // dmaBlockLength is not important in  this case
        REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 0);
        REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 0);
}

TEST_CASE ("Valid one by one", "[integrity]")
{
        std::array a = {
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
                0b00000000000000111000000000000000U, //
                0b00000000001110000000000000000000U, //
                0b00000011100000000000000000000000U, //
                0b00111000000000000000000000000000U, //
                0b10000000000000000000000000000000U, //
                0b00000000000000000000000000000011U, //
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
        };

        an::ClockSignalCheck csc;

        for (auto const &x : a) {
                csc.analyzeDataIntegrity (x);
        }

        REQUIRE (csc.errorNum () == 0);
}

TEST_CASE ("Invalid", "[integrity]")
{
        std::array a = {
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000001100000000011100000000000U, //
                0b00000000000000111000000000000000U, //
                0b00000000001110000000000000000000U, //
                0b00000011100000000000000000000000U, //
        };

        std::array b = {
                0b00111000000000000001100000000000U, //
                0b10000000000000000000000000000000U, //
                0b00000000000000000000000000000011U, //
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
        };

        an::ClockSignalCheck csc;
        REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 1);
        REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 2);

        // Reatart test
        csc = an::ClockSignalCheck{};
        REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 1);
        REQUIRE ((csc.analyzeDataIntegrity (b.data (), b.size ())) == 2);
}

TEST_CASE ("Invalid one by one", "[integrity]")
{
        std::array a = {
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000001100000000011100000000000U, //
                0b00000000000000111000000000000000U, //
                0b00000000001110000000000000000000U, //
                0b00000011100000000000000000000000U, //
                0b00111000000000000001100000000000U, //
                0b10000000000000000000000000000000U, //
                0b00000000000000000000000000000011U, //
                0b00000000000000000000000000111000U, //
                0b00000000000000000000001110000000U, //
                0b00000000000000000011100000000000U, //
        };

        an::ClockSignalCheck csc;

        for (auto const &x : a) {
                csc.analyzeDataIntegrity (x);
        }

        REQUIRE (csc.errorNum () == 2);
}

TEST_CASE ("Use case 1", "[integrity]")
{
        std::array a = {

                0b11111100000000001111111111000000U, //
                0b00000000111111111100000000001111U, //
        };

        an::ClockSignalCheck csc; // dmaBlockLength is not important in  this case
        REQUIRE ((csc.analyzeDataIntegrity (a.data (), a.size ())) == 0);
}