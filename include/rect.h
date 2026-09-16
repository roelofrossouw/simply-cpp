#pragma once
#include <algorithm>
#include <concepts>
#include <ostream>
#include <vector>

namespace sc {
    template<typename T>
    concept Numeric = std::integral<T> || std::floating_point<T>;

    template<typename Derived, Numeric T>
    class pair_ {
    public:
        pair_(const T &x = 0, const T &y = 0);

        template<typename U1, typename U2>
            requires(std::convertible_to<U1, T> && std::convertible_to<U2, T>)
        pair_(U1 x, U2 y) : pair_(static_cast<T>(x), static_cast<T>(y)) {
        }

        template<typename U, typename V>
        operator pair_<U, V>() const {
            return {x_, y_};
        }

        operator Derived() { return {x_, y_}; }

        bool operator==(const pair_ &) const = default;

        // + operators...
        Derived operator+=(const pair_ &r) {
            x_ += r.x_;
            y_ += r.y_;
            validate();
            return *this;
        }

        Derived operator+(const pair_ &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        template<Numeric U>
        Derived operator+=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ += value;
            y_ += value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator+(const U &r) const {
            auto tmp = *this;
            return tmp += r;
        }

        // - operators...
        Derived operator-=(const pair_ &r) {
            x_ -= r.x_;
            y_ -= r.y_;
            validate();
            return *this;
        }

        Derived operator-(const pair_ &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        template<Numeric U>
        Derived operator-=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ -= value;
            y_ -= value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator-(const U &r) const {
            auto tmp = *this;
            return tmp -= r;
        }

        // * operators...
        Derived operator*=(const pair_ &r) {
            x_ *= r.x_;
            y_ *= r.y_;
            validate();
            return *this;
        }

        Derived operator*(const pair_ &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        template<Numeric U>
        Derived operator*=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ *= value;
            y_ *= value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator*(const U &r) const {
            auto tmp = *this;
            return tmp *= r;
        }

        // / operators...
        Derived operator/=(const pair_ &r) {
            x_ /= r.x_;
            y_ /= r.y_;
            validate();
            return *this;
        }

        Derived operator/(const pair_ &r) const {
            auto tmp = *this;
            return tmp /= r;
        }

        template<Numeric U>
        Derived operator/=(const U &r) {
            const auto value = static_cast<T>(r);
            x_ /= value;
            y_ /= value;
            validate();
            return *this;
        }

        template<Numeric U>
        Derived operator/(const U &r) const {
            auto tmp = *this;
            return tmp /= r;
        }

    protected:
        T x_, y_;

        void validate() {
            if constexpr (requires(Derived &d) { d.on_validate(); })
                static_cast<Derived *>(this)->on_validate();
        }

    private:
        template<typename A, typename B>
            requires(!std::same_as<A, Derived> || !std::same_as<B, T>)
        friend bool operator==(const pair_ &a, const pair_<A, B> &b) {
            return a == static_cast<pair_>(b);
        }
    };

    template<typename T>
    class point_ : public pair_<point_<T>, T> {
    public:
        using pair_<point_<T>, T>::pair_;

        template<typename U>
        operator point_<U>() {
            return {this->x_, this->y_};
        }

        T x() const { return this->x_; }
        T x(const T &r) { return this->x_ = r; }
        T y() const { return this->y_; }
        T y(const T &r) { return this->y_ = r; }

    private:
        friend std::ostream &operator<<(std::ostream &lhs, const point_ &rhs) { return lhs << "(" << rhs.x_ << "," << rhs.y_ << ")"; }
    };

    template<typename T>
    class size_ : public pair_<size_<T>, T> {
    public:
        using pair_<size_<T>, T>::pair_;

        T width() const { return this->x_; }
        T width(const T &r) { return this->x_ = r; }
        T height() const { return this->y_; }
        T height(const T &r) { return this->y_ = r; }
        T area() const { return width() * height(); }
        friend class pair_<size_, T>;

    private:
        void on_validate();

        friend std::ostream &operator<<(std::ostream &lhs, const size_ &rhs) { return lhs << "[" << rhs.x_ << "," << rhs.y_ << "]"; }
    };

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

        rect_ &operator-=(const rect_ &rhs);

        rect_ &operator+=(const T &i);

        rect_ &operator-=(const T &i);

        template<typename U>
            requires std::convertible_to<U, T>
        rect_ &operator+=(const U &i) {
            return *this += static_cast<T>(i);
        }

        template<typename U>
            requires std::convertible_to<U, T>
        rect_ &operator-=(const U &i) {
            return *this -= static_cast<T>(i);
        }

        rect_ &operator*=(const size_<double> &rhs);

        rect_ operator+(const rect_ &r) const;

        rect_ operator+(const T &i) const;

        template<typename U>
            requires std::convertible_to<U, T>
        rect_ operator+(U i) const {
            return *this + static_cast<T>(i);
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

    // Aliases...
    using point = point_<double>;
    using point_i = point_<int>;
    using size = size_<double>;
    using size_i = size_<int>;
    using rect = rect_<double>;
    using rect_i = rect_<int>;
} // namespace sc
