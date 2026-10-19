// sc::transform and sc::group.
#include <transform.h>

#include <sc_test.h>

#include <cmath>
#include <stdexcept>
#include <variant>

namespace {
    bool near(const sc::point &p, const double x, const double y) { return std::abs(p.x() - x) < 1e-9 && std::abs(p.y() - y) < 1e-9; }

    bool near(const sc::rect &r, const double left, const double top, const double width, const double height) {
        return std::abs(r.left() - left) < 1e-9 && std::abs(r.top() - top) < 1e-9 && std::abs(r.width() - width) < 1e-9 &&
               std::abs(r.height() - height) < 1e-9;
    }

    template<typename T>
    bool holds(const sc::shape &s) { return std::holds_alternative<T>(s.value()); }
}

int main() {
    SECTION("Points through moves, scales, rotations and flips");
    {
        CHECK(near(sc::transform{}.apply(sc::point{3, 4}), 3, 4));
        CHECK(near(sc::transform::translate(10, -2).apply(sc::point{3, 4}), 13, 2));
        CHECK(near(sc::transform::scale(2).apply(sc::point{3, 4}), 6, 8));
        CHECK(near(sc::transform::scale(2, 3, {1, 1}).apply(sc::point{2, 2}), 3, 4)); // about (1, 1)
        CHECK(near(sc::transform::rotate(90).apply(sc::point{1, 0}), 0, 1));          // clockwise on screen: x towards y down
        CHECK(near(sc::transform::rotate(180, {5, 5}).apply(sc::point{6, 5}), 4, 5));
        CHECK(near(sc::transform::flip_horizontal(10).apply(sc::point{3, 4}), 17, 4));
        CHECK(near(sc::transform::flip_vertical(10).apply(sc::point{3, 4}), 3, 16));
        CHECK(near(sc::transform::flip_horizontal(1).apply(sc::point_i{3, 4}), -1, 4)); // an int point, in doubles
    }

    SECTION("then(): one transform after another");
    {
        const auto both = sc::transform::scale(2).then(sc::transform::translate(10, 0));
        CHECK(near(both.apply(sc::point{1, 1}), 12, 2));
        const auto other_way = sc::transform::translate(10, 0).then(sc::transform::scale(2));
        CHECK(near(other_way.apply(sc::point{1, 1}), 22, 2));
        const auto twice = sc::transform::flip_horizontal(5).then(sc::transform::flip_horizontal(5));
        CHECK(near(twice.apply(sc::point{1, 2}), 1, 2));
        CHECK(twice == sc::transform{});
    }

    SECTION("inverse(): undoing a transform");
    {
        const auto t = sc::transform::rotate(30, {5, 5}).then(sc::transform::scale(2, 3)).then(sc::transform::translate(7, -1));
        const auto back = t.inverse().apply(t.apply(sc::point{3, 4}));
        CHECK(near(back, 3, 4));
        CHECK(near(t.then(t.inverse()).apply(sc::point{-2, 9}), -2, 9));
        CHECK_THROWS_AS(sc::transform::scale(0, 1).inverse(), std::domain_error);
    }

    SECTION("What a transform keeps");
    {
        CHECK(sc::transform::rotate(30).conformal());
        CHECK(!sc::transform::rotate(30).keeps_axes());
        CHECK(sc::transform::rotate(90).keeps_axes());
        CHECK(sc::transform::flip_horizontal(3).conformal());
        CHECK(!sc::transform::scale(2, 3).conformal());
        CHECK(sc::transform::scale(2, 3).keeps_axes());
        CHECK(!sc::transform(1, 0, 0.5, 1, 0, 0).keeps_axes()); // a shear
        CHECK_NEAR(sc::transform::scale(3).scale_factor(), 3.0, 1e-12);
        CHECK_NEAR(sc::transform::rotate(40, {7, 7}).scale_factor(), 1.0, 1e-12);
    }

    SECTION("Lines, polygons and paths keep their kind");
    {
        const auto mirror = sc::transform::flip_horizontal(0);
        const auto line = mirror.apply(sc::line{{1, 2}, {3, 4}});
        CHECK(near(line.from(), -1, 2));
        CHECK(near(line.to(), -3, 4));
        const auto triangle = mirror.apply(sc::polygon{{0, 0}, {2, 0}, {1, 1}});
        CHECK_EQ(triangle.size(), std::size_t{3});
        CHECK(near(triangle[1], -2, 0));
        sc::path arch;
        arch.move_to({0, 0}).cubic_to({0, 10}, {10, 10}, {10, 0}).close();
        const auto moved = sc::transform::translate(5, 5).apply(arch);
        CHECK_EQ(moved.steps().size(), arch.steps().size());
        CHECK(near(moved.bounds(), 5, 5, 10, 7.5)); // exact bounds move with it
    }

    SECTION("A rect: a rect, a rotated_rect or a polygon");
    {
        const sc::rect box{10, 20, 40, 10};
        const auto flipped = sc::transform::flip_horizontal(0).apply(box);
        CHECK(holds<sc::rect>(flipped));
        CHECK(near(flipped.bounds(), -50, 20, 40, 10));
        const auto stretched = sc::transform::scale(2, 3).apply(box);
        CHECK(holds<sc::rect>(stretched));
        CHECK(near(stretched.bounds(), 20, 60, 80, 30));
        CHECK(holds<sc::rect>(sc::transform::rotate(90).apply(box))); // a quarter turn keeps it axis-aligned
        const auto turned = sc::transform::rotate(30, box.center()).apply(box);
        CHECK(holds<sc::rotated_rect>(turned));
        const auto &as_rotated = std::get<sc::rotated_rect>(turned.value());
        CHECK_NEAR(as_rotated.angle(), 30.0, 1e-9);
        CHECK(near(as_rotated.center(), 30, 25));
        CHECK_NEAR(as_rotated.width(), 40.0, 1e-9);
        CHECK(holds<sc::polygon>(sc::transform(1, 0, 0.5, 1, 0, 0).apply(box))); // sheared: a parallelogram
        CHECK(holds<sc::rect>(sc::transform::translate(1, 1).apply(sc::rect_i{0, 0, 2, 2})));
    }

    SECTION("A rounded_rect, a rotated_rect and a circle");
    {
        const sc::rounded_rect card{sc::rect{0, 0, 40, 20}, 4};
        const auto bigger = sc::transform::scale(2).apply(card);
        CHECK(holds<sc::rounded_rect>(bigger));
        CHECK_NEAR(std::get<sc::rounded_rect>(bigger.value()).radius(), 8.0, 1e-12);
        CHECK(holds<sc::path>(sc::transform::scale(2, 1).apply(card)));  // the corners would be elliptical
        CHECK(holds<sc::path>(sc::transform::rotate(10).apply(card)));

        const sc::rotated_rect tilted{{0, 0}, {10, 4}, 20};
        const auto mirrored = sc::transform::flip_horizontal(0).apply(tilted);
        CHECK(holds<sc::rotated_rect>(mirrored));
        const auto &as_rotated = std::get<sc::rotated_rect>(mirrored.value());
        CHECK_NEAR(std::abs(std::remainder(as_rotated.angle() - 160, 180)), 0.0, 1e-9); // 180 - 20: the width's direction mirrored
        CHECK(holds<sc::polygon>(sc::transform::scale(1, 2).apply(tilted)));

        const auto ring = sc::transform::scale(3, {0, 0}).apply(sc::circle{{1, 1}, 2});
        CHECK(holds<sc::circle>(ring));
        CHECK_NEAR(std::get<sc::circle>(ring.value()).radius(), 6.0, 1e-12);
        const auto oval = sc::transform::scale(2, 1).apply(sc::circle{{0, 0}, 1});
        CHECK(holds<sc::path>(oval));
        CHECK(near(oval.bounds(), -2, -1, 4, 2));
    }

    SECTION("A group: a geometry of geometries");
    {
        const sc::group empty;
        CHECK(empty.empty());
        CHECK(near(empty.bounds(), 0, 0, 0, 0));
        const sc::group marks{sc::rect{10, 10, 20, 20}, sc::circle{{100, 50}, 10}, sc::line{{0, 80}, {5, 90}}};
        CHECK_EQ(marks.size(), std::size_t{3});
        CHECK(near(marks.bounds(), 0, 10, 110, 80));
        CHECK(near(to_path(marks).bounds(), 0, 10, 110, 80));
        const sc::shape held = marks; // a group is a shape, so groups nest
        const sc::group nested{held, sc::rect{200, 200, 1, 1}};
        CHECK(near(nested.bounds(), 0, 10, 201, 191));
    }

    SECTION("Flipping a group: in place, or across a page");
    {
        sc::group marks{sc::rect{10, 10, 20, 20}, sc::circle{{100, 50}, 10}};
        const auto before = marks.bounds();
        marks.flip_horizontal(); // about its own middle: the same bounds, the shapes swapped round
        CHECK(near(marks.bounds(), before.left(), before.top(), before.width(), before.height()));
        CHECK(near(marks[0].bounds(), 90, 10, 20, 20)); // the square moved to the right
        CHECK(near(std::get<sc::circle>(marks[1].value()).center(), 20, 50));

        sc::group on_page{sc::rect{10, 10, 20, 20}};
        on_page.flip_horizontal(sc::rect{0, 0, 595, 842}); // across an A4 page: left margin to right margin
        CHECK(near(on_page.bounds(), 565, 10, 20, 20));
        on_page.flip_vertical(sc::rect{0, 0, 595, 842});
        CHECK(near(on_page.bounds(), 565, 812, 20, 20));
        CHECK(holds<sc::rect>(on_page[0])); // still a rect

        auto nested = sc::group{sc::shape{sc::group{sc::rect{0, 0, 10, 10}}}};
        nested.apply(sc::transform::translate(5, 0));
        CHECK(holds<sc::group>(nested[0]));
        CHECK(near(nested.bounds(), 5, 0, 10, 10));
    }

    TEST_SUMMARY();
}
