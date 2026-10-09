// Dates and times in simply-cpp core: calendar days with sc::date, points in time with
// sc::datetime, and durations with sc::timer. Each line shows a call, as written, and what it
// returned. Needs no server.

#include <chrono>
#include <string>

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: dates and times");

    sc::console::heading("Calendar days: sc::date");
    SC_SHOW(sc::date{"2024-02-28"}.add(1));         // a leap year
    SC_SHOW(sc::date{"2026-01-31"}.add(1, "M"));    // a month on
    SC_SHOW(sc::date{"2026-10-09"}.sub(2, "Y"));    // two years back
    SC_SHOW(sc::date{"2026-10-09"}.trunc("M"));     // the first of its month
    SC_SHOW(sc::date{"2026-10-09"}.trunc("Y"));     // the first of its year
    SC_SHOW(sc::date{"2026-10-09"}.format("%A %d %B %Y"));
    SC_SHOW(sc::date{"2026-12-25"}.as_julian() - sc::date{"2026-10-09"}.as_julian()); // days between

    sc::console::subheading("Relative to today");
    SC_SHOW(sc::date{});
    SC_SHOW(sc::date::month(1)); // the first of last month
    SC_SHOW(sc::date::year());   // the first of this year

    sc::console::heading("Points in time: sc::datetime");
    sc::console::note("Text is read and written in local time, unless it ends in Z or utc is asked for.");
    const sc::datetime meeting{"2026-10-09 14:30:00"};
    SC_SHOW(meeting);
    SC_SHOW(meeting + std::chrono::minutes{90});
    SC_SHOW(sc::datetime{meeting}.add(2, "D").format("%a %d %b %H:%M"));
    SC_SHOW((sc::datetime{"2026-10-10 09:00:00"} - meeting).count()); // seconds between
    SC_SHOW(meeting < meeting + std::chrono::seconds{1});

    sc::console::subheading("Unix time and UTC");
    SC_SHOW(sc::datetime{"2026-10-09T12:30:00Z"}.as_unix());
    SC_SHOW(sc::datetime::from_unix(1'800'000'000).format("%Y-%m-%d %H:%M:%S UTC", true));

    sc::console::heading("Durations: sc::timer");
    SC_SHOW(std::string(sc::timer::from_millis(90'061'001)));
    SC_SHOW(sc::timer::from_millis(90'061'001).hours());
    SC_SHOW(sc::timer::from_micros(2'500).millis());

    sc::console::subheading("Timing some work");
    const auto harmonic = [](const int terms) { // 1 + 1/2 + 1/3 + ... + 1/terms
        double total = 0;
        for (int i = 1; i <= terms; ++i) total += 1.0 / i;
        return total;
    };
    sc::timer stopwatch;
    SC_SHOW(harmonic(1'000'000));
    SC_STEP(stopwatch.stop());
    sc::console::note("that took " + std::string(stopwatch));
    return 0;
}
