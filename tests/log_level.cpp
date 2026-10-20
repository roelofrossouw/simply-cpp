// sc::log_level: syslog's order, and numbers from a configuration file.
#include <log_level.h>

#include "sc_test.h"

int main() {
    SECTION("Ordered as syslog's levels: a message is written when its level is at or below the one set");
    {
        CHECK(sc::log_level::error < sc::log_level::warning);
        CHECK(sc::log_level::emergency < sc::log_level::debug);
        CHECK_EQ(static_cast<int>(sc::log_level::warning), 4);
    }

    SECTION("From a number, out-of-range ones taken as the nearest");
    {
        CHECK(sc::log_level_from(3) == sc::log_level::error);
        CHECK(sc::log_level_from(7) == sc::log_level::debug);
        CHECK(sc::log_level_from(-1) == sc::log_level::emergency);
        CHECK(sc::log_level_from(9) == sc::log_level::debug);
        static_assert(sc::log_level_from(4) == sc::log_level::warning); // usable at compile time
    }

    TEST_SUMMARY();
}
