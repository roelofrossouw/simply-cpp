#include "utf8.h"

namespace sc {
    std::size_t utf8::find_invalid(std::string_view s) {
        std::size_t i = 0;
        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);
            if (!r.valid) return r.error_index;
            i += r.length;
        }
        return npos;
    }

    bool utf8::is_continuation(unsigned char c) {
        return (c & 0xC0) == 0x80;
    }

    std::string utf8::from_latin1(std::string_view s) {
        std::string out;
        out.reserve(s.size() * 2);
        for (unsigned char c: s) {
            if (c < 0x80) out += static_cast<char>(c);
            else append_codepoint(out, c);
        }
        return out;
    }

    std::string utf8::from_cp1252(std::string_view s) {
        std::string out;
        out.reserve(s.size() * 2);
        for (unsigned char c: s) {
            if (c < 0x80) {
                out += static_cast<char>(c);
            } else if (c >= 0xA0) {
                append_codepoint(out, c); // same as Latin-1
            } else {
                append_codepoint(out, cp1252_high(c));
            }
        }
        return out;
    }

    std::string utf8::sanitize(std::string_view s) {
        std::string out;
        out.reserve(s.size());
        std::size_t i = 0;
        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);
            if (r.valid) {
                out.append(s.substr(i, r.length));
                i += r.length;
            } else {
                append_codepoint(out, kReplacementChar);
                ++i; // skip one byte and resynchronise
            }
        }
        return out;
    }

    std::string_view utf8::strip_bom(std::string_view s) {
        if (s.size() >= 3 &&
            static_cast<unsigned char>(s[0]) == 0xEF &&
            static_cast<unsigned char>(s[1]) == 0xBB &&
            static_cast<unsigned char>(s[2]) == 0xBF)
            return s.substr(3);
        return s;
    }

    std::size_t utf8::length(std::string_view s) {
        std::size_t count = 0, i = 0;
        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);
            i += r.valid ? r.length : 1;
            ++count;
        }
        return count;
    }

    std::vector<std::uint32_t> utf8::code_points(std::string_view s) {
        std::vector<std::uint32_t> cps;
        cps.reserve(s.size());
        std::size_t i = 0;
        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);
            if (r.valid) {
                cps.push_back(r.code_point);
                i += r.length;
            } else {
                cps.push_back(kReplacementChar);
                ++i;
            }
        }
        return cps;
    }

    void utf8::append_codepoint(std::string &out, std::uint32_t cp) {
        if ((cp >= 0xD800 && cp <= 0xDFFF) || cp > 0x10FFFF)
            cp = kReplacementChar;
        if (cp < 0x80) {
            out += static_cast<char>(cp);
        } else if (cp < 0x800) {
            out += static_cast<char>(0xC0 | (cp >> 6));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else if (cp < 0x10000) {
            out += static_cast<char>(0xE0 | (cp >> 12));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        } else {
            out += static_cast<char>(0xF0 | (cp >> 18));
            out += static_cast<char>(0x80 | ((cp >> 12) & 0x3F));
            out += static_cast<char>(0x80 | ((cp >> 6) & 0x3F));
            out += static_cast<char>(0x80 | (cp & 0x3F));
        }
    }

    std::string utf8::encode(std::uint32_t cp) {
        std::string out;
        append_codepoint(out, cp);
        return out;
    }

    std::string_view utf8::truncate_bytes(std::string_view s, std::size_t max_bytes) {
        if (s.size() <= max_bytes) return s;
        std::size_t end = max_bytes;
        while (end > 0 && is_continuation(static_cast<unsigned char>(s[end])))
            --end;
        return s.substr(0, end);
    }

    std::string_view utf8::substr(std::string_view s, std::size_t cp_start, std::size_t cp_count) {
        std::size_t byte_start = advance(s, 0, cp_start);
        std::size_t byte_end = (cp_count == npos)
                                   ? s.size()
                                   : advance(s, byte_start, cp_count);
        return s.substr(byte_start, byte_end - byte_start);
    }

    TextAnalysis utf8::analyze(std::string_view s) {
        TextAnalysis result;

        result.bytes = s.size();
        result.valid_utf8 = is_valid(s);

        std::size_t i = 0;

        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);

            std::uint32_t cp;

            if (r.valid) {
                cp = r.code_point;
                i += r.length;
            } else {
                cp = kReplacementChar;
                ++i;
            }

            ++result.codepoints;

            if (cp < 128)
                ++result.ascii;

            if (cp == kReplacementChar)
                ++result.replacement;

            if (cp == ' ' ||
                cp == '\t' ||
                cp == '\r' ||
                cp == '\n') {
                ++result.whitespace;
            }

            if (cp >= 32 && cp != 127)
                ++result.printable;

            if (cp < 32 &&
                cp != '\t' &&
                cp != '\r' &&
                cp != '\n') {
                ++result.controls;
            }

            if ((cp >= 0xE000 && cp <= 0xF8FF) ||
                (cp >= 0xF0000 && cp <= 0xFFFFD) ||
                (cp >= 0x100000 && cp <= 0x10FFFD)) {
                ++result.private_use;
            }
        }

        return result;
    }

    utf8::DecodeResult utf8::decode_at(std::string_view s, std::size_t i) {
        const auto *p = reinterpret_cast<const unsigned char *>(s.data());
        const std::size_t n = s.size();
        DecodeResult r;
        r.error_index = i;

        unsigned char c = p[i];
        std::size_t len;
        std::uint32_t cp;
        if (c < 0x80) {
            r = {true, c, 1, 0};
            return r;
        } else if ((c & 0xE0) == 0xC0) {
            len = 2;
            cp = c & 0x1Fu;
        } else if ((c & 0xF0) == 0xE0) {
            len = 3;
            cp = c & 0x0Fu;
        } else if ((c & 0xF8) == 0xF0) {
            len = 4;
            cp = c & 0x07u;
        } else return r; // stray continuation byte or invalid lead (F8..FF)

        if (i + len > n) return r; // truncated sequence
        for (std::size_t j = 1; j < len; ++j) {
            if (!is_continuation(p[i + j])) {
                r.error_index = i + j;
                return r;
            }
            cp = (cp << 6) | (p[i + j] & 0x3Fu);
        }
        if ((len == 2 && cp < 0x80) || // overlong checks
            (len == 3 && cp < 0x800) ||
            (len == 4 && cp < 0x10000) ||
            (cp >= 0xD800 && cp <= 0xDFFF) ||
            cp > 0x10FFFF)
            return r;

        r.valid = true;
        r.code_point = cp;
        r.length = len;
        return r;
    }

    std::size_t utf8::advance(std::string_view s, std::size_t from, std::size_t count) {
        std::size_t i = from;
        while (count-- && i < s.size()) {
            DecodeResult r = decode_at(s, i);
            i += r.valid ? r.length : 1;
        }
        return i;
    }

    std::uint32_t utf8::cp1252_high(unsigned char c) {
        static constexpr std::uint32_t table[32] = {
            0x20AC, 0xFFFD, 0x201A, 0x0192, 0x201E, 0x2026, 0x2020, 0x2021,
            0x02C6, 0x2030, 0x0160, 0x2039, 0x0152, 0xFFFD, 0x017D, 0xFFFD,
            0xFFFD, 0x2018, 0x2019, 0x201C, 0x201D, 0x2022, 0x2013, 0x2014,
            0x02DC, 0x2122, 0x0161, 0x203A, 0x0153, 0xFFFD, 0x017E, 0xFFFD
        };
        return table[c - 0x80];
    }

    std::size_t utf8::replacement_count(std::string_view s) {
        std::size_t count = 0;

        std::size_t i = 0;
        while (i < s.size()) {
            DecodeResult r = decode_at(s, i);

            if (r.valid) {
                if (r.code_point == kReplacementChar)
                    ++count;

                i += r.length;
            } else {
                ++count;
                ++i;
            }
        }

        return count;
    }
}
