#include <sc.h>

using namespace std;

int main() {
    string sample = "Hello";
    cout << sample << " => " << "(" << sc::base64::encode(sample) << ")" << endl;
    return 0;
}
