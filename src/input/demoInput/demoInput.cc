/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#include "common/params.hh"
#include <algorithm>
#include <atomic>
#include <cstdlib>
module logic;
import :input.demo;

namespace logic {

void DemoInput::run (Queue<RawCompressedBlock> *queue, size_t singleTransferLenB)
{
        Bytes singleTransfer (singleTransferLenB);

        while (true) {
                if (running_) {
                        std::ranges::generate (singleTransfer, [] { return std::rand () % 256; });
                        RawCompressedBlock rcd{0, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
                        queue->push (std::move (rcd));
                }

                if (stop_ && singleTransfer.empty ()) {
                        break;
                }
        }
}

/****************************************************************************/

void DemoInput::start () { running_ = true; }

/****************************************************************************/

void DemoInput::stop () { running_ = false; }

} // namespace logic