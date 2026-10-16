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
#include <utility>
#include <variant>

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

    /// Any one of sc-core's shapes, in doubles: for collections of different shapes (a list of
    /// annotations: boxes, circles, lines). An int shape converts on the way in (sc::rect{box_i}).
    ///
    ///     std::vector<sc::shape> marks{sc::rect{10, 10, 50, 20}, sc::circle{{40, 40}, 5}, sc::line{{0, 0}, {9, 9}}};
    ///     for (const auto &mark: marks) canvas.draw(mark, sc::color::Red);
    class shape {
    public:
        using value_type = std::variant<rect, rounded_rect, rotated_rect, polygon, circle, line, path>;

        template<typename G> requires std::constructible_from<value_type, G>
        shape(G geometry) : value_(std::move(geometry)) {
        }

        /// The shape it holds, for std::visit or std::get.
        [[nodiscard]] const value_type &value() const { return value_; }

        [[nodiscard]] rect bounds() const {
            return std::visit([](const auto &held) { return rect{held.bounds()}; }, value_);
        }

    private:
        value_type value_;
    };

    inline path to_path(const shape &held) {
        return std::visit([](const auto &each) { return to_path(each); }, held.value());
    }

    static_assert(geometry<rect> && geometry<rect_i> && geometry<rounded_rect> && geometry<rotated_rect> && geometry<polygon> &&
                  geometry<polygon_i> && geometry<circle> && geometry<line> && geometry<path> && geometry<shape>);
    static_assert(!geometry<point> && !geometry<size>);
}
