#ifndef SC_DUAL_H
#define SC_DUAL_H

namespace sc {
    template<typename T>
    concept numeric_type = std::floating_point<T> || std::integral<T>;

    template<typename>
    struct is_dual : std::false_type {
    };

    template<typename T>
    concept dual_type = std::floating_point<T> || is_dual<T>::value;

    template<dual_type T>
    struct dual;

    template<typename T>
    struct is_dual<dual<T> > : std::true_type {
    };

    template<dual_type T>
    struct dual {
        T real{0};
        T eps{0};

        dual(T real, T eps = {0}) : real(real), eps(eps) {
        }

        template<numeric_type R>
        dual(R scalar) { real = scalar; }

        // Named function needed?
        // static dual<dual> seed(dual value) noexcept {
        //     return {value, 1};
        // }

        dual<dual> operator~() {
            return {*this, 1};
            // or override a named function if needed return dual::seed(*this);
        }

        dual operator+(const dual &rhs) noexcept { return {real + rhs.real, eps + rhs.eps}; }
        dual operator-(const dual &rhs) noexcept { return {real - rhs.real, eps - rhs.eps}; }
        dual operator-() noexcept { return {-real, -eps}; }
        dual operator*(const dual &rhs) noexcept { return {real * rhs.real, real * rhs.eps + eps * rhs.real}; }
        dual operator/(const dual &rhs) noexcept { return {real / rhs.real, (eps * rhs.real - real * rhs.eps) / (rhs.real * rhs.real)}; }
        dual &operator+=(const dual &rhs) noexcept { return *this = *this + rhs; }
        dual &operator-=(const dual &rhs) noexcept { return *this = *this - rhs; }
        dual &operator*=(const dual &rhs) noexcept { return *this = *this * rhs; }
        dual &operator/=(const dual &rhs) noexcept { return *this = *this / rhs; }

        // dual sin(const dual<T> &d) {
        //         return dual<T>(std::sin(d.real), d.eps * std::cos(d.real));
        //     }

        template<std::floating_point R>
        operator R() const {
            if (std::is_floating_point_v<decltype(eps)>) return static_cast<R>(real);
            return static_cast<R>(eps);
        }

        T absval() const {
            if (std::is_floating_point_v<decltype(eps)>) return abs(real);
            return eps;
        }
    };

    template<std::floating_point R>
    std::ostream &operator<<(std::ostream &os, const dual<R> &d) {
        os << d.real;
        if (abs(d.eps) > 0) os << " + " << d.eps << "\xCE\xB5"; // "ε" (UTF-8)
        return os;
    }

    template<std::floating_point R>
    std::ostream &operator<<(std::ostream &os, const dual<dual<R> > &d) {
        os << d.real << " + " << d.eps << "\xCE\xB5"; // "ε" (UTF-8)
        return os;
    }

    template<dual_type R>
    std::ostream &operator<<(std::ostream &os, const dual<R> &d) {
        os << "(" << d.real << ") + (" << d.eps << ")\xCE\xB5"; // "ε" (UTF-8)
        return os;
    }


    template<std::floating_point R>
    R abs(const dual<R> &n) {
        return n.absval();
    }

    template<dual_type R>
    R abs(const dual<R> &n) {
        return n.absval();
    }

    namespace dual_math {
        template<std::floating_point T>
        T dsin(const T &d) { return sin(d); }

        template<std::floating_point T>
        T dcos(const T &d) { return cos(d); }

        template<dual_type T>
        dual<T> dsin(const dual<T> &d);

        template<dual_type T>
        dual<T> dcos(const dual<T> &d);

        template<dual_type T>
        dual<T> dcos(const dual<T> &d) {
            return sc::dual<T>(dcos(d.real), -(static_cast<T>(d.eps) * static_cast<T>(dsin(d.real))));
        }

        template<dual_type T>
        dual<T> dsin(const dual<T> &d) {
            return sc::dual<T>(dsin(d.real), static_cast<T>(d.eps) * static_cast<T>(dcos(d.real)));
        }

        template<dual_type T>
        dual<T> exp(const dual<T> &d) {
            T e = std::exp(d.real);
            return dual<T>(e, d.eps * e);
        }

        template<dual_type T>
        dual<T> sqrt(const dual<T> &d) {
            T s = std::sqrt(d.real);
            return dual<T>(s, d.eps / (T{2} * s));
        }
    } // namespace dual_math

    // Not needed now...
    //     template<std::size_t I, dual_type T>
    //     constexpr T get(const dual<T> &d) noexcept {
    //         if constexpr (I == 0)
    //             return static_cast<T>(d.real);
    //         else
    //             return static_cast<T>(d.eps);
    //     }
} // namespace sc


// (Removed) using namespace sc; -> Don't want to force other files that include this file to the namespace
// Not needed for now, but might want it later...
// template<sc::dual_type T>
// struct std::tuple_size<sc::dual<T> > : std::integral_constant<std::size_t, 2> {
// };
//
// template<sc::dual_type T>
// struct std::tuple_element<0, sc::dual<T> > {
//     using type = T;
// };
//
// template<sc::dual_type T>
// struct std::tuple_element<1, sc::dual<T> > {
//     using type = T;
// };
// template<sc::dual_type T>
// struct std::hash<sc::dual<T> > {
//     std::size_t operator()(const sc::dual<T> &d) const noexcept {
//         const std::size_t h1 = std::hash<T>{}(d.real);
//         const std::size_t h2 = std::hash<T>{}(d.eps);
//         return h1 ^ (h2 << 1);
//     }
// };

#endif // SC_DUAL_H
