#pragma once
#include "point.h"
#include "size.h"

#include <concepts>
#include <ostream>
#include <utility>
#include <vector>

namespace sc {
    class polygon;

    template<Numeric T>
    class rotated_rect_;

    template<typename T>
    class rect_ {
    public:
        rect_(T left = 0, T top = 0, T width = 0, T height = 0);

        rect_(point_<T> origin, size_<T> size);

        static rect_ ltrb(T left, T top, T right, T bottom) { return {left, top, right - left, bottom - top}; }

        template<typename U1, typename U2, typename U3, typename U4>
            requires(std::convertible_to<U1, T> && std::convertible_to<U2, T> && std::convertible_to<U3, T> && std::convertible_to<U4, T>)
        rect_(U1 left = 0, U2 top = 0, U3 width = 0, U4 height = 0) : rect_(static_cast<T>(left), static_cast<T>(top), static_cast<T>(width),
                                                                            static_cast<T>(height)) {
        }

        [[nodiscard]] T left() const;

        [[nodiscard]] T bottom() const;

        [[nodiscard]] T width() const;

        [[nodiscard]] T height() const;

        [[nodiscard]] T right() const;

        [[nodiscard]] T top() const;

        rect_ &operator+=(const rect_ &rhs);

        rect_ &operator+=(const T &i);

        template<typename U> requires std::convertible_to<U, T>
        rect_ &operator+=(const point_<U> &i) {
            origin_ += i;
            return *this;
        }

        template<typename U> requires std::convertible_to<U, T>
        rect_ &operator+=(const U &i) {
            return *this += static_cast<T>(i);
        }

        rect_ &operator-=(const rect_ &rhs);

        rect_ &operator-=(const T &i);

        template<typename U> requires std::convertible_to<U, T>
        rect_ &operator-=(const U &i) {
            return *this -= static_cast<T>(i);
        }

        template<typename U> requires std::convertible_to<U, T>
        rect_ &operator*=(const U &i) {
            origin_ *= i;
            rect_size_ *= i;
            return *this;
        }

        rect_ &operator*=(const size_<double> &rhs);

        template<typename U> requires std::convertible_to<U, T>
        rect_ operator*(const U &i) {
            auto tmp = *this;
            return tmp *= i;
        }

        rect_ operator+(const rect_ &r) const;

        rect_ operator+(const T &i) const;

        template<typename U> requires std::convertible_to<U, T>
        rect_ operator+(U i) const {
            return *this + static_cast<T>(i);
        }

        template<typename U> requires std::convertible_to<U, T>
        rect_ operator+(const point_<U> &i) {
            auto tmp = *this;
            return tmp += i;
        }

        rect_ operator-(const rect_ &r) const;

        rect_ operator-(const T &i) const;

        template<Numeric U>
        rect_ &operator-=(point_<U> i) {
            origin_ -= i;
            return *this;
        }

        template<Numeric U>
        rect_ operator-(point_<U> i) const {
            auto tmp = *this;
            return tmp -= i;
        }

        template<Numeric U>
        rect_ &operator/=(size_<U> i) {
            origin_ /= i;
            rect_size_ /= i;
            return *this;
        }

        template<Numeric U>
        rect_ operator/(size_<U> i) const {
            auto tmp = *this;
            return tmp /= i;
        }

        template<Numeric U>
        rect_ operator*(size_<U> i) const {
            auto tmp = *this;
            tmp.origin_ *= i;
            tmp.rect_size_ *= i;
            return tmp;
        }

        template<typename U>
            requires std::convertible_to<U, T>
        rect_ operator-(U i) const {
            return *this - static_cast<T>(i);
        }

        [[nodiscard]] point_<T> center() const;

        [[nodiscard]] size_<T> size() const;

        bool operator<(const rect_ &r) const;

        bool operator^(const rect_ &r) const;

        friend std::ostream &operator<<(std::ostream &lhs, const rect_ &rhs) { return lhs << rhs.origin_ << "x" << rhs.rect_size_; }

        [[nodiscard]] T area() const;

        [[nodiscard]] double iou(const rect_ &rhs) const;

        void include(const rect_ &rhs);

        [[nodiscard]] T distance(T cx, T cy) const;

        // Exact minimum distance between the two boxes (0 if they touch/overlap).
        // Symmetric: a.distance(b) == b.distance(a).
        [[nodiscard]] T distance(const rect_ &rhs) const;

        // Horizontal gap between x-ranges (0 if they overlap on x).
        [[nodiscard]] T gap_x(const rect_ &rhs) const;

        // Vertical gap between y-ranges (0 if they overlap on y).
        [[nodiscard]] T gap_y(const rect_ &rhs) const;

        [[nodiscard]] bool overlaps_x(const rect_ &rhs) const;

        [[nodiscard]] bool overlaps_y(const rect_ &rhs) const;

        [[nodiscard]] rect_ intersect(const rect_ &rhs) const;

        point_<T> left_top() const { return {left(), top()}; }
        point_<T> right_bottom() const { return {right(), bottom()}; }

        [[nodiscard]] operator polygon() const;

        [[nodiscard]] operator rotated_rect_<T>() const;

        static rect_ from_points(T left, T top, T right, T bottom);

        // Connected-component grouping (union-find, order-independent).
        // Boxes i and j are linked when iou > min_iou (min_iou == 1 disables)
        // OR box-to-box distance < max_dist (max_dist < 0 disables).
        // Group rect is the union of its members.
        static std::vector<std::pair<rect_, std::vector<size_t> > > group(const std::vector<rect_> &boxes, T min_iou = 0, T max_dist = 0);

        // Document-layout grouping with per-axis thresholds: boxes are linked when
        // gap_x < max_dx AND gap_y < max_dy. Use a strict max_dx (~1 char width) and
        // a generous max_dy (~1.5 line heights) so paragraphs merge vertically
        // without welding adjacent columns.
        static std::vector<std::pair<rect_, std::vector<size_t> > > group_adjacent(const std::vector<rect_> &boxes, T max_dx, T max_dy);

    protected:
        point_<T> origin_;
        size_<T> rect_size_;
    };

    using rect = rect_<double>;
    using rect_i = rect_<int>;
} // namespace sc
