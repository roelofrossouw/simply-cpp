#pragma once

#include "rect.h"

#include <cmath>
#include <stdexcept>

namespace sc {
    /// A rectangle with rounded corners: the rectangle, and the corners' radius. A canvas draws the
    /// radius as at most half the rectangle's shorter side, and a radius of 0 as square corners.
    ///
    ///     canvas.draw(sc::rounded_rect{sc::rect{50, 130, 200, 80}, 8}, {sc::color::Black, sc::color{0.9, 0.95, 1}});
    template<Numeric T>
    class rounded_rect_ {
    public:
        rounded_rect_(rect_<T> rect = {}, T radius = 0) : rect_value_(rect), radius_(radius) {
            validate_radius(radius_);
        }

        /// From the other flavour: rounded_rect_i to rounded_rect implicitly; rounded_rect to
        /// rounded_rect_i explicitly, rounding the rectangle and the radius (std::overflow_error when
        /// one doesn't fit an int).
        template<Numeric U> requires (!std::same_as<T, U>)
        explicit(std::integral<T> && std::floating_point<U>) rounded_rect_(const rounded_rect_<U> &other)
            : rounded_rect_(rect_<T>(other.rect()), detail::coordinate<T>(static_cast<double>(other.radius()))) {
        }

        [[nodiscard]] rect_<T> rect() const { return rect_value_; }
        rect_<T> rect(const rect_<T> &value) { return rect_value_ = value; }

        /// Its rectangle.
        [[nodiscard]] rect_<T> bounds() const { return rect_value_; }
        [[nodiscard]] T radius() const { return radius_; }

        T radius(const T value) {
            validate_radius(value);
            return radius_ = value;
        }

        bool operator==(const rounded_rect_ &) const = default;

    private:
        static void validate_radius(const T radius) {
            if constexpr (std::floating_point<T>) {
                if (!std::isfinite(radius) || radius < 0)
                    throw std::invalid_argument{"Rounded rectangle radius must be finite and non-negative"};
            } else if (radius < 0) {
                throw std::invalid_argument{"Rounded rectangle radius must be non-negative"};
            }
        }

        rect_<T> rect_value_;
        T radius_;
    };

    using rounded_rect = rounded_rect_<double>;
    using rounded_rect_i = rounded_rect_<int>;
}
