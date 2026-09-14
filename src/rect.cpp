#include "rect.h"

#include <algorithm>
#include <cmath>
#include <numeric>
#include <ostream>

namespace
{
    // Minimal union-find with path halving.
    struct dsu
    {
        std::vector<size_t> parent;

        explicit dsu(const size_t n) : parent(n)
        {
            std::iota(parent.begin(), parent.end(), size_t{0});
        }

        size_t find(size_t a)
        {
            while (parent[a] != a) a = parent[a] = parent[parent[a]];
            return a;
        }

        void unite(const size_t a, const size_t b)
        {
            parent[find(a)] = find(b);
        }
    };

    // Assemble (union-rect, member-indices) groups from a linkage predicate.
    template <typename Linked, typename T>
    std::vector<std::pair<sc::rect_<T>, std::vector<size_t>>>
    components(const std::vector<sc::rect_<T>>& boxes, Linked&& linked)
    {
        const size_t n = boxes.size();
        dsu ds(n);
        for (size_t i = 0; i < n; ++i)
            for (size_t j = i + 1; j < n; ++j)
                if (linked(boxes[i], boxes[j])) ds.unite(i, j);

        std::vector<std::pair<sc::rect_<T>, std::vector<size_t>>> groups;
        std::vector<long> slot(n, -1); // root index -> position in groups
        for (size_t i = 0; i < n; ++i)
        {
            const size_t root = ds.find(i);
            if (slot[root] < 0)
            {
                slot[root] = static_cast<long>(groups.size());
                groups.emplace_back(boxes[i], std::vector<size_t>{});
            }
            auto& [bbox, members] = groups[static_cast<size_t>(slot[root])];
            bbox.include(boxes[i]);
            members.push_back(i);
        }
        return groups;
    }
} // namespace

namespace sc
{
    template <typename Derived, Numeric T>
    pair_<Derived, T>::pair_(const T& x, const T& y) : x_(x), y_(y)
    {
        validate();
    }

    template <typename T>
    T size_<T>::width() const
    {
        return this->x_;
    }

    template <typename T>
    void size_<T>::on_validate()
    {
        this->x_ = std::max(T{0}, this->x_);
        this->y_ = std::max(T{0}, this->y_);
    }

    template <typename T>
    rect_<T>::rect_(const T left, const T top, const T width, const T height) : origin_(left, top), rect_size_(width, height)
    {
    }

    template <typename T>
    rect_<T>::rect_(point_<T> origin, ::sc::size_<T> size) : origin_(origin), rect_size_(size)
    {
    }

    template <typename T>
    T rect_<T>::left() const { return origin_.x(); }

    template <typename T>
    T rect_<T>::bottom() const { return top() + height(); }

    template <typename T>
    T rect_<T>::width() const { return rect_size_.width(); }

    template <typename T>
    T rect_<T>::height() const { return rect_size_.height(); }

    template <typename T>
    T rect_<T>::right() const { return left() + width(); }

    template <typename T>
    T rect_<T>::top() const { return origin_.y(); }

    template <typename T>
    rect_<T>& rect_<T>::operator+=(const rect_& rhs)
    {
        origin_ += rhs.origin_;
        rect_size_ += rhs.rect_size_;
        return *this;
    }

    template <typename T>
    rect_<T>& rect_<T>::operator-=(const rect_& rhs)
    {
        origin_ -= rhs.origin_;
        rect_size_ -= rhs.rect_size_;
        return *this;
    }

    template <typename T>
    rect_<T>& rect_<T>::operator+=(const T& i)
    {
        origin_ -= i;
        rect_size_ += (2 * i);
        return *this;
    }

    template <typename T>
    rect_<T>& rect_<T>::operator-=(const T& i)
    {
        return *this += (-i);
    }

    template <typename T>
    rect_<T>& rect_<T>::operator*=(const ::sc::size_<double>& rhs)
    {
        origin_ *= ::sc::point_<T>{(T)rhs.width(), (T)rhs.height()};
        rect_size_ *= ::sc::size_<T>{(T)rhs.width(), (T)rhs.height()};
        return *this;
    }

    template <typename T>
    rect_<T> rect_<T>::operator+(const rect_& r) const
    {
        auto tmp = *this;
        return tmp += r;
    }

    template <typename T>
    rect_<T> rect_<T>::operator+(const T& i) const
    {
        auto tmp = *this;
        return tmp += i;
    }

    template <typename T>
    rect_<T> rect_<T>::operator-(const rect_& r) const
    {
        auto tmp = *this;
        return tmp -= r;
    }

    template <typename T>
    rect_<T> rect_<T>::operator-(const T& i) const
    {
        return *this + (-i);
    }


    template <typename T>
    point_<T> rect_<T>::center() const
    {
        return origin_ + rect_size_ / 2;
    }

    template <typename T>
    size_<T> rect_<T>::size() const
    {
        return rect_size_;
    }

    template <typename T>
    bool rect_<T>::operator<(const rect_& r) const
    {
        return left() < r.left() || (left() == r.left() && top() < r.top());
    }

    template <typename T>
    bool rect_<T>::operator^(const rect_& r) const
    {
        return top() < r.top() || (top() == r.top() && left() < r.left());
    }

    template <typename T>
    T rect_<T>::area() const
    {
        return rect_size_.area();
    }

    template <typename T>
    double rect_<T>::iou(const rect_& rhs) const
    {
        const T interArea = intersect(rhs).area();
        if (interArea == T{}) return {};
        const T unionArea = area() + rhs.area() - interArea;
        if (unionArea == T{}) return {};
        return interArea / unionArea;
    }

    template <typename T>
    void rect_<T>::include(const rect_& rhs)
    {
        auto w = std::max(right(), rhs.right()) - std::min(left(), rhs.left());
        auto h = std::max(bottom(), rhs.bottom()) - std::min(top(), rhs.top());
        rect_size_ = {}; // {w, h};
        auto x = std::min(left(), rhs.left());
        auto y = std::min(top(), rhs.top());
        origin_ = {}; // {x, y};
    }

    template <typename T>
    T rect_<T>::distance(const T cx, const T cy) const
    {
        const T dx = std::max({left() - cx, static_cast<T>(0), cx - right()});
        const T dy = std::max({top() - cy, static_cast<T>(0), cy - bottom()});
        if (dx == 0 && dy == 0) return 0;
        return std::hypot(dx, dy);
    }

    template <typename T>
    T rect_<T>::distance(const rect_& rhs) const
    {
        const T dx = gap_x(rhs);
        const T dy = gap_y(rhs);
        if (dx == 0 && dy == 0) return 0;
        return std::hypot(dx, dy);
    }

    template <typename T>
    T rect_<T>::gap_x(const rect_& rhs) const
    {
        return std::max({left() - rhs.right(), rhs.left() - right(), static_cast<T>(0)});
    }

    template <typename T>
    T rect_<T>::gap_y(const rect_& rhs) const
    {
        return std::max({top() - rhs.bottom(), rhs.top() - bottom(), static_cast<T>(0)});
    }

    template <typename T>
    bool rect_<T>::overlaps_x(const rect_& rhs) const
    {
        return gap_x(rhs) == T{};
    }

    template <typename T>
    bool rect_<T>::overlaps_y(const rect_& rhs) const
    {
        return gap_y(rhs) == T{};
    }

    template <typename T>
    rect_<T> rect_<T>::intersect(const rect_& rhs) const
    {
        const T x0 = std::max(left(), rhs.left());
        const T y0 = std::max(top(), rhs.top());
        return {x0, y0, std::min(right(), rhs.right()) - x0, std::min(bottom(), rhs.bottom()) - y0};
    }

    template <typename T>
    rect_<T> rect_<T>::from_points(const T left, const T top, const T right, const T bottom)
    {
        return {left, top, right - left, bottom - top};
    }

    template <typename T>
    std::vector<std::pair<rect_<T>, std::vector<size_t>>> rect_<T>::group(
        const std::vector<rect_>& boxes, const T min_iou, const T max_dist)
    {
        if (boxes.empty()) return {};
        return components(boxes, [min_iou, max_dist](const rect_& a, const rect_& b)
        {
            if (min_iou < 1 && a.iou(b) > min_iou) return true; // min_iou == 1 disables
            if (max_dist >= 0 && a.distance(b) < max_dist) return true; // max_dist < 0 disables
            return false;
        });
    }

    template <typename T>
    std::vector<std::pair<rect_<T>, std::vector<size_t>>> rect_<T>::group_adjacent(
        const std::vector<rect_>& boxes, const T max_dx, const T max_dy)
    {
        if (boxes.empty()) return {};
        return components(boxes, [max_dx, max_dy](const rect_& a, const rect_& b)
        {
            return a.gap_x(b) < max_dx && a.gap_y(b) < max_dy;
        });
    }
}

template class sc::pair_<sc::point_<int>, int>;
template class sc::pair_<sc::size_<int>, int>;
template class sc::rect_<int>;

template class sc::pair_<sc::point_<double>, double>;
template class sc::pair_<sc::size_<double>, double>;
template class sc::rect_<double>;
