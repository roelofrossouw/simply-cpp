#pragma once

#include <arpa/inet.h>
#include <cstdint>
#include <ostream>
#include <stdexcept>
#include <string>

class ip_address {
public:
    using value_type = std::uint32_t;

    constexpr ip_address() noexcept = default;

    constexpr explicit ip_address(const value_type s_addr) noexcept : address_(s_addr) {
    }

    explicit ip_address(const std::string &ip) {
        in_addr addr{};
        if (inet_pton(AF_INET, ip.c_str(), &addr) != 1) {
            throw std::invalid_argument("invalid IPv4 address: " + ip);
        }
        address_ = addr.s_addr;
    }

    explicit ip_address(const char *ip) : ip_address(std::string(ip)) {
    }

    [[nodiscard]] constexpr value_type s_addr() const noexcept {
        return address_;
    }

    [[nodiscard]] std::string to_string() const {
        char buffer[INET_ADDRSTRLEN];

        in_addr addr{};
        addr.s_addr = address_;

        if (!inet_ntop(AF_INET, &addr, buffer, sizeof(buffer))) {
            throw std::runtime_error("inet_ntop failed");
        }

        return buffer;
    }

    operator std::string() const { return to_string(); }

    friend std::ostream &operator<<(std::ostream &os, const ip_address &ip) { return os << ip.to_string(); }

    friend constexpr bool operator==(const ip_address &, const ip_address &) noexcept = default;

private:
    value_type address_ = 0;
};
