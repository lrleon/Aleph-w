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

/** @file tpl_ca_ghost_lattice.H
 *  @brief Ghost-layer lattice for high-performance boundary handling.
 *
 *  `Aleph::CA::Ghost_Lattice<Storage, Boundary, Halo>` wraps a backing
 *  storage that is larger than the user-visible lattice by `Halo`
 *  cells on every side of every axis, so that neighbour reads near
 *  the border land in real storage instead of branching on the
 *  boundary policy.
 *
 *  The lattice exposes *user* coordinates in `[0, size(d))`. Internally
 *  it translates them to *store* coordinates in
 *  `[Halo, Halo + size(d))` of the underlying `Storage` object.
 *
 *  Halo cells are refreshed by `refresh_halo()` before every
 *  synchronous step (the engine calls it automatically when the
 *  lattice advertises the method). Four policies are supported:
 *
 *    - `OpenBoundary`                  — halo cells hold `state_type{}`.
 *    - `ToroidalBoundary` / `PeriodicBoundary`
 *                                      — halo cells copy the opposite edge.
 *    - `ReflectiveBoundary`            — halo cells mirror the interior.
 *    - `ConstantBoundary<T, V>` /
 *      `DirichletBoundary<T, V>`       — halo cells hold `V`.
 *    - `NeumannBoundary`               — halo cells replicate the nearest
 *                                        interior cell (zero gradient).
 *
 *  For any neighbourhood radius `R <= Halo`, `at_safe(c)` degenerates
 *  into a single `store_.at(c + Halo)` call with no boundary branch
 *  inside the rule loop. That is the whole point of Phase 4: lift the
 *  boundary logic out of the hot loop.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_GHOST_LATTICE_H
#define TPL_CA_GHOST_LATTICE_H

#include <cstddef>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_lattice.H>  // for ca_lattice_detail::is_constant_boundary_v

namespace Aleph {
namespace CA {

namespace ca_ghost_detail {

/// Bump the storage extents by `2 * Halo` on every axis.
template <std::size_t N>
inline std::array<ca_size_t, N> enlarge_extents(const std::array<ca_size_t, N> &user,
                                                const std::size_t halo) noexcept
{
  std::array<ca_size_t, N> e{};
  for (std::size_t d = 0; d < N; ++d)
    e[d] = user[d] == 0 ? 0 : user[d] + 2 * halo;
  return e;
}

/// Wrap a possibly-negative index into `[0, n)` with positive modulo.
inline ca_size_t positive_mod(ca_index_t v, const ca_size_t n) noexcept
{
  const ca_index_t m = static_cast<ca_index_t>(n);
  ca_index_t r = v % m;
  if (r < 0)
    r += m;
  return static_cast<ca_size_t>(r);
}

/// Reflect a signed coordinate into `[0, n)` using the no-repeat triangle
/// wave that also powers `Lattice::at_safe` under `ReflectiveBoundary`.
inline ca_size_t reflect_index(const ca_index_t v, const ca_size_t n) noexcept
{
  if (n == 1)
    return 0;
  const ca_index_t period = static_cast<ca_index_t>(2 * n);
  ca_index_t r = v % period;
  if (r < 0)
    r += period;
  const ca_index_t ni = static_cast<ca_index_t>(n);
  if (r >= ni)
    r = period - 1 - r;
  return static_cast<ca_size_t>(r);
}

/// Clamp a signed coordinate into `[0, n - 1]` for Neumann fills.
inline ca_size_t clamp_index(const ca_index_t v, const ca_size_t n) noexcept
{
  if (v < 0)
    return 0;
  const ca_size_t u = static_cast<ca_size_t>(v);
  return u >= n ? n - 1 : u;
}

}  // namespace ca_ghost_detail

/** @brief Lattice with `Halo` ghost layers around the user-visible cells.
 *
 *  @tparam Storage  storage type satisfying `CA::LatticeLike`. The
 *                   storage is constructed with extents `user + 2*Halo`
 *                   so that the halo is plain backing memory.
 *  @tparam Boundary boundary tag (default `OpenBoundary`). See the
 *                   file-level documentation for the accepted tags.
 *  @tparam Halo     halo thickness in cells (default 1). The
 *                   neighbourhood radius consumed by the engine must
 *                   be `<= Halo` for the fast path to kick in.
 *
 *  @par Complexity
 *  - `at`, `set`, `at_safe`: O(`rank`) (a few additions per call).
 *  - `refresh_halo`: O(boundary-surface cells), i.e. linear in the
 *    halo volume (which for 2D/3D is `O(n^{rank-1})`).
 *
 *  @par Thread-safety
 *  Same guarantees as the underlying `Storage`.
 */
template <typename Storage, typename Boundary = OpenBoundary, std::size_t Halo = 1>
class Ghost_Lattice
{
  static_assert(Halo >= 1, "Ghost_Lattice requires Halo >= 1");

public:
  using storage_type = Storage;
  using boundary_type = Boundary;
  using state_type = typename Storage::state_type;
  using coord_type = typename Storage::coord_type;
  using extents_type = typename Storage::extents_type;

  /// Number of axes (same as the underlying storage).
  static constexpr std::size_t rank = Storage::rank;
  /// Halo thickness (ghost cells per side per axis).
  static constexpr std::size_t halo_v = Halo;

private:
  extents_type user_ext_{};   ///< extents seen from the outside
  extents_type store_ext_{};  ///< extents of the backing storage (`user + 2H`)
  Storage store_{};           ///< underlying storage (allocated once)

  /// Convert a user coordinate into a store coordinate.
  [[nodiscard]] coord_type to_store(const coord_type &c) const noexcept
  {
    coord_type s = c;
    for (std::size_t d = 0; d < rank; ++d)
      s[d] = c[d] + static_cast<ca_index_t>(Halo);
    return s;
  }

  /// Value returned when `at_safe` sees a coordinate outside the halo.
  [[nodiscard]] state_type out_of_range_value() const
  {
    if constexpr (ca_lattice_detail::is_constant_boundary_v<Boundary>)
      return static_cast<state_type>(Boundary::value);
    else
      return state_type{};
  }

  /// Resolve a user coordinate through the boundary policy (slow path).
  /// This is only used by `at_safe` when the coordinate steps outside
  /// the halo — i.e. when the neighbourhood radius exceeds `Halo`.
  [[nodiscard]] state_type resolve_via_policy(coord_type c) const
  {
    using namespace ca_ghost_detail;
    for (std::size_t d = 0; d < rank; ++d)
      {
        const ca_size_t n = user_ext_[d];
        if (n == 0)
          return out_of_range_value();

        if constexpr (std::is_same_v<Boundary, ToroidalBoundary>)
          {
            c[d] = static_cast<ca_index_t>(positive_mod(c[d], n));
          }
        else if constexpr (std::is_same_v<Boundary, ReflectiveBoundary>)
          {
            c[d] = static_cast<ca_index_t>(reflect_index(c[d], n));
          }
        else if constexpr (std::is_same_v<Boundary, NeumannBoundary>)
          {
            c[d] = static_cast<ca_index_t>(clamp_index(c[d], n));
          }
        else
          {
            if (c[d] < 0 or static_cast<ca_size_t>(c[d]) >= n)
              return out_of_range_value();
          }
      }
    return store_.at(to_store(c));
  }

  // -----------------------------------------------------------------
  // Halo fillers (one per boundary policy). Each walks the halo cells
  // and writes the right value into the backing storage. They all use
  // the same helper `for_each_halo_cell` because the structure of the
  // halo is identical across policies.
  // -----------------------------------------------------------------

  /// Invoke `f(user_coord)` on every halo cell of the storage.
  ///
  /// `user_coord` uses the same convention as `at(c)`: axes are in
  /// the user frame, so values in `[-Halo, 0)` or `[user_ext[d], user_ext[d] + Halo)`
  /// mean "halo cell on that side".
  template <typename F>
  void for_each_halo_cell(F &&f) const
  {
    coord_type u{};
    const ca_index_t h = static_cast<ca_index_t>(Halo);

    // Total cells in the enlarged box, minus the interior.
    auto enumerate = [&](auto &&self_fn, std::size_t d) -> void
    {
      if (d == rank)
        {
          // Determine whether any dim is in halo range.
          bool halo_cell = false;
          for (std::size_t k = 0; k < rank; ++k)
            {
              const ca_index_t n = static_cast<ca_index_t>(user_ext_[k]);
              if (u[k] < 0 or u[k] >= n)
                {
                  halo_cell = true;
                  break;
                }
            }
          if (halo_cell)
            f(u);
          return;
        }
      const ca_index_t n = static_cast<ca_index_t>(user_ext_[d]);
      for (ca_index_t i = -h; i < n + h; ++i)
        {
          u[d] = i;
          self_fn(self_fn, d + 1);
        }
    };
    enumerate(enumerate, 0);
  }

  /// Write `value` directly into the store at the given user coord.
  /// No bounds-check (the caller is trusted).
  void store_set(const coord_type &user_c, const state_type &value)
  {
    store_.set(to_store(user_c), value);
  }

  /// Read directly from the store at the given user coord.
  [[nodiscard]] state_type store_get(const coord_type &user_c) const
  {
    return store_.at(to_store(user_c));
  }

public:
  /// Construct an empty ghost lattice (no cells allocated).
  Ghost_Lattice() = default;

  /** @brief Construct a ghost lattice with given user extents and init value.
   *
   *  Allocates `prod(user_ext + 2*Halo)` cells and initialises every
   *  interior and halo cell with `init`. `refresh_halo()` is *not*
   *  called automatically, since the initial state of a CA is a pure
   *  user responsibility.
   *
   *  @param[in] user_extents per-axis user-visible sizes.
   *  @param[in] init initial value for every cell (default `state_type{}`).
   *  @throws std::bad_alloc on allocation failure.
   */
  explicit Ghost_Lattice(const extents_type &user_extents,
                         const state_type &init = state_type{})
    : user_ext_(user_extents)
    , store_ext_(ca_ghost_detail::enlarge_extents<rank>(user_extents, Halo))
    , store_(store_ext_, init)
  {}

  // ------------------------------------------------------------------
  // LatticeLike surface (see `tpl_ca_concepts.H`).
  // ------------------------------------------------------------------

  /// @return the compile-time lattice dimension.
  [[nodiscard]] static constexpr std::size_t dimension() noexcept { return rank; }

  /// @return total user-visible cell count (excludes the halo).
  [[nodiscard]] ca_size_t size() const noexcept
  {
    ca_size_t p = 1;
    for (std::size_t d = 0; d < rank; ++d)
      p *= user_ext_[d];
    return p;
  }

  /** @brief User-visible extent along axis `d`.
   *  @param[in] d axis index.
   *  @throws std::out_of_range if `d >= rank`.
   */
  [[nodiscard]] ca_size_t size(std::size_t d) const
  {
    ah_out_of_range_error_if(d >= rank)
      << "Ghost_Lattice::size: axis " << d << " out of range";
    return user_ext_[d];
  }

  /// @return the per-axis user extents.
  [[nodiscard]] const extents_type &extents() const noexcept { return user_ext_; }

  /// @return the per-axis storage extents (user + `2*Halo` on every axis).
  [[nodiscard]] const extents_type &store_extents() const noexcept { return store_ext_; }

  /// @return the halo thickness.
  [[nodiscard]] static constexpr std::size_t halo_radius() noexcept { return Halo; }

  /** @brief Strict read through a user coordinate.
   *  @param[in] c user coordinate in `[0, size(d))`.
   *  @throws std::out_of_range if any component of `c` is out of range.
   */
  [[nodiscard]] state_type at(const coord_type &c) const
  {
    for (std::size_t d = 0; d < rank; ++d)
      ah_out_of_range_error_if(c[d] < 0 or static_cast<ca_size_t>(c[d]) >= user_ext_[d])
        << "Ghost_Lattice::at: coord[" << d << "]=" << c[d] << " out of [0, "
        << user_ext_[d] << ")";
    return store_get(c);
  }

  /** @brief Strict write through a user coordinate.
   *  @param[in] c user coordinate in `[0, size(d))`.
   *  @param[in] v value to write.
   *  @throws std::out_of_range if any component of `c` is out of range.
   */
  void set(const coord_type &c, const state_type &v)
  {
    for (std::size_t d = 0; d < rank; ++d)
      ah_out_of_range_error_if(c[d] < 0 or static_cast<ca_size_t>(c[d]) >= user_ext_[d])
        << "Ghost_Lattice::set: coord[" << d << "]=" << c[d] << " out of [0, "
        << user_ext_[d] << ")";
    store_set(c, v);
  }

  /** @brief Boundary-aware read exploiting the halo as a fast path.
   *
   *  For a coordinate inside `[-Halo, size(d) + Halo)` on every axis
   *  this method performs a single direct read from the backing
   *  storage — no branch on the boundary policy, so interior steps
   *  are branch-free as long as the neighbourhood radius is
   *  `<= Halo`. For coordinates further out the slow path applies
   *  the policy on the fly so the function still honours the
   *  declared semantics.
   *
   *  @param[in] c possibly out-of-range user coordinate.
   *  @return the cell value resolved through the halo/policy pipeline.
   */
  [[nodiscard]] state_type at_safe(const coord_type &c) const
  {
    const ca_index_t h = static_cast<ca_index_t>(Halo);
    bool in_halo_range = true;
    for (std::size_t d = 0; d < rank; ++d)
      {
        const ca_index_t n = static_cast<ca_index_t>(user_ext_[d]);
        if (c[d] < -h or c[d] >= n + h)
          {
            in_halo_range = false;
            break;
          }
      }
    if (in_halo_range)
      return store_get(c);
    return resolve_via_policy(c);
  }

  // ------------------------------------------------------------------
  // Halo refresh — the Phase 4 raison d'être.
  // ------------------------------------------------------------------

  /** @brief Populate every ghost cell according to the boundary policy.
   *
   *  The engine calls this automatically before each `step()` so the
   *  rule loop can read neighbour values without any boundary
   *  branch. Calling it manually is harmless (idempotent under every
   *  policy implemented here).
   *
   *  @par Complexity
   *  Linear in the number of halo cells (`O(surface_area * Halo)`).
   */
  void refresh_halo()
  {
    if constexpr (std::is_same_v<Boundary, OpenBoundary>)
      {
        for_each_halo_cell([&](const coord_type &u) { store_set(u, state_type{}); });
      }
    else if constexpr (ca_lattice_detail::is_constant_boundary_v<Boundary>)
      {
        const state_type v = static_cast<state_type>(Boundary::value);
        for_each_halo_cell([&](const coord_type &u) { store_set(u, v); });
      }
    else if constexpr (std::is_same_v<Boundary, NeumannBoundary>)
      {
        for_each_halo_cell([&](const coord_type &u)
        {
          coord_type src = u;
          for (std::size_t d = 0; d < rank; ++d)
            src[d] = static_cast<ca_index_t>(
              ca_ghost_detail::clamp_index(u[d], user_ext_[d]));
          store_set(u, store_get(src));
        });
      }
    else if constexpr (std::is_same_v<Boundary, ToroidalBoundary>)
      {
        for_each_halo_cell([&](const coord_type &u)
        {
          coord_type src = u;
          for (std::size_t d = 0; d < rank; ++d)
            src[d] = static_cast<ca_index_t>(
              ca_ghost_detail::positive_mod(u[d], user_ext_[d]));
          store_set(u, store_get(src));
        });
      }
    else if constexpr (std::is_same_v<Boundary, ReflectiveBoundary>)
      {
        for_each_halo_cell([&](const coord_type &u)
        {
          coord_type src = u;
          for (std::size_t d = 0; d < rank; ++d)
            src[d] = static_cast<ca_index_t>(
              ca_ghost_detail::reflect_index(u[d], user_ext_[d]));
          store_set(u, store_get(src));
        });
      }
    else
      {
        static_assert(std::is_same_v<Boundary, OpenBoundary>
                        or std::is_same_v<Boundary, ToroidalBoundary>
                        or std::is_same_v<Boundary, ReflectiveBoundary>
                        or std::is_same_v<Boundary, NeumannBoundary>
                        or ca_lattice_detail::is_constant_boundary_v<Boundary>,
                      "Ghost_Lattice: unsupported Boundary policy");
      }
  }

  // ------------------------------------------------------------------
  // Utilities shared with `Lattice`.
  // ------------------------------------------------------------------

  /// Direct access to the underlying storage (read-only).
  [[nodiscard]] const Storage &storage() const noexcept { return store_; }

  /// Direct access to the underlying storage.
  [[nodiscard]] Storage &storage() noexcept { return store_; }

  /// Set every user-visible cell to `value`. The halo is left
  /// untouched; call `refresh_halo()` afterwards if you need it
  /// synchronised with the new interior.
  void fill(const state_type &value)
  {
    coord_type u{};
    auto go = [&](auto &&self_fn, std::size_t d) -> void
    {
      if (d == rank)
        {
          store_set(u, value);
          return;
        }
      const ca_index_t n = static_cast<ca_index_t>(user_ext_[d]);
      for (ca_index_t i = 0; i < n; ++i)
        {
          u[d] = i;
          self_fn(self_fn, d + 1);
        }
    };
    go(go, 0);
  }

  /// O(1) swap with another ghost lattice of the same type.
  void swap(Ghost_Lattice &other) noexcept(noexcept(store_.swap(other.store_)))
  {
    using std::swap;
    swap(user_ext_, other.user_ext_);
    swap(store_ext_, other.store_ext_);
    store_.swap(other.store_);
  }
};

/** @brief Free-function swap so `Ghost_Lattice` works with `std::swap`.
 *  @param[in,out] a first lattice.
 *  @param[in,out] b second lattice.
 *  @tparam Storage backing storage type.
 *  @tparam Boundary boundary tag.
 *  @tparam Halo halo thickness.
 */
template <typename Storage, typename Boundary, std::size_t Halo>
inline void swap(Ghost_Lattice<Storage, Boundary, Halo> &a,
                 Ghost_Lattice<Storage, Boundary, Halo> &b) noexcept(noexcept(a.swap(b)))
{
  a.swap(b);
}

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_GHOST_LATTICE_H
