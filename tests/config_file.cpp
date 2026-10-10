#include <config_file.h>

#include "sc_test.h"

#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

namespace {
    const auto directory = std::filesystem::temp_directory_path() / "sc-config-file-test";

    std::filesystem::path write(const std::filesystem::path &name, const std::string &content) {
        const auto path = directory / name;
        std::filesystem::create_directories(path.parent_path());
        std::ofstream{path} << content;
        return path;
    }

    // The message sc::config throws for content, or "" when it loads.
    std::string error_for(const std::string &content) {
        try {
            sc::config{write("error.conf", content)};
            return {};
        } catch (const std::runtime_error &error) {
            return error.what();
        }
    }

    bool contains(const std::string &text, const std::string &part) { return text.find(part) != std::string::npos; }
}

int main() {
    std::filesystem::remove_all(directory);

    SECTION("Keys, sections and types");
    {
        const sc::config conf{write("basic.conf",
            "# comment\n"
            "; also a comment\n"
            "name = demo  \n"
            "server.port=8080\n"
            "[server]\n"
            "address = 0.0.0.0\n"
            "[server.tls]\n"
            "enabled = true\n"
            "[limits]\n"
            "ratio = 0.5\n"
            "offset = -3\n"
            "zip = 0123\n"
            "version = 1.50\n"
            "quoted = \"8080\"\n"
            "escaped = \"a \\\"b\\\"\\n\"\n"
            "empty =\n"
            "password = p@ss=word # not a comment\n")};
        CHECK_EQ(conf["name"].get<std::string>(), std::string{"demo"});
        CHECK_EQ(conf["server"]["port"].get<int>(), 8080);
        CHECK_EQ(conf["server"]["address"].get<std::string>(), std::string{"0.0.0.0"});
        CHECK(conf["server"]["tls"]["enabled"].get<bool>());
        CHECK_EQ(conf["limits"]["ratio"].get<double>(), 0.5);
        CHECK_EQ(conf["limits"]["offset"].get<int>(), -3);
        CHECK(conf["limits"]["zip"].is_string()); // 0123 wouldn't read back as written
        CHECK(conf["limits"]["version"].is_string());
        CHECK(conf["limits"]["quoted"].is_string());
        CHECK_EQ(conf["limits"]["escaped"].get<std::string>(), std::string{"a \"b\"\n"});
        CHECK_EQ(conf["limits"]["empty"].get<std::string>(), std::string{});
        CHECK_EQ(conf["limits"]["password"].get<std::string>(), std::string{"p@ss=word # not a comment"});
        const auto keys = conf.keys();
        CHECK_EQ(keys.size(), size_t{12});
        CHECK_EQ(keys.front(), std::string{"name"});
        CHECK_EQ(keys[2], std::string{"server.address"});
        CHECK_EQ(conf.dump(), conf.dump()); // still an ordered_json
        CHECK_EQ(conf.begin().key(), std::string{"name"});
    }

    SECTION("as<T>()");
    {
        const sc::config conf{write("as.conf", "[server]\nport = 8080\naddress = localhost\nsecure = false\n")};
        CHECK_EQ(conf.as<int>("server.port"), 8080);
        CHECK_EQ(conf.as<std::string>("server.port"), std::string{"8080"}); // the text written
        CHECK_EQ(conf.as<std::string>("server.secure"), std::string{"false"});
        CHECK_EQ(conf.as<std::string>("server.address"), std::string{"localhost"});
        CHECK_EQ(conf.as<int>("server.timeout", 30), 30);
        CHECK_EQ(conf.as<std::string>("jwt.secret", ""), std::string{});
        CHECK(conf.lookup("server.port") != nullptr);
        CHECK(conf.lookup("server.port.x") == nullptr);
        CHECK(conf.lookup("nothing") == nullptr);
        CHECK_THROWS_AS((void) conf.as<int>("server.address"), std::runtime_error);
        CHECK_THROWS_AS((void) conf.as<int>("server.missing"), std::runtime_error);
        try {
            (void) conf.as<int>("server.missing");
        } catch (const std::runtime_error &error) {
            CHECK_EQ(std::string{error.what()}, std::string{"Missing required configuration key: server.missing"});
        }
        try {
            (void) conf.as<int>("server.address");
        } catch (const std::runtime_error &error) {
            CHECK(contains(error.what(), "Invalid configuration value for server.address"));
        }
    }

    SECTION("Lists");
    {
        const sc::config conf{write("lists.conf",
            "[kafka]\n"
            "topic[] = first\n"
            "topic[] = second\n"
            "ports[] = 1\n"
            "cleared[] = gone\n"
            "cleared[] =\n")};
        CHECK_EQ(conf["kafka"]["topic"].size(), size_t{2});
        CHECK_EQ(conf["kafka"]["topic"][1].get<std::string>(), std::string{"second"});
        CHECK_EQ(conf["kafka"]["ports"][0].get<int>(), 1);
        CHECK(conf["kafka"]["cleared"].is_array() && conf["kafka"]["cleared"].empty());
        CHECK(conf.as<std::vector<std::string>>("kafka.topic") == std::vector<std::string>({"first", "second"}));
    }

    SECTION("Drop-ins and includes");
    {
        write("app.conf.d/20-local.conf", "[server]\nport = 9090\ntopic[] = extra\n");
        write("app.conf.d/10-first.conf", "[server]\nport = 9000\nname = first\n");
        write("app.conf.d/README", "not = read\n");
        write("extra/a.conf", "[extra]\nvalue = a\n");
        write("extra/b.conf", "[extra]\nvalue = b\n"); // after a.conf, so it wins
        write("extra/c.txt", "[extra]\nvalue = c\n");
        write("single.inc", "single = yes\n");
        const auto path = write("app.conf",
            "[server]\n"
            "port = 8080\n"
            "topic[] = main\n"
            "include single.inc\n"
            "after = include\n" // the include keeps the section
            "include extra\n");
        const sc::config conf{path};
        CHECK_EQ(conf.as<int>("server.port"), 9090);
        CHECK_EQ(conf.as<std::string>("server.name"), std::string{"first"});
        CHECK_EQ(conf["server"]["topic"].size(), size_t{2});
        CHECK_EQ(conf.as<std::string>("single"), std::string{"yes"});
        CHECK_EQ(conf.as<std::string>("server.after"), std::string{"include"});
        CHECK_EQ(conf.as<std::string>("extra.value"), std::string{"b"});
        CHECK(conf.lookup("not") == nullptr);
        CHECK_EQ(conf.files().size(), size_t{6});
        CHECK_EQ(conf.files().back().filename().string(), std::string{"20-local.conf"});

        const sc::config globbed{write("glob.conf", "include extra/*.txt\n")};
        CHECK_EQ(globbed.as<std::string>("extra.value"), std::string{"c"});
        const sc::config none{write("none.conf", "include extra/*.none\n")};
        CHECK(none.keys().empty());
    }

    SECTION("Mistakes are reported");
    {
        CHECK(contains(error_for("a = 1\na = 2\n"), "error.conf:2: a is set twice"));
        CHECK(contains(error_for("[s]\na = 1\n[s]\na = 2\n"), "s.a is set twice"));
        CHECK(contains(error_for("a = 1\na.b = 2\n"), "a has a value, so it can't also hold a.b"));
        CHECK(contains(error_for("a.b = 1\na = 2\n"), "a holds other keys"));
        CHECK(contains(error_for("a = 1\na[] = 2\n"), "a has a single value"));
        CHECK(contains(error_for("[open\n"), "closing ]"));
        CHECK(contains(error_for("[a..b]\n"), "invalid section name"));
        CHECK(contains(error_for("bad key = 1\n"), "invalid key \"bad key\""));
        CHECK(contains(error_for(".a = 1\n"), "invalid key"));
        CHECK(contains(error_for("no separator\n"), "error.conf:1: expected name = value"));
        CHECK(contains(error_for("include missing.conf\n"), "Unable to read configuration file"));
        CHECK(contains(error_for("include error.conf\n"), "nest too deeply"));
        CHECK(error_for("").empty());
        CHECK_THROWS_AS(sc::config{directory / "no-such-file.conf"}, std::runtime_error);
        CHECK_THROWS_AS(sc::config{directory}, std::runtime_error);
    }

    SECTION("JSON files");
    {
        write("json.conf.d/30-override.json", R"({"server": {"port": 9443}, "extra": [1, 2]})");
        write("json.conf.d/40-more.conf", "[server]\nname = drop-in\n");
        const sc::config conf{write("json.conf",
            "// a comment\n"
            R"({"server": {"port": 8443, "address": "0.0.0.0", "tls": true}, "topics": ["a", "b"]})")};
        CHECK_EQ(conf.as<int>("server.port"), 9443); // merged key by key
        CHECK_EQ(conf.as<std::string>("server.address"), std::string{"0.0.0.0"});
        CHECK_EQ(conf.as<std::string>("server.name"), std::string{"drop-in"});
        CHECK(conf.as<bool>("server.tls"));
        CHECK_EQ(conf["topics"].size(), size_t{2});
        CHECK_EQ(conf["extra"][1].get<int>(), 2);
        CHECK_EQ(conf.files().size(), size_t{3});

        const sc::config named{write("named.json", "  \n{\"a\": 1}")};
        CHECK_EQ(named.as<int>("a"), 1);
        const sc::config included{write("includes-json.conf", "include named.json\nb = 2\n")};
        CHECK_EQ(included.as<int>("a"), 1);
        CHECK(contains(error_for("{\"a\": }"), "invalid JSON"));
        try {
            sc::config{write("list.json", "[1, 2]")};
            CHECK(false);
        } catch (const std::runtime_error &error) {
            CHECK(contains(error.what(), "must be an object"));
        }
    }

    SECTION("A structure fills in defaults and checks values");
    {
        const nlohmann::json structure{
            {"type", "object"},
            {"additionalProperties", false},
            {"required", {"server"}},
            {"properties", {
                {"server", {
                    {"type", "object"},
                    {"additionalProperties", false},
                    {"required", {"endpoint"}},
                    {"properties", {
                        {"endpoint", {{"type", "string"}, {"format", "ip-endpoint"}}},
                        {"threads", {{"type", "integer"}, {"minimum", 1}, {"maximum", 64}, {"default", 4}}},
                        {"name", {{"type", "string"}, {"default", "oneapi"}}}}}}},
                {"auth", {
                    {"type", "object"},
                    {"properties", {
                        {"user", {{"type", "string"}, {"minLength", 1}, {"default", "demo"}}},
                        {"password", {{"type", "string"}, {"default", ""}}}}}}},
                {"topics", {{"type", "array"}, {"items", {{"type", "string"}, {"minLength", 1}}}, {"minItems", 1}, {"uniqueItems", true}}},
                {"level", {{"enum", {"debug", "info", "error"}}, {"default", "info"}}}}}};
        const auto path = write("schema.conf", "topics[] = a\n[server]\nendpoint = 127.0.0.1:8080\n");
        const sc::config conf{path, structure};
        CHECK_EQ(conf.as<int>("server.threads"), 4);
        CHECK_EQ(conf.as<std::string>("server.name"), std::string{"oneapi"});
        CHECK_EQ(conf.as<std::string>("auth.user"), std::string{"demo"}); // a section made for its defaults
        CHECK_EQ(conf.as<std::string>("level"), std::string{"info"});
        CHECK_EQ(conf.structure(), structure);

        // Text as well as JSON, and coercions.
        const auto text = structure.dump();
        const sc::config numbers{write("numbers.conf", "topics = single\n[server]\nendpoint = 10.0.0.1:1\n[auth]\npassword = 1234\n"), text};
        CHECK(numbers["auth"]["password"].is_string());
        CHECK_EQ(numbers.as<std::string>("auth.password"), std::string{"1234"});
        CHECK(numbers["topics"].is_array() && numbers["topics"].size() == 1);

        const auto rejected = [&](const std::string &content) {
            try {
                sc::config{write("rejected.conf", content), structure};
                return std::string{};
            } catch (const std::runtime_error &error) {
                return std::string{error.what()};
            }
        };
        const std::string valid = "topics[] = a\n[server]\nendpoint = 127.0.0.1:8080\n";
        CHECK(rejected(valid).empty());
        CHECK_EQ(rejected("topics[] = a\n"), std::string{"Missing required configuration key: server.endpoint"}); // made for its defaults
        CHECK_EQ(rejected("topics[] = a\n[server]\nthreads = 2\n"), std::string{"Missing required configuration key: server.endpoint"});
        CHECK_EQ(rejected(valid + "port = 1\n"), std::string{"Unknown configuration key: server.port"});
        CHECK_EQ(rejected(valid + "[other]\nx = 1\n"), std::string{"Unknown configuration key: other"});
        CHECK(contains(rejected(valid + "threads = 0\n"), "Invalid configuration value for server.threads: 0 is below the minimum 1"));
        CHECK(contains(rejected(valid + "threads = many\n"), "server.threads: expected integer, not \"many\""));
        CHECK(contains(rejected("topics[] = a\n[server]\nendpoint = localhost\n"), "server.endpoint: \"localhost\" is not a valid ip-endpoint"));
        CHECK(contains(rejected("topics[] =\n[server]\nendpoint = 127.0.0.1:1\n"), "topics: needs at least one item"));
        CHECK(contains(rejected("topics[] = a\ntopics[] = a\n[server]\nendpoint = 127.0.0.1:1\n"), "topics: \"a\" is listed twice"));
        CHECK(contains(rejected(valid + "[auth]\nuser =\n"), "auth.user: must not be empty"));
        CHECK(contains(rejected("level = loud\n" + valid), "level: \"loud\" is not one of"));
        CHECK(contains(rejected("topics.a = 1\n[server]\nendpoint = 127.0.0.1:1\n"), "topics: expected array, not a section"));
    }

    SECTION("More checks");
    {
        const auto check = [&](const nlohmann::json &schema, const std::string &content) {
            try {
                sc::config{write("more.conf", content), schema};
                return std::string{};
            } catch (const std::runtime_error &error) {
                return std::string{error.what()};
            }
        };
        const nlohmann::json number{{"properties", {{"n", {{"type", "number"}, {"exclusiveMinimum", 0}, {"exclusiveMaximum", 1}, {"multipleOf", 0.25}}}}}};
        CHECK(check(number, "n = 0.5\n").empty());
        CHECK(contains(check(number, "n = 0\n"), "must be above 0"));
        CHECK(contains(check(number, "n = 1\n"), "must be below 1"));
        CHECK(contains(check(number, "n = 0.3\n"), "not a multiple of 0.25"));
        const nlohmann::json strings{{"properties", {
            {"ip", {{"type", "string"}, {"format", "ipv4"}}},
            {"host", {{"type", "string"}, {"format", "hostname"}}},
            {"code", {{"type", "string"}, {"pattern", "^[A-Z]{3}$"}, {"maxLength", 3}}},
            {"mode", {{"const", "fast"}}},
            {"any", {{"type", {"integer", "string"}}}},
            {"never", false}}}};
        CHECK(check(strings, "ip = 10.0.0.255\nhost = db3.example.com\ncode = ZAF\nmode = fast\nany = 1\n").empty());
        CHECK(check(strings, "any = x\n").empty());
        CHECK(contains(check(strings, "ip = 10.0.0.256\n"), "not a valid ipv4"));
        CHECK(contains(check(strings, "host = bad_host\n"), "not a valid hostname"));
        CHECK(contains(check(strings, "code = ZA\n"), "doesn't match"));
        CHECK(contains(check(strings, "mode = slow\n"), "must be \"fast\""));
        CHECK_EQ(sc::config(write("any.conf", "any = true\n"), strings)["any"].get<std::string>(), std::string{"true"});
        CHECK(contains(check(strings, "any.x = 1\n"), "expected integer or string, not a section"));
        CHECK(contains(check(strings, "never = 1\n"), "never: not allowed here"));
        CHECK(contains(check({{"properties", {{"n", {{"type", "integer"}}}}}}, "n = 1.5\n"), "expected integer"));
    }

    SECTION("A structure the checker can't do is refused");
    {
        const auto path = write("plain.conf", "a = 1\n");
        const auto refused = [&](const nlohmann::json &schema) {
            try {
                sc::config{path, schema};
                return std::string{};
            } catch (const std::invalid_argument &error) {
                return std::string{error.what()};
            }
        };
        CHECK(refused(nullptr).empty());
        CHECK(contains(refused({{"properties", {{"a", {{"$ref", "#/x"}}}}}}), "configuration structure at a: unsupported keyword \"$ref\""));
        CHECK(contains(refused({{"oneOf", nlohmann::json::array()}}), "unsupported keyword \"oneOf\""));
        CHECK(contains(refused({{"type", "text"}}), "unknown type \"text\""));
        CHECK(contains(refused({{"format", "email"}}), "unsupported format"));
        CHECK(contains(refused({{"pattern", "("}}), "invalid pattern"));
        CHECK(contains(refused({{"minimum", "1"}}), "minimum must be a number"));
        CHECK(contains(refused(std::string{"{not json"}), "invalid JSON"));
        CHECK(refused({{"title", "x"}, {"description", "y"}, {"$schema", "https://json-schema.org/draft/2020-12/schema"}}).empty());
    }

    std::filesystem::remove_all(directory);
    TEST_SUMMARY();
}
