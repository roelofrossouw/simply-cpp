#include <sc.h>
#define MAX_ITER 1'000'000
#define TOLERANCE 1e-6

using namespace std;
using namespace dual_math;

template<dual_type T>
T f(T x) {
    return (x * x * x) + x - T{1};
}

int main() {
    double r = 3;
    dual d = dual<double>::seed(r);
    dual d2 = dual<dual<double>>::seed(d);

    cout << f(r) << endl;
    cout << f(d) << endl;
    cout << f(d2) << endl;

    auto [a, b] = f(d);
    auto [_, c] = f(d2).eps;
    cout << "f (" << r << ") = " << a << endl;
    cout << "f'(" << r << ") = " << b << endl;
    cout << "f\"(" << r << ") = " << c << endl;

    double x = 0;
    for (int i = 0; i < MAX_ITER; i++) {
        auto [fx, dfx] = f(dual<double>::seed(x));
        if (abs(fx) < TOLERANCE) {
            break;
        }
        x -= fx / dfx;
    }
    cout << "Root @ x = " << x << endl;

    return 0;
}
