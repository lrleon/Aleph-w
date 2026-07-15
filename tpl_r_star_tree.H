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

/** @file tpl_r_star_tree.H
 *  @brief R*-tree: an R-tree tuned with the Beckmann-Kriegel-Schneider-Seeger
 *         heuristics.
 *
 *  `RStarTree<Payload, MaxEntries, MinEntries>` is the same dynamic spatial
 *  index as `RTree` (same API, same `Rectangle`/`Payload` entries, same exact
 *  rational coordinates), but built with the R*-tree insertion strategy instead
 *  of Guttman's quadratic split. It is a thin alias over
 *  `RTree<Payload, MaxEntries, MinEntries, RTreeVariant::RStar>`, so it shares
 *  all of `RTree`'s query, deletion, verification and copy/move machinery.
 *
 *  Relative to the plain R-tree, the R*-tree changes three things:
 *
 *  - **ChooseSubtree**: when descending into a node whose children are leaves,
 *    it picks the child whose growth adds the least *overlap* with its siblings
 *    (ties broken by least area enlargement, then least area), instead of only
 *    minimizing area enlargement.
 *  - **Node split**: it chooses the split axis that minimizes the summed
 *    perimeter (margin) of the two resulting groups, then the distribution along
 *    that axis that minimizes their overlap area (ties: minimum total area),
 *    instead of Guttman's quadratic seed/assignment split.
 *  - **Forced reinsertion**: the first time a *leaf* overflows during an
 *    insertion, rather than splitting immediately it removes the entries
 *    farthest from the leaf centre and reinserts them from the root. This tends
 *    to produce more compact, less overlapping trees and better query
 *    performance. Reinsertion here is applied at the leaf level (internal
 *    overflows split with the R*-tree heuristic).
 *
 *  These heuristics usually yield lower node overlap and faster queries than the
 *  plain R-tree at the cost of somewhat more work per insertion. All structural
 *  invariants, complexity classes, exception-safety, thread-safety and
 *  reference-invalidation rules are exactly those documented for `RTree` in
 *  `tpl_r_tree.H`.
 *
 *  @ingroup Geometry
 */

#ifndef TPL_R_STAR_TREE_H
#define TPL_R_STAR_TREE_H

#include <tpl_r_tree.H>

namespace Aleph {

/** @brief Dynamic R*-tree spatial index (R-tree with the R*-tree heuristics).
 *
 *  @tparam Payload Value associated with each stored bounding box.
 *  @tparam MaxEntries Maximum number of entries per node (>= 2).
 *  @tparam MinEntries Minimum number of entries per non-root node. Defaults to
 *          `MaxEntries / 2`; must satisfy `2 * MinEntries <= MaxEntries + 1`.
 */
template <typename Payload, size_t MaxEntries = 16, size_t MinEntries = MaxEntries / 2>
using RStarTree = RTree<Payload, MaxEntries, MinEntries, RTreeVariant::RStar>;

}  // namespace Aleph

#endif  // TPL_R_STAR_TREE_H