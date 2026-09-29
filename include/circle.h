#pragma once

#include "rect.h"

#include <cmath>
#include <stdexcept>

namespace sc {
    template<Numeric T>
    class circle_ {
    public:
        circle_(point_<T> center = {}, T radius = 0) : center_(center), radius_(radius) {
            validate_radius(radius_);
        }

        [[nodiscard]] point_<T> center() const { return center_; }
        point_<T> center(const point_<T> &value) { return center_ = value; }
        [[nodiscard]] T radius() const { return radius_; }
        T radius(const T value) {
            validate_radius(value);
            return radius_ = value;
        }

        bool operator==(const circle_ &) const = default;

    private:
        static void validate_radius(const T radius) {
            if constexpr (std::floating_point<T>) {
                if (!std::isfinite(radius) || radius < 0)
                    throw std::invalid_argument{"Circle radius must be finite and non-negative"};
            } else if (radius < 0) {
                throw std::invalid_argument{"Circle radius must be non-negative"};
            }
        }

        point_<T> center_;
        T radius_;
    };

    using circle = circle_<double>;
    using circle_i = circle_<int>;
}
