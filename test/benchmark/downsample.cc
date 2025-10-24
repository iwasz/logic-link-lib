/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

import logic;
using namespace logic;

#include <celero/Celero.h>

#include <cmath>
#include <cstdlib>
#include <random>

///
/// This is the main(int argc, char** argv) for the entire celero program.
/// You can write your own, or use this macro to insert the standard one into the project.
///
CELERO_MAIN

std::random_device RandomDevice;
std::uniform_int_distribution<uint8_t> UniformDistribution (0, 255);
Bytes data (DEFAULT_USB_TRANSFER_SIZE_B);

Bytes baselineFunction (Bytes const &in)
{
        Bytes copy (in.size ());

        for (size_t i = 0; auto ch : in) {
                copy.at (i++) = ch;
        }

        return copy;
}

// BASELINE (Benchmark, Baseline, 10, 1000) { celero::DoNotOptimizeAway (baselineFunction (data)); }

BASELINE (Benchmark, Downsample2, 10, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<2> (data, &s));
}

BENCHMARK (Benchmark, Downsample2_3, 10, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample3 (data, &s));
}

BENCHMARK (Benchmark, Downsample4, 2, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<4> (data, &s));
}

BENCHMARK (Benchmark, Downsample8, 4, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<8> (data, &s));
}

BENCHMARK (Benchmark, Downsample16, 8, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<16> (data, &s));
}
