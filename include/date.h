#ifndef SC_DATE_H
#define SC_DATE_H
#include <memory>
#include <string>
#include <ostream>

namespace sc {
    namespace impl {
        class date;
    }

    // A calendar day, with no time of day or timezone (sc::datetime is a point in time).
    class date {
    public:
        // "Today", "YYYY-MM-DD" or "DD/MM/YYYY"; text with neither '-' nor '/' is today.
        date(const std::string &dateInput = "Today");

        // Copies are independent: changing one doesn't change the other. There are no separate
        // move operations, so a moved-from date stays a valid date.
        date(const date &copy);

        date &operator=(const date &copy);

        ~date();

        operator std::string() const;

        // The Julian day number, as as_julian(): subtracting two gives the days between them.
        operator long() const;

        // strftime() format.
        std::string format(const std::string &format = "%Y-%m-%d") const;

        date &operator+=(int i);

        date &operator-=(int i);

        date &operator=(const std::string &str);

        date operator++(int);

        date &operator++();

        static date from_julian(long julian_day);

        long as_julian() const;

        // These change this date and return a copy of it. trunc() goes to the first of its month
        // ("M") or year ("Y"); add() and sub() move by days ("D"), months ("M") or years ("Y"). A
        // month from the last day of one is the last day of the next: 2026-01-31 gives 2026-02-28.
        date trunc(const std::string &type = "M");

        date add(int days, const std::string &type = "D");

        date sub(int days, const std::string &type = "D");

        // The first of the month monthsBack months ago, the day daysBack days ago, and the first of
        // the year yearsBack years ago.
        static date month(int monthsBack = 0);

        static date day(int daysBack = 0);

        static date year(int yearsBack = 0);

    private:
        date(long julian_day);

        std::unique_ptr<impl::date> impl;

        void normalize();

        friend std::ostream &operator<<(std::ostream &lhs, const date &rhs);
    };
}

#endif //SC_DATE_H
