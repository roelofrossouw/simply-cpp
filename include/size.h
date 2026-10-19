#pragma once
#include "point.h"

#include <ostream>

namespace sc {
    /// A width and height: size (double) or size_i (whole pixels). Never negative: a value below 0,
    /// given or worked out, becomes 0.
    template<typename T>
    class size_ : public pair_<size_<T>, T> {
    public:
        using pair_<size_<T>, T>::pair_;

        /// The same size in a unit, for a size of distances: pdf.size().in<sc::dist::mm>() is
        /// [210mm,297mm], the same as static_cast<sc::size_<sc::dist::mm>>(pdf.size()).
        template<typename Unit>
            requires(std::derived_from<T, distance> && std::derived_from<Unit, distance>)
        [[nodiscard]] size_<Unit> in() const {
            return static_cast<size_<Unit>>(*this);
        }

        /// The same size in another unit (or distances from numbers of points):
        /// static_cast<sc::size_<sc::dist::mm>>(pdf.size()) is [210mm,297mm].
        template<Coordinate U>
            requires(!std::same_as<U, T> && !std::convertible_to<U, T> && std::constructible_from<T, U>)
        explicit size_(const size_<U> &other) : size_(T(other.width()), T(other.height())) {
        }

        /// A size of numbers of points and one of distances are the same, either way; a unit
        /// becomes a plain distance. (Not double to int: that rounds, and is explicit, as above.)
        template<Coordinate U>
            requires(!std::same_as<U, T> && std::convertible_to<T, U> && !(std::floating_point<T> && std::integral<U>))
        operator size_<U>() const {
            return {static_cast<U>(this->x_), static_cast<U>(this->y_)};
        }

        operator point_<T>() { return {this->x_, this->y_}; }
        T width() const { return this->x_; }
        T width(const T &r) { return this->x_ = r; }
        T height() const { return this->y_; }
        T height(const T &r) { return this->y_ = r; }
        /// For plain numbers only: an area isn't a distance.
        T area() const
            requires Numeric<T>
        {
            return width() * height();
        }
        friend class pair_<size_, T>;

    private:
        // Never below 0 (defined here, so it works for any unit).
        void on_validate() {
            if constexpr (Numeric<T>) {
                if (this->x_ < T{0}) this->x_ = T{0};
                if (this->y_ < T{0}) this->y_ = T{0};
            } else {
                if (this->x_.points() < 0) this->x_ = detail::from_points<T>(0);
                if (this->y_.points() < 0) this->y_ = detail::from_points<T>(0);
            }
        }

        friend std::ostream &operator<<(std::ostream &lhs, const size_ &rhs) { return lhs << "[" << rhs.x_ << "," << rhs.y_ << "]"; }
    };

    using size = size_<double>;
    using size_i = size_<int>;
} // namespace sc
