#include "date.h"

using namespace std;

namespace sc {
    namespace impl {
        class date {
        public:
            date(const string &dateInput) { initialize(dateInput); }

            date(const long julian_day) {
                const time_t temp = (julian_day - unix_epoch + 1) * seconds_per_day;
                storage = *localtime(&temp);
            }

            operator long() { return mktime(&storage) / seconds_per_day + unix_epoch; }

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
                storage.tm_hour = storage.tm_min = storage.tm_sec = 0;
            }

            static constexpr long seconds_per_day = 24 * 60 * 60;
            const long unix_epoch = 2440589;
        };
    }

    date::date(const string &dateInput) : impl(new impl::date(dateInput)) {
    }

    date::date(const long julian_day) : impl(new impl::date(julian_day)) {
    }

    date::date(const date &copy) : impl(new impl::date((long) copy)) {
    }

    date::operator string() const { return format("%Y-%m-%d"); }

    date::operator long() const {
        return *impl;
    }

    string date::format(const string &format) const {
        char buffer[format.length() * 4 + 30];
        strftime(buffer, sizeof(buffer), format.c_str(), &impl->storage);
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
        impl = new impl::date(str);
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

    void date::normalize() {
        auto temp = mktime(&impl->storage);
        impl->storage = *localtime(&temp);
    }

    std::ostream &operator<<(std::ostream &lhs, const date &rhs) { return lhs << (string) rhs; }
}
