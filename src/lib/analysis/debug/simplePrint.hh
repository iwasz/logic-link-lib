/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analysis/analysis.hh"
#include "data/rawData.hh"

namespace an {

/**
 * Simply prints basic info for denug purposes only.
 */
class SimplePrint : public AbstractCheck {
public:
        using AbstractCheck::AbstractCheck;

        void start () override {}
        void run (data::SampleData const &samples) override;
        void stop () override {}

private:
        // size_t cnt{};
};

/**
 * Simply prints basic info for denug purposes only.
 */
class RawPrint : public AbstractCheck {
public:
        using AbstractCheck::AbstractCheck;

        void start () override {}
        void run (data::RawData const &rd) override;
        void run (data::SampleData const &samples) override {}
        void stop () override {}

private:
        size_t cnt{};
};

} // namespace an