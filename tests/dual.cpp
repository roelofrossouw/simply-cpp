#include <sc.h>

#include <cmath>
#include <string>

#include "sc_test.h"

using namespace std;

namespace {
    constexpr double tolerance = 1e-12;

    // f(x)  = x^3 + x - 1        f(3)  = 29
    // f'(x) = 3x^2 + 1           f'(3) = 28
    // f"(x) = 6x                 f"(3) = 18
    template<typename T>
    T fn(const T &x) {
        return x * x * x + x - 1;
    }
}

int main() {
    SECTION("Construction and the epsilon seed");
    {
        const sc::dual<double> plain{3.0};
        CHECK_EQ(plain.real, 3.0);
        CHECK_EQ(plain.eps, 0.0);

        const sc::dual<double> seeded{3.0, 1.0};
        CHECK_EQ(seeded.real, 3.0);
        CHECK_EQ(seeded.eps, 1.0);

        // Implicit conversion from a scalar.
        const sc::dual<double> from_int = 7;
        CHECK_EQ(from_int.real, 7.0);
        CHECK_EQ(from_int.eps, 0.0);

        // ~ lifts a value into the next order and seeds its derivative with 1.
        sc::dual<double> x{3.0};
        const auto lifted = ~x;
        CHECK_EQ(lifted.real.real, 3.0);
        CHECK_EQ(lifted.real.eps, 0.0);
        CHECK_EQ(lifted.eps.real, 1.0);
        CHECK_EQ(lifted.eps.eps, 0.0);
    }

    SECTION("Arithmetic follows the dual number rules");
    {
        const sc::dual<double> a{2.0, 3.0};
        const sc::dual<double> b{5.0, 7.0};

        const auto sum = a + b;
        CHECK_EQ(sum.real, 7.0);
        CHECK_EQ(sum.eps, 10.0);

        const auto difference = a - b;
        CHECK_EQ(difference.real, -3.0);
        CHECK_EQ(difference.eps, -4.0);

        // Product rule: (ac, ad + bc)
        const auto product = a * b;
        CHECK_EQ(product.real, 10.0);
        CHECK_EQ(product.eps, 29.0);

        // Quotient rule: (a/c, (bc - ad) / c^2)
        const auto quotient = a / b;
        CHECK_NEAR(quotient.real, 0.4, tolerance);
        CHECK_NEAR(quotient.eps, 0.04, tolerance);

        const auto negated = -a;
        CHECK_EQ(negated.real, -2.0);
        CHECK_EQ(negated.eps, -3.0);
    }

    SECTION("Compound assignment");
    {
        sc::dual<double> value{2.0, 3.0};
        value += sc::dual<double>{5.0, 7.0};
        CHECK_EQ(value.real, 7.0);
        CHECK_EQ(value.eps, 10.0);
        value -= sc::dual<double>{5.0, 7.0};
        CHECK_EQ(value.real, 2.0);
        CHECK_EQ(value.eps, 3.0);
        value *= sc::dual<double>{5.0, 7.0};
        CHECK_EQ(value.real, 10.0);
        CHECK_EQ(value.eps, 29.0);
        value /= sc::dual<double>{5.0, 7.0};
        CHECK_NEAR(value.real, 2.0, tolerance);
        CHECK_NEAR(value.eps, 3.0, tolerance);
    }

    SECTION("Mixing with plain numbers");
    {
        const sc::dual<double> a{2.0, 3.0};
        const auto scaled = a * 3;
        CHECK_EQ(scaled.real, 6.0);
        CHECK_EQ(scaled.eps, 9.0);
        // The free operator* handles a scalar on the left.
        const auto scaled_left = 3 * a;
        CHECK_EQ(scaled_left.real, 6.0);
        CHECK_EQ(scaled_left.eps, 9.0);

        const auto shifted = a + 1;
        CHECK_EQ(shifted.real, 3.0);
        CHECK_EQ(shifted.eps, 3.0); // a constant has no derivative

        const auto halved = a / 2;
        CHECK_EQ(halved.real, 1.0);
        CHECK_EQ(halved.eps, 1.5);
    }

    SECTION("Automatic differentiation of a polynomial");
    {
        sc::dual<double> x{3.0};
        CHECK_EQ(fn(x).real, 29.0);
        CHECK_EQ(fn(x).eps, 0.0); // no seed, so no derivative

        const auto first = fn(~x);
        CHECK_EQ(first.real.real, 29.0); // f(3)
        CHECK_EQ(first.eps.real, 28.0);  // f'(3)

        // Casting a first order result yields the derivative, not the value.
        CHECK_EQ(static_cast<double>(fn(x)), 29.0);
        CHECK_EQ(static_cast<double>(fn(~x)), 28.0);

        // Structured bindings expose the pair directly.
        auto [value, derivative] = fn(~x);
        CHECK_EQ(static_cast<double>(value), 29.0);
        CHECK_EQ(static_cast<double>(derivative), 28.0);

        // Second order.
        const auto second = fn(~~x);
        CHECK_EQ(second.real.real.real, 29.0); // f(3)
        CHECK_EQ(second.eps.real.real, 28.0);  // f'(3)
        CHECK_EQ(second.eps.eps.real, 18.0);   // f"(3)
    }

    SECTION("Derivatives of the transcendental helpers");
    {
        const double at = 0.5;
        sc::dual<double> x{at};

        const auto sine = sc::sin(~x);
        CHECK_NEAR(sine.real.real, std::sin(at), tolerance);
        CHECK_NEAR(sine.eps.real, std::cos(at), tolerance);

        const auto cosine = sc::cos(~x);
        CHECK_NEAR(cosine.real.real, std::cos(at), tolerance);
        CHECK_NEAR(cosine.eps.real, -std::sin(at), tolerance);

        const auto exponential = sc::exp(~x);
        CHECK_NEAR(exponential.real.real, std::exp(at), tolerance);
        CHECK_NEAR(exponential.eps.real, std::exp(at), tolerance);

        const auto root = sc::sqrt(~x);
        CHECK_NEAR(root.real.real, std::sqrt(at), tolerance);
        CHECK_NEAR(root.eps.real, 1.0 / (2 * std::sqrt(at)), tolerance);

        // The member forms agree with the free functions.
        CHECK_NEAR(x.sin().real, sc::sin(x).real, tolerance);
        CHECK_NEAR(x.cos().real, sc::cos(x).real, tolerance);
        CHECK_NEAR(x.exp().real, sc::exp(x).real, tolerance);
        CHECK_NEAR(x.sqrt().real, sc::sqrt(x).real, tolerance);

        CHECK_EQ(sc::abs(sc::dual<double>{-2.0}), 2.0);
        CHECK_EQ(sc::abs(sc::dual<double>{2.0}), 2.0);
    }

    SECTION("String conversion shows every order");
    {
        sc::dual<double> x{3.0};
        CHECK_EQ(static_cast<string>(x), string{"3"});
        CHECK_EQ(static_cast<string>(sc::dual<double>{3.25}), string{"3.25"});
        CHECK_EQ(static_cast<string>(~x), string{"3 + 1" + sc::EPS_SIGN});
        CHECK_EQ(static_cast<string>(~~x), string{"(3 + 1" + sc::EPS_SIGN + ") + (1 + 0" + sc::EPS_SIGN + ")" + sc::EPS_SIGN});

        // Streaming prints the numeric cast, not the full expansion.
        ostringstream stream;
        stream << x;
        CHECK_EQ(stream.str(), string{"3"});
    }

    SECTION("Newton's method converges on the real root of x^3 + x - 1");
    {
        // The single real root, to double precision.
        constexpr double expected_root = 0.68232780382801932;
        constexpr int max_iterations = 50;

        sc::dual<double> x{1.0}; // deterministic starting point
        int iterations = 0;
        double residual = 1.0;
        while (iterations < max_iterations && std::fabs(residual) > 1e-15) {
            auto [fx, dfx] = fn(~x);
            const double value = static_cast<double>(fx);
            const double slope = static_cast<double>(dfx);
            CHECK(std::isfinite(value) && std::isfinite(slope));
            CHECK_NE(slope, 0.0);
            x -= value / slope;
            residual = static_cast<double>(fn(x).real);
            ++iterations;
        }

        CHECK_NEAR(x.real, expected_root, 1e-12);
        CHECK_NEAR(residual, 0.0, 1e-14);
        CHECK_LT(iterations, max_iterations); // quadratic convergence, so this is quick
        // The derivative at the root is non-zero, i.e. it is a simple root.
        CHECK_NEAR(static_cast<double>(fn(~x)), 3 * expected_root * expected_root + 1, 1e-9);
    }

    TEST_SUMMARY();
}
