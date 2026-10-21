#pragma once

#include <nlohmann/json.hpp>

#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sc {
    // Command-line arguments read into an nlohmann::ordered_json, by a structure that lists the
    // parameters, in GNU style:
    //
    //     --name=value, --name value   a long option; an unambiguous prefix (--conf) is enough
    //     -n value, -nvalue            a short option
    //     -abc                         short flags together
    //     --                           everything after it is an argument, not an option
    //     -                            an argument (by convention stdin or stdout)
    //
    // Options and arguments may come in any order. -h and --help are reserved: they show help()
    // and exit. --version is reserved too when the structure has a version, and --license and
    // --license-text when the program was built with its licence (see the end of this file).
    //
    // The structure is JSON (or JSON text, which is parsed), in this form; every field but the
    // names is optional:
    //
    //     {
    //         "name": "oneapi",                  the program, for help (default: argv[0]'s name)
    //         "description": "What it does.",    shown under the usage line
    //         "version": "1.0.7",                makes --version print "oneapi 1.0.7"
    //         "epilog": "Text at the end.",
    //         "options": {                       keyed by long name, in help order
    //             "config": {
    //                 "short": "c",              a one-letter -c as well
    //                 "type": "string",          string, integer, number or boolean (a flag that
    //                                            takes no value: true when given, else false)
    //                 "default": "/etc/x.conf",  set when the option isn't given
    //                 "required": true,          an error when it isn't given
    //                 "multiple": true,          may be given again; the value is a list
    //                 "choices": ["a", "b"],     the value must be one of these (text in any
    //                                            case, given back as the choice is written)
    //                 "value": "FILE",           the placeholder in help (default: CONFIG)
    //                 "help": "read this file"
    //             }
    //         },
    //         "positional": {                    arguments in order, keyed by name
    //             "input": {"type": ..., "default": ..., "required": ..., "choices": ...,
    //                       "help": ...}         required unless it has a default or
    //                                            "required": false; optional ones come last
    //         },
    //         "extra": {                         takes any arguments after the positional ones,
    //             "files": {"type": ..., "required": ..., "help": ...}   as a list; without
    //         }                                  it, more arguments are an error
    //     }
    //
    // The values are keyed by name: args["config"], args["input"], args["files"]. Options with a
    // default, flags, multiple options and extra are always there; others only when given.
    // A mistake in the structure throws std::invalid_argument.
    //
    //     int main(int argc, char *argv[]) {
    //         const sc::params args{argc, argv, {
    //             {"description", "Reads files."},
    //             {"options", {
    //                 {"verbose", {{"short", "v"}, {"type", "boolean"}, {"help", "say more"}}},
    //                 {"count", {{"short", "n"}, {"type", "integer"}, {"default", 10}}}}},
    //             {"extra", {{"files", {{"required", true}, {"help", "files to read"}}}}}}};
    //         for (const auto &file: args["files"]) ...
    //         if (args["verbose"].get<bool>()) ...
    //         const int count = args.as<int>("count");
    //     }
    class params : public nlohmann::ordered_json {
    public:
        // A mistake in the arguments, worded for the person running the program.
        class error : public std::runtime_error {
        public:
            using std::runtime_error::runtime_error;
        };

        params() = default;

        // Reads main()'s arguments. For -h or --help it prints help() to stdout and exits 0, for
        // --version the version, for --license the licence's name and where to read it, and for
        // --license-text its text (exiting 1 when it can't be read). For a mistake it prints
        // "<name>: <mistake>" and a pointer to --help to stderr and exits 2, like the GNU tools.
        params(int argc, const char *const argv[], nlohmann::ordered_json structure);

        // Reads arguments (without the program name) and throws params::error for a mistake,
        // without printing or exiting. help_requested(), version_requested(), license_requested()
        // and license_text_requested() tell which of those was given; required parameters aren't
        // checked then.
        params(const std::vector<std::string> &arguments, nlohmann::ordered_json structure);

        [[nodiscard]] const nlohmann::ordered_json &structure() const { return structure_; }
        [[nodiscard]] const std::string &program() const { return program_; }
        [[nodiscard]] bool help_requested() const { return help_requested_; }
        [[nodiscard]] bool version_requested() const { return version_requested_; }
        [[nodiscard]] bool license_requested() const { return license_requested_; }
        [[nodiscard]] bool license_text_requested() const { return license_text_requested_; }

        // "Usage: <name> [OPTION]... INPUT [FILE]..."
        [[nodiscard]] std::string usage() const;

        // The usage line, description, arguments and options, wrapped at 80 columns.
        [[nodiscard]] std::string help() const;

        // The value of a parameter, or nullptr when it has none.
        [[nodiscard]] const nlohmann::ordered_json *lookup(std::string_view name) const;

        // The value of a parameter as a T; throws params::error naming it when there is none or
        // it isn't a T.
        template<typename T>
        [[nodiscard]] T as(const std::string_view name) const {
            const auto *value = lookup(name);
            if (!value) throw error("no value for " + std::string{name});
            return convert<T>(*value, name);
        }

        // The same, or fallback when the parameter has no value.
        template<typename T>
        [[nodiscard]] T as(const std::string_view name, const std::type_identity_t<T> &fallback) const {
            const auto *value = lookup(name);
            return value ? convert<T>(*value, name) : fallback;
        }

    private:
        nlohmann::ordered_json structure_;
        std::string program_;
        bool help_requested_ = false;
        bool version_requested_ = false;
        bool license_requested_ = false;
        bool license_text_requested_ = false;

        void parse(const std::vector<std::string> &arguments);

        template<typename T>
        static T convert(const nlohmann::ordered_json &value, const std::string_view name) {
            if constexpr (std::is_same_v<T, std::string>) {
                if (value.is_number() || value.is_boolean()) return value.dump();
            }
            try {
                return value.get<T>();
            } catch (const nlohmann::ordered_json::exception &failure) {
                throw error("invalid value for " + std::string{name} + ": " + failure.what());
            }
        }
    };
}

namespace sc::detail {
    // A program's licence, recorded when it's compiled: its name ("Apache-2.0"), where to read it,
    // where to fetch its text, and the files that may hold the text (null-terminated), such as the
    // copyright file its package installs. Trivially destructible: it may live in a static.
    struct program_license {
        const char *name = nullptr;
        const char *url = nullptr;
        const char *text_url = nullptr;
        const char *const *files = nullptr;
    };

    // Kept for sc::params' --license and --license-text. Programs don't call these: see below.
    void register_program_license(const program_license &license) noexcept;
    [[nodiscard]] const program_license *registered_program_license() noexcept;

    // The program's copyright line ("Copyright 2025-2026 Roelof Rossouw"), from its NOTICE file:
    // help shows it last, with the licence's notice, for an Apache-2.0 program.
    void register_program_copyright(const char *copyright) noexcept;
}

// The program's licence, when its build recorded one. SimplyCppFunctions.cmake's
// sc_program_info(<target>) (add_sc_example() calls it) generates a header with the licence's
// name and where to read it, and names that header in SC_PROGRAM_INFO_HEADER for that target only.
// The values are fixed when the program is compiled, as a version is. A program built without it
// has no --license or --license-text. Each file that includes params.h hands them over before
// main() runs, from an object with internal linkage, so files compiled with and without the
// header don't clash.
#ifdef SC_PROGRAM_INFO_HEADER
#include SC_PROGRAM_INFO_HEADER
namespace {
    [[maybe_unused]] const bool sc_program_license_registered = (::sc::detail::register_program_license(
        {SC_PROGRAM_LICENSE, SC_PROGRAM_LICENSE_URL, SC_PROGRAM_LICENSE_TEXT_URL, sc_program_license_files}), true);
#ifdef SC_PROGRAM_COPYRIGHT
    [[maybe_unused]] const bool sc_program_copyright_registered = (::sc::detail::register_program_copyright(SC_PROGRAM_COPYRIGHT), true);
#endif
}
#endif
