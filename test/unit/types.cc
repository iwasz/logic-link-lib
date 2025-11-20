/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
import logic.data;
using namespace logic;

/**
 * SampleIdx and SampleNum are equal only if get() and sampleRate() match for lhs and rhs OR
 * if get() is 0 for both rhs and lhs:
 */
TEST_CASE ("SampleIndex", "[types]")
{
        SECTION ("eq")
        {
                auto a = SampleIdx{0, 1_Sps};
                auto b = SampleIdx{0, 1_Sps};
                REQUIRE (a == b);

                auto c = SampleIdx{0, 1_Sps};
                auto d = SampleIdx{0, 2_Sps};
                REQUIRE (c == d);

                auto e = SampleIdx{100, 1_Sps};
                auto f = SampleIdx{100, 1_Sps};
                REQUIRE (e == f);
        }

        SECTION ("neq")
        {
                {
                        auto a = SampleIdx{10, 1_Sps};
                        auto b = SampleIdx{10, 2_Sps};
                        REQUIRE (a != b);
                }

                {
                        auto a = SampleIdx{400, 2_Sps};
                        auto b = SampleIdx{400, 8_Sps};
                        REQUIRE (a != b);
                }
        }

        SECTION ("eq Sn")
        {
                auto a = SampleNum{0, 1_Sps};
                auto b = SampleNum{0, 1_Sps};
                REQUIRE (a == b);

                auto c = SampleNum{0, 1_Sps};
                auto d = SampleNum{0, 2_Sps};
                REQUIRE (c == d);

                auto e = SampleNum{100, 1_Sps};
                auto f = SampleNum{100, 1_Sps};
                REQUIRE (e == f);
        }

        SECTION ("neq Sn")
        {
                {
                        auto a = SampleNum{10, 1_Sps};
                        auto b = SampleNum{10, 2_Sps};
                        REQUIRE (a != b);
                }

                {
                        auto a = SampleNum{400, 2_Sps};
                        auto b = SampleNum{400, 8_Sps};
                        REQUIRE (a != b);
                }
        }
}

TEST_CASE ("relativeIndex", "[types]")
{
        REQUIRE (relativeBegin (0_SI, 0_SI, 1) == 0_SI);
        REQUIRE (relativeBegin (2_SI, 0_SI, 1) == 2_SI);
        REQUIRE (relativeBegin (1000_SI, 0_SI, 1) == 1000_SI);

        REQUIRE (relativeBegin (20_SI, 16_SI, 1) == 4_SI);
        REQUIRE (relativeBegin (25_SI, 16_SI, 1) == 9_SI);

        REQUIRE (relativeBegin (20_SI, 16_SI, 2) == 2_SI);
        REQUIRE (relativeBegin (25_SI, 16_SI, 2) == 4_SI);

        REQUIRE (relativeEnd (20_SI, 16_SI, 1) == 4_SI);
        REQUIRE (relativeEnd (25_SI, 16_SI, 1) == 9_SI);

        REQUIRE (relativeEnd (20_SI, 16_SI, 2) == 1_SI);
        REQUIRE (relativeEnd (25_SI, 16_SI, 2) == 4_SI);
}

template <typename T> class Cls {};

/**
 * SampleNum and SampleIdx can be added and subtracted one from another.
 */
TEST_CASE ("operations", "[types]")
{
        SECTION ("idx subtract")
        {
                auto a = SampleIdx{10, 1_Sps};
                auto b = SampleIdx{5, 1_Sps};
                auto c = a - b;
                static_assert (std::is_same_v<decltype (c), SampleIdx>);

                REQUIRE (c.get () == 5);
                REQUIRE (c.sampleRate () == 1_Sps);
        }

        // SECTION ("fake subtract")
        // {
        //         auto a = Cls<int>{};
        //         auto b = Cls<int>{};
        //         auto c = a - b;
        //         static_assert (std::is_same_v<decltype (c), SampleIdx>);
        // }

        // auto len = SampleNum{10, 1_Sps};
        // auto idx = SampleIdx{5, 1_Sps};

        // auto len2 = len - idx;
        // static_assert (std::is_same_v<decltype (len2), SampleNum>);
        // REQUIRE (len2.get () == 5);
        // REQUIRE (len2.sampleRate () == 1_Sps);

        SECTION ("resample")
        {
                REQUIRE (resample (SampleIdx{5, 1_Sps}, 2_Sps) == SampleIdx{10, 2_Sps});
                REQUIRE (resample (SampleIdx{40, 10_Sps}, 1_Sps) == SampleIdx{4, 1_Sps});
                REQUIRE (resample (SampleIdx{40, 10_Sps}, 10_Sps) == SampleIdx{40, 10_Sps});
        }
}
