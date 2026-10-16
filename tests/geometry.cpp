#include <geometry.h>

#include <sc_test.h>
#include <array>
#include <limits>
#include <stdexcept>

int main() {
    SECTION("Clustering numeric values");
    {
        const std::vector<double> values{0.0, 0.1, 0.2, 5.0, 5.1, 20.0};
        CHECK_EQ(sc::dbscan(values, 0.25, 3), (std::vector<int>{0, 0, 0, -1, -1, -1}));

        const std::vector<double> angles{1.0, 179.0, 90.0};
        CHECK_EQ(sc::dbscan(angles, 3.0, 2, 180.0), (std::vector<int>{0, 0, -1}));
        CHECK_THROWS_AS(sc::dbscan(values, -1.0, 2), std::invalid_argument);
        CHECK_THROWS_AS(sc::dbscan(values, 1.0, 0), std::invalid_argument);
        CHECK_THROWS_AS(sc::dbscan(values, 1.0, 2, -1.0), std::invalid_argument);
        CHECK_THROWS_AS(sc::dbscan(
                            std::vector<double>{0.0, std::numeric_limits<double>::infinity()}, 1.0, 1),
                        std::invalid_argument);
    }

    SECTION("Clustering with a custom distance");
    {
        const std::vector<std::array<double, 2>> points{{0, 0}, {1, 0}, {10, 10}};
        const auto labels = sc::dbscan_by(points, 1.1, 2, [](const auto &lhs, const auto &rhs) {
            return std::hypot(lhs[0] - rhs[0], lhs[1] - rhs[1]);
        });
        CHECK_EQ(labels, (std::vector<int>{0, 0, -1}));
        CHECK_THROWS_AS(sc::dbscan_by(points, 1.1, 2, [](const auto &, const auto &) { return -1.0; }),
                        std::invalid_argument);
    }

    SECTION("Circle geometry");
    {
        const sc::circle circle{{3.5, 4.5}, 2.25};
        CHECK_EQ(circle.center(), (sc::point{3.5, 4.5}));
        CHECK_EQ(circle.radius(), 2.25);
        CHECK_THROWS_AS((sc::circle{{}, -1.0}), std::invalid_argument);
        CHECK_THROWS_AS((sc::circle{{}, std::numeric_limits<double>::infinity()}), std::invalid_argument);
    }

    SECTION("Lines");
    {
        const sc::line diagonal{{0, 0}, {3, 4}};
        CHECK_EQ(diagonal.length(), 5.0);
        CHECK_EQ(diagonal.to(), (sc::point{3, 4}));
        const sc::line widened = sc::line_i{{1, 2}, {3, 4}}; // int to double: implicit
        CHECK_EQ(widened.from(), (sc::point{1, 2}));
        const sc::line_i rounded(sc::line{{0.4, 1.6}, {2.5, -0.5}}); // rounded, so explicit
        CHECK_EQ(rounded, (sc::line_i{{0, 2}, {3, -1}}));
    }

    SECTION("Rounded rectangles: a rectangle and a radius, in either flavour");
    {
        const sc::rounded_rect box{sc::rect{10, 20, 200, 80}, 8};
        CHECK_EQ(box.rect().width(), 200.0);
        CHECK_EQ(box.radius(), 8.0);
        CHECK(box == (sc::rounded_rect{sc::rect{10, 20, 200, 80}, 8}));
        CHECK(box != (sc::rounded_rect{sc::rect{10, 20, 200, 80}, 9}));
        const sc::rounded_rect_i whole{box};                 // double to int: explicit, rounded
        CHECK_EQ(whole.radius(), 8);
        const sc::rounded_rect back = sc::rounded_rect_i{sc::rect_i{1, 2, 3, 4}, 1}; // int to double: implicit
        CHECK_EQ(back.rect().height(), 4.0);
        CHECK_EQ(sc::rounded_rect{}.radius(), 0.0);
        CHECK_THROWS_AS((sc::rounded_rect{sc::rect{0, 0, 10, 10}, -1}), std::invalid_argument);
    }

    TEST_SUMMARY();
}
