#include "include/rect.h"

#include <sc.h>

using namespace std;

int main()
{
    cout << "Standard type <double>\n";
    sc::rect r{13.5, 20, 30, 40};
    cout << r << endl;
    cout << r + 10.25 << endl;
    r += {100.5, 100};
    cout << r << endl;

    cout << "\nSpecial type <int>\n";
    sc::rect_i r2{13.5, 20.0, 30.0, 40.0};
    cout << r2 << endl;
    cout << r2 + 10.25 << endl;
    r2 += {100, 100};
    cout << r2 << endl;
}
