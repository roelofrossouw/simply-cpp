// A short tour of simply-cpp's core: base64, dates and times, server endpoints, and strings and
// the environment. Each line shows a call, as written, and what it returned. Needs no server.

#include <sc.h>

#include <iostream>
#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <string_view>
#include <vector>

namespace {
    // How a result is printed: strings quoted, lists in brackets, anything else with <<.
    template<typename T>
    void print(std::ostream &out, const T &value) { out << value; }

    void print(std::ostream &out, const std::string &value) { out << '"' << value << '"'; }

    template<typename T>
    void print(std::ostream &out, const std::vector<T> &values) {
        out << '[';
        for (std::size_t i = 0; i < values.size(); ++i) {
            if (i) out << ", ";
            print(out, values[i]);
        }
        out << ']';
    }

    // One step of the demo: the call, as written in the source, and what it returned.
    template<typename T>
    void show(const std::string_view call, const T &result) {
        std::ostringstream text;
        print(text, result);
        std::cout << "  " << call << "\n      -> " << text.str() << '\n';
    }

    void heading(const std::string_view title) { std::cout << '\n' << title << '\n'; }
}

#define SHOW(expression) show(#expression, expression)

int main() {
    std::cout << "simply-cpp core: a few of the basics, each call with what it returned\n";
    sc::timer sw;

    // [readme]
    heading("Base64");
    SHOW(sc::base64::encode("Hello World!"));
    SHOW(sc::base64::decode("SGVsbG8gV29ybGQh"));

    heading("Dates and times");
    SHOW(sc::date{"2026-01-31"}.add(1, "M"));                         // calendar arithmetic
    SHOW(sc::datetime{"2026-10-09 14:30:00"}.add(90, "i").format()); // 90 minutes later
    SHOW(sc::datetime::from_unix(0).format("%d %b %Y %H:%M %Z", true));

    heading("Server endpoints");
    SHOW(sc::ip_endpoints("redis1;redis2:6380", 6379)); // ';'-separated, with a default port
    SHOW(sc::ip_endpoint::parse("[::1]:5432").host);

    heading("Strings and the environment");
    SHOW(sc::explode("a;b;;c"));
    SHOW(sc::getenv("HOME", "(not set)"));
    // [/readme]

    std::cout << "\nAll of that took " << sw << ".\n";
    return 0;
}
