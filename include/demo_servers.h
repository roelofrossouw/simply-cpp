#pragma once

#include <ip_endpoint.h>

#include <cstdlib>
#include <iostream>
#include <stdexcept>
#include <string_view>
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

        std::vector<ip_endpoint> servers;
        std::string_view rest{value};
        while (!rest.empty()) {
            const auto separator = rest.find(';');
            auto item = rest.substr(0, separator);
            rest = separator == std::string_view::npos ? std::string_view{} : rest.substr(separator + 1);

            const auto first = item.find_first_not_of(" \t");
            if (first == std::string_view::npos) continue;
            item = item.substr(first, item.find_last_not_of(" \t") - first + 1);
            try {
                servers.push_back(ip_endpoint::parse(item, default_port));
            } catch (const std::invalid_argument &error) {
                std::cerr << variable << " ignored, " << error.what() << '\n';
                return fallback;
            }
        }
        return servers.empty() ? fallback : servers;
    }
}
