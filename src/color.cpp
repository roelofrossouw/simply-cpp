#include "color.h"
#include <algorithm>

namespace sc {
    color::color(double red, double green, double blue, double alpha) : r(red), g(green), b(blue), a(alpha) {
        k = 1 - std::max(r, std::max(g, b));
        if (k == 1) {
            c = 0;
            m = 0;
            y = 0;
        } else {
            c = (1 - r - k) / (1 - k);
            m = (1 - g - k) / (1 - k);
            y = (1 - b - k) / (1 - k);
        }
    }

    color color::from_cmyk(double cyan, double magenta, double yellow, double black, double alpha) {
        double red = (1.0 - cyan) * (1.0 - black);
        double green = (1.0 - magenta) * (1.0 - black);
        double blue = (1.0 - yellow) * (1.0 - black);
        return {red, green, blue, alpha};
    }

    color color::from_web(int red, int green, int blue, double alpha) {
        return {red / 255.0, green / 255.0, blue / 255.0, alpha};
    }

    double color::cyan() const {
        return c;
    }

    double color::yellow() const {
        return y;
    }

    double color::magenta() const {
        return m;
    }

    double color::black() const {
        return k;
    }

    double color::red() const {
        return 1 - c - k;
    }

    double color::green() const {
        return 1 - m - k;
    }

    double color::blue() const {
        return 1 - y - k;
    }

    double color::alpha() const {
        return a;
    }

    void color::flatten(const color &background) {
    }

    const color color::Red = color(1.0, 0.0, 0.0);
    const color color::Green = color(0.0, 1.0, 0.0);
    const color color::Blue = color(0.0, 0.0, 1.0);
    const color color::Black = color(0.0, 0.0, 0.0);
    const color color::White = color(1.0, 1.0, 1.0);
    const color color::Transparent = color(0.0, 0.0, 0.0, 0.0);
} // sc
