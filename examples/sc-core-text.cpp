// Text in simply-cpp core: UTF-8 that is measured, cut and repaired by code point, base64, splitting
// strings, and reading and writing files. Each line shows a call, as written, and what it returned.
// Needs no server.

#include <string>
#include <filesystem>

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: text");

    sc::console::heading("UTF-8: sc::utf8");
    const std::string greeting = "Hëllo wörld 👋";
    SC_SHOW(greeting.size());              // bytes
    SC_SHOW(sc::utf8::length(greeting));   // characters (code points)
    SC_SHOW(sc::utf8::substr(greeting, 6, 5));
    SC_SHOW(sc::utf8::truncate_bytes(greeting, 2)); // never cuts a character in half
    SC_SHOW(sc::utf8::code_points("€"));
    SC_SHOW(sc::utf8::encode(0x1F600));

    sc::console::subheading("Repairing text from older systems");
    SC_SHOW(sc::utf8::is_valid("caf\xE9"));       // Latin-1 bytes, not UTF-8
    SC_SHOW(sc::utf8::from_latin1("caf\xE9"));
    SC_SHOW(sc::utf8::from_cp1252("\x93quoted\x94")); // Windows-1252's curly quotes
    SC_SHOW(sc::utf8::sanitize("caf\xE9"));       // or replace what can't be read
    SC_SHOW(sc::utf8::analyze("plain text\n").likely_text());
    SC_SHOW(sc::utf8::analyze(std::string{"\x00\x01\x02\x03", 4}).likely_text());

    sc::console::heading("Base64: sc::base64");
    SC_SHOW(sc::base64::encode("Hello World!"));
    SC_SHOW(sc::base64::decode("SGVsbG8gV29ybGQh"));

    sc::console::heading("Strings");
    SC_SHOW(sc::explode("a;b;;c"));          // empty items are kept
    SC_SHOW(sc::explode("one, two, three", ", "));
    SC_SHOW(sc::basename("/var/log/syslog.1"));
    SC_SHOW(sc::getenv("HOME", "(not set)"));
    SC_SHOW(sc::getenv("SC_CORE_DEMO_UNSET", "the fallback"));

    sc::console::heading("Files");
    const auto path = (std::filesystem::temp_directory_path() / "sc-core-text-demo.txt").string();
    SC_STEP(sc::file_put_contents(path, "first line\n"));
    SC_STEP(sc::file_put_contents(path, "second line\n", std::ios::app));
    sc::console::show_text("sc::file_get_contents(path)", sc::file_get_contents(path));
    std::filesystem::remove(path);
    return 0;
}
