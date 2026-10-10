#include "config.h"

#include "ip_endpoint.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>
#include <regex>
#include <set>
#include <sstream>

namespace {
    using json = nlohmann::ordered_json;

    constexpr int maximum_include_depth = 16;

    std::string_view trim(std::string_view text) {
        const auto first = text.find_first_not_of(" \t\r\n");
        if (first == std::string_view::npos) return {};
        const auto last = text.find_last_not_of(" \t\r\n");
        return text.substr(first, last - first + 1);
    }

    // '*' matches any run of characters, '?' any one.
    bool matches(const std::string_view pattern, const std::string_view name) {
        size_t p = 0, n = 0, star = std::string_view::npos, resume = 0;
        while (n < name.size()) {
            if (p < pattern.size() && (pattern[p] == '?' || pattern[p] == name[n])) {
                ++p;
                ++n;
            } else if (p < pattern.size() && pattern[p] == '*') {
                star = p++;
                resume = n;
            } else if (star != std::string_view::npos) {
                p = star + 1;
                n = ++resume;
            } else {
                return false;
            }
        }
        while (p < pattern.size() && pattern[p] == '*') ++p;
        return p == pattern.size();
    }

    // The regular files in directory whose names match pattern, in name order.
    std::vector<std::filesystem::path> matching_files(const std::filesystem::path &directory, const std::string_view pattern) {
        std::vector<std::filesystem::path> result;
        std::error_code error;
        for (const auto &entry: std::filesystem::directory_iterator{directory, error}) {
            if (entry.is_regular_file() && matches(pattern, entry.path().filename().string())) result.push_back(entry.path());
        }
        std::ranges::sort(result);
        return result;
    }

    // The .conf and .json files in directory, in name order.
    std::vector<std::filesystem::path> config_files(const std::filesystem::path &directory) {
        auto result = matching_files(directory, "*.conf");
        std::ranges::move(matching_files(directory, "*.json"), std::back_inserter(result));
        std::ranges::sort(result, {}, [](const std::filesystem::path &path) { return path.filename(); });
        return result;
    }

    // Whether text opens with '{', after whitespace and // or /* */ comments.
    bool starts_as_json(const std::string_view text) {
        size_t at = 0;
        while (at < text.size()) {
            if (std::isspace(static_cast<unsigned char>(text[at]))) {
                ++at;
            } else if (text.substr(at, 2) == "//") {
                at = text.find('\n', at);
            } else if (text.substr(at, 2) == "/*") {
                at = text.find("*/", at + 2);
                if (at != std::string_view::npos) at += 2;
            } else {
                return text[at] == '{';
            }
        }
        return false;
    }

    // Copies source into target: objects merge key by key, anything else replaces.
    void merge(json &target, const json &source) {
        if (!target.is_object() || !source.is_object()) {
            target = source;
            return;
        }
        for (const auto &[name, value]: source.items()) merge(target[name], value);
    }

    std::vector<std::string> split_key(const std::string_view key) {
        std::vector<std::string> parts;
        size_t start = 0;
        while (true) {
            const auto dot = key.find('.', start);
            parts.emplace_back(key.substr(start, dot == std::string_view::npos ? std::string_view::npos : dot - start));
            if (dot == std::string_view::npos) break;
            start = dot + 1;
        }
        return parts;
    }

    bool valid_key(const std::string_view key) {
        if (key.empty()) return false;
        for (const auto &part: split_key(key)) {
            if (part.empty()) return false;
            for (const unsigned char c: part) {
                if (!std::isalnum(c) && c != '_' && c != '-') return false;
            }
        }
        return true;
    }

    std::string unquote(const std::string_view text) {
        std::string result;
        for (size_t i = 1; i + 1 < text.size(); ++i) {
            if (text[i] == '\\' && i + 2 < text.size()) {
                switch (const char next = text[++i]) {
                    case 'n': result += '\n'; break;
                    case 't': result += '\t'; break;
                    default: result += next; // \" and \\ and anything else as itself
                }
            } else {
                result += text[i];
            }
        }
        return result;
    }

    // A boolean or number when it reads back as exactly the text written, else the text.
    json typed(const std::string_view text) {
        if (text.size() >= 2 && text.front() == '"' && text.back() == '"') return unquote(text);
        if (text == "true") return true;
        if (text == "false") return false;
        if (!text.empty() && (std::isdigit(static_cast<unsigned char>(text.front())) || text.front() == '-')) {
            const auto number = json::parse(text, nullptr, false);
            if (number.is_number() && number.dump() == text) return number;
        }
        return std::string{text};
    }

    class reader {
    public:
        reader(json &root, std::vector<std::filesystem::path> &files) : root_(root), files_(files) {
        }

        void read(const std::filesystem::path &path, const int depth) {
            if (depth > maximum_include_depth) {
                throw std::runtime_error("Configuration includes nest too deeply at " + path.string());
            }
            std::ifstream file{path};
            if (!file || std::filesystem::is_directory(path)) throw std::runtime_error("Unable to read configuration file: " + path.string());
            files_.push_back(path);
            std::stringstream input;
            input << file.rdbuf();

            const auto content = input.str();
            if (path.extension() == ".json" || starts_as_json(content)) {
                const auto parsed = json::parse(content, nullptr, false, true);
                if (parsed.is_discarded()) throw std::runtime_error(path.string() + ": invalid JSON");
                if (!parsed.is_object()) throw std::runtime_error(path.string() + ": a JSON configuration must be an object");
                merge(root_, parsed);
                return;
            }

            std::set<std::string> set_here; // a key set twice in one file is a mistake
            std::string section;
            std::string text;
            unsigned int line_number = 0;
            while (std::getline(input, text)) {
                ++line_number;
                const auto where = [&] { return path.string() + ':' + std::to_string(line_number) + ": "; };
                const auto line = trim(text);
                if (line.empty() || line.front() == '#' || line.front() == ';') continue;

                if (line.front() == '[') {
                    if (line.back() != ']') throw std::runtime_error(where() + "a section needs a closing ]");
                    section = trim(line.substr(1, line.size() - 2));
                    if (!valid_key(section)) throw std::runtime_error(where() + "invalid section name [" + section + "]");
                    continue;
                }

                const auto separator = line.find('=');
                if (separator == std::string_view::npos) {
                    if (line.starts_with("include") && line.size() > 7 && (line[7] == ' ' || line[7] == '\t')) {
                        include(path, trim(line.substr(8)), depth, where);
                        continue;
                    }
                    throw std::runtime_error(where() + "expected name = value, [section] or include <path>");
                }

                auto name = trim(line.substr(0, separator));
                const auto value = trim(line.substr(separator + 1));
                const bool append = name.ends_with("[]");
                if (append) name = trim(name.substr(0, name.size() - 2));
                if (!valid_key(name)) {
                    throw std::runtime_error(where() + "invalid key \"" + std::string{name} + "\"; use letters, digits, _ and -, with . between levels");
                }
                const std::string key = section.empty() ? std::string{name} : section + '.' + std::string{name};
                if (!append && !set_here.insert(key).second) {
                    throw std::runtime_error(where() + key + " is set twice; use " + key + "[] = ... for a list");
                }
                set(key, value, append, where);
            }
        }

    private:
        json &root_;
        std::vector<std::filesystem::path> &files_;

        template<typename Where>
        void include(const std::filesystem::path &from, const std::string_view target, const int depth, const Where &where) {
            if (target.empty()) throw std::runtime_error(where() + "include needs a path");
            std::filesystem::path path{std::string{target}};
            if (path.is_relative()) path = from.parent_path() / path;
            const auto name = path.filename().string();
            if (name.find_first_of("*?") != std::string::npos) {
                for (const auto &file: matching_files(path.parent_path(), name)) read(file, depth + 1);
            } else if (std::filesystem::is_directory(path)) {
                for (const auto &file: config_files(path)) read(file, depth + 1);
            } else {
                read(path, depth + 1);
            }
        }

        template<typename Where>
        void set(const std::string &key, const std::string_view value, const bool append, const Where &where) {
            const auto parts = split_key(key);
            json *node = &root_;
            std::string path;
            for (size_t i = 0; i + 1 < parts.size(); ++i) {
                path += (path.empty() ? "" : ".") + parts[i];
                node = &(*node)[parts[i]];
                if (!node->is_null() && !node->is_object()) {
                    throw std::runtime_error(where() + path + " has a value, so it can't also hold " + key);
                }
            }
            json &leaf = (*node)[parts.back()];
            if (leaf.is_object()) throw std::runtime_error(where() + key + " holds other keys, so it can't have a value");
            if (!append) {
                leaf = typed(value);
            } else if (value.empty()) {
                leaf = json::array();
            } else {
                if (!leaf.is_null() && !leaf.is_array()) {
                    throw std::runtime_error(where() + key + " has a single value, so [] can't add to it");
                }
                leaf.push_back(typed(value));
            }
        }
    };

    // JSON Schema keywords the checker below understands, and ones that only annotate.
    const std::set<std::string, std::less<>> schema_keywords{
        "type", "properties", "required", "additionalProperties", "default", "items", "minItems", "maxItems",
        "uniqueItems", "enum", "const", "minimum", "maximum", "exclusiveMinimum", "exclusiveMaximum", "multipleOf",
        "minLength", "maxLength", "pattern", "format"};
    const std::set<std::string, std::less<>> annotation_keywords{
        "title", "description", "$schema", "$id", "$comment", "examples"};
    const std::set<std::string, std::less<>> schema_types{
        "object", "array", "string", "integer", "number", "boolean", "null"};
    const std::set<std::string, std::less<>> schema_formats{"ipv4", "hostname", "ip-endpoint"};

    std::string join_key(const std::string &parent, const std::string &name) {
        return parent.empty() ? name : parent + '.' + name;
    }

    // Throws std::invalid_argument for anything in schema the checker can't do.
    void check_schema(const nlohmann::json &schema, const std::string &at) {
        const auto where = [&] { return "configuration structure" + (at.empty() ? std::string{} : " at " + at); };
        const auto invalid = [&](const std::string &message) { return std::invalid_argument(where() + ": " + message); };
        if (schema.is_boolean()) return;
        if (!schema.is_object()) throw invalid("a schema must be an object or a boolean");
        for (const auto &[keyword, value]: schema.items()) {
            if (annotation_keywords.contains(keyword)) continue;
            if (!schema_keywords.contains(keyword)) throw invalid("unsupported keyword \"" + keyword + '"');
            if (keyword == "type") {
                const auto types = value.is_array() ? value : nlohmann::json::array({value});
                for (const auto &type: types) {
                    if (!type.is_string() || !schema_types.contains(type.get<std::string>())) throw invalid("unknown type " + type.dump());
                }
            } else if (keyword == "properties") {
                if (!value.is_object()) throw invalid("properties must be an object");
                for (const auto &[name, property]: value.items()) check_schema(property, join_key(at, name));
            } else if (keyword == "required") {
                if (!value.is_array() && !value.is_string()) throw invalid("required must be a list of names");
                for (const auto &name: value.is_array() ? value : nlohmann::json::array({value})) {
                    if (!name.is_string()) throw invalid("required must be a list of names");
                }
            } else if (keyword == "additionalProperties" || keyword == "items") {
                check_schema(value, at + (keyword == "items" ? "[]" : ".*"));
            } else if (keyword == "enum") {
                if (!value.is_array()) throw invalid("enum must be a list");
            } else if (keyword == "uniqueItems") {
                if (!value.is_boolean()) throw invalid("uniqueItems must be true or false");
            } else if (keyword == "pattern") {
                if (!value.is_string()) throw invalid("pattern must be a string");
                try {
                    std::regex{value.get<std::string>(), std::regex::ECMAScript};
                } catch (const std::regex_error &) {
                    throw invalid("invalid pattern " + value.dump());
                }
            } else if (keyword == "format") {
                if (!value.is_string() || !schema_formats.contains(value.get<std::string>())) throw invalid("unsupported format " + value.dump());
            } else if (keyword == "minItems" || keyword == "maxItems" || keyword == "minLength" || keyword == "maxLength") {
                if (!value.is_number_unsigned() && !(value.is_number_integer() && value.get<long long>() >= 0)) {
                    throw invalid(keyword + " must be a non-negative integer");
                }
            } else if (keyword == "multipleOf") {
                if (!value.is_number() || value.get<double>() <= 0) throw invalid("multipleOf must be a positive number");
            } else if (keyword != "default" && keyword != "const" && !value.is_number()) {
                throw invalid(keyword + " must be a number");
            }
        }
    }

    // nlohmann::json and ordered_json don't compare with each other.
    nlohmann::json plain(const json &value) { return nlohmann::json::parse(value.dump()); }
    json ordered(const nlohmann::json &value) { return json::parse(value.dump()); }

    bool has_type(const json &value, const std::string &type) {
        if (type == "integer") return value.is_number_integer() || (value.is_number_float() && std::trunc(value.get<double>()) == value.get<double>());
        if (type == "number") return value.is_number();
        if (type == "string") return value.is_string();
        if (type == "boolean") return value.is_boolean();
        if (type == "object") return value.is_object();
        if (type == "array") return value.is_array();
        return value.is_null();
    }

    // UTF-8 characters, as JSON Schema counts string length.
    size_t characters(const std::string &text) {
        return static_cast<size_t>(std::ranges::count_if(text, [](const unsigned char c) { return (c & 0xC0) != 0x80; }));
    }

    bool has_format(const std::string &text, const std::string &format) {
        if (format == "ipv4") {
            static const std::regex ipv4{R"(^((25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)\.){3}(25[0-5]|2[0-4]\d|1\d\d|[1-9]?\d)$)"};
            return std::regex_match(text, ipv4);
        }
        if (format == "hostname") {
            static const std::regex hostname{R"(^(?=.{1,253}$)([A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?)(\.[A-Za-z0-9]([A-Za-z0-9-]{0,61}[A-Za-z0-9])?)*$)"};
            return std::regex_match(text, hostname);
        }
        try { // ip-endpoint
            return sc::ip_endpoint::parse(text).port != 0;
        } catch (const std::invalid_argument &) {
            return false;
        }
    }

    // Whether schema, or an object schema somewhere under it, gives a default.
    bool has_defaults(const nlohmann::json &schema) {
        if (!schema.is_object()) return false;
        if (schema.contains("default")) return true;
        if (const auto properties = schema.find("properties"); properties != schema.end()) {
            for (auto property = properties->begin(); property != properties->end(); ++property) {
                if (has_defaults(property.value())) return true;
            }
        }
        return false;
    }

    // Fills in defaults under value, adjusts what the schema allows (a number or boolean to the
    // text written where it wants a string, a single value to a list of one where it wants an
    // array), then checks value; key names it in errors.
    void apply_schema(json &value, const nlohmann::json &schema, const std::string &key) {
        const auto fail = [&](const std::string &message) {
            return std::runtime_error("Invalid configuration value for " + (key.empty() ? std::string{"the configuration"} : key) + ": " + message);
        };
        if (schema.is_boolean()) {
            if (!schema.get<bool>()) throw fail("not allowed here");
            return;
        }

        if (const auto type = schema.find("type"); type != schema.end()) {
            std::vector<std::string> types;
            for (const auto &name: type->is_array() ? *type : nlohmann::json::array({*type})) types.push_back(name.get<std::string>());
            const auto allows = [&](const std::string &name) { return std::ranges::find(types, name) != types.end(); };
            const auto matches = [&] { return std::ranges::any_of(types, [&](const std::string &name) { return has_type(value, name); }); };
            if (!matches()) {
                if (allows("string") && (value.is_number() || value.is_boolean())) value = value.dump();
                else if (allows("array") && !value.is_array() && !value.is_object()) value = json::array({value});
            }
            if (!matches()) {
                std::string expected;
                for (const auto &name: types) expected += (expected.empty() ? "" : " or ") + name;
                throw fail("expected " + expected + ", not " + (value.is_object() ? std::string{"a section"} : value.dump()));
            }
        }

        if (const auto options = schema.find("enum"); options != schema.end()) {
            const auto mine = plain(value);
            if (std::find(options->begin(), options->end(), mine) == options->end()) throw fail(value.dump() + " is not one of " + options->dump());
        }
        if (const auto constant = schema.find("const"); constant != schema.end() && plain(value) != *constant) {
            throw fail("must be " + constant->dump());
        }

        if (value.is_number()) {
            const double number = value.get<double>();
            const auto limit = [&](const char *name) { return schema.contains(name) ? schema[name].get<double>() : NAN; };
            if (const double v = limit("minimum"); !std::isnan(v) && number < v) throw fail(value.dump() + " is below the minimum " + schema["minimum"].dump());
            if (const double v = limit("maximum"); !std::isnan(v) && number > v) throw fail(value.dump() + " is above the maximum " + schema["maximum"].dump());
            if (const double v = limit("exclusiveMinimum"); !std::isnan(v) && number <= v) throw fail(value.dump() + " must be above " + schema["exclusiveMinimum"].dump());
            if (const double v = limit("exclusiveMaximum"); !std::isnan(v) && number >= v) throw fail(value.dump() + " must be below " + schema["exclusiveMaximum"].dump());
            if (const double v = limit("multipleOf"); !std::isnan(v)) {
                const double ratio = number / v;
                if (std::abs(ratio - std::round(ratio)) > 1e-9) throw fail(value.dump() + " is not a multiple of " + schema["multipleOf"].dump());
            }
        }

        if (value.is_string()) {
            const auto text = value.get<std::string>();
            if (schema.contains("minLength") && characters(text) < schema["minLength"].get<size_t>()) {
                throw fail(text.empty() ? "must not be empty" : '"' + text + "\" is shorter than " + schema["minLength"].dump() + " characters");
            }
            if (schema.contains("maxLength") && characters(text) > schema["maxLength"].get<size_t>()) {
                throw fail("longer than " + schema["maxLength"].dump() + " characters");
            }
            if (schema.contains("pattern") && !std::regex_search(text, std::regex{schema["pattern"].get<std::string>(), std::regex::ECMAScript})) {
                throw fail('"' + text + "\" doesn't match " + schema["pattern"].dump());
            }
            if (schema.contains("format") && !has_format(text, schema["format"].get<std::string>())) {
                throw fail('"' + text + "\" is not a valid " + schema["format"].get<std::string>());
            }
        }

        if (value.is_array()) {
            if (schema.contains("minItems") && value.size() < schema["minItems"].get<size_t>()) {
                throw fail(value.empty() ? "needs at least one item" : "needs at least " + schema["minItems"].dump() + " items");
            }
            if (schema.contains("maxItems") && value.size() > schema["maxItems"].get<size_t>()) {
                throw fail("has more than " + schema["maxItems"].dump() + " items");
            }
            if (const auto items = schema.find("items"); items != schema.end()) {
                for (size_t i = 0; i < value.size(); ++i) apply_schema(value[i], *items, key + '[' + std::to_string(i) + ']');
            }
            if (schema.value("uniqueItems", false)) {
                for (size_t i = 0; i < value.size(); ++i) {
                    for (size_t j = i + 1; j < value.size(); ++j) {
                        if (value[i] == value[j]) throw fail(value[i].dump() + " is listed twice");
                    }
                }
            }
        }

        if (value.is_object()) {
            std::vector<std::string> required;
            if (const auto names = schema.find("required"); names != schema.end()) {
                for (const auto &name: names->is_array() ? *names : nlohmann::json::array({*names})) required.push_back(name.get<std::string>());
            }
            // Unknown keys first: a misspelt key is the likely reason a required one is missing.
            const auto properties = schema.find("properties");
            const auto additional = schema.find("additionalProperties");
            for (auto child = value.begin(); child != value.end(); ++child) {
                if (properties != schema.end() && properties->contains(child.key())) continue;
                if (additional == schema.end()) continue;
                if (additional->is_boolean() && !additional->get<bool>()) {
                    throw std::runtime_error("Unknown configuration key: " + join_key(key, child.key()));
                }
                apply_schema(child.value(), *additional, join_key(key, child.key()));
            }
            if (properties != schema.end()) {
                for (const auto &[name, property]: properties->items()) {
                    const auto child_key = join_key(key, name);
                    if (const auto found = value.find(name); found != value.end()) {
                        apply_schema(*found, property, child_key);
                    } else if (property.is_object() && property.contains("default")) {
                        value[name] = ordered(property["default"]);
                        apply_schema(value[name], property, child_key);
                    } else if (has_defaults(property) && (!property.contains("type") || property["type"] == "object")) {
                        // A section that isn't there but has defaults under it.
                        json section = json::object();
                        apply_schema(section, property, child_key);
                        value[name] = std::move(section);
                    }
                }
            }
            for (const auto &name: required) {
                if (!value.contains(name)) throw std::runtime_error("Missing required configuration key: " + join_key(key, name));
            }
        }
    }

    void collect_keys(const json &node, const std::string &prefix, std::vector<std::string> &keys) {
        for (const auto &[name, value]: node.items()) {
            const auto key = prefix.empty() ? name : prefix + '.' + name;
            if (value.is_object()) collect_keys(value, key, keys);
            else keys.push_back(key);
        }
    }
}

namespace sc {
    config::config(const std::filesystem::path &path, nlohmann::json structure)
        : nlohmann::ordered_json(nlohmann::ordered_json::object()), structure_(std::move(structure)) {
        if (structure_.is_string()) {
            const auto text = structure_.get<std::string>();
            structure_ = nlohmann::json::parse(text, nullptr, false, true);
            if (structure_.is_discarded()) throw std::invalid_argument("configuration structure: invalid JSON");
        }
        if (!structure_.is_null()) check_schema(structure_, {});

        reader files{*this, files_};
        files.read(path, 0);
        auto drop_ins = path;
        drop_ins += ".d";
        for (const auto &file: config_files(drop_ins)) files.read(file, 0);

        if (!structure_.is_null()) apply_schema(*this, structure_, {});
    }

    std::vector<std::string> config::keys() const {
        std::vector<std::string> result;
        if (is_object()) collect_keys(*this, {}, result);
        return result;
    }

    const nlohmann::ordered_json *config::lookup(const std::string_view key) const {
        const nlohmann::ordered_json *node = this;
        for (const auto &part: split_key(key)) {
            if (!node->is_object()) return nullptr;
            const auto found = node->find(part);
            if (found == node->end()) return nullptr;
            node = &*found;
        }
        return node;
    }

    const nlohmann::ordered_json &config::required(const std::string_view key) const {
        const auto *value = lookup(key);
        if (!value) throw std::runtime_error("Missing required configuration key: " + std::string{key});
        return *value;
    }
}
