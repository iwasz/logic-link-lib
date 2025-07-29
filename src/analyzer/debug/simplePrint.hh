/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#if 0
#pragma once
#include "analyzer/analyzer.hh"
#include "data/types.hh"

namespace logic::an {

/**
 * Simply prints basic info for denug purposes only.
 */
class SimplePrint : public AbstractAnalyzer {
public:
        using AbstractAnalyzer::AbstractAnalyzer;

        void start () override {}
        AugumentedData run (SampleBuffers const &samples) override;
        void stop () override {}

private:
        // size_t cnt{};
};

/**
 * Simply prints basic info for denug purposes only.
 */
class RawPrint : public AbstractAnalyzer {
public:
        using AbstractAnalyzer::AbstractAnalyzer;

        void start () override {}
        AugumentedData runRaw (RawData const &rd) override;
        AugumentedData run (SampleBuffers const &samples) override { return {}; }
        void stop () override {}

private:
        size_t cnt{};
};

} // namespace logic::an
#endif