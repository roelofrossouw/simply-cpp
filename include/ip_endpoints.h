#pragma once

#include <ip_endpoint.h>

#include <cstddef>
#include <initializer_list>
#include <ostream>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace sc {
    // A list of endpoints, written as "host[:port]" / "[ipv6]:port" entries separated by ';',
    // e.g. "redis1:6379;redis2:6380" - the format SC_<MODULE>_DEMO_SERVER uses.
    // Behaves like a std::vector<ip_endpoint> and converts to one implicitly, so it can be
    // passed straight to sc::redis or sc::postgres. The string conversion is explicit, so it
    // never competes with those constructors' std::string overloads.
    class ip_endpoints {
    public:
        using value_type = ip_endpoint;
        using container = std::vector<ip_endpoint>;
        using iterator = container::iterator;
        using const_iterator = container::const_iterator;
        using size_type = container::size_type;
        using reference = container::reference;
        using const_reference = container::const_reference;

        ip_endpoints() = default;

        ip_endpoints(std::vector<ip_endpoint> endpoints) : endpoints_(std::move(endpoints)) {
        }

        ip_endpoints(const std::initializer_list<ip_endpoint> endpoints) : endpoints_(endpoints) {
        }

        // Parses ';'-separated endpoints. Whitespace around entries and empty entries are
        // ignored, so "" gives an empty list. default_port fills in a missing port. Throws
        // std::invalid_argument naming the first invalid entry.
        explicit ip_endpoints(const std::string_view text, const int default_port = 0) {
            std::string_view rest = text;
            while (!rest.empty()) {
                const auto separator = rest.find(';');
                auto item = rest.substr(0, separator);
                rest = separator == std::string_view::npos ? std::string_view{} : rest.substr(separator + 1);

                const auto first = item.find_first_not_of(" \t");
                if (first == std::string_view::npos) continue;
                item = item.substr(first, item.find_last_not_of(" \t") - first + 1);
                endpoints_.push_back(ip_endpoint::parse(item, default_port));
            }
        }

        // The endpoints in the format the string constructor reads. Pass "," for a list such as
        // Kafka's bootstrap.servers.
        [[nodiscard]] std::string to_string(const std::string_view separator = ";") const {
            std::string text;
            for (const auto &endpoint: endpoints_) {
                if (!text.empty()) text += separator;
                text += endpoint.to_string();
            }
            return text;
        }

        explicit operator std::string() const { return to_string(); }

        operator const std::vector<ip_endpoint> &() const { return endpoints_; }

        friend std::ostream &operator<<(std::ostream &os, const ip_endpoints &endpoints) {
            return os << endpoints.to_string();
        }

        friend bool operator==(const ip_endpoints &lhs, const ip_endpoints &rhs) {
            if (lhs.size() != rhs.size()) return false;
            for (size_type i = 0; i < lhs.size(); ++i) {
                if (lhs[i].host != rhs[i].host || lhs[i].port != rhs[i].port) return false;
            }
            return true;
        }

        [[nodiscard]] iterator begin() noexcept { return endpoints_.begin(); }
        [[nodiscard]] iterator end() noexcept { return endpoints_.end(); }
        [[nodiscard]] const_iterator begin() const noexcept { return endpoints_.begin(); }
        [[nodiscard]] const_iterator end() const noexcept { return endpoints_.end(); }
        [[nodiscard]] const_iterator cbegin() const noexcept { return endpoints_.cbegin(); }
        [[nodiscard]] const_iterator cend() const noexcept { return endpoints_.cend(); }

        [[nodiscard]] size_type size() const noexcept { return endpoints_.size(); }
        [[nodiscard]] bool empty() const noexcept { return endpoints_.empty(); }

        [[nodiscard]] reference front() { return endpoints_.front(); }
        [[nodiscard]] const_reference front() const { return endpoints_.front(); }
        [[nodiscard]] reference back() { return endpoints_.back(); }
        [[nodiscard]] const_reference back() const { return endpoints_.back(); }
        [[nodiscard]] reference operator[](const size_type index) { return endpoints_[index]; }
        [[nodiscard]] const_reference operator[](const size_type index) const { return endpoints_[index]; }
        [[nodiscard]] reference at(const size_type index) { return endpoints_.at(index); }
        [[nodiscard]] const_reference at(const size_type index) const { return endpoints_.at(index); }

        void push_back(const ip_endpoint &endpoint) { endpoints_.push_back(endpoint); }
        void push_back(ip_endpoint &&endpoint) { endpoints_.push_back(std::move(endpoint)); }

        template<typename... Args>
        reference emplace_back(Args &&... args) { return endpoints_.emplace_back(std::forward<Args>(args)...); }

        iterator erase(const const_iterator position) { return endpoints_.erase(position); }
        void clear() noexcept { endpoints_.clear(); }
        void reserve(const size_type capacity) { endpoints_.reserve(capacity); }

    private:
        container endpoints_;
    };
}
