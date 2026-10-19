// sc::distance and its units: kept in points, converting to them, and taken by geometry as is.
#include "distance.h"
#include "point.h"
#include "rect.h"
#include "size.h"

#include "sc_test.h"

int main() {
    SECTION("Units into points");
    CHECK_NEAR(sc::dist::mm(25.4).points(), 72.0, 1e-9);
    CHECK_NEAR(sc::dist::inch(1).points(), 72.0, 1e-9);
    CHECK_EQ(sc::distance(72).points(), 72.0);
    CHECK_NEAR(sc::dist::mm(10).points(), 28.3464567, 1e-6);
    CHECK_NEAR(sc::distance(72).millimetres(), 25.4, 1e-9);
    CHECK_NEAR(sc::dist::mm(10).inches(), 10 / 25.4, 1e-12);
    CHECK_NEAR(sc::dist::inch(2).millimetres(), 50.8, 1e-9);
    CHECK_NEAR(sc::dist::cm(2.54).points(), 72.0, 1e-9);
    CHECK_EQ(sc::dist::point(12).points(), 12.0);
    CHECK(sc::dist::cm(1) == sc::dist::mm(10));

    SECTION("Converting to a number gives points");
    const double points = sc::dist::inch(0.5);
    CHECK_EQ(points, 36.0);
    CHECK_EQ(static_cast<int>(sc::dist::inch(1)), 72);
    CHECK_NEAR(sc::dist::mm(15) + sc::dist::inch(0.5), 78.519685, 1e-6); // arithmetic is on the points
    CHECK(sc::dist::mm(25.4) == sc::dist::inch(1));
    CHECK(sc::dist::mm(10) < sc::dist::inch(1));
    const sc::distance kept = sc::dist::mm(10); // a unit copied into a distance keeps its value
    CHECK_NEAR(kept.millimetres(), 10.0, 1e-9);
    static_assert(sc::dist::inch(1).points() == 72); // constexpr

    SECTION("Geometry takes distances as they are, mixed with numbers (points)");
    const sc::rect box{sc::dist::mm(10), sc::dist::inch(1.1), sc::dist::point(72), 72};
    CHECK_NEAR(box.left(), 28.3464567, 1e-6);
    CHECK_NEAR(box.top(), 79.2, 1e-9);
    CHECK_EQ(box.width(), 72.0);
    CHECK_EQ(box.height(), 72.0);
    const sc::point corner{sc::dist::inch(1), sc::dist::mm(25.4)};
    CHECK_NEAR(corner.x(), 72.0, 1e-9);
    CHECK_NEAR(corner.y(), 72.0, 1e-9);
    const sc::size a4{sc::dist::mm(210), sc::dist::mm(297)};
    CHECK_NEAR(a4.width(), 595.2756, 1e-4);
    CHECK_NEAR(a4.height(), 841.8898, 1e-4);

    TEST_SUMMARY();
}
