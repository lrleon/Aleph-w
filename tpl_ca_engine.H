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

/** @file tpl_ca_engine.H
 *  @brief Synchronous double-buffered engine for cellular automata.
 *
 *  `Synchronous_Engine<Lattice, Rule, Neighborhood, Order>` is the
 *  Phase 3 core of `Aleph::CA`. It owns a `current` / `next` pair of
 *  lattices (with O(1) swap) and applies the rule to every cell in the
 *  order chosen by `Order` (`RowMajor` by default; `Tile<W, H>` for a
 *  cache-friendly 2D schedule).
 *
 *  Design constraints:
 *
 *    - The neighbour values needed by the rule are gathered into a
 *      stack buffer of compile-time size `Neighborhood::size_v`. No
 *      heap allocation happens inside `step()`.
 *    - `frame()` only exposes the `current` buffer; the engine never
 *      lets the caller mistake `next` for `current`.
 *    - The optional pre/post-step hooks are stored as `std::function`
 *      and invoked synchronously around each `step()`. Phase 7 will
 *      replace them with a richer Observer concept.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_ENGINE_H
#define TPL_CA_ENGINE_H

#include <array>
#include <concepts>
#include <cstddef>
#include <functional>
#include <span>
#include <type_traits>
#include <utility>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_neighborhood.H>

namespace Aleph {
namespace CA {

namespace ca_engine_detail {
/// Detect whether `O` is a `Tile<W, H>` instantiation.
template <typename O>
struct is_tile : std::false_type
{
};

template <std::size_t W, std::size_t H>
struct is_tile<Tile<W, H>> : std::true_type
{
};

template <typename O>
inline constexpr bool is_tile_v = is_tile<O>::value;

/// Detect whether a lattice exposes a `refresh_halo()` method (the
/// hallmark of a `Ghost_Lattice` or any other halo-aware variant).
/// Used by the engine to keep the halo synchronised automatically.
template <typename L>
concept HasRefreshHalo = requires(L &l) { l.refresh_halo(); };

/// 1D row-major iteration callback `f(coord)`.
template <typename Coord, typename F>
inline void iterate_1d(ca_size_t n0, F &&f)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    {
      c[0] = i;
      f(c);
    }
}

/// 2D row-major iteration.
template <typename Coord, typename F>
inline void iterate_2d_row_major(ca_size_t n0, ca_size_t n1, F &&f)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
      {
        c[0] = i;
        c[1] = j;
        f(c);
      }
}

/// 2D column-major iteration.
template <typename Coord, typename F>
inline void iterate_2d_column_major(ca_size_t n0, ca_size_t n1, F &&f)
{
  Coord c{};
  for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
    for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
      {
        c[0] = i;
        c[1] = j;
        f(c);
      }
}

/// 2D tiled iteration with tile size `H` along axis 0 and `W` along
/// axis 1. Tiles at the boundary are clipped to the lattice extents.
template <std::size_t W, std::size_t H, typename Coord, typename F>
inline void iterate_2d_tile(const ca_size_t n0, const ca_size_t n1, F &&f)
{
  Coord c{};
  for (ca_size_t ti = 0; ti < n0; ti += H)
    for (ca_size_t tj = 0; tj < n1; tj += W)
      {
        const ca_size_t i_end = ti + H < n0 ? ti + H : n0;
        const ca_size_t j_end = tj + W < n1 ? tj + W : n1;
        for (ca_size_t i = ti; i < i_end; ++i)
          for (ca_size_t j = tj; j < j_end; ++j)
            {
              c[0] = static_cast<ca_index_t>(i);
              c[1] = static_cast<ca_index_t>(j);
              f(c);
            }
      }
}

/// 3D row-major iteration.
template <typename Coord, typename F>
inline void iterate_3d_row_major(const ca_size_t n0, const ca_size_t n1, const ca_size_t n2, F &&f)
{
  Coord c{};
  for (ca_index_t i = 0; i < static_cast<ca_index_t>(n0); ++i)
    for (ca_index_t j = 0; j < static_cast<ca_index_t>(n1); ++j)
      for (ca_index_t k = 0; k < static_cast<ca_index_t>(n2); ++k)
        {
          c[0] = i;
          c[1] = j;
          c[2] = k;
          f(c);
        }
}
}  // namespace ca_engine_detail

/** @brief Synchronous double-buffered engine.
 *
 *  @tparam Lattice      lattice type (e.g. `Lattice<Dense_Cell_Storage<int,2>,
 *                       ToroidalBoundary>`).
 *  @tparam Rule         rule type satisfying `RuleLike<Rule, Lattice>`.
 *  @tparam Neighborhood neighbourhood type satisfying `NeighborhoodLike`.
 *  @tparam Order        iteration order tag (`RowMajor` or `Tile<W, H>`).
 *
 *  @par Complexity
 *  `step()` is `O(N * S)` where `N` is the cell count and `S` the
 *  neighbour count. Buffer swap is O(1).
 *
 *  @par Thread-safety
 *  The engine itself is not thread-safe; engines are intended to be
 *  driven by a single thread. Phase 5 introduces a parallel variant.
 */
template <typename Lattice, typename Rule, typename Neighborhood, typename Order = RowMajor>
class Synchronous_Engine
{
  static_assert(LatticeLike<Lattice>, "Synchronous_Engine requires a LatticeLike lattice");
  static_assert(NeighborhoodLike<Neighborhood>,
                "Synchronous_Engine requires a NeighborhoodLike neighborhood");
  static_assert(RuleLike<Rule, Lattice>, "Synchronous_Engine requires a RuleLike rule");
  static_assert(Neighborhood::rank_v == Lattice::rank, "Neighborhood rank must match lattice rank");

public:
  /// Lattice type.
  using lattice_type = Lattice;
  /// Rule type.
  using rule_type = Rule;
  /// Neighborhood type.
  using neighborhood_type = Neighborhood;
  /// Iteration order.
  using order_type = Order;

  /// Cell state type.
  using state_type = typename Lattice::state_type;
  /// Coordinate type.
  using coord_type = typename Lattice::coord_type;
  /// Extents type.
  using extents_type = typename Lattice::extents_type;

  /// Lattice dimension.
  static constexpr std::size_t rank = Lattice::rank;
  /// Number of neighbours.
  static constexpr std::size_t neighbour_count = Neighborhood::size_v;

  /// Hook signature: `(step_index_just_completed, frame)`.
  using hook_type = std::function<void(std::size_t, const Lattice &)>;

private:
  Lattice cur_buf_;
  Lattice nxt_buf_;
  Rule rule_;
  Neighborhood nh_;
  hook_type pre_hook_;
  hook_type post_hook_;
  std::size_t step_count_ = 0;

public:
  /** @brief Build an engine on top of an existing initial lattice.
   *
   *  The next-step buffer is allocated with the same extents and a
   *  default-constructed cell value.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r       rule instance.
   *  @param[in] n       neighbourhood instance.
   *
   *  @throws std::bad_alloc if allocating the next buffer fails.
   *
   *  @par Complexity
   *  O(N) for allocating the next buffer, where N is the cell count.
   */
  Synchronous_Engine(Lattice initial, Rule r, Neighborhood n)
    : cur_buf_(std::move(initial)), nxt_buf_(cur_buf_.extents()), rule_(std::move(r)), nh_(std::move(n))
  {}

  /** @brief Build an engine with a default-constructed neighbourhood.
   *
   *  This overload is available only when `Neighborhood` is default
   *  constructible.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r       rule instance.
   *
   *  @throws std::bad_alloc if allocating the next buffer fails.
   *
   *  @par Complexity
   *  O(N) for allocating the next buffer, where N is the cell count.
   */
  Synchronous_Engine(Lattice initial, Rule r)
    requires std::default_initializable<Neighborhood>
    : Synchronous_Engine(std::move(initial), std::move(r), Neighborhood{})
  {}

  /** @brief Return the current frame.
   *
   *  @return read-only view of the current lattice buffer.
   */
  [[nodiscard]] const Lattice &frame() const noexcept
  {
    return cur_buf_;
  }

  /** @brief Return the number of completed steps.
   *
   *  @return the number of `step()` invocations performed so far.
   */
  [[nodiscard]] std::size_t steps_run() const noexcept
  {
    return step_count_;
  }

  /** @brief Return the frame extents.
   *
   *  @return the lattice extents forwarded from the current buffer.
   */
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return cur_buf_.extents();
  }

  /** @brief Register a hook fired *before* every `step()`.
   *  The hook receives the step index that is about to start
   *  (`steps_run()`) and a const reference to the current frame.
   */
  template <typename F>
  void on_pre_step(F &&f)
  {
    pre_hook_ = std::forward<F>(f);
  }

  /** @brief Register a hook fired *after* every `step()`.
   *  The hook receives the step index that just completed
   *  (`steps_run()` after the swap) and a const reference to the
   *  resulting frame.
   */
  template <typename F>
  void on_post_step(F &&f)
  {
    post_hook_ = std::forward<F>(f);
  }

  /** @brief Apply the rule to every cell once and swap buffers.
   *
   *  `step()` gathers neighbours into a fixed-size stack buffer,
   *  writes the result into the next buffer, swaps the buffers in O(1),
   *  then fires the optional post-step hook.
   *
   *  @throws Any exception propagated by the rule, lattice or hooks.
   *
   *  @par Complexity
   *  O(N * K), where N is the cell count and K is the neighbourhood size.
   */
  void step()
  {
    using namespace ca_engine_detail;

    // Refresh the ghost layer, if the lattice has one. This is what
    // allows Phase 4 lattices to keep the boundary logic out of the
    // inner loop: by the time `gather_neighbors` runs, every cell
    // inside the halo already carries the correct policy value.
    if constexpr (HasRefreshHalo<Lattice>)
      cur_buf_.refresh_halo();

    if (pre_hook_)
      pre_hook_(step_count_, cur_buf_);

    // Stack buffer for the gathered neighbour values. Size is fixed
    // by the neighbourhood at compile time, so no heap involvement.
    std::array<state_type, neighbour_count> nbuf{};

    auto compute_one = [&](const coord_type &c)
    {
      const std::size_t neigh_sz = std::min(nh_.size(), nbuf.size());
      gather_neighbors(nh_, cur_buf_, c, std::span<state_type>(nbuf.data(), neigh_sz));
      const Cell_Context<rank> ctx{step_count_, c};
      nxt_buf_.set(c,
                   apply_rule(rule_,
                              cur_buf_.at(c),
                              Neighbor_View<state_type>(nbuf.data(), neigh_sz),
                              ctx));
    };

    if constexpr (rank == 1)
      {
        iterate_1d<coord_type>(cur_buf_.size(0), compute_one);
      }
    else if constexpr (rank == 2)
      {
        if constexpr (is_tile_v<Order>)
          iterate_2d_tile<Order::width, Order::height, coord_type>(cur_buf_.size(0),
                                                                   cur_buf_.size(1),
                                                                   compute_one);
        else if constexpr (std::is_same_v<Order, ColumnMajor>)
          iterate_2d_column_major<coord_type>(cur_buf_.size(0), cur_buf_.size(1), compute_one);
        else
          iterate_2d_row_major<coord_type>(cur_buf_.size(0), cur_buf_.size(1), compute_one);
      }
    else if constexpr (rank == 3)
      {
        iterate_3d_row_major<coord_type>(cur_buf_.size(0), cur_buf_.size(1), cur_buf_.size(2), compute_one);
      }
    else
      static_assert(rank <= 3, "Synchronous_Engine currently supports rank <= 3");

    cur_buf_.swap(nxt_buf_);
    ++step_count_;

    if (post_hook_)
      post_hook_(step_count_, cur_buf_);
  }

  /** @brief Run several synchronous steps.
   *
   *  @param[in] steps number of consecutive `step()` calls to perform.
   *
   *  @throws Any exception propagated by `step()`.
   *
   *  @par Complexity
   *  O(steps * N * K), where N is the cell count and K is the
   *  neighbourhood size.
   */
  void run(const std::size_t steps)
  {
    for (std::size_t i = 0; i < steps; ++i)
      step();
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_ENGINE_H
