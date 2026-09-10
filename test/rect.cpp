#include <sc.h>

using namespace std;

template <typename T>
T test()
{
    T r{13.5, 20, 30, 40};
    cout << r << endl;
    cout << r + 10.25 << endl;
    cout << (r += 10.25) << endl;
    cout << r << endl;
    sc::size sz{10, 10};
    r *= sz;
    cout << r << endl;

    cout << r.center() << endl;
    cout << r.area() << endl;
    return r;
}

int main()
{
    cout << "Standard type <double>\n";
    test<sc::rect>();

    cout << "\nSpecial type <int>\n";
    test<sc::rect_i>();
}
