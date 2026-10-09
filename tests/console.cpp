#include <sc.h>

#include "sc_test.h"

#include <map>
#include <optional>
#include <sstream>
#include <string>
#include <vector>

using strings = std::vector<std::string>;

int main() {
    SECTION("format");
    CHECK_EQ(sc::console::format("text"), std::string{"\"text\""});
    CHECK_EQ(sc::console::format(std::string{"text"}), std::string{"\"text\""});
    CHECK_EQ(sc::console::format(std::string_view{"view"}), std::string{"\"view\""});
    CHECK_EQ(sc::console::format(42), std::string{"42"});
    CHECK_EQ(sc::console::format(true), std::string{"true"});
    CHECK_EQ(sc::console::format(std::optional<std::string>{}), std::string{"(none)"});
    CHECK_EQ(sc::console::format(std::optional<std::string>{"x"}), std::string{"\"x\""});
    CHECK_EQ(sc::console::format(strings{"a", "b"}), std::string{"[\"a\", \"b\"]"});
    CHECK_EQ(sc::console::format(strings{}), std::string{"[]"});
    CHECK_EQ(sc::console::format(std::map<std::string, int>{{"a", 1}, {"b", 2}}), std::string{"{\"a\": 1, \"b\": 2}"});
    CHECK_EQ(sc::console::format(std::vector<std::optional<std::string>>{"x", std::nullopt}),
             std::string{"[\"x\", (none)]"});
    CHECK_EQ(sc::console::format(std::vector<std::map<std::string, std::string>>{{{"k", "v"}}}),
             std::string{"[{\"k\": \"v\"}]"});
    // A type with its own << (and begin/end) is printed with <<, not as a list.
    CHECK_EQ(sc::console::format(sc::ip_endpoints{"redis1;redis2:6380", 6379}), std::string{"redis1:6379;redis2:6380"});

    SECTION("Printing");
    std::ostringstream printed;
    sc::console::output(printed);
    sc::console::title("Demo ü");
    sc::console::heading("Section");
    sc::console::subheading("Part ü");
    sc::console::note("a note");
    sc::console::show("call()", std::string{"result"});
    sc::console::show_text("GET /", "{\n  \"a\": 1\n}\n");
    int runs = 0;
    SC_STEP(++runs);
    SC_SHOW(runs + 1);
    sc::console::output(std::cout);
    CHECK_EQ(runs, 1);
    CHECK_EQ(printed.str(), std::string{
        "Demo ü\n"
        "======\n"
        "\n"
        "Section\n"
        "-------\n"
        "\n"
        "  \u25B8 Part ü\n"
        "  a note\n"
        "  call()\n"
        "      -> \"result\"\n"
        "  GET /\n"
        "      -> {\n"
        "           \"a\": 1\n"
        "         }\n"
        "  ++runs\n"
        "  runs + 1\n"
        "      -> 2\n"});

    TEST_SUMMARY();
}
