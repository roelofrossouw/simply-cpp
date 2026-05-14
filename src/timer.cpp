#include "sc.h"
#include <chrono>

namespace sc {
    namespace impl {
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
        impl = new impl::timer();
    }

    timer::~timer() {
        delete impl;
    }

    void timer::reset() {
        impl->startTime = std::chrono::steady_clock::now();
        impl->taken.zero();
    }

    void timer::start() {
        if (!impl->stopped) return;
        impl->startTime = std::chrono::steady_clock::now();
        impl->stopped = false;
    }

    void timer::stop() {
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

    template <typename T>
    long long timer::getDuration() const {
        auto currentTime = std::chrono::steady_clock::now();
        impl->taken += currentTime - impl->startTime;
        impl->startTime = currentTime;
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

    std::ostream &operator<<(std::ostream &lhs, timer &rhs) {
        rhs.lap();
#if defined(__cpp_lib_format)
        lhs << std::format("{:%H:%M:%S}", rhs.impl->taken);
#else
        auto ms = rhs.impl->taken;
        auto h = std::chrono::duration_cast<std::chrono::hours>(ms);
        ms -= h;
        auto m = std::chrono::duration_cast<std::chrono::minutes>(ms);
        ms -= m;
        auto s = std::chrono::duration_cast<std::chrono::seconds>(ms);
        ms -= s;
        auto millis = std::chrono::duration_cast<std::chrono::microseconds>(ms);

        lhs << std::setfill('0')
                << std::setw(2) << h.count() << ":"
                << std::setw(2) << m.count() << ":"
                << std::setw(2) << s.count() << "."
                << std::setw(3) << millis.count();
#endif
        return lhs;
    }
} // sc
