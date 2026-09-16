#include <sc.h>

#include <sstream>
#include <string>

#include "sc_test.h"

using namespace std;

namespace {
    template<typename T>
    string to_text(const T &value) {
        ostringstream stream;
        stream << value;
        return stream.str();
    }
}

int main() {
    SECTION("point<double>: accessors and construction");
    {
        sc::point p{13.5, 20};
        CHECK_EQ(p.x(), 13.5);
        CHECK_EQ(p.y(), 20.0);
        CHECK_EQ(to_text(p), string{"(13.5,20)"});

        // Setters return the value they stored.
        CHECK_EQ(p.x(-1.25), -1.25);
        CHECK_EQ(p.y(7.5), 7.5);
        CHECK_EQ(p.x(), -1.25);
        CHECK_EQ(p.y(), 7.5);

        // A default constructed point is the origin.
        const sc::point origin;
        CHECK_EQ(origin.x(), 0.0);
        CHECK_EQ(origin.y(), 0.0);
        CHECK_EQ(sc::point{}, sc::point(0, 0));
    }

    SECTION("point<int>: mixed argument types are truncated");
    {
        const sc::point_i p{13.9, 20.7f};
        CHECK_EQ(p.x(), 13);
        CHECK_EQ(p.y(), 20);
        CHECK_EQ(to_text(p), string{"(13,20)"});

        const sc::point_i negative{-13.9, -20.7};
        CHECK_EQ(negative.x(), -13);
        CHECK_EQ(negative.y(), -20);
    }

    SECTION("Scalar arithmetic applies to both components");
    {
        sc::point p{13.5, 20};
        CHECK_EQ(p + 10.25, sc::point(23.75, 30.25));
        CHECK_EQ(p, sc::point(13.5, 20)); // operator+ leaves the original alone
        CHECK_EQ(p - 0.5, sc::point(13.0, 19.5));
        CHECK_EQ(p * 2, sc::point(27.0, 40.0));
        CHECK_EQ(p / 2.0, sc::point(6.75, 10.0));

        p += 1;
        CHECK_EQ(p, sc::point(14.5, 21.0));
        p -= 15;
        CHECK_EQ(p, sc::point(-0.5, 6.0));
        p *= 4;
        CHECK_EQ(p, sc::point(-2.0, 24.0));
        p /= -2;
        CHECK_EQ(p, sc::point(1.0, -12.0));
    }

    SECTION("Component-wise arithmetic between points");
    {
        sc::point p{13.5, 20};
        const sc::point other{1.5, -4};
        CHECK_EQ(p + other, sc::point(15.0, 16.0));
        CHECK_EQ(p - other, sc::point(12.0, 24.0));
        CHECK_EQ(p * other, sc::point(20.25, -80.0));
        CHECK_EQ(p / other, sc::point(9.0, -5.0));

        p += {123.456, 100};
        CHECK_NEAR(p.x(), 136.956, 1e-12);
        CHECK_EQ(p.y(), 120.0);
        p -= {123.456, 100};
        CHECK_NEAR(p.x(), 13.5, 1e-12);
        CHECK_EQ(p.y(), 20.0);
    }

    SECTION("Integer arithmetic truncates");
    {
        sc::point_i p{13, 20};
        CHECK_EQ(p + 10.25, sc::point_i(23, 30));
        CHECK_EQ(p / 2, sc::point_i(6, 10));
        p *= 3;
        CHECK_EQ(p, sc::point_i(39, 60));
    }

    SECTION("Equality");
    {
        const sc::point p{122.956, 106};
        CHECK_EQ(p, sc::point(122.956, 106.0));
        CHECK_NE(p, sc::point(124.456, 106.0));
        CHECK_NE(p, sc::point(122.956, 107.0));
        CHECK(!(p != sc::point(122.956, 106.0)));
    }

    SECTION("size<double>: accessors, area and negative clamping");
    {
        sc::size s{13.5, 20};
        CHECK_EQ(s.width(), 13.5);
        CHECK_EQ(s.height(), 20.0);
        CHECK_EQ(s.area(), 270.0);
        CHECK_EQ(to_text(s), string{"[13.5,20]"});

        CHECK_EQ(s.width(4.0), 4.0);
        CHECK_EQ(s.height(5.0), 5.0);
        CHECK_EQ(s.area(), 20.0);

        // A size can never be constructed negative.
        const sc::size negative{-13.5, -20};
        CHECK_EQ(negative.width(), 0.0);
        CHECK_EQ(negative.height(), 0.0);
        CHECK_EQ(negative.area(), 0.0);

        const sc::size half_negative{-13.5, 20};
        CHECK_EQ(half_negative.width(), 0.0);
        CHECK_EQ(half_negative.height(), 20.0);
        CHECK_EQ(half_negative.area(), 0.0);
    }

    SECTION("size<int>");
    {
        const sc::size_i s{13.9, 20.7};
        CHECK_EQ(s.width(), 13);
        CHECK_EQ(s.height(), 20);
        CHECK_EQ(s.area(), 260);
        CHECK_EQ(to_text(s), string{"[13,20]"});

        const sc::size_i negative{-13, -20};
        CHECK_EQ(negative.area(), 0);
    }

    SECTION("size arithmetic");
    {
        sc::size s{10, 20};
        CHECK_EQ(s * 2, sc::size(20.0, 40.0));
        CHECK_EQ(s / 2, sc::size(5.0, 10.0));
        CHECK_EQ(s + sc::size(5, 5), sc::size(15.0, 25.0));
        s *= 3;
        CHECK_EQ(s, sc::size(30.0, 60.0));
        CHECK_EQ(s.area(), 1800.0);
    }

    SECTION("Points and sizes combine component-wise");
    {
        const sc::point a{1, 2};
        const sc::size b{3, 4};
        CHECK_EQ(a + b, sc::point(4.0, 6.0));
        CHECK_EQ(a * b, sc::point(3.0, 8.0));
        CHECK_EQ(a - b, sc::point(-2.0, -2.0));
    }

    TEST_SUMMARY();
}
