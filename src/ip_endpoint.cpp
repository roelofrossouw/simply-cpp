#include "ip_endpoint.h"

#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include <cstring>

static_assert(sizeof(sockaddr_storage) <= 128, "socket_address' storage is smaller than sockaddr_storage");
static_assert(sizeof(socklen_t) == sizeof(std::uint32_t), "socket_address keeps its size as a socklen_t");

int sc::socket_address::family() const {
    return size_ == 0 ? 0 : reinterpret_cast<const sockaddr_storage *>(storage_)->ss_family;
}

bool sc::socket_address::ipv6() const {
    return family() == AF_INET6;
}

std::vector<sc::socket_address> sc::ip_endpoint::resolve(const ip_use use, const ip_protocol protocol) const {
    addrinfo hints{};
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = protocol == ip_protocol::tcp ? SOCK_STREAM : SOCK_DGRAM;
    if (use == ip_use::listen) hints.ai_flags = AI_PASSIVE;
    addrinfo *found = nullptr;
    const auto service = std::to_string(port);
    const char *name = host.empty() ? nullptr : host.c_str();
    if (const int error = getaddrinfo(name, service.c_str(), &hints, &found); error != 0) {
        throw std::runtime_error(to_string() + ": " + gai_strerror(error));
    }
    std::vector<socket_address> addresses;
    for (const addrinfo *each = found; each; each = each->ai_next) {
        if (each->ai_addrlen > sizeof(socket_address::storage_)) continue;
        socket_address address;
        std::memcpy(address.storage_, each->ai_addr, each->ai_addrlen);
        address.size_ = static_cast<std::uint32_t>(each->ai_addrlen);
        address.socket_type_ = each->ai_socktype;
        address.protocol_ = each->ai_protocol;
        addresses.push_back(address);
    }
    freeaddrinfo(found);
    if (addresses.empty()) throw std::runtime_error(to_string() + ": no address");
    return addresses;
}

sc::ip_endpoint sc::ip_endpoint::from(const socket_address &address) {
    char text[INET6_ADDRSTRLEN]{};
    if (address.family() == AF_INET6) {
        const auto &v6 = *reinterpret_cast<const sockaddr_in6 *>(address.get());
        inet_ntop(AF_INET6, &v6.sin6_addr, text, sizeof text);
        std::string numeric = text;
        if (numeric.starts_with("::ffff:") && numeric.find('.') != std::string::npos) numeric = numeric.substr(7);
        return {numeric, ntohs(v6.sin6_port)};
    }
    if (address.family() == AF_INET) {
        const auto &v4 = *reinterpret_cast<const sockaddr_in *>(address.get());
        inet_ntop(AF_INET, &v4.sin_addr, text, sizeof text);
        return {text, ntohs(v4.sin_port)};
    }
    return {};
}

sc::ip_endpoint sc::ip_endpoint::local_of(const int socket) {
    socket_address address;
    if (getsockname(socket, address.get(), address.size_pointer()) != 0) return {};
    return from(address);
}

sc::ip_endpoint sc::ip_endpoint::peer_of(const int socket) {
    socket_address address;
    if (getpeername(socket, address.get(), address.size_pointer()) != 0) return {};
    return from(address);
}

std::optional<std::uint32_t> sc::ip_endpoint::ipv4() const {
    in_addr address{};
    if (inet_pton(AF_INET, host.c_str(), &address) != 1) return std::nullopt;
    return address.s_addr;
}
