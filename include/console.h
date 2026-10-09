#pragma once

#include <cstddef>
#include <iostream>
#include <optional>
#include <ostream>
#include <sstream>
#include <string>
#include <string_view>
#include <type_traits>

namespace sc {
    namespace console_detail {
        template<typename T>
        inline constexpr bool is_optional = false;
        template<typename T>
        inline constexpr bool is_optional<std::optional<T>> = true;
    }

    // Plain, readable console output, as the simply-cpp demos use it: a title, headings, and each
    // step of a program shown as written with its result below it.
    //
    //   sc::console::title("simply-cpp redis");
    //   sc::console::heading("A string value");
//   sc::console::subheading("Setting it");
    //   SC_STEP(cache.set("greeting", "Hello World!"));   //   cache.set("greeting", "Hello World!")
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

        // The program's title line, underlined.
        static void title(const std::string_view text) {
            output() << text << '\n' << std::string(display_width(text), '=') << '\n';
        }

        // A blank line, then a section heading.
        static void heading(const std::string_view text) { output() << '\n' << text << '\n'; }

        // A blank line, then a heading within a section, indented with its steps and underlined.
        static void subheading(const std::string_view text) {
            output() << "\n  " << text << "\n  " << std::string(display_width(text), '-') << '\n';
        }

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

        // How a result reads: strings quoted, bool as true/false, an empty optional as (none),
        // lists as [a, b], maps as {key: value}, anything else with <<.
        template<typename T>
        static std::string format(const T &value) {
            std::ostringstream text;
            write(text, value);
            return text.str();
        }

    private:
        inline static std::ostream *stream_ = &std::cout;

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
#define SC_SHOW(expression) ::sc::console::show(#expression, expression)

// Shows a step without a result, the statement printed as written, then runs it:
// SC_STEP(cache.set("key", "value")).
#define SC_STEP(statement) (::sc::console::step(#statement), statement)
