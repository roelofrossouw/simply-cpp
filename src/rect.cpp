#include "rect.h"
#include <ranges>
#include <algorithm>
#include <math.h>

namespace sc {
    rect::rect(const double left, const double top, const double width, const double height)
        : x(left), y(top), w(std::max(0.0, width)), h(std::max(0.0, height)) {
    }

    rect::rect() : x(0), y(0), w(0), h(0) {
    }

    rect::rect(const rect &r) : x(r.x), y(r.y), w(r.w), h(r.h) {
    }

    double rect::left() const {
        return x;
    }

    double rect::top() const {
        return y;
    }

    double rect::width() const {
        return w;
    }

    double rect::height() const {
        return h;
    }

    double rect::right() const {
        return x + w;
    }

    double rect::bottom() const {
        return y + h;
    }

    rect &rect::operator+=(const rect &rhs) {
        x += rhs.x;
        y += rhs.y;
        w += rhs.w;
        h += rhs.h;
        return *this;
    }

    rect &rect::operator-=(const rect &rhs) {
        x -= rhs.x;
        y -= rhs.y;
        w = std::max(0.0, w - rhs.w);
        h = std::max(0.0, h - rhs.h);
        return *this;
    }

    rect rect::operator+(int i) const {
        return {x - i, y - i, w + i + i, h + i + i};
    }

    rect rect::operator+(const rect &r) const {
        return {x + r.x, y + r.y, w + r.w, h + r.h};
    }

    rect rect::operator-(int i) const {
        return {x + i, y + i, w - i - i, h - i - i};
    }

    double rect::middle() const {
        return y + h / 2;
    }

    double rect::center() const {
        return x + w / 2;
    }

    bool rect::operator<(const rect &r) const {
        return x < r.x || (x == r.x && y < r.y);
    }

    double rect::area() const {
        return w * h;
    }

    rect rect::intersect(const rect &rhs) const {
        const double x0 = std::max(x, rhs.left());
        const double y0 = std::max(y, rhs.top());
        return {x0, y0, std::min(right(), rhs.right()) - x0, std::min(bottom(), rhs.bottom()) - y0};
    }

    rect rect::from_points(double left, double top, double right, double bottom) {
        return {left, top, right - left, bottom - top};
    }

    rect rect::centroid() const {
        return {center(), middle(), 0, 0};
    }

    double rect::iou(const rect &rhs) const {
        const double interArea = intersect(rhs).area();
        if (interArea == 0.0) return 0.0;
        const double unionArea = area() + rhs.area() - interArea;
        if (unionArea == 0.0) return 0.0;
        return interArea / unionArea;
    }

    void rect::include(const rect &rhs) {
        w = std::max(right(), rhs.right()) - std::min(x, rhs.x);
        h = std::max(bottom(), rhs.bottom()) - std::min(y, rhs.y);
        x = std::min(x, rhs.x);
        y = std::min(y, rhs.y);
    }

    double rect::distance(const double cx, const double cy) const {
        const double dx = std::max({left() - cx, 0.0, cx - right()});
        const double dy = std::max({top() - cy, 0.0, cy - bottom()});
        if (dx == 0 && dy == 0) return 0;
        return std::hypot(dx, dy);
    }

    double rect::distance(const rect &rhs) const {
        // Shortcut if no size, just one check.
        // if (rhs.w == 0 && rhs.h == 0) return distance(rhs.x, rhs.h);
        // Shortcut for either w == 0 or h == 0 could be done, but both probability and savings are low.
        // Min distance to each corner.
        return std::min({
                distance(rhs.x, rhs.y),
                distance(rhs.x, rhs.bottom()),
                distance(rhs.right(), rhs.y),
                distance(rhs.right(), rhs.bottom())
            }
        );
    }

    std::ostream &operator<<(std::ostream &lhs, const sc::rect &rhs) {
        lhs << "(" << rhs.x << "," << rhs.y << ")x[" << rhs.w << "," << rhs.h << "]";
        return lhs;
    }

    std::vector<std::pair<rect, std::vector<size_t> > > rect::group(const std::vector<rect> &boxes, double min_iou, double max_dist) {
        if (boxes.empty()) return {};
        std::vector<std::pair<rect, std::vector<size_t> > > groups;

        for (size_t i = 0; i < boxes.size(); ++i) {
            bool assigned = false;

            for (auto &group: groups) {
                // IOU == 1 disables iou check
                if (min_iou < 1) {
                    // 1. Check IoU with any member in the group
                    for (size_t idx = 0; idx < group.second.size(); ++idx) {
                        if (boxes[i].iou(boxes[group.second[idx]]) > min_iou) {
                            group.second.push_back(i);
                            group.first.include(boxes[i]);
                            assigned = true;
                            break;
                        }
                    }
                }
                if (assigned) break;

                // max_dist < 0 disables proximity check
                if (max_dist >= 0) {
                    // 2. Check proximity to the nearest edge of the group's bounding box
                    // TODO: Not sure if centroid, or closest distance would be best...
                    // std::cout << group.first.distance(boxes[i].centroid()) << " vs " << group.first.distance(boxes[i]) << "\n";
                    if (group.first.distance(boxes[i].centroid()) < max_dist) {
                        group.second.push_back(i);
                        group.first.include(boxes[i]);
                        assigned = true;
                        break;
                    }
                }
            }

            if (!assigned) {
                rect current_box{boxes[i]};
                groups.push_back(std::make_pair<rect, std::vector<size_t> >(std::move(current_box), {i}));
            }
        }

        return groups;
    }
}
