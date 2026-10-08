#include "datetime.h"

#include <ctime>
#include <stdexcept>

using namespace std;

namespace sc {
    namespace {
        // strptime() with the whole input consumed, so trailing junk isn't silently ignored.
        bool parse(const string &input, const char *format, tm &parts) {
            parts = tm{};
            const char *end = strptime(input.c_str(), format, &parts);
            return end && *end == '\0';
        }
    }

    datetime::datetime(const string &input) {
        if (input == "Now") {
            time_ = time_point_cast<chrono::seconds>(clock::now());
            return;
        }

        const bool utc = !input.empty() && input.back() == 'Z';
        const string text = utc ? input.substr(0, input.size() - 1) : input;
        tm parts{};
        if (!parse(text, "%Y-%m-%d %H:%M:%S", parts) && !parse(text, "%Y-%m-%dT%H:%M:%S", parts) &&
            !parse(text, "%Y-%m-%d", parts)) {
            throw invalid_argument("invalid datetime, expected YYYY-MM-DD[ HH:MM:SS]: " + input);
        }
        parts.tm_isdst = -1; // let mktime() work out daylight saving for the local time
        const time_t seconds = utc ? timegm(&parts) : mktime(&parts);
        time_ = clock::from_time_t(seconds);
    }

    datetime::datetime(const clock::time_point time) : time_(time) {
    }

    datetime datetime::from_unix(const long long seconds) {
        return datetime{clock::time_point{chrono::seconds{seconds}}};
    }

    datetime datetime::now() { return datetime{}; }

    long long datetime::as_unix() const {
        return chrono::duration_cast<chrono::seconds>(time_.time_since_epoch()).count();
    }

    string datetime::format(const string &format, const bool utc) const {
        const time_t seconds = clock::to_time_t(time_);
        tm parts{};
        if (utc) gmtime_r(&seconds, &parts);
        else localtime_r(&seconds, &parts);
        constexpr int max_length = 128;
        char buffer[max_length];
        const size_t length = strftime(buffer, max_length, format.c_str(), &parts);
        return {buffer, length};
    }

    datetime::operator string() const { return format(); }

    datetime &datetime::operator+=(const chrono::seconds duration) {
        time_ += duration;
        return *this;
    }

    datetime &datetime::operator-=(const chrono::seconds duration) {
        time_ -= duration;
        return *this;
    }

    datetime &datetime::add(const long long number, const string &type) {
        if (type == "s") return *this += chrono::seconds{number};
        if (type == "i") return *this += chrono::minutes{number};
        if (type == "H") return *this += chrono::hours{number};
        if (type == "D") return *this += chrono::hours{24 * number};
        throw invalid_argument("invalid datetime unit, expected s, i, H or D: " + type);
    }

    datetime &datetime::sub(const long long number, const string &type) { return add(-number, type); }

    std::ostream &operator<<(std::ostream &lhs, const datetime &rhs) { return lhs << rhs.format(); }
}
