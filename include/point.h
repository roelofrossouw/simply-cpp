#pragma once
#include "distance.h"

#include <cmath>
#include <concepts>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace sc {
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    /// What a point's or size's coordinates can be: a plain number, or a distance (sc::distance, or a
    /// unit from sc::dist), which keeps its unit: sc::size_<sc::distance> prints [595.276pt,841.89pt].
    template<typename T>
    concept Coordinate = Numeric<T> || std::derived_from<T, distance>;

    namespace detail {
        /// A coordinate from a number: as it is for a plain number, as points for a distance (so a
        /// unit's arithmetic is done in points: dist::mm(10) plus 5 is 10 mm and 5 points).
        template<Coordinate T>
        T from_points(const double value) {
            if constexpr (Numeric<T>) return static_cast<T>(value);
            else return T(distance(value));
        }
    }

    /// CRTP base for a pair of coordinates (point_, size_): arithmetic with another
    /// pair or a scalar, each returning Derived so the operators chain naturally.
    template<typename Derived, Coordinate T>
    class pair_ {
    public:
        pair_(const T &x = T{}, const T &y = T{}) : x_(x), y_(y) { validate(); }

        template<typename U1, typename U2>
            requires(std::convertible_to<U1, T> && std::convertible_to<U2, T>)
        pair_(U1 x, U2 y) : pair_(static_cast<T>(x), static_cast<T>(y)) {
        }

        template<typename U, typename V>
        operator pair_<U, V>() const {
            return {x_, y_};
        }

        operator Derived() { return {x_, y_}; }

        bool operator==(const pair_ &) const = default;

        Derived operator+=(const pair_ &r) {
            x_ = combine(x_, r.x_, [](const auto a, const auto b) { return a + b; });
            y_ = combine(y_, r.y_, [](const auto a, const auto b) { return a + b; });
            validate();
            return *this;
        }

        Derived operator+(const pair_ &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        template<Numeric U>
        Derived operator+=(const U &r) {
            x_ = with(x_, r, [](const auto a, const auto b) { return a + b; });
            y_ = with(y_, r, [](const auto a, const auto b) { return a + b; });
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator+(const U &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        Derived operator-=(const pair_ &r) {
            x_ = combine(x_, r.x_, [](const auto a, const auto b) { return a - b; });
            y_ = combine(y_, r.y_, [](const auto a, const auto b) { return a - b; });
            validate();
            return *this;
        }

        Derived operator-(const pair_ &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        template<Numeric U>
        Derived operator-=(const U &r) {
            x_ = with(x_, r, [](const auto a, const auto b) { return a - b; });
            y_ = with(y_, r, [](const auto a, const auto b) { return a - b; });
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator-(const U &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        Derived operator*=(const pair_ &r) {
            x_ = combine(x_, r.x_, [](const auto a, const auto b) { return a * b; });
            y_ = combine(y_, r.y_, [](const auto a, const auto b) { return a * b; });
            validate();
            return *this;
        }

        Derived operator*(const pair_ &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        template<Numeric U>
        Derived operator*=(const U &r) {
            x_ = with(x_, r, [](const auto a, const auto b) { return a * b; });
            y_ = with(y_, r, [](const auto a, const auto b) { return a * b; });
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator*(const U &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        Derived operator/=(const pair_ &r) {
            x_ = combine(x_, r.x_, [](const auto a, const auto b) { return a / b; });
            y_ = combine(y_, r.y_, [](const auto a, const auto b) { return a / b; });
            validate();
            return *this;
        }

        Derived operator/(const pair_ &r) const {
            auto tmp = *this;
            return tmp /= r;
        }

        template<Numeric U>
        Derived operator/=(const U &r) {
            x_ = with(x_, r, [](const auto a, const auto b) { return a / b; });
            y_ = with(y_, r, [](const auto a, const auto b) { return a / b; });
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator/(const U &r) const {
            auto tmp = *this;
            return tmp /= r;
        }

    protected:
        T x_, y_;

        // a op b: for plain numbers as they are, for distances on their points.
        template<typename Op>
        static T combine(const T &a, const T &b, Op op) {
            if constexpr (Numeric<T>) return static_cast<T>(op(a, b));
            else return detail::from_points<T>(op(a.points(), b.points()));
        }

        // a op a number: for plain numbers the number as a T (as before), for distances a number of
        // points to add or take away, or a factor to multiply or divide by.
        template<typename U, typename Op>
        static T with(const T &a, const U &r, Op op) {
            if constexpr (Numeric<T>) return static_cast<T>(op(a, static_cast<T>(r)));
            else return detail::from_points<T>(op(a.points(), static_cast<double>(r)));
        }

        void validate() {
            if constexpr (requires(Derived &d) { d.on_validate(); })
                static_cast<Derived *>(this)->on_validate();
        }

    private:
        template<typename A, typename B>
            requires(!std::same_as<A, Derived> || !std::same_as<B, T>)
        friend bool operator==(const pair_ &a, const pair_<A, B> &b) {
            return a == static_cast<pair_>(b);
        }
    };

    /// A position: point (double) or point_i (whole pixels). y grows downwards, as on a screen.
    template<typename T>
    class point_ : public pair_<point_<T>, T> {
    public:
        using pair_<point_<T>, T>::pair_;

        template<typename U>
            requires std::convertible_to<T, U>
        operator point_<U>() const {
            return {this->x_, this->y_};
        }

        /// The same position in another unit (or a distance from a number of points):
        /// static_cast<sc::point_<sc::dist::mm>>(where).
        template<Coordinate U>
            requires(!std::same_as<U, T> && !std::convertible_to<U, T> && std::constructible_from<T, U>)
        explicit point_(const point_<U> &other) : point_(T(other.x()), T(other.y())) {
        }

        T x() const { return this->x_; }
        T x(const T &r) { return this->x_ = r; }
        T y() const { return this->y_; }
        T y(const T &r) { return this->y_ = r; }

    private:
        friend std::ostream &operator<<(std::ostream &lhs, const point_ &rhs) { return lhs << "(" << rhs.x_ << "," << rhs.y_ << ")"; }
    };

    using point = point_<double>;
    using point_i = point_<int>;

    namespace detail {
        /// Rounds to the nearest int; std::overflow_error when the value doesn't fit (or isn't finite).
        inline int polygon_coordinate(const double value) {
            if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max())
                throw std::overflow_error{"Polygon coordinate exceeds integer range"};
            return static_cast<int>(std::lround(value));
        }

        /// A coordinate as type T: rounded and range checked for an int, as is otherwise. What
        /// converting a shape between its double and int flavours does to each number.
        template<Numeric T>
        T coordinate(const double value) {
            if constexpr (std::integral<T>) return static_cast<T>(polygon_coordinate(value));
            else return static_cast<T>(value);
        }
    }
} // namespace sc
