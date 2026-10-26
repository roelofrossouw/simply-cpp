#include "params.h"
#include "console.h"
#include "rest.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <optional>
#include <set>

namespace {
    using json = nlohmann::ordered_json;

    // The program's licence, once a file of it registers one (params.h): constant-initialised and
    // trivially destructible, so it's there whenever a registration runs and is never destroyed.
    sc::detail::program_license registered_license{};
    bool license_registered = false;
    const char *registered_copyright = nullptr; // "Copyright 2025-2026 Roelof Rossouw"

    // The notice the Apache License asks to be shown with a program under it: its two paragraphs
    // and the link between them.
    constexpr const char *apache_licensed = R"(Licensed under the Apache License, Version 2.0 (the "License"); you may not use this file except in compliance with the License. You may obtain a copy of the License at)";
    constexpr const char *apache_link = "http://www.apache.org/licenses/LICENSE-2.0";
    constexpr const char *apache_warranty = R"(Unless required by applicable law or agreed to in writing, software distributed under the License is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the License for the specific language governing permissions and limitations under the License.)";

    // The licence's text: the first of its files that can be read (its package's copyright file,
    // or the source's LICENSE), else fetched from where it's published; empty when neither works.
    std::string license_text(const sc::detail::program_license &license) {
        for (auto file = license.files; file && *file; ++file) {
            if (std::ifstream in{*file, std::ios::binary}; in) return {std::istreambuf_iterator<char>{in}, {}};
        }
        if (license.text_url && *license.text_url) {
            sc::rest request{license.text_url};
            auto text = request.get();
            if (request.status() == 200) return text; // not an error page's body
        }
        return {};
    }

    constexpr size_t help_width = 80;

    struct parameter {
        std::string name;
        char short_name = 0;
        std::string type = "string";
        json fallback; // the default, null when there is none
        json choices;  // null when any value goes
        bool required = false;
        bool multiple = false;
        std::string value; // the placeholder in help
        std::string help;
    };

    struct layout {
        std::string name;
        std::string description;
        std::string version;
        std::string epilog;
        std::vector<parameter> options;
        std::vector<parameter> positional;
        std::optional<parameter> extra;
    };

    std::invalid_argument invalid(const std::string &message) {
        return std::invalid_argument("parameter structure: " + message);
    }

    std::string text_field(const json &spec, const char *field, const std::string &at) {
        const auto found = spec.find(field);
        if (found == spec.end()) return {};
        if (!found->is_string()) throw invalid(at + ": " + field + " must be text");
        return found->get<std::string>();
    }

    bool flag_field(const json &spec, const char *field, const std::string &at) {
        const auto found = spec.find(field);
        if (found == spec.end()) return false;
        if (!found->is_boolean()) throw invalid(at + ": " + field + " must be true or false");
        return found->get<bool>();
    }

    bool fits_type(const json &value, const std::string &type) {
        if (type == "integer") return value.is_number_integer();
        if (type == "number") return value.is_number();
        if (type == "boolean") return value.is_boolean();
        return value.is_string();
    }

    // One option, positional argument or extra from the structure.
    parameter read_parameter(const std::string &name, const json &spec, const std::set<std::string> &fields,
                             const std::string &kind) {
        const auto at = kind + " " + name;
        if (name.empty() || !std::ranges::all_of(name, [](const unsigned char c) { return std::isalnum(c) || c == '-' || c == '_'; })) {
            throw invalid("invalid " + kind + " name \"" + name + '"');
        }
        if (!spec.is_object()) throw invalid(at + " must be an object");
        for (auto field = spec.begin(); field != spec.end(); ++field) {
            if (!fields.contains(field.key())) throw invalid(at + ": unknown field \"" + field.key() + '"');
        }
        parameter result;
        result.name = name;
        if (spec.contains("type")) {
            result.type = text_field(spec, "type", at);
            if (result.type != "string" && result.type != "integer" && result.type != "number" && result.type != "boolean") {
                throw invalid(at + ": type must be string, integer, number or boolean");
            }
        }
        if (const auto short_name = text_field(spec, "short", at); !short_name.empty()) {
            if (short_name.size() != 1 || !std::isalnum(static_cast<unsigned char>(short_name[0]))) {
                throw invalid(at + ": short must be one letter or digit");
            }
            if (short_name == "h") throw invalid(at + ": -h is reserved for --help");
            result.short_name = short_name[0];
        }
        result.required = flag_field(spec, "required", at);
        result.multiple = flag_field(spec, "multiple", at);
        result.help = text_field(spec, "help", at);
        result.value = text_field(spec, "value", at);
        if (result.value.empty()) {
            for (const char c: name) result.value += c == '-' ? '_' : static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        }
        if (const auto found = spec.find("choices"); found != spec.end()) {
            if (!found->is_array() || found->empty()) throw invalid(at + ": choices must be a list of values");
            for (const auto &choice: *found) {
                if (!fits_type(choice, result.type)) throw invalid(at + ": choice " + choice.dump() + " is not a " + result.type);
            }
            result.choices = *found;
        }
        if (const auto found = spec.find("default"); found != spec.end()) {
            if (!fits_type(*found, result.type)) throw invalid(at + ": default " + found->dump() + " is not a " + result.type);
            if (result.required) throw invalid(at + " can't be required and have a default");
            result.fallback = *found;
        }
        return result;
    }

    layout read_layout(const json &structure) {
        if (!structure.is_object()) throw invalid("must be an object");
        const std::set<std::string> top{"name", "description", "version", "epilog", "options", "positional", "extra"};
        for (auto field = structure.begin(); field != structure.end(); ++field) {
            if (!top.contains(field.key())) throw invalid("unknown field \"" + field.key() + '"');
        }
        layout result;
        result.name = text_field(structure, "name", "the structure");
        result.description = text_field(structure, "description", "the structure");
        result.version = text_field(structure, "version", "the structure");
        result.epilog = text_field(structure, "epilog", "the structure");

        std::set<std::string> names;
        std::set<char> shorts;
        const auto claim = [&](const parameter &item) {
            if (!names.insert(item.name).second) throw invalid("\"" + item.name + "\" is defined twice");
            if (item.short_name && !shorts.insert(item.short_name).second) {
                throw invalid(std::string{"-"} + item.short_name + " is defined twice");
            }
        };
        if (const auto options = structure.find("options"); options != structure.end()) {
            if (!options->is_object()) throw invalid("options must be an object keyed by name");
            for (auto option = options->begin(); option != options->end(); ++option) {
                if (option.key() == "help" || (option.key() == "version" && !result.version.empty())) {
                    throw invalid("--" + option.key() + " is reserved");
                }
                auto item = read_parameter(option.key(), option.value(),
                                           {"short", "type", "default", "required", "multiple", "choices", "value", "help"}, "option");
                if (item.type == "boolean" && (item.required || item.multiple || !item.choices.is_null())) {
                    throw invalid("option " + item.name + ": a boolean is a flag, so it can't be required, multiple or have choices");
                }
                claim(item);
                result.options.push_back(std::move(item));
            }
        }
        const std::set<std::string> argument_fields{"type", "default", "required", "choices", "value", "help"};
        if (const auto positional = structure.find("positional"); positional != structure.end()) {
            if (!positional->is_object()) throw invalid("positional must be an object keyed by name");
            bool optional_seen = false;
            for (auto argument = positional->begin(); argument != positional->end(); ++argument) {
                const auto &spec = argument.value();
                auto item = read_parameter(argument.key(), spec, argument_fields, "argument");
                if (item.type == "boolean") throw invalid("argument " + item.name + " can't be a boolean");
                // Required unless it has a default or says otherwise.
                item.required = item.fallback.is_null() && !(spec.is_object() && spec.contains("required") && !spec["required"].get<bool>());
                if (item.required && optional_seen) throw invalid("argument " + item.name + " is required but follows an optional one");
                optional_seen = optional_seen || !item.required;
                claim(item);
                result.positional.push_back(std::move(item));
            }
        }
        if (const auto extra = structure.find("extra"); extra != structure.end()) {
            if (!extra->is_object() || extra->size() != 1) throw invalid("extra must be an object with one name");
            auto item = read_parameter(extra->begin().key(), extra->begin().value(), {"type", "required", "value", "help"}, "extra");
            if (item.type == "boolean") throw invalid("extra " + item.name + " can't be a boolean");
            claim(item);
            result.extra = std::move(item);
        }
        return result;
    }

    std::string shown(const json &value) { return value.is_string() ? value.get<std::string>() : value.dump(); }

    std::string listed(const json &choices) {
        std::string list;
        for (const auto &choice: choices) list += (list.empty() ? "" : ", ") + shown(choice);
        return list;
    }

    // A parameter's help with its choices, default and the rest after it.
    std::string notes(const parameter &item, const bool is_option) {
        std::string extra;
        if (!item.choices.is_null()) extra += " (one of: " + listed(item.choices) + ')';
        if (!item.fallback.is_null()) extra += " (default: " + shown(item.fallback) + ')';
        if (item.required && is_option) extra += " (required)";
        if (item.multiple) extra += " (can be given more than once)";
        return item.help.empty() ? extra.empty() ? std::string{} : extra.substr(1) : item.help + extra;
    }

    // Parses structure when it is JSON text; returns its name, else fallback.
    std::string prepare(json &structure, std::string fallback) {
        if (structure.is_string()) structure = json::parse(structure.get<std::string>());
        if (structure.is_object() && structure.contains("name") && structure["name"].is_string()) return structure["name"].get<std::string>();
        return fallback;
    }

    // text as a value of item's type, or a params::error naming what it was given for.
    json convert_value(const parameter &item, const std::string &text, const std::string &given_as) {
        json value;
        if (item.type == "integer") {
            long long number = 0;
            const auto [end, failure] = std::from_chars(text.data(), text.data() + text.size(), number);
            if (text.empty() || failure != std::errc{} || end != text.data() + text.size()) {
                throw sc::params::error("invalid value '" + text + "' for " + given_as + ": expected a whole number");
            }
            value = number;
        } else if (item.type == "number") {
            char *end = nullptr;
            const double number = std::strtod(text.c_str(), &end);
            if (text.empty() || end != text.c_str() + text.size()) {
                throw sc::params::error("invalid value '" + text + "' for " + given_as + ": expected a number");
            }
            value = number;
        } else {
            value = text;
        }
        // Text matches a choice in any case, and becomes the choice as written ("a4" is "A4").
        if (!item.choices.is_null() && value.is_string()) {
            const auto same = [&](const json &choice) {
                if (!choice.is_string()) return false;
                const auto &a = choice.get_ref<const std::string &>();
                return a.size() == text.size() && std::equal(a.begin(), a.end(), text.begin(), [](const unsigned char x, const unsigned char y) {
                    return std::tolower(x) == std::tolower(y);
                });
            };
            if (const auto found = std::find_if(item.choices.begin(), item.choices.end(), same); found != item.choices.end()) value = *found;
        }
        if (!item.choices.is_null() && std::find(item.choices.begin(), item.choices.end(), value) == item.choices.end()) {
            throw sc::params::error("invalid value '" + text + "' for " + given_as + ": choose from " + listed(item.choices));
        }
        return value;
    }

    bool is_negative_number(const std::string &text) {
        if (text.size() < 2 || text[0] != '-') return false;
        char *end = nullptr;
        std::strtod(text.c_str(), &end);
        return end == text.c_str() + text.size();
    }

    // text wrapped to fit help_width, its lines after the first indented to indent.
    std::string wrapped(const std::string &text, const size_t indent, size_t column) {
        std::string result;
        size_t start = 0;
        while (start < text.size()) {
            auto end = text.find(' ', start);
            if (end == std::string::npos) end = text.size();
            const auto word = text.substr(start, end - start);
            if (column > indent && column + 1 + word.size() > help_width) {
                result += '\n' + std::string(indent, ' ');
                column = indent;
            } else if (column > indent) {
                result += ' ';
                ++column;
            }
            result += word;
            column += word.size();
            start = end + 1;
        }
        return result;
    }

    // Text justified to help_width, each line of it (a "\n" in it) a paragraph of its own, so a
    // link can start a line.
    std::string paragraphs(const std::string_view text) {
        std::string out;
        std::size_t start = 0;
        while (start <= text.size()) {
            auto end = text.find('\n', start);
            if (end == std::string_view::npos) end = text.size();
            const auto paragraph = text.substr(start, end - start);
            out += paragraph.empty() ? std::string{"\n"} : sc::console::justified(paragraph, help_width);
            start = end + 1;
        }
        return out;
    }

    // Text for troff: a backslash as \e and a dash that starts a word (an option, "-" for stdin) as
    // \-, a minus rather than a hyphen.
    std::string troff(const std::string_view text) {
        std::string out;
        for (std::size_t i = 0; i < text.size(); ++i) {
            const char c = text[i];
            if (c == '\\') out += "\\e";
            else if (c == '-' && (i == 0 || std::string_view{" -([|"}.find(text[i - 1]) != std::string_view::npos)) out += "\\-";
            else out += c;
        }
        return out;
    }

    // A line of text that troff mustn't read as a request (one starting with . or ').
    std::string text_line(std::string line) {
        if (line.starts_with('.') || line.starts_with('\'')) line.insert(0, "\\&");
        return line + '\n';
    }

    // Markdown's `code` and **bold** as bold, *words* as italic, the rest as troff().
    std::string marked_up(const std::string_view text) {
        std::string out;
        std::size_t plain = 0, i = 0;
        const auto span = [&](const std::string_view open, const std::string_view font) {
            if (!text.substr(i).starts_with(open)) return false;
            const auto close = text.find(open, i + open.size());
            if (close == std::string_view::npos || close == i + open.size()) return false;
            out += troff(text.substr(plain, i - plain));
            out += std::string{"\\f"} + std::string{font} + troff(text.substr(i + open.size(), close - i - open.size())) + "\\fR";
            i = plain = close + open.size();
            return true;
        };
        while (i < text.size()) {
            if (!span("`", "B") && !span("**", "B") && !(text[i] == '*' && i + 1 < text.size() && text[i + 1] != ' ' && span("*", "I"))) ++i;
        }
        return out + troff(text.substr(plain));
    }

    std::string uppercase(std::string text) {
        for (auto &c: text) c = static_cast<char>(std::toupper(static_cast<unsigned char>(c)));
        return text;
    }

    std::string trimmed(const std::string_view text) {
        const auto first = text.find_first_not_of(" \t\r");
        return first == std::string_view::npos ? std::string{} : std::string{text.substr(first, text.find_last_not_of(" \t\r") - first + 1)};
    }

    // A man page section: its name ("EXAMPLES") and its troff.
    struct man_section {
        std::string name;
        std::string body;
    };

    // man_page()'s extra: Markdown sections as troff. Text before the first heading belongs to
    // DESCRIPTION.
    std::vector<man_section> read_man_sections(const std::string_view extra) {
        std::vector<man_section> sections;
        std::string paragraph;
        bool item = false, verbatim = false;
        const auto body = [&]() -> std::string & {
            if (sections.empty()) sections.push_back({"DESCRIPTION", {}});
            return sections.back().body;
        };
        const auto finish_paragraph = [&] {
            if (!paragraph.empty()) body() += (item ? ".IP \\(bu 2\n" : ".PP\n") + text_line(marked_up(paragraph));
            paragraph.clear();
            item = false;
        };
        std::size_t start = 0;
        while (start < extra.size()) {
            auto end = extra.find('\n', start);
            if (end == std::string_view::npos) end = extra.size();
            const auto line = extra.substr(start, end - start);
            start = end + 1;
            if (verbatim) {
                if (trimmed(line).starts_with("```")) {
                    body() += ".fi\n.RE\n";
                    verbatim = false;
                } else {
                    body() += text_line(troff(line));
                }
                continue;
            }
            const auto text = trimmed(line);
            if (text.starts_with("```")) {
                finish_paragraph();
                body() += ".PP\n.RS 4\n.nf\n";
                verbatim = true;
            } else if (text.starts_with("# ")) {
                finish_paragraph();
                sections.push_back({uppercase(trimmed(text.substr(2))), {}});
            } else if (text.empty()) {
                finish_paragraph();
            } else if (text.starts_with("- ")) {
                finish_paragraph();
                paragraph = text.substr(2);
                item = true;
            } else {
                paragraph += (paragraph.empty() ? "" : " ") + text;
            }
        }
        finish_paragraph();
        if (verbatim) body() += ".fi\n.RE\n";
        return sections;
    }

    // A section heading, quoted when the name has a space ("EXIT STATUS").
    std::string section_heading(const std::string &name) {
        return name.find(' ') == std::string::npos ? ".SH " + name + '\n' : ".SH \"" + name + "\"\n";
    }

    // The day the page is dated: SOURCE_DATE_EPOCH's, for a reproducible build, else today (UTC).
    std::string man_date() {
        auto now = std::time(nullptr);
        if (const char *epoch = std::getenv("SOURCE_DATE_EPOCH"); epoch && *epoch) now = static_cast<std::time_t>(std::strtoll(epoch, nullptr, 10));
        std::tm day{};
        gmtime_r(&now, &day);
        char text[16];
        std::strftime(text, sizeof text, "%Y-%m-%d", &day);
        return text;
    }
}

namespace sc {
    params::params(const int argc, const char *const argv[], nlohmann::ordered_json structure)
        : structure_(std::move(structure)) {
        program_ = prepare(structure_, argc > 0 ? std::filesystem::path{argv[0]}.filename().string() : "program");
        // The man page, for sc_man_page() at build time; the variable may name a file of extra sections.
        if (const char *man = std::getenv("SC_PARAMS_MAN_PAGE"); man && *man) {
            std::string extra;
            if (std::ifstream file{man}; file) extra.assign(std::istreambuf_iterator<char>{file}, {});
            std::cout << man_page(extra) << std::flush;
            std::exit(0);
        }
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        try {
            parse(arguments);
        } catch (const error &mistake) {
            std::cerr << program_ << ": " << mistake.what() << "\nTry '" << program_ << " --help' for more information." << std::endl;
            std::exit(2);
        }
        if (help_requested_) {
            std::cout << help() << std::flush;
            std::exit(0);
        }
        if (version_requested_) {
            std::cout << program_ << ' ' << structure_["version"].get<std::string>() << std::endl;
            std::exit(0);
        }
        if (license_requested_) {
            std::cout << program_ << " is licensed under " << registered_license.name;
            if (registered_license.url && *registered_license.url) std::cout << ": " << registered_license.url;
            std::cout << std::endl;
            std::exit(0);
        }
        if (license_text_requested_) {
            const auto text = license_text(registered_license);
            if (text.empty()) {
                std::cerr << program_ << ": can't read the licence's text";
                if (registered_license.url && *registered_license.url) std::cerr << "; it's at " << registered_license.url;
                std::cerr << std::endl;
                std::exit(1);
            }
            std::cout << text << (text.ends_with('\n') ? "" : "\n") << std::flush;
            std::exit(0);
        }
    }

    params::params(const std::vector<std::string> &arguments, nlohmann::ordered_json structure)
        : structure_(std::move(structure)) {
        program_ = prepare(structure_, "program");
        parse(arguments);
    }

    void params::parse(const std::vector<std::string> &arguments) {
        const auto structure = read_layout(structure_);
        json values = json::object();
        // The values in the structure's order, however they were given.
        const auto finish = [&] {
            json ordered = json::object();
            for (const auto *group: {&structure.options, &structure.positional}) {
                for (const auto &item: *group) {
                    if (values.contains(item.name)) ordered[item.name] = values[item.name];
                }
            }
            if (structure.extra && values.contains(structure.extra->name)) ordered[structure.extra->name] = values[structure.extra->name];
            static_cast<json &>(*this) = std::move(ordered);
        };

        const auto find_long = [&](const std::string &name) -> const parameter & {
            for (const auto &option: structure.options) {
                if (option.name == name) return option;
            }
            std::vector<const parameter *> candidates;
            for (const auto &option: structure.options) {
                if (option.name.starts_with(name)) candidates.push_back(&option);
            }
            if (candidates.size() == 1) return *candidates.front();
            if (candidates.empty()) throw error("unrecognized option '--" + name + "'");
            std::string list;
            for (const auto *candidate: candidates) list += " '--" + candidate->name + "'";
            throw error("option '--" + name + "' is ambiguous; possibilities:" + list);
        };
        const auto set = [&](const parameter &option, const std::string &text, const std::string &given_as) {
            auto value = convert_value(option, text, given_as);
            if (option.multiple) values[option.name].push_back(std::move(value));
            else values[option.name] = std::move(value); // given again, the last one counts
        };
        const bool digit_short = std::ranges::any_of(structure.options, [](const parameter &option) {
            return std::isdigit(static_cast<unsigned char>(option.short_name));
        });

        std::vector<std::string> plain;
        bool options_done = false;
        for (size_t i = 0; i < arguments.size(); ++i) {
            const auto &argument = arguments[i];
            if (options_done || argument.size() < 2 || argument[0] != '-' || (!digit_short && is_negative_number(argument))) {
                plain.push_back(argument);
            } else if (argument == "--") {
                options_done = true;
            } else if (argument.starts_with("--")) {
                const auto equals = argument.find('=');
                const auto name = argument.substr(2, equals == std::string::npos ? std::string::npos : equals - 2);
                const bool inline_value = equals != std::string::npos;
                // --help and --version, or an abbreviation of them no option shares.
                const auto reserved = [&](const std::string_view word) {
                    return !name.empty() && word.starts_with(name) &&
                           std::ranges::none_of(structure.options, [&](const parameter &option) { return option.name.starts_with(name); });
                };
                if (reserved("help")) {
                    help_requested_ = true;
                    continue;
                }
                if (!structure.version.empty() && reserved("version")) {
                    version_requested_ = true;
                    continue;
                }
                // --license and --license-text, written out: one is the start of the other.
                if (license_registered && (name == "license" || name == "license-text") &&
                    std::ranges::none_of(structure.options, [&](const parameter &option) { return option.name == name; })) {
                    (name == "license" ? license_requested_ : license_text_requested_) = true;
                    continue;
                }
                const auto &option = find_long(name);
                const auto given_as = "'--" + option.name + "'";
                if (option.type == "boolean") {
                    if (inline_value) throw error("option " + given_as + " doesn't allow an argument");
                    values[option.name] = true;
                } else if (inline_value) {
                    set(option, argument.substr(equals + 1), given_as);
                } else if (i + 1 < arguments.size()) {
                    set(option, arguments[++i], given_as);
                } else {
                    throw error("option " + given_as + " requires an argument");
                }
            } else {
                // -abc: flags together, the last of them may take a value: -ofile or -o file.
                for (size_t j = 1; j < argument.size(); ++j) {
                    const char letter = argument[j];
                    if (letter == 'h') {
                        help_requested_ = true;
                        continue;
                    }
                    const auto option = std::ranges::find(structure.options, letter, &parameter::short_name);
                    if (option == structure.options.end()) throw error(std::string{"invalid option -- '"} + letter + "'");
                    const auto given_as = std::string{"'-"} + letter + "'";
                    if (option->type == "boolean") {
                        values[option->name] = true;
                        continue;
                    }
                    if (j + 1 < argument.size()) set(*option, argument.substr(j + 1), given_as);
                    else if (i + 1 < arguments.size()) set(*option, arguments[++i], given_as);
                    else throw error("option requires an argument -- '" + std::string{letter} + "'");
                    break;
                }
            }
        }

        // Defaults, and flags and lists that weren't given.
        for (const auto &option: structure.options) {
            if (values.contains(option.name)) continue;
            if (option.type == "boolean") values[option.name] = false;
            else if (option.multiple) values[option.name] = option.fallback.is_null() ? json::array() : json::array({option.fallback});
            else if (!option.fallback.is_null()) values[option.name] = option.fallback;
        }
        if (help_requested_ || version_requested_ || license_requested_ || license_text_requested_) return finish();

        for (const auto &option: structure.options) {
            if (option.required && (!values.contains(option.name) || (option.multiple && values[option.name].empty()))) {
                throw error("option '--" + option.name + "' is required");
            }
        }

        size_t next = 0;
        for (const auto &argument: structure.positional) {
            if (next < plain.size()) {
                values[argument.name] = convert_value(argument, plain[next++], argument.value);
            } else if (argument.required) {
                throw error("missing " + argument.value);
            } else if (!argument.fallback.is_null()) {
                values[argument.name] = argument.fallback;
            }
        }
        if (structure.extra) {
            auto &rest = values[structure.extra->name] = json::array();
            for (; next < plain.size(); ++next) rest.push_back(convert_value(*structure.extra, plain[next], structure.extra->value));
            if (structure.extra->required && rest.empty()) throw error("missing " + structure.extra->value);
        } else if (next < plain.size()) {
            throw error("extra operand '" + plain[next] + "'");
        }
        finish();
    }

    std::string params::usage() const {
        const auto structure = read_layout(structure_);
        std::string line = "Usage: " + program_ + " [OPTION]...";
        for (const auto &argument: structure.positional) line += argument.required ? ' ' + argument.value : " [" + argument.value + ']';
        if (structure.extra) line += structure.extra->required ? ' ' + structure.extra->value + "..." : " [" + structure.extra->value + "]...";
        return line;
    }

    std::string params::help() const {
        const auto structure = read_layout(structure_);
        std::string text = usage() + '\n';
        if (!structure.description.empty()) text += '\n' + paragraphs(structure.description);

        // Each row: the label ("  -c, --config=FILE") and its help, the help in a column.
        struct row {
            std::string label;
            std::string help;
        };
        std::vector<row> arguments;
        for (const auto &argument: structure.positional) arguments.push_back({"  " + argument.value, notes(argument, false)});
        if (structure.extra) arguments.push_back({"  " + structure.extra->value + "...", notes(*structure.extra, false)});
        std::vector<row> options;
        for (const auto &option: structure.options) {
            std::string label = option.short_name ? std::string{"  -"} + option.short_name + ", --" : "      --";
            label += option.name;
            if (option.type != "boolean") label += '=' + option.value;
            options.push_back({label, notes(option, true)});
        }
        options.push_back({"  -h, --help", "display this help and exit"});
        if (!structure.version.empty()) options.push_back({"      --version", "output version information and exit"});
        if (license_registered) {
            const auto mine = [&](const std::string &name) {
                return std::ranges::any_of(structure.options, [&](const parameter &option) { return option.name == name; });
            };
            if (!mine("license")) options.push_back({"      --license", std::string{"show the licence ("} + registered_license.name + ") and where to read it"});
            if (!mine("license-text")) options.push_back({"      --license-text", "output the licence's text and exit"});
        }

        size_t column = 0;
        for (const auto *rows: {&arguments, &options}) {
            for (const auto &item: *rows) column = std::max(column, item.label.size() + 2);
        }
        column = std::clamp<size_t>(column, 16, 32);
        const auto section = [&](const std::string &title, const std::vector<row> &rows) {
            if (rows.empty()) return;
            text += '\n' + title + ":\n";
            for (const auto &item: rows) {
                text += item.label;
                if (item.help.empty()) {
                    text += '\n';
                    continue;
                }
                if (item.label.size() + 2 > column) text += '\n' + std::string(column, ' ');
                else text += std::string(column - item.label.size(), ' ');
                text += wrapped(item.help, column, column) + '\n';
            }
        };
        section("Arguments", arguments);
        section("Options", options);
        if (!structure.epilog.empty()) text += '\n' + paragraphs(structure.epilog);
        // Last, an Apache-2.0 program's notice: its copyright and the link centred, the licence's
        // words justified.
        if (license_registered && registered_copyright && *registered_copyright && std::string_view{registered_license.name} == "Apache-2.0") {
            text += '\n' + sc::console::centred(registered_copyright, help_width) + "\n\n";
            text += sc::console::justified(apache_licensed, help_width) + '\n';
            text += sc::console::centred(apache_link, help_width) + "\n\n";
            text += sc::console::justified(apache_warranty, help_width);
        }
        return text;
    }

    std::string params::man_page(const std::string_view extra) const {
        const auto structure = read_layout(structure_);
        auto sections = read_man_sections(extra);
        const auto take = [&](const std::string &name) {
            std::string body;
            for (auto each = sections.begin(); each != sections.end();) {
                if (each->name == name) {
                    body += each->body;
                    each = sections.erase(each);
                } else {
                    ++each;
                }
            }
            return body;
        };

        std::string page = ".TH " + uppercase(program_) + " 1 \"" + man_date() + "\" \"" + program_ +
                           (structure.version.empty() ? "" : ' ' + structure.version) + "\" \"User Commands\"\n";

        // NAME: the description's first sentence, unless extra says it.
        if (auto name = take("NAME"); !name.empty()) {
            while (name.starts_with(".PP\n")) name.erase(0, 4); // one line: "html2pdf \- HTML files as PDFs"
            page += ".SH NAME\n" + name;
        } else {
            auto summary = structure.description.substr(0, structure.description.find('\n'));
            if (const auto stop = summary.find(". "); stop != std::string::npos) summary.resize(stop);
            if (summary.ends_with('.')) summary.pop_back();
            if (!summary.empty()) summary[0] = static_cast<char>(std::tolower(static_cast<unsigned char>(summary[0])));
            page += ".SH NAME\n" + troff(program_) + (summary.empty() ? "" : " \\- " + troff(summary)) + '\n';
        }

        page += ".SH SYNOPSIS\n.B " + troff(program_) + "\n[\\fIOPTION\\fR]...";
        for (const auto &argument: structure.positional) {
            page += argument.required ? " \\fI" + argument.value + "\\fR" : " [\\fI" + argument.value + "\\fR]";
        }
        if (structure.extra) page += structure.extra->required ? " \\fI" + structure.extra->value + "\\fR..." : " [\\fI" + structure.extra->value + "\\fR]...";
        page += '\n';

        // DESCRIPTION: the description and the epilog, a paragraph for each of their lines.
        std::string description;
        for (const auto *text: {&structure.description, &structure.epilog}) {
            std::size_t start = 0;
            while (start < text->size()) {
                auto end = text->find('\n', start);
                if (end == std::string::npos) end = text->size();
                if (const auto line = trimmed(std::string_view{*text}.substr(start, end - start)); !line.empty()) description += ".PP\n" + text_line(troff(line));
                start = end + 1;
            }
        }
        description += take("DESCRIPTION");
        if (!description.empty()) page += ".SH DESCRIPTION\n" + description;

        const auto entry = [](const std::string &label, const std::string &help) {
            return ".TP\n" + label + '\n' + (help.empty() ? std::string{} : text_line(troff(help)));
        };
        std::string arguments;
        for (const auto &argument: structure.positional) arguments += entry("\\fI" + argument.value + "\\fR", notes(argument, false));
        if (structure.extra) arguments += entry("\\fI" + structure.extra->value + "\\fR...", notes(*structure.extra, false));
        if (!arguments.empty()) page += ".SH ARGUMENTS\n" + arguments;

        page += ".SH OPTIONS\n";
        const auto dashed = [](const std::string &name) {
            std::string out;
            for (const char c: name) out += c == '-' ? std::string{"\\-"} : std::string(1, c);
            return out;
        };
        for (const auto &option: structure.options) {
            std::string label = option.short_name ? std::string{"\\fB\\-"} + option.short_name + "\\fR, " : "";
            label += "\\fB\\-\\-" + dashed(option.name) + "\\fR";
            if (option.type != "boolean") label += "=\\fI" + option.value + "\\fR";
            page += entry(label, notes(option, true));
        }
        page += entry("\\fB\\-h\\fR, \\fB\\-\\-help\\fR", "display the help and exit");
        if (!structure.version.empty()) page += entry("\\fB\\-\\-version\\fR", "output version information and exit");
        if (license_registered) {
            const auto mine = [&](const std::string &name) {
                return std::ranges::any_of(structure.options, [&](const parameter &option) { return option.name == name; });
            };
            if (!mine("license")) page += entry("\\fB\\-\\-license\\fR", std::string{"show the licence ("} + registered_license.name + ") and where to read it");
            if (!mine("license-text")) page += entry("\\fB\\-\\-license\\-text\\fR", "output the licence's text and exit");
        }
        page += take("OPTIONS");

        // extra's other sections in man's usual order; ones it doesn't know before COPYRIGHT.
        for (const std::string name: {"EXIT STATUS", "ENVIRONMENT", "FILES", "NOTES", "CAVEATS", "BUGS", "EXAMPLES", "AUTHORS", "AUTHOR", "REPORTING BUGS"}) {
            if (auto body = take(name); !body.empty()) page += section_heading(name) + body;
        }
        const auto see_also = take("SEE ALSO");
        auto copyright = take("COPYRIGHT");
        for (const auto &each: sections) {
            if (each.name != "COPYRIGHT" && each.name != "SEE ALSO") page += section_heading(each.name) + each.body;
        }
        std::string notice;
        if (registered_copyright && *registered_copyright) notice += text_line(troff(registered_copyright));
        if (license_registered) {
            notice += (notice.empty() ? "" : ".br\n") + text_line(troff(std::string{"Licensed under "} + registered_license.name +
                                                                       (registered_license.url && *registered_license.url ? std::string{": "} + registered_license.url : "") + '.'));
        }
        if (!notice.empty() || !copyright.empty()) page += ".SH COPYRIGHT\n" + notice + copyright;
        if (!see_also.empty()) page += ".SH \"SEE ALSO\"\n" + see_also;
        // A section's text starts its first paragraph itself: no .PP right after .SH.
        std::string tidy;
        std::size_t start = 0;
        bool after_heading = false;
        while (start < page.size()) {
            const auto end = page.find('\n', start);
            const auto line = std::string_view{page}.substr(start, end - start);
            if (!(after_heading && line == ".PP")) tidy += std::string{line} + '\n';
            after_heading = line.starts_with(".SH ");
            start = end + 1;
        }
        return tidy;
    }

    const nlohmann::ordered_json *params::lookup(const std::string_view name) const {
        const auto found = find(std::string{name});
        return found == end() ? nullptr : &*found;
    }
}

namespace sc::detail {
    void register_program_license(const program_license &license) noexcept {
        if (!license.name || !*license.name) return;
        registered_license = license;
        license_registered = true;
    }

    const program_license *registered_program_license() noexcept { return license_registered ? &registered_license : nullptr; }

    void register_program_copyright(const char *copyright) noexcept { registered_copyright = copyright; }
}
