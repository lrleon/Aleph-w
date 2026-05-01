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


/** @file ca-engine-utils.H
 *  @brief Convenience builders for the Phase 3 synchronous engine.
 *
 *  Provides factory functions that pick reasonable defaults for the
 *  common cases:
 *
 *    - `make_gol_engine(rows, cols)` — Conway's Game of Life on a
 *      toroidal `int` lattice with a Moore radius-1 neighbourhood.
 *    - `make_wolfram_engine(rule_no, width)` — elementary 1D Wolfram
 *      rule on an open-boundary `int` lattice.
 *
 *  These helpers exist for users who want a one-liner setup; the
 *  underlying engine class lives in `tpl_ca_engine.H` and accepts any
 *  combination of lattice / rule / neighbourhood / iteration order.
 *
 *  @ingroup CellularAutomata
 *  @author Leandro Rabindranath Leon
 */

# ifndef CA_ENGINE_UTILS_H
# define CA_ENGINE_UTILS_H

# include <cstdint>
# include <utility>

# include <ca-traits.H>
# include <tpl_ca_storage.H>
# include <tpl_ca_lattice.H>
# include <tpl_ca_neighborhood.H>
# include <tpl_ca_rule.H>
# include <tpl_ca_engine.H>

namespace Aleph
{
namespace CA
{

  /// Game-of-Life engine type alias (toroidal, integer cells).
  using Game_Of_Life_Engine =
    Synchronous_Engine<
      Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>,
      Game_Of_Life_Rule,
      Moore<2, 1>>;

  /** @brief Build a Conway's Game of Life engine of the given shape.
   *
   *  All cells start dead. To stamp a pattern before stepping, build
   *  a `Lattice initial(...)` and pass it to the seeded overload.
   *
   *  @param[in] rows number of rows (axis 0).
   *  @param[in] cols number of columns (axis 1).
   *  @return a synchronous Game of Life engine with toroidal borders.
   *
   *  @par Complexity
   *  O(rows * cols) for lattice allocation and initialisation.
   */
  [[nodiscard]] inline Game_Of_Life_Engine
  make_gol_engine(ca_size_t rows, ca_size_t cols)
  {
    using L = Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary>;
    return Game_Of_Life_Engine(L({ rows, cols }, 0),
                               make_game_of_life_rule(),
                               Moore<2, 1>{});
  }

  /** @brief Build a Game of Life engine seeded with an explicit lattice.
   *
   *  Use this overload when the initial pattern must be stamped before
   *  the first `step()`.
   *
   *  @param[in] initial lattice that becomes the current frame.
   *  @return a synchronous Game of Life engine with toroidal borders.
   *
   *  @par Complexity
   *  O(initial.size()) for the next-buffer allocation.
   */
  [[nodiscard]] inline Game_Of_Life_Engine
  make_gol_engine(Lattice<Dense_Cell_Storage<int, 2>, ToroidalBoundary> initial)
  {
    return Game_Of_Life_Engine(std::move(initial),
                               make_game_of_life_rule(),
                               Moore<2, 1>{});
  }

  /// 1D Wolfram engine type alias (open boundary, integer cells).
  using Wolfram_1D_Engine =
    Synchronous_Engine<
      Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>,
      Lookup_Rule<2, 2>,
      Custom_Neighborhood<1, 2>>;

  /** @brief Build a 1D elementary Wolfram engine of the given width.
   *
   *  The lattice starts at zero everywhere. Stamp the impulse(s)
   *  before invoking `step()` through the second overload, or by
   *  building the initial lattice yourself and using the
   *  `Synchronous_Engine` constructor directly.
   *
   *  @param[in] rule_no Wolfram elementary rule number, `[0, 255]`.
   *  @param[in] width   number of cells along the single axis.
   *  @return a synchronous elementary Wolfram engine with open borders.
   *
   *  @par Complexity
   *  O(width) for lattice allocation and initialisation.
   */
  [[nodiscard]] inline Wolfram_1D_Engine
  make_wolfram_engine(std::uint8_t rule_no, ca_size_t width)
  {
    using L = Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary>;
    Custom_Neighborhood<1, 2> nh(std::array<Offset_Vec<1>, 2>{ Offset_Vec<1>{ -1 },
                                                               Offset_Vec<1>{  1 } });
    return Wolfram_1D_Engine(L({ width }, 0),
                             make_wolfram_elementary_rule(rule_no),
                             std::move(nh));
  }

  /** @brief Build a 1D elementary Wolfram engine from an initial row.
   *
   *  @param[in] rule_no Wolfram elementary rule number, `[0, 255]`.
   *  @param[in] initial lattice that becomes the current frame.
   *  @return a synchronous elementary Wolfram engine with open borders.
   *
   *  @par Complexity
   *  O(initial.size()) for the next-buffer allocation.
   */
  [[nodiscard]] inline Wolfram_1D_Engine
  make_wolfram_engine(std::uint8_t rule_no,
                      Lattice<Dense_Cell_Storage<int, 1>, OpenBoundary> initial)
  {
    Custom_Neighborhood<1, 2> nh(std::array<Offset_Vec<1>, 2>{ Offset_Vec<1>{ -1 },
                                                               Offset_Vec<1>{  1 } });
    return Wolfram_1D_Engine(std::move(initial),
                             make_wolfram_elementary_rule(rule_no),
                             std::move(nh));
  }

} // namespace CA
} // namespace Aleph

# endif // CA_ENGINE_UTILS_H
