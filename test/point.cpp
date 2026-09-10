#include <sc.h>

using namespace std;

namespace
{
    template <typename T>
    T test_pair()
    {
        T r{13.5, 20};
        cout << r << endl;
        T r2{-13.5f, -20};
        cout << r2 << endl;
        cout << r2 / 2.f << endl;
        r -= 15;
        cout << r << endl;
        r += 1;
        cout << r << endl;
        cout << r + 10.25 << endl;
        r += {123.456, 100};
        cout << r << endl;
        T cmp{122.956, 106.f};
        T cmp2{124.456, 106};
        cout << r << (r == cmp ? " == " : " != ") << cmp << endl;
        cout << r << (r == cmp2 ? " == " : " != ") << cmp2 << endl;
        cout << r + cmp << endl;
        return r;
    }

    template <typename T>
    void test_point()
    {
        auto r = test_pair<T>();
        cout << "Ending with x => " << r.x() << " and y => " << r.y() << endl;
    }

    template <typename T>
    void test_size()
    {
        auto r = test_pair<T>();
        cout << "Ending with width => " << r.width() << " and height => " << r.height() << endl;
    }
}

int main()
{
    cout << "\n\nTest point<double>\n";
    test_point<sc::point>();
    cout << "\n\nTest point<int>\n";
    test_point<sc::point_i>();
    cout << "\n\nTest size<double>\n";
    test_size<sc::size>();
    cout << "\n\nTest size<int>\n";
    test_size<sc::size_i>();

    sc::point a{1, 2};
    sc::size b{3, 4};

    cout << a + b << endl;
    cout << a * b << endl;

    return 0;
}
