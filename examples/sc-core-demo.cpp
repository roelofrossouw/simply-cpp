// A short tour of simply-cpp's core: base64, dates and times, server endpoints, and strings and
// the environment. Each line shows a call, as written, and what it returned. Needs no server.

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: a few of the basics, each call with what it returned");
    sc::timer sw;

    // [readme]
    sc::console::heading("Base64");
    SC_SHOW(sc::base64::encode("Hello World!"));
    SC_SHOW(sc::base64::decode("SGVsbG8gV29ybGQh"));

    sc::console::heading("Dates and times");
    SC_SHOW(sc::date{"2026-01-31"}.add(1, "M"));                         // calendar arithmetic
    SC_SHOW(sc::datetime{"2026-10-09 14:30:00"}.add(90, "i").format()); // 90 minutes later
    SC_SHOW(sc::datetime::from_unix(0).format("%d %b %Y %H:%M %Z", true));

    sc::console::heading("Server endpoints");
    SC_SHOW(sc::ip_endpoints("redis1;redis2:6380", 6379)); // ';'-separated, with a default port
    SC_SHOW(sc::ip_endpoint::parse("[::1]:5432").host);

    sc::console::heading("Strings and the environment");
    SC_SHOW(sc::explode("a;b;;c"));
    SC_SHOW(sc::getenv("HOME", "(not set)"));
    // [/readme]

    sc::console::output() << "\nAll of that took " << sw << ".\n";
    return 0;
}
