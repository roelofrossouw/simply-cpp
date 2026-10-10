#include <params.h>

#include "sc_test.h"

#include <stdexcept>
#include <string>
#include <vector>

namespace {
    const nlohmann::ordered_json structure{
        {"name", "copy"},
        {"description", "Copies INPUT to OUTPUT, and any FILEs after it."},
        {"version", "1.2.3"},
        {"options", {
            {"config", {{"short", "c"}, {"value", "FILE"}, {"default", "/etc/copy.conf"}, {"help", "read settings from FILE"}}},
            {"verbose", {{"short", "v"}, {"type", "boolean"}, {"help", "say what is being done"}}},
            {"force", {{"short", "f"}, {"type", "boolean"}, {"help", "overwrite existing files"}}},
            {"count", {{"short", "n"}, {"type", "integer"}, {"help", "copy at most this many"}}},
            {"ratio", {{"type", "number"}, {"default", 0.5}}},
            {"mode", {{"choices", {"fast", "safe"}}, {"default", "safe"}, {"help", "how to copy"}}},
            {"define", {{"short", "D"}, {"multiple", true}, {"value", "NAME=VALUE"}, {"help", "set a variable"}}},
            {"dry-run", {{"type", "boolean"}, {"help", "show what would happen, change nothing"}}}}},
        {"positional", {
            {"input", {{"help", "the file to read"}}},
            {"output", {{"default", "-"}, {"help", "where to write"}}}}},
        {"extra", {{"file", {{"help", "more files to copy"}}}}},
        {"epilog", "Report bugs to nobody."}};

    // The message params throws for arguments, or "" when they parse.
    std::string error_for(const std::vector<std::string> &arguments, const nlohmann::ordered_json &with = structure) {
        try {
            sc::params{arguments, with};
            return {};
        } catch (const sc::params::error &error) {
            return error.what();
        }
    }

    std::string refused(const nlohmann::ordered_json &bad) {
        try {
            sc::params{std::vector<std::string>{}, bad};
            return {};
        } catch (const std::invalid_argument &error) {
            return error.what();
        }
    }

    bool contains(const std::string &text, const std::string &part) { return text.find(part) != std::string::npos; }
}

int main() {
    SECTION("Defaults");
    {
        const sc::params args{{"in.txt"}, structure};
        CHECK_EQ(args["input"].get<std::string>(), std::string{"in.txt"});
        CHECK_EQ(args["output"].get<std::string>(), std::string{"-"});
        CHECK_EQ(args["config"].get<std::string>(), std::string{"/etc/copy.conf"});
        CHECK(!args["verbose"].get<bool>());
        CHECK(!args["dry-run"].get<bool>());
        CHECK(!args.contains("count"));
        CHECK_EQ(args.as<int>("count", 7), 7);
        CHECK_EQ(args["ratio"].get<double>(), 0.5);
        CHECK_EQ(args["mode"].get<std::string>(), std::string{"safe"});
        CHECK(args["define"].is_array() && args["define"].empty());
        CHECK(args["file"].is_array() && args["file"].empty());
        CHECK(!args.help_requested());
        CHECK_EQ(args.program(), std::string{"copy"});
        CHECK_EQ(args.begin().key(), std::string{"config"}); // in the structure's order
    }

    SECTION("Every form");
    {
        const sc::params args{{"-vf", "--config=/tmp/a.conf", "in", "-n", "5", "out", "--ratio", "0.25", "-DA=1",
                               "--define", "B=2", "--mode=fast", "--dry", "x", "--", "-y", "-"}, structure};
        CHECK(args["verbose"].get<bool>());
        CHECK(args["force"].get<bool>());
        CHECK(args["dry-run"].get<bool>()); // an unambiguous prefix
        CHECK_EQ(args["config"].get<std::string>(), std::string{"/tmp/a.conf"});
        CHECK_EQ(args["count"].get<int>(), 5);
        CHECK_EQ(args.as<std::string>("count"), std::string{"5"});
        CHECK_EQ(args["ratio"].get<double>(), 0.25);
        CHECK_EQ(args["mode"].get<std::string>(), std::string{"fast"});
        CHECK(args["define"] == nlohmann::ordered_json({"A=1", "B=2"}));
        CHECK_EQ(args["input"].get<std::string>(), std::string{"in"});
        CHECK_EQ(args["output"].get<std::string>(), std::string{"out"});
        CHECK(args["file"] == nlohmann::ordered_json({"x", "-y", "-"}));

        const sc::params attached{{"-n12", "-vcfile", "in"}, structure};
        CHECK_EQ(attached["count"].get<int>(), 12);
        CHECK_EQ(attached["config"].get<std::string>(), std::string{"file"});
        CHECK(attached["verbose"].get<bool>());

        const sc::params last{{"-n", "1", "--count", "2", "in"}, structure};
        CHECK_EQ(last["count"].get<int>(), 2); // given again, the last one counts

        const sc::params negative{{"-n", "-3", "-4"}, structure};
        CHECK_EQ(negative["count"].get<int>(), -3);
        CHECK_EQ(negative["input"].get<std::string>(), std::string{"-4"}); // a number, not an option
    }

    SECTION("Help and version");
    {
        const sc::params help{{"--help"}, structure};
        CHECK(help.help_requested());
        CHECK(sc::params({"-h"}, structure).help_requested());
        CHECK(sc::params({"-vh"}, structure).help_requested());
        CHECK(sc::params({"--he"}, structure).help_requested());
        CHECK(sc::params({"--version"}, structure).version_requested());
        CHECK(!sc::params({"--help"}, structure).version_requested());

        const auto text = help.help();
        CHECK_EQ(help.usage(), std::string{"Usage: copy [OPTION]... INPUT [OUTPUT] [FILE]..."});
        CHECK(text.starts_with("Usage: copy [OPTION]... INPUT [OUTPUT] [FILE]...\nCopies INPUT to OUTPUT"));
        CHECK(contains(text, "\nArguments:\n  INPUT"));
        CHECK(!contains(text, "the file to read (required)"));
        CHECK(contains(text, "  OUTPUT"));
        CHECK(contains(text, "where to write (default: -)"));
        CHECK(contains(text, "  FILE..."));
        CHECK(contains(text, "\nOptions:\n  -c, --config=FILE"));
        CHECK(contains(text, "read settings from FILE (default: /etc/copy.conf)"));
        CHECK(contains(text, "  -v, --verbose"));
        CHECK(contains(text, "      --ratio=RATIO"));
        CHECK(contains(text, "how to copy (one of: fast, safe) (default: safe)"));
        CHECK(contains(text, "(can be given more than once)"));
        CHECK(contains(text, "  -h, --help"));
        CHECK(contains(text, "      --version"));
        CHECK(text.ends_with("\nReport bugs to nobody.\n"));
        size_t longest = 0, start = 0;
        while (start < text.size()) {
            const auto end = text.find('\n', start);
            longest = std::max(longest, end - start);
            start = end + 1;
        }
        CHECK_LT(longest, size_t{81});
    }

    SECTION("Mistakes are reported");
    {
        CHECK_EQ(error_for({}), std::string{"missing INPUT"});
        CHECK_EQ(error_for({"in", "--nope"}), std::string{"unrecognized option '--nope'"});
        CHECK_EQ(error_for({"in", "-x"}), std::string{"invalid option -- 'x'"});
        CHECK_EQ(error_for({"in", "--config"}), std::string{"option '--config' requires an argument"});
        CHECK_EQ(error_for({"in", "-c"}), std::string{"option requires an argument -- 'c'"});
        CHECK_EQ(error_for({"in", "--verbose=yes"}), std::string{"option '--verbose' doesn't allow an argument"});
        CHECK_EQ(error_for({"in", "-n", "five"}), std::string{"invalid value 'five' for '-n': expected a whole number"});
        CHECK_EQ(error_for({"in", "--ratio=half"}), std::string{"invalid value 'half' for '--ratio': expected a number"});
        CHECK_EQ(error_for({"in", "--mode=slow"}), std::string{"invalid value 'slow' for '--mode': choose from fast, safe"});
        CHECK(contains(error_for({"in", "--d"}), "option '--d' is ambiguous; possibilities: '--define' '--dry-run'"));

        const nlohmann::ordered_json strict{
            {"options", {{"port", {{"type", "integer"}, {"required", true}}}}},
            {"positional", {{"source", nlohmann::ordered_json::object()}}}};
        CHECK_EQ(error_for({"a"}, strict), std::string{"option '--port' is required"});
        CHECK_EQ(error_for({"--port=1", "a", "b"}, strict), std::string{"extra operand 'b'"});
        CHECK(error_for({"--port=1", "a"}, strict).empty());
        CHECK(error_for({"--help"}, strict).empty()); // nothing is required with --help
        CHECK_EQ(error_for({"--port=1", "a", "--version"}, strict), std::string{"unrecognized option '--version'"});

        const nlohmann::ordered_json some{{"extra", {{"files", {{"required", true}, {"type", "integer"}}}}}};
        CHECK_EQ(error_for({}, some), std::string{"missing FILES"});
        CHECK_EQ(error_for({"1", "x"}, some), std::string{"invalid value 'x' for FILES: expected a whole number"});
        CHECK(sc::params({"1", "2"}, some)["files"] == nlohmann::ordered_json({1, 2}));
    }

    SECTION("JSON text");
    {
        const sc::params args{{"-q", "a"}, std::string{R"({
            "options": {"quiet": {"short": "q", "type": "boolean"}},
            "positional": {"name": {}}
        })"}};
        CHECK(args["quiet"].get<bool>());
        CHECK_EQ(args["name"].get<std::string>(), std::string{"a"});
        CHECK_EQ(args.usage(), std::string{"Usage: program [OPTION]... NAME"});
    }

    SECTION("A structure with mistakes is refused");
    {
        CHECK(contains(refused({{"option", nlohmann::ordered_json::object()}}), "unknown field \"option\""));
        CHECK(contains(refused({{"options", {{"x", {{"shrt", "x"}}}}}}), "option x: unknown field \"shrt\""));
        CHECK(contains(refused({{"options", {{"help", nlohmann::ordered_json::object()}}}}), "--help is reserved"));
        CHECK(contains(refused({{"version", "1"}, {"options", {{"version", nlohmann::ordered_json::object()}}}}), "--version is reserved"));
        CHECK(refused({{"options", {{"version", nlohmann::ordered_json::object()}}}}).empty()); // free without a version
        CHECK(contains(refused({{"options", {{"x", {{"short", "h"}}}}}}), "-h is reserved"));
        CHECK(contains(refused({{"options", {{"x", {{"short", "xy"}}}}}}), "short must be one letter"));
        CHECK(contains(refused({{"options", {{"x", {{"short", "a"}}}, {"y", {{"short", "a"}}}}}}), "-a is defined twice"));
        CHECK(contains(refused({{"options", {{"x", {{"type", "text"}}}}}}), "type must be"));
        CHECK(contains(refused({{"options", {{"x", {{"type", "integer"}, {"default", "1"}}}}}}), "default \"1\" is not a integer"));
        CHECK(contains(refused({{"options", {{"x", {{"required", true}, {"default", "1"}}}}}}), "can't be required and have a default"));
        CHECK(contains(refused({{"options", {{"x", {{"type", "boolean"}, {"required", true}}}}}}), "a boolean is a flag"));
        CHECK(contains(refused({{"options", {{"bad name", nlohmann::ordered_json::object()}}}}), "invalid option name"));
        CHECK(contains(refused({{"positional", {{"a", {{"default", "x"}}}, {"b", nlohmann::ordered_json::object()}}}}), "argument b is required but follows an optional one"));
        CHECK(contains(refused({{"options", {{"a", nlohmann::ordered_json::object()}}}, {"positional", {{"a", nlohmann::ordered_json::object()}}}}), "\"a\" is defined twice"));
        CHECK(contains(refused({{"extra", {{"a", nlohmann::ordered_json::object()}, {"b", nlohmann::ordered_json::object()}}}}), "extra must be an object with one name"));
    }

    TEST_SUMMARY();
}
