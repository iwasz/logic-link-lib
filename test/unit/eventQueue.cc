/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <functional>
import logic;
using namespace logic;

TEST_CASE ("Event queue", "[eventQueue]")
{

        EventQueue events;
        std::string lastName;

        std::function<void (std::string const &)> fun{[&lastName] (std::string const &name) { lastName = name; }};

        events.addCallback ("connected", std::make_unique<UsbCallback> (fun));
        events.addEvent ("connected", std::make_unique<UsbConnected> ("deviceName"));

        REQUIRE (lastName.empty ());
        events.run ();
        REQUIRE (lastName == "deviceName");
}

// TEST_CASE ("Event queue threads", "[eventQueue]")
// {

//         EventQueue events;
//         std::string lastName;

//         events.addCallback<UsbConnected> ([&lastName] (std::string const &name) { lastName = name; });
//         events.addEvent<UsbConnected> ("deviceName");

//         REQUIRE (lastName.empty ());
//         events.run ();
//         REQUIRE (lastName == "deviceName");
// }
