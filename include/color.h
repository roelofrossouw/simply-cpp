#ifndef SC_COLOR_H
#define SC_COLOR_H

namespace sc {
    class color {
        double c, m, y, k;
        double r, g, b, a;

    public:
        color(double red, double green, double blue, double alpha = 1);

        static color from_cmyk(double cyan, double magenta, double yellow, double black, double alpha);

        static color from_web(int red, int green, int blue, double alpha = 1);

        double cyan() const;

        double yellow() const;

        double magenta() const;

        double black() const;

        double red() const;

        double green() const;

        double blue() const;

        double alpha() const;

        void flatten(const color &background);

        // Standard color constants
        static const color Red;
        static const color Green;
        static const color Blue;
        static const color Black;
        static const color White;
        static const color Transparent;
    };
} // sc

#endif //SC_COLOR_H
