#include <cassert>
#include <string>

#include "sc.h"

int main()
{
    sc::percent value{0.40524};
    assert(static_cast<double>(value) == 40.52);
    assert(static_cast<std::string>(value) == "40.52%");

    value.decimals(1);
    assert(static_cast<double>(value) == 40.5);
    assert(static_cast<std::string>(value) == "40.5%");

    const sc::percent precise{0.40524, 3};
    assert(static_cast<double>(precise) == 40.524);
    assert(static_cast<std::string>(precise) == "40.524%");

    assert(static_cast<double>(sc::percent{10}) == 10);
    assert(static_cast<double>(sc::percent{0.1}) == 10);
    assert(static_cast<double>(sc::percent{110}) == 100);
    assert(static_cast<std::string>(sc::percent{110}) == "100.00%");
}
