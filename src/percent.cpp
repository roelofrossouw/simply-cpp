#include "percent.h"

#include <algorithm>
#include <cmath>
#include <iomanip>
#include <sstream>

namespace sc
{
    percent::percent(const double value, const int decimals)
        : decimals_(decimals), value(value > 1 ? value / 100 : value)
    {
        if (this->value > 1) this->value = 1;
    }

    percent::operator double() const
    {
        const int precision = std::max(0, decimals_);
        const double factor = std::pow(10.0, precision);
        return std::round(value * 100 * factor) / factor;
    }

    percent::operator std::string() const
    {
        std::ostringstream result;
        result << std::fixed << std::setprecision(std::max(0, decimals_))
            << static_cast<double>(*this) << '%';
        return result.str();
    }

    void percent::decimals(int set)
    {
        decimals_ = set;
    }

    std::string operator+(const std::string& lhs, percent rhs)
    {
        return lhs + static_cast<std::string>(rhs);
    }
} // namespace sc
