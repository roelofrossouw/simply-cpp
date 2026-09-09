#ifndef SC_RECT_H
#define SC_RECT_H

#include <ostream>
#include <vector>

namespace sc
{
    template <typename T>
    class rect_
    {
    public:
        rect_(T left, T top, T width = 0, T height = 0);

        template <typename U1, typename U2, typename U3, typename U4>
            requires (std::convertible_to<U1, T> && std::convertible_to<U2, T>
                && std::convertible_to<U3, T> && std::convertible_to<U4, T>)
        rect_(U1 left, U2 top, U3 width = 0, U4 height = 0)
            : rect_(static_cast<T>(left), static_cast<T>(top), static_cast<T>(width), static_cast<T>(height))
        {
        }

        rect_();

        [[nodiscard]] T left() const;

        [[nodiscard]] T bottom() const;

        [[nodiscard]] T width() const;

        [[nodiscard]] T height() const;

        [[nodiscard]] T right() const;

        [[nodiscard]] T top() const;

        rect_& operator+=(const rect_& rhs);

        rect_& operator-=(const rect_& rhs);

        rect_ operator+(T i) const;

        template <typename U>
            requires std::convertible_to<U, T>
        rect_ operator+(U i) const { return *this + static_cast<T>(i); }

        rect_ operator+(const rect_& r) const;

        rect_ operator-(T i) const;

        template <typename U>
            requires std::convertible_to<U, T>
        rect_ operator-(U i) const { return *this - static_cast<T>(i); }

        [[nodiscard]] T middle() const;

        [[nodiscard]] T center() const;

        bool operator<(const rect_& r) const;

        bool operator^(const rect_& r) const;

        friend std::ostream& operator<<(std::ostream& lhs, const rect_& rhs)
        {
            return lhs << "(" << rhs.x << "," << rhs.y << ")x[" << rhs.w << "," << rhs.h << "]";
        }

        [[nodiscard]] T area() const;

        [[nodiscard]] double iou(const rect_& rhs) const;

        void include(const rect_& rhs);

        [[nodiscard]] T distance(T cx, T cy) const;

        // Exact minimum distance between the two boxes (0 if they touch/overlap).
        // Symmetric: a.distance(b) == b.distance(a).
        [[nodiscard]] T distance(const rect_& rhs) const;

        // Horizontal gap between x-ranges (0 if they overlap on x).
        [[nodiscard]] T gap_x(const rect_& rhs) const;

        // Vertical gap between y-ranges (0 if they overlap on y).
        [[nodiscard]] T gap_y(const rect_& rhs) const;

        [[nodiscard]] bool overlaps_x(const rect_& rhs) const;

        [[nodiscard]] bool overlaps_y(const rect_& rhs) const;

        [[nodiscard]] rect_ intersect(const rect_& rhs) const;

        static rect_ from_points(T left, T top, T right, T bottom);

        [[nodiscard]] rect_ centroid() const;

        // Connected-component grouping (union-find, order-independent).
        // Boxes i and j are linked when iou > min_iou (min_iou == 1 disables)
        // OR box-to-box distance < max_dist (max_dist < 0 disables).
        // Group rect is the union of its members.
        static std::vector<std::pair<rect_, std::vector<size_t>>> group(
            const std::vector<rect_>& boxes, T min_iou = 0, T max_dist = 0);

        // Document-layout grouping with per-axis thresholds: boxes are linked when
        // gap_x < max_dx AND gap_y < max_dy. Use a strict max_dx (~1 char width) and
        // a generous max_dy (~1.5 line heights) so paragraphs merge vertically
        // without welding adjacent columns.
        static std::vector<std::pair<rect_, std::vector<size_t>>> group_adjacent(
            const std::vector<rect_>& boxes, T max_dx, T max_dy);

    protected:
        T x, y, w, h;
    };

    using rect = rect_<double>;
    using rect_i = rect_<int>;
    using rect_f = rect_<float>;
} // sc

#endif //SC_RECT_H
