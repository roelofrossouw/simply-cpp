#pragma once
#include "point.h"
#include "rect.h"
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
    template<Numeric T>
    class rotated_rect_;

    /// A list of corner points: polygon (double) or polygon_i (whole pixels), like point and rect.
    /// An int polygon converts to a double one implicitly; the other way rounds, so it's explicit.
    template<Numeric T>
    class polygon_ {
    public:
        using point_type = point_<T>;
        using iterator = typename std::vector<point_type>::iterator;
        using const_iterator = typename std::vector<point_type>::const_iterator;

        polygon_() = default;

        polygon_(std::initializer_list<point_type> points) : points_(points) {
        }

        explicit polygon_(std::vector<point_type> points) : points_(std::move(points)) {
        }

        template<Numeric U> requires (!std::same_as<T, U>)
        explicit(std::integral<T> && std::floating_point<U>) polygon_(const polygon_<U> &other) {
            points_.reserve(other.size());
            for (const auto &point: other)
                points_.emplace_back(detail::coordinate<T>(static_cast<double>(point.x())),
                                     detail::coordinate<T>(static_cast<double>(point.y())));
        }

        [[nodiscard]] size_t size() const noexcept { return points_.size(); }
        [[nodiscard]] bool empty() const noexcept { return points_.empty(); }
        [[nodiscard]] point_type &operator[](const size_t index) { return points_[index]; }
        [[nodiscard]] const point_type &operator[](const size_t index) const { return points_[index]; }
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

        /// The area it covers: the axis-aligned box around its points (empty without points).
        [[nodiscard]] rect_<T> bounds() const { return static_cast<rect_<T>>(*this); }

        /// Returns the axis-aligned bounding box of this polygon's points.
        template<typename U>
        [[nodiscard]] operator rect_<U>() const {
            if (points_.empty()) return {};
            T left = points_.front().x();
            T right = left;
            T top = points_.front().y();
            T bottom = top;
            for (const auto &point: points_) {
                left = std::min(left, point.x());
                right = std::max(right, point.x());
                top = std::min(top, point.y());
                bottom = std::max(bottom, point.y());
            }
            return {
                static_cast<U>(left),
                static_cast<U>(top),
                static_cast<U>(right - left),
                static_cast<U>(bottom - top)
            };
        }

        /// Recovers a rotated rect from its four corners - throws if they aren't one.
        template<Numeric U>
        [[nodiscard]] operator rotated_rect_<U>() const {
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
            return rotated_rect_<U>{
                point_<U>{static_cast<U>(center_x), static_cast<U>(center_y)},
                size_<U>{static_cast<U>(width), static_cast<U>(height)},
                static_cast<U>(angle)
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

        /// Expands the longer dimension by a fraction (<= 1) or points per side (> 1).
        [[nodiscard]] polygon_ expand_w(const double amount) const {
            return expand_dimensions(amount, 0);
        }

        /// Expands the shorter dimension by a fraction (<= 1) or points per side (> 1).
        [[nodiscard]] polygon_ expand_h(const double amount) const {
            return expand_dimensions(0, amount);
        }

        /// Expands both dimensions by a fraction (<= 1) or points per side (> 1).
        [[nodiscard]] polygon_ expanded(const double amount) const {
            return expand_dimensions(amount, amount);
        }

        bool operator==(const polygon_ &) const = default;

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

        [[nodiscard]] polygon_ expand_dimensions(const double width_amount, const double height_amount) const {
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

            polygon_ result;
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
            points_.emplace_back(detail::coordinate<T>(x), detail::coordinate<T>(y));
        }

        std::vector<point_type> points_;
    };

    using polygon = polygon_<double>;
    using polygon_i = polygon_<int>;

    // rect.h declares this; it's here, where polygon_ is complete.
    template<typename T>
    template<Numeric U>
    rect_<T>::operator polygon_<U>() const {
        const auto corner = [](const T x, const T y) {
            return point_<U>{detail::coordinate<U>(static_cast<double>(x)), detail::coordinate<U>(static_cast<double>(y))};
        };
        return polygon_<U>{corner(left(), top()), corner(right(), top()), corner(right(), bottom()), corner(left(), bottom())};
    }
} // namespace sc
