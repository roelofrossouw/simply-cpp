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

    SECTION("lowercase");
    CHECK_EQ(sc::lowercase("Content-Type"), "content-type");
    CHECK_EQ(sc::lowercase(""), "");
    CHECK_EQ(sc::lowercase("ABC xyz 123"), "abc xyz 123");
    CHECK_EQ(sc::lowercase("\xC3\x89T\xC3\x89"), "\xC3\x89t\xC3\x89");

    TEST_SUMMARY();
}
