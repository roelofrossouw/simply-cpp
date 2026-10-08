#include <sc.h>

#include "sc_test.h"

#include <sstream>

int main() {
    SECTION("Construction");
    const sc::ip_endpoint endpoint{"example.com", 8080};
    CHECK_EQ(endpoint.host, "example.com");
    CHECK_EQ(endpoint.port, 8080);

    SECTION("Defaults");
    const sc::ip_endpoint unspecified;
    CHECK_EQ(unspecified.host, "");
    CHECK_EQ(unspecified.port, 0);

    SECTION("Parse host and port");
    CHECK_EQ(sc::ip_endpoint::parse("example.com:8080").host, "example.com");
    CHECK_EQ(sc::ip_endpoint::parse("example.com:8080").port, 8080);
    CHECK_EQ(sc::ip_endpoint::parse("10.0.0.1:1").port, 1);
    CHECK_EQ(sc::ip_endpoint::parse("10.0.0.1:65535").port, 65535);

    SECTION("Parse default port");
    CHECK_EQ(sc::ip_endpoint::parse("example.com").host, "example.com");
    CHECK_EQ(sc::ip_endpoint::parse("example.com").port, 0);
    CHECK_EQ(sc::ip_endpoint::parse("example.com", 6379).port, 6379);
    CHECK_EQ(sc::ip_endpoint::parse("example.com:80", 6379).port, 80);

    SECTION("Parse IPv6");
    CHECK_EQ(sc::ip_endpoint::parse("[::1]:6379").host, "::1");
    CHECK_EQ(sc::ip_endpoint::parse("[::1]:6379").port, 6379);
    CHECK_EQ(sc::ip_endpoint::parse("[fe80::1]", 443).host, "fe80::1");
    CHECK_EQ(sc::ip_endpoint::parse("[fe80::1]", 443).port, 443);
    CHECK_EQ(sc::ip_endpoint::parse("::1", 53).host, "::1");
    CHECK_EQ(sc::ip_endpoint::parse("::1", 53).port, 53);

    SECTION("Parse invalid");
    CHECK_THROWS_AS(sc::ip_endpoint::parse(""), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse(":8080"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:http"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:80x"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:0"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:65536"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("example.com:-1"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("[::1"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("[::1]8080"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::parse("[]:8080"), std::invalid_argument);

    SECTION("From Redis redirect");
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 3999 127.0.0.1:6381").host, "127.0.0.1");
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 3999 127.0.0.1:6381").port, 6381);
    CHECK_EQ(sc::ip_endpoint::from_redis("ASK 0 redis2.example.com:6379").host, "redis2.example.com");
    CHECK_EQ(sc::ip_endpoint::from_redis("ASK 0 redis2.example.com:6379").port, 6379);
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 16383 ::1:6380").host, "::1");
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 16383 ::1:6380").port, 6380);
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 1 fe80::1:2:6380").host, "fe80::1:2");
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 1 [::1]:6380").host, "::1");
    CHECK_EQ(sc::ip_endpoint::from_redis("MOVED 1 [::1]:6380").port, 6380);

    SECTION("From Redis redirect invalid");
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis(""), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 host"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 host:"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 :6380"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 host:0"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 host:99999"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED 3999 [::1]"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("MOVED x host:6380"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("ERR 3999 host:6380"), std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoint::from_redis("host:6380"), std::invalid_argument);

    SECTION("To string");
    CHECK_EQ((sc::ip_endpoint{"example.com", 8080}.to_string()), "example.com:8080");
    CHECK_EQ((sc::ip_endpoint{"10.0.0.1", 6379}.to_string()), "10.0.0.1:6379");
    CHECK_EQ((sc::ip_endpoint{"::1", 6379}.to_string()), "[::1]:6379");
    CHECK_EQ((sc::ip_endpoint{"example.com", 0}.to_string()), "example.com");
    CHECK_EQ((sc::ip_endpoint{"::1", 0}.to_string()), "::1");
    std::ostringstream stream;
    stream << sc::ip_endpoint{"fe80::1", 443} << ' ' << sc::ip_endpoint{"redis1", 6379};
    CHECK_EQ(stream.str(), "[fe80::1]:443 redis1:6379");

    SECTION("To string parses back");
    for (const sc::ip_endpoint &endpoint : {sc::ip_endpoint{"example.com", 8080}, sc::ip_endpoint{"::1", 6379},
                                            sc::ip_endpoint{"fe80::1:2", 1}, sc::ip_endpoint{"redis1", 0},
                                            sc::ip_endpoint{"::1", 0}}) {
        const auto parsed = sc::ip_endpoint::parse(endpoint.to_string());
        CHECK_EQ(parsed.host, endpoint.host);
        CHECK_EQ(parsed.port, endpoint.port);
    }

    TEST_SUMMARY();
}
