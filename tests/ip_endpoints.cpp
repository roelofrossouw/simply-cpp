#include <sc.h>

#include "sc_test.h"

#include <sstream>
#include <stdexcept>
#include <vector>

namespace {
    // Stands in for constructors such as sc::redis's, which take either form.
    int overload(const std::string &) { return 1; }
    int overload(const std::vector<sc::ip_endpoint> &) { return 2; }
}

int main() {
    SECTION("Parse a ';'-separated string");
    const sc::ip_endpoints parsed{" redis1:7000 ;; [::1]:7001 ;\tredis3 ", 6379};
    CHECK_EQ(parsed.size(), size_t{3});
    CHECK_EQ(parsed[0].to_string(), "redis1:7000");
    CHECK_EQ(parsed[1].to_string(), "[::1]:7001");
    CHECK_EQ(parsed[2].to_string(), "redis3:6379");
    CHECK_EQ(parsed.front().host, "redis1");
    CHECK_EQ(parsed.back().port, 6379);

    SECTION("Empty text gives an empty list");
    CHECK(sc::ip_endpoints{""}.empty());
    CHECK(sc::ip_endpoints{" ; "}.empty());
    CHECK(sc::ip_endpoints{}.empty());

    SECTION("Without a default port, entries without one have port 0");
    CHECK_EQ(sc::ip_endpoints{"redis1"}.front().port, 0);

    SECTION("An invalid entry throws");
    CHECK_THROWS_AS(sc::ip_endpoints{"redis1;redis2:notaport"}, std::invalid_argument);
    CHECK_THROWS_AS(sc::ip_endpoints{"[::1"}, std::invalid_argument);

    SECTION("String conversion writes the format it reads");
    CHECK_EQ(parsed.to_string(), "redis1:7000;[::1]:7001;redis3:6379");
    CHECK_EQ(static_cast<std::string>(parsed), "redis1:7000;[::1]:7001;redis3:6379");
    CHECK_EQ(parsed.to_string(","), "redis1:7000,[::1]:7001,redis3:6379");
    CHECK((sc::ip_endpoints{parsed.to_string()} == parsed));
    CHECK_EQ(sc::ip_endpoints{}.to_string(), "");
    std::ostringstream stream;
    stream << parsed;
    CHECK_EQ(stream.str(), "redis1:7000;[::1]:7001;redis3:6379");

    SECTION("Vector conversion");
    const std::vector<sc::ip_endpoint> vector = parsed;
    CHECK_EQ(vector.size(), size_t{3});
    CHECK_EQ(vector[1].host, "::1");
    const sc::ip_endpoints from_vector = vector;
    CHECK((from_vector == parsed));
    CHECK_EQ(overload(parsed), 2);
    CHECK_EQ(overload(static_cast<std::string>(parsed)), 1);

    SECTION("Container functions");
    sc::ip_endpoints endpoints{{"db1", 5432}};
    endpoints.push_back(sc::ip_endpoint{"db2", 5433});
    endpoints.emplace_back(sc::ip_endpoint::parse("db3", 5432));
    CHECK_EQ(endpoints.size(), size_t{3});
    std::vector<std::string> hosts;
    for (const auto &endpoint: endpoints) hosts.push_back(endpoint.host);
    CHECK((hosts == std::vector<std::string>{"db1", "db2", "db3"}));
    for (auto &endpoint: endpoints) endpoint.port = 1;
    CHECK_EQ(endpoints.to_string(), "db1:1;db2:1;db3:1");
    CHECK_EQ(endpoints.at(2).host, "db3");
    CHECK_THROWS_AS((void) endpoints.at(3), std::out_of_range);
    endpoints.erase(endpoints.begin());
    CHECK_EQ(endpoints.front().host, "db2");
    CHECK((endpoints != parsed));
    endpoints.clear();
    CHECK(endpoints.empty());

    TEST_SUMMARY();
}
