#include "transform.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <stdexcept>
#include <variant>

namespace sc {
    namespace {
        // Tolerance for "is this entry zero" or "are these equal", relative to the transform's size.
        double tolerance(const transform &t) {
            return 1e-12 * std::max({1.0, std::abs(t.a()), std::abs(t.b()), std::abs(t.c()), std::abs(t.d())});
        }

        double radians(const double degrees) { return degrees * std::numbers::pi / 180; }

        double degrees(const double radians) { return radians * 180 / std::numbers::pi; }
    }

    transform::transform(const double a, const double b, const double c, const double d, const double e, const double f)
        : a_(a), b_(b), c_(c), d_(d), e_(e), f_(f) {
    }

    transform transform::translate(const double dx, const double dy) { return {1, 0, 0, 1, dx, dy}; }

    transform transform::translate(const point &by) { return translate(by.x(), by.y()); }

    transform transform::scale(const double sx, const double sy, const point &about) {
        return {sx, 0, 0, sy, about.x() * (1 - sx), about.y() * (1 - sy)};
    }

    transform transform::scale(const double factor, const point &about) { return scale(factor, factor, about); }

    transform transform::rotate(const double degrees, const point &about) {
        const double cosine = std::cos(radians(degrees)), sine = std::sin(radians(degrees));
        return translate(-about.x(), -about.y()).then({cosine, sine, -sine, cosine, 0, 0}).then(translate(about));
    }

    transform transform::flip_horizontal(const double axis_x) { return {-1, 0, 0, 1, 2 * axis_x, 0}; }

    transform transform::flip_vertical(const double axis_y) { return {1, 0, 0, -1, 0, 2 * axis_y}; }

    transform transform::then(const transform &next) const {
        return {next.a_ * a_ + next.c_ * b_, next.b_ * a_ + next.d_ * b_,
                next.a_ * c_ + next.c_ * d_, next.b_ * c_ + next.d_ * d_,
                next.a_ * e_ + next.c_ * f_ + next.e_, next.b_ * e_ + next.d_ * f_ + next.f_};
    }

    transform transform::inverse() const {
        const double determinant = a_ * d_ - b_ * c_;
        if (std::abs(determinant) <= 1e-300) throw std::domain_error{"sc::transform: this transform can't be undone"};
        const double a = d_ / determinant, b = -b_ / determinant, c = -c_ / determinant, d = a_ / determinant;
        return {a, b, c, d, -(a * e_ + c * f_), -(b * e_ + d * f_)};
    }

    double transform::scale_factor() const { return std::sqrt(std::abs(a_ * d_ - b_ * c_)); }

    bool transform::conformal() const {
        // The x axis's image (a, b) and the y axis's (c, d): as long as each other, and at right angles.
        const double eps = tolerance(*this);
        return std::abs(std::hypot(a_, b_) - std::hypot(c_, d_)) <= eps && std::abs(a_ * c_ + b_ * d_) <= eps;
    }

    bool transform::keeps_axes() const {
        const double eps = tolerance(*this);
        return (std::abs(b_) <= eps && std::abs(c_) <= eps) || (std::abs(a_) <= eps && std::abs(d_) <= eps);
    }

    point transform::apply(const point &at) const {
        return {a_ * at.x() + c_ * at.y() + e_, b_ * at.x() + d_ * at.y() + f_};
    }

    line transform::apply(const line &shape) const { return {apply(shape.from()), apply(shape.to())}; }

    polygon transform::apply(const polygon &shape) const {
        std::vector<point> corners;
        corners.reserve(shape.size());
        for (const auto &corner: shape) corners.push_back(apply(corner));
        return polygon{std::move(corners)};
    }

    path transform::apply(const path &shape) const {
        path result;
        for (const auto &step: shape.steps()) {
            switch (step.type) {
                case path::kind::move: result.move_to(apply(step.to)); break;
                case path::kind::line: result.line_to(apply(step.to)); break;
                case path::kind::cubic: result.cubic_to(apply(step.first), apply(step.second), apply(step.to)); break;
                case path::kind::close: result.close(); break;
            }
        }
        return result;
    }

    namespace {
        // A rectangle centred on centre, its width along the direction at angle degrees, through a
        // conformal transform: still a rectangle, turned and scaled.
        rotated_rect turned(const transform &t, const point &centre, const size &extent, const double angle) {
            const point along = t.apply(point{std::cos(radians(angle)), std::sin(radians(angle))}) - t.apply(point{0, 0});
            const double scale = t.scale_factor();
            return {t.apply(centre), {extent.width() * scale, extent.height() * scale}, degrees(std::atan2(along.y(), along.x()))};
        }

        // The box with these two opposite corners.
        rect spanning(const point &one, const point &other) {
            return {std::min(one.x(), other.x()), std::min(one.y(), other.y()), std::abs(other.x() - one.x()), std::abs(other.y() - one.y())};
        }
    }

    shape transform::apply(const rect &shape) const {
        if (keeps_axes()) return spanning(apply(shape.left_top()), apply(shape.right_bottom()));
        if (conformal()) return turned(*this, shape.center(), shape.size(), 0);
        return apply(static_cast<polygon>(shape));
    }

    shape transform::apply(const rounded_rect &shape) const {
        if (keeps_axes() && conformal()) {
            const rect box = spanning(apply(shape.rect().left_top()), apply(shape.rect().right_bottom()));
            return rounded_rect{box, shape.radius() * scale_factor()};
        }
        return apply(to_path(shape));
    }

    shape transform::apply(const rotated_rect &shape) const {
        if (conformal()) return turned(*this, shape.center(), shape.size(), shape.angle());
        return apply(static_cast<polygon>(shape));
    }

    shape transform::apply(const circle &shape) const {
        if (conformal()) return circle{apply(shape.center()), shape.radius() * scale_factor()};
        return apply(to_path(shape));
    }

    shape transform::apply(const shape &held) const {
        return std::visit([this](const auto &each) { return shape{apply(each)}; }, held.value());
    }

    group transform::apply(const group &shapes) const {
        group result;
        for (const auto &item: shapes) result.push_back(apply(item));
        return result;
    }

    group &group::apply(const transform &change) {
        *this = change.apply(*this);
        return *this;
    }

    group &group::flip_horizontal() { return flip_horizontal(bounds()); }

    group &group::flip_horizontal(const rect &within) { return apply(transform::flip_horizontal(within.center().x())); }

    group &group::flip_vertical() { return flip_vertical(bounds()); }

    group &group::flip_vertical(const rect &within) { return apply(transform::flip_vertical(within.center().y())); }
}
