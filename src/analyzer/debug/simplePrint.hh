/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "types.hh"

namespace logic::an {

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
        void runRaw (data::RawData const &rd) override;
        void run (data::SampleData const &samples) override {}
        void stop () override {}

private:
        size_t cnt{};
};

} // namespace logic::an