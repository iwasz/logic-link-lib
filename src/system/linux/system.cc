/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include "exception.hh"
#include <csignal>
#include <signal.h>

namespace logic::sys {
volatile std::sig_atomic_t gSignalStatus;
namespace {
}

void signalHandler (int signal) { gSignalStatus = signal; }

/****************************************************************************/

void init ()
{
        struct sigaction sa{};

        sa.sa_handler = signalHandler;
        sigemptyset (&sa.sa_mask);
        sa.sa_flags = SA_RESTART;                /* Restart functions if
                                                    interrupted by handler */
        if (sigaction (SIGINT, &sa, NULL) == -1) /* Handle error */
                throw Exception{"Failed to install a signal handler"};

        /* Further code */
}

/****************************************************************************/

bool isTermRequested () { return gSignalStatus != 0; }

} // namespace logic::sys