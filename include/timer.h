#pragma once


class ostream;

namespace sc {
    namespace base64_impl {
        class timer;
    }

    class timer {
    public:
        timer();

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

