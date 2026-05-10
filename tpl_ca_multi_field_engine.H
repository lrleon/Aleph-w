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

/** @file tpl_ca_multi_field_engine.H
 *  @brief Phase 14 synchronous double-buffered engine for multi-field
 *         cellular automata.
 *
 *  `Multi_Field_Engine<Lattice, Rule, Neighborhood>` is the multi-field
 *  twin of `Synchronous_Engine`: it owns a current and a next
 *  `Multi_Field_Lattice` (with O(1) per-field swap when SoA), gathers
 *  per-field neighbour values into stack buffers, calls the
 *  `Multi_Field_Rule`, and writes the resulting tuple of new field
 *  values into the next-buffer. Buffers are swapped at the end of
 *  every step.
 *
 *  Design constraints:
 *
 *    - The multi-field path is fully synchronous and double-buffered
 *      (the simplest, safest update). The reusable update-scheme
 *      strategies of Phase 13 stay on the mono-field path.
 *    - Per-field neighbour buffers live on the stack: their size is
 *      `Neighborhood::size_v`, fixed at compile time. No heap
 *      involvement inside `step()`.
 *    - The rule is invoked via `apply_multi_field_rule`, so contextual
 *      multi-field rules (Phase 8 RNG style) are accepted transparently.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_MULTI_FIELD_ENGINE_H
#define TPL_CA_MULTI_FIELD_ENGINE_H

#include <array>
#include <cstddef>
#include <functional>
#include <span>
#include <tuple>
#include <type_traits>
#include <utility>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_multi_field_lattice.H>
#include <tpl_ca_multi_field_rule.H>
#include <tpl_ca_neighborhood.H>

namespace Aleph {
namespace CA {

namespace ca_mf_engine_detail {

/// Recurse over the Field_Indices and gather one Neighbor_View per field.
template <typename Lattice, typename Neighborhood, typename Buffers, std::size_t... I>
inline auto build_views(const Lattice &lat,
                        const Neighborhood &nh,
                        const typename Lattice::coord_type &center,
                        Buffers &bufs,
                        std::index_sequence<I...>)
{
  using Coord = typename Lattice::coord_type;
  std::size_t neigh_sz = nh.size();
  if (neigh_sz > std::tuple_size_v<std::tuple_element_t<0, Buffers>>)
    neigh_sz = std::tuple_size_v<std::tuple_element_t<0, Buffers>>;

  std::size_t cursor = 0;
  nh.for_each_offset(center, [&](const auto &off)
  {
    Coord c = center;
    for (std::size_t d = 0; d < Lattice::rank; ++d)
      c[d] = static_cast<ca_index_t>(c[d] + off[d]);
    if (cursor < neigh_sz)
      ((std::get<I>(bufs)[cursor] = lat.template at_safe<I>(c)), ...);
    ++cursor;
  });

  return std::tuple{
    Neighbor_View<typename Lattice::template field_type<I>>(std::get<I>(bufs).data(), neigh_sz)...};
}

}  // namespace ca_mf_engine_detail

/** @brief Synchronous double-buffered engine for multi-field lattices.
 *
 *  @tparam Lattice      `Multi_Field_Lattice<...>` instance.
 *  @tparam Rule         multi-field rule (`MultiFieldRuleLike`).
 *  @tparam Neighborhood mono-field neighbourhood applied uniformly to
 *                       every field (`NeighborhoodLike`).
 *
 *  @par Complexity
 *  `step()` is `O(N * S * F)` where `N` is the cell count, `S` the
 *  neighbour count and `F` the number of fields.
 */
template <typename Lattice, typename Rule, typename Neighborhood>
class Multi_Field_Engine
{
  static_assert(NeighborhoodLike<Neighborhood>,
                "Multi_Field_Engine requires a NeighborhoodLike neighbourhood");
  static_assert(Neighborhood::rank_v == Lattice::rank, "Neighbourhood rank must match lattice rank");

public:
  using lattice_type = Lattice;
  using rule_type = Rule;
  using neighborhood_type = Neighborhood;
  using coord_type = typename Lattice::coord_type;
  using extents_type = typename Lattice::extents_type;
  using fields_tuple = typename Lattice::field_types;

  static constexpr std::size_t rank = Lattice::rank;
  static constexpr std::size_t field_count = Lattice::field_count;
  static constexpr std::size_t neighbour_count = Neighborhood::size_v;

  /// Hook signature: `(step_index_just_completed, frame)`.
  using hook_type = std::function<void(std::size_t, const Lattice &)>;

private:
  Lattice cur_;
  Lattice nxt_;
  Rule rule_;
  Neighborhood nh_;
  hook_type pre_hook_;
  hook_type post_hook_;
  std::size_t step_count_ = 0;

  template <std::size_t... I>
  static auto make_buffers(std::index_sequence<I...>)
  {
    return std::tuple{std::array<typename Lattice::template field_type<I>,
                                 (neighbour_count == 0 ? std::size_t{1} : neighbour_count)>{}...};
  }

  template <std::size_t... I>
  void write_cell(const coord_type &c, const fields_tuple &v, std::index_sequence<I...>)
  {
    ((nxt_.template set<I>(c, std::get<I>(v))), ...);
  }

public:
  /** @brief Build a multi-field engine.
   *
   *  Allocates a next-step lattice with the same extents as the initial
   *  lattice. Both buffers share the boundary policy and layout.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r       rule instance (consumed by move).
   *  @param[in] n       neighbourhood instance (consumed by move).
   *  @throws std::bad_alloc on next-buffer allocation failure.
   */
  Multi_Field_Engine(Lattice initial, Rule r, Neighborhood n)
    : cur_(std::move(initial)), nxt_(cur_.extents()), rule_(std::move(r)), nh_(std::move(n))
  {}

  /** @brief Build with a default-constructed neighbourhood. */
  Multi_Field_Engine(Lattice initial, Rule r)
    requires std::default_initializable<Neighborhood>
    : Multi_Field_Engine(std::move(initial), std::move(r), Neighborhood{})
  {}

  /// @return the current frame.
  [[nodiscard]] const Lattice &frame() const noexcept
  {
    return cur_;
  }

  /// @return the number of completed steps.
  [[nodiscard]] std::size_t steps_run() const noexcept
  {
    return step_count_;
  }

  /// @return the lattice extents.
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return cur_.extents();
  }

  /** @brief Register a hook fired *before* every `step()`. */
  template <typename F>
  void on_pre_step(F &&f)
  {
    pre_hook_ = std::forward<F>(f);
  }

  /** @brief Register a hook fired *after* every `step()`. */
  template <typename F>
  void on_post_step(F &&f)
  {
    post_hook_ = std::forward<F>(f);
  }

  /** @brief Apply the multi-field rule to every cell once and swap
   *         buffers.
   *
   *  Per-field neighbour values are gathered into stack arrays whose
   *  sizes are fixed at compile time by `Neighborhood::size_v`.
   *
   *  @throws Any exception propagated by the rule, the lattice or
   *          the hooks.
   *
   *  @par Complexity
   *  O(N * K * F).
   */
  void step()
  {
    if (pre_hook_)
      pre_hook_(step_count_, cur_);

    auto bufs = make_buffers(std::make_index_sequence<field_count>{});

    auto compute_one = [&](const coord_type &c)
    {
      auto views = ca_mf_engine_detail::build_views(cur_, nh_, c, bufs,
                                                    std::make_index_sequence<field_count>{});
      const fields_tuple cur_vals = cur_.at_cell(c);
      const Cell_Context<rank> ctx{step_count_, c};
      const fields_tuple new_vals
        = apply_multi_field_rule<Rule, fields_tuple, rank>(rule_, cur_vals, views, ctx);
      write_cell(c, new_vals, std::make_index_sequence<field_count>{});
    };

    iterate_lattice(compute_one);

    cur_.swap(nxt_);
    ++step_count_;

    if (post_hook_)
      post_hook_(step_count_, cur_);
  }

  /** @brief Run several steps.
   *  @param[in] steps number of consecutive `step()` calls.
   *  @throws Any exception propagated by `step()`.
   */
  void run(const std::size_t steps)
  {
    for (std::size_t i = 0; i < steps; ++i)
      step();
  }

private:
  template <typename F>
  void iterate_lattice(F &&f)
  {
    coord_type c{};
    if constexpr (rank == 1)
      {
        const auto n0 = static_cast<ca_index_t>(cur_.size(0));
        for (ca_index_t i = 0; i < n0; ++i)
          {
            c[0] = i;
            f(c);
          }
      }
    else if constexpr (rank == 2)
      {
        const auto n0 = static_cast<ca_index_t>(cur_.size(0));
        const auto n1 = static_cast<ca_index_t>(cur_.size(1));
        for (ca_index_t i = 0; i < n0; ++i)
          for (ca_index_t j = 0; j < n1; ++j)
            {
              c[0] = i;
              c[1] = j;
              f(c);
            }
      }
    else if constexpr (rank == 3)
      {
        const auto n0 = static_cast<ca_index_t>(cur_.size(0));
        const auto n1 = static_cast<ca_index_t>(cur_.size(1));
        const auto n2 = static_cast<ca_index_t>(cur_.size(2));
        for (ca_index_t i = 0; i < n0; ++i)
          for (ca_index_t j = 0; j < n1; ++j)
            for (ca_index_t k = 0; k < n2; ++k)
              {
                c[0] = i;
                c[1] = j;
                c[2] = k;
                f(c);
              }
      }
    else
      static_assert(rank <= 3, "Multi_Field_Engine currently supports rank <= 3");
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_MULTI_FIELD_ENGINE_H