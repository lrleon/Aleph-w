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

/** @file tpl_ca_block_rule.H
 *  @brief Block-based local rules for `Aleph::CA` (Phase 13).
 *
 *  Where a `RuleLike` rule consumes `(state, neighbours)` and returns
 *  one cell value, a *block rule* consumes a fixed-size block of cell
 *  values and returns the new block in one shot. Block rules are the
 *  natural fit for the Margolus update (`tpl_ca_update_scheme.H`):
 *  the partition decides which 2×2 cells form the active block, and
 *  the rule rewrites those four cells atomically.
 *
 *  The bundled rules cover the canonical reversible automata:
 *
 *    - `Critters_Rule` — invert population if `popcount != 2`, then
 *      rotate the block 180°. Reversible because both invert and
 *      rotate-180 are involutions.
 *    - `BBM_Rule` — Fredkin & Toffoli's Billiard Ball Machine logic.
 *    - `TM_Gas_Rule` — Toffoli–Margolus lattice gas (variant of BBM
 *      that conserves particles and momentum).
 *    - `Cell_Rule_As_Block_Rule<R>` — adaptor that wraps a regular
 *      cell rule (whose neighbourhood is empty) as a block rule.
 *
 *  Convention for the 2×2 block array:
 *
 *      index 0 = NW (top-left)     index 1 = NE (top-right)
 *      index 2 = SW (bottom-left)  index 3 = SE (bottom-right)
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

#ifndef TPL_CA_BLOCK_RULE_H
#define TPL_CA_BLOCK_RULE_H

#include <array>
#include <concepts>
#include <cstddef>
#include <type_traits>
#include <utility>

namespace Aleph {
namespace CA {

/** @brief Fixed-size 2×2 block of cell values used by Margolus rules.
 *
 *  Layout:
 *    block[0] = NW, block[1] = NE,
 *    block[2] = SW, block[3] = SE.
 *
 *  @tparam State cell state type.
 */
template <typename State>
using Block_2x2 = std::array<State, 4>;

/** @brief A rule that maps a 2×2 block to a 2×2 block.
 *
 *  @tparam R candidate rule type.
 *  @tparam State cell state type.
 */
template <typename R, typename State>
concept Block_Rule_2x2 = requires(const R &r, Block_2x2<State> b) {
  { r(b) } -> std::convertible_to<Block_2x2<State>>;
};

namespace ca_block_rule_detail {

/// Toggle a cell value between its zero and a non-zero counterpart.
/// Works for `bool`, integral types and small enums.
template <typename State>
[[nodiscard]] inline constexpr State invert(const State &s) noexcept
{
  if constexpr (std::is_same_v<State, bool>)
    return not s;
  else if constexpr (std::is_integral_v<State>)
    return s == State{0} ? State{1} : State{0};
  else
    return s == State{} ? State{1} : State{};
}

/// Population count of a 2×2 block: number of non-zero cells.
template <typename State>
[[nodiscard]] inline constexpr std::size_t popcount(const Block_2x2<State> &b) noexcept
{
  std::size_t n = 0;
  for (const auto &v : b)
    if (v != State{})
      ++n;
  return n;
}

}  // namespace ca_block_rule_detail

// =======================================================================
// Critters
// =======================================================================

/** @brief Critters reversible CA block rule.
 *
 *  Specification (Toffoli & Margolus, 1987):
 *    - If the 2×2 block has exactly two alive cells, leave it
 *      unchanged.
 *    - Otherwise, invert each cell (alive ↔ empty) and rotate the
 *      block 180° around its centre.
 *
 *  Both branches are involutions and their composition is also an
 *  involution: applying the rule twice to the same block returns
 *  the original block. Combined with the Margolus 2×2 partition
 *  (which alternates origin between consecutive steps), the
 *  resulting CA is bit-exact reversible.
 */
class Critters_Rule
{
public:
  /** @brief Apply the Critters block rule.
   *  @tparam State cell state (binary; `bool` or any integral with
   *          0 = empty / 1 = alive).
   *  @param[in] in current 2×2 block (NW, NE, SW, SE).
   *  @return next 2×2 block.
   *  @throws This function does not throw.
   */
  template <typename State>
  [[nodiscard]] constexpr Block_2x2<State> operator()(const Block_2x2<State> &in) const noexcept
  {
    using namespace ca_block_rule_detail;
    if (popcount(in) == 2)
      return in;
    // Invert each cell, then rotate 180°: NW ↔ SE, NE ↔ SW.
    Block_2x2<State> out{};
    out[0] = invert(in[3]);
    out[1] = invert(in[2]);
    out[2] = invert(in[1]);
    out[3] = invert(in[0]);
    return out;
  }
};

// =======================================================================
// Billiard Ball Machine (BBM)
// =======================================================================

/** @brief Fredkin–Toffoli Billiard Ball Machine block rule.
 *
 *  Specification (Margolus 1984): the block's next state is computed
 *  by counting alive cells:
 *
 *    - 1 alive → swap to the diagonally-opposite corner (the ball
 *      moves diagonally through the 2×2 block).
 *    - 2 alive on the same diagonal → swap to the other diagonal
 *      (head-on collision: balls leave at right angles).
 *    - Other configurations → leave the block unchanged.
 *
 *  The rule is its own inverse on every input pattern, so the
 *  associated Margolus CA is reversible and conserves the number
 *  of alive cells (particles).
 */
class BBM_Rule
{
public:
  /** @brief Apply the BBM block rule.
   *  @tparam State binary cell state.
   *  @param[in] in current 2×2 block.
   *  @return next 2×2 block.
   *  @throws This function does not throw.
   */
  template <typename State>
  [[nodiscard]] constexpr Block_2x2<State> operator()(const Block_2x2<State> &in) const noexcept
  {
    using namespace ca_block_rule_detail;
    const std::size_t n = popcount(in);

    if (n == 1)
      {
        // Ball moves to the diagonally-opposite corner.
        Block_2x2<State> out{};
        out[0] = in[3];
        out[1] = in[2];
        out[2] = in[1];
        out[3] = in[0];
        return out;
      }
    if (n == 2)
      {
        // Diagonal swap only on the two "head-on" patterns.
        const bool main_diag
          = (in[0] != State{}) and (in[3] != State{}) and (in[1] == State{}) and (in[2] == State{});
        const bool anti_diag
          = (in[1] != State{}) and (in[2] != State{}) and (in[0] == State{}) and (in[3] == State{});
        if (main_diag or anti_diag)
          {
            Block_2x2<State> out{};
            out[0] = in[3];
            out[1] = in[2];
            out[2] = in[1];
            out[3] = in[0];
            return out;
          }
      }
    return in;
  }
};

// =======================================================================
// Toffoli–Margolus lattice gas
// =======================================================================

/** @brief Toffoli–Margolus (TM) lattice-gas block rule.
 *
 *  A close cousin of BBM that conserves particle count while exposing
 *  a different mobility pattern: particles bounce off horizontal
 *  walls instead of moving diagonally. For each 2×2 block:
 *
 *    - 1 or 3 alive → swap horizontally (NW ↔ NE, SW ↔ SE),
 *      modelling a horizontal-axis reflection.
 *    - 2 alive on the same row → swap to the other row (vertical
 *      reflection).
 *    - 2 alive on the same column → swap to the other column
 *      (horizontal reflection).
 *    - 2 alive on a diagonal → swap to the other diagonal.
 *    - 0 or 4 alive → unchanged.
 *
 *  Each branch is an involution and the resulting CA conserves the
 *  particle count exactly, making the Margolus update reversible
 *  bit-for-bit (`forward(N) ∘ backward(N) = identity`).
 */
class TM_Gas_Rule
{
public:
  /** @brief Apply the TM lattice-gas block rule.
   *  @tparam State binary cell state.
   *  @param[in] in current 2×2 block.
   *  @return next 2×2 block.
   *  @throws This function does not throw.
   */
  template <typename State>
  [[nodiscard]] constexpr Block_2x2<State> operator()(const Block_2x2<State> &in) const noexcept
  {
    using namespace ca_block_rule_detail;
    const std::size_t n = popcount(in);

    if (n == 1 or n == 3)
      {
        // Horizontal swap: NW ↔ NE, SW ↔ SE.
        Block_2x2<State> out{};
        out[0] = in[1];
        out[1] = in[0];
        out[2] = in[3];
        out[3] = in[2];
        return out;
      }
    if (n == 2)
      {
        const bool top_row
          = (in[0] != State{}) and (in[1] != State{}) and (in[2] == State{}) and (in[3] == State{});
        const bool bottom_row
          = (in[2] != State{}) and (in[3] != State{}) and (in[0] == State{}) and (in[1] == State{});
        if (top_row or bottom_row)
          {
            // Vertical swap: top row ↔ bottom row.
            Block_2x2<State> out{};
            out[0] = in[2];
            out[1] = in[3];
            out[2] = in[0];
            out[3] = in[1];
            return out;
          }

        const bool left_col
          = (in[0] != State{}) and (in[2] != State{}) and (in[1] == State{}) and (in[3] == State{});
        const bool right_col
          = (in[1] != State{}) and (in[3] != State{}) and (in[0] == State{}) and (in[2] == State{});
        if (left_col or right_col)
          {
            // Horizontal swap: left column ↔ right column.
            Block_2x2<State> out{};
            out[0] = in[1];
            out[1] = in[0];
            out[2] = in[3];
            out[3] = in[2];
            return out;
          }

        const bool main_diag
          = (in[0] != State{}) and (in[3] != State{}) and (in[1] == State{}) and (in[2] == State{});
        const bool anti_diag
          = (in[1] != State{}) and (in[2] != State{}) and (in[0] == State{}) and (in[3] == State{});
        if (main_diag or anti_diag)
          {
            // Diagonal swap.
            Block_2x2<State> out{};
            out[0] = in[3];
            out[1] = in[2];
            out[2] = in[1];
            out[3] = in[0];
            return out;
          }
      }
    return in;
  }
};

// =======================================================================
// Cell_Rule_As_Block_Rule
// =======================================================================

/** @brief Adapt a regular cell rule (with empty neighborhood) to a block rule.
 *
 *  Wraps a `RuleLike` cell rule whose transition depends only on its
 *  own state and applies it independently to every cell of the block.
 *  This is the trivial bridge that makes simple per-cell updates
 *  available to any Block_Rule consumer (e.g. `Margolus_Update`).
 *
 *  The wrapped rule must be invocable as `cell_rule(state)` returning
 *  the next state, mirroring an "outer 0-arg" (no neighbours) form.
 *
 *  @tparam Cell_Rule wrapped cell-rule type.
 */
template <typename Cell_Rule>
class Cell_Rule_As_Block_Rule
{
  Cell_Rule cell_;

public:
  /** @brief Wrap `cr` as a block rule.
   *  @param[in] cr cell rule (consumed by move).
   *  @throws Any exception thrown by `Cell_Rule`'s move constructor.
   */
  constexpr explicit Cell_Rule_As_Block_Rule(Cell_Rule cr) : cell_(std::move(cr)) {}

  /** @brief Apply the wrapped rule to every cell of the 2×2 block.
   *  @tparam State cell state type.
   *  @param[in] in current 2×2 block.
   *  @return next 2×2 block.
   *  @throws Any exception propagated by the wrapped cell rule.
   */
  template <typename State>
  [[nodiscard]] constexpr Block_2x2<State> operator()(const Block_2x2<State> &in) const
  {
    Block_2x2<State> out{};
    for (std::size_t i = 0; i < 4; ++i)
      out[i] = cell_(in[i]);
    return out;
  }
};

}  // namespace CA
}  // namespace Aleph

#endif  // TPL_CA_BLOCK_RULE_H