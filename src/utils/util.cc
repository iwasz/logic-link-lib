/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

module;
#ifdef __linux__
#include <pthread.h>
#endif
#include <format>
#include <string>

#ifdef TRACY_ENABLE
#include <Tracy.hpp>
#endif
module logic.util;
import logic.core;

#ifdef __linux__
void setThreadName (std::string const &name)
{
#ifdef TRACY_ENABLE
        tracy::SetThreadName (name.c_str ());
#else
        pthread_t tid = pthread_self ();
        if (pthread_setname_np (tid, name.c_str ()) != 0) {
                throw logic::Exception{std::format ("setThreadName to '{}' has failed", name)};
        }
#endif
}
#else
void setThreadName (std::string const &name) {}
#endif
