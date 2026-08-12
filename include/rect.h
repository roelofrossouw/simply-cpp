#ifndef SC_RECT_H
#define SC_RECT_H

#include <iosfwd>
#include <vector>

namespace sc {
    class rect {
    public:
        rect(double left, double top, double width = 0, double height = 0);

        rect();

        [[nodiscard]] double left() const;

        [[nodiscard]] double bottom() const;

        [[nodiscard]] double width() const;

        [[nodiscard]] double height() const;

        [[nodiscard]] double right() const;

        [[nodiscard]] double top() const;

        rect &operator+=(const rect &rhs);

        rect &operator-=(const rect &rhs);

        rect operator+(int i) const;

        rect operator+(const rect &r) const;

        rect operator-(int i) const;

        [[nodiscard]] double middle() const;

        [[nodiscard]] double center() const;

        bool operator<(const rect &r) const;

        bool operator^(const rect &r) const;

        friend std::ostream &operator<<(std::ostream &lhs, const sc::rect &rhs);

        [[nodiscard]] double area() const;

        [[nodiscard]] double iou(const rect &rhs) const;

        void include(const rect &rhs);

        [[nodiscard]] double distance(double cx, double cy) const;

        // Exact minimum distance between the two boxes (0 if they touch/overlap).
        // Symmetric: a.distance(b) == b.distance(a).
        [[nodiscard]] double distance(const rect &rhs) const;

        // Horizontal gap between x-ranges (0 if they overlap on x).
        [[nodiscard]] double gap_x(const rect &rhs) const;

        // Vertical gap between y-ranges (0 if they overlap on y).
        [[nodiscard]] double gap_y(const rect &rhs) const;

        [[nodiscard]] bool overlaps_x(const rect &rhs) const;

        [[nodiscard]] bool overlaps_y(const rect &rhs) const;

        [[nodiscard]] rect intersect(const rect &rhs) const;

        static rect from_points(double left, double top, double right, double bottom);

        [[nodiscard]] rect centroid() const;

        // Connected-component grouping (union-find, order-independent).
        // Boxes i and j are linked when iou > min_iou (min_iou == 1 disables)
        // OR box-to-box distance < max_dist (max_dist < 0 disables).
        // Group rect is the union of its members.
        static std::vector<std::pair<rect, std::vector<size_t> > > group(
            const std::vector<rect> &boxes, double min_iou = 0, double max_dist = 0);

        // Document-layout grouping with per-axis thresholds: boxes are linked when
        // gap_x < max_dx AND gap_y < max_dy. Use a strict max_dx (~1 char width) and
        // a generous max_dy (~1.5 line heights) so paragraphs merge vertically
        // without welding adjacent columns.
        static std::vector<std::pair<rect, std::vector<size_t> > > group_adjacent(
            const std::vector<rect> &boxes, double max_dx, double max_dy);

    protected:
        double x, y, w, h;
    };
} // sc

#endif //SC_RECT_H
