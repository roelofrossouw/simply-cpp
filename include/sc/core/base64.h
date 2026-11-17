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

        // Base64url (RFC 4648 section 5: '-' and '_' for '+' and '/'), unpadded unless pad, as JWTs
        // and WebAuthn use it. decode_url() takes it padded or not, and nothing else: no '+' or '/',
        // no length of 1 mod 4, no bits set past the end (one text for each bytes); it throws
        // std::runtime_error for the rest.
        static std::string encode_url(std::string_view text, bool pad = false);

        static std::string decode_url(std::string_view encoded);
    };
}
