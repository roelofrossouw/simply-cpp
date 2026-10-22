#pragma once

#include <charconv>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <ostream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <vector>

struct sockaddr;

namespace sc {
    // A socket address as the system's socket calls take it (bind, connect, accept), so code
    // working with ip_endpoint needn't include the socket headers or handle sockaddr itself:
    //     auto address = endpoint.resolve(sc::ip_use::listen).front();
    //     bind(fd, address.get(), address.size());
    //     sc::socket_address peer;
    //     const int client = accept(fd, peer.get(), peer.size_pointer());
    //     sc::ip_endpoint::from(peer);   // {"192.0.2.7", 51234}
    // Since sc-core 1.21.0.
    class socket_address {
    public:
        socket_address() = default;

        [[nodiscard]] sockaddr *get() { return reinterpret_cast<sockaddr *>(storage_); }
        [[nodiscard]] const sockaddr *get() const { return reinterpret_cast<const sockaddr *>(storage_); }
        // The address' length (a socklen_t); before an accept() or getsockname(), the room there is.
        [[nodiscard]] std::uint32_t size() const { return size_; }
        // For the calls that fill it in and report the length (accept, getsockname, getpeername).
        [[nodiscard]] std::uint32_t *size_pointer() { return &size_; }
        // AF_INET, AF_INET6 ...; 0 when empty.
        [[nodiscard]] int family() const;
        [[nodiscard]] bool ipv6() const;
        // The socket's type and protocol to make for it (as getaddrinfo gives them: SOCK_STREAM,
        // IPPROTO_TCP for a TCP address).
        [[nodiscard]] int socket_type() const { return socket_type_; }
        [[nodiscard]] int protocol() const { return protocol_; }

    private:
        friend struct ip_endpoint;
        alignas(16) std::byte storage_[128]{}; // sockaddr_storage's size on Linux and macOS
        std::uint32_t size_ = sizeof storage_;
        int socket_type_ = 0;
        int protocol_ = 0;
    };

    // What an address is resolved for: listening (an empty host or "0.0.0.0" is every interface)
    // or connecting.
    enum class ip_use { connect, listen };

    // TCP or UDP, for resolve().
    enum class ip_protocol { tcp, udp };

    // A host (name, IPv4 or IPv6 address) and port; port 0 means none was given.
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

        bool operator==(const ip_endpoint &) const = default;

        // The addresses the host and port name (getaddrinfo), IPv4 and IPv6 as the system orders
        // them. Throws std::runtime_error, naming the endpoint, when there are none. Since 1.21.0.
        [[nodiscard]] std::vector<socket_address> resolve(ip_use use = ip_use::connect, ip_protocol protocol = ip_protocol::tcp) const;

        // An address as an endpoint: the numeric host and the port. An IPv4 address on an IPv6
        // socket ("::ffff:192.0.2.7") comes back as IPv4. Since 1.21.0.
        [[nodiscard]] static ip_endpoint from(const socket_address &address);
        // A socket's own address (getsockname), and the other end's (getpeername); empty ({"", 0})
        // when the call fails. Since 1.21.0.
        [[nodiscard]] static ip_endpoint local_of(int socket);
        [[nodiscard]] static ip_endpoint peer_of(int socket);

        // The host as an IPv4 address in network byte order (in_addr's s_addr, as sc::ip_address
        // keeps it); nothing for an IPv6 address or a name. Since 1.21.0.
        [[nodiscard]] std::optional<std::uint32_t> ipv4() const;

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
