#include <sc.h>
#include <filesystem>
#include <string>
#include "sc_test.h"

using namespace std;

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

    SECTION("A failed request is reported, not thrown");
    {
        // Port 9 is the discard port: nothing is listening, so this fails fast.
        sc::rest unreachable{"http://127.0.0.1:9/"};
        unreachable.connect_timeout(2);
        unreachable.timeout(2);
        CHECK_NOTHROW(unreachable.get());
        CHECK_EQ(unreachable.get(), string{});

        sc::rest posting{"http://127.0.0.1:9/"};
        posting.connect_timeout(2);
        posting.timeout(2);
        const auto failure = posting.post(R"({"ping": true})");
        // The POST path reports the curl error as a JSON object.
        CHECK_MSG(failure.find("\"error\"") != string::npos, "got \"" + failure + '"');
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
