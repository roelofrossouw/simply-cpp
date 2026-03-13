#ifndef BASE64_H
#define BASE64_H
#include <string>

namespace rjr {
    class base64 {
    public:
        static std::string decode(const std::string &encoded);
        static std::string encode(const std::string &text);
    };
}

#endif //BASE64_H
