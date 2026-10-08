// Base64-encodes a string and decodes it again, timing the round trip.
// Needs no server, so it runs anywhere simply-cpp is installed.

#include <sc.h>

#include <iostream>
#include <string>

int main() {
    // [readme]
    sc::timer sw;
    const std::string sample = "Hello World!";
    const auto encoded = sc::base64::encode(sample);
    const auto decoded = sc::base64::decode(encoded);
    std::cout << sample << " => " << encoded << " => " << decoded << '\n';
    std::cout << "Done after " << sw << '\n';
    // [/readme]
    return decoded == sample ? 0 : 1;
}
