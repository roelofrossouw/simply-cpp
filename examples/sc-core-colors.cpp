// Colours in simply-cpp core: CSS colour names, hex, rgb()/rgba(), HSL and CMYK, compared and
// flattened onto a background. Each line shows a call, as written, and what it returned. Needs no
// server.

#include <string>
#include <vector>

#include <sc/core/sc.h>

int main() {
    sc::console::title("simply-cpp core: colours");

    sc::console::heading("Colours: sc::color");
    SC_SHOW(sc::color{"orange"});
    SC_SHOW(sc::color{"#36c"}.to_hex());
    SC_SHOW(sc::color{"rgba(255, 0, 0, 0.5)"});
    SC_SHOW(sc::color::from_hsl(120, 100, 25, 1));
    SC_SHOW(sc::color::from_cmyk(0, 0.5, 1, 0, 1));
    const sc::color teal{"teal"};
    SC_SHOW(std::vector{teal.cyan(), teal.magenta(), teal.yellow(), teal.black()}); // as CMYK
    SC_SHOW(sc::color{"Teal"} == teal);
    auto glass = sc::color{"rgba(255, 0, 0, 0.5)"};
    SC_STEP(glass.flatten(sc::color::White)); // half-transparent red, painted on white
    SC_SHOW(glass);
    return 0;
}
