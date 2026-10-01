#pragma once
#include <cmath>
#include <concepts>
#include <limits>
#include <ostream>
#include <stdexcept>

namespace sc {
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    /// CRTP base for a pair of coordinates (point_, size_): arithmetic with another
    /// pair or a scalar, each returning Derived so the operators chain naturally.
    template<typename Derived, Numeric T>
    class pair_ {
    public:
        pair_(const T &x = 0, const T &y = 0) : x_(x), y_(y) { validate(); }

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

        // + operators...
        Derived operator+=(const pair_ &r) {
            x_ += r.x_;
            y_ += r.y_;
            validate();
            return *this;
        }

        Derived operator+(const pair_ &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        template<Numeric U>
        Derived operator+=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ += value;
            y_ += value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator+(const U &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        // - operators...
        Derived operator-=(const pair_ &r) {
            x_ -= r.x_;
            y_ -= r.y_;
            validate();
            return *this;
        }

        Derived operator-(const pair_ &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        template<Numeric U>
        Derived operator-=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ -= value;
            y_ -= value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator-(const U &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        // * operators...
        Derived operator*=(const pair_ &r) {
            x_ *= r.x_;
            y_ *= r.y_;
            validate();
            return *this;
        }

        Derived operator*(const pair_ &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        template<Numeric U>
        Derived operator*=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ *= value;
            y_ *= value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator*(const U &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        // / operators...
        Derived operator/=(const pair_ &r) {
            x_ /= r.x_;
            y_ /= r.y_;
            validate();
            return *this;
        }

        Derived operator/(const pair_ &r) const {
            auto tmp = *this;
            return tmp /= r;
        }

        template<Numeric U>
        Derived operator/=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ /= value;
            y_ /= value;
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

    template<typename T>
    class point_ : public pair_<point_<T>, T> {
    public:
        using pair_<point_<T>, T>::pair_;

        template<typename U>
        operator point_<U>() {
            return {this->x_, this->y_};
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
        /// Rounds to the nearest int, throwing if the value doesn't fit - used wherever
        /// a double-valued shape (rect, rotated_rect) is converted to an int-pixel one.
        inline int polygon_coordinate(const double value) {
            if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max())
                throw std::overflow_error{"Polygon coordinate exceeds integer range"};
            return static_cast<int>(std::lround(value));
        }
    }
} // namespace sc
