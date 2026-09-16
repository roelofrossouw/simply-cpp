#include <sc.h>

#include <stdexcept>
#include <string>
#include <string_view>

#include "sc_test.h"

using namespace std;

namespace {
    constexpr string_view alphabet{"ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"};

    // Encode and decode must agree with the published vector in both directions.
    void check_vector(const string_view plain, const string_view expected) {
        CHECK_EQ(sc::base64::encode(plain), string{expected});
        CHECK_EQ(sc::base64::decode(expected), string{plain});
    }

    void check_round_trip(const string_view input) {
        const auto encoded = sc::base64::encode(input);
        CHECK_EQ(encoded.size(), (input.size() + 2) / 3 * 4);
        CHECK_EQ(sc::base64::decode(encoded), string{input});
    }

    // Padding is only ever at the end, and its length follows from the input size.
    void check_shape(const string_view input) {
        const auto encoded = sc::base64::encode(input);
        const size_t expected_padding = input.empty() ? 0 : (3 - input.size() % 3) % 3;
        size_t padding = 0;
        while (padding < encoded.size() && encoded[encoded.size() - 1 - padding] == '=') ++padding;
        CHECK_EQ(padding, expected_padding);
        for (size_t i = 0; i + padding < encoded.size(); ++i)
            CHECK_MSG(alphabet.find(encoded[i]) != string_view::npos,
                      "unexpected character '" + string(1, encoded[i]) + "' at index " + to_string(i));
    }

    string byte_pattern(const size_t length, const int stride, const int offset) {
        string input(length, '\0');
        for (size_t i = 0; i < length; ++i) input[i] = static_cast<char>((i * stride + offset) & 0xFF);
        return input;
    }
}

int main() {
    SECTION("RFC 4648 test vectors");
    check_vector("", "");
    check_vector("f", "Zg==");
    check_vector("fo", "Zm8=");
    check_vector("foo", "Zm9v");
    check_vector("foob", "Zm9vYg==");
    check_vector("fooba", "Zm9vYmE=");
    check_vector("foobar", "Zm9vYmFy");

    SECTION("Encoded shape: alphabet and padding");
    check_shape("");
    check_shape("f");
    check_shape("fo");
    check_shape("foo");
    check_shape("sure.");
    for (size_t len = 0; len <= 64; ++len) check_shape(byte_pattern(len, 37, 17));
    // Bytes that map to the tail of the alphabet ('+' and '/') must survive.
    CHECK_EQ(sc::base64::encode(string("\xFB\xFF", 2)), string{"+/8="});
    CHECK_EQ(sc::base64::decode("+/8="), string("\xFB\xFF", 2));

    SECTION("Embedded NUL bytes are preserved");
    {
        const string with_nuls("a\0b\0\0c", 6);
        CHECK_EQ(with_nuls.size(), size_t{6});
        const auto decoded = sc::base64::decode(sc::base64::encode(with_nuls));
        CHECK_EQ(decoded.size(), with_nuls.size());
        CHECK_EQ(decoded, with_nuls);
    }

    SECTION("All 256 byte values round trip");
    {
        string binary;
        binary.reserve(256);
        for (int i = 0; i < 256; ++i) binary.push_back(static_cast<char>(i));
        check_round_trip(binary);
    }

    SECTION("Every length from 0..4096 round trips");
    for (size_t len = 0; len <= 4096; ++len) check_round_trip(byte_pattern(len, 1, 0));

    SECTION("Deterministic pseudo-random pattern round trips");
    for (size_t len = 0; len <= 4096; ++len) check_round_trip(byte_pattern(len, 37, 17));

    SECTION("Malformed input is rejected");
    // Lengths that are not a multiple of four.
    CHECK_THROWS_AS(sc::base64::decode("A"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("AAA"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("AAAA$"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("abc"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("abcde"), runtime_error);
    // Characters outside the alphabet.
    CHECK_THROWS_AS(sc::base64::decode("!!!!"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("????"), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("Zm9 "), runtime_error);
    CHECK_THROWS_AS(sc::base64::decode("ab*d"), runtime_error);
    // Valid input must not throw.
    CHECK_NOTHROW(sc::base64::decode("Zm9vYmFy"));
    CHECK_NOTHROW(sc::base64::decode(""));

    TEST_SUMMARY();
}
