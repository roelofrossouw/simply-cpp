// sc::distance and its units: kept in points, converting to them, and taken by geometry as is.
#include "console.h"
#include "distance.h"
#include "point.h"
#include "rect.h"
#include "size.h"

#include <string>
#include <type_traits>

#include "sc_test.h"

template<typename Size>
concept has_area = requires(Size s) { s.area(); };

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

    SECTION("Written with their units, and cast from one unit to another");
    CHECK_EQ(sc::console::format(sc::distance(595)), std::string{"595pt"});
    CHECK_EQ(sc::console::format(sc::dist::mm(210)), std::string{"210mm"});
    CHECK_EQ(sc::console::format(sc::dist::cm(2.5)), std::string{"2.5cm"});
    CHECK_EQ(sc::console::format(sc::dist::inch(1.5)), std::string{"1.5in"});
    CHECK_EQ(sc::console::format(sc::dist::point(12)), std::string{"12pt"});
    const sc::distance width = sc::dist::mm(210);
    CHECK_EQ(sc::console::format(width), std::string{"595.276pt"});                          // a distance: points
    CHECK_EQ(sc::console::format(static_cast<sc::dist::mm>(width)), std::string{"210mm"});   // the same length in mm
    CHECK_EQ(sc::console::format(static_cast<sc::dist::inch>(sc::distance(72))), std::string{"1in"});
    CHECK_NEAR(static_cast<sc::dist::cm>(sc::dist::inch(1)).centimetres(), 2.54, 1e-12);       // unit to unit
    CHECK_EQ(sc::dist::mm(10).points(), sc::dist::mm(sc::dist::mm(10)).points());               // not 10 points read as mm

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

    SECTION("Points and sizes of distances keep their units");
    {
        const sc::size_<sc::distance> a4{sc::dist::mm(210), sc::dist::mm(297)};
        CHECK_EQ(sc::console::format(a4), std::string{"[595.276pt,841.89pt]"});
        const auto in_mm = static_cast<sc::size_<sc::dist::mm>>(a4);           // the same size, in mm
        CHECK_EQ(sc::console::format(in_mm), std::string{"[210mm,297mm]"});
        CHECK_EQ(sc::console::format(static_cast<sc::size_<sc::dist::inch>>(sc::size_<sc::distance>{72, 144})), std::string{"[1in,2in]"});

        const sc::size plain = a4;                                              // numbers of points
        CHECK_NEAR(plain.width(), 595.2756, 1e-4);
        const sc::size_<sc::distance> back = plain;                             // and back
        CHECK(back == a4);
        const sc::size_<sc::distance> from_mm = in_mm;                          // a unit to a plain distance
        CHECK_NEAR(from_mm.height().points(), 841.8898, 1e-4);

        CHECK_EQ(sc::console::format(a4 * 0.5), std::string{"[297.638pt,420.945pt]"}); // a factor
        CHECK_EQ(sc::console::format(sc::size_<sc::distance>{10, 20} + 5), std::string{"[15pt,25pt]"}); // a number is points
        const sc::size_<sc::dist::mm> mm_plus = in_mm + sc::size_<sc::dist::mm>{sc::dist::mm(10), sc::dist::mm(3)};
        CHECK_EQ(sc::console::format(mm_plus), std::string{"[220mm,300mm]"});
        CHECK_NEAR((in_mm + 72).width().millimetres(), 235.4, 1e-9);           // 72 points: an inch
        CHECK_EQ(sc::console::format(sc::size_<sc::distance>{-5, 10}), std::string{"[0pt,10pt]"}); // never below 0

        const sc::point_<sc::dist::mm> corner{sc::dist::mm(10), sc::dist::mm(20)};
        CHECK_EQ(sc::console::format(corner), std::string{"(10mm,20mm)"});
        const sc::point at = corner;                                            // points
        CHECK_NEAR(at.x(), 28.3464567, 1e-6);
        CHECK_EQ(sc::console::format(static_cast<sc::point_<sc::dist::cm>>(corner)), std::string{"(1cm,2cm)"});
        CHECK_EQ(sc::console::format(sc::size_<sc::distance>{}), std::string{"[0pt,0pt]"});

        // What doesn't compile: an area of distances, and rounding a size to whole numbers implicitly.
        static_assert(!has_area<sc::size_<sc::distance>>);
        static_assert(has_area<sc::size>);
        static_assert(!std::is_convertible_v<sc::size, sc::size_i>);
        static_assert(std::is_convertible_v<sc::size_i, sc::size>);
        static_assert(!std::is_convertible_v<sc::size_<sc::distance>, sc::size_<sc::dist::mm>>); // a unit: say so
    }

    TEST_SUMMARY();
}
