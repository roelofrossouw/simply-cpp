#pragma once

#include <ip_endpoints.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <vector>

namespace sc {
    // Servers for a demo, read from an environment variable such as SC_REDIS_DEMO_SERVER.
    // The value is one or more "host[:port]" / "[ipv6]:port" entries separated by ';'.
    // Whitespace and empty entries are ignored, and default_port fills in a missing port.
    // Unset or empty falls back to 127.0.0.1:default_port; so does an invalid entry, with a
    // one-line warning on std::cerr.
    [[nodiscard]] inline std::vector<ip_endpoint> demo_servers(const char *variable, const int default_port) {
        const std::vector<ip_endpoint> fallback{{"127.0.0.1", default_port}};
        const char *value = std::getenv(variable);
        if (!value || !*value) return fallback;

        try {
            const ip_endpoints servers{value, default_port};
            if (servers.empty()) return fallback;
            return servers;
        } catch (const std::invalid_argument &error) {
            std::cerr << variable << " ignored, " << error.what() << '\n';
            return fallback;
        }
    }
}
