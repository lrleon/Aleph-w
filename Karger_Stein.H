/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/** @file Karger_Stein.H
 *  @brief Karger-Stein randomized min-cut algorithm (alias for Karger.H fast method).
 *
 *  This header provides the Karger-Stein algorithm as an alias to the
 *  `fast()` method in Karger_Min_Cut. The implementation is in Karger.H.
 *
 *  ## Algorithm Description
 *
 *  The Karger-Stein algorithm improves upon the basic Karger algorithm by
 *  observing that most of the randomness is "wasted" when the graph is large.
 *  The probability of contracting a min-cut edge increases as the graph
 *  shrinks, so the algorithm:
 *
 *  1. **Contract to √(2n) vertices**: Randomly contract edges until only
 *     ⌈n/√2⌉ + 1 vertices remain. This phase has high probability of
 *     preserving the min-cut.
 *
 *  2. **Recurse twice**: Make two independent recursive calls on the
 *     contracted graph. This branching compensates for the increasing
 *     probability of error as the graph shrinks.
 *
 *  3. **Return minimum**: Return the smaller of the two cuts found.
 *
 *  ## Time Complexity
 *
 *  - **Karger basic**: O(n² m) per iteration × O(n²) iterations = O(n⁴ m)
 *  - **Karger-Stein**: O(n² log³ n) total
 *
 *  ## Usage
 *
 *  Use `Karger_Min_Cut<GT>::fast()` method from Karger.H:
 *
 *  @code
 *  #include <Karger.H>
 *
 *  Karger_Min_Cut<GT> karger;
 *  DynList<GT::Node*> S, T;
 *  DynList<GT::Arc*> cut_arcs;
 *
 *  // Fast Karger-Stein: O(n² log³ n)
 *  size_t cut_size = karger.fast(g, S, T, cut_arcs);
 *  @endcode
 *
 *  @see Karger_Min_Cut The main class with both standard and fast methods
 *  @see Stoer_Wagner_Min_Cut Deterministic O(nm + n² log n) algorithm
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef KARGER_STEIN_H
# define KARGER_STEIN_H

# include <Karger.H>

namespace Aleph
{
  /**
   * @brief Alias for Karger_Min_Cut (use fast() method for Karger-Stein)
   *
   * The Karger-Stein algorithm is implemented as the `fast()` method
   * in Karger_Min_Cut. This alias is provided for clarity.
   *
   * @tparam GT Graph type
   * @tparam SA Arc filter (default: show all arcs)
   *
   * @code
   * Karger_Stein_Min_Cut<GT> ks;
   * size_t cut = ks.fast(g, S, T, cut_arcs);  // O(n² log³ n)
   * @endcode
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  using Karger_Stein_Min_Cut = Karger_Min_Cut<GT, SA>;

} // namespace Aleph

# endif // KARGER_STEIN_H
