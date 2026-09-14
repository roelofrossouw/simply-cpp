#pragma once
#include <string_view>
#include <string>

namespace sc {
    class base64 {
    public:
        static std::string encode(std::string_view text);

        static std::string decode(std::string_view encoded);
    };
}
