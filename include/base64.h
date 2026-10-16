#pragma once
#include <string_view>
#include <string>

namespace sc {
    // Standard base64 (RFC 4648, padded with '='). decode() throws std::runtime_error for text that
    // isn't base64.
    class base64 {
    public:
        static std::string encode(std::string_view text);

        static std::string decode(std::string_view encoded);
    };
}
