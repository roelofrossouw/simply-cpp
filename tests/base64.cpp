#include <sc.h>
#undef NDEBUG // Allow tests in release
#include <cassert>
#include <iostream>
#include <stdexcept>
#include <string>
#include <string_view>


using namespace std;

static void test_vector(const std::string_view plain, const std::string_view expected) {
    const auto encoded = sc::base64::encode(plain);
    assert(encoded == expected);
    const auto decoded = sc::base64::decode(expected);
    assert(decoded == plain);
}

static void test_round_trip(const std::string_view input) {
    const auto encoded = sc::base64::encode(input);
    const auto decoded = sc::base64::decode(encoded);
    assert(decoded == input);
}

static void expect_failure(const std::string_view input) {
    try {
        (void) sc::base64::decode(std::string{input});
        std::cerr << "Expected failure: " << input << '\n';
        assert(false);
    } catch (const std::runtime_error &) {
    }
}

int main() {
    // RFC 4648 test vectors
    test_vector("", "");
    test_vector("f", "Zg==");
    test_vector("fo", "Zm8=");
    test_vector("foo", "Zm9v");
    test_vector("foob", "Zm9vYg==");
    test_vector("fooba", "Zm9vYmE=");
    test_vector("foobar", "Zm9vYmFy");

    // All possible byte values
    {
        std::string binary;
        binary.reserve(256);
        for (int i = 0; i < 256; ++i) binary.push_back(static_cast<char>(i));
        test_round_trip(binary);
    }

    // Every length from 0..4096
    {
        for (size_t len = 0; len <= 4096; ++len) {
            std::string input(len, '\0');
            for (size_t i = 0; i < len; ++i) input[i] = static_cast<char>(i & 0xFF);
            test_round_trip(input);
        }
    }

    // Deterministic pseudo-random pattern
    {
        for (size_t len = 0; len <= 4096; ++len) {
            std::string input(len, '\0');
            for (size_t i = 0; i < len; ++i) input[i] = static_cast<char>((i * 37 + 17) & 0xFF);
            test_round_trip(input);
        }
    }

    // Invalid input tests
    expect_failure("A");
    expect_failure("AAA");
    expect_failure("AAAA$");
    expect_failure("!!!!");
    expect_failure("????");
    expect_failure("abc");
    expect_failure("abcde");

    std::cout << "All tests passed." << std::endl;
}
