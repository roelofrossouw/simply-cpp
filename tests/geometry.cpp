#include <geometry.h>

#include <sc_test.h>
#include <array>
#include <cmath>
#include <vector>
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

    SECTION("Every shape's bounds: the area it covers");
    {
        const auto near = [](const sc::rect &a, const sc::rect &b) {
            return std::abs(a.left() - b.left()) < 1e-9 && std::abs(a.top() - b.top()) < 1e-9 && std::abs(a.width() - b.width()) < 1e-9 &&
                   std::abs(a.height() - b.height()) < 1e-9;
        };
        CHECK(near(sc::rect{1, 2, 3, 4}.bounds(), sc::rect{1, 2, 3, 4}));
        CHECK(near(sc::rounded_rect{sc::rect{1, 2, 3, 4}, 1}.bounds(), sc::rect{1, 2, 3, 4}));
        CHECK(near(sc::circle{{10, 20}, 5}.bounds(), sc::rect{5, 15, 10, 10}));
        CHECK(near(sc::line{{8, 1}, {2, 5}}.bounds(), sc::rect{2, 1, 6, 4})); // either direction
        CHECK(near(sc::polygon{{0, 0}, {4, 1}, {2, 3}}.bounds(), sc::rect{0, 0, 4, 3}));
        CHECK(near(sc::rotated_rect{{0, 0}, {2, 2}, 45}.bounds(), sc::rect{-std::sqrt(2.0), -std::sqrt(2.0), 2 * std::sqrt(2.0), 2 * std::sqrt(2.0)}));
        const sc::rect_i whole = sc::circle_i{{10, 10}, 3}.bounds(); // each flavour its own
        CHECK_EQ(whole.width(), 6);

        // A path's bounds are exact: this curve's control points reach y = 10, the curve only 7.5.
        sc::path arch;
        arch.move_to({0, 0}).cubic_to({0, 10}, {10, 10}, {10, 0});
        CHECK(near(arch.bounds(), sc::rect{0, 0, 10, 7.5}));
        sc::path s_curve; // turning in x too: it bulges past both ends
        s_curve.move_to({0, 0}).cubic_to({10, 0}, {-10, 10}, {0, 10});
        CHECK_BETWEEN(s_curve.bounds().right(), 2.8, 2.9); // max x = 2.887 at t = 0.21
        CHECK_BETWEEN(s_curve.bounds().left(), -2.9, -2.8);
        CHECK(near(sc::path{}.bounds(), sc::rect{}));
    }

    SECTION("Every shape as a path of the same outline");
    {
        const auto near = [](const sc::rect &a, const sc::rect &b) {
            return std::abs(a.left() - b.left()) < 1e-6 && std::abs(a.top() - b.top()) < 1e-6 && std::abs(a.width() - b.width()) < 1e-6 &&
                   std::abs(a.height() - b.height()) < 1e-6;
        };
        const sc::circle disc{{10, 20}, 5};
        CHECK(near(to_path(disc).bounds(), disc.bounds()));
        CHECK_EQ(to_path(disc).steps().size(), std::size_t{6}); // a move, four curves, a close
        const sc::rounded_rect card{sc::rect{0, 0, 100, 50}, 10};
        CHECK(near(to_path(card).bounds(), card.bounds()));
        CHECK(near(to_path(sc::rounded_rect{sc::rect{0, 0, 100, 50}, 500}).bounds(), sc::rect{0, 0, 100, 50})); // radius at most 25
        CHECK_EQ(to_path(sc::rect_i{1, 2, 3, 4}).steps().size(), std::size_t{5});
        CHECK(to_path(sc::line{{0, 0}, {5, 5}}).steps().back().type == sc::path::kind::line); // open: no close
        CHECK(to_path(sc::polygon{}).empty());
        CHECK(to_path(sc::circle{}).empty());
        CHECK(near(to_path(sc::rotated_rect{{0, 0}, {2, 2}, 45}).bounds(), sc::rotated_rect{{0, 0}, {2, 2}, 45}.bounds()));
    }

    SECTION("sc::shape: any of them, in one collection");
    {
        const std::vector<sc::shape> marks{sc::rect{10, 10, 50, 20}, sc::circle{{40, 40}, 5}, sc::line{{0, 0}, {9, 9}}};
        CHECK_EQ(marks[1].bounds().left(), 35.0);
        CHECK(std::holds_alternative<sc::line>(marks[2].value()));
        CHECK_EQ(to_path(marks[0]).steps().size(), std::size_t{5});
        const sc::shape from_int = sc::rect{sc::rect_i{1, 2, 3, 4}}; // an int shape converts first
        CHECK_EQ(from_int.bounds().width(), 3.0);
    }

    TEST_SUMMARY();
}
