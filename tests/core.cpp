#include <sc.h>

#include "sc_test.h"

#include <cstdlib>
#include <stdexcept>

using strings = std::vector<std::string>;

int main() {
    SECTION("getenv");
    const std::string variable = "SC_CORE_TEST_GETENV";
    unsetenv(variable.c_str());
    CHECK_EQ(sc::getenv(variable), "");
    CHECK_EQ(sc::getenv(variable, "fallback"), "fallback");
    setenv(variable.c_str(), "", 1);
    CHECK_EQ(sc::getenv(variable, "fallback"), "fallback");
    setenv(variable.c_str(), "value", 1);
    CHECK_EQ(sc::getenv(variable), "value");
    CHECK_EQ(sc::getenv(variable, "fallback"), "value");
    unsetenv(variable.c_str());

    SECTION("explode");
    CHECK((sc::explode("a;b;c") == strings{"a", "b", "c"}));
    CHECK((sc::explode("a") == strings{"a"}));
    CHECK((sc::explode("") == strings{""}));
    CHECK((sc::explode("a;;b;") == strings{"a", "", "b", ""}));
    CHECK((sc::explode(";") == strings{"", ""}));
    CHECK((sc::explode("a, b,c", ",") == strings{"a", " b", "c"}));
    CHECK((sc::explode("a::b:c", "::") == strings{"a", "b:c"}));
    CHECK((sc::explode("a;b", ",") == strings{"a;b"}));
    CHECK_THROWS_AS(sc::explode("a;b", ""), std::invalid_argument);

    TEST_SUMMARY();
}
