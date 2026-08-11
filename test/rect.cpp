#include "include/rect.h"

#include <sc.h>

using namespace std;

int main() {
    sc::rect r{10, 20, 30, 40};
    cout << r << endl;
    cout << r + 10 << endl;
    r += {100, 100};
    cout << r << endl;
}
