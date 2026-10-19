#ifndef SC_DISTANCE_H
#define SC_DISTANCE_H

namespace sc {
    /// A distance, kept in points (1/72 inch), the unit PDFs are drawn in. The units in sc::dist
    /// (dist::mm, dist::cm, dist::inch, dist::point) are distances given in their own unit, and a
    /// plain number is points. Every distance converts to its number of points as a double (or an
    /// int, as a double does), so geometry and drawing calls take them as they are, mixed with
    /// plain numbers:
    ///
    ///     sc::rect box{sc::dist::mm(10), sc::dist::inch(1.1), sc::dist::point(72), 72}; // 28.35, 79.2, 72, 72
    ///     double margin = sc::dist::mm(15) + sc::dist::inch(0.5);                      // 78.52 points
    ///
    /// points(), millimetres() and inches() read it back. Copying a unit into a distance keeps it
    /// whole: the value is always in points. Where a canvas's unit isn't points (an sc::image's is
    /// pixels), a distance counts as that many pixels: 72 to the inch.
    class distance {
    public:
        static constexpr double points_per_inch = 72;
        static constexpr double mm_per_inch = 25.4;

        /// A number of points.
        constexpr distance(const double points = 0) : points_(points) {}

        [[nodiscard]] constexpr double points() const { return points_; }

        [[nodiscard]] constexpr double millimetres() const { return points_ / points_per_inch * mm_per_inch; }

        [[nodiscard]] constexpr double inches() const { return points_ / points_per_inch; }

        /// Its number of points.
        constexpr operator double() const { return points_; }

    private:
        double points_;
    };

    /// The units a distance can be given in, each a distance converting its own unit to points. In
    /// a namespace of their own so they can have their plain names (dist::point isn't sc::point, the
    /// 2D point) and more can join without crowding sc.
    namespace dist {
        /// Points, PDF's unit: the same as a plain sc::distance, named for clarity.
        class point : public distance {
        public:
            constexpr explicit point(const double points) : distance(points) {}
        };

        /// Millimetres.
        class mm : public distance {
        public:
            constexpr explicit mm(const double millimetres) : distance(millimetres / mm_per_inch * points_per_inch) {}
        };

        /// Centimetres.
        class cm : public distance {
        public:
            constexpr explicit cm(const double centimetres) : distance(centimetres * 10 / mm_per_inch * points_per_inch) {}
        };

        /// Inches.
        class inch : public distance {
        public:
            constexpr explicit inch(const double inches) : distance(inches * points_per_inch) {}
        };
    }
}

#endif //SC_DISTANCE_H
