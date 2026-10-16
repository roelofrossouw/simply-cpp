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
    /// A shape of straight lines and cubic Bezier curves, such as an SVG path: one or more
    /// subpaths, each started by move_to() and optionally closed by close(). Coordinates are y down.
    /// Only the shape: how it's drawn (stroke, fill, and how overlapping subpaths fill, an "o"'s
    /// hole) is sc-graphics' paint (sc::stroke, sc::fill, sc::fill_rule).
    class path {
    public:
        enum class kind { move, line, cubic, close };

        /// One step: to is where it ends; a cubic's control points are first and second.
        struct step {
            kind type;
            point to;
            point first{};
            point second{};

            bool operator==(const step &) const = default;
        };

        path &move_to(const point &to);

        path &line_to(const point &to);

        path &cubic_to(const point &first, const point &second, const point &to);

        /// Closes the current subpath, back to where it started.
        path &close();

        [[nodiscard]] const std::vector<step> &steps() const noexcept { return steps_; }
        [[nodiscard]] bool empty() const noexcept { return steps_.empty(); }

        /// The area it covers, exactly: every point it passes through, curves included (where a
        /// curve turns, not its control points). Empty for an empty path.
        [[nodiscard]] rect bounds() const;

        /// Scaled about the origin, then moved by offset.
        [[nodiscard]] path transformed(double scale_x, double scale_y, const point &offset = {}) const;

        /// One subpath as straight segments: its points, curves cut into short lines, and whether
        /// it was closed.
        struct outline {
            polygon points;
            bool closed;
        };

        /// The subpaths as straight segments, curves cut into segments no more than tolerance
        /// off the curve, for canvases that only draw straight lines.
        [[nodiscard]] std::vector<outline> flattened(double tolerance = 0.25) const;

        bool operator==(const path &) const = default;

    private:
        std::vector<step> steps_;
    };

    // Each shape as a path of the same outline, in doubles: rectangles, polygons and rotated
    // rectangles as closed straight lines, a line as an open one, a circle as four cubic curves and a
    // rounded rectangle's corners as one each (the usual Bezier approximation of a quarter circle,
    // within 0.03% of the radius). To draw any shape the same way, or measure it as a path. A shape
    // of your own satisfies sc::geometry with a bounds() member and a to_path(const your_shape &) of
    // its own, found next to it (by argument-dependent lookup).

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
