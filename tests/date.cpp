#include <sc.h>

#include <cstring>
#include <ctime>
#include <string>

#include "sc_test.h"

using namespace std;

namespace {
    // Today, computed independently of sc::date so the comparison is worth something.
    string today(const int days_offset = 0, const bool first_of_month = false, const bool first_of_year = false) {
        time_t now = time(nullptr);
        tm parts = *localtime(&now);
        parts.tm_mday += days_offset;
        if (first_of_month || first_of_year) parts.tm_mday = 1;
        if (first_of_year) parts.tm_mon = 0;
        const time_t normalized = mktime(&parts);
        parts = *localtime(&normalized);
        char buffer[16];
        strftime(buffer, sizeof buffer, "%Y-%m-%d", &parts);
        return {buffer};
    }

    string months_back(const int count) {
        time_t now = time(nullptr);
        tm parts = *localtime(&now);
        parts.tm_mday = 1;
        parts.tm_mon -= count;
        const time_t normalized = mktime(&parts);
        parts = *localtime(&normalized);
        char buffer[16];
        strftime(buffer, sizeof buffer, "%Y-%m-%d", &parts);
        return {buffer};
    }

    string years_back(const int count) {
        time_t now = time(nullptr);
        tm parts = *localtime(&now);
        parts.tm_mday = 1;
        parts.tm_mon = 0;
        parts.tm_year -= count;
        const time_t normalized = mktime(&parts);
        parts = *localtime(&normalized);
        char buffer[16];
        strftime(buffer, sizeof buffer, "%Y-%m-%d", &parts);
        return {buffer};
    }

    bool looks_like_iso_date(const string &value) {
        if (value.size() != 10) return false;
        for (size_t i = 0; i < value.size(); ++i) {
            const bool separator = i == 4 || i == 7;
            if (separator && value[i] != '-') return false;
            if (!separator && !isdigit(static_cast<unsigned char>(value[i]))) return false;
        }
        return true;
    }
}

int main() {
    SECTION("Default construction is today");
    {
        const sc::date now;
        CHECK_EQ(static_cast<string>(now), today());
        CHECK_EQ(now.format(), today());
        CHECK_EQ(static_cast<string>(sc::date{"Today"}), today());
    }

    SECTION("Parsing");
    {
        const sc::date slashes{"15/3/1999"};
        CHECK_EQ(static_cast<string>(slashes), string{"1999-03-15"});
        CHECK_EQ(slashes.format("%d/%m/%Y"), string{"15/03/1999"});

        const sc::date dashes{"1977-05-08"};
        CHECK_EQ(static_cast<string>(dashes), string{"1977-05-08"});

        // Input with no separator at all falls back to today instead of throwing.
        CHECK_EQ(static_cast<string>(sc::date{"asd;nasdjl "}), today());
        // Unparseable input that does contain a separator still yields a printable date.
        const sc::date junk{"asd/1/asd"};
        CHECK_MSG(looks_like_iso_date(static_cast<string>(junk)), "got \"" + static_cast<string>(junk) + '"');
    }

    SECTION("Custom formats");
    {
        const sc::date sample{"1999-03-15"};
        CHECK_EQ(sample.format("%d of %m in --> %Y"), string{"15 of 03 in --> 1999"});
        CHECK_EQ(sample.format("%Y"), string{"1999"});
        CHECK_EQ(sample.format(), static_cast<string>(sample));
    }

    SECTION("Assignment from a string replaces the value");
    {
        sc::date sample{"1999-03-15"};
        sample = "1977-05-08";
        CHECK_EQ(static_cast<string>(sample), string{"1977-05-08"});
        sample = "9/11/2001";
        CHECK_EQ(static_cast<string>(sample), string{"2001-11-09"});
    }

    SECTION("Day arithmetic normalises across month and year ends");
    {
        sc::date sample{"1999-03-15"};
        sample += 1;
        CHECK_EQ(static_cast<string>(sample), string{"1999-03-16"});
        sample -= 1;
        CHECK_EQ(static_cast<string>(sample), string{"1999-03-15"});
        sample += 30;
        CHECK_EQ(static_cast<string>(sample), string{"1999-04-14"});
        sample -= 30;
        CHECK_EQ(static_cast<string>(sample), string{"1999-03-15"});

        sc::date month_end{"2021-01-31"};
        month_end += 1;
        CHECK_EQ(static_cast<string>(month_end), string{"2021-02-01"});

        sc::date year_end{"2020-12-31"};
        year_end += 1;
        CHECK_EQ(static_cast<string>(year_end), string{"2021-01-01"});

        sc::date leap{"2020-02-28"};
        leap += 1;
        CHECK_EQ(static_cast<string>(leap), string{"2020-02-29"});
        leap += 1;
        CHECK_EQ(static_cast<string>(leap), string{"2020-03-01"});

        sc::date common{"2021-02-28"};
        common += 1;
        CHECK_EQ(static_cast<string>(common), string{"2021-03-01"});
    }

    SECTION("Increment operators");
    {
        sc::date sample{"1999-03-15"};
        // Post-increment returns the old value and advances the original.
        CHECK_EQ(static_cast<string>(sample++), string{"1999-03-15"});
        CHECK_EQ(static_cast<string>(sample), string{"1999-03-16"});
        // Pre-increment returns the advanced value.
        CHECK_EQ(static_cast<string>(++sample), string{"1999-03-17"});
        CHECK_EQ(static_cast<string>(sample), string{"1999-03-17"});
    }

    SECTION("Julian day conversion");
    {
        const sc::date sample{"1999-03-15"};
        const long julian = sample.as_julian();
        CHECK_EQ(static_cast<long>(sample), julian);
        CHECK_EQ(static_cast<string>(sc::date::from_julian(julian)), string{"1999-03-15"});

        // One calendar day is exactly one Julian day.
        const sc::date next{"1999-03-16"};
        CHECK_EQ(next.as_julian() - julian, 1L);

        // 2000 was a leap year, so this span is 366 days.
        const sc::date a_year_later{"2000-03-15"};
        CHECK_EQ(a_year_later.as_julian() - julian, 366L);

        // 1999 was not.
        const sc::date start_1998{"1998-03-15"};
        CHECK_EQ(julian - start_1998.as_julian(), 365L);

        // Round trip a spread of dates, including across the epoch. Dates before the
        // local epoch have a negative time_t, which is where a truncating rather than
        // flooring division used to lose a day.
        for (const char *text: {"1900-01-01", "1930-06-15", "1969-12-30", "1969-12-31", "1970-01-01", "1970-01-02",
                                "1999-12-31", "2000-01-01", "2000-02-29", "2024-02-29", "2038-01-19"}) {
            const sc::date original{text};
            CHECK_EQ(static_cast<string>(sc::date::from_julian(original.as_julian())), string{text});
        }

        // Consecutive days stay consecutive on both sides of the epoch.
        CHECK_EQ(sc::date{"1970-01-01"}.as_julian() - sc::date{"1969-12-31"}.as_julian(), 1L);
        CHECK_EQ(sc::date{"1969-12-31"}.as_julian() - sc::date{"1969-12-30"}.as_julian(), 1L);
        // 1970 was not a leap year, so this span is 365 days.
        CHECK_EQ(sc::date{"1970-01-01"}.as_julian() - sc::date{"1969-01-01"}.as_julian(), 365L);
    }

    SECTION("Julian day numbers are absolute, not local");
    {
        // A date is a calendar day, so its Julian day number must not depend on the
        // machine's timezone. These are the standard reference values.
        CHECK_EQ(sc::date{"1970-01-01"}.as_julian(), 2440588L); // the Unix epoch
        CHECK_EQ(sc::date{"1858-11-17"}.as_julian(), 2400001L); // the Modified Julian Date epoch
        CHECK_EQ(sc::date{"2000-01-01"}.as_julian(), 2451545L); // J2000.0
        CHECK_EQ(sc::date{"1900-01-01"}.as_julian(), 2415021L);
        CHECK_EQ(sc::date{"1999-03-15"}.as_julian(), 2451253L);

        // Far outside the range a time_t based conversion could reach.
        for (const char *text: {"1600-02-29", "1752-09-14", "2400-12-31"}) {
            const sc::date original{text};
            CHECK_EQ(static_cast<string>(sc::date::from_julian(original.as_julian())), string{text});
        }
        CHECK_EQ(sc::date{"1600-02-29"}.as_julian(), 2305507L);
        CHECK_EQ(sc::date{"2400-12-31"}.as_julian(), 2598007L);
    }

    SECTION("Weekday and day of year survive normalisation");
    {
        CHECK_EQ(sc::date{"1970-01-01"}.format("%a %j"), string{"Thu 001"});
        CHECK_EQ(sc::date{"1900-01-01"}.format("%a %j"), string{"Mon 001"});
        CHECK_EQ(sc::date{"1600-02-29"}.format("%a %j"), string{"Tue 060"});
        CHECK_EQ(sc::date{"2400-12-31"}.format("%a %j"), string{"Sun 366"});
        // After arithmetic, not just after parsing.
        sc::date sample{"2021-02-28"};
        sample += 1;
        CHECK_EQ(sample.format("%a %j"), string{"Mon 060"});
    }

    SECTION("Daylight saving transitions do not move the day");
    {
        // Each of these is the local DST switch in some timezone, including the ones
        // that switch at midnight, where a date resolved through a time_t can slip.
        for (const char *text: {"2021-03-13", "2021-03-14", "2021-11-06", "2021-11-07", "2021-10-30",
                                "2021-10-31", "2020-10-03", "2020-10-04", "2019-02-16", "2019-02-17"}) {
            const sc::date original{text};
            CHECK_EQ(static_cast<string>(original), string{text});
            CHECK_EQ(static_cast<string>(sc::date::from_julian(original.as_julian())), string{text});

            sc::date stepped{text};
            stepped += 1;
            stepped -= 1;
            CHECK_EQ(static_cast<string>(stepped), string{text});
        }

        // Truncating from a month that may be in DST into one that is not.
        sc::date summer{"2021-09-15"};
        CHECK_EQ(static_cast<string>(summer.trunc("Y")), string{"2021-01-01"});
        sc::date winter{"2021-01-15"};
        CHECK_EQ(static_cast<string>(winter.add(6, "M")), string{"2021-07-15"});
    }

    SECTION("add / sub by day, month and year");
    {
        sc::date sample{"1977-05-08"};
        CHECK_EQ(static_cast<string>(sample.sub(2, "Y")), string{"1975-05-08"});
        CHECK_EQ(static_cast<string>(sample), string{"1975-05-08"}); // sub mutates in place
        CHECK_EQ(static_cast<string>(sample.add(2, "Y")), string{"1977-05-08"});
        CHECK_EQ(static_cast<string>(sample.sub(8, "D")), string{"1977-04-30"});
        CHECK_EQ(static_cast<string>(sample.add(8, "D")), string{"1977-05-08"});
        CHECK_EQ(static_cast<string>(sample.sub(6, "M")), string{"1976-11-08"});
        CHECK_EQ(static_cast<string>(sample.add(6, "M")), string{"1977-05-08"});
        // An unknown unit is treated as days.
        CHECK_EQ(static_cast<string>(sample.add(1, "banana")), string{"1977-05-09"});
    }

    SECTION("Month arithmetic sticks to the end of the month");
    {
        sc::date month_end{"2021-01-31"};
        CHECK_EQ(static_cast<string>(month_end.add(1, "M")), string{"2021-02-28"});
        sc::date leap_end{"2020-01-31"};
        CHECK_EQ(static_cast<string>(leap_end.add(1, "M")), string{"2020-02-29"});
        sc::date mid_month{"2021-01-15"};
        CHECK_EQ(static_cast<string>(mid_month.add(1, "M")), string{"2021-02-15"});
    }

    SECTION("trunc");
    {
        sc::date sample{"1977-05-08"};
        CHECK_EQ(static_cast<string>(sample.trunc("M")), string{"1977-05-01"});
        CHECK_EQ(static_cast<string>(sample), string{"1977-05-01"}); // trunc mutates in place
        sc::date other{"1977-05-08"};
        CHECK_EQ(static_cast<string>(other.trunc("Y")), string{"1977-01-01"});
        // An unknown unit leaves the date alone.
        sc::date untouched{"1977-05-08"};
        CHECK_EQ(static_cast<string>(untouched.trunc("Q")), string{"1977-05-08"});
    }

    SECTION("Relative factory helpers");
    {
        CHECK_EQ(static_cast<string>(sc::date::day(0)), today());
        CHECK_EQ(static_cast<string>(sc::date::day(1)), today(-1));
        CHECK_EQ(static_cast<string>(sc::date::day(-1)), today(1));
        CHECK_EQ(static_cast<string>(sc::date::day(365)), today(-365));
        CHECK_EQ(static_cast<string>(sc::date::day(365 * 4)), today(-365 * 4));

        CHECK_EQ(static_cast<string>(sc::date::month(0)), today(0, true));
        CHECK_EQ(static_cast<string>(sc::date::month(2)), months_back(2));
        CHECK_EQ(static_cast<string>(sc::date::month(14)), months_back(14));

        CHECK_EQ(static_cast<string>(sc::date::year(0)), today(0, false, true));
        CHECK_EQ(static_cast<string>(sc::date::year(2)), years_back(2));

        // day(n) is exactly n Julian days before today.
        CHECK_EQ(sc::date::day(0).as_julian() - sc::date::day(7).as_julian(), 7L);

        // Invariants that hold whatever today is, and whether or not it is in DST:
        // these are built from localtime(), so they used to carry today's tm_isdst
        // into a month that does not have it and lose a day.
        for (int back = 0; back < 26; ++back) {
            const auto month_start = static_cast<string>(sc::date::month(back));
            CHECK_MSG(month_start.substr(8) == "01", "month(" + to_string(back) + ") = " + month_start);
            const auto year_start = static_cast<string>(sc::date::year(back));
            CHECK_MSG(year_start.substr(5) == "01-01", "year(" + to_string(back) + ") = " + year_start);
        }
        // Consecutive months are one month apart, and never the same month twice.
        CHECK_NE(static_cast<string>(sc::date::month(0)), static_cast<string>(sc::date::month(1)));
        CHECK_EQ(static_cast<string>(sc::date::year(0)).substr(0, 4),
                 static_cast<string>(sc::date::day(0)).substr(0, 4));
    }

    SECTION("Stream output matches the string conversion");
    {
        const sc::date sample{"1999-03-15"};
        ostringstream stream;
        stream << sample;
        CHECK_EQ(stream.str(), static_cast<string>(sample));
    }

    TEST_SUMMARY();
}
