#include "sc.h"
#include <chrono>

namespace sc {
    namespace impl {
        class timer {
        public:
            timer() {
                since = std::chrono::steady_clock::now();
                stopped = false;
            }

            bool stopped = false;
            std::chrono::steady_clock::time_point since;
            std::chrono::steady_clock::time_point until;
            std::chrono::milliseconds taken{};
        };
    }

    timer::timer() {
        impl = new impl::timer();
    }

    timer::~timer() {
        delete impl;
    }

    void timer::reset() {
        impl->since = std::chrono::steady_clock::now();
        impl->taken.zero();
    }

    void timer::start() {
        if (!impl->stopped) return;
        impl->since = std::chrono::steady_clock::now();
        impl->stopped = false;
    }

    void timer::stop() {
        impl->until = std::chrono::steady_clock::now();
        impl->stopped = true;
        impl->taken += std::chrono::duration_cast<std::chrono::milliseconds>(impl->until - impl->since);
    }

    void timer::lap() {
        if (impl->stopped) {
            // Work like a real stopwatch button where lap/reset is the same button?
            // reset();
            return;
        }
        impl->until = std::chrono::steady_clock::now();
        impl->taken += std::chrono::duration_cast<std::chrono::milliseconds>(impl->until - impl->since);
        impl->since = impl->until;
    }


    std::ostream &operator<<(std::ostream &lhs, timer &rhs) {
        rhs.lap();
        lhs << rhs.impl->taken.count() << "ms";
        return lhs;
    }
} // sc
