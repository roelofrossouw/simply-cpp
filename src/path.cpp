#include "path.h"

#include <algorithm>
#include <cmath>

namespace sc {
    path &path::move_to(const point &to) {
        steps_.push_back({kind::move, to});
        return *this;
    }

    path &path::line_to(const point &to) {
        if (steps_.empty()) return move_to(to);
        steps_.push_back({kind::line, to});
        return *this;
    }

    path &path::cubic_to(const point &first, const point &second, const point &to) {
        if (steps_.empty()) move_to(first);
        steps_.push_back({kind::cubic, to, first, second});
        return *this;
    }

    path &path::close() {
        if (!steps_.empty() && steps_.back().type != kind::close) steps_.push_back({kind::close, {}});
        return *this;
    }

    rect path::bounds() const {
        bool any = false;
        double left = 0, top = 0, right = 0, bottom = 0;
        const auto include = [&](const point &p) {
            if (!any) {
                left = right = p.x();
                top = bottom = p.y();
                any = true;
                return;
            }
            left = std::min(left, p.x());
            right = std::max(right, p.x());
            top = std::min(top, p.y());
            bottom = std::max(bottom, p.y());
        };
        // A cubic's extent: its end points and, per axis, where it turns: the roots in (0, 1) of its
        // derivative, the quadratic a t^2 + b t + c (divided by 3).
        const auto turns = [](const double p0, const double p1, const double p2, const double p3, double roots[2]) {
            const double a = -p0 + 3 * p1 - 3 * p2 + p3, b = 2 * (p0 - 2 * p1 + p2), c = p1 - p0;
            int count = 0;
            const auto keep = [&](const double t) {
                if (t > 0 && t < 1) roots[count++] = t;
            };
            if (std::abs(a) < 1e-12) {
                if (std::abs(b) > 1e-12) keep(-c / b);
            } else if (const double d = b * b - 4 * a * c; d >= 0) {
                const double root = std::sqrt(d);
                keep((-b + root) / (2 * a));
                keep((-b - root) / (2 * a));
            }
            return count;
        };
        const auto at = [](const point &p0, const point &p1, const point &p2, const point &p3, const double t) {
            const double u = 1 - t, a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
            return point{a * p0.x() + b * p1.x() + c * p2.x() + d * p3.x(), a * p0.y() + b * p1.y() + c * p2.y() + d * p3.y()};
        };
        point current{};
        for (const auto &s: steps_) {
            if (s.type == kind::close) continue; // back to its subpath's start, already counted
            if (s.type == kind::cubic) {
                double roots[2];
                for (int axis = 0; axis < 2; ++axis) {
                    const auto coordinate = [axis](const point &p) { return axis == 0 ? p.x() : p.y(); };
                    const int n = turns(coordinate(current), coordinate(s.first), coordinate(s.second), coordinate(s.to), roots);
                    for (int i = 0; i < n; ++i) include(at(current, s.first, s.second, s.to, roots[i]));
                }
            }
            include(s.to);
            current = s.to;
        }
        return rect::ltrb(left, top, right, bottom);
    }

    path path::transformed(const double scale_x, const double scale_y, const point &offset) const {
        const auto map = [&](const point &p) { return point{p.x() * scale_x + offset.x(), p.y() * scale_y + offset.y()}; };
        path result;
        result.steps_.reserve(steps_.size());
        for (const auto &s: steps_) {
            if (s.type == kind::close) result.steps_.push_back(s);
            else result.steps_.push_back({s.type, map(s.to), map(s.first), map(s.second)});
        }
        return result;
    }

    std::vector<path::outline> path::flattened(const double tolerance) const {
        std::vector<outline> result;
        std::vector<point> current;
        point start, at;
        const auto finish = [&](const bool closed) {
            if (current.size() >= 2) result.push_back({polygon{std::move(current)}, closed});
            current.clear();
        };
        for (const auto &s: steps_) {
            switch (s.type) {
                case kind::move:
                    finish(false);
                    start = at = s.to;
                    current.push_back(at);
                    break;
                case kind::line:
                    if (current.empty()) current.push_back(at); // carrying on after a close
                    at = s.to;
                    current.push_back(at);
                    break;
                case kind::cubic: {
                    if (current.empty()) current.push_back(at);
                    // Enough segments that none strays more than tolerance from the curve: the
                    // control polygon's length bounds the curve's, and its sag shrinks with 1/n^2.
                    const double reach = std::hypot(s.first.x() - at.x(), s.first.y() - at.y()) +
                                         std::hypot(s.second.x() - s.first.x(), s.second.y() - s.first.y()) +
                                         std::hypot(s.to.x() - s.second.x(), s.to.y() - s.second.y());
                    const int segments = std::clamp(static_cast<int>(std::ceil(std::sqrt(reach / std::max(tolerance, 1e-3)))), 1, 256);
                    for (int i = 1; i <= segments; ++i) {
                        const double t = static_cast<double>(i) / segments, u = 1 - t;
                        const double a = u * u * u, b = 3 * u * u * t, c = 3 * u * t * t, d = t * t * t;
                        current.emplace_back(a * at.x() + b * s.first.x() + c * s.second.x() + d * s.to.x(),
                                             a * at.y() + b * s.first.y() + c * s.second.y() + d * s.to.y());
                    }
                    at = s.to;
                    break;
                }
                case kind::close:
                    finish(true);
                    at = start;
                    break;
            }
        }
        finish(false);
        return result;
    }
}
