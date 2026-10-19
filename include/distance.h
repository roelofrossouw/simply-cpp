#ifndef SC_DISTANCE_H
#define SC_DISTANCE_H

namespace sc {
    /// A distance, kept in points (1/72 inch), the unit PDFs are drawn in. sc::mm and sc::inch are
    /// distances given in their own unit; a plain number is points. Every distance converts to its
    /// number of points as a double (or an int, as a double does), so geometry and drawing calls
    /// take them as they are, mixed with plain numbers:
    ///
    ///     sc::rect box{sc::mm(10), sc::inch(1.1), sc::distance(72), 72};   // 28.35, 79.2, 72, 72
    ///     double margin = sc::mm(15) + sc::inch(0.5);                      // 78.52 points
    ///
    /// points(), millimetres() and inches() read it back. Copying an mm or an inch into a distance
    /// keeps it whole: the value is always in points.
    /// Where a canvas's unit isn't points (an sc::image's is pixels), a distance counts as that
    /// many pixels: 72 to the inch.
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

    /// A distance in millimetres.
    class mm : public distance {
    public:
        constexpr explicit mm(const double millimetres) : distance(millimetres / mm_per_inch * points_per_inch) {}
    };

    /// A distance in inches.
    class inch : public distance {
    public:
        constexpr explicit inch(const double inches) : distance(inches * points_per_inch) {}
    };
}

#endif //SC_DISTANCE_H
