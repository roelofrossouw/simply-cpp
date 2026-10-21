#pragma once
#include "point.h"
#include "size.h"

#include <concepts>
#include <ostream>
#include <utility>
#include <vector>

namespace sc {
    template<Numeric T>
    class polygon_;

    template<Numeric T>
    class rotated_rect_;

    /// An upright rectangle: left, top, width and height; rect (double) or rect_i (whole pixels).
    /// y grows downwards, so bottom() is top() + height(). The size is never negative.
    template<typename T>
    class rect_ {
    public:
        rect_(T left = 0, T top = 0, T width = 0, T height = 0);

        rect_(point_<T> origin, size_<T> size);

        /// From the other flavour: rect_i to rect implicitly; rect to rect_i explicitly, rounding
        /// each of left, top, width and height (std::overflow_error when one doesn't fit an int).
        template<typename U> requires (!std::same_as<T, U>)
        explicit(std::integral<T> && std::floating_point<U>) rect_(const rect_<U> &other)
            : rect_(detail::coordinate<T>(static_cast<double>(other.left())), detail::coordinate<T>(static_cast<double>(other.top())),
                    detail::coordinate<T>(static_cast<double>(other.width())), detail::coordinate<T>(static_cast<double>(other.height()))) {
        }

        /// From its edges.
        static rect_ ltrb(T left, T top, T right, T bottom) { return {left, top, right - left, bottom - top}; }

        /// Itself (see sc::geometry).
        [[nodiscard]] rect_ bounds() const { return *this; }

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

        /// Grows by i on every side (-= shrinks); adding a point moves it instead.
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
        rect_ operator*(const U &i) const {
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

        /// Orders left to right, then top to bottom; ^ orders top to bottom, then left to right
        /// (reading order).
        bool operator<(const rect_ &r) const;

        bool operator==(const rect_ &) const = default;

        bool operator^(const rect_ &r) const;

        friend std::ostream &operator<<(std::ostream &lhs, const rect_ &rhs) { return lhs << rhs.origin_ << "x" << rhs.rect_size_; }

        [[nodiscard]] T area() const;

        /// Intersection over union: 0 for boxes apart, 1 for the same box.
        [[nodiscard]] double iou(const rect_ &rhs) const;

        /// Grows to cover rhs as well.
        void include(const rect_ &rhs);

        /// Distance from the point (cx, cy) to the box; 0 inside it.
        [[nodiscard]] T distance(T cx, T cy) const;

        /// The shortest distance between the two boxes; 0 when they touch or overlap.
        [[nodiscard]] T distance(const rect_ &rhs) const;

        /// The horizontal gap between their x ranges; 0 when they overlap on x.
        [[nodiscard]] T gap_x(const rect_ &rhs) const;

        /// The vertical gap between their y ranges; 0 when they overlap on y.
        [[nodiscard]] T gap_y(const rect_ &rhs) const;

        [[nodiscard]] bool overlaps_x(const rect_ &rhs) const;

        [[nodiscard]] bool overlaps_y(const rect_ &rhs) const;

        /// The overlap; empty (no width or height) when they don't overlap.
        [[nodiscard]] rect_ intersect(const rect_ &rhs) const;

        point_<T> left_top() const { return {left(), top()}; }
        point_<T> right_bottom() const { return {right(), bottom()}; }

        /// Its corners (left-top, right-top, right-bottom, left-bottom); rounded for polygon_i,
        /// throwing std::overflow_error when one doesn't fit an int.
        template<Numeric U>
        [[nodiscard]] operator polygon_<U>() const;

        [[nodiscard]] operator rotated_rect_<T>() const;

        /// The same as ltrb().
        static rect_ from_points(T left, T top, T right, T bottom);

        /// Groups boxes that are linked, directly or through others (order doesn't matter): linked
        /// when iou > min_iou (min_iou 1 turns that off) or distance < max_dist (max_dist < 0 turns
        /// that off). Each group is the box around its members, and their indices.
        static std::vector<std::pair<rect_, std::vector<size_t> > > group(const std::vector<rect_> &boxes, T min_iou = 0, T max_dist = 0);

        /// group() for page layout, by axis: linked when gap_x < max_dx and gap_y < max_dy. A tight
        /// max_dx (about a character) and a generous max_dy (about 1.5 lines) merge a paragraph's
        /// lines without joining neighbouring columns.
        static std::vector<std::pair<rect_, std::vector<size_t> > > group_adjacent(const std::vector<rect_> &boxes, T max_dx, T max_dy);

    protected:
        point_<T> origin_;
        size_<T> rect_size_;
    };

    using rect = rect_<double>;
    using rect_i = rect_<int>;
}

// The polygon conversion is defined there, where polygon_ is complete.
#include "polygon.h"
