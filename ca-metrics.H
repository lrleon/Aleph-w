/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/

/** @file ca-metrics.H
 *  @brief Free metric helpers for cellular automata frames.
 *
 *  Provides allocation-free utilities to compute simple aggregate
 *  statistics over any `LatticeLike` lattice without coupling them
 *  to a particular boundary policy or storage type. Used both by
 *  the Phase-7 observers (`ca-observer.H`) and by user code that
 *  needs to evaluate a metric ad-hoc on a frame.
 *
 *  Catalogue:
 *
 *    - `for_each_cell(lat, f)`        — generic cell visitor for
 *      ranks 1, 2 and 3, plus the rank-1 graph lattice.
 *    - `count_state(lat, s)`          — number of cells in state `s`.
 *    - `count_alive(lat)`             — `count_state(lat, T{}, true)`,
 *      i.e., number of cells whose value differs from `state_type{}`.
 *    - `density(lat, s)`              — alive ratio in `[0, 1]`.
 *    - `state_histogram(lat, max)`    — vector of counts per integer
 *      state in `[0, max)`.
 *    - `shannon_entropy(lat, max)`    — Shannon entropy in nats over
 *      the state distribution; `0` for empty or constant lattices.
 *    - `frame_hash(lat)`              — deterministic 64-bit FNV-1a
 *      hash of the cell sequence in row-major order. Stable across
 *      runs and across builds (no random seed, no implementation-
 *      defined hashing).
 *    - `frames_equal(a, b)`           — bit-for-bit comparison.
 *    - `cell_diff_count(a, b)`        — number of cells that differ.
 *
 *  All helpers are header-only `inline` templates and do not
 *  allocate (`state_histogram` allocates one buffer that the caller
 *  may pre-size).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef CA_METRICS_H
#define CA_METRICS_H

#include <cmath>
#include <cstddef>
#include <cstdint>
#include <type_traits>
#include <vector>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_metrics_detail {

/// FNV-1a 64-bit basis and prime constants.
inline constexpr std::uint64_t fnv_basis = 14695981039346656037ull;
inline constexpr std::uint64_t fnv_prime = 1099511628211ull;

/// Mix one byte into an FNV-1a hash.
inline constexpr std::uint64_t fnv1a_step(std::uint64_t h, const std::uint8_t b) noexcept
{
  h ^= static_cast<std::uint64_t>(b);
  h *= fnv_prime;
  return h;
}

/// Mix every byte of `value` (in little-endian order) into an FNV-1a hash.
template <typename T>
inline std::uint64_t fnv1a_mix(std::uint64_t h, const T &value) noexcept
{
  static_assert(std::is_trivially_copyable_v<T>,
                "frame_hash requires trivially copyable cell states");
  std::uint8_t buf[sizeof(T)];
  // Defensive memcpy avoids strict-aliasing pitfalls.
  __builtin_memcpy(buf, &value, sizeof(T));
  for (std::size_t i = 0; i < sizeof(T); ++i)
    h = fnv1a_step(h, buf[i]);
  return h;
}

/// Detect whether `L` exposes a graph-style `at_node(size_t)` accessor.
template <typename L>
concept HasAtNode = requires(const L &l) { l.at_node(std::size_t{}); };

}  // namespace ca_metrics_detail

/** @brief Visit every cell of a lattice in canonical row-major order.
 *
 *  Calls `f(value)` once per cell. Works for the rectangular ranks
 *  (1, 2, 3) and for graph lattices (any type that exposes
 *  `at_node(size_t)`).
 *
 *  @tparam Lattice lattice type satisfying `LatticeLike`.
 *  @tparam F invocable with the cell `state_type`.
 *  @param[in] lat lattice to traverse.
 *  @param[in] f   functor applied to every cell value.
 *
 *  @par Complexity
 *  O(N), where N is the cell count.
 */
template <typename Lattice, typename F>
inline void for_each_cell(const Lattice &lat, F &&f)
{
  using coord_t = typename Lattice::coord_type;
  if constexpr (ca_metrics_detail::HasAtNode<Lattice>)
    {
      const ca_size_t n = lat.size();
      for (ca_size_t i = 0; i < n; ++i)
        f(lat.at_node(i));
    }
  else if constexpr (Lattice::rank == 1)
    {
      const ca_size_t n0 = lat.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        f(lat.at(coord_t{static_cast<ca_index_t>(i)}));
    }
  else if constexpr (Lattice::rank == 2)
    {
      const ca_size_t n0 = lat.size(0);
      const ca_size_t n1 = lat.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          f(lat.at(coord_t{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)}));
    }
  else if constexpr (Lattice::rank == 3)
    {
      const ca_size_t n0 = lat.size(0);
      const ca_size_t n1 = lat.size(1);
      const ca_size_t n2 = lat.size(2);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          for (ca_size_t k = 0; k < n2; ++k)
            f(lat.at(coord_t{static_cast<ca_index_t>(i),
                             static_cast<ca_index_t>(j),
                             static_cast<ca_index_t>(k)}));
    }
  else
    {
      static_assert(Lattice::rank <= 3, "for_each_cell currently supports rank <= 3");
    }
}

/// @return number of cells whose value equals `s`.
template <typename Lattice>
[[nodiscard]] inline ca_size_t count_state(const Lattice &lat, const typename Lattice::state_type &s)
{
  ca_size_t cnt = 0;
  for_each_cell(lat,
                [&](const auto &v)
  {
    if (v == s)
      ++cnt;
  });
  return cnt;
}

/// @return number of cells whose value differs from `state_type{}`.
template <typename Lattice>
[[nodiscard]] inline ca_size_t count_alive(const Lattice &lat)
{
  using state_t = typename Lattice::state_type;
  const state_t dead{};
  ca_size_t cnt = 0;
  for_each_cell(lat,
                [&](const auto &v)
  {
    if (v != dead)
      ++cnt;
  });
  return cnt;
}

/// @return total cell count of the lattice (works for rectangular and
/// graph lattices alike).
template <typename Lattice>
[[nodiscard]] inline ca_size_t total_cells(const Lattice &lat) noexcept
{
  return lat.size();
}

/// @return ratio in `[0, 1]` of cells in state `s`. Returns `0` for
/// empty lattices.
template <typename Lattice>
[[nodiscard]] inline double density(const Lattice &lat, const typename Lattice::state_type &s)
{
  const ca_size_t total = total_cells(lat);
  if (total == 0)
    return 0.0;
  return static_cast<double>(count_state(lat, s)) / static_cast<double>(total);
}

/// @return ratio in `[0, 1]` of cells whose value differs from
/// `state_type{}`. Convenience for binary CAs.
template <typename Lattice>
[[nodiscard]] inline double alive_density(const Lattice &lat)
{
  const ca_size_t total = total_cells(lat);
  if (total == 0)
    return 0.0;
  return static_cast<double>(count_alive(lat)) / static_cast<double>(total);
}

/** @brief Count occurrences of every integer state in `[0, max_state)`.
 *
 *  Useful for non-binary CAs that work over a small discrete state
 *  set. Cells whose state is outside the range are silently ignored.
 *
 *  @param[in] lat       lattice to inspect.
 *  @param[in] max_state exclusive upper bound on the state values.
 *  @return vector of size `max_state` with the count for each state.
 */
template <typename Lattice>
[[nodiscard]] inline std::vector<ca_size_t> state_histogram(const Lattice &lat, std::size_t max_state)
{
  std::vector<ca_size_t> hist(max_state, 0);
  for_each_cell(lat,
                [&](const auto &v)
  {
    using S = typename Lattice::state_type;
    if constexpr (std::is_integral_v<S> or std::is_enum_v<S>)
      {
        const long long iv = static_cast<long long>(v);
        if (iv >= 0 and iv < static_cast<long long>(max_state))
          ++hist[static_cast<std::size_t>(iv)];
      }
    else
      {
        // Non-integral states are not classifiable into
        // a small histogram; the caller should provide a
        // bespoke metric. Suppress the unused parameter
        // warning while keeping the signature uniform.
        (void) v;
      }
  });
  return hist;
}

/** @brief Shannon entropy in nats of the `[0, max_state)` distribution.
 *
 *  Returns `0` when the lattice is empty or every cell holds the same
 *  state. Uses natural logarithms; multiply by `1 / log(2)` to obtain
 *  bits.
 *
 *  @param[in] lat       lattice to evaluate.
 *  @param[in] max_state exclusive upper bound on state values.
 *  @return Shannon entropy `H = -Σ p_i ln(p_i)`.
 */
template <typename Lattice>
[[nodiscard]] inline double shannon_entropy(const Lattice &lat, std::size_t max_state)
{
  const auto hist = state_histogram(lat, max_state);
  ca_size_t total = 0;
  for (auto c : hist)
    total += c;
  if (total == 0)
    return 0.0;
  double h = 0.0;
  const double inv = 1.0 / static_cast<double>(total);
  for (auto c : hist)
    if (c != 0)
      {
        const double p = static_cast<double>(c) * inv;
        h -= p * std::log(p);
      }
  return h;
}

/** @brief Deterministic 64-bit FNV-1a hash of the lattice cells.
 *
 *  Visits cells in canonical row-major order and folds their byte
 *  representation into a running FNV-1a 64-bit hash. The result is
 *  stable across runs, builds and machines with the same endianness
 *  for any trivially copyable `state_type`. The cell-by-cell layout
 *  guarantees that two lattices that compare equal hash to the same
 *  value.
 *
 *  @param[in] lat lattice to hash.
 *  @return 64-bit FNV-1a hash of the cell sequence.
 *
 *  @par Complexity
 *  O(N) byte mixes, where N is the cell count.
 */
template <typename Lattice>
[[nodiscard]] inline std::uint64_t frame_hash(const Lattice &lat)
{
  std::uint64_t h = ca_metrics_detail::fnv_basis;
  for_each_cell(lat,
                [&](const auto &v)
  {
    h = ca_metrics_detail::fnv1a_mix(h, v);
  });
  return h;
}

/// @return `true` iff `a` and `b` have identical extents and values.
template <typename Lattice>
[[nodiscard]] inline bool frames_equal(const Lattice &a, const Lattice &b)
{
  if (a.extents() != b.extents())
    return false;
  using coord_t = typename Lattice::coord_type;
  if constexpr (ca_metrics_detail::HasAtNode<Lattice>)
    {
      const ca_size_t n = a.size();
      for (ca_size_t i = 0; i < n; ++i)
        if (a.at_node(i) != b.at_node(i))
          return false;
      return true;
    }
  else if constexpr (Lattice::rank == 1)
    {
      const ca_size_t n0 = a.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        {
          const coord_t c{static_cast<ca_index_t>(i)};
          if (a.at(c) != b.at(c))
            return false;
        }
      return true;
    }
  else if constexpr (Lattice::rank == 2)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          {
            const coord_t c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
            if (a.at(c) != b.at(c))
              return false;
          }
      return true;
    }
  else if constexpr (Lattice::rank == 3)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      const ca_size_t n2 = a.size(2);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          for (ca_size_t k = 0; k < n2; ++k)
            {
              const coord_t c{static_cast<ca_index_t>(i),
                              static_cast<ca_index_t>(j),
                              static_cast<ca_index_t>(k)};
              if (a.at(c) != b.at(c))
                return false;
            }
      return true;
    }
  else
    {
      static_assert(Lattice::rank <= 3, "frames_equal currently supports rank <= 3");
      return false;
    }
}

/// @return number of cells where `a` and `b` differ. Both lattices
/// must have the same extents.
template <typename Lattice>
[[nodiscard]] inline ca_size_t cell_diff_count(const Lattice &a, const Lattice &b)
{
  ah_length_error_if(a.extents() != b.extents())
    << "cell_diff_count: lattices have different extents";

  using coord_t = typename Lattice::coord_type;
  ca_size_t diff = 0;
  if constexpr (ca_metrics_detail::HasAtNode<Lattice>)
    {
      const ca_size_t n = a.size();
      for (ca_size_t i = 0; i < n; ++i)
        if (a.at_node(i) != b.at_node(i))
          ++diff;
    }
  else if constexpr (Lattice::rank == 1)
    {
      const ca_size_t n0 = a.size(0);
      for (ca_size_t i = 0; i < n0; ++i)
        {
          const coord_t c{static_cast<ca_index_t>(i)};
          if (a.at(c) != b.at(c))
            ++diff;
        }
    }
  else if constexpr (Lattice::rank == 2)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          {
            const coord_t c{static_cast<ca_index_t>(i), static_cast<ca_index_t>(j)};
            if (a.at(c) != b.at(c))
              ++diff;
          }
    }
  else if constexpr (Lattice::rank == 3)
    {
      const ca_size_t n0 = a.size(0);
      const ca_size_t n1 = a.size(1);
      const ca_size_t n2 = a.size(2);
      for (ca_size_t i = 0; i < n0; ++i)
        for (ca_size_t j = 0; j < n1; ++j)
          for (ca_size_t k = 0; k < n2; ++k)
            {
              const coord_t c{i, j, k};
              if (a.at(c) != b.at(c))
                ++diff;
            }
    }
  else
    {
      static_assert(Lattice::rank <= 3, "cell_diff_count currently supports rank <= 3");
    }
  return diff;
}

}  // namespace CA
}  // namespace Aleph

#endif  // CA_METRICS_H