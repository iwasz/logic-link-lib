/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#pragma once
#include <exception>
#include <string>

namespace logic {

class Exception : public std::exception {
public:
        Exception (std::string message) : message_{std::move (message)} {}
        const char *what () const noexcept override { return message_.c_str (); }

        std::string &message () { return message_; }
        std::string const &message () const { return message_; }

private:
        std::string message_;
};

} // namespace logic