/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "data/types.hh"

namespace logic::an::uart {

enum class Parity : uint8_t { none, odd, even };

class UartAnalyzer : public AbstractAnalyzer {
public:
        using AbstractAnalyzer::AbstractAnalyzer;

        void start () override {}
        data::AugumentedData run (data::SampleBlockStream const &samples) override;
        void stop () override {}

private:
};

} // namespace logic::an::uart