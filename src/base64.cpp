#include "base64.h"
#include <stdexcept>
#include <cstdint>
#include <iostream>

namespace sc {
    namespace base64_impl {
        constexpr uint32_t invalid_bit = 0x01000000;
        constexpr uint32_t invalid = 0x01FFFFFF;
        constexpr char padding = '=';
        constexpr std::string_view alphabet{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

        struct decode_table {
            uint32_t data[256];
            constexpr uint32_t &operator[](const size_t i) { return data[i]; }
            constexpr const uint32_t &operator[](const size_t i) const { return data[i]; }

            explicit constexpr decode_table(auto f) : data{} {
                for (auto &item: data) item = invalid;
                for (uint32_t i = 0; i < alphabet.size(); ++i) data[static_cast<uint8_t>(alphabet[i])] = f(i);
            }
        };
    }

    inline constexpr base64_impl::decode_table table_0{[](const uint32_t i) { return i << 2; }};
    inline constexpr base64_impl::decode_table table_1{[](const uint32_t i) { return ((i & 0x0F) << 12) | (i >> 4); }};
    inline constexpr base64_impl::decode_table table_2{[](const uint32_t i) { return ((i & 0x03) << 22) | ((i >> 2) << 8); }};
    inline constexpr base64_impl::decode_table table_3{[](const uint32_t i) { return i << 16; }};

    std::string base64::encode(const std::string_view text) {
        if (empty(text)) return {};
        const auto input_size = text.size();
        const auto pad_size = input_size % 3;
        const auto output_size = (input_size + 2) / 3 * 4;
        std::string encoded;
        encoded.resize(output_size);

        auto *buffer_in = reinterpret_cast<const uint8_t *>(text.data());
        auto *buffer_out = &encoded[0];
        std::uint32_t value;

        for (size_t i = input_size / 3; i; --i) {
            const auto b0 = static_cast<std::uint8_t>(*buffer_in++);
            const auto b1 = static_cast<std::uint8_t>(*buffer_in++);
            const auto b2 = static_cast<std::uint8_t>(*buffer_in++);
            value = (std::uint32_t{b0} << 16) | (std::uint32_t{b1} << 8) | std::uint32_t{b2};
            *buffer_out++ = base64_impl::alphabet[(value >> 18) & 63];
            *buffer_out++ = base64_impl::alphabet[(value >> 12) & 63];
            *buffer_out++ = base64_impl::alphabet[(value >> 6) & 63];
            *buffer_out++ = base64_impl::alphabet[(value >> 0) & 63];
        }

        if (pad_size >= 1) {
            std::uint8_t b1{0};
            const auto b0 = static_cast<std::uint8_t>(*buffer_in++);
            if (pad_size == 2) b1 = static_cast<std::uint8_t>(*buffer_in++);
            value = (std::uint32_t{b0} << 16) | (std::uint32_t{b1} << 8);
            *buffer_out++ = base64_impl::alphabet[(value >> 18) & 63];
            *buffer_out++ = base64_impl::alphabet[(value >> 12) & 63];
            *buffer_out++ = pad_size == 2 ? base64_impl::alphabet[(value >> 6) & 63] : base64_impl::padding;
            *buffer_out++ = base64_impl::padding;
        }
        return encoded;
    }


    std::string base64::decode(const std::string_view encoded) {
        if (encoded.empty()) { return {}; }
        const auto encoded_size = encoded.size();
        if ((encoded_size & 3) != 0) { throw std::runtime_error{"Invalid base64 (size)"}; }
        const size_t pad_size = encoded[encoded_size - 1] == base64_impl::padding
                                    ? (encoded_size > 1 && encoded[encoded_size - 2] == base64_impl::padding ? 2 : 1)
                                    : 0;

        std::string decoded;
        decoded.resize((encoded_size * 3 >> 2) - pad_size);

        auto *buffer_in = reinterpret_cast<const uint8_t *>(encoded.data());
        auto *buffer_out = &decoded[0];

        uint32_t value;
        for (size_t i = (encoded_size >> 2) - (pad_size != 0); i; --i) {
            value = table_0[*buffer_in++];
            value |= table_1[*buffer_in++];
            value |= table_2[*buffer_in++];
            value |= table_3[*buffer_in++];
            if (value & base64_impl::invalid_bit) throw std::runtime_error{"Invalid base64 character"};
            *buffer_out++ = static_cast<char>((value >> 0) & 0xFF);
            *buffer_out++ = static_cast<char>((value >> 8) & 0xFF);
            *buffer_out++ = static_cast<char>((value >> 16) & 0xFF);
        }

        if (pad_size >= 1) {
            value = table_0[*buffer_in++];
            value |= table_1[*buffer_in++];
            if (pad_size == 1) value |= table_2[*buffer_in++];
            if (value & base64_impl::invalid_bit) throw std::runtime_error{"Invalid base64 character"};
            *buffer_out++ = static_cast<char>((value >> 0) & 0xFF);
            if (pad_size == 1) *buffer_out++ = static_cast<char>((value >> 8) & 0xFF);
        }
        return decoded;
    }

    namespace base64_impl {
        constexpr std::string_view url_alphabet{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789-_"};

        constexpr int url_sextet(const char c) {
            if (c >= 'A' && c <= 'Z') return c - 'A';
            if (c >= 'a' && c <= 'z') return c - 'a' + 26;
            if (c >= '0' && c <= '9') return c - '0' + 52;
            if (c == '-') return 62;
            if (c == '_') return 63;
            return -1;
        }
    }

    std::string base64::encode_url(const std::string_view text, const bool pad) {
        std::string out;
        out.reserve((text.size() + 2) / 3 * 4);
        std::size_t i = 0;
        for (; i + 2 < text.size(); i += 3) {
            const auto n = std::uint32_t{static_cast<std::uint8_t>(text[i])} << 16 | std::uint32_t{static_cast<std::uint8_t>(text[i + 1])} << 8 |
                           std::uint32_t{static_cast<std::uint8_t>(text[i + 2])};
            out += base64_impl::url_alphabet[n >> 18 & 63];
            out += base64_impl::url_alphabet[n >> 12 & 63];
            out += base64_impl::url_alphabet[n >> 6 & 63];
            out += base64_impl::url_alphabet[n & 63];
        }
        if (const auto rest = text.size() - i; rest > 0) {
            auto n = std::uint32_t{static_cast<std::uint8_t>(text[i])} << 16;
            if (rest == 2) n |= std::uint32_t{static_cast<std::uint8_t>(text[i + 1])} << 8;
            out += base64_impl::url_alphabet[n >> 18 & 63];
            out += base64_impl::url_alphabet[n >> 12 & 63];
            if (rest == 2) out += base64_impl::url_alphabet[n >> 6 & 63];
            if (pad) out.append(rest == 1 ? 2 : 1, base64_impl::padding);
        }
        return out;
    }

    std::string base64::decode_url(std::string_view encoded) {
        // Padding, when there is any, must make it a whole number of quads.
        if (!encoded.empty() && encoded.back() == base64_impl::padding) {
            if (encoded.size() % 4 != 0) throw std::runtime_error{"Invalid base64url (padding)"};
            encoded.remove_suffix(encoded.size() >= 2 && encoded[encoded.size() - 2] == base64_impl::padding ? 2 : 1);
        }
        if (encoded.size() % 4 == 1) throw std::runtime_error{"Invalid base64url (size)"};
        std::string out;
        out.reserve(encoded.size() * 3 / 4);
        std::uint32_t n = 0;
        int bits = 0;
        for (const char c: encoded) {
            const auto v = base64_impl::url_sextet(c);
            if (v < 0) throw std::runtime_error{"Invalid base64url character"};
            n = n << 6 | static_cast<std::uint32_t>(v);
            bits += 6;
            if (bits >= 8) {
                bits -= 8;
                out += static_cast<char>(n >> bits & 0xFF);
            }
        }
        if (bits > 0 && (n & ((1u << bits) - 1)) != 0) throw std::runtime_error{"Invalid base64url (bits past the end)"};
        return out;
    }
}
