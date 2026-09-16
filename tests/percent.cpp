#include <sc.h>

#include <string>

#include "sc_test.h"

using namespace std;

int main() {
    SECTION("Construction, rounding and formatting");
    {
        sc::percent value{0.40524};
        CHECK_EQ(static_cast<double>(value), 40.52);
        CHECK_EQ(static_cast<string>(value), string{"40.52%"});

        // Changing the precision re-rounds the stored value, it does not truncate a string.
        value.decimals(1);
        CHECK_EQ(static_cast<double>(value), 40.5);
        CHECK_EQ(static_cast<string>(value), string{"40.5%"});

        value.decimals(0);
        CHECK_EQ(static_cast<double>(value), 41.0);
        CHECK_EQ(static_cast<string>(value), string{"41%"});

        // Precision is restored when set back.
        value.decimals(3);
        CHECK_EQ(static_cast<double>(value), 40.524);
        CHECK_EQ(static_cast<string>(value), string{"40.524%"});
    }

    SECTION("Precision given at construction");
    {
        const sc::percent precise{0.40524, 3};
        CHECK_EQ(static_cast<double>(precise), 40.524);
        CHECK_EQ(static_cast<string>(precise), string{"40.524%"});

        const sc::percent coarse{0.40524, 0};
        CHECK_EQ(static_cast<double>(coarse), 41.0);
        CHECK_EQ(static_cast<string>(coarse), string{"41%"});

        // A negative precision is clamped to zero rather than being an error.
        const sc::percent nonsense{0.40524, -3};
        CHECK_EQ(static_cast<double>(nonsense), 41.0);
        CHECK_EQ(static_cast<string>(nonsense), string{"41%"});
    }

    SECTION("Defaults");
    {
        const sc::percent zero;
        CHECK_EQ(static_cast<double>(zero), 0.0);
        CHECK_EQ(static_cast<string>(zero), string{"0.00%"});
    }

    SECTION("Values above 1 are read as whole percentages");
    {
        CHECK_EQ(static_cast<double>(sc::percent{10}), 10.0);
        CHECK_EQ(static_cast<double>(sc::percent{0.1}), 10.0);
        CHECK_EQ(static_cast<double>(sc::percent{2}), 2.0);
        CHECK_EQ(static_cast<double>(sc::percent{99.5}), 99.5);
        // 1 is the boundary: it is a fraction, so it reads as 100%.
        CHECK_EQ(static_cast<double>(sc::percent{1}), 100.0);
    }

    SECTION("Values above 100% are clamped");
    {
        CHECK_EQ(static_cast<double>(sc::percent{110}), 100.0);
        CHECK_EQ(static_cast<string>(sc::percent{110}), string{"100.00%"});
        CHECK_EQ(static_cast<double>(sc::percent{100000}), 100.0);
    }

    SECTION("Negative values pass through unclamped");
    {
        CHECK_EQ(static_cast<double>(sc::percent{-0.25}), -25.0);
        CHECK_EQ(static_cast<string>(sc::percent{-0.25}), string{"-25.00%"});
    }

    SECTION("String concatenation operator");
    {
        const sc::percent battery{0.8};
        const sc::percent rounded{0.8, 0};
        CHECK_EQ(string{"Battery: "} + battery, string{"Battery: 80.00%"});
        CHECK_EQ(string{} + rounded, string{"80%"});
    }

    TEST_SUMMARY();
}
