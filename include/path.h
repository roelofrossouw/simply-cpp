#pragma once

#include "circle.h"
#include "line.h"
#include "point.h"
#include "polygon.h"
#include "rect.h"
#include "rotated_rect.h"
#include "rounded_rect.h"

#include <algorithm>

#include <vector>

namespace sc {
    /// Straight lines and cubic Bezier curves, as an SVG path: subpaths started by move_to(),
    /// optionally closed. Only geometry: stroke, fill and fill rule are sc-graphics' paint.
    class path {
    public:
        enum class kind { move, line, cubic, close };

        /// first and second are a cubic's control points, unused otherwise.
        struct step {
            kind type;
            point to;
            point first{};
            point second{};

            bool operator==(const step &) const = default;
        };

        /// Starts a subpath. On an empty path, line_to() starts at its point and cubic_to() at its
        /// first control point.
        path &move_to(const point &to);

        path &line_to(const point &to);

        path &cubic_to(const point &first, const point &second, const point &to);

        /// Back to the subpath's start; nothing on an empty path or after another close().
        path &close();

        /// Adds other's subpaths after this one's.
        path &append(const path &other);

        [[nodiscard]] const std::vector<step> &steps() const noexcept { return steps_; }
        [[nodiscard]] bool empty() const noexcept { return steps_.empty(); }

        /// Exact: curves count where they turn, not their control points. Empty for an empty path.
        [[nodiscard]] rect bounds() const;

        /// Scaled about the origin, then moved by offset.
        [[nodiscard]] path transformed(double scale_x, double scale_y, const point &offset = {}) const;

        struct outline {
            polygon points;
            bool closed;
        };

        /// The subpaths as straight segments, curves cut to within tolerance of the curve. A
        /// subpath of one point is left out.
        [[nodiscard]] std::vector<outline> flattened(double tolerance = 0.25) const;

        bool operator==(const path &) const = default;

    private:
        std::vector<step> steps_;
    };

    // to_path(): each shape's outline in doubles (see sc::geometry). A circle is four cubics and a
    // rounded corner one, the usual Bezier quarter circle (within 0.03% of the radius).

    /// Control point distance for a quarter circle drawn as one cubic Bezier curve, as a fraction of
    /// its radius.
    inline constexpr double quarter_circle = 0.5522847498;

    inline path to_path(const path &shape) { return shape; }

    template<typename T>
    path to_path(const rect_<T> &shape) {
        const rect box{shape};
        path result;
        result.move_to({box.left(), box.top()}).line_to({box.right(), box.top()}).line_to({box.right(), box.bottom()}).line_to({box.left(), box.bottom()}).close();
        return result;
    }

    /// The radius as a canvas draws it: at most half the shorter side.
    template<Numeric T>
    path to_path(const rounded_rect_<T> &shape) {
        const rect box{shape.rect()};
        const double radius = std::min(static_cast<double>(shape.radius()), std::min(box.width(), box.height()) / 2);
        if (radius <= 0) return to_path(box);
        const double l = box.left(), t = box.top(), r = box.right(), b = box.bottom(), k = radius * quarter_circle;
        path result;
        result.move_to({l + radius, t}).line_to({r - radius, t});
        result.cubic_to({r - radius + k, t}, {r, t + radius - k}, {r, t + radius}).line_to({r, b - radius});
        result.cubic_to({r, b - radius + k}, {r - radius + k, b}, {r - radius, b}).line_to({l + radius, b});
        result.cubic_to({l + radius - k, b}, {l, b - radius + k}, {l, b - radius}).line_to({l, t + radius});
        result.cubic_to({l, t + radius - k}, {l + radius - k, t}, {l + radius, t}).close();
        return result;
    }

    /// Empty without points.
    template<Numeric T>
    path to_path(const polygon_<T> &shape) {
        path result;
        if (shape.empty()) return result;
        bool first = true;
        for (const auto &corner: shape) {
            const point at{static_cast<double>(corner.x()), static_cast<double>(corner.y())};
            if (first) result.move_to(at);
            else result.line_to(at);
            first = false;
        }
        return result.close();
    }

    template<Numeric T>
    path to_path(const rotated_rect_<T> &shape) { return to_path(static_cast<polygon>(shape)); }

    /// Empty for a radius of 0.
    template<Numeric T>
    path to_path(const circle_<T> &shape) {
        path result;
        const double x = shape.center().x(), y = shape.center().y(), r = shape.radius(), k = r * quarter_circle;
        if (r <= 0) return result;
        result.move_to({x + r, y});
        result.cubic_to({x + r, y + k}, {x + k, y + r}, {x, y + r});
        result.cubic_to({x - k, y + r}, {x - r, y + k}, {x - r, y});
        result.cubic_to({x - r, y - k}, {x - k, y - r}, {x, y - r});
        result.cubic_to({x + k, y - r}, {x + r, y - k}, {x + r, y});
        return result.close();
    }

    /// Open: a line has no inside.
    template<Numeric T>
    path to_path(const line_<T> &shape) {
        path result;
        result.move_to({static_cast<double>(shape.from().x()), static_cast<double>(shape.from().y())})
              .line_to({static_cast<double>(shape.to().x()), static_cast<double>(shape.to().y())});
        return result;
    }
}
