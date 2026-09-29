#pragma once
#include <algorithm>
#include <array>
#include <concepts>
#include <cmath>
#include <initializer_list>
#include <limits>
#include <ostream>
#include <stdexcept>
#include <utility>
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

    // Aliases...
    using point = point_<double>;
    using point_i = point_<int>;
    using size = size_<double>;
    using size_i = size_<int>;
    using rect = rect_<double>;
    using rect_i = rect_<int>;

    namespace detail {
        inline int polygon_coordinate(const double value) {
            if (!std::isfinite(value) || value < std::numeric_limits<int>::min() ||
                value > std::numeric_limits<int>::max())
                throw std::overflow_error{"Polygon coordinate exceeds integer range"};
            return static_cast<int>(std::lround(value));
        }
    }

    template<Numeric T>
    class rotated_rect_ {
    public:
        rotated_rect_(point_<T> center = {}, size_<T> size = {}, T angle = 0)
            : center_(center), size_(size), angle_(angle) {
        }

        explicit rotated_rect_(const rect_<T> &rectangle)
            : center_(rectangle.center()), size_(rectangle.size()), angle_(0) {
        }

        [[nodiscard]] point_<T> center() const { return center_; }
        point_<T> center(const point_<T> &value) { return center_ = value; }
        [[nodiscard]] size_<T> size() const { return size_; }
        size_<T> size(const size_<T> &value) { return size_ = value; }
        [[nodiscard]] T width() const { return size_.width(); }
        [[nodiscard]] T height() const { return size_.height(); }
        [[nodiscard]] T angle() const { return angle_; }
        T angle(const T value) { return angle_ = value; }

        [[nodiscard]] operator rect_<T>() const {
            const double radians = static_cast<double>(angle_) * std::acos(-1.0) / 180.0;
            const double cosine = std::abs(std::cos(radians));
            const double sine = std::abs(std::sin(radians));
            const double bounds_width = cosine * width() + sine * height();
            const double bounds_height = sine * width() + cosine * height();
            return {
                static_cast<T>(center_.x() - bounds_width / 2),
                static_cast<T>(center_.y() - bounds_height / 2),
                static_cast<T>(bounds_width),
                static_cast<T>(bounds_height)
            };
        }

        [[nodiscard]] operator polygon() const;

        bool operator==(const rotated_rect_ &) const = default;

    private:
        point_<T> center_;
        size_<T> size_;
        T angle_;
    };

    using rotated_rect = rotated_rect_<double>;
    using rotated_rect_i = rotated_rect_<int>;

    class polygon {
    public:
        using iterator = std::vector<point_i>::iterator;
        using const_iterator = std::vector<point_i>::const_iterator;

        polygon() = default;

        polygon(std::initializer_list<point_i> points) : points_(points) {
        }

        explicit polygon(std::vector<point_i> points) : points_(std::move(points)) {
        }

        [[nodiscard]] size_t size() const noexcept { return points_.size(); }
        [[nodiscard]] bool empty() const noexcept { return points_.empty(); }
        [[nodiscard]] point_i &operator[](const size_t index) { return points_[index]; }
        [[nodiscard]] const point_i &operator[](const size_t index) const { return points_[index]; }
        [[nodiscard]] iterator begin() noexcept { return points_.begin(); }
        [[nodiscard]] iterator end() noexcept { return points_.end(); }
        [[nodiscard]] const_iterator begin() const noexcept { return points_.begin(); }
        [[nodiscard]] const_iterator end() const noexcept { return points_.end(); }

        [[nodiscard]] bool is_rect() const noexcept {
            if (points_.size() != 4) return false;

            std::array<double, 4> edge_lengths{};
            for (size_t i = 0; i < points_.size(); ++i) {
                const auto &from = points_[i];
                const auto &to = points_[(i + 1) % points_.size()];
                edge_lengths[i] = std::hypot(static_cast<double>(to.x()) - from.x(),
                                             static_cast<double>(to.y()) - from.y());
                if (edge_lengths[i] == 0) return false;
            }

            const auto opposite_sides_match = [](const double lhs, const double rhs) {
                return std::abs(lhs - rhs) / std::max(lhs, rhs) <= 0.05;
            };
            if (!opposite_sides_match(edge_lengths[0], edge_lengths[2]) ||
                !opposite_sides_match(edge_lengths[1], edge_lengths[3]))
                return false;

            const double first_x = static_cast<double>(points_[0].x()) - points_[1].x();
            const double first_y = static_cast<double>(points_[0].y()) - points_[1].y();
            const double second_x = static_cast<double>(points_[2].x()) - points_[1].x();
            const double second_y = static_cast<double>(points_[2].y()) - points_[1].y();
            const double cosine = (first_x * second_x + first_y * second_y) /
                                  (edge_lengths[0] * edge_lengths[1]);
            const double angle = std::acos(std::clamp(cosine, -1.0, 1.0)) * 180.0 / std::acos(-1.0);
            return angle >= 80.0 && angle <= 100.0;
        }

        template<Numeric T>
        [[nodiscard]] operator rect_<T>() const {
            if (points_.empty()) return {};
            int left = points_.front().x();
            int right = left;
            int top = points_.front().y();
            int bottom = top;
            for (const auto &point: points_) {
                left = std::min(left, point.x());
                right = std::max(right, point.x());
                top = std::min(top, point.y());
                bottom = std::max(bottom, point.y());
            }
            return {
                static_cast<T>(left),
                static_cast<T>(top),
                static_cast<T>(right - left),
                static_cast<T>(bottom - top)
            };
        }

        template<Numeric T>
        [[nodiscard]] operator rotated_rect_<T>() const {
            if (points_.size() != 4)
                throw std::invalid_argument{"Rotated rectangle conversion requires four polygon points"};

            std::array<double, 4> edge_lengths{};
            std::array<point, 4> midpoints{};
            for (size_t i = 0; i < 4; ++i) {
                const auto &from = points_[i];
                const auto &to = points_[(i + 1) % 4];
                edge_lengths[i] = std::hypot(static_cast<double>(to.x()) - from.x(),
                                             static_cast<double>(to.y()) - from.y());
                midpoints[i] = {
                    (static_cast<double>(from.x()) + to.x()) / 2,
                    (static_cast<double>(from.y()) + to.y()) / 2
                };
            }
            if (std::ranges::any_of(edge_lengths, [](const double length) { return length == 0; }))
                throw std::invalid_argument{"Cannot convert polygon with zero-length sides to rotated rectangle"};

            const double pair_02 = edge_lengths[0] + edge_lengths[2];
            const double pair_13 = edge_lengths[1] + edge_lengths[3];
            const bool edges_02_are_short = pair_02 <= pair_13;
            const size_t first_short_edge = edges_02_are_short ? 0 : 1;
            const size_t second_short_edge = edges_02_are_short ? 2 : 3;
            const double width = edges_02_are_short
                                     ? (edge_lengths[1] + edge_lengths[3]) / 2
                                     : (edge_lengths[0] + edge_lengths[2]) / 2;
            const double height = edges_02_are_short ? pair_02 / 2 : pair_13 / 2;

            const double axis_x = midpoints[second_short_edge].x() - midpoints[first_short_edge].x();
            const double axis_y = midpoints[second_short_edge].y() - midpoints[first_short_edge].y();
            double angle = std::atan2(axis_y, axis_x) * 180.0 / std::acos(-1.0);
            if (angle < -90) angle += 180;
            else if (angle >= 90) angle -= 180;
            const double center_x = (static_cast<double>(points_[0].x()) + points_[1].x() +
                                     points_[2].x() + points_[3].x()) / 4;
            const double center_y = (static_cast<double>(points_[0].y()) + points_[1].y() +
                                     points_[2].y() + points_[3].y()) / 4;
            return rotated_rect_<T>{
                point_<T>{static_cast<T>(center_x), static_cast<T>(center_y)},
                size_<T>{static_cast<T>(width), static_cast<T>(height)},
                static_cast<T>(angle)
            };
        }

        [[nodiscard]] double width() const {
            const auto dimensions = rectangle_dimensions();
            return std::max(dimensions.edge_01, dimensions.edge_12);
        }

        [[nodiscard]] double height() const {
            const auto dimensions = rectangle_dimensions();
            return std::min(dimensions.edge_01, dimensions.edge_12);
        }

        /// Expands the longer dimension by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expand_w(const double amount) const {
            return expand_dimensions(amount, 0);
        }

        /// Expands the shorter dimension by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expand_h(const double amount) const {
            return expand_dimensions(0, amount);
        }

        /// Expands both dimensions by a fraction (<= 1) or pixels per side (> 1).
        [[nodiscard]] polygon expanded(const double amount) const {
            return expand_dimensions(amount, amount);
        }

        bool operator==(const polygon &) const = default;

    private:
        struct rectangle_geometry {
            double center_x;
            double center_y;
            double width_axis_x;
            double width_axis_y;
            double height_axis_x;
            double height_axis_y;
            double edge_01;
            double edge_12;
        };

        [[nodiscard]] rectangle_geometry rectangle_dimensions() const {
            if (points_.size() != 4)
                throw std::invalid_argument{"Rotated rectangle dimensions require a four-point polygon"};

            const auto edge = [this](const size_t start) {
                const auto &from = points_[start];
                const auto &to = points_[(start + 1) % points_.size()];
                return std::pair{
                    static_cast<double>(to.x()) - from.x(),
                    static_cast<double>(to.y()) - from.y()
                };
            };
            const auto [edge_01_x, edge_01_y] = edge(0);
            const auto [edge_12_x, edge_12_y] = edge(1);
            const double edge_01 = std::hypot(edge_01_x, edge_01_y);
            const double edge_12 = std::hypot(edge_12_x, edge_12_y);
            if (!is_rect()) throw std::invalid_argument{"Polygon is not a rotated rectangle"};

            const double center_x = (static_cast<double>(points_[0].x()) + points_[1].x() + points_[2].x() + points_[3].x()) / 4;
            const double center_y = (static_cast<double>(points_[0].y()) + points_[1].y() + points_[2].y() + points_[3].y()) / 4;
            const bool edge_01_is_width = edge_01 >= edge_12;
            const double width_edge = edge_01_is_width ? edge_01 : edge_12;
            const double height_edge = edge_01_is_width ? edge_12 : edge_01;
            const double width_dx = edge_01_is_width ? edge_01_x : edge_12_x;
            const double width_dy = edge_01_is_width ? edge_01_y : edge_12_y;
            const double height_dx = edge_01_is_width ? edge_12_x : edge_01_x;
            const double height_dy = edge_01_is_width ? edge_12_y : edge_01_y;
            return {
                center_x,
                center_y,
                width_dx / width_edge,
                width_dy / width_edge,
                height_dx / height_edge,
                height_dy / height_edge,
                edge_01,
                edge_12
            };
        }

        [[nodiscard]] polygon expand_dimensions(const double width_amount, const double height_amount) const {
            if (!std::isfinite(width_amount) || width_amount < 0 ||
                !std::isfinite(height_amount) || height_amount < 0)
                throw std::invalid_argument{"Polygon expansion must be finite and non-negative"};
            const auto geometry = rectangle_dimensions();
            const double width_edge = std::max(geometry.edge_01, geometry.edge_12);
            const double height_edge = std::min(geometry.edge_01, geometry.edge_12);
            const auto scale = [](const double amount, const double edge) {
                return amount <= 1 ? 1 + amount : (edge + 2 * amount) / edge;
            };
            const double width_scale = scale(width_amount, width_edge);
            const double height_scale = scale(height_amount, height_edge);

            polygon result;
            for (const auto &point: points_) {
                const double offset_x = point.x() - geometry.center_x;
                const double offset_y = point.y() - geometry.center_y;
                const double width_projection =
                        offset_x * geometry.width_axis_x + offset_y * geometry.width_axis_y;
                const double height_projection =
                        offset_x * geometry.height_axis_x + offset_y * geometry.height_axis_y;
                const double x = geometry.center_x +
                                 width_projection * width_scale * geometry.width_axis_x +
                                 height_projection * height_scale * geometry.height_axis_x;
                const double y = geometry.center_y +
                                 width_projection * width_scale * geometry.width_axis_y +
                                 height_projection * height_scale * geometry.height_axis_y;
                result.add_checked(x, y);
            }
            return result;
        }

        void add_checked(const double x, const double y) {
            if (!std::isfinite(x) || !std::isfinite(y) ||
                x < std::numeric_limits<int>::min() || x > std::numeric_limits<int>::max() ||
                y < std::numeric_limits<int>::min() || y > std::numeric_limits<int>::max())
                throw std::overflow_error{"Expanded polygon coordinates exceed integer range"};
            points_.emplace_back(static_cast<int>(std::lround(x)), static_cast<int>(std::lround(y)));
        }

        std::vector<point_i> points_;
    };

    template<typename T>
    rect_<T>::operator polygon() const {
        return polygon{
            point_i{detail::polygon_coordinate(left()), detail::polygon_coordinate(top())},
            point_i{detail::polygon_coordinate(right()), detail::polygon_coordinate(top())},
            point_i{detail::polygon_coordinate(right()), detail::polygon_coordinate(bottom())},
            point_i{detail::polygon_coordinate(left()), detail::polygon_coordinate(bottom())}
        };
    }

    template<typename T>
    rect_<T>::operator rotated_rect_<T>() const {
        return rotated_rect_<T>{center(), size(), T{0}};
    }

    template<Numeric T>
    rotated_rect_<T>::operator polygon() const {
        const double radians = static_cast<double>(angle_) * std::acos(-1.0) / 180.0;
        const double cosine = std::cos(radians);
        const double sine = std::sin(radians);
        const double half_width = static_cast<double>(width()) / 2;
        const double half_height = static_cast<double>(height()) / 2;
        const std::array<point, 4> local_points = {
            point{-half_width, -half_height},
            point{half_width, -half_height},
            point{half_width, half_height},
            point{-half_width, half_height}
        };
        std::vector<point_i> points;
        points.reserve(local_points.size());
        for (const auto &local: local_points) {
            const double x = center_.x() + local.x() * cosine - local.y() * sine;
            const double y = center_.y() + local.x() * sine + local.y() * cosine;
            points.emplace_back(detail::polygon_coordinate(x), detail::polygon_coordinate(y));
        }
        return polygon{std::move(points)};
    }
} // namespace sc
