#pragma once
#include "point.h"
#include "size.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <ranges>
#include <stdexcept>
#include <utility>
#include <vector>

namespace sc {
    template<typename T>
    class rect_;

    template<Numeric T>
    class rotated_rect_;

    class polygon {
    public:
        using iterator = std::vector<point_i>::iterator;
        using const_iterator = std::vector<point_i>::const_iterator;

        polygon() = default;

        polygon(std::initializer_list<point_i> points) : points_(points) {
        }

        explicit polygon(std::vector<point_i> points) : points_(std::move(points)) {
        }

        [[nodiscard]] size_t size() const noexcept { return points_.size(); }
        [[nodiscard]] bool empty() const noexcept { return points_.empty(); }
        [[nodiscard]] point_i &operator[](const size_t index) { return points_[index]; }
        [[nodiscard]] const point_i &operator[](const size_t index) const { return points_[index]; }
        [[nodiscard]] iterator begin() noexcept { return points_.begin(); }
        [[nodiscard]] iterator end() noexcept { return points_.end(); }
        [[nodiscard]] const_iterator begin() const noexcept { return points_.begin(); }
        [[nodiscard]] const_iterator end() const noexcept { return points_.end(); }

        [[nodiscard]] bool is_rect() const noexcept {
            if (points_.size() != 4) return false;

            std::array<double, 4> edge_lengths{};
            for (size_t i = 0; i < points_.size(); ++i) {
                const auto &from = points_[i];
                const auto &to = points_[(i + 1) % points_.size()];
                edge_lengths[i] = std::hypot(static_cast<double>(to.x()) - from.x(),
                                             static_cast<double>(to.y()) - from.y());
                if (edge_lengths[i] == 0) return false;
            }

            const auto opposite_sides_match = [](const double lhs, const double rhs) {
                return std::abs(lhs - rhs) / std::max(lhs, rhs) <= 0.05;
            };
            if (!opposite_sides_match(edge_lengths[0], edge_lengths[2]) ||
                !opposite_sides_match(edge_lengths[1], edge_lengths[3]))
                return false;

            const double first_x = static_cast<double>(points_[0].x()) - points_[1].x();
            const double first_y = static_cast<double>(points_[0].y()) - points_[1].y();
            const double second_x = static_cast<double>(points_[2].x()) - points_[1].x();
            const double second_y = static_cast<double>(points_[2].y()) - points_[1].y();
            const double cosine = (first_x * second_x + first_y * second_y) /
                                  (edge_lengths[0] * edge_lengths[1]);
            const double angle = std::acos(std::clamp(cosine, -1.0, 1.0)) * 180.0 / std::acos(-1.0);
            return angle >= 80.0 && angle <= 100.0;
        }

        /// Returns the axis-aligned bounding box of this polygon's points.
        template<Numeric T>
        [[nodiscard]] operator rect_<T>() const {
            if (points_.empty()) return {};
            int left = points_.front().x();
            int right = left;
            int top = points_.front().y();
            int bottom = top;
            for (const auto &point: points_) {
                left = std::min(left, point.x());
                right = std::max(right, point.x());
                top = std::min(top, point.y());
                bottom = std::max(bottom, point.y());
            }
            return {
                static_cast<T>(left),
                static_cast<T>(top),
                static_cast<T>(right - left),
                static_cast<T>(bottom - top)
            };
        }

        /// Recovers a rotated rect from its four corners - throws if they aren't one.
        template<Numeric T>
        [[nodiscard]] operator rotated_rect_<T>() const {
            if (points_.size() != 4)
                throw std::invalid_argument{"Rotated rectangle conversion requires four polygon points"};

            std::array<double, 4> edge_lengths{};
            std::array<point, 4> midpoints{};
            for (size_t i = 0; i < 4; ++i) {
                const auto &from = points_[i];
                const auto &to = points_[(i + 1) % 4];
                edge_lengths[i] = std::hypot(static_cast<double>(to.x()) - from.x(),
                                             static_cast<double>(to.y()) - from.y());
                midpoints[i] = {
                    (static_cast<double>(from.x()) + to.x()) / 2,
                    (static_cast<double>(from.y()) + to.y()) / 2
                };
            }
            if (std::ranges::any_of(edge_lengths, [](const double length) { return length == 0; }))
                throw std::invalid_argument{"Cannot convert polygon with zero-length sides to rotated rectangle"};

            const double pair_02 = edge_lengths[0] + edge_lengths[2];
            const double pair_13 = edge_lengths[1] + edge_lengths[3];
            const bool edges_02_are_short = pair_02 <= pair_13;
            const size_t first_short_edge = edges_02_are_short ? 0 : 1;
            const size_t second_short_edge = edges_02_are_short ? 2 : 3;
            const double width = edges_02_are_short
                                     ? (edge_lengths[1] + edge_lengths[3]) / 2
                                     : (edge_lengths[0] + edge_lengths[2]) / 2;
            const double height = edges_02_are_short ? pair_02 / 2 : pair_13 / 2;

            const double axis_x = midpoints[second_short_edge].x() - midpoints[first_short_edge].x();
            const double axis_y = midpoints[second_short_edge].y() - midpoints[first_short_edge].y();
            double angle = std::atan2(axis_y, axis_x) * 180.0 / std::acos(-1.0);
            if (angle < -90) angle += 180;
            else if (angle >= 90) angle -= 180;
            const double center_x = (static_cast<double>(points_[0].x()) + points_[1].x() +
                                     points_[2].x() + points_[3].x()) / 4;
            const double center_y = (static_cast<double>(points_[0].y()) + points_[1].y() +
                                     points_[2].y() + points_[3].y()) / 4;
            return rotated_rect_<T>{
                point_<T>{static_cast<T>(center_x), static_cast<T>(center_y)},
                size_<T>{static_cast<T>(width), static_cast<T>(height)},
                static_cast<T>(angle)
            };
        }

        [[nodiscard]] double width() const {
            const auto dimensions = rectangle_dimensions();
            return std::max(dimensions.edge_01, dimensions.edge_12);
        }

        [[nodiscard]] double height() const {
            const auto dimensions = rectangle_dimensions();
            return std::min(dimensions.edge_01, dimensions.edge_12);
        }

        /// Expands the longer dimension by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expand_w(const double amount) const {
            return expand_dimensions(amount, 0);
        }

        /// Expands the shorter dimension by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expand_h(const double amount) const {
            return expand_dimensions(0, amount);
        }

        /// Expands both dimensions by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expanded(const double amount) const {
            return expand_dimensions(amount, amount);
        }

        bool operator==(const polygon &) const = default;

    private:
        struct rectangle_geometry {
            double center_x;
            double center_y;
            double width_axis_x;
            double width_axis_y;
            double height_axis_x;
            double height_axis_y;
            double edge_01;
            double edge_12;
        };

        [[nodiscard]] rectangle_geometry rectangle_dimensions() const {
            if (points_.size() != 4)
                throw std::invalid_argument{"Rotated rectangle dimensions require a four-point polygon"};

            const auto edge = [this](const size_t start) {
                const auto &from = points_[start];
                const auto &to = points_[(start + 1) % points_.size()];
                return std::pair{
                    static_cast<double>(to.x()) - from.x(),
                    static_cast<double>(to.y()) - from.y()
                };
            };
            const auto [edge_01_x, edge_01_y] = edge(0);
            const auto [edge_12_x, edge_12_y] = edge(1);
            const double edge_01 = std::hypot(edge_01_x, edge_01_y);
            const double edge_12 = std::hypot(edge_12_x, edge_12_y);
            if (!is_rect()) throw std::invalid_argument{"Polygon is not a rotated rectangle"};

            const double center_x = (static_cast<double>(points_[0].x()) + points_[1].x() + points_[2].x() + points_[3].x()) / 4;
            const double center_y = (static_cast<double>(points_[0].y()) + points_[1].y() + points_[2].y() + points_[3].y()) / 4;
            const bool edge_01_is_width = edge_01 >= edge_12;
            const double width_edge = edge_01_is_width ? edge_01 : edge_12;
            const double height_edge = edge_01_is_width ? edge_12 : edge_01;
            const double width_dx = edge_01_is_width ? edge_01_x : edge_12_x;
            const double width_dy = edge_01_is_width ? edge_01_y : edge_12_y;
            const double height_dx = edge_01_is_width ? edge_12_x : edge_01_x;
            const double height_dy = edge_01_is_width ? edge_12_y : edge_01_y;
            return {
                center_x,
                center_y,
                width_dx / width_edge,
                width_dy / width_edge,
                height_dx / height_edge,
                height_dy / height_edge,
                edge_01,
                edge_12
            };
        }

        [[nodiscard]] polygon expand_dimensions(const double width_amount, const double height_amount) const {
            if (!std::isfinite(width_amount) || width_amount < 0 ||
                !std::isfinite(height_amount) || height_amount < 0)
                throw std::invalid_argument{"Polygon expansion must be finite and non-negative"};
            const auto geometry = rectangle_dimensions();
            const double width_edge = std::max(geometry.edge_01, geometry.edge_12);
            const double height_edge = std::min(geometry.edge_01, geometry.edge_12);
            const auto scale = [](const double amount, const double edge) {
                return amount <= 1 ? 1 + amount : (edge + 2 * amount) / edge;
            };
            const double width_scale = scale(width_amount, width_edge);
            const double height_scale = scale(height_amount, height_edge);

            polygon result;
            for (const auto &point: points_) {
                const double offset_x = point.x() - geometry.center_x;
                const double offset_y = point.y() - geometry.center_y;
                const double width_projection =
                        offset_x * geometry.width_axis_x + offset_y * geometry.width_axis_y;
                const double height_projection =
                        offset_x * geometry.height_axis_x + offset_y * geometry.height_axis_y;
                const double x = geometry.center_x +
                                 width_projection * width_scale * geometry.width_axis_x +
                                 height_projection * height_scale * geometry.height_axis_x;
                const double y = geometry.center_y +
                                 width_projection * width_scale * geometry.width_axis_y +
                                 height_projection * height_scale * geometry.height_axis_y;
                result.add_checked(x, y);
            }
            return result;
        }

        void add_checked(const double x, const double y) {
            if (!std::isfinite(x) || !std::isfinite(y) ||
                x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
                y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
                throw std::overflow_error{"Expanded polygon coordinates exceed integer range"};
            points_.emplace_back(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)));
        }

        std::vector<point_i> points_;
    };
} // namespace sc
