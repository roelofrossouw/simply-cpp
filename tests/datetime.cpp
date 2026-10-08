#include <sc.h>

#include <chrono>
#include <cstdlib>
#include <ctime>
#include <sstream>
#include <stdexcept>
#include <string>

#include "sc_test.h"

using namespace std;
using namespace std::chrono_literals;

namespace {
    // Local time in a fixed zone, so the checks don't depend on where the test runs.
    void use_timezone(const char *zone) {
        setenv("TZ", zone, 1);
        tzset();
    }
}

int main() {
    use_timezone("UTC0");

    SECTION("Unix time");
    {
        CHECK_EQ(sc::datetime::from_unix(0).format("%Y-%m-%d %H:%M:%S", true), string{"1970-01-01 00:00:00"});
        CHECK_EQ(sc::datetime::from_unix(1791462896).as_unix(), 1791462896LL);
        CHECK_EQ(sc::datetime::from_unix(-86400).format("%Y-%m-%d", true), string{"1969-12-31"});
    }

    SECTION("Now");
    {
        const auto now = static_cast<long long>(time(nullptr));
        CHECK(llabs(sc::datetime{}.as_unix() - now) <= 2);
        CHECK(llabs(sc::datetime::now().as_unix() - now) <= 2);
    }

    SECTION("Parsing");
    {
        CHECK_EQ(sc::datetime{"2026-10-08T12:34:56Z"}.as_unix(), 1791462896LL);
        CHECK_EQ(sc::datetime{"2026-10-08 12:34:56Z"}.as_unix(), 1791462896LL);
        CHECK_EQ(sc::datetime{"2026-10-08Z"}.format("%Y-%m-%d %H:%M:%S", true), string{"2026-10-08 00:00:00"});
        CHECK_THROWS_AS(sc::datetime{"nonsense"}, invalid_argument);
        CHECK_THROWS_AS(sc::datetime{"2026-10-08 12:34:56 and more"}, invalid_argument);
        CHECK_THROWS_AS(sc::datetime{""}, invalid_argument);
    }

    SECTION("Local time");
    {
        use_timezone("SAST-2"); // UTC+2, no daylight saving
        CHECK_EQ(sc::datetime::from_unix(0).format(), string{"1970-01-01 02:00:00"});
        CHECK_EQ(sc::datetime::from_unix(0).format("%H:%M", true), string{"00:00"});
        // Text without 'Z' is local time, and formats back the same.
        const sc::datetime local{"2026-10-08 12:34:56"};
        CHECK_EQ(local.format(), string{"2026-10-08 12:34:56"});
        CHECK_EQ(local.as_unix(), 1791462896LL - 2 * 3600);
        CHECK_EQ(static_cast<string>(sc::datetime{"2026-10-08"}), string{"2026-10-08 00:00:00"});
        ostringstream stream;
        stream << local;
        CHECK_EQ(stream.str(), string{"2026-10-08 12:34:56"});
        use_timezone("UTC0");
    }

    SECTION("Arithmetic and comparison");
    {
        const auto start = sc::datetime::from_unix(1000);
        auto later = start;
        later += 30s;
        CHECK_EQ(later.as_unix(), 1030LL);
        CHECK_EQ((start + 1h).as_unix(), 4600LL);
        CHECK_EQ((start - 1000s).as_unix(), 0LL);
        CHECK_EQ((later - start).count(), 30LL);
        CHECK_EQ(sc::datetime::from_unix(0).add(2, "D").format("%Y-%m-%d", true), string{"1970-01-03"});
        CHECK_EQ(sc::datetime::from_unix(0).add(90, "i").as_unix(), 5400LL);
        CHECK_EQ(sc::datetime::from_unix(0).add(3, "H").sub(1, "H").as_unix(), 7200LL);
        CHECK_EQ(sc::datetime::from_unix(10).sub(10).as_unix(), 0LL);
        CHECK_THROWS_AS(sc::datetime::from_unix(0).add(1, "M"), invalid_argument);
        CHECK(start < later);
        CHECK(later > start);
        CHECK(start == sc::datetime::from_unix(1000));
        CHECK(start != later);
    }

    TEST_SUMMARY();
}
