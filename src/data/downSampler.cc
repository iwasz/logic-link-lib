/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/constants.hh"
#include <Tracy.hpp>
module logic.data;
import logic.processing;

namespace logic {

Bytes DigitalDownSampler::operator() (Bytes const &block, size_t zoomOut) const { return logic::lut::downsample (block, zoomOut, &state); }

} // namespace logic