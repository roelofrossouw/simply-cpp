#ifndef SC_COLOR_H
#define SC_COLOR_H
#include <ostream>
#include <string>

namespace sc {
    /// A colour with alpha, its channels from 0 to 1 (alpha 1 is opaque), readable as RGB or CMYK.
    class color {
        double c, m, y, k;
        double r, g, b, a;

    public:
        color() = default;

        color(double red, double green, double blue, double alpha = 1);

        /// CSS text, as from_string() reads it.
        color(const std::string &color_name);

        static color from_cmyk(double cyan, double magenta, double yellow, double black, double alpha);

        /// Channels from 0 to 255, as on the web.
        static color from_web(int red, int green, int blue, double alpha = 1);

        /// "#rrggbb" or "#rgb"; anything else is black.
        static color from_hex(const std::string &hex);

        /// Hue in degrees, saturation and lightness in percent.
        static color from_hsl(double x, double s, double l, double a);

        /// CSS: #rgb, #rrggbb, rgb(), rgba(), hsl(), hsla(), a CSS colour name (in any case),
        /// "transparent" or "none". Anything else is black.
        static color from_string(const std::string &str);

        double cyan() const;

        double yellow() const;

        double magenta() const;

        double black() const;

        double red() const;

        double green() const;

        double blue() const;

        double alpha() const;

        /// Lays this colour over background, as if painted on it: a half-transparent red on white
        /// becomes an opaque pink. Opaque over anything stays as it is.
        void flatten(const color &background);

        /// "#rrggbb", leaving out alpha; from_hex() reads it back.
        std::string to_hex() const;

        bool operator!=(const color & c2) const;

        bool operator==(const color & c2) const;

        static const color Red;
        static const color Green;
        static const color Blue;
        static const color Black;
        static const color White;
        static const color Transparent;

    private:
        /// CSS: rgb(255, 128, 0), or rgba(255, 128, 0, 0.5) when not opaque; from_string() reads it back.
        friend std::ostream &operator<<(std::ostream &lhs, const color &rhs);
    };
}

#endif //SC_COLOR_H
