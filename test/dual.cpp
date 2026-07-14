#include <sc.h>
#include <random>
#define MAX_ITER 1000
#define TOLERANCE 1e-8

using namespace std;

template<typename T>
T fn(const T &x) {
    return x * x * x + x - 1;
    // return sqrt(x); // Tricky root because of NAN when taking sqrt < 0
    // return exp(x);
    // return 3 * x * x * x - 5 * x + 13; //  => Root not found starting at 1.0, add search direction? or multiple starting points?...
    // return sc::sin(x);
    // return sc::cos(x);
}

int main() {
    sc::dual r((float) 3);
    // sc::dual<float> r(3); // Implicitly...

    cout << static_cast<string>(r) << endl;
    cout << static_cast<string>(~r) << endl;
    cout << static_cast<string>(~~r) << endl;
    cout << static_cast<string>(~~~r) << endl << endl;

    cout << "fn (" << r << ") = " << fn(r) << endl;
    cout << "fn'(" << r << ") = fn(" << static_cast<string>(~r) << ") = " << fn(~r) << endl;
    cout << "fn\"(" << r << ") = " << fn(~~r) << endl;
    cout << "fn\"'(" << r << ") = " << fn(~~~r) << endl;
    cout << "fn\"\"(" << r << ") = " << fn(~~~~r) << endl << endl;

    cout << "Finding a root." << endl;

    int last_iteration = 0;
    sc::dual last_value = sc::rand(-10, 10);
    while (isnan(static_cast<double>(fn(last_value)))) last_value = sc::rand(-10, 10);

    sc::dual x(last_value);
    cout << "Starting loop at: fn(" << std::setprecision(15) << x << ") = " << fn(x) << endl;
    auto afx = TOLERANCE;
    for (int i = 0; i <= MAX_ITER && afx >= TOLERANCE; i++) {
        last_iteration = i;
        auto [fx, dfx] = fn(~x);
        afx = abs(fx);
        if (isnan(afx) || isinf(afx)) {
            x = sc::rand(abs(x) * -2, abs(x) * 2);
            continue;
        }
        last_value = x;
        double mv = fx / dfx;
        while (abs(mv) > TOLERANCE && isnan(static_cast<float>(fn(x - mv)))) mv /= sc::rand(0.9, 5);
        x -= mv;
    }
    cout << (afx <= TOLERANCE ? "Found root" : "Could not find root. Last attempt");
    cout << " after " << last_iteration << " iterations: fn(" << std::setprecision(15) << last_value << ") = " << fn(last_value) << endl;


    return 0;
}
