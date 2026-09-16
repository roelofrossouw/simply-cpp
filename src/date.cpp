#include "date.h"
#include <ctime>

using namespace std;

namespace sc {
    namespace base64_impl {
        // A date here is a calendar day, not an instant, so the Julian day conversion
        // works on the civil fields directly. Going via a time_t made the result depend
        // on the local UTC offset, which put whole timezones a day out.
        class date {
        public:
            date(const string &dateInput) { initialize(dateInput); }

            date(const long julian_day) {
                int year;
                unsigned month, day;
                civil_from_days(julian_day - julian_epoch, year, month, day);
                storage = tm{};
                storage.tm_year = year - 1900;
                storage.tm_mon = static_cast<int>(month) - 1;
                storage.tm_mday = static_cast<int>(day);
                normalize();
            }

            operator long() {
                return days_from_civil(storage.tm_year + 1900, storage.tm_mon + 1, storage.tm_mday) + julian_epoch;
            }

            // Carries any out of range field (day 32, month 13, day 0) into the next one
            // and re-derives tm_wday/tm_yday. mktime() used to do this, but it resolved
            // the fields against the local timezone: a date built from today carried
            // today's tm_isdst, so truncating into a non-DST month moved the day back an
            // hour and, at midnight, off the day entirely. Civil arithmetic has no such
            // trap, and no time_t range limit either.
            void normalize() {
                // Months first, so the year is settled before the day count is taken.
                const long months = static_cast<long>(storage.tm_year) * 12 + storage.tm_mon;
                const long year = (months >= 0 ? months : months - 11) / 12;
                const auto month = static_cast<unsigned>(months - year * 12); // [0, 11]

                const long days = days_from_civil(year + 1900, month + 1, 1) + storage.tm_mday - 1;

                int normalized_year;
                unsigned normalized_month, normalized_day;
                civil_from_days(days, normalized_year, normalized_month, normalized_day);

                storage.tm_year = normalized_year - 1900;
                storage.tm_mon = static_cast<int>(normalized_month) - 1;
                storage.tm_mday = static_cast<int>(normalized_day);
                storage.tm_wday = static_cast<int>((days % 7 + 11) % 7); // 1970-01-01 was a Thursday
                storage.tm_yday = static_cast<int>(days - days_from_civil(normalized_year, 1, 1));
                storage.tm_hour = storage.tm_min = storage.tm_sec = 0;
                storage.tm_isdst = -1; // a calendar day has no daylight saving state
            }

            tm storage{};

        private:
            void initialize(const string &dateInput) {
                if (dateInput == "Today") {
                    time_t now = time(nullptr);
                    storage = *localtime(&now);
                    // TODO: Process common words, e.g. tomorrow, etc.
                } else if (dateInput.find('/') != std::string::npos) {
                    strptime(dateInput.c_str(), "%d/%m/%Y", &storage);
                } else if (dateInput.find('-') != std::string::npos) {
                    strptime(dateInput.c_str(), "%Y-%m-%d", &storage);
                } else {
                    // throw invalid_argument("Invalid date format");
                    time_t now = time(nullptr);
                    storage = *localtime(&now);
                }
                normalize();
            }

            // Days between 1970-01-01 and y-m-d, proleptic Gregorian, month in [1, 12].
            // Howard Hinnant's chrono-compatible civil calendar algorithms.
            static long days_from_civil(long y, const unsigned m, const unsigned d) {
                y -= m <= 2;
                const long era = (y >= 0 ? y : y - 399) / 400;
                const auto yoe = static_cast<unsigned>(y - era * 400);               // [0, 399]
                const unsigned doy = (153 * (m + (m > 2 ? -3 : 9)) + 2) / 5 + d - 1; // [0, 365]
                const unsigned doe = yoe * 365 + yoe / 4 - yoe / 100 + doy;          // [0, 146096]
                return era * 146097 + static_cast<long>(doe) - 719468;
            }

            static void civil_from_days(long z, int &y, unsigned &m, unsigned &d) {
                z += 719468;
                const long era = (z >= 0 ? z : z - 146096) / 146097;
                const auto doe = static_cast<unsigned>(z - era * 146097);                  // [0, 146096]
                const unsigned yoe = (doe - doe / 1460 + doe / 36524 - doe / 146096) / 365; // [0, 399]
                const unsigned doy = doe - (365 * yoe + yoe / 4 - yoe / 100);               // [0, 365]
                const unsigned mp = (5 * doy + 2) / 153;                                    // [0, 11]
                d = doy - (153 * mp + 2) / 5 + 1;                                           // [1, 31]
                m = mp + (mp < 10 ? 3 : -9);                                                // [1, 12]
                y = static_cast<int>(static_cast<long>(yoe) + era * 400 + (m <= 2));
            }

            // Julian day number of 1970-01-01.
            static constexpr long julian_epoch = 2440588;
        };
    }

    date::date(const string &dateInput) : impl(new base64_impl::date(dateInput)) {
    }

    date::date(const long julian_day) : impl(new base64_impl::date(julian_day)) {
    }

    date::date(const date &copy) : impl(new base64_impl::date((long) copy)) {
    }

    date::operator string() const { return format("%Y-%m-%d"); }

    date::operator long() const {
        return *impl;
    }

    string date::format(const string &format) const {
        constexpr int max_length = 96;
        char buffer[max_length];
        strftime(buffer, max_length, format.c_str(), &impl->storage);
        return {buffer};
    }

    date &date::operator+=(const int i) {
        impl->storage.tm_mday += i;
        normalize();
        return *this;
    }

    date &date::operator-=(const int i) { return (*this) += (-i); }

    date &date::operator=(const string &str) {
        delete impl;
        impl = new base64_impl::date(str);
        return *this;
    }

    date date::operator++(int) {
        date copy(*this);
        *this += 1;
        return copy;
    }

    date &date::operator++() { return *this += 1; }

    date date::from_julian(long julian_day) { return {julian_day}; }

    long date::as_julian() const { return *this; }

    date date::day(int daysBack) {
        date new_date;
        return new_date += -daysBack;
    }

    date date::month(int monthsBack) {
        date new_date;
        new_date.impl->storage.tm_mday = 1;
        new_date.impl->storage.tm_mon -= monthsBack;
        new_date.normalize();
        return new_date;
    }

    date date::year(int yearsBack) {
        date new_date;
        new_date.impl->storage.tm_mday = 1;
        new_date.impl->storage.tm_mon = 0;
        new_date.impl->storage.tm_year -= yearsBack;
        new_date.normalize();
        return new_date;
    }

    date date::trunc(const string &type) {
        if (type == "M") {
            impl->storage.tm_mday = 1;
        } else if (type == "Y") {
            impl->storage.tm_mday = 1;
            impl->storage.tm_mon = 0;
        }
        normalize();
        return *this;
    }

    date date::add(int number, const string &type) {
        if (type == "Y") impl->storage.tm_year += number;
        else if (type == "M") {
            // Add 1 and Sub 1 to stay on the last day of the month.
            add(1);
            impl->storage.tm_mon += number;
            sub(1);
        } else impl->storage.tm_mday += number;
        normalize();
        return *this;
    }

    date date::sub(int number, const string &type) { return add(-number, type); }

    void date::normalize() { impl->normalize(); }

    std::ostream &operator<<(std::ostream &lhs, const date &rhs) { return lhs << (string) rhs; }
}
