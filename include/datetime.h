#ifndef SC_DATETIME_H
#define SC_DATETIME_H
#include <chrono>
#include <compare>
#include <ostream>
#include <string>

namespace sc {
    // A point in time to the second, like sc::date is a calendar day. It's a plain value
    // (copyable, comparable) around a std::chrono::system_clock::time_point. Text is read and
    // written in local time unless it says otherwise: a trailing 'Z' when parsing, utc = true
    // when formatting.
    class datetime {
    public:
        using clock = std::chrono::system_clock;

        // "Now", or "YYYY-MM-DD HH:MM:SS", "YYYY-MM-DDTHH:MM:SS" (either with an optional trailing
        // 'Z' for UTC) or "YYYY-MM-DD" (midnight). Throws std::invalid_argument for anything else.
        datetime(const std::string &input = "Now");

        explicit datetime(clock::time_point time);

        // Seconds since 1970-01-01 00:00:00 UTC, as in a JWT's "exp" or a time_t.
        static datetime from_unix(long long seconds);

        static datetime now();

        [[nodiscard]] long long as_unix() const;

        [[nodiscard]] clock::time_point time_point() const { return time_; }

        // strftime() format, in local time or UTC.
        [[nodiscard]] std::string format(const std::string &format = "%Y-%m-%d %H:%M:%S", bool utc = false) const;

        operator std::string() const;

        datetime &operator+=(std::chrono::seconds duration);

        datetime &operator-=(std::chrono::seconds duration);

        friend datetime operator+(datetime lhs, const std::chrono::seconds rhs) { return lhs += rhs; }

        friend datetime operator-(datetime lhs, const std::chrono::seconds rhs) { return lhs -= rhs; }

        friend std::chrono::seconds operator-(const datetime &lhs, const datetime &rhs) {
            return std::chrono::duration_cast<std::chrono::seconds>(lhs.time_ - rhs.time_);
        }

        // Adds number seconds ("s"), minutes ("i"), hours ("H") or days ("D"); a day is 86400
        // seconds, so across a daylight saving change the local clock time moves by an hour.
        datetime &add(long long number, const std::string &type = "s");

        datetime &sub(long long number, const std::string &type = "s");

        friend auto operator<=>(const datetime &, const datetime &) = default;

        friend bool operator==(const datetime &, const datetime &) = default;

    private:
        clock::time_point time_;

        friend std::ostream &operator<<(std::ostream &lhs, const datetime &rhs);
    };
}

#endif //SC_DATETIME_H
