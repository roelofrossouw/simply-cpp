#include <sc.h>

#include <chrono>
#include <string>
#include <thread>

#include "sc_test.h"

using namespace std;
using namespace std::chrono_literals;

namespace {
    // Upper bounds are deliberately loose: a loaded machine can oversleep badly.
    constexpr long long slack_ms = 5000;

    void sleep_ms(const int milliseconds) { this_thread::sleep_for(chrono::milliseconds(milliseconds)); }
}

int main() {
    SECTION("A new timer starts running");
    {
        sc::timer t;
        sleep_ms(120);
        const long long first = t.millis();
        CHECK_BETWEEN(first, 100LL, slack_ms);
        sleep_ms(50);
        // Still running, so the reading must have grown.
        CHECK_LT(first, t.millis());
    }

    SECTION("stop() freezes the reading");
    {
        sc::timer t;
        sleep_ms(120);
        t.stop();
        const long long frozen = t.nanos();
        CHECK_BETWEEN(t.millis(), 100LL, slack_ms);
        sleep_ms(80);
        CHECK_EQ(t.nanos(), frozen);
        CHECK_EQ(t.millis(), frozen / 1000000);
        // Stopping twice is a no-op rather than an error.
        t.stop();
        CHECK_EQ(t.nanos(), frozen);
    }

    SECTION("start() resumes and keeps the time already accumulated");
    {
        sc::timer t;
        sleep_ms(100);
        t.stop();
        const long long after_first = t.millis();
        CHECK_BETWEEN(after_first, 90LL, slack_ms);
        sleep_ms(100); // not counted, the timer is stopped
        t.start();
        sleep_ms(100);
        t.stop();
        const long long total = t.millis();
        CHECK_BETWEEN(total, after_first + 90, after_first + slack_ms);
        // start() on an already running timer must not restart the clock.
        t.start();
        t.start();
        CHECK_BETWEEN(t.millis(), total, total + slack_ms);
    }

    SECTION("reset() clears the accumulated time but leaves a stopped timer stopped");
    {
        sc::timer t;
        sleep_ms(80);
        t.stop();
        CHECK_BETWEEN(t.millis(), 70LL, slack_ms);
        t.reset();
        CHECK_EQ(t.nanos(), 0LL);
        sleep_ms(50);
        CHECK_EQ(t.nanos(), 0LL); // reset does not restart a stopped timer
        t.start();
        sleep_ms(100);
        t.stop();
        CHECK_BETWEEN(t.millis(), 90LL, slack_ms);
    }

    SECTION("lap() banks the elapsed time without stopping");
    {
        sc::timer t;
        sleep_ms(100);
        t.lap();
        const long long first_lap = t.millis();
        CHECK_BETWEEN(first_lap, 90LL, slack_ms);
        sleep_ms(100);
        CHECK_LT(first_lap, t.millis()); // still running
    }

    SECTION("Unit conversions are consistent");
    {
        sc::timer t = sc::timer::from_millis(1500);
        CHECK_EQ(t.nanos(), 1500000000LL);
        CHECK_EQ(t.micros(), 1500000LL);
        CHECK_EQ(t.millis(), 1500LL);
        CHECK_EQ(t.secs(), 1LL); // truncated, not rounded
        CHECK_EQ(t.mins(), 0LL);
        CHECK_EQ(t.hours(), 0LL);

        sc::timer micros = sc::timer::from_micros(1500);
        CHECK_EQ(micros.nanos(), 1500000LL);
        CHECK_EQ(micros.micros(), 1500LL);
        CHECK_EQ(micros.millis(), 1LL);

        sc::timer nanos = sc::timer::from_nanos(1999);
        CHECK_EQ(nanos.nanos(), 1999LL);
        CHECK_EQ(nanos.micros(), 1LL);
        CHECK_EQ(nanos.millis(), 0LL);

        sc::timer zero = sc::timer::from_nanos(0);
        CHECK_EQ(zero.nanos(), 0LL);
        CHECK_EQ(zero.secs(), 0LL);

        // 1h 1m 1s
        sc::timer long_run = sc::timer::from_millis(3661000);
        CHECK_EQ(long_run.secs(), 3661LL);
        CHECK_EQ(long_run.mins(), 61LL);
        CHECK_EQ(long_run.hours(), 1LL);
    }

    SECTION("Constructed timers are stopped, so their reading is stable");
    {
        sc::timer t = sc::timer::from_millis(250);
        sleep_ms(60);
        CHECK_EQ(t.millis(), 250LL);
    }

    SECTION("String and stream output");
    {
        sc::timer t = sc::timer::from_millis(3661000);
        const auto text = static_cast<string>(t);
        CHECK_EQ(text.substr(0, 8), string{"01:01:01"});
        CHECK_MSG(text.size() > 8 && text[8] == '.', "expected a fractional part, got \"" + text + '"');
        // The reading survives being formatted.
        CHECK_EQ(t.millis(), 3661000LL);

        sc::timer zero = sc::timer::from_nanos(0);
        CHECK_EQ(static_cast<string>(zero).substr(0, 8), string{"00:00:00"});

        sc::timer streamed = sc::timer::from_millis(3661000);
        ostringstream stream;
        stream << streamed;
        CHECK_EQ(stream.str(), static_cast<string>(streamed));
    }

    TEST_SUMMARY();
}
