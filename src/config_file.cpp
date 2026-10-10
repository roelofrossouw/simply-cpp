#include "config_file.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <set>

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
            std::ifstream input{path};
            if (!input || std::filesystem::is_directory(path)) throw std::runtime_error("Unable to read configuration file: " + path.string());
            files_.push_back(path);

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
                for (const auto &file: matching_files(path, "*.conf")) read(file, depth + 1);
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

    void collect_keys(const json &node, const std::string &prefix, std::vector<std::string> &keys) {
        for (const auto &[name, value]: node.items()) {
            const auto key = prefix.empty() ? name : prefix + '.' + name;
            if (value.is_object()) collect_keys(value, key, keys);
            else keys.push_back(key);
        }
    }
}

namespace sc {
    config::config(const std::filesystem::path &path) : nlohmann::ordered_json(nlohmann::ordered_json::object()) {
        reader files{*this, files_};
        files.read(path, 0);
        auto drop_ins = path;
        drop_ins += ".d";
        for (const auto &file: matching_files(drop_ins, "*.conf")) files.read(file, 0);
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
