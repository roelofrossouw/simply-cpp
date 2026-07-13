#include <sc.h>
#define MAX_ITER 100
#define TOLERANCE 1e-6

using namespace std;

template<sc::dual_type T>
T fn(T x) {
    return (x * x * x) + x - T{1};
    // return sc::dual_math::dsin(x);
    // return sc::dual_math::dcos(x);
}

int main() {
    sc::dual r(3.0);

    // cout << r << endl;
    // cout << ~r << endl;
    // cout << ~~r << endl;
    // cout << ~~~r << endl;

    cout << "fn (" << r << ") = " << (float) fn(r) << endl; // << " (" << fn(r) << ")" << endl;
    cout << "fn'(" << r << ") = " << (float) fn(~r) << endl; //  << " (" << fn(~r) << ")" << endl;
    cout << "fn\"(" << r << ") = " << (float) fn(~~r) << endl; //  << " (" << fn(~~r) << ")" << endl;
    cout << "fn\"'(" << r << ") = " << (float) fn(~~~r) << endl; //  << " (" << fn(~~~r) << ")" << endl;
    cout << "fn\"\"(" << r << ") = " << (float) fn(~~~~r) << endl; //  << " (" << fn(~~~~r) << ")" << endl;

    cout << "\nFinding a root.\n";
    sc::dual x(0.0f);
    for (int i = 0; i < MAX_ITER; i++) {
        auto [fx, dfx] = fn(~x);
        if (abs(fx) < TOLERANCE) break;
        x -= fx / dfx;
        cout << '.'; // See how many iterations it took.
    }
    cout << "Root @ x = " << x << endl;

    return 0;
}
