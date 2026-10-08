#pragma once

#include <charconv>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>

namespace sc {
    struct ip_endpoint {
        std::string host;
        int port = 0;

        // "host:port", "[ipv6]:port", or just the host when there is no port (port 0).
        // The same format parse() reads.
        [[nodiscard]] std::string to_string() const {
            if (port == 0) return host;
            if (host.find(':') != std::string::npos) return '[' + host + "]:" + std::to_string(port);
            return host + ':' + std::to_string(port);
        }

        friend std::ostream &operator<<(std::ostream &os, const ip_endpoint &endpoint) {
            return os << endpoint.to_string();
        }

        // Parses "host", "host:port", "[ipv6]" or "[ipv6]:port". A bare IPv6 address
        // ("::1") has no port. default_port is used when the text has none.
        [[nodiscard]] static ip_endpoint parse(const std::string_view text, const int default_port = 0) {
            std::string_view host = text;
            std::string_view port;
            bool has_port = false;

            if (!text.empty() && text.front() == '[') {
                const auto close = text.find(']');
                if (close == std::string_view::npos) {
                    throw std::invalid_argument("invalid endpoint, missing ']': " + std::string(text));
                }
                host = text.substr(1, close - 1);
                const auto rest = text.substr(close + 1);
                if (!rest.empty()) {
                    if (rest.front() != ':') {
                        throw std::invalid_argument("invalid endpoint, expected ':' after ']': " + std::string(text));
                    }
                    port = rest.substr(1);
                    has_port = true;
                }
            } else if (const auto colon = text.find(':'); colon != std::string_view::npos &&
                                                          colon == text.rfind(':')) {
                host = text.substr(0, colon);
                port = text.substr(colon + 1);
                has_port = true;
            }

            if (host.empty()) {
                throw std::invalid_argument("invalid endpoint, missing host: " + std::string(text));
            }
            if (!has_port) {
                return {std::string(host), default_port};
            }

            int number = 0;
            const auto [end, error] = std::from_chars(port.data(), port.data() + port.size(), number);
            if (port.empty() || error != std::errc{} || end != port.data() + port.size() ||
                number < 1 || number > 65535) {
                throw std::invalid_argument("invalid endpoint port, expected 1 to 65535: " + std::string(text));
            }
            return {std::string(host), number};
        }

        // Parses the target of a Redis Cluster redirect error, "MOVED <slot> <host>:<port>" or
        // "ASK <slot> <host>:<port>". Redis writes IPv6 hosts unbracketed ("::1:6380"), so the
        // last ':' always separates the port.
        [[nodiscard]] static ip_endpoint from_redis(const std::string_view message) {
            const auto invalid = [&] {
                return std::invalid_argument("invalid Redis redirect: " + std::string(message));
            };

            const auto first_space = message.find(' ');
            const auto second_space = message.find(' ', first_space == std::string_view::npos ? 0 : first_space + 1);
            if (first_space == std::string_view::npos || second_space == std::string_view::npos) throw invalid();

            const auto kind = message.substr(0, first_space);
            const auto slot = message.substr(first_space + 1, second_space - first_space - 1);
            const auto address = message.substr(second_space + 1);
            if ((kind != "MOVED" && kind != "ASK") || slot.empty() ||
                slot.find_first_not_of("0123456789") != std::string_view::npos) {
                throw invalid();
            }

            const auto port_separator = address.rfind(':');
            if (port_separator == std::string_view::npos) throw invalid();
            const auto host = address.substr(0, port_separator);
            const bool bracketed = !host.empty() && host.front() == '[';
            ip_endpoint endpoint;
            try {
                endpoint = parse(host.find(':') == std::string_view::npos || bracketed
                                     ? std::string(address)
                                     : '[' + std::string(host) + ']' + std::string(address.substr(port_separator)));
            } catch (const std::invalid_argument &) {
                throw invalid();
            }
            if (endpoint.port == 0) throw invalid();
            return endpoint;
        }
    };
}
