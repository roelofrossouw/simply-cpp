#pragma once

#include "circle.h"
#include "color.h"
#include "point.h"
#include "polygon.h"
#include "rect.h"
#include "rotated_rect.h"
#include "text.h"

#include <vector>

namespace sc {
    /// Something to draw on, such as an sc::image or an sc::pdf, so the same calls draw on either.
    ///
    /// Every shape is drawn with draw(shape, stroke, fill, width): its outline in stroke, width wide,
    /// and its inside in fill. A transparent colour (alpha 0, such as color::Transparent, the fill's
    /// default) leaves that part out. Coordinates are the canvas's own (pixels on an image, points on
    /// a PDF), from the top-left corner with y down.
    ///
    /// A canvas implements a few primitives (polygon, circle, line, text and, if it can do better
    /// than a polygon, rounded rectangles); the draw() calls here turn everything else into those.
    class canvas {
    public:
        virtual ~canvas() = default;

        /// A rectangle, its corners rounded with radius when that's above 0 (at most half the
        /// shorter side).
        void draw(const rect &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1, double radius = 0);

        void draw(const rect_i &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1, double radius = 0);

        /// A rectangle turned angle degrees about its centre, clockwise (y points down).
        void draw(const rotated_rect &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1);

        /// A closed polygon through the points; fewer than two draw nothing.
        void draw(const polygon &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1);

        void draw(const polygon_i &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1);

        /// A circle; a radius of 0 draws nothing.
        void draw(const circle &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1);

        void draw(const circle_i &shape, const color &stroke = color::Black, const color &fill = color::Transparent,
                  double width = 1);

        /// A line of text, its baseline starting at sample.position, in colour.
        void draw(const text &sample, const color &colour = color::Black);

        /// Each of shapes, the same way.
        template<typename Geometry>
        void draw(const std::vector<Geometry> &shapes, const color &stroke = color::Black,
                  const color &fill = color::Transparent, const double width = 1) {
            for (const auto &shape: shapes) draw(shape, stroke, fill, width);
        }

        /// A straight line.
        void draw_line(const point &from, const point &to, const color &colour = color::Black, double width = 1);

    protected:
        // The primitives. draw() has already left out a transparent stroke or fill (passing it as
        // color::Transparent, with a width of 0 for no stroke) and skipped what draws nothing.
        virtual void paint_polygon(const polygon &shape, const color &stroke, const color &fill, double width) = 0;

        virtual void paint_circle(const circle &shape, const color &stroke, const color &fill, double width) = 0;

        virtual void paint_line(const point &from, const point &to, const color &colour, double width) = 0;

        virtual void paint_text(const text &sample, const color &colour) = 0;

        /// radius is above 0 and at most half the shorter side. By default the corners are arcs of
        /// short straight segments; a canvas with curves can do better.
        virtual void paint_rounded_rect(const rect &shape, double radius, const color &stroke, const color &fill, double width);
    };
}
