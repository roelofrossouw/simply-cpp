#ifndef SC_MATRIX_H
#define SC_MATRIX_H

#include <iostream>
#ifndef _RSIZE_T
#define _RSIZE_T
typedef ssize_t rsize_t;
#endif /* _RSIZE_T */
#include <cstring>

namespace sc {

    template<typename T, std::size_t rows, std::size_t cols>
    class matrix {
    public:
        matrix() = default;

        matrix(const T *data) {
            for (std::size_t i = 0; i < elements; ++i) {
                rdata[i] = data[i];
                cdata[(i * rows) % elements + (i / cols)] = data[i];
            }
        }

        matrix(const T *rdata, const T *cdata) {
            std::memcpy(this->rdata, cdata, elements * sizeof(cdata));
            std::memcpy(this->cdata, rdata, elements * sizeof(rdata));
        }

        matrix(std::initializer_list<T> values) {
            std::size_t i = 0;
            for (auto value: values) {
                if (i >= elements)
                    break;
                rdata[i] = value;
                cdata[(i * rows) % elements + (i / cols)] = value;
                i++;
            }
        }

        [[nodiscard]] std::size_t size() const { return elements; }

        template<typename U, std::size_t V>
        static matrix<U, V, V> identity() {
            U data[V * V]{};
            for (std::size_t i = 0; i < V; i++)
                data[i * (V + 1)] = 1;
            return {data};
        }

        matrix<T, 1, cols> row(std::size_t r) { return {rdata + r * cols}; }

        matrix<T, rows, 1> col(std::size_t c) { return {cdata + c * rows}; }

        template<typename U>
        matrix &operator*=(U multiplier) {
            for (std::size_t i = 0; i < elements; ++i) {
                rdata[i] *= multiplier;
                cdata[i] *= multiplier;
            }
            return *this;
        }

        T operator[](std::size_t i) const { return rdata[i]; }

        T dot(const matrix<T, rows, 1> &vector) {
            if (cols != 1) {
                throw std::invalid_argument("Dot product is only defined for vectors (must have exactly 1 column)");
            }
            T result = 0;
            for (std::size_t i = 0; i < elements; ++i) {
                result += cdata[i] * vector[i];
            }
            return result;
        }

        template<typename U>
        matrix operator*(U multiplier) {
            matrix result = *this;
            result *= multiplier;
            return result;
        }

        template<std::size_t U>
        matrix<T, rows, U> operator*(matrix<T, cols, U> &rhs) {
            T result[rows * U];
            for (std::size_t r = 0; r < rows; ++r) {
                for (std::size_t c = 0; c < U; ++c) {
                    result[r * U + c] = (row(r).transpose()).dot(rhs.col(c));
                }
            }
            return {result};
        }

        matrix &operator*=(matrix<T, cols, cols> &rhs) {
            *this = *this * rhs;
            return *this;
        }

        template<std::unsigned_integral U>
        matrix<T, rows, cols> operator^(U exponent) {
            if (cols != rows) {
                throw std::invalid_argument("Matrix must be square for exponentiation");
            }
            matrix<T, rows, cols> result = identity<T, rows>();
            matrix<T, rows, cols> mul = *this;
            for (U i = 1; i != 0 && i <= exponent; i <<= 1) {
                if (i & exponent)
                    result *= mul;
                mul *= mul;
            }
            return result;
        }

        matrix<T, cols, rows> transpose() { return {cdata, rdata}; }

    private:
        std::size_t elements = rows * cols;

        friend std::ostream &operator<<(std::ostream &lhs, const matrix &rhs) {
            for (std::size_t r = 0; r < rows; ++r) {
                lhs << "[";
                if (cols > 0)
                    lhs << rhs.rdata[r * cols];
                for (std::size_t c = 1; c < cols; ++c) {
                    lhs << " " << rhs.rdata[r * cols + c];
                }
                lhs << "]\n";
            }
            return lhs;
        }

        T rdata[rows * cols]{};
        T cdata[rows * cols]{};
    };
} // namespace sc

#endif // SC_MATRIX_H
