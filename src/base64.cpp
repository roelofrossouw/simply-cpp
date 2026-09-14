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

        // 65 66 67 68 69 70 71 72 73 74 75 76 77 78 79 80 81 82 83 84 85 86 87 88 89 90 97 98 99 100 101 102 103 104 105 106 107 108 109 110 111 112 113 114 115 116 117 118 119 120 121 122
        // 48 49 50 51 52 53 54 55 56 57 
        // 43
        // 47
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
}
