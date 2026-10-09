// Numbers in simply-cpp core: percentages, colours, small fixed-size matrices, and dual numbers,
// which work out derivatives as they calculate. Each line shows a call, as written, and what it
// returned. Needs no server.

#include <string>
#include <vector>

#include <sc.h>

int main() {
    sc::console::title("simply-cpp core: numbers");

    sc::console::heading("Percentages: sc::percent");
    sc::console::note("A fraction (0 to 1) or a number of percent (above 1), rounded for display.");
    SC_SHOW(std::string(sc::percent{0.256}));
    SC_SHOW(std::string(sc::percent{12.345, 1}));
    SC_SHOW(std::string{"Done: "} + sc::percent{0.75, 0});

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

    sc::console::heading("Matrices: sc::matrix");
    sc::matrix<long, 2, 2> fibonacci{1, 1, 1, 0};
    SC_SHOW(fibonacci);
    SC_SHOW(fibonacci ^ 10u); // its 10th power holds the 9th to 11th Fibonacci numbers
    sc::matrix<double, 2, 2> rotate{0, -1, 1, 0};
    sc::matrix<double, 2, 1> point{3, 4};
    SC_SHOW(rotate * point); // (3, 4) turned a quarter
    SC_SHOW(rotate * 2.5);

    sc::console::heading("Dual numbers: sc::dual");
    sc::console::note("~ marks the variable; every result then carries its derivative as the ε part.");
    auto x = ~sc::dual<double>{3};
    sc::console::show_text("f(x) = x * x * x + x * 2 at x = 3", std::string(x * x * x + x * 2)); // f = 33, f' = 3x² + 2 = 29
    sc::console::show_text("sin(x) at x = 0", std::string(sc::sin(~sc::dual<double>{0})));      // sin 0 = 0, cos 0 = 1
    sc::console::show_text("sqrt(x) at x = 16", std::string(sc::sqrt(~sc::dual<double>{16}))); // 4, 1/(2*4)
    return 0;
}
