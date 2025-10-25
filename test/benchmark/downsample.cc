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
        uint8_t s{};
        celero::DoNotOptimizeAway (pop::downsample (data, 2, &s));
}

BENCHMARK (Benchmark, Downsample2, 10, 1000)
{
        uint8_t s{};
        celero::DoNotOptimizeAway (lut::downsample (data, 2, &s));
}

BENCHMARK (Benchmark, DownsampleT4, 5, 1000)
{
        uint8_t s{};
        celero::DoNotOptimizeAway (pop::downsample (data, 4, &s));
}

BENCHMARK (Benchmark, Downsample2x2, 5, 1000)
{
        uint8_t s1{};
        uint8_t s2{};
        celero::DoNotOptimizeAway (pop::downsample (pop::downsample (data, 2, &s1), 2, &s2));
}

BENCHMARK (Benchmark, Downsample4, 5, 1000)
{
        uint8_t s = 0;
        celero::DoNotOptimizeAway (lut::downsample (data, 4, &s));
}

BENCHMARK (Benchmark, Downsample2x2x2, 5, 1000)
{
        uint8_t s1{};
        uint8_t s2{};
        uint8_t s3{};
        celero::DoNotOptimizeAway (pop::downsample (pop::downsample (pop::downsample (data, 2, &s1), 2, &s2), 2, &s3));
}

BENCHMARK (Benchmark, Downsample8, 5, 1000)
{
        uint8_t s = 0;
        celero::DoNotOptimizeAway (lut::downsample (data, 8, &s));
}
