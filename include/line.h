#pragma once

#include "point.h"
#include "rect.h"

#include <algorithm>

#include <cmath>
#include <concepts>

namespace sc {
    /// A straight line from one point to another: line (double) or line_i (whole pixels). An int
    /// line converts to a double one implicitly; the other way rounds, so it's explicit.
    template<Numeric T>
    class line_ {
    public:
        line_(point_<T> from = {}, point_<T> to = {}) : from_(from), to_(to) {
        }

        template<Numeric U> requires (!std::same_as<T, U>)
        explicit(std::integral<T> && std::floating_point<U>) line_(const line_<U> &other)
            : from_(detail::coordinate<T>(static_cast<double>(other.from().x())), detail::coordinate<T>(static_cast<double>(other.from().y()))),
              to_(detail::coordinate<T>(static_cast<double>(other.to().x())), detail::coordinate<T>(static_cast<double>(other.to().y()))) {
        }

        [[nodiscard]] point_<T> from() const { return from_; }
        point_<T> from(const point_<T> &value) { return from_ = value; }
        [[nodiscard]] point_<T> to() const { return to_; }
        point_<T> to(const point_<T> &value) { return to_ = value; }

        [[nodiscard]] double length() const {
            return std::hypot(static_cast<double>(to_.x()) - from_.x(), static_cast<double>(to_.y()) - from_.y());
        }

        /// The box with the line as its diagonal (no width).
        [[nodiscard]] rect_<T> bounds() const {
            return rect_<T>::ltrb(std::min(from_.x(), to_.x()), std::min(from_.y(), to_.y()), std::max(from_.x(), to_.x()), std::max(from_.y(), to_.y()));
        }

        bool operator==(const line_ &) const = default;

    private:
        point_<T> from_;
        point_<T> to_;
    };

    using line = line_<double>;
    using line_i = line_<int>;
}
