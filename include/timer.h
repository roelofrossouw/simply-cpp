#pragma once

#include <iosfwd>
#include <string>

namespace sc {
    namespace base64_impl {
        class timer;
    }

    // A stopwatch on the steady clock, running from when it's made. stop() pauses it, start()
    // carries on, reset() zeroes it. The readings include the time so far while it runs; as text
    // (or <<) it is HH:MM:SS.nnnnnnnnn. from_nanos() and the like make a stopped one of that length.
    class timer {
    public:
        timer();

        // A copy has its own state, starting from this timer's: copying is a snapshot.
        timer(const timer &other);

        timer &operator=(const timer &other);

        ~timer();

        void reset();

        void start();

        void stop();

        void lap();

        long long nanos() const;

        long long micros() const;

        long long millis() const;

        long long secs() const;

        long long mins() const;

        long long hours() const;

        operator std::string();

        static timer from_nanos(long long ns);

        static timer from_micros(long long us);

        static timer from_millis(long long ms);

    private:
        base64_impl::timer *impl;

        template<typename T>
        long long getDuration() const;

        friend std::ostream &operator<<(std::ostream &lhs, timer &rhs);
    };
}

