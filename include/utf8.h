#ifndef SC_UTF8_H
#define SC_UTF8_H
#include <string_view>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace sc {
    struct TextAnalysis {
        std::size_t bytes = 0;
        std::size_t codepoints = 0;

        std::size_t ascii = 0;
        std::size_t whitespace = 0;
        std::size_t printable = 0;

        std::size_t controls = 0;
        std::size_t replacement = 0;
        std::size_t private_use = 0;

        bool valid_utf8 = true;

        [[nodiscard]]
        bool likely_text() const {
            return valid_utf8 &&
                   replacement == 0 &&
                   private_use == 0 &&
                   (codepoints == 0 ||
                    (controls * 100 / codepoints) < 5);
        }
    };

    class utf8 {
    public:
        utf8() = delete; // static-only utility class


        static constexpr std::size_t npos = static_cast<std::size_t>(-1);
        static constexpr std::uint32_t kReplacementChar = 0xFFFD;

        // ---------------------------------------------------------------------
        // Validation
        // ---------------------------------------------------------------------

        // Index of the first invalid byte, or npos if the string is valid UTF-8.
        // Rejects overlong encodings, UTF-16 surrogates and values > U+10FFFF.
        static std::size_t find_invalid(std::string_view s);

        static bool is_valid(std::string_view s) {
            return find_invalid(s) == npos;
        }

        // True if byte is a UTF-8 continuation byte (10xxxxxx).
        static bool is_continuation(unsigned char c);

        // ---------------------------------------------------------------------
        // Repair / conversion
        // ---------------------------------------------------------------------

        // Reinterpret ISO-8859-1 (Latin-1) as UTF-8. Every byte 0x80..0xFF maps
        // directly to the code point of the same value.
        static std::string from_latin1(std::string_view s);

        // Reinterpret Windows-1252 as UTF-8. Identical to Latin-1 except for
        // the 0x80..0x9F range (curly quotes, em-dash, euro sign, etc.).
        static std::string from_cp1252(std::string_view s);

        // Single-pass sanitizer: replaces every malformed byte/sequence with
        // U+FFFD. Lossy, but the result is guaranteed valid UTF-8.
        static std::string sanitize(std::string_view s);

        // Remove a leading UTF-8 BOM (EF BB BF) if present.
        static std::string_view strip_bom(std::string_view s);

        // ---------------------------------------------------------------------
        // Inspection / iteration
        // ---------------------------------------------------------------------

        // Number of code points (not bytes). Invalid bytes count as one each.
        static std::size_t length(std::string_view s);

        // Decode all code points. Invalid bytes become U+FFFD.
        static std::vector<std::uint32_t> code_points(std::string_view s);

        // Encode a single code point and append it to `out`.
        // Invalid code points (surrogates, > U+10FFFF) are written as U+FFFD.
        static void append_codepoint(std::string &out, std::uint32_t cp);

        static std::string encode(std::uint32_t cp);

        // ---------------------------------------------------------------------
        // Safe slicing (never cuts a code point in half)
        // ---------------------------------------------------------------------

        // Truncate to at most `max_bytes`, backing up so no multi-byte sequence
        // is split. Essential when chunking text for LLM/API calls.
        static std::string_view truncate_bytes(std::string_view s,
                                               std::size_t max_bytes);

        // Substring measured in code points rather than bytes.
        static std::string_view substr(std::string_view s,
                                       std::size_t cp_start,
                                       std::size_t cp_count = npos);


        static TextAnalysis analyze(std::string_view s);

    private:
        struct DecodeResult {
            bool valid = false;
            std::uint32_t code_point = 0;
            std::size_t length = 0; // bytes consumed when valid
            std::size_t error_index = 0; // offending byte when invalid
        };

        // Decode the code point starting at byte offset `i`.
        static DecodeResult decode_at(std::string_view s, std::size_t i);

        // Advance `count` code points from byte offset `from`; returns byte offset.
        static std::size_t advance(std::string_view s, std::size_t from,
                                   std::size_t count);

        // Windows-1252 mapping for bytes 0x80..0x9F.
        static std::uint32_t cp1252_high(unsigned char c);


        static std::size_t replacement_count(std::string_view s);

        static std::size_t control_count(std::string_view s);

        static std::size_t private_use_count(std::string_view s);

        static bool has_private_use(std::string_view s);
    };
} // namespace sc

#endif //SC_UTF8_H
