#include "color.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <sstream>
#include <regex>
#include <unordered_map>

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

    color::color(const std::string &color_name) {
        this->operator=(from_string(color_name));
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

    color color::from_hex(const std::string &hex) {
        double r = 0, g = 0, b = 0;
        if (hex.size() == 7) {
            // #RRGGBB
            r = std::stoi(hex.substr(1, 2), nullptr, 16) / 255.0;
            g = std::stoi(hex.substr(3, 2), nullptr, 16) / 255.0;
            b = std::stoi(hex.substr(5, 2), nullptr, 16) / 255.0;
        } else if (hex.size() == 4) {
            // #RGB shorthand
            r = std::stoi(std::string(2, hex[1]), nullptr, 16) / 255.0;
            g = std::stoi(std::string(2, hex[2]), nullptr, 16) / 255.0;
            b = std::stoi(std::string(2, hex[3]), nullptr, 16) / 255.0;
        }
        return {r, g, b, 1};
    }

    color color::from_hsl(double h, double s, double l, double a) {
        h /= 360.0; // std::fmod(h, 360.0) / 360.0;
        s /= 100.0;
        l /= 100.0;

        auto hue2rgb = [](double p, double q, double t) {
            if (t < 0) t += 1;
            if (t > 1) t -= 1;
            if (t < 1.0 / 6) return p + (q - p) * 6 * t;
            if (t < 1.0 / 2) return q;
            if (t < 2.0 / 3) return p + (q - p) * (2.0 / 3 - t) * 6;
            return p;
        };

        double r, g, b;
        if (s == 0) {
            r = g = b = l; // achromatic
        } else {
            double q = l < 0.5 ? l * (1 + s) : l + s - l * s;
            double p = 2 * l - q;
            r = hue2rgb(p, q, h + 1.0 / 3);
            g = hue2rgb(p, q, h);
            b = hue2rgb(p, q, h - 1.0 / 3);
        }
        return {r, g, b, a};
    }

    color color::from_string(const std::string &str) {
        if (str.empty()) return {0, 0, 0, 1};

        if (str[0] == '#') {
            return from_hex(str);
        }

        std::smatch match;
        // rgb() or rgba()
        if (std::regex_match(str, match, std::regex(R"(rgba?\(([^)]+)\))"))) {
            std::stringstream ss(match[1].str());
            int r, g, b;
            double a = 1.0;
            char comma;
            ss >> r >> comma >> g >> comma >> b;
            if (ss >> comma >> a) {
            } // optional alpha
            return from_web(r, g, b, a);
        }

        // hsl() or hsla()
        if (std::regex_match(str, match, std::regex(R"(hsla?\(([^)]+)\))"))) {
            std::stringstream ss(match[1].str());
            double h, s, l, a = 1.0;
            char comma, percent;
            ss >> h >> comma >> s >> percent >> comma >> l >> percent;
            if (ss >> comma >> a) {
            }
            return from_hsl(h, s, l, a);
        }


        auto it = NamedColors.find(str);
        if (it != NamedColors.end()) {
            auto [red,green,blue] = it->second;
            return from_web(red, green, blue, 1);
        }

        if (str == "transparent" || str == "none") return {0, 0, 0, 0};

        std::string lstr = str;
        std::transform(lstr.begin(), lstr.end(), lstr.begin(), [](unsigned char c) { return std::tolower(c); });


        it = NamedColors.find(lstr);
        if (it != NamedColors.end()) {
            auto [red,green,blue] = it->second;
            return from_web(red, green, blue, 1);
        }
        if (lstr == "transparent" || lstr == "none") return {0, 0, 0, 0};

        return {0, 0, 0, 1}; // fallback: black
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
        return k == 1 ? 0 : 1 + c * k - k - c;
    }

    double color::green() const {
        return k == 1 ? 0 : 1 + m * k - k - m;
    }

    double color::blue() const {
        return k == 1 ? 0 : 1 + y * k - k - y;
    }

    double color::alpha() const {
        return a;
    }

    void color::flatten(const color &background) {
        // Porter-Duff "over": this colour laid on top of the background, by its alpha.
        const double alpha = a + background.a * (1 - a);
        if (alpha <= 0) {
            *this = Transparent;
            return;
        }
        const auto blend = [&](const double top, const double bottom) {
            return (top * a + bottom * background.a * (1 - a)) / alpha;
        };
        *this = color(blend(r, background.r), blend(g, background.g), blend(b, background.b), alpha);
    }

    namespace {
        // A 0..1 channel as 0..255.
        int channel_byte(const double value) {
            return static_cast<int>(std::lround(std::clamp(value, 0.0, 1.0) * 255));
        }
    }

    std::string color::to_hex() const {
        char text[8];
        std::snprintf(text, sizeof text, "#%02x%02x%02x", channel_byte(r), channel_byte(g), channel_byte(b));
        return text;
    }

    std::ostream &operator<<(std::ostream &lhs, const color &rhs) {
        const int red = channel_byte(rhs.r), green = channel_byte(rhs.g), blue = channel_byte(rhs.b);
        if (rhs.a >= 1) return lhs << "rgb(" << red << ", " << green << ", " << blue << ')';
        return lhs << "rgba(" << red << ", " << green << ", " << blue << ", " << rhs.a << ')';
    }

    bool color::operator!=(const color &c2) const {
        return !operator==(c2);
    }

    bool color::operator==(const color &c2) const {
        return c == c2.c && y == c2.y && m == c2.m && k == c2.k && a == c2.a;
    }

    const color color::Red = color(1.0, 0.0, 0.0);
    const color color::Green = color(0.0, 1.0, 0.0);
    const color color::Blue = color(0.0, 0.0, 1.0);
    const color color::Black = color(0.0, 0.0, 0.0);
    const color color::White = color(1.0, 1.0, 1.0);
    const color color::Transparent = color(0.0, 0.0, 0.0, 0.0);

    const std::unordered_map<std::string, std::tuple<int, int, int> > color::NamedColors = {
#include "named_colors.inc" // generated from the CSS Color 4 spec by cmake/NamedColors.cmake
    };
} // sc
