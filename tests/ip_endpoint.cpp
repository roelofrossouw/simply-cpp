#include <sc.h>

#include "sc_test.h"

int main() {
    SECTION("Construction");
    const sc::ip_endpoint endpoint{"example.com", 8080};
    CHECK_EQ(endpoint.host, "example.com");
    CHECK_EQ(endpoint.port, 8080);

    SECTION("Defaults");
    const sc::ip_endpoint unspecified;
    CHECK_EQ(unspecified.host, "");
    CHECK_EQ(unspecified.port, 0);

    TEST_SUMMARY();
}
