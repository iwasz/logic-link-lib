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

BASELINE (Benchmark, DownsampleT2, 10, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<2> (data, &s));
}

BENCHMARK (Benchmark, Downsample2, 10, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample2 (data, &s));
}

BENCHMARK (Benchmark, DownsampleT4, 5, 1000)
{
        bool s{};
        celero::DoNotOptimizeAway (downsample<4> (data, &s));
}

BENCHMARK (Benchmark, Downsample2x2, 5, 1000)
{
        bool s1{};
        bool s2{};
        celero::DoNotOptimizeAway (downsample<2> (downsample<2> (data, &s1), &s2));
}

BENCHMARK (Benchmark, Downsample4, 5, 1000)
{
        uint8_t s = 0;
        celero::DoNotOptimizeAway (downsample4 (data, &s));
}
