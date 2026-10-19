#pragma once

#include "geometry.h"

namespace sc {
    /// A 2D affine transform: moving, scaling, rotating, mirroring and any mix of them, as SVG's
    /// matrix(a b c d e f): x' = a x + c y + e, y' = b x + d y + f. Coordinates are y down, so a
    /// positive rotation turns clockwise on screen, as sc::rotated_rect's angle does.
    ///
    ///     const auto mirror = sc::transform::flip_horizontal(595.0 / 2);       // across an A4 page
    ///     const auto turn = sc::transform::rotate(30, {100, 100}).then(sc::transform::translate(10, 0));
    ///     const sc::shape moved = turn.apply(sc::rect{50, 50, 100, 40});      // a rotated_rect now
    class transform {
    public:
        /// The identity.
        transform() = default;

        transform(double a, double b, double c, double d, double e, double f);

        static transform translate(double dx, double dy);
        static transform translate(const point &by);

        /// About the point about (the origin by default); one factor scales both axes.
        static transform scale(double sx, double sy, const point &about = {});
        static transform scale(double factor, const point &about = {});

        /// Degrees clockwise on screen, about the point about.
        static transform rotate(double degrees, const point &about = {});

        /// Mirrored about the line x = axis_x, or y = axis_y.
        static transform flip_horizontal(double axis_x = 0);
        static transform flip_vertical(double axis_y = 0);

        /// This transform, then next.
        [[nodiscard]] transform then(const transform &next) const;

        /// The transform that undoes this one; throws std::domain_error when there's none (it squashes
        /// everything onto a line or a point).
        [[nodiscard]] transform inverse() const;

        [[nodiscard]] double a() const { return a_; }
        [[nodiscard]] double b() const { return b_; }
        [[nodiscard]] double c() const { return c_; }
        [[nodiscard]] double d() const { return d_; }
        [[nodiscard]] double e() const { return e_; }
        [[nodiscard]] double f() const { return f_; }

        /// How much it scales lengths: the square root of its area scale (2 for scale(2)); for
        /// stroke widths.
        [[nodiscard]] double scale_factor() const;

        /// Whether it keeps angles and circles: moves, rotations, mirrors and even scales only.
        [[nodiscard]] bool conformal() const;

        /// Whether upright rectangles stay upright: quarter turns at most, no shear.
        [[nodiscard]] bool keeps_axes() const;

        [[nodiscard]] point apply(const point &at) const;

        /// Lines, polygons and paths keep their kind under any transform (a path's curves exactly).
        [[nodiscard]] line apply(const line &shape) const;
        [[nodiscard]] polygon apply(const polygon &shape) const;
        [[nodiscard]] path apply(const path &shape) const;

        /// The others keep their kind when the transform allows it, else become the closest exact one:
        ///   - rect: a rect when it keeps_axes(), a rotated_rect when conformal(), else a polygon;
        ///   - rounded_rect: a rounded_rect when both, else a path;
        ///   - rotated_rect: a rotated_rect when conformal(), else a polygon;
        ///   - circle: a circle when conformal(), else a path (an ellipse, or a slanted one).
        [[nodiscard]] shape apply(const rect &shape) const;
        [[nodiscard]] shape apply(const rounded_rect &shape) const;
        [[nodiscard]] shape apply(const rotated_rect &shape) const;
        [[nodiscard]] shape apply(const circle &shape) const;

        /// Each shape as above; a group stays a group of them.
        [[nodiscard]] shape apply(const shape &shape) const;
        [[nodiscard]] group apply(const group &shapes) const;

        /// Int shapes, in doubles.
        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const rect_<T> &shape) const { return apply(rect{shape}); }

        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const rounded_rect_<T> &shape) const { return apply(rounded_rect{shape}); }

        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const polygon_<T> &shape) const { return apply(polygon{shape}); }

        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const circle_<T> &shape) const { return apply(circle{shape}); }

        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const line_<T> &shape) const { return apply(line{shape}); }

        template<Numeric T> requires std::integral<T>
        [[nodiscard]] auto apply(const point_<T> &at) const { return apply(point{static_cast<double>(at.x()), static_cast<double>(at.y())}); }

        /// Any other sc::geometry (a shape of your own), as its path.
        template<geometry G>
        [[nodiscard]] path apply(const G &shape) const { return apply(to_path(shape)); }

        bool operator==(const transform &) const = default;

    private:
        double a_ = 1, b_ = 0, c_ = 0, d_ = 1, e_ = 0, f_ = 0;
    };
}
