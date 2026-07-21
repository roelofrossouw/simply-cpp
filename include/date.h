#ifndef SC_DATE_H
#define SC_DATE_H
#include <string>

namespace sc {
    namespace impl {
        class date;
    }

    class date {
    public:
        date(const std::string &dateInput = "Today");

        operator std::string() const;

        operator long() const;

        std::string format(const std::string &format = "%Y-%m-%d") const;

        date &operator+=(int i);

        date &operator-=(int i);

        date &operator=(const std::string &str);

        date operator++(int);

        date &operator++();

        static date from_julian(long julian_day);

        long as_julian() const;

        date trunc(const std::string &type = "M");

        date add(int days, const std::string &type = "D");

        date sub(int days, const std::string &type = "D");

        static date month(int monthsBack = 0);

        static date day(int daysBack = 0);

        static date year(int yearsBack = 0);

    private:
        date(long julian_day);

        date(const date &copy);

        impl::date *impl;

        void normalize();

        friend std::ostream &operator<<(std::ostream &lhs, const date &rhs);
    };
}

#endif //SC_DATE_H
