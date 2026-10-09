#include "../include/sc.h"
#include <chrono>
#include <iomanip>
#include <sstream>

namespace sc {
    namespace base64_impl {
        class timer {
        public:
            timer() {
                startTime = std::chrono::steady_clock::now();
                stopped = false;
            }

            bool stopped = false;
            std::chrono::steady_clock::time_point startTime;
            std::chrono::nanoseconds taken{};
        };
    }

    timer::timer() {
        impl = new base64_impl::timer();
    }

    timer::~timer() {
        delete impl;
    }

    void timer::reset() {
        impl->startTime = std::chrono::steady_clock::now();
        impl->taken = std::chrono::nanoseconds::zero();
    }

    void timer::start() {
        if (!impl->stopped) return;
        impl->startTime = std::chrono::steady_clock::now();
        impl->stopped = false;
    }

    void timer::stop() {
        if (impl->stopped) return;
        auto stopTime = std::chrono::steady_clock::now();
        impl->stopped = true;
        impl->taken += stopTime - impl->startTime;
    }

    void timer::lap() {
        if (impl->stopped) {
            // Work like a real stopwatch button where lap/reset is the same button?
            // reset();
            return;
        }
        auto stopTime = std::chrono::steady_clock::now();
        impl->taken += stopTime - impl->startTime;
        impl->startTime = stopTime;
    }

    template<typename T>
    long long timer::getDuration() const {
        if (!impl->stopped) {
            auto currentTime = std::chrono::steady_clock::now();
            impl->taken += currentTime - impl->startTime;
            impl->startTime = currentTime;
        }
        return std::chrono::duration_cast<T>(impl->taken).count();
    }

    long long timer::nanos() const {
        return getDuration<std::chrono::nanoseconds>();
    }

    long long timer::micros() const {
        return getDuration<std::chrono::microseconds>();
    }

    long long timer::millis() const {
        return getDuration<std::chrono::milliseconds>();
    }

    long long timer::secs() const {
        return getDuration<std::chrono::seconds>();
    }

    long long timer::mins() const {
        return getDuration<std::chrono::minutes>();
    }

    long long timer::hours() const {
        return getDuration<std::chrono::hours>();
    }

    timer::operator std::string() {
        lap();
        // HH:MM:SS.nnnnnnnnn, with the hours running on past a day. Formatted by hand rather than
        // with std::format, which not every supported compiler has (Ubuntu 22.04's GCC 11), so
        // there is one way to get it everywhere.
        auto rest = impl->taken;
        const auto hours = std::chrono::duration_cast<std::chrono::hours>(rest);
        rest -= hours;
        const auto minutes = std::chrono::duration_cast<std::chrono::minutes>(rest);
        rest -= minutes;
        const auto seconds = std::chrono::duration_cast<std::chrono::seconds>(rest);
        rest -= seconds;
        std::ostringstream text;
        text << std::setfill('0')
                << std::setw(2) << hours.count() << ':'
                << std::setw(2) << minutes.count() << ':'
                << std::setw(2) << seconds.count() << '.'
                << std::setw(9) << rest.count();
        return text.str();
    }

    timer timer::from_nanos(long long ns) {
        timer temp;
        temp.impl->stopped = true;
        temp.impl->taken = std::chrono::nanoseconds(ns);
        return temp;
    }

    timer timer::from_micros(long long us) {
        timer temp;
        temp.impl->stopped = true;
        temp.impl->taken = std::chrono::microseconds(us);
        return temp;
    }

    timer timer::from_millis(long long ms) {
        timer temp;
        temp.impl->stopped = true;
        temp.impl->taken = std::chrono::milliseconds(ms);
        return temp;
    }

    std::ostream &operator<<(std::ostream &lhs, timer &rhs) { return lhs << static_cast<std::string>(rhs); }
} // sc
