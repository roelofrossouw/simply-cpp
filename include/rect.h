#ifndef SC_RECT_H
#define SC_RECT_H

#include <iostream>
#include <vector>

namespace sc {
    class rect {
    public:
        rect(double left, double bottom, double width = 0, double height = 0);

        rect();

        rect(const rect &r);

        rect &operator=(const rect &r) = default;

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

        double middle() const;

        double center() const;

        bool operator<(const rect &r) const;

        bool operator^(const rect &r) const;

        friend std::ostream &operator<<(std::ostream &lhs, const sc::rect &rhs);

        double area() const;

        double iou(const rect &rhs) const;

        void include(const rect &rhs);

        double distance(double cx, double cy) const;

        double distance(const rect &rhs) const;

        rect intersect(const rect &rhs) const;

        static rect from_points(double left, double top, double right, double bottom);

        rect centroid() const;

        static std::vector<std::pair<rect, std::vector<size_t> > > group(const std::vector<rect> &boxes, double min_iou = 0, double max_dist = 0);

    protected:
        double x, y, w, h;
    };
} // sc

#endif //SC_RECT_H
