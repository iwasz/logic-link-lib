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
#include <any>
#include <atomic>
#include <cstdlib>
#include <mutex>
module logic;
import :input.demo;

namespace logic {

void DemoInput::open (std::any const &info) { blockSize = std::any_cast<size_t> (info); }

/****************************************************************************/

void DemoInput::run ()
{
        Bytes singleTransfer;

        while (true) {
                if (running_) {
                        singleTransfer.resize (blockSize);
                        std::ranges::generate (singleTransfer, [] { return std::rand () % 256; });
                        RawCompressedBlock rcd{0, 0, std::move (singleTransfer)}; // TODO resize blocks (if needed)
                        session->rawQueue.push (std::move (rcd));
                }
                else {
                        session = nullptr;
                }

                if (stop_ && singleTransfer.empty ()) {
                        break;
                }
        }
}

/****************************************************************************/

void DemoInput::start (Session *session)
{
        std::lock_guard lock{mutex};
        this->session = session;
        running_ = true;
}

/****************************************************************************/

void DemoInput::stop () { running_ = false; }

} // namespace logic