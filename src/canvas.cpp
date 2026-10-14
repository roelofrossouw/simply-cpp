#include "canvas.h"

#include <algorithm>
#include <cmath>
#include <numbers>
#include <utility>

namespace sc {
    namespace {
        // Nothing to stroke without a colour or a width, nothing to fill without a colour.
        bool visible(const color &stroke, const color &fill, const double width) {
            return (stroke.alpha() > 0 && width > 0) || fill.alpha() > 0;
        }

        // The stroke as the primitives get it: transparent with no width when it doesn't show.
        std::pair<color, double> outline(const color &stroke, const double width) {
            if (stroke.alpha() > 0 && width > 0) return {stroke, width};
            return {color::Transparent, 0};
        }
    }

    void canvas::draw(const rect &shape, const color &stroke, const color &fill, const double width, const double radius) {
        if (!visible(stroke, fill, width)) return;
        const double limit = std::min(shape.width(), shape.height()) / 2;
        if (radius > 0 && limit > 0) {
            const auto [line, line_width] = outline(stroke, width);
            paint_rounded_rect(shape, std::min(radius, limit), line, fill, line_width);
        } else {
            draw(static_cast<polygon>(shape), stroke, fill, width);
        }
    }

    void canvas::draw(const rect_i &shape, const color &stroke, const color &fill, const double width, const double radius) {
        draw(rect{shape.left(), shape.top(), shape.width(), shape.height()}, stroke, fill, width, radius);
    }

    void canvas::draw(const rotated_rect &shape, const color &stroke, const color &fill, const double width) {
        draw(static_cast<polygon>(shape), stroke, fill, width);
    }

    void canvas::draw(const polygon &shape, const color &stroke, const color &fill, const double width) {
        if (shape.size() < 2 || !visible(stroke, fill, width)) return;
        const auto [line, line_width] = outline(stroke, width);
        paint_polygon(shape, line, fill, line_width);
    }

    void canvas::draw(const polygon_i &shape, const color &stroke, const color &fill, const double width) {
        draw(polygon{shape}, stroke, fill, width);
    }

    void canvas::draw(const circle &shape, const color &stroke, const color &fill, const double width) {
        if (shape.radius() <= 0 || !visible(stroke, fill, width)) return;
        const auto [line, line_width] = outline(stroke, width);
        paint_circle(shape, line, fill, line_width);
    }

    void canvas::draw(const circle_i &shape, const color &stroke, const color &fill, const double width) {
        draw(circle{point{static_cast<double>(shape.center().x()), static_cast<double>(shape.center().y())},
                    static_cast<double>(shape.radius())}, stroke, fill, width);
    }

    void canvas::draw(const text &sample, const color &colour) {
        if (sample.content.empty() || colour.alpha() <= 0 || sample.font.size <= 0) return;
        paint_text(sample, colour);
    }

    void canvas::draw_line(const point &from, const point &to, const color &colour, const double width) {
        if (colour.alpha() <= 0 || width <= 0) return;
        paint_line(from, to, colour, width);
    }

    void canvas::paint_rounded_rect(const rect &shape, const double radius, const color &stroke, const color &fill,
                                    const double width) {
        // Each corner a quarter circle of short segments, around clockwise (y down) from the top left.
        constexpr int segments = 8;
        const point centres[] = {
            {shape.left() + radius, shape.top() + radius}, {shape.right() - radius, shape.top() + radius},
            {shape.right() - radius, shape.bottom() - radius}, {shape.left() + radius, shape.bottom() - radius}
        };
        std::vector<point> points;
        points.reserve(4 * (segments + 1));
        for (int corner = 0; corner < 4; ++corner) {
            const double start = std::numbers::pi * (1 + corner / 2.0); // top left starts at 180 degrees
            for (int step = 0; step <= segments; ++step) {
                const double angle = start + std::numbers::pi / 2 * step / segments;
                points.emplace_back(centres[corner].x() + radius * std::cos(angle), centres[corner].y() + radius * std::sin(angle));
            }
        }
        paint_polygon(polygon{std::move(points)}, stroke, fill, width);
    }
}
