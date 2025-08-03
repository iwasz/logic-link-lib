/****************************************************************************
 *                                                                          *
 *  Author : lukasz.iwaszkiewicz@gmail.com                                  *
 *  ~~~~~~~~                                                                *
 *  License : see COPYING file for details.                                 *
 *  ~~~~~~~~~                                                               *
 ****************************************************************************/

#include <any>
#include <catch2/catch_test_macros.hpp>
#include <functional>
#include <set>
#include <string>
#include <thread>
import logic;
using namespace logic;
using namespace std::chrono_literals;
using namespace std::string_literals;

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

/****************************************************************************/

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

/****************************************************************************/

class TestAlarm : public AbstractAlarm {
public:
        void execute (std::any const &func) const override
        {
                auto f = std::any_cast<std::function<void ()>> (func);
                f ();
        }

        std::size_t hash () const override { return 0; }
        // std::unique_ptr<IAlarm> clone () const override { return std::make_unique<TestAlarm> (); }
};

/*--------------------------------------------------------------------------*/

TEST_CASE ("Alarm", "[eventQueue]")
{
        EventQueue events;
        std::string lastName;
        events.addCallback<UsbConnectedAlarm::Set> ([&lastName] (std::string const &name) { lastName = name + ".set"; });
        events.addCallback<UsbConnectedAlarm::Clear> ([&lastName] (std::string const &name) { lastName = name + ".clear"; });

        SECTION ("set - clear")
        {
                REQUIRE (lastName.empty ());
                events.setAlarm<UsbConnectedAlarm> ("deviceName"s); // Adding a new alarm genrates an event as well.
                events.run ();
                REQUIRE (lastName == "deviceName.set");
                lastName.clear ();

                events.setAlarm<UsbConnectedAlarm> ("deviceName"s); // Adding the same (same type and same parameter) alarm do nothing.
                events.run ();
                REQUIRE (lastName == ""); // lastName was cleared, so if the event was indeed not generated it should stay empty.

                events.clearAlarm<UsbConnectedAlarm> ("deviceName"s);
                events.run ();
                REQUIRE (lastName == "deviceName.clear");
                lastName.clear ();

                events.clearAlarm<UsbConnectedAlarm> ("deviceName"s);
                events.run ();
                REQUIRE (lastName == "");
        }

        SECTION ("hash")
        {
                events.setAlarm<UsbConnectedAlarm> ("11"s);
                events.setAlarm<UsbConnectedAlarm> ("22"s);
                events.setAlarm<UsbConnectedAlarm> ("11"s);
                events.setAlarm<UsbConnectedAlarm> ("22"s);
                int cnt{};

                events.visitAlarms<UsbConnectedAlarm> ([&cnt] (std::string const & /* name */) { ++cnt; });
                REQUIRE (cnt == 2);
        }

        SECTION ("hash type")
        {
                events.setAlarm<UsbConnectedAlarm> ("11"s);
                events.setAlarm<UsbConnectedAlarm> ("22"s);
                events.setAlarm<TestAlarm> ();
                events.setAlarm<UsbConnectedAlarm> ("11"s);
                events.setAlarm<UsbConnectedAlarm> ("22"s);
                events.setAlarm<TestAlarm> ();

                std::set<std::string> names;
                events.visitAlarms<UsbConnectedAlarm> ([&names] (std::string const &name) { names.insert (name); });
                REQUIRE (names == std::set<std::string>{"11", "22"});
        }
}
