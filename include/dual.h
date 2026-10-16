#ifndef SC_DUAL_H
#define SC_DUAL_H

#include <cmath>
#include <concepts>
#include <iostream>
#include <sstream>
#include <iomanip>
#include <type_traits>

namespace sc {
    const std::string EPS_SIGN = "\xCE\xB5";

    template<typename T>
    concept numeric_type = std::floating_point<T> || std::integral<T>;

    template<typename>
    struct is_dual : std::false_type {
    };

    template<typename T>
    concept dual_type = std::floating_point<T> || is_dual<T>::value;

    template<dual_type T>
    class dual;

    template<typename T>
    struct is_dual<dual<T> > : std::true_type {
    };

    /// A dual number, real + eps ε with ε² = 0: arithmetic on it carries a derivative along.
    /// ~x makes x the variable (ε part 1), so f(~x) has f'(x) as its ε part; nesting goes further.
    ///
    ///     auto x = ~sc::dual<double>{3};
    ///     std::string(x * x * x + x * 2);   // "33 + 29ε": f(3) and f'(3)
    template<dual_type T>
    class dual {
    public:
        T real{0};
        T eps{0};

        dual(T real, T eps = {0}) : real(real), eps(eps) {
        }

        template<numeric_type R>
        dual(R scalar) { real = scalar; }

        dual<dual> operator~() { return {*this, 1}; }

        dual operator+(const dual &rhs) const { return {real + rhs.real, eps + rhs.eps}; }
        dual operator-(const dual &rhs) const { return {real - rhs.real, eps - rhs.eps}; }
        dual operator-() const { return {-real, -eps}; }
        dual operator*(const dual &rhs) const { return {real * rhs.real, real * rhs.eps + eps * rhs.real}; }
        dual operator/(const dual &rhs) const { return {real / rhs.real, (eps * rhs.real - real * rhs.eps) / (rhs.real * rhs.real)}; }
        dual &operator+=(const dual &rhs) { return *this = *this + rhs; }
        dual &operator-=(const dual &rhs) { return *this = *this - rhs; }
        dual &operator*=(const dual &rhs) { return *this = *this * rhs; }
        dual &operator/=(const dual &rhs) { return *this = *this / rhs; }

        template<numeric_type R>
        dual operator+(const R &rhs) const { return *this + dual(rhs); }

        template<numeric_type R>
        dual operator-(const R &rhs) const { return *this - dual(rhs); }

        template<numeric_type R>
        dual operator*(const R &rhs) const { return *this * dual(rhs); }

        template<numeric_type R>
        dual operator/(const R &rhs) const { return *this / dual(rhs); }


        dual cos() const { return get_cos(*this); }
        dual sin() const { return get_sin(*this); }
        dual exp() const { return get_exp(*this); }
        dual sqrt() const { return get_sqrt(*this); }
        T abs() const { return this->get_abs(); }

        /// The value and its ε parts, as "33 + 29ε".
        operator std::string() const { return print(*this); }

        template<std::floating_point R>
        operator R() const {
            if (std::is_floating_point_v<decltype(eps)>) return static_cast<R>(real);
            return static_cast<R>(eps);
        }

    private:
        // Utility Functions
        template<std::floating_point R> // Base order - no eps
        static std::string print(const dual<R> &num) {
            std::ostringstream oss;
            oss << std::setprecision(16) << static_cast<double>(num.real);
            return oss.str();
        }

        template<std::floating_point R> // First order - no brackets.
        static std::string print(const dual<dual<R> > &num) {
            std::ostringstream oss;
            oss << print(num.real) << " + " << print(num.eps) << EPS_SIGN;
            return oss.str();
        }

        template<dual_type R>
        static std::string print(const dual<dual<R> > &num) {
            std::ostringstream oss;
            oss << "(" << print(num.real) << ") + (" << print(num.eps) << ")" << EPS_SIGN;
            return oss.str();
        }

        T get_abs() const {
            if (std::is_floating_point_v<decltype(eps)>) return std::abs(real);
            return eps;
        }

        template<std::floating_point R>
        static dual<R> get_sin(const dual<R> &num) {
            return {std::sin(num.real), num.eps * std::cos(num.real)};
        }

        template<dual_type R>
        static dual<dual<R> > get_sin(const dual<dual<R> > &num) {
            return {get_sin(num.real), num.eps * get_cos(num.real)};
        }

        template<std::floating_point R>
        static dual<R> get_cos(const dual<R> &num) {
            return {std::cos(num.real), -num.eps * std::sin(num.real)};
        }

        template<dual_type R>
        static dual<dual<R> > get_cos(const dual<dual<R> > &num) {
            return {get_cos(num.real), -num.eps * get_sin(num.real)};
        }

        template<std::floating_point R>
        static dual<R> get_exp(const dual<R> &num) {
            auto e = std::exp(num.real);
            return {e, num.eps * e};
        }

        template<dual_type R>
        static dual<dual<R> > get_exp(const dual<dual<R> > &num) {
            auto e = get_exp(num.real);
            return {e, num.eps * e};
        }

        template<std::floating_point R>
        static dual<R> get_sqrt(const dual<R> &num) {
            auto s = std::sqrt(num.real);
            return {s, num.eps / (2 * s)};
        }

        template<dual_type R>
        static dual<dual<R> > get_sqrt(const dual<dual<R> > &num) {
            auto s = get_sqrt(num.real);
            return {s, num.eps / (2 * s)};
        }
    };

    /// The value only; std::string(d) shows the ε parts too.
    template<dual_type R>
    std::ostream &operator<<(std::ostream &os, const dual<R> &rhs) { return os << static_cast<double>(rhs); }

    template<dual_type L, numeric_type R>
    dual<L> operator*(R lhs, const dual<L> &rhs) { return {rhs * dual<L>(lhs)}; }

    template<dual_type R>
    R abs(const dual<R> &n) { return n.abs(); }

    template<dual_type T>
    dual<T> cos(const dual<T> &d) { return d.cos(); }

    template<dual_type T>
    dual<T> sin(const dual<T> &d) { return d.sin(); }

    template<dual_type T>
    dual<T> exp(const dual<T> &d) { return d.exp(); }

    template<dual_type T>
    dual<T> sqrt(const dual<T> &d) { return d.sqrt(); }
} // namespace sc

#endif // SC_DUAL_H
