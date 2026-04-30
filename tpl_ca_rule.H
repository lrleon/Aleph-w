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


/** @file tpl_ca_rule.H
 *  @brief Rule mechanisms for `Aleph::CA`.
 *
 *  All rules in this header are *invocable* with the canonical signature
 *
 *      next_state = rule(current_state, neighbour_view)
 *
 *  so they satisfy the `RuleLike<R, L>` concept of `tpl_ca_concepts.H`.
 *  Catalogue:
 *
 *    - `Outer_Totalistic_Rule<F>` — `F(state, alive_count) -> state`
 *      (Game of Life and any B/S-style rule).
 *    - `Totalistic_Rule<F>`       — `F(sum_including_self) -> state`
 *      (Wolfram totalistic codes).
 *    - `Lookup_Rule<S, K>`        — precomputed table over `(self, k0..)`
 *      with index = `self * S^K + neighbours[0] * S^(K-1) + ...`.
 *      The factory `make_wolfram_elementary_rule(rule_no)` builds the
 *      corresponding `Lookup_Rule<2, 2>` for the 256 elementary
 *      Wolfram rules of 1D radius-1 binary CAs.
 *    - `Probabilistic_Rule<F>`    — thin wrapper that delegates the
 *      randomness to the user-provided functor `F`. A full reproducible
 *      RNG framework is the subject of Phase 8.
 *    - `Composite_Rule<R...>`     — sequential composition.
 *    - `Game_Of_Life_Rule`        — convenience instance (B3/S23).
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

# ifndef TPL_CA_RULE_H
# define TPL_CA_RULE_H

# include <array>
# include <cstddef>
# include <cstdint>
# include <tuple>
# include <utility>

# include <ca-traits.H>

namespace Aleph
{
namespace CA
{

  // -----------------------------------------------------------------------
  // Outer_Totalistic_Rule
  // -----------------------------------------------------------------------

  /** @brief Rule whose next state depends on `(current, alive_count)`.
   *
   *  "Alive" is defined as any non-zero neighbour value (so this rule
   *  works equally well for `bool` and small integral state types).
   *  The wrapped functor `F` receives the current state and the number
   *  of non-zero neighbours, and returns the next state.
   *
   *  @tparam F functor type with signature `(State, std::size_t) -> State`.
   */
  template <typename F>
  class Outer_Totalistic_Rule
  {
    F f;

  public:

    constexpr explicit Outer_Totalistic_Rule(F func)
      : f(std::move(func))
    {
    }

    template <typename State>
    [[nodiscard]] constexpr State
    operator()(const State & current, Neighbor_View<State> neighbours) const
    {
      std::size_t alive = 0;
      for (const auto & v : neighbours)
        if (v != State{})
          ++alive;
      return f(current, alive);
    }
  };

  // -----------------------------------------------------------------------
  // Totalistic_Rule
  // -----------------------------------------------------------------------

  /** @brief Rule whose next state depends on the sum of every cell in
   *         the neighbourhood (including the centre).
   *
   *  Used by Wolfram's totalistic codes and by averaging-style updates.
   *  The functor receives a single `State` argument carrying the sum
   *  and returns the next state.
   *
   *  @tparam F functor type with signature `(State) -> State`.
   */
  template <typename F>
  class Totalistic_Rule
  {
    F f;

  public:

    constexpr explicit Totalistic_Rule(F func)
      : f(std::move(func))
    {
    }

    template <typename State>
    [[nodiscard]] constexpr State
    operator()(const State & current, Neighbor_View<State> neighbours) const
    {
      State sum = current;
      for (const auto & v : neighbours)
        sum = static_cast<State>(sum + v);
      return f(sum);
    }
  };

  // -----------------------------------------------------------------------
  // Lookup_Rule
  // -----------------------------------------------------------------------

  namespace ca_rule_detail
  {
    constexpr std::size_t static_pow_rule(std::size_t base,
                                          std::size_t exp) noexcept
    {
      std::size_t r = 1;
      for (std::size_t i = 0; i < exp; ++i) r *= base;
      return r;
    }
  } // namespace ca_rule_detail

  /** @brief Precomputed transition table for `(self, neighbours...)`.
   *
   *  For `S` states and `K` neighbours, the table size is `S^(K+1)`.
   *  The index used to look up the next state is
   *
   *      idx = self * S^K + neighbours[0] * S^(K-1) + ... + neighbours[K-1]
   *
   *  which matches the canonical "self-major" convention. The factory
   *  `make_wolfram_elementary_rule(rule_no)` translates the standard
   *  Wolfram code (which uses a different bit layout) into a table for
   *  this convention assuming the neighbourhood used at the call site
   *  is `Custom_Neighborhood<1, 2>{ {-1, +1} }` — i.e. left first,
   *  right second.
   *
   *  @tparam NumStates  number of distinct cell states.
   *  @tparam NumNeighbors number of neighbours seen by the rule.
   */
  template <std::size_t NumStates, std::size_t NumNeighbors>
  class Lookup_Rule
  {
  public:

    static constexpr std::size_t num_states_v    = NumStates;
    static constexpr std::size_t num_neighbours_v = NumNeighbors;
    static constexpr std::size_t table_size_v    =
      ca_rule_detail::static_pow_rule(NumStates, NumNeighbors + 1);

    using table_type = std::array<std::uint8_t, table_size_v>;

  private:

    table_type table { };

  public:

    constexpr Lookup_Rule() = default;

    constexpr explicit Lookup_Rule(table_type t)
      : table(t)
    {
    }

    [[nodiscard]] constexpr const table_type & raw_table() const noexcept
    {
      return table;
    }

    template <typename State>
    [[nodiscard]] constexpr State
    operator()(const State & current, Neighbor_View<State> neighbours) const
    {
      std::size_t idx = static_cast<std::size_t>(current);
      for (const auto & v : neighbours)
        idx = idx * NumStates + static_cast<std::size_t>(v);
      return static_cast<State>(table[idx]);
    }
  };

  /** @brief Build the elementary 1D Wolfram rule `rule_no` (0..255) as a
   *         `Lookup_Rule<2, 2>` over neighbourhood `{-1, +1}`.
   *
   *  The standard Wolfram code orders patterns as `(left, self, right)`
   *  with bit weight `(left << 2) | (self << 1) | (right << 0)` and
   *  packs the next-state bits into `rule_no`. Our `Lookup_Rule` uses
   *  `(self, left, right)` with weight `(self << 2) | (left << 1) |
   *  right`. This factory rewrites the table accordingly.
   *
   *  @param[in] rule_no Wolfram rule number, in `[0, 255]`.
   */
  [[nodiscard]] inline constexpr Lookup_Rule<2, 2>
  make_wolfram_elementary_rule(std::uint8_t rule_no) noexcept
  {
    Lookup_Rule<2, 2>::table_type t { };
    for (std::size_t pattern = 0; pattern < 8; ++pattern)
      {
        const std::size_t left  = (pattern >> 2) & 1;
        const std::size_t self  = (pattern >> 1) & 1;
        const std::size_t right = (pattern >> 0) & 1;
        const std::size_t lookup_idx = (self << 2) | (left << 1) | right;
        t[lookup_idx] =
          static_cast<std::uint8_t>((rule_no >> pattern) & 1);
      }
    return Lookup_Rule<2, 2>(t);
  }

  // -----------------------------------------------------------------------
  // Probabilistic_Rule
  // -----------------------------------------------------------------------

  /** @brief Thin wrapper that flags a rule as stochastic.
   *
   *  The wrapped functor `F` is responsible for whatever random source
   *  it uses; this class adds no synchronisation. Phase 8 will
   *  introduce `Per_Thread_RNG` and reproducible probabilistic rules
   *  on top of this scaffolding.
   */
  template <typename F>
  class Probabilistic_Rule
  {
    mutable F f;

  public:

    constexpr explicit Probabilistic_Rule(F func)
      : f(std::move(func))
    {
    }

    template <typename State>
    [[nodiscard]] State
    operator()(const State & current, Neighbor_View<State> neighbours) const
    {
      return f(current, neighbours);
    }
  };

  // -----------------------------------------------------------------------
  // Composite_Rule
  // -----------------------------------------------------------------------

  /** @brief Sequential composition of one or more rules.
   *
   *  `Composite_Rule<R1, R2, ..., Rn>` applies `R1` first, then `R2`
   *  on the resulting state, and so on. The neighbour view passed to
   *  every stage is the same (read at the start of the step), which
   *  is the standard semantics for layered totalistic rules.
   */
  template <typename... Rules>
  class Composite_Rule
  {
    std::tuple<Rules...> rules;

  public:

    constexpr explicit Composite_Rule(Rules... rs)
      : rules(std::move(rs)...)
    {
    }

    template <typename State>
    [[nodiscard]] constexpr State
    operator()(const State & current, Neighbor_View<State> neighbours) const
    {
      State acc = current;
      std::apply([&](const auto &... r)
        {
          ((acc = r(acc, neighbours)), ...);
        }, rules);
      return acc;
    }
  };

  // -----------------------------------------------------------------------
  // Game of Life: B3/S23 — convenience instance
  // -----------------------------------------------------------------------

  /// Functor implementing Conway's Game of Life canonical rule (B3/S23).
  struct Game_Of_Life_Functor
  {
    template <typename State>
    [[nodiscard]] constexpr State
    operator()(const State & current, std::size_t alive) const noexcept
    {
      const bool alive_now = current != State{};
      const bool alive_next =
        (alive_now and (alive == 2 or alive == 3)) or
        (not alive_now and alive == 3);
      return alive_next ? static_cast<State>(1) : static_cast<State>(0);
    }
  };

  using Game_Of_Life_Rule = Outer_Totalistic_Rule<Game_Of_Life_Functor>;

  /// Build the canonical Game of Life rule.
  [[nodiscard]] inline constexpr Game_Of_Life_Rule
  make_game_of_life_rule() noexcept
  {
    return Game_Of_Life_Rule(Game_Of_Life_Functor{});
  }

} // namespace CA
} // namespace Aleph

# endif // TPL_CA_RULE_H