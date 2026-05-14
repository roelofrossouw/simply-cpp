#ifndef SC_TIMER_TIMER_H
#define SC_TIMER_TIMER_H

class ostream;

namespace sc {
    namespace impl {
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

    private:
        impl::timer *impl;

        template <typename T>
        long long getDuration() const;

        friend std::ostream &operator<<(std::ostream &lhs, timer &rhs);
    };
}


#endif //SC_TIMER_TIMER_H
