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

    std::filesystem::remove_all(directory);
    TEST_SUMMARY();
}
