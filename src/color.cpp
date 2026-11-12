#include "color.h"
#include "core.h"
#include <algorithm>
#include <cmath>
#include <cstdio>
#include <charconv>
#include <numbers>
#include <cctype>
#include <vector>
#include <tuple>
#include <unordered_map>

namespace sc {
    namespace {
        // The CSS colour names, in a function-local static rather than a global: sc-core is linked
        // statically into several shared libraries and the programs using them, and a global with a
        // destructor is then merged into one object but destroyed by every copy (a double free at
        // exit).
        const std::unordered_map<std::string, std::tuple<int, int, int> > &named_colors() {
            static const std::unordered_map<std::string, std::tuple<int, int, int> > names = {
#include "named_colors.inc" // generated from the CSS Color 4 spec by cmake/NamedColors.cmake
            };
            return names;
        }

        // One argument of rgb() or hsl(): a number and its unit ("", "%", "deg", ...).
        struct argument {
            double number;
            std::string unit;
        };

        // The arguments of rgb()/hsl() in either CSS form: "255, 0, 0, 0.5" or "255 0 0 / 50%".
        // Something that isn't a number gives NAN, which the callers turn into black.
        std::vector<argument> arguments(const std::string &text) {
            std::vector<argument> out;
            std::size_t at = 0;
            while (at < text.size()) {
                if (std::isspace(static_cast<unsigned char>(text[at])) || text[at] == ',' || text[at] == '/') {
                    ++at;
                    continue;
                }
                // from_chars, not strtod: the same in every locale (PHP may set one with a decimal comma)
                if (text[at] == '+') ++at;
                double number = 0;
                const auto [end, error] = std::from_chars(text.data() + at, text.data() + text.size(), number);
                if (error != std::errc{}) return {{NAN, ""}};
                at = end - text.data();
                const auto unit_start = at;
                while (at < text.size() && (std::isalpha(static_cast<unsigned char>(text[at])) || text[at] == '%')) ++at;
                out.push_back({number, text.substr(unit_start, at - unit_start)});
            }
            return out;
        }

        double alpha_of(const argument &value) {
            const auto alpha = value.unit == "%" ? value.number / 100 : value.unit.empty() ? value.number : NAN;
            return std::isfinite(alpha) ? std::clamp(alpha, 0.0, 1.0) : NAN;
        }

        double degrees_of(const argument &value) {
            if (value.unit.empty() || value.unit == "deg") return value.number;
            if (value.unit == "rad") return value.number * 180 / std::numbers::pi;
            if (value.unit == "grad") return value.number * 0.9;
            if (value.unit == "turn") return value.number * 360;
            return NAN;
        }
    }

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
            r = std::stoi(hex.substr(1, 2), nullptr, 16) / 255.0;
            g = std::stoi(hex.substr(3, 2), nullptr, 16) / 255.0;
            b = std::stoi(hex.substr(5, 2), nullptr, 16) / 255.0;
        } else if (hex.size() == 4) {
            r = std::stoi(std::string(2, hex[1]), nullptr, 16) / 255.0;
            g = std::stoi(std::string(2, hex[2]), nullptr, 16) / 255.0;
            b = std::stoi(std::string(2, hex[3]), nullptr, 16) / 255.0;
        }
        return {r, g, b, 1};
    }

    color color::from_hsl(double h, double s, double l, double a) {
        h /= 360.0;
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
            r = g = b = l;
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

        if (str[0] == '#') return from_hex(str);

        const auto lower = lowercase(str);
        const auto open = lower.find('(');
        if (open != std::string::npos && lower.back() == ')') {
            const auto function = lower.substr(0, open);
            const auto values = arguments(lower.substr(open + 1, lower.size() - open - 2));
            if (function == "rgb" || function == "rgba") {
                if (values.size() != 3 && values.size() != 4) return {0, 0, 0, 1};
                double channel[3];
                for (int i = 0; i < 3; ++i) {
                    const auto value = values[i].unit == "%" ? values[i].number / 100 : values[i].unit.empty() ? values[i].number / 255 : NAN;
                    if (!std::isfinite(value)) return {0, 0, 0, 1};
                    channel[i] = std::clamp(value, 0.0, 1.0);
                }
                const auto alpha = values.size() == 4 ? alpha_of(values[3]) : 1.0;
                if (!std::isfinite(alpha)) return {0, 0, 0, 1};
                return {channel[0], channel[1], channel[2], alpha};
            }
            if (function == "hsl" || function == "hsla") {
                if (values.size() != 3 && values.size() != 4) return {0, 0, 0, 1};
                const auto hue = degrees_of(values[0]);
                // Saturation and lightness in percent; CSS Color 4 also takes plain numbers.
                const auto percent = [](const argument &value) {
                    return value.unit == "%" || value.unit.empty() ? std::clamp(value.number, 0.0, 100.0) : NAN;
                };
                const auto saturation = percent(values[1]), lightness = percent(values[2]);
                const auto alpha = values.size() == 4 ? alpha_of(values[3]) : 1.0;
                if (!std::isfinite(hue) || !std::isfinite(saturation) || !std::isfinite(lightness) || !std::isfinite(alpha))
                    return {0, 0, 0, 1};
                return from_hsl(std::fmod(std::fmod(hue, 360) + 360, 360), saturation, lightness, alpha);
            }
            return {0, 0, 0, 1};
        }

        const auto &name = lower;
        if (const auto it = named_colors().find(name); it != named_colors().end()) {
            const auto [red, green, blue] = it->second;
            return from_web(red, green, blue, 1);
        }
        if (name == "transparent" || name == "none") return {0, 0, 0, 0};

        return {0, 0, 0, 1};
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

    // The RGB channels as given, not worked back from CMYK: that round trip isn't exact, and
    // #990000's green and blue came back as -5.6e-17, below 0 (sc::pdflite refuses such a colour).
    double color::red() const {
        return r;
    }

    double color::green() const {
        return g;
    }

    double color::blue() const {
        return b;
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
}
