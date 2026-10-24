#include <sc.h>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <random>
#include <thread>
#include <string>
#include "sc_test.h"

using namespace std;

#define REQUIRE_SYSTEM(command) CHECK_EQ((command), 0)

namespace {
    constexpr auto probe_url = "https://www.1web.co.za/myip.php";

    string trimmed(string value) {
        while (!value.empty() && isspace(static_cast<unsigned char>(value.back()))) value.pop_back();
        return value;
    }
}

int main() {
    const auto work_dir = filesystem::temp_directory_path() / "sc-rest-test";
    filesystem::create_directories(work_dir);

    SECTION("fetch reads a local file before trying the network");
    {
        const auto path = (work_dir / "payload.txt").string();
        sc::file_put_contents(path, "hello from disk");
        CHECK(filesystem::exists(path));
        CHECK_EQ(sc::rest::fetch(path), string{"hello from disk"});
    }

    SECTION("fetch caches by url");
    {
        const auto path = (work_dir / "cached.txt").string();
        sc::file_put_contents(path, "first");
        CHECK_EQ(sc::rest::fetch(path), string{"first"});
        // The file changes but the cached answer does not.
        sc::file_put_contents(path, "second");
        CHECK_EQ(sc::file_get_contents(path), string{"second"});
        CHECK_EQ(sc::rest::fetch(path), string{"first"});
    }

    SECTION("fetch resolves a url against a base url");
    {
        const auto path = (work_dir / "based.txt").string();
        sc::file_put_contents(path, "resolved locally");
        const string base = "https://example.test/";
        CHECK_EQ(sc::rest::fetch(base + path, base), string{"resolved locally"});
    }

    SECTION("fetch refuses urls longer than 100 characters");
    {
        const string long_url = "https://example.test/" + string(100, 'a');
        CHECK_LT(size_t{100}, long_url.size());
        CHECK_EQ(sc::rest::fetch(long_url), string{});
    }

    SECTION("fetch of an empty url is empty");
    CHECK_EQ(sc::rest::fetch(""), string{});

    SECTION("A failed request is reported, not thrown");
    {
        // Port 9 is the discard port: nothing is listening, so this fails fast.
        sc::rest unreachable{"http://127.0.0.1:9/"};
        unreachable.connect_timeout(2);
        unreachable.timeout(2);
        CHECK_NOTHROW(unreachable.get());
        CHECK_EQ(unreachable.get(), string{});
        CHECK_EQ(unreachable.status(), 0L); // no response at all
        CHECK(unreachable.response_headers().empty());

        sc::rest posting{"http://127.0.0.1:9/"};
        posting.connect_timeout(2);
        posting.timeout(2);
        const auto failure = posting.post(R"({"ping": true})");
        // The POST path reports the curl error as a JSON object.
        CHECK_MSG(failure.find("\"error\"") != string::npos, "got \"" + failure + '"');
    }

    SECTION("TLS: a client certificate, a server trusted by its own certificate, or by its pinned key");
    if (std::system("openssl version > /dev/null 2>&1") != 0) {
        cout << "   (no openssl command, not verified)" << endl;
    } else {
        // A self-signed server that wants a client certificate signed by itself (the client's own,
        // self-signed too), as LXD does. Its certificate names no address, so verify_host(false).
        const auto dir = work_dir / "tls";
        filesystem::create_directories(dir);
        const auto at = [&](const char *name) { return (dir / name).string(); };
        const auto quiet = string{" > /dev/null 2>&1"};
        const auto make_pair = [&](const string &name) {
            return "openssl req -x509 -newkey ec -pkeyopt ec_paramgen_curve:P-256 -nodes -days 1 -subj /CN=sc-test-" + name +
                   " -keyout " + at((name + ".key").c_str()) + " -out " + at((name + ".crt").c_str()) + quiet;
        };
        REQUIRE_SYSTEM(std::system(make_pair("server").c_str()));
        REQUIRE_SYSTEM(std::system(make_pair("client").c_str()));
        REQUIRE_SYSTEM(std::system(make_pair("stranger").c_str()));
        REQUIRE_SYSTEM(std::system(("openssl x509 -in " + at("server.crt") + " -pubkey -noout | openssl pkey -pubin -outform der | "
                                    "openssl dgst -sha256 -binary | openssl base64 > " + at("pin.txt")).c_str()));
        const auto pin = "sha256//" + trimmed(sc::file_get_contents(at("pin.txt")));
        const int port = 18443 + static_cast<int>(std::random_device{}() % 1000);
        REQUIRE_SYSTEM(std::system(("openssl s_server -accept " + to_string(port) + " -cert " + at("server.crt") + " -key " + at("server.key") +
                                    " -Verify 1 -verify_return_error -CAfile " + at("client.crt") + " -www > " + at("server.log") + " 2>&1 & echo $! > " + at("server.pid")).c_str()));
        const auto url = "https://127.0.0.1:" + to_string(port) + "/";
        const auto client = [&](const char *certificate = "client") {
            sc::rest request{url};
            request.connect_timeout(2);
            request.timeout(5);
            request.quiet().client_certificate(at((string{certificate} + ".crt").c_str()), at((string{certificate} + ".key").c_str()));
            return request;
        };
        for (int tries = 0; tries < 50; ++tries) { // until it listens
            auto probe = client();
            (void) probe.trust(at("server.crt")).verify_host(false).get();
            if (probe.status() != 0) break;
            std::this_thread::sleep_for(std::chrono::milliseconds{100});
        }

        auto trusted = client();
        CHECK(trusted.trust(at("server.crt")).verify_host(false).get().find("s_server") != string::npos);
        CHECK_EQ(trusted.status(), 200L);
        CHECK(trusted.error().empty());

        auto named = client(); // the certificate names no 127.0.0.1
        (void) named.trust(at("server.crt")).get();
        CHECK_EQ(named.status(), 0L);
        CHECK(!named.error().empty());

        auto system_trust = client(); // not signed by anyone the system knows
        (void) system_trust.verify_host(false).get();
        CHECK_EQ(system_trust.status(), 0L);

        auto stranger = client("stranger"); // a certificate the server doesn't know
        (void) stranger.trust(at("server.crt")).verify_host(false).get();
        CHECK_EQ(stranger.status(), 0L);

        auto pinned = client(); // its key pinned, the certificate trusted too
        (void) pinned.trust(at("server.crt")).verify_host(false).pinned_public_key(pin).get();
        CHECK_EQ(pinned.status(), 200L);
        auto wrong_pin = client();
        (void) wrong_pin.trust(at("server.crt")).verify_host(false).pinned_public_key("sha256//AAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA=").get();
        CHECK_EQ(wrong_pin.status(), 0L);
        CHECK(!wrong_pin.error().empty());

        (void) std::system(("kill $(cat " + at("server.pid") + ")" + quiet).c_str());
        filesystem::remove_all(dir);
    }

    SECTION("The HTTP status of the last request");
    {
        sc::rest found{"https://raw.githubusercontent.com/roelofrossouw/simply-cpp/main/LICENSE"};
        CHECK(found.get().find("Apache License") != string::npos);
        CHECK_EQ(found.status(), 200L);
        // Its headers, by lowercased name.
        CHECK_MSG(found.response_headers().contains("content-type"), "no content-type among the response headers");
        CHECK(found.response_headers().at("content-type").starts_with("text/plain"));
        sc::rest missing{"https://raw.githubusercontent.com/roelofrossouw/simply-cpp/main/NO-SUCH-FILE"};
        (void) missing.get(); // the body is GitHub's error page
        CHECK_EQ(missing.status(), 404L);
    }

    SECTION("Headers and timeouts are configurable");
    {
        sc::rest request{probe_url};
        CHECK_NOTHROW(request.header("X-Test", "1"));
        CHECK_NOTHROW(request.bearer("not-a-real-token"));
        // timeout() returns the object so it can be chained.
        CHECK_EQ(&request.timeout(5), &request);
    }

    SECTION("Live request");
    {
        sc::rest probe{probe_url};
        probe.connect_timeout(5);
        probe.timeout(10);
        const auto body = trimmed(probe.get());
        if (body.empty()) {
            // No network on this machine: the offline checks above still stand.
            cout << "   (no network, live request not verified)" << endl;
        } else {
            // The endpoint echoes the caller's IP address.
            CHECK(!body.empty());
            CHECK_LT(body.size(), size_t{64});
            CHECK_MSG(body.find_first_not_of("0123456789.:abcdefABCDEF") == string::npos,
                      "expected an IP address, got \"" + body + '"');
            // A second request on the same object answers on its own, not appended to the first.
            CHECK_EQ(trimmed(probe.get()), body);
            CHECK_EQ(trimmed(probe.post()), body);
            // fetch() goes over the network for a url with no local file, and caches it.
            const auto fetched = trimmed(sc::rest::fetch(probe_url));
            CHECK_EQ(fetched, body);
            CHECK_EQ(trimmed(sc::rest::fetch(probe_url)), fetched);
        }
    }

    error_code ignored;
    filesystem::remove_all(work_dir, ignored);

    TEST_SUMMARY();
}
