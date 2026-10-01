#pragma once
#include "point.h"

#include <ostream>

namespace sc {
    template<typename T>
    class size_ : public pair_<size_<T>, T> {
    public:
        using pair_<size_<T>, T>::pair_;
        operator point_<T>() { return {this->x_, this->y_}; }
        T width() const { return this->x_; }
        T width(const T &r) { return this->x_ = r; }
        T height() const { return this->y_; }
        T height(const T &r) { return this->y_ = r; }
        T area() const { return width() * height(); }
        friend class pair_<size_, T>;

    private:
        void on_validate();

        friend std::ostream &operator<<(std::ostream &lhs, const size_ &rhs) { return lhs << "[" << rhs.x_ << "," << rhs.y_ << "]"; }
    };

    using size = size_<double>;
    using size_i = size_<int>;
} // namespace sc
