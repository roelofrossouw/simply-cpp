#include "params.h"

#include <algorithm>
#include <cctype>
#include <charconv>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <set>

namespace {
    using json = nlohmann::ordered_json;

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
        if (!item.choices.is_null() && std::find(item.choices.begin(), item.choices.end(), value) == item.choices.end()) {
            std::string list;
            for (const auto &choice: item.choices) list += (list.empty() ? "" : ", ") + (choice.is_string() ? choice.get<std::string>() : choice.dump());
            throw sc::params::error("invalid value '" + text + "' for " + given_as + ": choose from " + list);
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

    std::string shown(const json &value) { return value.is_string() ? value.get<std::string>() : value.dump(); }
}

namespace sc {
    params::params(const int argc, const char *const argv[], nlohmann::ordered_json structure)
        : structure_(std::move(structure)) {
        std::vector<std::string> arguments;
        for (int i = 1; i < argc; ++i) arguments.emplace_back(argv[i]);
        if (structure_.is_string()) structure_ = json::parse(structure_.get<std::string>());
        program_ = structure_.is_object() && structure_.contains("name") && structure_["name"].is_string()
                       ? structure_["name"].get<std::string>()
                       : argc > 0 ? std::filesystem::path{argv[0]}.filename().string() : std::string{"program"};
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
    }

    params::params(const std::vector<std::string> &arguments, nlohmann::ordered_json structure)
        : structure_(std::move(structure)) {
        if (structure_.is_string()) structure_ = json::parse(structure_.get<std::string>());
        program_ = structure_.is_object() && structure_.contains("name") && structure_["name"].is_string()
                       ? structure_["name"].get<std::string>()
                       : std::string{"program"};
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
        if (help_requested_ || version_requested_) return finish();

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
        if (!structure.description.empty()) text += wrapped(structure.description, 0, 0) + '\n';

        // Each row: the label ("  -c, --config=FILE") and its help, the help in a column.
        struct row {
            std::string label;
            std::string help;
        };
        const auto notes = [](const parameter &item, const bool is_option) {
            std::string extra;
            if (!item.choices.is_null()) {
                std::string list;
                for (const auto &choice: item.choices) list += (list.empty() ? "" : ", ") + shown(choice);
                extra += " (one of: " + list + ')';
            }
            if (!item.fallback.is_null()) extra += " (default: " + shown(item.fallback) + ')';
            if (item.required && is_option) extra += " (required)";
            if (item.multiple) extra += " (can be given more than once)";
            return item.help.empty() ? extra.empty() ? std::string{} : extra.substr(1) : item.help + extra;
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
        if (!structure.epilog.empty()) text += '\n' + wrapped(structure.epilog, 0, 0) + '\n';
        return text;
    }

    const nlohmann::ordered_json *params::lookup(const std::string_view name) const {
        const auto found = find(std::string{name});
        return found == end() ? nullptr : &*found;
    }
}
