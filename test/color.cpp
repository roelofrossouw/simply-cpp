#include "include/color.h"

#include <sc.h>

using namespace std;

std::ostream &operator<<(std::ostream &lhs, const sc::color &rhs) {
    lhs << rhs.cyan() << ", " << rhs.magenta() << ", " << rhs.yellow() << ", " << rhs.black() << ", " << rhs.alpha() << endl;
    return lhs;
}

int main() {
    sc::color k = sc::color::from_web(255, 0, 0);
    cout << k << endl;

    k = sc::color::from_web(0, 128, 0);
    cout << k << endl;


    k = sc::color::from_web(128, 128, 128);
    cout << k << endl;
}
