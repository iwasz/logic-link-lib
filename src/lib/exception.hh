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

class Exception : public std::exception {
public:
        Exception (std::string message) : message_{std::move (message)} {}
        const char *what () const noexcept override { return message_.c_str (); }

private:
        std::string message_;
};