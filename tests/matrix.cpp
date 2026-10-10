#include <sc.h>

#include <vector>

#include "sc_test.h"

namespace {
    // Every element, row by row.
    template<typename T, std::size_t rows, std::size_t cols>
    std::vector<T> elements(const sc::matrix<T, rows, cols> &m) {
        std::vector<T> result;
        for (std::size_t i = 0; i < m.size(); ++i) result.push_back(m[i]);
        return result;
    }

    // Transposing, multiplying and powers for one element type, on a non-square matrix.
    template<typename T>
    void check_type() {
        sc::matrix<T, 2, 3> a{1, 2, 3,
                              4, 5, 6};
        auto at = a.transpose(); // 3 x 2
        CHECK_EQ(at.size(), std::size_t{6});
        CHECK(elements(at) == std::vector<T>({1, 4,
                                              2, 5,
                                              3, 6}));
        CHECK(elements(at.row(0)) == std::vector<T>({1, 4}));
        CHECK(elements(at.col(1)) == std::vector<T>({4, 5, 6}));
        CHECK(elements(at.transpose()) == elements(a)); // back again

        sc::matrix<T, 3, 2> b{7, 8,
                              9, 10,
                              11, 12};
        const auto ab = a * b; // 2 x 2
        CHECK(elements(ab) == std::vector<T>({58, 64,
                                              139, 154}));
        auto ba = b * a; // 3 x 3
        CHECK(elements(ba) == std::vector<T>({39, 54, 69,
                                              49, 68, 87,
                                              59, 82, 105}));

        sc::matrix<T, 3, 1> v{1, 2, 3};
        auto vt = v.transpose(); // 1 x 3
        CHECK(elements(vt) == std::vector<T>({1, 2, 3}));
        CHECK_EQ(v.dot(sc::matrix<T, 3, 1>{4, 5, 6}), T{32});

        sc::matrix<T, 2, 2> fibonacci{1, 1, 1, 0};
        CHECK(elements(fibonacci ^ 10u) == std::vector<T>({89, 55, 55, 34}));
        auto identity = sc::matrix<T, 3, 3>::template identity<T, 3>();
        CHECK(elements(ba * identity) == elements(ba));
    }
}

int main() {
    SECTION("int");
    check_type<int>();

    SECTION("long");
    check_type<long>();

    SECTION("double");
    check_type<double>();

    SECTION("short");
    check_type<short>(); // smaller than a pointer, which the copy once assumed

    SECTION("Scaling");
    {
        sc::matrix<int, 2, 3> a{1, 2, 3, 4, 5, 6};
        CHECK(elements(a * 2) == std::vector<int>({2, 4, 6, 8, 10, 12}));
        CHECK(elements((a * 2).transpose()) == std::vector<int>({2, 8, 4, 10, 6, 12}));
    }

    TEST_SUMMARY();
}
