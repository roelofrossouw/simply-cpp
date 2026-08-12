#include "rect.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <ostream>

namespace {
    // Minimal union-find with path halving.
    struct dsu {
        std::vector<size_t> parent;

        explicit dsu(const size_t n) : parent(n) {
            std::iota(parent.begin(), parent.end(), size_t{0});
        }

        size_t find(size_t a) {
            while (parent[a] != a) a = parent[a] = parent[parent[a]];
            return a;
        }

        void unite(const size_t a, const size_t b) {
            parent[find(a)] = find(b);
        }
    };

    // Assemble (union-rect, member-indices) groups from a linkage predicate.
    template<typename Linked>
    std::vector<std::pair<sc::rect, std::vector<size_t> > >
    components(const std::vector<sc::rect> &boxes, Linked &&linked) {
        const size_t n = boxes.size();
        dsu ds(n);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j)
                if (linked(boxes[i], boxes[j])) ds.unite(i, j);

        std::vector<std::pair<sc::rect, std::vector<size_t> > > groups;
        std::vector<long> slot(n, -1); // root index -> position in groups
        for (size_t i = 0; i < n; ++i) {
            const size_t root = ds.find(i);
            if (slot[root] < 0) {
                slot[root] = static_cast<long>(groups.size());
                groups.emplace_back(boxes[i], std::vector<size_t>{});
            }
            auto &[bbox, members] = groups[static_cast<size_t>(slot[root])];
            bbox.include(boxes[i]);
            members.push_back(i);
        }
        return groups;
    }
} // namespace

namespace sc {
    rect::rect(const double left, const double top, const double width, const double height)
        : x(left), y(top), w(std::max(0.0, width)), h(std::max(0.0, height)) {
    }

    rect::rect() : x(0), y(0), w(0), h(0) {
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

    rect rect::operator+(const int i) const {
        return {x - i, y - i, w + i + i, h + i + i};
    }

    rect rect::operator+(const rect &r) const {
        return {x + r.x, y + r.y, w + r.w, h + r.h};
    }

    rect rect::operator-(const int i) const {
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

    bool rect::operator^(const rect &r) const {
        return y < r.y || (y == r.y && x < r.x);
    }

    double rect::area() const {
        return w * h;
    }

    rect rect::intersect(const rect &rhs) const {
        const double x0 = std::max(x, rhs.left());
        const double y0 = std::max(y, rhs.top());
        return {x0, y0, std::min(right(), rhs.right()) - x0, std::min(bottom(), rhs.bottom()) - y0};
    }

    rect rect::from_points(const double left, const double top, const double right, const double bottom) {
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

    double rect::gap_x(const rect &rhs) const {
        return std::max({left() - rhs.right(), rhs.left() - right(), 0.0});
    }

    double rect::gap_y(const rect &rhs) const {
        return std::max({top() - rhs.bottom(), rhs.top() - bottom(), 0.0});
    }

    bool rect::overlaps_x(const rect &rhs) const {
        return gap_x(rhs) == 0.0;
    }

    bool rect::overlaps_y(const rect &rhs) const {
        return gap_y(rhs) == 0.0;
    }

    double rect::distance(const rect &rhs) const {
        const double dx = gap_x(rhs);
        const double dy = gap_y(rhs);
        if (dx == 0 && dy == 0) return 0;
        return std::hypot(dx, dy);
    }

    std::ostream &operator<<(std::ostream &lhs, const sc::rect &rhs) {
        lhs << "(" << rhs.x << "," << rhs.y << ")x[" << rhs.w << "," << rhs.h << "]";
        return lhs;
    }

    std::vector<std::pair<rect, std::vector<size_t> > > rect::group(
        const std::vector<rect> &boxes, const double min_iou, const double max_dist) {
        if (boxes.empty()) return {};
        return components(boxes, [min_iou, max_dist](const rect &a, const rect &b) {
            if (min_iou < 1 && a.iou(b) > min_iou) return true;          // min_iou == 1 disables
            if (max_dist >= 0 && a.distance(b) < max_dist) return true;  // max_dist < 0 disables
            return false;
        });
    }

    std::vector<std::pair<rect, std::vector<size_t> > > rect::group_adjacent(
        const std::vector<rect> &boxes, const double max_dx, const double max_dy) {
        if (boxes.empty()) return {};
        return components(boxes, [max_dx, max_dy](const rect &a, const rect &b) {
            return a.gap_x(b) < max_dx && a.gap_y(b) < max_dy;
        });
    }
}
