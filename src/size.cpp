#include "size.h"

#include <algorithm>

namespace sc {
    template<typename T>
    void size_<T>::on_validate() {
        this->x_ = std::max(T{0}, this->x_);
        this->y_ = std::max(T{0}, this->y_);
    }
}

// on_validate() is called from pair_'s arithmetic operators in the header, so it
// needs an out-of-line copy in the library: an optimized build would otherwise
// inline it away and the symbol would never reach callers in other translation units.
template class sc::pair_<sc::size_<int>, int>;
template class sc::size_<int>;
template class sc::pair_<sc::size_<double>, double>;
template class sc::size_<double>;
