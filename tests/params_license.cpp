// sc::params with the licence sc_program_info() records at compile time (tests/CMakeLists.txt
// builds this test with it, as add_sc_example() builds demos): --license and --license-text.
#include <params.h>

#include "sc_test.h"

#include <string>

namespace {
    const nlohmann::ordered_json structure{
        {"name", "licensed"},
        {"options", {{"count", {{"short", "n"}, {"type", "integer"}, {"default", 1}}}}}};

    bool contains(const std::string &text, const std::string &part) { return text.find(part) != std::string::npos; }
}

int main() {
    SECTION("The licence is recorded when the program is compiled");
    const auto *license = sc::detail::registered_program_license();
    CHECK(license != nullptr);
    if (!license) TEST_SUMMARY();
    CHECK_EQ(std::string{license->name}, std::string{SC_PROGRAM_LICENSE});
    CHECK_EQ(std::string{license->name}, std::string{"Apache-2.0"}); // sc-core's LICENSE
    CHECK(contains(license->url, "LICENSE") || contains(license->url, "spdx.org"));

    SECTION("--license and --license-text are reserved, and in --help");
    {
        const sc::params plain({"-n", "3"}, structure);
        CHECK(!plain.license_requested());
        CHECK(!plain.license_text_requested());
        CHECK(sc::params({"--license"}, structure).license_requested());
        CHECK(sc::params({"--license-text"}, structure).license_text_requested());
        const auto help = plain.help();
        CHECK(contains(help, "--license "));
        CHECK(contains(help, "show the licence (Apache-2.0)"));
        CHECK(contains(help, "--license-text"));
    }

    SECTION("A program's own --license option wins");
    {
        const nlohmann::ordered_json own{{"options", {{"license", {{"help", "the licence key to use"}}}}}};
        const sc::params args({"--license", "ABC-123"}, own);
        CHECK(!args.license_requested());
        CHECK_EQ(args.as<std::string>("license"), std::string{"ABC-123"});
        CHECK(!contains(args.help(), "show the licence"));
    }

    SECTION("The text comes from a file: here, the source's LICENSE");
    {
        bool readable = false;
        for (auto file = license->files; file && *file; ++file) readable = readable || std::string{*file}.ends_with("/LICENSE");
        CHECK(readable);
    }

    TEST_SUMMARY();
}
