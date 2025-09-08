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

class TestConnectedAlarm : public AbstractAlarm<TestConnectedAlarm> {
public:
        explicit TestConnectedAlarm (std::string const &name) : name{name} {}

        void execute (std::any const &func) const override
        {
                auto f = std::any_cast<std::function<void (std::string const &name)>> (func);
                f (name);
        }

        std::size_t hash () const override { return std::hash<std::string>{}(name); }

private:
        std::string name;
};

/*--------------------------------------------------------------------------*/

TEST_CASE ("Event queue", "[eventQueue]")
{
        EventQueue events;
        std::string lastName;

        events.addCallback<ErrorEvent> ([&lastName] (std::string const &name) { lastName = name; });
        events.addEvent<ErrorEvent> (std::string{"deviceName"});

        REQUIRE (lastName.empty ());
        events.run ();
        REQUIRE (lastName == "deviceName");
}

/****************************************************************************/

TEST_CASE ("Event queue threads", "[eventQueue]")
{
        EventQueue events;
        std::vector<std::string> names;

        events.addCallback<ErrorEvent> ([&names] (std::string const &name) { names.push_back (name); });

        std::thread t{[&events] {
                for (int i = 0; i < 10; ++i) {
                        events.addEvent<ErrorEvent> (std::to_string (i));
                        std::this_thread::sleep_for (10ms);
                }
        }};

        std::thread t2{[&events] {
                for (int i = 10; i < 20; ++i) {
                        events.addEvent<ErrorEvent> (std::to_string (i));
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

class TestAlarm : public AbstractAlarm<TestAlarm> {
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
        events.addCallback<TestConnectedAlarm::Set> ([&lastName] (std::string const &name) { lastName = name + ".set"; });
        events.addCallback<TestConnectedAlarm::Clear> ([&lastName] (std::string const &name) { lastName = name + ".clear"; });

        SECTION ("set - clear")
        {
                REQUIRE (lastName.empty ());
                events.setAlarm<TestConnectedAlarm> ("deviceName"s); // Adding a new alarm genrates an event as well.
                events.run ();
                REQUIRE (lastName == "deviceName.set");
                lastName.clear ();

                events.setAlarm<TestConnectedAlarm> ("deviceName"s); // Adding the same (same type and same parameter) alarm do nothing.
                events.run ();
                REQUIRE (lastName == ""); // lastName was cleared, so if the event was indeed not generated it should stay empty.

                events.clearAlarm<TestConnectedAlarm> ("deviceName"s);
                events.run ();
                REQUIRE (lastName == "deviceName.clear");
                lastName.clear ();

                events.clearAlarm<TestConnectedAlarm> ("deviceName"s);
                events.run ();
                REQUIRE (lastName == "");
        }

        SECTION ("hash")
        {
                events.setAlarm<TestConnectedAlarm> ("11"s);
                events.setAlarm<TestConnectedAlarm> ("22"s);
                events.setAlarm<TestConnectedAlarm> ("11"s);
                events.setAlarm<TestConnectedAlarm> ("22"s);
                int cnt{};

                events.visitAlarms<TestConnectedAlarm> ([&cnt] (std::string const & /* name */) { ++cnt; });
                REQUIRE (cnt == 2);
        }

        SECTION ("hash type")
        {
                events.setAlarm<TestConnectedAlarm> ("11"s);
                events.setAlarm<TestConnectedAlarm> ("22"s);
                events.setAlarm<TestAlarm> ();
                events.setAlarm<TestConnectedAlarm> ("11"s);
                events.setAlarm<TestConnectedAlarm> ("22"s);
                events.setAlarm<TestAlarm> ();

                std::set<std::string> names;
                events.visitAlarms<TestConnectedAlarm> ([&names] (std::string const &name) { names.insert (name); });
                REQUIRE (names == std::set<std::string>{"11", "22"});
        }
}
