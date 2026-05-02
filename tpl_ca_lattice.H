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

/** @file tpl_ca_lattice.H
 *  @brief Cellular automata lattice with pluggable boundary policies.
 *
 *  Provides `Aleph::CA::Lattice<Storage, Boundary>` — a thin layer on
 *  top of any `CA::Lattice`-conformant storage that adds boundary-aware
 *  access via `at_safe(coord)`. The `Boundary` tag type drives the
 *  resolution of out-of-range coordinates at compile time:
 *
 *    - `OpenBoundary`        — out-of-range cells read as `state_type{}`.
 *    - `ToroidalBoundary`    — coordinates wrap around on every axis.
 *    - `ReflectiveBoundary`  — coordinates mirror back into the lattice
 *                              (no-repeat reflection).
 *    - `ConstantBoundary<T, V>` — out-of-range cells read as `V`
 *                              (Dirichlet condition).
 *    - `NeumannBoundary`     — coordinates clamp to the nearest in-range
 *                              cell (zero-gradient).
 *
 *  Strict access (`at`, `set`) goes straight through the storage and
 *  retains the storage's bounds-checking. Use `at_safe` only when the
 *  caller may legitimately provide out-of-range coordinates (e.g., a
 *  neighbourhood iterating across a corner).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_LATTICE_H
#define TPL_CA_LATTICE_H

#include <cstddef>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>

namespace Aleph {
namespace CA {

namespace ca_lattice_detail {

/// Wrap a signed coordinate into `[0, n)` with positive modulo.
inline ca_size_t wrap_into_range(ca_index_t c, const ca_size_t n) noexcept
{
  const ca_index_t m = static_cast<ca_index_t>(n);
  ca_index_t r = c % m;
  if (r < 0)
    r += m;
  return static_cast<ca_size_t>(r);
}

/// Reflect a signed coordinate into `[0, n)` (no-repeat triangle wave).
/// Maps `-1 → 0`, `-2 → 1`, `n → n-1`, `n+1 → n-2`, etc.
inline ca_size_t reflect_into_range(const ca_index_t c, const ca_size_t n) noexcept
{
  if (n == 1)
    return 0;
  const ca_index_t period = static_cast<ca_index_t>(2 * n);
  ca_index_t r = c % period;
  if (r < 0)
    r += period;
  const ca_index_t ni = static_cast<ca_index_t>(n);
  if (r >= ni)
    r = period - 1 - r;
  return static_cast<ca_size_t>(r);
}

/// True iff `B` is a `ConstantBoundary<T, V>`.
template <typename B>
struct is_constant_boundary : std::false_type
{
};

template <typename T, T V>
struct is_constant_boundary<ConstantBoundary<T, V>> : std::true_type
{
};

template <typename B>
inline constexpr bool is_constant_boundary_v = is_constant_boundary<B>::value;
}  // namespace ca_lattice_detail

/** @brief Lattice that adds boundary-aware access on top of a storage.
 *
 *  @tparam Storage  storage type satisfying `CA::LatticeLike` (e.g.
 *                   `Dense_Cell_Storage<T, N>` or `Bit_Cell_Storage<N>`).
 *  @tparam Boundary boundary tag type (default `OpenBoundary`).
 *
 *  @par Complexity
 *  `at`, `set`, `at_safe` are O(N) where `N` is the lattice rank
 *  (which is a small compile-time constant — 1, 2 or 3 in practice).
 */
template <typename Storage, typename Boundary = OpenBoundary>
class Lattice
{
public:
  using storage_type = Storage;
  using boundary_type = Boundary;
  using state_type = typename Storage::state_type;
  using coord_type = typename Storage::coord_type;
  using extents_type = typename Storage::extents_type;

  static constexpr std::size_t rank = Storage::rank;

private:
  Storage store_;

  [[nodiscard]] state_type out_of_range_value() const
  {
    if constexpr (ca_lattice_detail::is_constant_boundary_v<Boundary>)
      return static_cast<state_type>(Boundary::value);
    else
      return state_type{};
  }

  /// Try to fold `c` into a valid in-range coordinate using the
  /// boundary policy. Returns `false` for `OpenBoundary` /
  /// `ConstantBoundary` when `c` is out of range, in which case the
  /// caller should return `out_of_range_value()` instead.
  [[nodiscard]] bool fold_coord(coord_type &c) const noexcept
  {
    using namespace ca_lattice_detail;
    for (std::size_t d = 0; d < rank; ++d)
      {
        const ca_size_t n = store_.size(d);
        if (n == 0)
          return false;

        if constexpr (std::is_same_v<Boundary, ToroidalBoundary>)
          {
            c[d] = static_cast<ca_index_t>(wrap_into_range(c[d], n));
          }
        else if constexpr (std::is_same_v<Boundary, ReflectiveBoundary>)
          {
            c[d] = static_cast<ca_index_t>(reflect_into_range(c[d], n));
          }
        else if constexpr (std::is_same_v<Boundary, NeumannBoundary>)
          {
            // Zero-gradient: clamp to the nearest in-range coordinate.
            if (c[d] < 0)
              c[d] = 0;
            else if (static_cast<ca_size_t>(c[d]) >= n)
              c[d] = static_cast<ca_index_t>(n - 1);
          }
        else
          {
            // OpenBoundary or ConstantBoundary<...>
            if (c[d] < 0 or static_cast<ca_size_t>(c[d]) >= n)
              return false;
          }
      }
    return true;
  }

public:
  /// Construct an empty lattice (no cells allocated).
  Lattice() = default;

  /** @brief Construct a lattice with the given extents and initial value.
   *  @param[in] extents per-axis sizes.
   *  @param[in] init initial value for every cell.
   */
  explicit Lattice(const extents_type &extents, const state_type &init = state_type{})
    : store_(extents, init)
  {}

  /// Wrap an existing storage by move.
  explicit Lattice(Storage &&s) noexcept(std::is_nothrow_move_constructible_v<Storage>)
    : store_(std::move(s))
  {}

  [[nodiscard]] static constexpr std::size_t dimension() noexcept
  {
    return rank;
  }

  [[nodiscard]] ca_size_t size() const noexcept
  {
    return store_.size();
  }
  [[nodiscard]] ca_size_t size(std::size_t d) const
  {
    return store_.size(d);
  }

  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return store_.extents();
  }

  /// Strict access: throws if `c` is out of range.
  [[nodiscard]] state_type at(const coord_type &c) const
  {
    return store_.at(c);
  }

  /// Strict write: throws if `c` is out of range.
  void set(const coord_type &c, const state_type &v)
  {
    store_.set(c, v);
  }

  /** @brief Boundary-aware read.
   *
   *  Out-of-range coordinates are resolved according to `Boundary`:
   *
   *  - `OpenBoundary`        → returns `state_type{}`.
   *  - `ConstantBoundary<T, V>` → returns `V`.
   *  - `ToroidalBoundary`    → wraps around.
   *  - `ReflectiveBoundary`  → mirrors back.
   *  - `NeumannBoundary`     → clamps to the nearest in-range cell.
   *
   *  @param[in] c possibly out-of-range coordinate.
   *  @return the cell value resolved through the boundary policy.
   */
  [[nodiscard]] state_type at_safe(coord_type c) const
  {
    if (fold_coord(c))
      return store_.at(c);
    return out_of_range_value();
  }

  /// Direct read access to the underlying storage (read-only).
  [[nodiscard]] const Storage &storage() const noexcept
  {
    return store_;
  }

  /// Direct read-write access to the underlying storage.
  [[nodiscard]] Storage &storage() noexcept
  {
    return store_;
  }

  /// Set every cell to `value`.
  void fill(const state_type &value)
  {
    store_.fill(value);
  }

  /// O(1) swap.
  void swap(Lattice &other) noexcept(noexcept(store_.swap(other.store_)))
  {
    store_.swap(other.store_);
  }
};

/** @brief Free-function swap so the lattice plays nicely with std::swap.
 *  @param[in,out] a first lattice.
 *  @param[in,out] b second lattice.
 *  @tparam Storage storage type.
 *  @tparam Boundary boundary tag type.
 */
template <typename Storage, typename Boundary>
inline void swap(Lattice<Storage, Boundary> &a,
                 Lattice<Storage, Boundary> &b) noexcept(noexcept(a.swap(b)))
{
  a.swap(b);
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_LATTICE_H TPL_CA_LATTICE_H