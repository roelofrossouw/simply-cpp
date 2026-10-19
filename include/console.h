#pragma once

#include <algorithm>
#include <cstddef>
#include <cstdlib>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>
#include <type_traits>
#include <unistd.h>

namespace sc {
    namespace console_detail {
        template<typename T>
        inline constexpr bool is_optional = false;
        template<typename T>
        inline constexpr bool is_optional<std::optional<T>> = true;
    }

    // Plain, readable console output, as the simply-cpp demos use it: a title, headings, and each
    // step of a program shown as written with its result below it. Headings are bold on a
    // terminal (not when NO_COLOR is set); everything else is plain text.
    //
    //   sc::console::title("simply-cpp redis");            //   simply-cpp redis, underlined with =
    //   sc::console::heading("A string value");            //   A string value, underlined with -
    //   sc::console::subheading("Setting it");             //     ▸ Setting it
    //   SC_STEP(cache.set("greeting", "Hello World!"));    //   cache.set("greeting", "Hello World!")
    //   SC_SHOW(cache.get("greeting"));                    //   cache.get("greeting")
    //                                                      //       -> "Hello World!"
    //
    // Results are formatted by format(): strings quoted, bool as true/false, an empty optional as
    // (none), lists as [a, b], maps as {key: value}, nested ones alike, anything else with <<.
    class console {
    public:
        // Where everything goes: std::cout unless set otherwise.
        static std::ostream &output() { return *stream_; }
        static void output(std::ostream &stream) { stream_ = &stream; }

        // The program's title line, underlined with '='.
        static void title(const std::string_view text) {
            output() << bold(text) << '\n' << std::string(display_width(text), '=') << '\n';
        }

        // A blank line, then a section heading, underlined with '-'.
        static void heading(const std::string_view text) {
            output() << '\n' << bold(text) << '\n' << std::string(display_width(text), '-') << '\n';
        }

        // A blank line, then a heading within a section, marked with '▸'.
        static void subheading(const std::string_view text) { output() << "\n  \u25B8 " << bold(text) << '\n'; }

        // An indented line of commentary.
        static void note(const std::string_view text) { output() << "  " << text << '\n'; }

        // A step without a result to show, such as a call returning void.
        static void step(const std::string_view call) { output() << "  " << call << '\n'; }

        // A step and its result, formatted by format().
        template<typename T>
        static void show(const std::string_view call, const T &result) { show_text(call, format(result)); }

        // A step and its result as plain text, such as a response body. Lines after the first are
        // indented to line up under it.
        static void show_text(const std::string_view call, const std::string_view text) {
            output() << "  " << call << "\n      -> ";
            std::size_t start = 0;
            while (true) {
                const auto end = text.find('\n', start);
                output() << text.substr(start, end == std::string_view::npos ? end : end - start) << '\n';
                if (end == std::string_view::npos || end + 1 == text.size()) break;
                start = end + 1;
                output() << "         ";
            }
        }

        // How a result reads (see above).
        template<typename T>
        static std::string format(const T &value) {
            std::ostringstream text;
            write(text, value);
            return text.str();
        }

        // Text in lines at most width characters wide, broken at spaces (a word longer than that
        // gets a line of its own); runs of spaces count as one.
        static std::vector<std::string> wrap(const std::string_view text, const std::size_t width) {
            std::vector<std::string> lines;
            std::string line;
            std::size_t at = 0;
            while (at < text.size()) {
                const auto start = text.find_first_not_of(' ', at);
                if (start == std::string_view::npos) break;
                auto end = text.find(' ', start);
                if (end == std::string_view::npos) end = text.size();
                const auto word = text.substr(start, end - start);
                if (!line.empty() && display_width(line) + 1 + display_width(word) > width) {
                    lines.push_back(line);
                    line.clear();
                }
                if (!line.empty()) line += ' ';
                line += word;
                at = end;
            }
            if (!line.empty()) lines.push_back(line);
            return lines;
        }

        // Text wrapped to width with its lines but the last spread out to fill it (spaces added
        // between words, the leftmost gaps first), as a justified paragraph; lines end in '\n'.
        static std::string justified(const std::string_view text, const std::size_t width) {
            const auto lines = wrap(text, width);
            std::string out;
            for (std::size_t i = 0; i < lines.size(); ++i) {
                const auto &line = lines[i];
                const auto gaps = static_cast<std::size_t>(std::count(line.begin(), line.end(), ' '));
                const auto room = width > display_width(line) ? width - display_width(line) : 0;
                if (i + 1 == lines.size() || gaps == 0 || room == 0) {
                    out += line + '\n';
                    continue;
                }
                std::size_t gap = 0;
                for (const char c: line) {
                    out += c;
                    if (c != ' ') continue;
                    out += std::string(room / gaps + (gap < room % gaps ? 1 : 0), ' ');
                    ++gap;
                }
                out += '\n';
            }
            return out;
        }

        // A line centred in width (left as it is when it's wider).
        static std::string centred(const std::string_view line, const std::size_t width) {
            const auto length = display_width(line);
            return std::string(length < width ? (width - length) / 2 : 0, ' ') + std::string{line};
        }

    private:
        inline static std::ostream *stream_ = &std::cout;

        // Text in bold when the output is std::cout on a terminal and NO_COLOR is not set; plain
        // otherwise, so files, pipes and captured output get no escape codes.
        static std::string bold(const std::string_view text) {
            const char *no_color = std::getenv("NO_COLOR");
            if (stream_ != &std::cout || !isatty(STDOUT_FILENO) || (no_color && *no_color)) return std::string{text};
            return "\033[1m" + std::string{text} + "\033[0m";
        }

        // Characters rather than bytes, so a UTF-8 title gets an underline of the right length.
        static std::size_t display_width(const std::string_view text) {
            std::size_t width = 0;
            for (const unsigned char c: text) width += (c & 0xC0) != 0x80;
            return width;
        }

        template<typename T>
        static void write(std::ostream &out, const T &value) {
            if constexpr (std::is_convertible_v<const T &, std::string_view>) {
                out << '"' << std::string_view{value} << '"';
            } else if constexpr (std::is_same_v<T, bool>) {
                out << (value ? "true" : "false");
            } else if constexpr (console_detail::is_optional<T>) {
                if (value) write(out, *value);
                else out << "(none)";
            } else if constexpr (requires { typename T::key_type; typename T::mapped_type; value.begin(); }) {
                out << '{';
                bool first = true;
                for (const auto &[key, mapped]: value) {
                    out << (first ? "" : ", ");
                    first = false;
                    write(out, key);
                    out << ": ";
                    write(out, mapped);
                }
                out << '}';
            } else if constexpr (requires { value.begin(); value.end(); } && !requires { out << value; }) {
                out << '[';
                bool first = true;
                for (const auto &item: value) {
                    out << (first ? "" : ", ");
                    first = false;
                    write(out, item);
                }
                out << ']';
            } else {
                out << value;
            }
        }
    };
}

// Shows a step and its result, the expression printed as written: SC_SHOW(cache.get("key")).
// Variadic, so an expression with a comma outside parentheses, sc::point{1, 2} say, still works.
#define SC_SHOW(...) ::sc::console::show(#__VA_ARGS__, __VA_ARGS__)

// Shows a step without a result, the statement printed as written, then runs it:
// SC_STEP(cache.set("key", "value")).
#define SC_STEP(...) (::sc::console::step(#__VA_ARGS__), __VA_ARGS__)
