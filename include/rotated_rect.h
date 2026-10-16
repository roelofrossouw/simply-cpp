#pragma once
#include "point.h"
#include "polygon.h"
#include "size.h"

#include <array>
#include <cmath>
#include <ostream>
#include <vector>

namespace sc {
    template<typename T>
    class rect_;

    /// A rectangle turned about its centre: centre, size and angle in degrees (clockwise on screen,
    /// as y grows downwards). rotated_rect (double) or rotated_rect_i (whole pixels).
    template<Numeric T>
    class rotated_rect_ {
    public:
        rotated_rect_(point_<T> center = {}, size_<T> size = {}, T angle = 0)
            : center_(center), rect_size_(size), angle_(angle) {
        }

        explicit rotated_rect_(const rect_<T> &rectangle)
            : center_(rectangle.center()), rect_size_(rectangle.size()), angle_(0) {
        }

        [[nodiscard]] point_<T> center() const { return center_; }
        point_<T> center(const point_<T> &value) { return center_ = value; }
        [[nodiscard]] size_<T> size() const { return rect_size_; }
        size_<T> size(const size_<T> &value) { return rect_size_ = value; }
        [[nodiscard]] T width() const { return rect_size_.width(); }
        [[nodiscard]] T height() const { return rect_size_.height(); }
        [[nodiscard]] T area() const { return rect_size_.area(); }
        [[nodiscard]] T left() const { return static_cast<rect_<T>>(*this).left(); }
        [[nodiscard]] T top() const { return static_cast<rect_<T>>(*this).top(); }
        [[nodiscard]] T right() const { return static_cast<rect_<T>>(*this).right(); }
        [[nodiscard]] T bottom() const { return static_cast<rect_<T>>(*this).bottom(); }
        [[nodiscard]] T angle() const { return angle_; }
        T angle(const T value) { return angle_ = value; }

        /// The upright box around its corners.
        [[nodiscard]] rect_<T> bounds() const { return static_cast<rect_<T>>(*this); }

        [[nodiscard]] operator rect_<T>() const {
            const double radians = static_cast<double>(angle_) * std::acos(-1.0) / 180.0;
            const double cosine = std::abs(std::cos(radians));
            const double sine = std::abs(std::sin(radians));
            const double bounds_width = cosine * width() + sine * height();
            const double bounds_height = sine * width() + cosine * height();
            return {
                static_cast<T>(center_.x() - bounds_width / 2),
                static_cast<T>(center_.y() - bounds_height / 2),
                static_cast<T>(bounds_width),
                static_cast<T>(bounds_height)
            };
        }

        /// Its corners; rounded for polygon_i, throwing std::overflow_error when one doesn't fit an int.
        template<Numeric U>
        [[nodiscard]] operator polygon_<U>() const;

        bool operator==(const rotated_rect_ &) const = default;

    private:
        friend std::ostream &operator<<(std::ostream &lhs, const rotated_rect_ &rhs) {
            return lhs << rhs.center_ << "x" << rhs.rect_size_ << "@" << rhs.angle_;
        }

        point_<T> center_;
        size_<T> rect_size_;
        T angle_;
    };

    using rotated_rect = rotated_rect_<double>;
    using rotated_rect_i = rotated_rect_<int>;

    template<Numeric T>
    template<Numeric U>
    rotated_rect_<T>::operator polygon_<U>() const {
        const double radians = static_cast<double>(angle_) * std::acos(-1.0) / 180.0;
        const double cosine = std::cos(radians);
        const double sine = std::sin(radians);
        const double half_width = static_cast<double>(width()) / 2;
        const double half_height = static_cast<double>(height()) / 2;
        const std::array<point, 4> local_points = {
            point{-half_width, -half_height},
            point{half_width, -half_height},
            point{half_width, half_height},
            point{-half_width, half_height}
        };
        std::vector<point_<U> > points;
        points.reserve(local_points.size());
        for (const auto &local: local_points) {
            const double x = center_.x() + local.x() * cosine - local.y() * sine;
            const double y = center_.y() + local.x() * sine + local.y() * cosine;
            points.emplace_back(detail::coordinate<U>(x), detail::coordinate<U>(y));
        }
        return polygon_<U>{std::move(points)};
    }
} // namespace sc
