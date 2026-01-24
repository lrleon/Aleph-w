
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
