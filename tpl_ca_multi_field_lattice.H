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

/** @file tpl_ca_multi_field_lattice.H
 *  @brief Phase 14 multi-field cellular automata lattice (AoS / SoA).
 *
 *  A multi-field lattice keeps several typed fields per cell that
 *  evolve as a coupled system: temperature + concentration,
 *  prey + predator, susceptible + infected + recovered, and so on.
 *  The same logical lattice is exposed in two interchangeable layouts
 *  controlled by a tag template parameter:
 *
 *    - `Layout_AoS` (Array of Structs): the cell is a
 *      `std::tuple<Fields...>` stored in a single `Dense_Cell_Storage`.
 *      Cache-friendly when every field is read together (small grids,
 *      tightly coupled rules).
 *    - `Layout_SoA` (Struct of Arrays): one independent
 *      `Dense_Cell_Storage<Field, Rank>` per field, wrapped in a
 *      `Lattice` with the chosen boundary policy. Vectorisable, the
 *      natural choice for large grids and rules that touch one field
 *      at a time. Each per-field buffer can also be handed to the
 *      legacy mono-field engines through `field<I>()`.
 *
 *  Both layouts share a uniform field-indexed access surface:
 *
 *      const auto u = lat.template at<0>(coord);
 *      lat.template set<1>(coord, new_v);
 *      const auto safe_w = lat.template at_safe<2>(coord);
 *
 *  The boundary policy is shared across fields. The lattice rank is
 *  fixed at compile time (typical values: 1, 2, 3).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_MULTI_FIELD_LATTICE_H
#define TPL_CA_MULTI_FIELD_LATTICE_H

#include <array>
#include <concepts>
#include <cstddef>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_lattice.H>
#include <tpl_ca_storage.H>

namespace Aleph {
namespace CA {

/** @brief Layout tag: each cell is a `std::tuple<Fields...>` inside a
 *         single contiguous storage. */
struct Layout_AoS
{
};

/** @brief Layout tag: one contiguous storage per field. The natural
 *         choice for large grids and rules that touch one field at a
 *         time. */
struct Layout_SoA
{
};

namespace ca_mf_detail {

/// Index sequence helper.
template <std::size_t... I>
using index_seq = std::index_sequence<I...>;

/// Per-field SoA storage tuple type.
template <std::size_t Rank, typename Boundary, typename... Fields>
using soa_storages = std::tuple<Lattice<Dense_Cell_Storage<Fields, Rank>, Boundary>...>;

/// AoS cell type.
template <typename... Fields>
using aos_cell = std::tuple<Fields...>;

/// AoS lattice type.
template <std::size_t Rank, typename Boundary, typename... Fields>
using aos_lattice = Lattice<Dense_Cell_Storage<aos_cell<Fields...>, Rank>, Boundary>;

}  // namespace ca_mf_detail

/** @brief Multi-field lattice with selectable storage layout.
 *
 *  @tparam Layout   `Layout_AoS` or `Layout_SoA`.
 *  @tparam Boundary boundary policy shared by every field
 *                   (`OpenBoundary`, `ToroidalBoundary`,
 *                   `ReflectiveBoundary`, `NeumannBoundary`,
 *                   `ConstantBoundary<T, V>`).
 *  @tparam Rank     compile-time lattice dimension (1, 2 or 3).
 *  @tparam Fields   per-cell field types. Must satisfy `CellState`.
 */
template <typename Layout, typename Boundary, std::size_t Rank, typename... Fields>
class Multi_Field_Lattice
{
  static_assert(sizeof...(Fields) >= 1, "Multi_Field_Lattice requires at least one field");
  static_assert((CellState<Fields> and ...), "Every field type must satisfy CellState");
  static_assert(std::is_same_v<Layout, Layout_AoS> or std::is_same_v<Layout, Layout_SoA>,
                "Layout must be Layout_AoS or Layout_SoA");

public:
  /// Layout tag (`Layout_AoS` or `Layout_SoA`).
  using layout_type = Layout;
  /// Boundary policy.
  using boundary_type = Boundary;
  /// Coordinate type.
  using coord_type = Coord_Vec<Rank>;
  /// Per-axis extents type.
  using extents_type = std::array<ca_size_t, Rank>;
  /// Number of fields.
  static constexpr std::size_t field_count = sizeof...(Fields);
  /// Lattice dimension.
  static constexpr std::size_t rank = Rank;
  /// Compile-time tuple of field types.
  using field_types = std::tuple<Fields...>;

  /// Field type at index `I`.
  template <std::size_t I>
  using field_type = std::tuple_element_t<I, field_types>;

private:
  // Pick the underlying representation based on the layout tag.
  using soa_storage_type = ca_mf_detail::soa_storages<Rank, Boundary, Fields...>;
  using aos_lattice_type = ca_mf_detail::aos_lattice<Rank, Boundary, Fields...>;

  using storage_type
    = std::conditional_t<std::is_same_v<Layout, Layout_SoA>, soa_storage_type, aos_lattice_type>;

  storage_type store_{};
  extents_type extents_{};

  // ---------------------------------------------------------------------
  // SoA construction helpers
  // ---------------------------------------------------------------------
  template <std::size_t... I>
  static soa_storage_type make_soa(const extents_type &ext, ca_mf_detail::index_seq<I...>)
  {
    return soa_storage_type{Lattice<Dense_Cell_Storage<Fields, Rank>, Boundary>(ext, Fields{})...};
  }

  template <std::size_t... I>
  static soa_storage_type make_soa_init(const extents_type &ext,
                                        const std::tuple<Fields...> &init,
                                        ca_mf_detail::index_seq<I...>)
  {
    return soa_storage_type{
      Lattice<Dense_Cell_Storage<Fields, Rank>, Boundary>(ext, std::get<I>(init))...};
  }

public:
  /** @brief Build an empty multi-field lattice. */
  Multi_Field_Lattice() = default;

  /** @brief Build a multi-field lattice with the given extents.
   *
   *  Every field is value-initialised (default `Field{}`).
   *
   *  @param[in] ext per-axis sizes.
   *  @throws std::bad_alloc on allocation failure.
   */
  explicit Multi_Field_Lattice(const extents_type &ext) : extents_(ext)
  {
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      store_ = make_soa(ext, std::index_sequence_for<Fields...>{});
    else
      store_ = aos_lattice_type(ext, ca_mf_detail::aos_cell<Fields...>{});
  }

  /** @brief Build a multi-field lattice with the given extents and
   *         per-field initial values.
   *
   *  @param[in] ext per-axis sizes.
   *  @param[in] init per-field initial values, one per field, in
   *             declaration order.
   *  @throws std::bad_alloc on allocation failure.
   */
  Multi_Field_Lattice(const extents_type &ext, const std::tuple<Fields...> &init) : extents_(ext)
  {
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      store_ = make_soa_init(ext, init, std::index_sequence_for<Fields...>{});
    else
      store_ = aos_lattice_type(ext, init);
  }

  // ---------------------------------------------------------------------
  // Geometry
  // ---------------------------------------------------------------------

  [[nodiscard]] static constexpr std::size_t dimension() noexcept
  {
    return Rank;
  }

  /// Total cell count (product of extents).
  [[nodiscard]] ca_size_t size() const noexcept
  {
    ca_size_t n = 1;
    for (std::size_t d = 0; d < Rank; ++d)
      n *= extents_[d];
    return n;
  }

  /// Extent along axis `d` (`d < Rank`).
  [[nodiscard]] ca_size_t size(const std::size_t d) const
  {
    ah_out_of_range_error_if(d >= Rank) << "Multi_Field_Lattice::size: axis " << d << " out of range";
    return extents_[d];
  }

  /// Per-axis extents.
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return extents_;
  }

  // ---------------------------------------------------------------------
  // Per-field access (uniform across layouts)
  // ---------------------------------------------------------------------

  /** @brief Strict read access to field `I`.
   *  @tparam I field index in `[0, field_count)`.
   *  @param[in] c cell coordinate.
   *  @return value of field `I` at `c`.
   *  @throws std::out_of_range if `c` is out of range.
   */
  template <std::size_t I>
  [[nodiscard]] field_type<I> at(const coord_type &c) const
  {
    static_assert(I < field_count, "field index out of range");
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      return std::get<I>(store_).at(c);
    else
      return std::get<I>(store_.at(c));
  }

  /** @brief Boundary-aware read access to field `I`.
   *  @tparam I field index.
   *  @param[in] c possibly out-of-range coordinate.
   *  @return field `I` value resolved through the boundary policy.
   */
  template <std::size_t I>
  [[nodiscard]] field_type<I> at_safe(const coord_type &c) const
  {
    static_assert(I < field_count, "field index out of range");
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      return std::get<I>(store_).at_safe(c);
    else
      return std::get<I>(store_.at_safe(c));
  }

  /** @brief Strict write access to field `I`.
   *  @tparam I field index.
   *  @param[in] c cell coordinate.
   *  @param[in] v new value for field `I` at `c`.
   *  @throws std::out_of_range if `c` is out of range.
   */
  template <std::size_t I, typename V>
  void set(const coord_type &c, const V &v)
  {
    static_assert(I < field_count, "field index out of range");
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      {
        std::get<I>(store_).set(c, static_cast<field_type<I>>(v));
      }
    else
      {
        ca_mf_detail::aos_cell<Fields...> cell = store_.at(c);
        std::get<I>(cell) = static_cast<field_type<I>>(v);
        store_.set(c, cell);
      }
  }

  /** @brief Strict write of every field at coordinate `c`.
   *  @param[in] c cell coordinate.
   *  @param[in] cell tuple of new field values.
   *  @throws std::out_of_range if `c` is out of range.
   */
  void set_cell(const coord_type &c, const std::tuple<Fields...> &cell)
  {
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      set_cell_impl(c, cell, std::index_sequence_for<Fields...>{});
    else
      store_.set(c, cell);
  }

  /** @brief Read every field at coordinate `c` as a tuple.
   *  @param[in] c cell coordinate.
   *  @return tuple of field values.
   *  @throws std::out_of_range if `c` is out of range.
   */
  [[nodiscard]] std::tuple<Fields...> at_cell(const coord_type &c) const
  {
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      return at_cell_impl(c, std::index_sequence_for<Fields...>{});
    else
      return store_.at(c);
  }

  // ---------------------------------------------------------------------
  // SoA-only: hand a single-field Lattice<> to the legacy mono-field
  // engines.
  // ---------------------------------------------------------------------

  /** @brief Mutable reference to the underlying single-field lattice
   *         for field `I`.
   *
   *  Available only for the SoA layout: each field owns its own
   *  contiguous buffer wrapped as a regular `Lattice<Storage, Boundary>`
   *  that any mono-field engine accepts.
   *
   *  @tparam I field index.
   *  @return reference to the per-field lattice.
   */
  template <std::size_t I>
  [[nodiscard]] auto &field()
    requires std::is_same_v<Layout, Layout_SoA>
  {
    static_assert(I < field_count, "field index out of range");
    return std::get<I>(store_);
  }

  /** @brief Const reference to the underlying single-field lattice
   *         for field `I` (SoA only).
   *  @tparam I field index.
   */
  template <std::size_t I>
  [[nodiscard]] const auto &field() const
    requires std::is_same_v<Layout, Layout_SoA>
  {
    static_assert(I < field_count, "field index out of range");
    return std::get<I>(store_);
  }

  // ---------------------------------------------------------------------
  // Bulk operations
  // ---------------------------------------------------------------------

  /** @brief Set every cell to the supplied per-field values.
   *  @param[in] init tuple of per-field constant values.
   */
  void fill(const std::tuple<Fields...> &init)
  {
    if constexpr (std::is_same_v<Layout, Layout_SoA>)
      fill_impl(init, std::index_sequence_for<Fields...>{});
    else
      store_.fill(init);
  }

  /// O(1) swap with another lattice of the same type.
  void swap(Multi_Field_Lattice &other) noexcept
  {
    using std::swap;
    swap(store_, other.store_);
    swap(extents_, other.extents_);
  }

  /** @brief Swap a single field's buffer with another lattice.
   *
   *  Available only for SoA: lets the engine perform per-field
   *  double buffering without touching the other fields.
   *
   *  @tparam I field index.
   *  @param[in,out] other other lattice (same type).
   */
  template <std::size_t I>
  void swap_field(Multi_Field_Lattice &other)
    requires std::is_same_v<Layout, Layout_SoA>
  {
    static_assert(I < field_count, "field index out of range");
    using std::swap;
    swap(std::get<I>(store_), std::get<I>(other.store_));
  }

private:
  // SoA helpers ---------------------------------------------------------
  template <std::size_t... I>
  void set_cell_impl(const coord_type &c,
                     const std::tuple<Fields...> &cell,
                     ca_mf_detail::index_seq<I...>)
  {
    ((std::get<I>(store_).set(c, std::get<I>(cell))), ...);
  }

  template <std::size_t... I>
  std::tuple<Fields...> at_cell_impl(const coord_type &c, ca_mf_detail::index_seq<I...>) const
  {
    return std::tuple<Fields...>{std::get<I>(store_).at(c)...};
  }

  template <std::size_t... I>
  void fill_impl(const std::tuple<Fields...> &init, ca_mf_detail::index_seq<I...>)
  {
    ((std::get<I>(store_).fill(std::get<I>(init))), ...);
  }
};

/** @brief Free-function swap so the lattice plays nicely with `std::swap`. */
template <typename Layout, typename Boundary, std::size_t Rank, typename... Fields>
inline void swap(Multi_Field_Lattice<Layout, Boundary, Rank, Fields...> &a,
                 Multi_Field_Lattice<Layout, Boundary, Rank, Fields...> &b) noexcept
{
  a.swap(b);
}

/** @brief Concept: a lattice that exposes the multi-field interface.
 *
 *  Mono-field engines stay compatible with the legacy `LatticeLike`
 *  concept; this concept is used by the new
 *  `Multi_Field_Engine` to detect and process multi-field lattices.
 *
 *  @tparam L candidate lattice type.
 */
template <typename L>
concept MultiFieldLatticeLike = requires {
  typename L::coord_type;
  typename L::extents_type;
  { L::field_count } -> std::convertible_to<std::size_t>;
  { L::rank } -> std::convertible_to<std::size_t>;
} and requires(const L &l, std::size_t d) {
  { l.dimension() } -> std::convertible_to<std::size_t>;
  { l.size(d) } -> std::convertible_to<std::size_t>;
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_MULTI_FIELD_LATTICE_H