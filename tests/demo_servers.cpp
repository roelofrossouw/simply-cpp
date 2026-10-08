#include <sc.h>

#include "sc_test.h"

#include <cstdlib>

namespace {
    const char *VARIABLE = "SC_CORE_TEST_DEMO_SERVER";

    std::vector<sc::ip_endpoint> servers_for(const char *value) {
        if (value) setenv(VARIABLE, value, 1);
        else unsetenv(VARIABLE);
        return sc::demo_servers(VARIABLE, 6379);
    }

    bool is_fallback(const std::vector<sc::ip_endpoint> &servers) {
        return servers.size() == 1 && servers[0].host == "127.0.0.1" && servers[0].port == 6379;
    }
}

int main() {
    SECTION("Unset or empty falls back");
    CHECK(is_fallback(servers_for(nullptr)));
    CHECK(is_fallback(servers_for("")));
    CHECK(is_fallback(servers_for(" ; ;\t")));

    SECTION("One server, default port");
    const auto one = servers_for("redis1");
    CHECK_EQ(one.size(), size_t{1});
    if (one.size() == 1) {
        CHECK_EQ(one[0].host, "redis1");
        CHECK_EQ(one[0].port, 6379);
    }

    SECTION("Several servers, whitespace and empty entries ignored");
    const auto several = servers_for(" redis1:7000 ;; [::1]:7001 ;\tredis3 ");
    CHECK_EQ(several.size(), size_t{3});
    if (several.size() == 3) {
        CHECK_EQ(several[0].to_string(), "redis1:7000");
        CHECK_EQ(several[1].to_string(), "[::1]:7001");
        CHECK_EQ(several[2].to_string(), "redis3:6379");
    }

    SECTION("Any invalid entry falls back");
    CHECK(is_fallback(servers_for("redis1;redis2:notaport")));
    CHECK(is_fallback(servers_for("[::1")));

    unsetenv(VARIABLE);
    TEST_SUMMARY();
}
