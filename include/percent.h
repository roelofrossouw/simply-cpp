#ifndef SC_PERCENT_H
#define SC_PERCENT_H

#include <string>

namespace sc
{
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
