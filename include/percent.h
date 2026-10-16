#ifndef SC_PERCENT_H
#define SC_PERCENT_H

#include <string>

namespace sc
{
    // A percentage, from a fraction (0.25) or, above 1, a number of percent (25); at most 100%.
    // As a double it is the number of percent rounded to decimals (25.00), as text with a '%'.
    class percent
    {
    public:
        percent(double value = 0, int decimals = 2);
        operator double() const;
        operator std::string() const;
        friend std::string operator +(const std::string& lhs, percent rhs);
        void decimals(int set);

    private:
        double value;
        int decimals_{2};
    };
} // namespace sc

#endif // SC_PERCENT_H
