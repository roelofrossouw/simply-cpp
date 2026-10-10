#pragma once

#include <nlohmann/json.hpp>

#include <filesystem>
#include <stdexcept>
#include <string>
#include <string_view>
#include <type_traits>
#include <vector>

namespace sc {
    // A configuration file read into an nlohmann::ordered_json, in the order it was written:
    //
    //     # comment            (or ; comment)
    //     name = value         spaces around '=' and the value are trimmed
    //     server.port = 8080   a '.' nests the key: conf["server"]["port"]
    //     [server]             later keys go in this section: port = 8080 is server.port
    //     [server.tls]         sections nest the same way
    //     topic[] = first      [] appends to a list; "topic[] =" with no value empties it
    //     include extra.conf   reads another file here, relative to this one; a directory or
    //                          a '*' in the name reads every match (*.conf in a directory) in
    //                          name order
    //
    // After the file, every *.conf in "<file>.d/" (so oneapi.conf.d/) is read in name order,
    // the Linux drop-in convention for local changes that survive package upgrades. A key set
    // again in a later file replaces the earlier value; set twice in one file it is an error.
    //
    // Values are typed when that loses nothing: true and false are booleans, and numbers that
    // read back exactly as written (8080, -1, 0.5, but not 0123 or 1.50) are numbers. Anything
    // else, and anything in double quotes ("8080", with \" \\ \n \t escapes), is a string.
    // as<std::string>() gives a number or boolean back as the text that was written.
    //
    //     const sc::config conf{"/etc/oneapi/oneapi.conf"};
    //     const int port = conf["server"]["port"].get<int>();     // nlohmann::json access
    //     const int port = conf.as<int>("server.port");           // errors name the key
    //     const auto secret = conf.as<std::string>("jwt.secret", ""); // with a default
    //
    // Errors are std::runtime_error, naming the file and line or the key.
    class config : public nlohmann::ordered_json {
    public:
        config() = default;

        explicit config(const std::filesystem::path &path);

        // Every file read, in the order read: the file, its includes and its drop-ins.
        [[nodiscard]] const std::vector<std::filesystem::path> &files() const { return files_; }

        // Every value's dotted key ("server.port"), in file order. A list is one key.
        [[nodiscard]] std::vector<std::string> keys() const;

        // The value at a dotted key, or nullptr when there is none.
        [[nodiscard]] const nlohmann::ordered_json *lookup(std::string_view key) const;

        // The value at a dotted key; throws when there is none.
        [[nodiscard]] const nlohmann::ordered_json &required(std::string_view key) const;

        // The value at a dotted key as a T; throws, naming the key, when there is none or it
        // isn't a T.
        template<typename T>
        [[nodiscard]] T as(const std::string_view key) const {
            return convert<T>(required(key), key);
        }

        // The same, or fallback when the key isn't set.
        template<typename T>
        [[nodiscard]] T as(const std::string_view key, const std::type_identity_t<T> &fallback) const {
            const auto *value = lookup(key);
            return value ? convert<T>(*value, key) : fallback;
        }

    private:
        std::vector<std::filesystem::path> files_;

        template<typename T>
        static T convert(const nlohmann::ordered_json &value, const std::string_view key) {
            if constexpr (std::is_same_v<T, std::string>) {
                // Only lossless numbers and booleans are typed, so dump() is the text written.
                if (value.is_number() || value.is_boolean()) return value.dump();
            }
            try {
                return value.get<T>();
            } catch (const nlohmann::ordered_json::exception &error) {
                throw std::runtime_error("Invalid configuration value for " + std::string(key) + ": " + error.what());
            }
        }
    };
}
