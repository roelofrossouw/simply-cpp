#pragma once

#include "point.h"
#include "size.h"
#include "rect.h"
#include "rotated_rect.h"
#include "rounded_rect.h"
#include "path.h"
#include "polygon.h"
#include "circle.h"
#include "line.h"
#include "dbscan.h"

#include <concepts>
#include <cstddef>
#include <initializer_list>
#include <utility>
#include <variant>
#include <vector>

namespace sc {
    /// A shape: something with an area it covers, bounds(), and an outline, to_path(shape) (an
    /// sc::path of lines and curves, exactly the shape). rect, rounded_rect, rotated_rect, polygon,
    /// circle, line and path are, in both flavours, and so is sc::shape; point and size aren't (they're
    /// coordinates). sc-graphics' canvas draws any of them: draw(shape, appearance).
    ///
    /// A shape of your own is one too with a bounds() member and a to_path(const your_shape &)
    /// function next to it (found by argument-dependent lookup).
    template<typename G>
    concept geometry = requires(const G &shape) {
        { shape.bounds() } -> std::convertible_to<rect>;
        { to_path(shape) } -> std::same_as<path>;
    };

    class shape;
    class transform;

    /// A collection of shapes that is one shape itself: its bounds() cover them all, its path is
    /// theirs one after the other, and a transform or flip moves them as a whole (each shape mirrored
    /// in place and moved to the other side). A group can hold groups, as SVG's <g> does.
    ///
    ///     sc::group page_marks{sc::rect{50, 50, 100, 40}, sc::circle{{300, 80}, 20}};
    ///     page_marks.flip_horizontal(sc::rect{0, 0, 595, 842});   // mirrored across the A4 page
    ///     canvas.draw(page_marks, sc::color::Red);
    class group {
    public:
        using value_type = shape;
        using const_iterator = std::vector<shape>::const_iterator;

        group() = default;

        group(std::initializer_list<shape> items);

        explicit group(std::vector<shape> items);

        void push_back(shape item);

        // Defined below sc::shape, which they need complete.
        [[nodiscard]] const std::vector<shape> &items() const { return items_; }
        [[nodiscard]] std::size_t size() const;
        [[nodiscard]] bool empty() const;
        [[nodiscard]] const shape &operator[](std::size_t index) const;
        [[nodiscard]] const_iterator begin() const;
        [[nodiscard]] const_iterator end() const;

        /// The area its shapes cover together; an empty rect for an empty group.
        [[nodiscard]] rect bounds() const;

        /// Every shape through the transform, which keeps each shape's kind where it can (see
        /// sc::transform::apply()).
        group &apply(const transform &change);

        /// Mirrored left to right about the vertical line through the middle of its own bounds, so
        /// it stays where it is; or through the middle of within (a page, say), so it lands on the
        /// other side of it.
        group &flip_horizontal();
        group &flip_horizontal(const rect &within);

        /// Mirrored top to bottom, the same way.
        group &flip_vertical();
        group &flip_vertical(const rect &within);

        bool operator==(const group &) const;

    private:
        std::vector<shape> items_;
    };

    /// Any one of sc-core's shapes, in doubles, or a group of them: for collections of different
    /// shapes (a list of annotations: boxes, circles, lines). An int shape converts on the way in
    /// (sc::rect{box_i}).
    ///
    ///     std::vector<sc::shape> marks{sc::rect{10, 10, 50, 20}, sc::circle{{40, 40}, 5}, sc::line{{0, 0}, {9, 9}}};
    ///     for (const auto &mark: marks) canvas.draw(mark, sc::color::Red);
    class shape {
    public:
        using value_type = std::variant<rect, rounded_rect, rotated_rect, polygon, circle, line, path, group>;

        template<typename G> requires std::constructible_from<value_type, G>
        shape(G geometry) : value_(std::move(geometry)) {
        }

        /// The shape it holds, for std::visit or std::get.
        [[nodiscard]] const value_type &value() const { return value_; }

        [[nodiscard]] rect bounds() const {
            return std::visit([](const auto &held) { return rect{held.bounds()}; }, value_);
        }

        bool operator==(const shape &) const = default;

    private:
        value_type value_;
    };

    inline group::group(std::initializer_list<shape> items) : items_(items) {
    }

    inline group::group(std::vector<shape> items) : items_(std::move(items)) {
    }

    inline void group::push_back(shape item) { items_.push_back(std::move(item)); }

    inline std::size_t group::size() const { return items_.size(); }

    inline bool group::empty() const { return items_.empty(); }

    inline const shape &group::operator[](const std::size_t index) const { return items_[index]; }

    inline group::const_iterator group::begin() const { return items_.begin(); }

    inline group::const_iterator group::end() const { return items_.end(); }

    inline rect group::bounds() const {
        if (items_.empty()) return {};
        rect covered = items_.front().bounds();
        for (const auto &item: items_) covered.include(item.bounds());
        return covered;
    }

    inline bool group::operator==(const group &other) const { return items_ == other.items_; }

    inline path to_path(const shape &held) {
        return std::visit([](const auto &each) { return to_path(each); }, held.value());
    }

    /// Its shapes' paths, one after the other.
    inline path to_path(const group &shapes) {
        path joined;
        for (const auto &item: shapes) joined.append(to_path(item));
        return joined;
    }

    static_assert(geometry<rect> && geometry<rect_i> && geometry<rounded_rect> && geometry<rotated_rect> && geometry<polygon> &&
                  geometry<polygon_i> && geometry<circle> && geometry<line> && geometry<path> && geometry<shape> && geometry<group>);
    static_assert(!geometry<point> && !geometry<size>);
}
