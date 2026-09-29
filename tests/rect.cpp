#include <sc.h>

#include <sstream>
#include <ranges>
#include <string>
#include <vector>

#include "sc_test.h"

using namespace std;

namespace {
    template<typename T>
    string to_text(const T &value) {
        ostringstream stream;
        stream << value;
        return stream.str();
    }

    template<typename T>
    void check_edges(const sc::rect_<T> &r, const T left, const T top, const T width, const T height) {
        CHECK_EQ(r.left(), left);
        CHECK_EQ(r.top(), top);
        CHECK_EQ(r.width(), width);
        CHECK_EQ(r.height(), height);
        CHECK_EQ(r.right(), static_cast<T>(left + width));
        CHECK_EQ(r.bottom(), static_cast<T>(top + height));
    }
}

int main() {
    SECTION("Polygon points");
    {
        const sc::polygon polygon{{1, 2}, {3, 4}, {5, 6}, {7, 8}};
        CHECK_EQ(polygon.size(), size_t{4});
        CHECK_EQ(polygon[0], (sc::point_i{1, 2}));
        CHECK_EQ(polygon[3], (sc::point_i{7, 8}));
        CHECK_EQ(std::ranges::distance(polygon), 4);
        CHECK(!polygon.is_rect());
    }

    SECTION("Expanding rectangular polygons");
    {
        const sc::polygon rectangle{{10, 10}, {30, 10}, {30, 30}, {10, 30}};
        CHECK(rectangle.is_rect());
        const auto expanded = rectangle.expanded(0.1);
        CHECK_EQ(expanded, (sc::polygon{{9, 9}, {31, 9}, {31, 31}, {9, 31}}));
        const sc::polygon wide_rectangle{{10, 10}, {40, 10}, {40, 30}, {10, 30}};
        CHECK_NEAR(wide_rectangle.width(), 30.0, 1e-6);
        CHECK_NEAR(wide_rectangle.height(), 20.0, 1e-6);
        const auto wider = wide_rectangle.expand_w(0.1);
        CHECK_NEAR(wider.width(), 33.0, 1.0);
        CHECK_NEAR(wider.height(), 20.0, 1.0);
        const auto taller = wide_rectangle.expand_h(3);
        CHECK_NEAR(taller.width(), 30.0, 1.0);
        CHECK_NEAR(taller.height(), 26.0, 1.0);

        const sc::polygon rotated_rectangle{{0, 0}, {12, 16}, {8, 19}, {-4, 3}};
        CHECK(rotated_rectangle.is_rect());
        CHECK_NEAR(rotated_rectangle.width(), 20.0, 1e-6);
        CHECK_NEAR(rotated_rectangle.height(), 5.0, 1e-6);
        const auto rotated_wider = rotated_rectangle.expand_w(0.2);
        CHECK_NEAR(rotated_wider.width(), 24.0, 1.0);
        CHECK_NEAR(rotated_wider.height(), 5.0, 1.0);
        const auto rotated_taller = rotated_rectangle.expand_h(5);
        CHECK_NEAR(rotated_taller.width(), 20.0, 1.0);
        CHECK_NEAR(rotated_taller.height(), 15.0, 1.0);

        const sc::polygon triangle{{0, 0}, {1, 1}, {2, 2}};
        CHECK(!triangle.is_rect());
        const sc::polygon unequal_opposite_sides{{0, 0}, {100, 0}, {95, 20}, {0, 20}};
        CHECK(!unequal_opposite_sides.is_rect());
        const sc::polygon oblique{{0, 0}, {10, 0}, {12, 10}, {2, 10}};
        CHECK(!oblique.is_rect());
        CHECK_THROWS_AS(triangle.expanded(0.1), invalid_argument);
        CHECK_THROWS_AS(rectangle.expanded(-0.1), invalid_argument);
    }

    SECTION("Rectangle, rotated rectangle, and polygon conversions");
    {
        const sc::rect rectangle{10.0, 20.0, 30.0, 10.0};
        const sc::polygon axis_aligned = static_cast<sc::polygon>(rectangle);
        CHECK_EQ(axis_aligned, (sc::polygon{{10, 20}, {40, 20}, {40, 30}, {10, 30}}));

        const sc::rotated_rect axis_rotated = static_cast<sc::rotated_rect>(rectangle);
        CHECK_EQ(axis_rotated.center(), (sc::point{25, 25}));
        CHECK_EQ(axis_rotated.size(), (sc::size{30, 10}));
        CHECK_EQ(axis_rotated.angle(), 0.0);

        const sc::polygon rotated_polygon{{0, 0}, {12, 16}, {8, 19}, {-4, 3}};
        const sc::rect bounds = static_cast<sc::rect>(rotated_polygon);
        check_edges(bounds, -4.0, 0.0, 16.0, 19.0);

        const sc::rotated_rect rotated = static_cast<sc::rotated_rect>(rotated_polygon);
        CHECK_NEAR(rotated.center().x(), 4.0, 1e-6);
        CHECK_NEAR(rotated.center().y(), 9.5, 1e-6);
        CHECK_NEAR(rotated.width(), 20.0, 1e-6);
        CHECK_NEAR(rotated.height(), 5.0, 1e-6);
        CHECK_NEAR(rotated.angle(), 53.1301, 1e-3);

        const sc::polygon round_trip = static_cast<sc::polygon>(rotated);
        const sc::rotated_rect restored = static_cast<sc::rotated_rect>(round_trip);
        CHECK_NEAR(restored.center().x(), rotated.center().x(), 1.0);
        CHECK_NEAR(restored.center().y(), rotated.center().y(), 1.0);
        CHECK_NEAR(restored.width(), rotated.width(), 1.0);
        CHECK_NEAR(restored.height(), rotated.height(), 1.0);
    }

    SECTION("Construction and accessors");
    {
        const sc::rect r{13.5, 20, 30, 40};
        check_edges(r, 13.5, 20.0, 30.0, 40.0);
        CHECK_EQ(r.center(), sc::point(28.5, 40.0));
        CHECK_EQ(r.size(), sc::size(30.0, 40.0));
        CHECK_EQ(r.area(), 1200.0);
        CHECK_EQ(r.left_top(), sc::point(13.5, 20.0));
        CHECK_EQ(r.right_bottom(), sc::point(43.5, 60.0));
        CHECK_EQ(to_text(r), string{"(13.5,20)x[30,40]"});

        const sc::rect empty;
        check_edges(empty, 0.0, 0.0, 0.0, 0.0);
        CHECK_EQ(empty.area(), 0.0);
    }

    SECTION("Construction from a point and a size");
    {
        const sc::rect r{sc::point{1, 2}, sc::size{3, 4}};
        check_edges(r, 1.0, 2.0, 3.0, 4.0);
        CHECK_EQ(r.size(), sc::size(3.0, 4.0));
    }

    SECTION("ltrb and from_points take edges, not a size");
    {
        const auto a = sc::rect::ltrb(1, 2, 11, 22);
        check_edges(a, 1.0, 2.0, 10.0, 20.0);
        const auto b = sc::rect::from_points(1, 2, 11, 22);
        check_edges(b, 1.0, 2.0, 10.0, 20.0);
        CHECK_EQ(a.area(), b.area());
    }

    SECTION("rect<int> truncates its arguments");
    {
        const sc::rect_i r{13.9, 20.7, 30.2, 40.9};
        check_edges(r, 13, 20, 30, 40);
        CHECK_EQ(r.area(), 1200);
        CHECK_EQ(to_text(r), string{"(13,20)x[30,40]"});
    }

    SECTION("Adding a scalar inflates the rect on every side");
    {
        const sc::rect r{13.5, 20, 30, 40};
        const auto inflated = r + 10.25;
        check_edges(inflated, 3.25, 9.75, 50.5, 60.5);
        check_edges(r, 13.5, 20.0, 30.0, 40.0); // operator+ does not touch the original
        // The centre is unchanged by inflation.
        CHECK_EQ(inflated.center(), r.center());

        sc::rect mutated{13.5, 20, 30, 40};
        mutated += 10.25;
        check_edges(mutated, 3.25, 9.75, 50.5, 60.5);
    }

    SECTION("Adding and subtracting another rect works component-wise");
    {
        const sc::rect r{0, 0, 10, 10};
        check_edges(r + sc::rect{1, 2, 3, 4}, 1.0, 2.0, 13.0, 14.0);
        check_edges(r - sc::rect{1, 2, 3, 4}, -1.0, -2.0, 7.0, 6.0);

        sc::rect mutated{0, 0, 10, 10};
        mutated += sc::rect{1, 2, 3, 4};
        check_edges(mutated, 1.0, 2.0, 13.0, 14.0);
        mutated -= sc::rect{1, 2, 3, 4};
        check_edges(mutated, 0.0, 0.0, 10.0, 10.0);
    }

    SECTION("Scaling by a size");
    {
        sc::rect r{10, 10, 30, 30};
        r *= sc::size{2, 3};
        check_edges(r, 20.0, 30.0, 60.0, 90.0);
        CHECK_EQ(r.area(), 5400.0);
        CHECK_EQ(r.center(), sc::point(50.0, 75.0));

        // operator* leaves the original alone.
        const sc::rect original{1, 2, 3, 4};
        check_edges(original * sc::size{2, 3}, 2.0, 6.0, 6.0, 12.0);
        check_edges(original, 1.0, 2.0, 3.0, 4.0);
        check_edges(original * sc::size{1, 1}, 1.0, 2.0, 3.0, 4.0);
    }

    SECTION("Subtracting a point moves the origin only");
    {
        const sc::rect r{10, 10, 30, 30};
        check_edges(r - sc::point{4, 6}, 6.0, 4.0, 30.0, 30.0);
    }

    SECTION("Dividing by a size scales origin and extent");
    {
        const sc::rect r{10, 20, 30, 40};
        check_edges(r / sc::size{2, 4}, 5.0, 5.0, 15.0, 10.0);
    }

    SECTION("Ordering: < is left-then-top, ^ is top-then-left");
    {
        const sc::rect origin{0, 0, 5, 5};
        const sc::rect right_of{1, 0, 5, 5};
        const sc::rect below{0, 1, 5, 5};

        CHECK(origin < right_of);
        CHECK(!(right_of < origin));
        CHECK(origin < below); // same left, so top decides
        CHECK(!(origin < origin));

        CHECK(origin ^ below);
        CHECK(!(below ^ origin));
        CHECK(origin ^ right_of); // same top, so left decides
        CHECK(!(origin ^ origin));

        // The two orderings disagree, which is the point of having both.
        const sc::rect wide{0, 10, 5, 5};
        const sc::rect tall{10, 0, 5, 5};
        CHECK(wide < tall);
        CHECK(tall ^ wide);
    }

    SECTION("intersect and iou");
    {
        const sc::rect a{0, 0, 10, 10};
        const sc::rect b{5, 5, 10, 10};
        check_edges(a.intersect(b), 5.0, 5.0, 5.0, 5.0);
        CHECK_EQ(a.intersect(b).area(), 25.0);
        // 25 overlapping out of 100 + 100 - 25 union.
        CHECK_NEAR(a.iou(b), 25.0 / 175.0, 1e-12);
        CHECK_NEAR(b.iou(a), a.iou(b), 1e-12); // symmetric
        CHECK_EQ(a.iou(a), 1.0); // identical boxes
        // Disjoint boxes have no overlap at all.
        const sc::rect far_away{100, 100, 10, 10};
        CHECK_EQ(a.iou(far_away), 0.0);
        CHECK_EQ(a.intersect(a).area(), a.area());
    }

    SECTION("gaps, overlap and distance");
    {
        const sc::rect a{0, 0, 10, 10};
        const sc::rect overlapping{5, 5, 10, 10};
        const sc::rect to_the_right{20, 0, 10, 10};
        const sc::rect diagonal{13, 14, 10, 10};

        CHECK_EQ(a.gap_x(overlapping), 0.0);
        CHECK_EQ(a.gap_y(overlapping), 0.0);
        CHECK(a.overlaps_x(overlapping));
        CHECK(a.overlaps_y(overlapping));
        CHECK_EQ(a.distance(overlapping), 0.0);

        CHECK_EQ(a.gap_x(to_the_right), 10.0);
        CHECK_EQ(a.gap_y(to_the_right), 0.0);
        CHECK(!a.overlaps_x(to_the_right));
        CHECK(a.overlaps_y(to_the_right));
        CHECK_EQ(a.distance(to_the_right), 10.0);
        CHECK_EQ(to_the_right.distance(a), a.distance(to_the_right)); // symmetric

        // 3 across and 4 down gives the 3-4-5 triangle.
        CHECK_EQ(a.gap_x(diagonal), 3.0);
        CHECK_EQ(a.gap_y(diagonal), 4.0);
        CHECK_EQ(a.distance(diagonal), 5.0);

        // Point distance: zero inside the rect, Euclidean outside.
        CHECK_EQ(a.distance(5.0, 5.0), 0.0);
        CHECK_EQ(a.distance(0.0, 0.0), 0.0);
        CHECK_EQ(a.distance(13.0, 14.0), 5.0);
        CHECK_EQ(a.distance(-3.0, -4.0), 5.0);
    }

    SECTION("group collects overlapping boxes");
    {
        const vector<sc::rect> boxes{{0, 0, 10, 10}, {5, 5, 10, 10}, {100, 100, 5, 5}};
        const auto groups = sc::rect::group(boxes, 0, -1); // link on overlap only
        CHECK_EQ(groups.size(), size_t{2});
        if (groups.size() == 2) {
            CHECK_EQ(groups[0].second, (vector<size_t>{0, 1}));
            CHECK_EQ(groups[1].second, (vector<size_t>{2}));
            // Each group rect is the union of its members.
            check_edges(groups[0].first, 0.0, 0.0, 15.0, 15.0);
            check_edges(groups[1].first, 100.0, 100.0, 5.0, 5.0);
        }
        CHECK(sc::rect::group({}, 0, -1).empty());

        // A single box is its own group.
        const auto single = sc::rect::group({{0, 0, 10, 10}}, 0, -1);
        CHECK_EQ(single.size(), size_t{1});
    }

    SECTION("group_adjacent uses per-axis thresholds");
    {
        const vector<sc::rect> boxes{{0, 0, 10, 10}, {11, 0, 10, 10}, {100, 100, 5, 5}};
        // A 1pt horizontal gap is within the 3pt threshold, so 0 and 1 merge.
        const auto merged = sc::rect::group_adjacent(boxes, 3, 3);
        CHECK_EQ(merged.size(), size_t{2});
        if (merged.size() == 2) {
            CHECK_EQ(merged[0].second, (vector<size_t>{0, 1}));
            CHECK_EQ(merged[1].second, (vector<size_t>{2}));
            check_edges(merged[0].first, 0.0, 0.0, 21.0, 10.0);
            check_edges(merged[1].first, 100.0, 100.0, 5.0, 5.0);
        }
        // Tightening the horizontal threshold below the gap splits them again.
        const auto split = sc::rect::group_adjacent(boxes, 1, 3);
        CHECK_EQ(split.size(), size_t{3});
        CHECK(sc::rect::group_adjacent({}, 3, 3).empty());
    }

    SECTION("include grows a rect to cover another");
    {
        sc::rect r{0, 0, 10, 10};
        r.include(sc::rect{20, 20, 10, 10});
        check_edges(r, 0.0, 0.0, 30.0, 30.0);

        // Including something already inside changes nothing.
        sc::rect unchanged{0, 0, 10, 10};
        unchanged.include(sc::rect{2, 2, 3, 3});
        check_edges(unchanged, 0.0, 0.0, 10.0, 10.0);
        unchanged.include(unchanged);
        check_edges(unchanged, 0.0, 0.0, 10.0, 10.0);

        // Growing towards the origin moves it.
        sc::rect grown{10, 10, 10, 10};
        grown.include(sc::rect{-5, 0, 2, 2});
        check_edges(grown, -5.0, 0.0, 25.0, 20.0);

        // Overlapping boxes union to their outer edges.
        sc::rect overlapping{0, 0, 10, 10};
        overlapping.include(sc::rect{5, 5, 10, 10});
        check_edges(overlapping, 0.0, 0.0, 15.0, 15.0);
    }

    SECTION("Subtracting a scalar deflates the rect on every side");
    {
        const sc::rect r{10, 10, 30, 30};
        check_edges(r - 5, 15.0, 15.0, 20.0, 20.0);
        check_edges(r, 10.0, 10.0, 30.0, 30.0); // operator- leaves the original alone
        // The centre is unchanged by deflation, as it is by inflation.
        CHECK_EQ((r - 5).center(), r.center());
        CHECK_EQ((r - 5).area(), 400.0);
        // Deflating by zero changes nothing.
        check_edges(r - 0, 10.0, 10.0, 30.0, 30.0);

        sc::rect mutated{10, 10, 30, 30};
        mutated -= 5;
        check_edges(mutated, 15.0, 15.0, 20.0, 20.0);

        // Inflating and deflating by the same amount is a round trip, either way round.
        sc::rect there_and_back{10, 10, 30, 30};
        there_and_back += 5;
        there_and_back -= 5;
        check_edges(there_and_back, 10.0, 10.0, 30.0, 30.0);

        sc::rect back_and_there{10, 10, 30, 30};
        back_and_there -= 5;
        back_and_there += 5;
        check_edges(back_and_there, 10.0, 10.0, 30.0, 30.0);

        const sc::rect_i integral{10, 10, 30, 30};
        check_edges(integral - 5, 15, 15, 20, 20);
        check_edges(integral + 5, 5, 5, 40, 40);
    }

    SECTION("A negative scalar reverses the direction");
    {
        sc::rect inflated{10, 10, 30, 30};
        inflated -= -5;
        check_edges(inflated, 5.0, 5.0, 40.0, 40.0);

        sc::rect deflated{10, 10, 30, 30};
        deflated += -5;
        check_edges(deflated, 15.0, 15.0, 20.0, 20.0);

        const sc::rect r{10, 10, 30, 30};
        check_edges(r - -5, 5.0, 5.0, 40.0, 40.0);
        check_edges(r + -5, 15.0, 15.0, 20.0, 20.0);

        // A negative scalar is just the other operator, so these agree.
        check_edges(r - -5, 5.0, 5.0, 40.0, 40.0);
        check_edges(r + 5, 5.0, 5.0, 40.0, 40.0);
    }

    SECTION("Sizes stay non-negative through arithmetic, not just construction");
    {
        // size_::on_validate is the invariant; it has to hold after mutation too.
        sc::size shrunk{10, 20};
        shrunk -= 15;
        CHECK_EQ(shrunk, sc::size(0.0, 5.0));
        CHECK_EQ(shrunk.area(), 0.0);

        sc::size negated{10, 20};
        negated *= -1;
        CHECK_EQ(negated, sc::size(0.0, 0.0));

        CHECK_EQ(sc::size(10, 20) - 15, sc::size(0.0, 5.0));
        CHECK_EQ(sc::size(10, 20) - sc::size(30, 30), sc::size(0.0, 0.0));

        sc::size component_wise{10, 20};
        component_wise -= sc::size{30, 5};
        CHECK_EQ(component_wise, sc::size(0.0, 15.0));

        const sc::size_i integral = sc::size_i{10, 20} - 15;
        CHECK_EQ(integral, sc::size_i(0, 5));
        CHECK_EQ(integral.area(), 0);

        // Points carry no such invariant and stay signed.
        CHECK_EQ(sc::point(10, 20) - 15, sc::point(-5.0, 5.0));
        CHECK_EQ(sc::point(10, 20) * -1, sc::point(-10.0, -20.0));
    }

    SECTION("Over-deflating collapses to an empty rect");
    {
        const sc::rect r{10, 10, 10, 10};
        CHECK_EQ((r - 20).area(), 0.0);
        CHECK_EQ((r - 20).size(), sc::size(0.0, 0.0));
        CHECK_EQ((r - 5).area(), 0.0); // exactly consumed
        CHECK_EQ((r - 4).area(), 4.0);

        sc::rect mutated{10, 10, 10, 10};
        mutated -= 20;
        CHECK_EQ(mutated.width(), 0.0);
        CHECK_EQ(mutated.height(), 0.0);
        CHECK_EQ(mutated.area(), 0.0);
        // Once collapsed the rect cannot be inflated back to what it was: the
        // extent is gone, only the origin still moves.
        mutated += 20;
        CHECK_EQ(mutated.area(), 1600.0);

        // A rect that is already empty stays empty.
        const sc::rect empty;
        CHECK_EQ((empty - 1).area(), 0.0);
    }

    TEST_SUMMARY();
}
