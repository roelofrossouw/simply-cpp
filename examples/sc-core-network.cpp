// Network addresses in simply-cpp core: IPv4 addresses, host:port endpoints, and ';'-separated
// lists of them, as the other modules take their servers. Each line shows a call, as written, and
// what it returned. Needs no server.

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: network addresses");

    sc::console::heading("IPv4 addresses: ip_address");
    SC_SHOW(ip_address{"192.168.1.10"});
    SC_SHOW(ip_address{"192.168.1.10"} == ip_address{"192.168.1.10"});

    sc::console::heading("One server: sc::ip_endpoint");
    SC_SHOW(sc::ip_endpoint::parse("db.example.com:5432"));
    SC_SHOW(sc::ip_endpoint::parse("db.example.com", 5432)); // a default port
    SC_SHOW(sc::ip_endpoint::parse("[::1]:6379").host);       // IPv6 in brackets
    SC_SHOW(sc::ip_endpoint::parse("::1", 6379));
    SC_SHOW(sc::ip_endpoint::from_redis("MOVED 3999 10.0.0.5:6381")); // a Redis cluster redirect

    sc::console::heading("Several servers: sc::ip_endpoints");
    const sc::ip_endpoints brokers{"kafka1;kafka2:9093; kafka3", 9092};
    SC_SHOW(brokers);
    SC_SHOW(brokers.size());
    SC_SHOW(brokers.to_string(","));
    SC_SHOW(brokers.front().port);
    sc::console::subheading("Each one in turn");
    for (const auto &broker: brokers) sc::console::note(broker.host + " on port " + std::to_string(broker.port));
    return 0;
}
