#ifndef SC_DUAL_H
#define SC_DUAL_H

#endif // SC_DUAL_H

namespace sc {

    template<typename T>
    struct is_dual : std::false_type {};

    template<typename T>
    concept dual_type = std::floating_point<T> || is_dual<T>::value;

    template<dual_type T>
    struct dual {
        T real{};
        T eps{};

        constexpr dual() = default;
        constexpr dual(T real, T eps = T{0}) noexcept : real(real), eps(eps) {}
        static constexpr dual seed(T value) noexcept { return dual(value, T{1}); }

        constexpr dual &operator+=(const dual &o) noexcept {
            real += o.real;
            eps += o.eps;
            return *this;
        }
        constexpr dual &operator-=(const dual &o) noexcept {
            real -= o.real;
            eps -= o.eps;
            return *this;
        }
        constexpr dual &operator*=(const dual &o) noexcept {
            eps = real * o.eps + eps * o.real;
            real *= o.real;
            return *this;
        }
        constexpr dual &operator/=(const dual &o) noexcept {
            eps = (eps * o.real - real * o.eps) / (o.real * o.real);
            real /= o.real;
            return *this;
        }

        friend constexpr dual operator+(dual a, const dual &b) noexcept { return a += b; }
        friend constexpr dual operator-(dual a, const dual &b) noexcept { return a -= b; }
        friend constexpr dual operator*(dual a, const dual &b) noexcept { return a *= b; }
        friend constexpr dual operator/(dual a, const dual &b) noexcept { return a /= b; }
        friend constexpr dual operator-(dual a) noexcept {
            a.real = -a.real;
            a.eps = -a.eps;
            return a;
        }

        friend constexpr bool operator==(const dual &, const dual &) noexcept = default;

        friend constexpr void swap(dual &a, dual &b) noexcept {
            using std::swap;
            swap(a.real, b.real);
            swap(a.eps, b.eps);
        }

        friend std::ostream &operator<<(std::ostream &os, const dual &d) {
            if (std::is_floating_point_v<T>) {
                return os << d.real << " + " << d.eps << "\xCE\xB5"; // "ε" (UTF-8)
            }
            return os << "(" << d.real << ") + (" << d.eps << ")\xCE\xB5";
        }
    };

    template<typename T>
    struct is_dual<dual<T>> : std::true_type {};

    namespace dual_math {

        template<dual_type T>
        constexpr dual<T> sin(const dual<T> &d) {
            using std::cos;
            using std::sin;
            return dual<T>(sin(d.real), d.eps * cos(d.real));
        }

        template<dual_type T>
        constexpr dual<T> cos(const dual<T> &d) {
            using std::cos;
            using std::sin;
            return dual<T>(cos(d.real), -(d.eps * sin(d.real)));
        }

        template<dual_type T>
        constexpr dual<T> exp(const dual<T> &d) {
            using std::exp;
            T e = exp(d.real);
            return dual<T>(e, d.eps * e);
        }

        template<dual_type T>
        constexpr dual<T> sqrt(const dual<T> &d) {
            using std::sqrt;
            T s = sqrt(d.real);
            return dual<T>(s, d.eps / (T{2} * s));
        }

    } // namespace dual_math

    template<std::size_t I, dual_type T>
    constexpr T get(const dual<T> &d) noexcept {
        if constexpr (I == 0)
            return d.real;
        else
            return d.eps;
    }

} // namespace sc

using namespace sc;
template<dual_type T>
struct std::tuple_size<dual<T>> : std::integral_constant<std::size_t, 2> {};

template<dual_type T>
struct std::tuple_element<0, dual<T>> {
    using type = T;
};
template<dual_type T>
struct std::tuple_element<1, dual<T>> {
    using type = T;
};

template<dual_type T>
struct std::hash<dual<T>> {
    std::size_t operator()(const dual<T> &d) const noexcept {
        const std::size_t h1 = std::hash<T>{}(d.real);
        const std::size_t h2 = std::hash<T>{}(d.eps);
        return h1 ^ (h2 << 1);
    }
};
