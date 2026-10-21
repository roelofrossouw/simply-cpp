#pragma once

#include <algorithm>
#include <cmath>
#include <concepts>
#include <cstddef>
#include <deque>
#include <functional>
#include <ranges>
#include <stdexcept>
#include <vector>

namespace sc {
    namespace detail {
        template<typename T, typename Distance>
        std::vector<int> dbscan_impl(const std::vector<T> &values, const double epsilon,
                                     const std::size_t minimum_neighbors, Distance distance) {
            constexpr int Unvisited = -2;
            constexpr int Noise = -1;
            std::vector<int> labels(values.size(), Unvisited);
            int cluster_id = 0;

            const auto neighbors_of = [&](const std::size_t index) {
                std::vector<std::size_t> neighbors;
                for (std::size_t candidate = 0; candidate < values.size(); ++candidate)
                    if (distance(values[index], values[candidate]) <= epsilon) neighbors.push_back(candidate);
                return neighbors;
            };

            for (std::size_t i = 0; i < values.size(); ++i) {
                if (labels[i] != Unvisited) continue;
                auto neighbors = neighbors_of(i);
                if (neighbors.size() < minimum_neighbors) {
                    labels[i] = Noise;
                    continue;
                }

                labels[i] = cluster_id;
                std::deque<std::size_t> candidates(neighbors.begin(), neighbors.end());
                std::vector<bool> queued(values.size());
                for (const std::size_t candidate: neighbors) queued[candidate] = true;
                while (!candidates.empty()) {
                    const std::size_t candidate = candidates.front();
                    candidates.pop_front();
                    if (labels[candidate] == Noise) labels[candidate] = cluster_id;
                    if (labels[candidate] != Unvisited) continue;

                    labels[candidate] = cluster_id;
                    auto candidate_neighbors = neighbors_of(candidate);
                    if (candidate_neighbors.size() < minimum_neighbors) continue;
                    for (const std::size_t neighbor: candidate_neighbors) {
                        if (queued[neighbor]) continue;
                        candidates.push_back(neighbor);
                        queued[neighbor] = true;
                    }
                }
                ++cluster_id;
            }
            return labels;
        }

        inline void validate_dbscan_parameters(const double epsilon, const std::size_t minimum_neighbors) {
            if (!std::isfinite(epsilon) || epsilon < 0)
                throw std::invalid_argument{"DBSCAN epsilon must be finite and non-negative"};
            if (minimum_neighbors == 0)
                throw std::invalid_argument{"DBSCAN minimum neighbor count must be greater than zero"};
        }
    }

    /// Clusters numbers with DBSCAN: a label per value, counting from 0, and -1 for noise.
    /// minimum_neighbors includes the value itself. A period above 0 makes distances wrap around
    /// (360 for compass bearings). Floating point values must be finite (std::invalid_argument).
    template<typename T>
        requires std::integral<T> || std::floating_point<T>
    [[nodiscard]] std::vector<int> dbscan(const std::vector<T> &values, const double epsilon,
                                          const std::size_t minimum_neighbors, const double period = 0) {
        detail::validate_dbscan_parameters(epsilon, minimum_neighbors);
        if (!std::isfinite(period) || period < 0)
            throw std::invalid_argument{"DBSCAN period must be finite and non-negative"};
        if constexpr (std::floating_point<T>) {
            if (std::ranges::any_of(values, [](const T value) { return !std::isfinite(value); }))
                throw std::invalid_argument{"DBSCAN values must be finite"};
        }

        return detail::dbscan_impl(values, epsilon, minimum_neighbors, [period](const T lhs, const T rhs) {
            const double difference = std::abs(static_cast<double>(lhs) - static_cast<double>(rhs));
            if (period == 0) return difference;
            const double wrapped = std::fmod(difference, period);
            return std::min(wrapped, period - wrapped);
        });
    }

    /// Clusters any values by a distance of your own, which must be finite and non-negative.
    /// Labels as dbscan().
    template<std::ranges::forward_range Range, typename Distance>
        requires std::copy_constructible<std::ranges::range_value_t<Range>> &&
                 std::invocable<Distance &, const std::ranges::range_value_t<Range> &,
                                const std::ranges::range_value_t<Range> &> &&
                 std::convertible_to<std::invoke_result_t<Distance &,
                                                         const std::ranges::range_value_t<Range> &,
                                                         const std::ranges::range_value_t<Range> &>,
                                     double>
    [[nodiscard]] std::vector<int> dbscan_by(const Range &values, const double epsilon,
                                             const std::size_t minimum_neighbors, Distance distance) {
        detail::validate_dbscan_parameters(epsilon, minimum_neighbors);
        using value_type = std::ranges::range_value_t<Range>;
        std::vector<value_type> copied(std::ranges::begin(values), std::ranges::end(values));
        const auto metric = [&distance](const value_type &lhs, const value_type &rhs) {
            const double result = std::invoke(distance, lhs, rhs);
            if (!std::isfinite(result) || result < 0)
                throw std::invalid_argument{"DBSCAN distance must be finite and non-negative"};
            return result;
        };
        return detail::dbscan_impl(copied, epsilon, minimum_neighbors, metric);
    }
}
