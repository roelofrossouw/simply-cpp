#ifndef SC_DISTANCE_H
#define SC_DISTANCE_H

#include <ostream>

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
    /// points(), millimetres(), centimetres() and inches() read it back, and << writes it with its
    /// unit: a distance in points ("595pt"), each unit in its own ("209.903mm"). A distance becomes
    /// another unit with static_cast, keeping its length: static_cast<sc::dist::mm>(width). Copying
    /// a unit into a distance keeps it whole: the value is always in points. Where a canvas's unit isn't points (an sc::image's is
    /// pixels), a distance counts as that many pixels: 72 to the inch.
    class distance {
    public:
        static constexpr double points_per_inch = 72;
        static constexpr double mm_per_inch = 25.4;

        /// A number of points.
        constexpr distance(const double points = 0) : points_(points) {}

        [[nodiscard]] constexpr double points() const { return points_; }

        [[nodiscard]] constexpr double millimetres() const { return points_ / points_per_inch * mm_per_inch; }

        [[nodiscard]] constexpr double centimetres() const { return millimetres() / 10; }

        [[nodiscard]] constexpr double inches() const { return points_ / points_per_inch; }

        /// Its number of points.
        constexpr operator double() const { return points_; }

    private:
        double points_;
    };

    /// In points: "595pt".
    inline std::ostream &operator<<(std::ostream &out, const distance &length) { return out << length.points() << "pt"; }

    /// The units a distance can be given in, each a distance converting its own unit to points. In
    /// a namespace of their own so they can have their plain names (dist::point isn't sc::point, the
    /// 2D point) and more can join without crowding sc.
    namespace dist {
        /// Points, PDF's unit: the same as a plain sc::distance, named for clarity.
        class point : public distance {
        public:
            constexpr point() = default;

            constexpr explicit point(const double points) : distance(points) {}

            /// The same length, in points: static_cast<sc::dist::point>(width).
            constexpr explicit point(const distance &length) : distance(length) {}
        };

        inline std::ostream &operator<<(std::ostream &out, const point &length) { return out << length.points() << "pt"; }

        /// Millimetres.
        class mm : public distance {
        public:
            constexpr mm() = default;

            constexpr explicit mm(const double millimetres) : distance(millimetres / mm_per_inch * points_per_inch) {}

            /// The same length, in millimetres: static_cast<sc::dist::mm>(width).
            constexpr explicit mm(const distance &length) : distance(length) {}
        };

        inline std::ostream &operator<<(std::ostream &out, const mm &length) { return out << length.millimetres() << "mm"; }

        /// Centimetres.
        class cm : public distance {
        public:
            constexpr cm() = default;

            constexpr explicit cm(const double centimetres) : distance(centimetres * 10 / mm_per_inch * points_per_inch) {}

            /// The same length, in centimetres: static_cast<sc::dist::cm>(width).
            constexpr explicit cm(const distance &length) : distance(length) {}
        };

        inline std::ostream &operator<<(std::ostream &out, const cm &length) { return out << length.centimetres() << "cm"; }

        /// Inches.
        class inch : public distance {
        public:
            constexpr inch() = default;

            constexpr explicit inch(const double inches) : distance(inches * points_per_inch) {}

            /// The same length, in inches: static_cast<sc::dist::inch>(width).
            constexpr explicit inch(const distance &length) : distance(length) {}
        };

        inline std::ostream &operator<<(std::ostream &out, const inch &length) { return out << length.inches() << "in"; }
    }
}

#endif //SC_DISTANCE_H
