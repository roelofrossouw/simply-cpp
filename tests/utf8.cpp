#include <sc.h>

#include "sc_test.h"

int main() {
    using sc::utf8;
    const std::string mixed = "a\xC3\xA9\xFF" "b"; // a, é, a stray byte, b

    SECTION("decode");
    auto d = utf8::decode(mixed, 0);
    CHECK_EQ(static_cast<std::uint32_t>(d.code_point), 'a');
    CHECK_EQ(d.length, 1u);
    CHECK(d.valid);
    d = utf8::decode(mixed, 1);
    CHECK_EQ(static_cast<std::uint32_t>(d.code_point), 0xE9u);
    CHECK_EQ(d.length, 2u);
    CHECK(d.valid);
    d = utf8::decode(mixed, 3);
    CHECK_EQ(static_cast<std::uint32_t>(d.code_point), 0xFFFDu);
    CHECK_EQ(d.length, 1u);
    CHECK(!d.valid);
    d = utf8::decode("\xE2\x82", 0); // truncated
    CHECK(!d.valid);
    CHECK_EQ(d.length, 1u);

    SECTION("validity");
    CHECK(utf8::is_valid("a\xC3\xA9"));
    CHECK_EQ(utf8::find_invalid(mixed), 3u);
    CHECK_EQ(utf8::find_invalid("\xC0\x80"), 0u); // overlong

    SECTION("length, code points, sanitize");
    CHECK_EQ(utf8::length(mixed), 4u);
    CHECK((utf8::code_points(mixed) == std::vector<std::uint32_t>{'a', 0xE9, 0xFFFD, 'b'}));
    CHECK_EQ(utf8::sanitize(mixed), "a\xC3\xA9\xEF\xBF\xBD" "b");
    CHECK_EQ(std::string(utf8::substr(mixed, 1, 2)), "\xC3\xA9\xFF");
    CHECK_EQ(std::string(utf8::truncate_bytes("a\xC3\xA9", 2)), "a");

    SECTION("analyze");
    const auto analysis = utf8::analyze(mixed);
    CHECK_EQ(analysis.codepoints, 4u);
    CHECK_EQ(analysis.replacement, 1u);
    CHECK(!analysis.valid_utf8);
    CHECK(!analysis.likely_text());
    CHECK(utf8::analyze("plain text\n").likely_text());

    TEST_SUMMARY();
}
