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

/** @file tpl_ca_async_engine.H
 *  @brief Phase 13 update-scheme aware engine.
 *
 *  `Async_Engine<Lattice, Rule, Neighborhood, Update_Scheme>` is the
 *  Phase 13 generic engine: it owns one (or, when the scheme requires
 *  it, two) lattice buffers and dispatches each `step()` to the chosen
 *  update scheme strategy. The scheme controls the visit order, the
 *  in-place vs double-buffer write strategy, and (for Margolus) the
 *  block-rule semantics.
 *
 *  The engine itself is intentionally minimal: it owns state, exposes
 *  the buffers and the neighborhood, and forwards `step()` to the
 *  scheme. All variation lives in the strategies of
 *  `tpl_ca_update_scheme.H`. This is the "ortogonal" design called for
 *  in Phase 13: combine any `Lattice × Rule × Update_Scheme` without
 *  spawning a new engine per scheme.
 *
 *  Design constraints:
 *
 *    - Memory: when `Update_Scheme::requires_double_buffer == false`
 *      (Sequential, Random_Asynchronous, Margolus) only the current
 *      lattice is allocated; the next-buffer member stays empty,
 *      saving roughly 50% of the storage compared to
 *      `Synchronous_Engine`.
 *    - Hooks: `on_pre_step` / `on_post_step` mirror the synchronous
 *      engine API.
 *    - Margolus: `step_back()` reverses the most recent step. For an
 *      involution Block_Rule the composition `forward(N) ∘ backward(N)`
 *      is the bit-exact identity.
 *
 *  Block-rule schemes (those with `requires_block_rule == true`) ignore
 *  the `Neighborhood` template parameter; pass `Null_Neighborhood<Rank>`
 *  in that case.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_ASYNC_ENGINE_H
#define TPL_CA_ASYNC_ENGINE_H

#include <cstddef>
#include <functional>
#include <type_traits>
#include <utility>

#include <ah-errors.H>

#include <ca-traits.H>
#include <tpl_ca_concepts.H>
#include <tpl_ca_neighborhood.H>
#include <tpl_ca_update_scheme.H>

namespace Aleph {
namespace CA {

namespace ca_async_engine_detail {

/// Detect whether a lattice exposes a `refresh_halo()` method (the
/// hallmark of a `Ghost_Lattice` or any other halo-aware variant).
template <typename L>
concept HasRefreshHalo = requires(L &l) { l.refresh_halo(); };

/// Detect whether the scheme exposes `apply_with_origin(engine, rule, origin)`
/// (Margolus). When present the engine can drive `step_back()` by
/// re-applying the active partition with the same parity that produced
/// the most recent forward step.
template <typename Scheme, typename Engine, typename Rule>
concept HasApplyWithOrigin
  = requires(const Scheme &s, Engine &e, Rule &r, ca_index_t o) { s.apply_with_origin(e, r, o); };

}  // namespace ca_async_engine_detail

/** @brief Placeholder neighbourhood for engines that drive block rules.
 *
 *  The Margolus update operates on 2×2 blocks and never queries a
 *  neighbourhood. To keep `Async_Engine` uniformly parameterised on a
 *  `Neighborhood` type, instantiate the engine with
 *  `Null_Neighborhood<Rank>`: the neighbourhood declares zero offsets
 *  and is therefore a no-op for any code path that consults it.
 *
 *  @tparam Rank lattice dimension.
 */
template <std::size_t Rank>
class Null_Neighborhood
{
public:
  /// Lattice dimension.
  static constexpr std::size_t rank_v = Rank;
  /// Empty neighbourhood: zero offsets.
  static constexpr std::size_t size_v = 0;
  /// Compile-time radius (zero, since the neighbourhood is empty).
  static constexpr std::size_t radius_v = 0;
  /// Coordinate type accepted by `for_each_offset`.
  using coord_type = Coord_Vec<Rank>;

  /** @brief Always returns 0 (no offsets).
   *  @return `0`.
   */
  [[nodiscard]] static constexpr std::size_t radius() noexcept
  {
    return 0;
  }

  /** @brief Always returns 0 (no offsets).
   *  @return `0`.
   */
  [[nodiscard]] static constexpr std::size_t size() noexcept
  {
    return 0;
  }

  /** @brief No-op: the neighbourhood is empty.
   *  @tparam F functor type.
   */
  template <typename F>
  static constexpr void for_each_offset(const coord_type &, F &&) noexcept
  {}
};

/** @brief Update-scheme aware engine.
 *
 *  Owns a current lattice buffer (and, when `Update_Scheme::requires_double_buffer`
 *  is true, a next buffer of identical extents). Each `step()`
 *  dispatches to the scheme's `apply()` method, which is responsible
 *  for visiting the cells in the desired order and updating the lattice
 *  in place or by double buffering.
 *
 *  @tparam Lattice lattice type.
 *  @tparam Rule rule (cell rule or block rule depending on the scheme).
 *  @tparam Neighborhood neighbourhood (`Null_Neighborhood<Rank>` for
 *          block-rule schemes).
 *  @tparam Update_Scheme update strategy (default
 *          `Synchronous_Update<RowMajor>`).
 */
template <typename Lattice, typename Rule, typename Neighborhood, typename Update_Scheme = Synchronous_Update<RowMajor>>
class Async_Engine
{
  static_assert(LatticeLike<Lattice>, "Async_Engine requires a LatticeLike lattice");
  static_assert(NeighborhoodLike<Neighborhood>,
                "Async_Engine requires a NeighborhoodLike neighborhood (use Null_Neighborhood for "
                "block rules)");
  static_assert(Neighborhood::rank_v == Lattice::rank, "Neighborhood rank must match lattice rank");

public:
  using lattice_type = Lattice;
  using rule_type = Rule;
  using neighborhood_type = Neighborhood;
  using scheme_type = Update_Scheme;

  using state_type = typename Lattice::state_type;
  using coord_type = typename Lattice::coord_type;
  using extents_type = typename Lattice::extents_type;

  /// Lattice dimension.
  static constexpr std::size_t rank = Lattice::rank;

  /// Hook signature: `(step_index, frame)`.
  using hook_type = std::function<void(std::size_t, const Lattice &)>;

private:
  Lattice cur_;
  Lattice nxt_;  // empty for in-place schemes
  Rule rule_;
  Neighborhood nh_;
  Update_Scheme scheme_;
  hook_type pre_hook_;
  hook_type post_hook_;
  std::size_t step_count_ = 0;

public:
  /** @brief Build an engine.
   *
   *  Allocates the next-buffer only when `Update_Scheme::requires_double_buffer`
   *  is `true`. For in-place schemes (Sequential, Random_Asynchronous,
   *  Margolus) the next-buffer member stays empty.
   *
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r rule instance (consumed by move).
   *  @param[in] n neighborhood instance (consumed by move).
   *  @param[in] s update-scheme instance (consumed by move).
   *  @throws std::bad_alloc on allocation failure.
   */
  Async_Engine(Lattice initial, Rule r, Neighborhood n, Update_Scheme s = {})
    : cur_(std::move(initial)), rule_(std::move(r)), nh_(std::move(n)), scheme_(std::move(s))
  {
    if constexpr (Update_Scheme::requires_double_buffer)
      nxt_ = Lattice(cur_.extents());
  }

  /** @brief Build an engine with a default-constructed neighborhood.
   *  @param[in] initial initial lattice (consumed by move).
   *  @param[in] r rule instance.
   *  @param[in] s update-scheme instance.
   *  @throws std::bad_alloc on allocation failure.
   */
  Async_Engine(Lattice initial, Rule r, Update_Scheme s = {})
    requires std::default_initializable<Neighborhood>
    : Async_Engine(std::move(initial), std::move(r), Neighborhood{}, std::move(s))
  {}

  /// @return read-only view of the current frame.
  [[nodiscard]] const Lattice &frame() const noexcept
  {
    return cur_;
  }

  /// @return number of completed `step()` calls.
  [[nodiscard]] std::size_t steps_run() const noexcept
  {
    return step_count_;
  }

  /// @return the lattice extents.
  [[nodiscard]] const extents_type &extents() const noexcept
  {
    return cur_.extents();
  }

  /// @return read-only access to the underlying neighborhood.
  [[nodiscard]] const Neighborhood &neighborhood() const noexcept
  {
    return nh_;
  }

  /// @return read-write access to the current lattice (used by schemes).
  [[nodiscard]] Lattice &current_buffer() noexcept
  {
    return cur_;
  }

  /// @return read-only access to the current lattice (used by schemes).
  [[nodiscard]] const Lattice &current_buffer() const noexcept
  {
    return cur_;
  }

  /// @return read-write access to the next lattice (only valid for
  ///         schemes that require double buffering).
  [[nodiscard]] Lattice &next_buffer() noexcept
  {
    return nxt_;
  }

  /// @return read-write access to the rule.
  [[nodiscard]] Rule &rule() noexcept
  {
    return rule_;
  }

  /// @return read-only access to the rule.
  [[nodiscard]] const Rule &rule() const noexcept
  {
    return rule_;
  }

  /// @return read-only access to the update scheme.
  [[nodiscard]] const Update_Scheme &scheme() const noexcept
  {
    return scheme_;
  }

  /// @return current step index (number of completed steps so far).
  [[nodiscard]] std::size_t step_count() const noexcept
  {
    return step_count_;
  }

  /// O(1) swap of the two buffers (called by double-buffer schemes).
  void swap_buffers() noexcept(noexcept(cur_.swap(nxt_)))
  {
    cur_.swap(nxt_);
  }

  /** @brief Register a hook fired *before* every `step()`.
   *  @tparam F callable accepting `(std::size_t, const Lattice &)`.
   *  @param[in] f hook to install.
   */
  template <typename F>
  void on_pre_step(F &&f)
  {
    pre_hook_ = std::forward<F>(f);
  }

  /** @brief Register a hook fired *after* every `step()`.
   *  @tparam F callable accepting `(std::size_t, const Lattice &)`.
   *  @param[in] f hook to install.
   */
  template <typename F>
  void on_post_step(F &&f)
  {
    post_hook_ = std::forward<F>(f);
  }

  /** @brief Run one engine step using the configured update scheme.
   *
   *  Calls `refresh_halo()` on lattices that have a halo, fires the
   *  pre-step hook, dispatches to `scheme_.apply(*this, rule_)`, then
   *  fires the post-step hook.
   *
   *  @throws Any exception propagated by the rule, the scheme or the
   *          hooks.
   */
  void step()
  {
    if constexpr (ca_async_engine_detail::HasRefreshHalo<Lattice>)
      cur_.refresh_halo();

    if (pre_hook_)
      pre_hook_(step_count_, cur_);

    scheme_.apply(*this, rule_);
    ++step_count_;

    if (post_hook_)
      post_hook_(step_count_, cur_);
  }

  /** @brief Reverse the most recent step (Margolus only).
   *
   *  When the scheme exposes `apply_with_origin(engine, rule, origin)`
   *  (the Margolus contract) and the rule is an involution, applying
   *  the rule with the same partition origin that produced the
   *  forward step undoes the step bit-by-bit. This method decrements
   *  the step counter and re-applies the scheme with the parity that
   *  generated the now-discarded forward step.
   *
   *  @pre `steps_run() > 0`.
   *  @throws std::out_of_range if no step has been run yet.
   *  @throws Any exception propagated by the rule or the scheme.
   */
  void step_back()
    requires ca_async_engine_detail::HasApplyWithOrigin<Update_Scheme, Async_Engine, Rule>
  {
    ah_out_of_range_error_if(step_count_ == 0) << "Async_Engine::step_back: no step to reverse";

    --step_count_;
    if constexpr (ca_async_engine_detail::HasRefreshHalo<Lattice>)
      cur_.refresh_halo();

    if (pre_hook_)
      pre_hook_(step_count_, cur_);

    scheme_.apply_with_origin(*this, rule_, Update_Scheme::origin_for_step(step_count_));

    if (post_hook_)
      post_hook_(step_count_, cur_);
  }

  /** @brief Run several steps.
   *  @param[in] steps number of `step()` invocations.
   *  @throws Any exception propagated by `step()`.
   */
  void run(const std::size_t steps)
  {
    for (std::size_t i = 0; i < steps; ++i)
      step();
  }

  /** @brief Reverse several steps (Margolus only).
   *  @param[in] steps number of `step_back()` invocations.
   *  @throws Any exception propagated by `step_back()`.
   */
  void run_back(const std::size_t steps)
    requires ca_async_engine_detail::HasApplyWithOrigin<Update_Scheme, Async_Engine, Rule>
  {
    for (std::size_t i = 0; i < steps; ++i)
      step_back();
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_ASYNC_ENGINE_H