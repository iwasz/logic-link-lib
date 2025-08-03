/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <catch2/catch_test_macros.hpp>
#include <thread>
import logic;
using namespace logic;
using namespace std::chrono_literals;

TEST_CASE ("Event queue", "[eventQueue]")
{
        EventQueue events;
        std::string lastName;

        events.addCallback<UsbConnected> ([&lastName] (std::string const &name) { lastName = name; });
        events.addEvent<UsbConnected> (std::string{"deviceName"});

        REQUIRE (lastName.empty ());
        events.run ();
        REQUIRE (lastName == "deviceName");
}

TEST_CASE ("Event queue threads", "[eventQueue]")
{
        EventQueue events;
        std::vector<std::string> names;

        events.addCallback<UsbConnected> ([&names] (std::string const &name) { names.push_back (name); });

        std::thread t{[&events] {
                for (int i = 0; i < 10; ++i) {
                        events.addEvent<UsbConnected> (std::to_string (i));
                        std::this_thread::sleep_for (10ms);
                }
        }};

        std::thread t2{[&events] {
                for (int i = 10; i < 20; ++i) {
                        events.addEvent<UsbConnected> (std::to_string (i));
                        std::this_thread::sleep_for (10ms);
                }
        }};

        REQUIRE (names.empty ());

        while (names.size () < 20) {
                events.run ();
        }

        REQUIRE (names.size () == 20);
        t.join ();
        t2.join ();
}
