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
module logic.util;
import logic.core;

#ifdef __linux__
void setThreadName (std::string const &name)
{
        pthread_t tid = pthread_self ();
        if (pthread_setname_np (tid, name.c_str ()) != 0) {
                throw logic::Exception{std::format ("setThreadName to '{}' has failed", name)};
        }
}
#else
void setThreadName (std::string const &name) {}
#endif