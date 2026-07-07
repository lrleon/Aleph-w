
/*
                          Aleph_w

  Data structures & Algorithms
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

/** @file ah-mdspan.H
 *  @brief Non-owning multidimensional array view (`Aleph::mdspan`).
 *
 *  `Aleph::mdspan<T, Extents, Layout, Accessor>` is a lightweight,
 *  non-owning "view" over a contiguous buffer the caller already owns
 *  (a `T*`, `Aleph::Array<T>::data()`, `std::vector<T>::data()`, a
 *  `CA::Dense_Cell_Storage<T,N>::view()`, ...), exposing it as a
 *  multidimensional grid indexable via `operator()(i, j, ...)`. It never
 *  allocates, copies, or owns anything — it is a pointer plus a shape.
 *
 *  `Aleph::mdspan`'s element access is exclusively `operator()(i, j, ...)`
 *  plus a bounds-checked `.at(i, j, ...)` — **not** `std::mdspan`'s own
 *  `operator[](i, j, ...)` (a C++23 *language* feature, multi-argument
 *  subscript, which cannot be backported to C++20; Aleph's baseline is
 *  C++20, GCC 11+/Clang 14+). This surface is identical regardless of the
 *  toolchain:
 *  - When the toolchain provides C++23's `<mdspan>` (`ALEPH_HAS_STD_MDSPAN`,
 *    see `ah-cpp-compat.H`), `Aleph::mdspan<T, Extents, Layout, Accessor>`
 *    is a thin wrapper *around* a `std::mdspan` (still zero-copy — the
 *    wrapper only holds the `std::mdspan` by value, itself just a
 *    pointer/mapping/accessor triple) that forwards everything except
 *    element access, and implements `operator()`/`.at()` on top of the
 *    real `std::mdspan::operator[]`. `Aleph::extents`, `Aleph::dextents`,
 *    `Aleph::layout_right`, `Aleph::layout_left` and
 *    `Aleph::default_accessor` remain plain aliases of their `std::`
 *    counterparts — only the class with the element-access surface needs
 *    wrapping.
 *  - Otherwise this header provides a native **C++20** implementation
 *    covering the same surface (construction, `extents()`/`extent(r)`/
 *    `rank()`/`size()`, `operator()`, `.at()`, `data_handle()`,
 *    `layout_right`/`layout_left`). It is not a bit-for-bit
 *    reimplementation of the standard: in particular, the polyfill's
 *    `extents<IndexType, Exts...>` stores every extent at runtime rather
 *    than folding compile-time (`static_extent`) values out of the
 *    object's storage — a deliberate simplification that costs nothing
 *    for the fully-dynamic `dextents` case this library actually uses
 *    (e.g. `CA::Dense_Cell_Storage`), and only forgoes a size
 *    optimization for mixed static/dynamic extents, which Aleph does not
 *    currently need.
 *
 *  @par Why a view, not a container
 *  `mdspan` never allocates. It is the multidimensional-shape counterpart
 *  of `std::span`: wrap an existing buffer to get readable `m(i, j)`
 *  indexing and shape introspection, with zero copies and zero overhead
 *  beyond the index-to-offset arithmetic itself.
 *
 *  @par Bounds checking
 *  `operator()` matches `std::mdspan`'s contract: out-of-range indices are
 *  undefined behavior (this is a zero-overhead view, not a checked
 *  container). Use `.at(i, j, ...)` for a bounds-checked access that
 *  throws `std::out_of_range` via `ah_out_of_range_error_if`, matching
 *  the `get_curr()`/`get_curr_ne()`-style checked/unchecked pairing used
 *  elsewhere in Aleph.
 *
 *  @par Example
 *  @code
 *  std::vector<double> buf(6, 0.0);
 *  Aleph::mdspan<double, Aleph::dextents<size_t, 2>> m(buf.data(), 2, 3);
 *  m(0, 0) = 1.0; m(1, 2) = 5.0;
 *  for (size_t i = 0; i < m.extent(0); ++i)
 *    for (size_t j = 0; j < m.extent(1); ++j)
 *      std::cout << m(i, j) << (j + 1 == m.extent(1) ? '\n' : ' ');
 *  @endcode
 *
 *  @see CA::Dense_Cell_Storage::view() in `tpl_ca_storage.H` for the
 *       cellular-automata lattice application.
 *  @see Examples/mdspan_matrix_view_example.cc for the matrix-view case.
 *  @ingroup Utilities
 *  @author Leandro Rabindranath Leon
 */

#ifndef AH_MDSPAN_H
#define AH_MDSPAN_H

#include <array>
#include <cstddef>
#include <type_traits>
#include <utility>

#include <ah-cpp-compat.H>
#include <ah-errors.H>

#if ALEPH_HAS_STD_MDSPAN
#  include <mdspan>
#endif

namespace Aleph
{

#if ALEPH_HAS_STD_MDSPAN

inline constexpr size_t dynamic_extent = std::dynamic_extent;

template <class IndexType, size_t... Exts>
using extents = std::extents<IndexType, Exts...>;

template <class IndexType, size_t Rank>
using dextents = std::dextents<IndexType, Rank>;

using layout_right = std::layout_right;
using layout_left = std::layout_left;

template <class ElementType>
using default_accessor = std::default_accessor<ElementType>;

/** @brief Aleph's stable view surface, wrapping `std::mdspan`.
 *
 *  `std::mdspan`'s own element access is `operator[](i, j, ...)` — a
 *  C++23 *language* feature (multi-argument subscript) — and it has no
 *  bounds-checked accessor at all. A bare `using mdspan = std::mdspan<...>`
 *  alias would therefore silently break every Aleph caller written
 *  against `operator()(i, j, ...)`/`.at(i, j, ...)` (this header's own
 *  documented surface, used throughout Aleph — see
 *  `CA::Dense_Cell_Storage::view()`) the moment a toolchain sets
 *  `ALEPH_HAS_STD_MDSPAN=1`. Wrapping `std::mdspan` instead keeps that
 *  surface identical on both paths, at the cost of one extra forwarding
 *  layer around the real `std::mdspan` (still zero-copy: this class
 *  holds a `std::mdspan` by value, which is itself just a pointer plus
 *  a mapping plus an accessor).
 */
template <class T, class Extents, class LayoutPolicy = std::layout_right,
          class AccessorPolicy = std::default_accessor<T>>
/**
 *  @par Thread-safety
 *  Concurrent reads on different cells are safe. Concurrent writes
 *  to overlapping cells are not synchronised.
 */
class mdspan
{
  using impl_type = std::mdspan<T, Extents, LayoutPolicy, AccessorPolicy>;

  impl_type impl_{};

  template <class, class, class, class>
  friend class mdspan;

public:
  using extents_type = typename impl_type::extents_type;
  using layout_type = typename impl_type::layout_type;
  using accessor_type = typename impl_type::accessor_type;
  using mapping_type = typename impl_type::mapping_type;
  using element_type = typename impl_type::element_type;
  using value_type = typename impl_type::value_type;
  using index_type = typename impl_type::index_type;
  using rank_type = typename impl_type::rank_type;
  using data_handle_type = typename impl_type::data_handle_type;
  using reference = typename impl_type::reference;

  constexpr mdspan() noexcept = default;
  constexpr mdspan(const mdspan &) noexcept = default;
  constexpr mdspan(mdspan &&) noexcept = default;
  constexpr mdspan &operator=(const mdspan &) noexcept = default;
  constexpr mdspan &operator=(mdspan &&) noexcept = default;

  /** @brief View `p` through the shape `e`.
   *  @param p pointer to the first element of the buffer to view.
   *  @param e extents object defining the shape.
   */
  constexpr mdspan(data_handle_type p, const extents_type &e) noexcept
    : impl_(p, e)
  {}

  /// View `p` through an already-built mapping.
  constexpr mdspan(data_handle_type p, const mapping_type &m) noexcept
    : impl_(p, m)
  {}

  /** @brief View `p` through mapping `m`, reading via accessor `a`.
   *  @param p pointer to the first element of the buffer to view.
   *  @param m mapping object defining the layout.
   *  @param a accessor object defining element access.
   */
  constexpr mdspan(data_handle_type p, const mapping_type &m, const accessor_type &a) noexcept
    : impl_(p, m, a)
  {}

  /** @brief View `p` as a grid with the given per-axis sizes.
   *  @param p pointer to the first element of the buffer to view.
   *  @param exts one size per **dynamic** axis of `Extents`, in rank order.
   */
  template <class... IndexTypes>
    requires(sizeof...(IndexTypes) >= 1 and
             (std::is_convertible_v<IndexTypes, index_type> and ...))
  constexpr explicit mdspan(data_handle_type p, IndexTypes... exts) noexcept
    : impl_(p, static_cast<index_type>(exts)...)
  {}

  /// View `p` as a grid whose dynamic axes are packed in an array.
  template <class OtherIndexType>
    requires std::is_convertible_v<const OtherIndexType &, index_type>
  constexpr mdspan(data_handle_type p,
                    const std::array<OtherIndexType, extents_type::rank_dynamic()> &dyn) noexcept
    : impl_(p, dyn)
  {}

  /// Converting constructor: `mdspan<T,...>` -> `mdspan<const T,...>`.
  template <class OtherT, class OtherAccessor>
    requires(not std::is_same_v<OtherT, T> and std::is_convertible_v<OtherT (*)[], T (*)[]>)
  constexpr mdspan(const mdspan<OtherT, Extents, LayoutPolicy, OtherAccessor> &other) noexcept
    : impl_(other.impl_)
  {}

  [[nodiscard]] static constexpr rank_type rank() noexcept { return impl_type::rank(); }
  [[nodiscard]] constexpr index_type extent(rank_type r) const noexcept { return impl_.extent(r); }
  [[nodiscard]] constexpr const extents_type &extents() const noexcept { return impl_.extents(); }
  [[nodiscard]] constexpr const mapping_type &mapping() const noexcept { return impl_.mapping(); }
  [[nodiscard]] constexpr const accessor_type &accessor() const noexcept { return impl_.accessor(); }
  [[nodiscard]] constexpr data_handle_type data_handle() const noexcept { return impl_.data_handle(); }
  [[nodiscard]] constexpr data_handle_type data() const noexcept { return impl_.data_handle(); }
  [[nodiscard]] constexpr index_type size() const noexcept { return impl_.size(); }
  [[nodiscard]] constexpr bool empty() const noexcept { return impl_.empty(); }

  /** @brief Unchecked element access.
   *  @param idxs one index per axis, in rank order.
   *  @return A reference to the accessed element.
   *  @warning Undefined behavior if any `idxs[r]` is outside
   *           `[0, extent(r))`. Use `at()` for a bounds-checked variant.
   */
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank())
  [[nodiscard]] constexpr reference operator()(Indices... idxs) const
  {
    return impl_[static_cast<index_type>(idxs)...];
  }

  /** @brief Bounds-checked element access.
   *  @param idxs one index per axis, in rank order.
   *  @return A reference to the accessed element.
   *  @throws std::out_of_range if any `idxs[r]` is outside `[0, extent(r))`.
   */
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank())
  [[nodiscard]] constexpr reference at(Indices... idxs) const
  {
    const index_type idx[]{static_cast<index_type>(idxs)...};
    for (rank_type r = 0; r < rank(); ++r)
      ah_out_of_range_error_if(std::cmp_less(idx[r], 0) or idx[r] >= extent(r))
        << "Aleph::mdspan::at(): index[" << r << "]=" << idx[r] << " out of [0, "
        << extent(r) << ")";
    return impl_[static_cast<index_type>(idxs)...];
  }
};

#else  // !ALEPH_HAS_STD_MDSPAN: native C++20 polyfill

/// Sentinel marking an axis whose extent is only known at run time.
inline constexpr size_t dynamic_extent = static_cast<size_t>(-1);

/// @cond INTERNAL
namespace mdspan_detail
{
template <size_t... Exts>
inline constexpr size_t count_dynamic = (size_t{0} + ... + (Exts == dynamic_extent ? 1 : 0));
}  // namespace mdspan_detail
/// @endcond

/** @brief Shape (per-axis sizes) of an `mdspan`, mixing compile-time and
 *         run-time extents.
 *
 *  @tparam IndexType signed or unsigned integral type used for indices.
 *  @tparam Exts one size per axis: either a fixed value known at compile
 *          time, or `Aleph::dynamic_extent` for an axis whose size is
 *          only known at construction time. `rank()` is `sizeof...(Exts)`.
 *
 *  @note Simplification vs. `std::extents`: every axis (static or
 *        dynamic) is stored at run time in this polyfill, so the object
 *        does not shrink when axes are marked static. This has no effect
 *        on any currently-supported Aleph use case, which always uses
 *        `dextents` (fully dynamic).
 */
template <class IndexType, size_t... Exts>
class extents
{
  static_assert(std::is_integral_v<IndexType>,
                "Aleph::extents: IndexType must be an integral type");

  static constexpr std::array<size_t, sizeof...(Exts)> Static{Exts...};

  std::array<IndexType, sizeof...(Exts)> values_{};

public:
  using index_type = IndexType;
  using size_type = std::make_unsigned_t<IndexType>;
  using rank_type = size_t;

  /// Number of axes (compile-time constant).
  static constexpr rank_type rank() noexcept { return sizeof...(Exts); }

  /// Number of axes whose extent is `dynamic_extent` (compile-time constant).
  static constexpr rank_type rank_dynamic() noexcept
  {
    return mdspan_detail::count_dynamic<Exts...>;
  }

  /// Compile-time size of axis `r`, or `dynamic_extent` if axis `r` is dynamic.
  static constexpr size_t static_extent(rank_type r) noexcept { return Static[r]; }

  /// All-static-or-default-initialised extents (dynamic axes start at 0).
  constexpr extents() noexcept
  {
    for (rank_type r = 0; r < rank(); ++r)
      values_[r] = Static[r] == dynamic_extent ? IndexType{0} : static_cast<IndexType>(Static[r]);
  }

  /** @brief Construct providing a value for each dynamic axis, in rank order.
   *
   *  @param dyn one value per axis for which `static_extent(r) ==
   *         dynamic_extent`, in increasing rank order. Static axes need no
   *         argument; their compile-time value is used unconditionally.
   */
  template <class... IndexTypes>
    requires(sizeof...(IndexTypes) == mdspan_detail::count_dynamic<Exts...> and
             (std::is_convertible_v<IndexTypes, IndexType> and ...))
  constexpr explicit extents(IndexTypes... dyn) noexcept
  {
    const std::array<IndexType, sizeof...(IndexTypes)> given{static_cast<IndexType>(dyn)...};
    rank_type k = 0;
    for (rank_type r = 0; r < rank(); ++r)
      values_[r] = Static[r] == dynamic_extent ? given[k++] : static_cast<IndexType>(Static[r]);
  }

  /** @brief Construct providing every dynamic axis's value packed in an array.
   *
   *  @param dyn one value per dynamic axis, in rank order; `dyn.size()`
   *         must equal `rank_dynamic()`.
   */
  template <class OtherIndexType>
    requires std::is_convertible_v<const OtherIndexType &, IndexType>
  constexpr explicit extents(
    const std::array<OtherIndexType, mdspan_detail::count_dynamic<Exts...>> &dyn) noexcept
  {
    rank_type k = 0;
    for (rank_type r = 0; r < rank(); ++r)
      values_[r] = Static[r] == dynamic_extent ? static_cast<IndexType>(dyn[k++])
                                                : static_cast<IndexType>(Static[r]);
  }

  /** @brief Run-time size of axis `r`.
   *  @param r The axis to query.
   *  @return The extent of the specified axis.
   */
  [[nodiscard]] constexpr index_type extent(rank_type r) const noexcept { return values_[r]; }

  [[nodiscard]] friend constexpr bool operator==(const extents &a, const extents &b) noexcept
  {
    return a.values_ == b.values_;
  }
};

/// @cond INTERNAL
namespace mdspan_detail
{
/// Expands an index sequence into an all-dynamic extents type.
template <class IndexType, size_t... Is>
struct dextents_helper
{
  using type = extents<IndexType, ((void) Is, dynamic_extent)...>;
};

/// Expands an index_sequence into dextents_helper without evaluating code.
///
/// Declared, never defined: used only inside decltype() below to expand an
/// index_sequence<Is...> into dextents_helper<IndexType, Is...> without
/// resorting to a lambda in an unevaluated operand.
template <class IndexType, size_t... Is>
dextents_helper<IndexType, Is...> dextents_of(std::index_sequence<Is...>);
}  // namespace mdspan_detail
/// @endcond

/// `extents<IndexType, ...>` with all `Rank` axes dynamic — the common case.
template <class IndexType, size_t Rank>
using dextents =
  typename decltype(mdspan_detail::dextents_of<IndexType>(
    std::make_index_sequence<Rank>{}))::type;

/** @brief Row-major layout: the last axis varies fastest (C convention).
 *
 *  Matches `CA::Dense_Cell_Storage`'s own layout and `std::mdspan`'s
 *  default. This is a *layout policy*: a tag type whose nested
 *  `mapping<Extents>` converts a multi-index into a linear offset.
 */
struct layout_right
{
  template <class Extents>
  /// Mapping from row-major multi-indices to linear offsets.
  class mapping
  {
    Extents exts_{};

  public:
    using extents_type = Extents;
    using index_type = typename Extents::index_type;
    using rank_type = typename Extents::rank_type;
    using layout_type = layout_right;

    constexpr mapping() noexcept = default;
    constexpr explicit mapping(const Extents &e) noexcept : exts_(e) {}

    [[nodiscard]] constexpr const extents_type &extents() const noexcept { return exts_; }

    /** @brief Calculate the flat offset for a multi-dimensional index.
     *  @param idxs one index per axis, in rank order.
     *  @return The linear memory offset computed in row-major order.
     *  @note Does not perform bounds checking.
     */
    template <class... Indices>
      requires(sizeof...(Indices) == Extents::rank())
    [[nodiscard]] constexpr index_type operator()(Indices... idxs) const noexcept
    {
      const index_type idx[]{static_cast<index_type>(idxs)...};
      index_type offset = 0;
      for (rank_type r = 0; r < Extents::rank(); ++r)
        offset = offset * exts_.extent(r) + idx[r];
      return offset;
    }

    [[nodiscard]] constexpr index_type required_span_size() const noexcept
    {
      index_type total = 1;
      for (rank_type r = 0; r < Extents::rank(); ++r)
        total *= exts_.extent(r);
      return total;
    }

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
  };
};

/** @brief Column-major layout: the first axis varies fastest (Fortran
 *         convention). Useful to view buffers produced by
 *         column-major-expecting numerical code (e.g. BLAS/LAPACK-style
 *         routines) without transposing any data.
 */
struct layout_left
{
  template <class Extents>
  /// Mapping from column-major multi-indices to linear offsets.
  class mapping
  {
    Extents exts_{};

  public:
    using extents_type = Extents;
    using index_type = typename Extents::index_type;
    using rank_type = typename Extents::rank_type;
    using layout_type = layout_left;

    constexpr mapping() noexcept = default;
    constexpr explicit mapping(const Extents &e) noexcept : exts_(e) {}

    [[nodiscard]] constexpr const extents_type &extents() const noexcept { return exts_; }

    /** @brief Calculate the flat offset for a multi-dimensional index.
     *  @param idxs one index per axis, in rank order.
     *  @return The linear memory offset computed in column-major order.
     *  @note Does not perform bounds checking.
     */
    template <class... Indices>
      requires(sizeof...(Indices) == Extents::rank())
    [[nodiscard]] constexpr index_type operator()(Indices... idxs) const noexcept
    {
      const index_type idx[]{static_cast<index_type>(idxs)...};
      index_type offset = 0;
      for (rank_type ri = 0; ri < Extents::rank(); ++ri)
        {
          const rank_type r = Extents::rank() - 1 - ri;
          offset = offset * exts_.extent(r) + idx[r];
        }
      return offset;
    }

    [[nodiscard]] constexpr index_type required_span_size() const noexcept
    {
      index_type total = 1;
      for (rank_type r = 0; r < Extents::rank(); ++r)
        total *= exts_.extent(r);
      return total;
    }

    static constexpr bool is_always_unique() noexcept { return true; }
    static constexpr bool is_always_exhaustive() noexcept { return true; }
    static constexpr bool is_always_strided() noexcept { return true; }
  };
};

/** @brief Trivial accessor policy: `access(p, i)` is just `p[i]`.
 *
 *  This is the only accessor policy this polyfill provides (matching
 *  `std::default_accessor`); custom accessors (atomic views, checked
 *  views, ...) are out of scope for this header.
 */
template <class ElementType>
struct default_accessor
{
  using offset_policy = default_accessor;
  using element_type = ElementType;
  using reference = ElementType &;
  using data_handle_type = ElementType *;

  constexpr default_accessor() noexcept = default;

  template <class OtherElementType>
    requires std::is_convertible_v<OtherElementType (*)[], element_type (*)[]>
  constexpr default_accessor(default_accessor<OtherElementType>) noexcept
  {}

  /** @brief Access an element from a pointer and an offset.
   *  @param p Pointer to the start of the data.
   *  @param i Linear offset.
   *  @return A reference to the element at the specified offset.
   */
  [[nodiscard]] constexpr reference access(data_handle_type p, size_t i) const noexcept
  {
    return p[i];
  }

  [[nodiscard]] constexpr data_handle_type offset(data_handle_type p, size_t i) const noexcept
  {
    return p + i;
  }
};

/** @brief Non-owning view of a contiguous buffer as a multidimensional grid.
 *
 *  See the file-level documentation for the overview, the rationale for
 *  `operator()` over multi-argument `operator[]`, and the bounds-checking
 *  policy (`operator()` is unchecked, `.at()` is checked).
 *
 *  @tparam T element type (may be `const`-qualified for a read-only view).
 *  @tparam Extents an `Aleph::extents<IndexType, ...>` (typically
 *          `Aleph::dextents<IndexType, Rank>`).
 *  @tparam LayoutPolicy `Aleph::layout_right` (default, row-major) or
 *          `Aleph::layout_left` (column-major).
 *  @tparam AccessorPolicy `Aleph::default_accessor<T>` (default).
 */
template <class T, class Extents, class LayoutPolicy = layout_right,
          class AccessorPolicy = default_accessor<T>>
/**
 *  @par Thread-safety
 *  Concurrent reads on different cells are safe. Concurrent writes
 *  to overlapping cells are not synchronised.
 */
class mdspan
{
public:
  using extents_type = Extents;
  using layout_type = LayoutPolicy;
  using accessor_type = AccessorPolicy;
  using mapping_type = typename layout_type::template mapping<extents_type>;
  using element_type = T;
  using value_type = std::remove_cv_t<T>;
  using index_type = typename extents_type::index_type;
  using rank_type = typename extents_type::rank_type;
  using data_handle_type = typename accessor_type::data_handle_type;
  using reference = typename accessor_type::reference;

private:
  data_handle_type ptr_{};
  mapping_type map_{};
  accessor_type acc_{};

public:
  /// Empty view: `data_handle() == nullptr`, every extent is 0.
  constexpr mdspan() noexcept = default;

  constexpr mdspan(const mdspan &) noexcept = default;
  constexpr mdspan(mdspan &&) noexcept = default;
  constexpr mdspan &operator=(const mdspan &) noexcept = default;
  constexpr mdspan &operator=(mdspan &&) noexcept = default;

  /** @brief Converting constructor: view a non-`const` element type as a
   *         `const`-qualified one (`mdspan<T,...>` -> `mdspan<const T,...>`),
   *         mirroring how `T*` converts to `const T*`.
   *
   *  Only participates when `OtherT` actually converts to `T` (so this
   *  does not become a silent implicit cast between unrelated element
   *  types) and the shapes/layouts/accessors match.
   */
  template <class OtherT, class OtherAccessor>
    requires(not std::is_same_v<OtherT, T> and std::is_convertible_v<OtherT (*)[], T (*)[]>)
  constexpr mdspan(const mdspan<OtherT, Extents, LayoutPolicy, OtherAccessor> &other) noexcept
    : ptr_(other.data_handle()), map_(other.mapping()), acc_(other.accessor())
  {}

  /** @brief View `p` through the shape `e`.
   *  @param p pointer to the first element of the buffer to view.
   *  @param e extents object defining the shape.
   */
  constexpr mdspan(data_handle_type p, const extents_type &e) noexcept
    : ptr_(p), map_(e)
  {}

  /** @brief View `p` through an already-built mapping.
   *  @param p pointer to the first element of the buffer to view.
   *  @param m mapping object defining the layout.
   */
  constexpr mdspan(data_handle_type p, const mapping_type &m) noexcept
    : ptr_(p), map_(m)
  {}

  /** @brief View `p` through mapping `m`, reading via accessor `a`.
   *  @param p pointer to the first element of the buffer to view.
   *  @param m mapping object defining the layout.
   *  @param a accessor object defining element access.
   */
  constexpr mdspan(data_handle_type p, const mapping_type &m, const accessor_type &a) noexcept
    : ptr_(p), map_(m), acc_(a)
  {}

  /** @brief View `p` as a grid with the given per-axis sizes.
   *
   *  Convenience constructor equivalent to `mdspan(p, extents_type(exts...))`.
   *  @param p pointer to the first element of the buffer to view.
   *  @param exts one size per **dynamic** axis of `Extents`, in rank order.
   */
  template <class... IndexTypes>
    requires(sizeof...(IndexTypes) >= 1 and
             (std::is_convertible_v<IndexTypes, index_type> and ...))
  constexpr explicit mdspan(data_handle_type p, IndexTypes... exts) noexcept
    : ptr_(p), map_(extents_type(static_cast<index_type>(exts)...))
  {}

  /** @brief View `p` as a grid whose dynamic axes are packed in an array.
   *
   *  Convenience constructor equivalent to `mdspan(p, extents_type(dyn))`.
   *  Matches `CA::Dense_Cell_Storage::extents_type` (a `std::array` of all
   *  per-axis sizes for a fully-dynamic `dextents` shape), so a lattice's
   *  own extents can be forwarded directly without unpacking them.
   */
  template <class OtherIndexType>
    requires std::is_convertible_v<const OtherIndexType &, index_type>
  constexpr mdspan(data_handle_type p,
                    const std::array<OtherIndexType, extents_type::rank_dynamic()> &dyn) noexcept
    : ptr_(p), map_(extents_type(dyn))
  {}

  /** @brief Number of axes (compile-time constant).
   *  @return The rank of the mdspan.
   */
  [[nodiscard]] static constexpr rank_type rank() noexcept { return extents_type::rank(); }

  /** @brief Run-time size of axis `r`.
   *  @param r The axis to query.
   *  @return The extent of the specified axis.
   */
  [[nodiscard]] constexpr index_type extent(rank_type r) const noexcept
  {
    return map_.extents().extent(r);
  }

  /// The shape of this view.
  [[nodiscard]] constexpr const extents_type &extents() const noexcept { return map_.extents(); }

  /// The index-to-offset mapping (layout + shape) of this view.
  [[nodiscard]] constexpr const mapping_type &mapping() const noexcept { return map_; }

  /// The element-access policy of this view.
  [[nodiscard]] constexpr const accessor_type &accessor() const noexcept { return acc_; }

  /// Underlying pointer to the first element (`nullptr` for a default-built view).
  [[nodiscard]] constexpr data_handle_type data_handle() const noexcept { return ptr_; }

  /// \overload data_handle() — mirrors the common `Array`/`std::vector`-style name.
  [[nodiscard]] constexpr data_handle_type data() const noexcept { return ptr_; }

  /// Total element count: the product of every `extent(r)`.
  [[nodiscard]] constexpr index_type size() const noexcept
  {
    index_type total = 1;
    for (rank_type r = 0; r < rank(); ++r)
      total *= extent(r);
    return total;
  }

  /** @brief Checks if the view has no elements.
   *  @return `true` if any axis has extent 0, `false` otherwise.
   */
  [[nodiscard]] constexpr bool empty() const noexcept
  {
    for (rank_type r = 0; r < rank(); ++r)
      if (extent(r) == 0)
        return true;
    return false;
  }

  /** @brief Unchecked element access.
   *
   *  @param idxs one index per axis, in rank order.
   *  @return A reference to the accessed element.
   *  @warning Undefined behavior if any `idxs[r]` is outside
   *           `[0, extent(r))` — matches `std::mdspan::operator[]`'s
   *           contract exactly. Use `at()` for a bounds-checked variant.
   */
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank())
  [[nodiscard]] constexpr reference operator()(Indices... idxs) const
  {
    return acc_.access(ptr_, map_(static_cast<index_type>(idxs)...));
  }

  /** @brief Bounds-checked element access.
   *
   *  @param idxs one index per axis, in rank order.
   *  @return A reference to the accessed element.
   *  @throws std::out_of_range if any `idxs[r]` is outside `[0, extent(r))`.
   */
  template <class... Indices>
    requires(sizeof...(Indices) == extents_type::rank())
  [[nodiscard]] constexpr reference at(Indices... idxs) const
  {
    const index_type idx[]{static_cast<index_type>(idxs)...};
    for (rank_type r = 0; r < rank(); ++r)
      ah_out_of_range_error_if(std::cmp_less(idx[r], 0) or idx[r] >= extent(r))
        << "Aleph::mdspan::at(): index[" << r << "]=" << idx[r] << " out of [0, "
        << extent(r) << ")";
    return acc_.access(ptr_, map_(static_cast<index_type>(idxs)...));
  }
};

#endif  // ALEPH_HAS_STD_MDSPAN

}  // namespace Aleph

#endif  // AH_MDSPAN_H
