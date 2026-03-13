#include "rjr.h"

using namespace std;

int main() {
    string sample = "Hello World in Base64...";
    cout << "Source Text: " << sample << endl;
    string encoded = rjr::base64::encode(sample);
    cout << "Encoded: " << encoded << endl;
    auto decoded = rjr::base64::decode(encoded);
    cout << "Decoded: " << decoded << endl;
}
