#ifndef SC_UTF8_H
#define SC_UTF8_H
#include <string_view>
#include <string>
#include <vector>
#include <cstdint>
#include <cstddef>

namespace sc {
    // What utf8::analyze() found, counted in code points (an invalid byte counts as one U+FFFD).
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

        // Valid UTF-8 with no U+FFFD or private-use characters, and under 5% control characters.
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
        utf8() = delete;

        static constexpr std::size_t npos = static_cast<std::size_t>(-1);
        static constexpr std::uint32_t kReplacementChar = 0xFFFD;

        struct decoded {
            char32_t code_point;
            std::size_t length; // bytes
            bool valid;
        };

        /// The code point starting at byte at (at < text.size()). Lenient: an invalid byte gives
        /// U+FFFD with length 1 and valid false, so stepping by length resynchronises.
        static decoded decode(std::string_view text, std::size_t at);

        /// Index of the first invalid byte, or npos. Overlong encodings, UTF-16 surrogates and
        /// values above U+10FFFF are invalid.
        static std::size_t find_invalid(std::string_view s);

        static bool is_valid(std::string_view s) {
            return find_invalid(s) == npos;
        }

        static bool is_continuation(unsigned char c);

        /// ISO-8859-1 (Latin-1) to UTF-8.
        static std::string from_latin1(std::string_view s);

        /// Windows-1252 to UTF-8: Latin-1 but for 0x80..0x9F (curly quotes, dashes, the euro sign).
        static std::string from_cp1252(std::string_view s);

        /// Valid UTF-8, every malformed byte or sequence replaced with U+FFFD.
        static std::string sanitize(std::string_view s);

        /// Without a leading BOM (EF BB BF).
        static std::string_view strip_bom(std::string_view s);

        /// In code points; an invalid byte counts as one.
        static std::size_t length(std::string_view s);

        /// Invalid bytes become U+FFFD.
        static std::vector<std::uint32_t> code_points(std::string_view s);

        /// Invalid code points (surrogates, above U+10FFFF) are written as U+FFFD.
        static void append_codepoint(std::string &out, std::uint32_t cp);

        static std::string encode(std::uint32_t cp);

        /// At most max_bytes, never splitting a code point.
        static std::string_view truncate_bytes(std::string_view s,
                                               std::size_t max_bytes);

        /// Measured in code points; never splits one.
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

        static DecodeResult decode_at(std::string_view s, std::size_t i);

        static std::size_t advance(std::string_view s, std::size_t from,
                                   std::size_t count);

        // Windows-1252 mapping for bytes 0x80..0x9F.
        static std::uint32_t cp1252_high(unsigned char c);
    };
} // namespace sc

#endif //SC_UTF8_H
