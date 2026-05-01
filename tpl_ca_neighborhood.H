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


/** @file tpl_ca_neighborhood.H
 *  @brief Neighborhoods catalogue for `Aleph::CA`.
 *
 *  Provides:
 *
 *    - `Moore<N, R>`            — Chebyshev-radius neighborhood ((2R+1)^N - 1).
 *    - `Von_Neumann<N, R>`      — L1-radius neighborhood.
 *    - `Hex_Neighborhood`       — 6 axial-coord neighbours over a 2D lattice.
 *    - `Triangular_Neighborhood`— 3 edge neighbours, parity-dependent.
 *    - `Custom_Neighborhood<N, K, Radius>` — explicit list of offsets supplied at runtime.
 *
 *  Every neighborhood satisfies the `NeighborhoodLike` concept and
 *  publishes a uniform compile-time API:
 *
 *    static constexpr std::size_t rank_v;     // lattice dim
 *    static constexpr std::size_t size_v;     // neighbour count
 *    static constexpr std::size_t radius_v;   // Chebyshev radius
 *    std::size_t radius() const;
 *    std::size_t size()   const;
 *    template <typename F>
 *    void for_each_offset(const Coord_Vec<rank_v> & center, F && f) const;
 *
 *  Plus a free function template:
 *
 *    template <typename Nbh, typename L, typename T>
 *    void gather_neighbors(const Nbh & nh, const L & lat,
 *                          const typename L::coord_type & center,
 *                          std::span<T> out);
 *
 *  which fills the caller-supplied stack buffer with the values returned
 *  by `lat.at_safe(center + offset)` in canonical order. No heap.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

# ifndef TPL_CA_NEIGHBORHOOD_H
# define TPL_CA_NEIGHBORHOOD_H

# include <array>
# include <cstddef>
# include <span>
# include <utility>

# include <ah-errors.H>
# include <ca-traits.H>
# include <tpl_ca_concepts.H>

namespace Aleph
{
namespace CA
{

  namespace ca_neighborhood_detail
  {
    /// Compile-time integer power.
    constexpr std::size_t static_pow(std::size_t base, std::size_t exp) noexcept
    {
      std::size_t r = 1;
      for (std::size_t i = 0; i < exp; ++i) r *= base;
      return r;
    }

    /// Number of integer points in the L1 ball of radius `R` in `N`
    /// dimensions, excluding the origin.
    template <std::size_t N, std::size_t R>
    constexpr std::size_t von_neumann_count() noexcept
    {
      const std::size_t total = static_pow(2 * R + 1, N);
      const ca_index_t  Ri    = static_cast<ca_index_t>(R);

      // Iterate over the Moore box and count points whose L1 norm lies
      // in [1, R]. Compile-time only; tiny N and R in practice.
      std::size_t cnt = 0;
      // current offset, initialised to (-R, -R, ..., -R).
      std::array<std::size_t, N> cur_buf { };
      for (std::size_t d = 0; d < N; ++d)
        cur_buf[d] = 0;  // we'll bias by -R when reading

      for (std::size_t i = 0; i < total; ++i)
        {
          // Convert cur_buf (in [0, 2R]) into signed offset in [-R, R].
          std::size_t l1 = 0;
          for (std::size_t d = 0; d < N; ++d)
            {
              const ca_index_t v =
                static_cast<ca_index_t>(cur_buf[d]) - Ri;
              l1 += static_cast<std::size_t>(v < 0 ? -v : v);
            }
          if (l1 != 0 and l1 <= R)
            ++cnt;

          // Increment cur_buf as a (2R+1)-base counter (last axis fastest).
          for (std::size_t d = N; d-- > 0; )
            {
              if (cur_buf[d] + 1 < 2 * R + 1)
                {
                  ++cur_buf[d];
                  break;
                }
              cur_buf[d] = 0;
            }
        }
      return cnt;
    }

    /// Generate the canonical-ordered list of Moore offsets for `N`/`R`.
    template <std::size_t N, std::size_t R>
    constexpr auto compute_moore_offsets() noexcept
    {
      constexpr std::size_t total = static_pow(2 * R + 1, N);
      constexpr std::size_t out_n = total - 1;
      std::array<Offset_Vec<N>, out_n> result { };

      Offset_Vec<N> cur { };
      for (std::size_t d = 0; d < N; ++d)
        cur[d] = -static_cast<ca_index_t>(R);

      std::size_t idx = 0;
      for (std::size_t i = 0; i < total; ++i)
        {
          bool zero = true;
          for (std::size_t d = 0; d < N; ++d)
            if (cur[d] != 0) { zero = false; break; }
          if (not zero)
            {
              result[idx] = cur;
              ++idx;
            }
          // Increment lex order (last axis fastest).
          for (std::size_t d = N; d-- > 0; )
            {
              if (cur[d] < static_cast<ca_index_t>(R))
                {
                  ++cur[d];
                  break;
                }
              cur[d] = -static_cast<ca_index_t>(R);
            }
        }
      return result;
    }

    /// Generate the canonical-ordered list of Von Neumann offsets for
    /// `N`/`R` (L1 radius). Same iteration as Moore but filtered.
    template <std::size_t N, std::size_t R>
    constexpr auto compute_von_neumann_offsets() noexcept
    {
      constexpr std::size_t total = static_pow(2 * R + 1, N);
      constexpr std::size_t out_n = von_neumann_count<N, R>();
      std::array<Offset_Vec<N>, out_n> result { };

      Offset_Vec<N> cur { };
      for (std::size_t d = 0; d < N; ++d)
        cur[d] = -static_cast<ca_index_t>(R);

      std::size_t idx = 0;
      for (std::size_t i = 0; i < total; ++i)
        {
          std::size_t l1 = 0;
          for (std::size_t d = 0; d < N; ++d)
            l1 += static_cast<std::size_t>(cur[d] < 0 ? -cur[d] : cur[d]);
          if (l1 != 0 and l1 <= R)
            {
              result[idx] = cur;
              ++idx;
            }
          for (std::size_t d = N; d-- > 0; )
            {
              if (cur[d] < static_cast<ca_index_t>(R))
                {
                  ++cur[d];
                  break;
                }
              cur[d] = -static_cast<ca_index_t>(R);
            }
        }
      return result;
    }
  } // namespace ca_neighborhood_detail

  // -----------------------------------------------------------------------
  // Moore neighborhood
  // -----------------------------------------------------------------------

  /** @brief Moore (Chebyshev) neighborhood of radius `R` in `N` dimensions.
   *
   *  Enumerates every offset whose components lie in `[-R, R]`, except
   *  the origin. Total: `(2R+1)^N - 1`. For 2D radius 1 this is the
   *  classic 8-neighbour pattern used by Conway's Game of Life.
   */
  template <std::size_t N, std::size_t R = 1>
  class Moore
  {
    static_assert(N >= 1, "Moore requires N >= 1");
    static_assert(R >= 1, "Moore requires R >= 1");

  public:

    static constexpr std::size_t rank_v   = N;
    static constexpr std::size_t radius_v = R;
    static constexpr std::size_t size_v   =
      ca_neighborhood_detail::static_pow(2 * R + 1, N) - 1;

    /// Compile-time, canonical-ordered offsets.
    static constexpr std::array<Offset_Vec<N>, size_v> offsets =
      ca_neighborhood_detail::compute_moore_offsets<N, R>();

    [[nodiscard]] constexpr std::size_t radius() const noexcept { return R; }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return size_v; }

    template <typename F>
    constexpr void for_each_offset(const Coord_Vec<N> & /*center*/,
                                   F && f) const
    {
      for (std::size_t i = 0; i < size_v; ++i)
        f(offsets[i]);
    }
  };

  // -----------------------------------------------------------------------
  // Von Neumann neighborhood
  // -----------------------------------------------------------------------

  /** @brief Von Neumann (L1) neighborhood of radius `R` in `N` dimensions.
   *
   *  Enumerates every offset whose `|x|_1` lies in `[1, R]`. For 2D
   *  radius 1 this is the classic 4-neighbour cross.
   */
  template <std::size_t N, std::size_t R = 1>
  class Von_Neumann
  {
    static_assert(N >= 1, "Von_Neumann requires N >= 1");
    static_assert(R >= 1, "Von_Neumann requires R >= 1");

  public:

    static constexpr std::size_t rank_v   = N;
    static constexpr std::size_t radius_v = R;
    static constexpr std::size_t size_v   =
      ca_neighborhood_detail::von_neumann_count<N, R>();

    static constexpr std::array<Offset_Vec<N>, size_v> offsets =
      ca_neighborhood_detail::compute_von_neumann_offsets<N, R>();

    [[nodiscard]] constexpr std::size_t radius() const noexcept { return R; }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return size_v; }

    template <typename F>
    constexpr void for_each_offset(const Coord_Vec<N> & /*center*/,
                                   F && f) const
    {
      for (std::size_t i = 0; i < size_v; ++i)
        f(offsets[i]);
    }
  };

  // -----------------------------------------------------------------------
  // Hexagonal neighborhood (axial coordinates)
  // -----------------------------------------------------------------------

  /** @brief Six-neighbour hex pattern in axial coordinates over a 2D lattice.
   *
   *  Axial coordinates encode a hexagonal grid using two integer
   *  axes `(q, r)`. Each cell has six neighbours at offsets:
   *
   *    (+1, 0), (-1, 0), (0, +1), (0, -1), (+1, -1), (-1, +1)
   *
   *  The lattice itself is a regular 2D `Lattice<Storage, Boundary>`;
   *  hexagonal interpretation is purely a function of which offsets
   *  the rule consumes.
   */
  class Hex_Neighborhood
  {
  public:

    static constexpr std::size_t rank_v   = 2;
    static constexpr std::size_t radius_v = 1;
    static constexpr std::size_t size_v   = 6;

    static constexpr std::array<Offset_Vec<2>, 6> offsets = {{
      {  1,  0 }, { -1,  0 }, {  0,  1 },
      {  0, -1 }, {  1, -1 }, { -1,  1 }
    }};

    [[nodiscard]] constexpr std::size_t radius() const noexcept { return 1; }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return 6; }

    template <typename F>
    constexpr void for_each_offset(const Coord_Vec<2> & /*center*/,
                                   F && f) const
    {
      for (const auto & o : offsets) f(o);
    }
  };

  // -----------------------------------------------------------------------
  // Triangular neighborhood
  // -----------------------------------------------------------------------

  /** @brief Three edge-neighbours over a 2D lattice with parity coupling.
   *
   *  Each cell is interpreted as a triangle whose orientation depends
   *  on `(i + j) % 2`. "Up" triangles (even parity) connect to
   *  `(i, j-1)`, `(i, j+1)`, `(i+1, j)`; "down" triangles (odd parity)
   *  to `(i, j-1)`, `(i, j+1)`, `(i-1, j)`. The third offset is
   *  therefore parity-dependent, so this class is the only one whose
   *  offsets cannot be cached as a static array.
   */
  class Triangular_Neighborhood
  {
  public:

    static constexpr std::size_t rank_v   = 2;
    static constexpr std::size_t radius_v = 1;
    static constexpr std::size_t size_v   = 3;

    [[nodiscard]] constexpr std::size_t radius() const noexcept { return 1; }
    [[nodiscard]] constexpr std::size_t size()   const noexcept { return 3; }

    template <typename F>
    constexpr void for_each_offset(const Coord_Vec<2> & center, F && f) const
    {
      // Two parity-independent edge-neighbours along the j axis.
      f(Offset_Vec<2>{ 0, -1 });
      f(Offset_Vec<2>{ 0,  1 });
      // Third edge-neighbour points up or down depending on parity.
      const ca_index_t parity = (center[0] + center[1]) & 1;
      f(parity == 0 ? Offset_Vec<2>{ 1, 0 } : Offset_Vec<2>{ -1, 0 });
    }
  };

  // -----------------------------------------------------------------------
  // Custom neighborhood
  // -----------------------------------------------------------------------

  /** @brief User-supplied list of offsets for arbitrary connectivity.
   *
   *  @tparam N lattice rank.
   *  @tparam K number of offsets (compile-time constant).
   *  @tparam Radius optional compile-time radius; `0` means unknown and
   *                  `radius()` computes the actual Chebyshev radius.
   */
  template <std::size_t N, std::size_t K, std::size_t Radius = 0>
  class Custom_Neighborhood
  {
    static_assert(N >= 1, "Custom_Neighborhood requires N >= 1");
    static_assert(K >= 1, "Custom_Neighborhood requires K >= 1");

    std::array<Offset_Vec<N>, K> offs;

  public:

    static constexpr std::size_t rank_v = N;
    static constexpr std::size_t size_v = K;
    static constexpr std::size_t radius_v = Radius;

    /** @brief Build a custom neighborhood from an `std::array` of offsets.
     *
     *  @param os offsets in the exact order exposed by `for_each_offset()`.
     */
    constexpr explicit Custom_Neighborhood(std::array<Offset_Vec<N>, K> os)
      : offs(os)
    {
    }

    /** @brief Build a custom neighborhood from a braced offset list.
     *
     *  This overload keeps direct-list initialization such as
     *  `Custom_Neighborhood<2, 3>({ Offset_Vec<2>{ ... }, ... })`
     *  unambiguous across GCC and Clang.
     *
     *  @param os offsets in the exact order exposed by `for_each_offset()`.
     */
    constexpr explicit Custom_Neighborhood(const Offset_Vec<N> (&os)[K])
    {
      for (std::size_t i = 0; i < K; ++i)
        offs[i] = os[i];
    }

    /// @return the largest Chebyshev component across the offset list.
    [[nodiscard]] constexpr std::size_t radius() const noexcept
    {
      std::size_t r = 0;
      for (std::size_t k = 0; k < K; ++k)
        for (std::size_t d = 0; d < N; ++d)
          {
            const auto v = offs[k][d];
            const std::size_t a =
              static_cast<std::size_t>(v < 0 ? -v : v);
            if (a > r) r = a;
          }
      return r;
    }

    [[nodiscard]] constexpr std::size_t size() const noexcept { return K; }

    [[nodiscard]] constexpr const std::array<Offset_Vec<N>, K> &
    offsets() const noexcept { return offs; }

    template <typename F>
    constexpr void for_each_offset(const Coord_Vec<N> & /*center*/,
                                   F && f) const
    {
      for (const auto & o : offs) f(o);
    }
  };

  // -----------------------------------------------------------------------
  // gather_neighbors: fills a caller-supplied buffer using at_safe.
  // -----------------------------------------------------------------------

  /** @brief Populate `out[0..nh.size())` with neighbour values of `center`.
   *
   *  The caller owns the buffer (typically a `std::array<T, N>` on the
   *  stack). Order matches `nh.for_each_offset`. Out-of-range neighbours
   *  are resolved by the lattice's boundary policy via `at_safe`.
   *
   *  @param[in]  nh     neighborhood instance.
   *  @param[in]  lat    lattice to read from.
   *  @param[in]  center center coordinate.
   *  @param[out] out    buffer of at least `nh.size()` cells.
   *  @throws std::length_error if `out` is smaller than `nh.size()`.
   */
  template <typename Nbh, typename L, typename T>
  inline void gather_neighbors(const Nbh & nh, const L & lat,
                               const typename L::coord_type & center,
                               std::span<T> out)
  {
    ah_length_error_if(out.size() < nh.size())
      << "gather_neighbors: output span has size " << out.size()
      << ", expected at least " << nh.size();
    using Coord = typename L::coord_type;
    std::size_t i = 0;
    nh.for_each_offset(center, [&](const auto & off)
      {
        Coord c = center;
        for (std::size_t d = 0; d < L::rank; ++d)
          c[d] = static_cast<ca_index_t>(c[d] + off[d]);
        out[i] = static_cast<T>(lat.at_safe(c));
        ++i;
      });
  }

} // namespace CA
} // namespace Aleph

# endif // TPL_CA_NEIGHBORHOOD_H
