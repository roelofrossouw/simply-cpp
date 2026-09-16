#include <sc.h>

#include <string>

// Declared before sc_test.h so the harness can print a colour when a check fails.
namespace sc {
    inline std::ostream &operator<<(std::ostream &lhs, const color &rhs) {
        return lhs << "cmyk(" << rhs.cyan() << "," << rhs.magenta() << "," << rhs.yellow() << "," << rhs.black() << ")"
                   << " rgba(" << rhs.red() << "," << rhs.green() << "," << rhs.blue() << "," << rhs.alpha() << ")";
    }
}

#include "sc_test.h"

using namespace std;

namespace {
    constexpr double tolerance = 1e-9;

    void check_rgba(const sc::color &value, const double red, const double green, const double blue, const double alpha = 1) {
        CHECK_NEAR(value.red(), red, tolerance);
        CHECK_NEAR(value.green(), green, tolerance);
        CHECK_NEAR(value.blue(), blue, tolerance);
        CHECK_NEAR(value.alpha(), alpha, tolerance);
    }

    void check_cmyk(const sc::color &value, const double cyan, const double magenta, const double yellow, const double black) {
        CHECK_NEAR(value.cyan(), cyan, tolerance);
        CHECK_NEAR(value.magenta(), magenta, tolerance);
        CHECK_NEAR(value.yellow(), yellow, tolerance);
        CHECK_NEAR(value.black(), black, tolerance);
    }
}

int main() {
    SECTION("Named colours, case insensitive");
    {
        const sc::color dark_red{"DarkRed"};
        check_rgba(dark_red, 139 / 255.0, 0, 0, 1);
        CHECK_EQ(dark_red, sc::color::from_string("darkred"));
        CHECK_EQ(sc::color{"BLUE"}, sc::color{"blue"});
        // CSS "green" is half intensity, which is not the same as the Green constant.
        check_rgba(sc::color::from_string("green"), 0, 128 / 255.0, 0);
        CHECK_NE(sc::color::from_string("green"), sc::color::Green);
        CHECK_EQ(sc::color::from_string("lime"), sc::color::Green);
    }

    SECTION("Transparent and unknown names");
    {
        const auto none = sc::color::from_string("None");
        check_rgba(none, 0, 0, 0, 0);
        CHECK_EQ(none, sc::color::Transparent);
        CHECK_EQ(sc::color::from_string("transparent"), sc::color::Transparent);
        // An unrecognised name falls back to opaque black rather than throwing.
        const auto unknown = sc::color::from_string("definitely-not-a-colour");
        check_rgba(unknown, 0, 0, 0, 1);
        CHECK_EQ(unknown, sc::color::Black);
        CHECK_EQ(sc::color::from_string(""), sc::color::Black);
        CHECK_NE(sc::color::Black, sc::color::Transparent); // they differ only in alpha
    }

    SECTION("Hex notation");
    {
        // #RGB shorthand expands each digit, so C becomes CC.
        check_rgba(sc::color::from_string("#C00"), 0xCC / 255.0, 0, 0, 1);
        check_rgba(sc::color::from_string("#c00000"), 0xC0 / 255.0, 0, 0, 1);
        CHECK_EQ(sc::color::from_string("#FFF"), sc::color::White);
        CHECK_EQ(sc::color::from_hex("#ffffff"), sc::color::White);
        CHECK_EQ(sc::color::from_hex("#000000"), sc::color::Black);
        CHECK_EQ(sc::color::from_hex("#ff0000"), sc::color::Red);
        CHECK_EQ(sc::color::from_hex("#0000ff"), sc::color::Blue);
    }

    SECTION("Functional notation");
    {
        check_rgba(sc::color::from_string("rgb(255,0,200)"), 1, 0, 200 / 255.0, 1);
        check_rgba(sc::color::from_string("rgba(255,0,200, 0.2)"), 1, 0, 200 / 255.0, 0.2);
        check_rgba(sc::color::from_string("hsl(120,100%,50%)"), 0, 1, 0, 1);
        CHECK_EQ(sc::color::from_string("hsl(120,100%,50%)"), sc::color::Green);
        check_rgba(sc::color::from_string("hsl(0,100%,50%)"), 1, 0, 0, 1);
        check_rgba(sc::color::from_string("hsl(0,0%,0%)"), 0, 0, 0, 1);   // achromatic black
        check_rgba(sc::color::from_string("hsl(0,0%,100%)"), 1, 1, 1, 1); // achromatic white
        check_rgba(sc::color::from_string("hsla(240,100%,50%,0.5)"), 0, 0, 1, 0.5);
    }

    SECTION("from_web matches the parsed form");
    {
        const auto parsed = sc::color::from_string("rgb(255,0,200)");
        const auto built = sc::color::from_web(255, 0, 200);
        CHECK_EQ(parsed, built);
        check_rgba(built, 1, 0, 200 / 255.0, 1);
        CHECK_EQ(sc::color::from_web(0, 0, 0, 0), sc::color::Transparent);
    }

    SECTION("CMYK conversion");
    {
        check_cmyk(sc::color::Red, 0, 1, 1, 0);
        check_cmyk(sc::color::Green, 1, 0, 1, 0);
        check_cmyk(sc::color::Blue, 1, 1, 0, 0);
        check_cmyk(sc::color::White, 0, 0, 0, 0);
        // Pure black is the special case where no hue information survives.
        check_cmyk(sc::color::Black, 0, 0, 0, 1);
        check_rgba(sc::color::Black, 0, 0, 0, 1);

        // Round trip rgb -> cmyk -> rgb.
        const auto original = sc::color::from_web(139, 0, 200);
        const auto rebuilt = sc::color::from_cmyk(original.cyan(), original.magenta(), original.yellow(),
                                                  original.black(), original.alpha());
        check_rgba(rebuilt, 139 / 255.0, 0, 200 / 255.0, 1);
        CHECK_EQ(original, rebuilt);
    }

    SECTION("Equality operators");
    {
        const sc::color from_name{"blue"};
        const sc::color from_components{0, 0, 1};
        CHECK_EQ(from_name, from_components);
        CHECK(!(from_name != from_components));
        CHECK_NE(sc::color{"blue"}, sc::color{"red"});
        // Alpha participates in equality.
        CHECK_NE(sc::color(0, 0, 1, 1), sc::color(0, 0, 1, 0.5));
    }

    TEST_SUMMARY();
}
