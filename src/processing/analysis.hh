/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include "analyzer/analyzer.hh"
#include "common/params.hh"
#include "data/types.hh"

namespace logic::an {

/**
 * @brief Perform the (debug) analysis using a strategy.
 *
 * @param rawData Data to analyze.
 * @param strategy Describes what to do.
 * @param discard Whether to discard the data or sotre it indefinitely in the RAM.
 */
void analyze (common::acq::Params const &params, data::Session *session, an::IAnalyzer *strategy, bool discard = false, bool decompress = false);

} // namespace logic::an