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

/** @file r_tree_debug_snapshot_test_helpers.H
 *  @brief Shared DebugSnapshot structural-invariant checker for RTree and
 *  RStarTree unit tests.
 */

#ifndef TESTS_R_TREE_DEBUG_SNAPSHOT_TEST_HELPERS_H
#define TESTS_R_TREE_DEBUG_SNAPSHOT_TEST_HELPERS_H

#include <gtest/gtest.h>
#include <point.H>
#include <tpl_array.H>

namespace Aleph::test_helpers
{
  /** @brief Computes the tight axis-aligned union of two rectangles.
   *  @param a First rectangle.
   *  @param b Second rectangle.
   *  @return The smallest axis-aligned rectangle enclosing both @p a and @p b.
   *  @throws Never throws.
   *  @note O(1) time and space. Pure function, hence thread-safe (no shared
   *  state, no side effects).
   */
  inline Rectangle union_of(const Rectangle &a, const Rectangle &b)
  {
    return Rectangle(a.get_xmin() < b.get_xmin() ? a.get_xmin() : b.get_xmin(),
                     a.get_ymin() < b.get_ymin() ? a.get_ymin() : b.get_ymin(),
                     a.get_xmax() > b.get_xmax() ? a.get_xmax() : b.get_xmax(),
                     a.get_ymax() > b.get_ymax() ? a.get_ymax() : b.get_ymax());
  }

  namespace detail
  {
    template <typename Snapshot>
    size_t check_snapshot_node_impl(const Snapshot &snap, const size_t idx,
                                     const size_t expected_depth, Array<bool> &visited)
    {
      if (idx >= snap.nodes.size())
        {
          ADD_FAILURE() << "node index " << idx << " out of range (nodes.size() = "
                         << snap.nodes.size() << ")";
          return 0;
        }
      if (visited(idx))
        {
          ADD_FAILURE() << "cycle detected: node index " << idx << " reached more than once";
          return 0;
        }
      visited(idx) = true;

      const auto &node = snap.nodes(idx);
      EXPECT_EQ(node.depth, expected_depth);

      if (node.is_leaf)
        {
          EXPECT_FALSE(node.entry_boxes.is_empty());
          if (node.entry_boxes.is_empty())
            return 0;
          Rectangle acc = node.entry_boxes(0);
          for (size_t i = 1; i < node.entry_boxes.size(); ++i)
            acc = union_of(acc, node.entry_boxes(i));
          EXPECT_EQ(node.bbox, acc);
          return node.entry_boxes.size();
        }

      EXPECT_FALSE(node.children.is_empty());
      if (node.children.is_empty())
        return 0;
      EXPECT_LT(node.children(0), snap.nodes.size());
      if (node.children(0) >= snap.nodes.size())
        return 0;
      Rectangle acc = snap.nodes(node.children(0)).bbox;
      size_t total = check_snapshot_node_impl(snap, node.children(0), expected_depth + 1, visited);
      for (size_t i = 1; i < node.children.size(); ++i)
        {
          EXPECT_LT(node.children(i), snap.nodes.size());
          if (node.children(i) >= snap.nodes.size())
            return total;
          total += check_snapshot_node_impl(snap, node.children(i), expected_depth + 1, visited);
          acc = union_of(acc, snap.nodes(node.children(i)).bbox);
        }
      EXPECT_EQ(node.bbox, acc);
      return total;
    }
  }

  /** @brief Recursively validates the structural invariants of a
   *  `DebugSnapshot` produced by `RTree::debug_snapshot()` or
   *  `RStarTree::debug_snapshot()`, reporting GTest failures on mismatch.
   *
   *  Checks, for the subtree rooted at @p idx: every node's recorded depth
   *  matches @p expected_depth (increasing by one per level), every leaf's
   *  stored bbox is the tight union of its `entry_boxes`, every internal
   *  node's bbox is the tight union of its children's bboxes, and every
   *  child index is in range. An out-of-range @p idx (including an empty
   *  snapshot) or a repeated node index (cycle) is reported as a test
   *  failure via GTest's non-fatal `ADD_FAILURE`/`EXPECT_*` macros instead
   *  of causing out-of-bounds access or unbounded recursion.
   *
   *  @param snap Snapshot to validate.
   *  @param idx Index into `snap.nodes` of the subtree root to check.
   *  @param expected_depth Depth expected for the node at @p idx.
   *  @return Total number of leaf entries found in the subtree, or a partial
   *  count (possibly 0) if an invariant violation was detected.
   *  @pre None; @p idx and the reachability of nodes are validated
   *  internally.
   *  @throws Never throws (violations are reported via GTest assertions,
   *  not exceptions).
   *  @note O(n) time and O(n) auxiliary space, where n = `snap.nodes.size()`
   *  (each node is visited at most once thanks to the cycle guard). Not
   *  thread-safe: relies on GTest's current-test-scoped assertion state and
   *  must not be called concurrently on the same test.
   */
  template <typename Snapshot>
  size_t check_snapshot_node(const Snapshot &snap, const size_t idx, const size_t expected_depth)
  {
    Array<bool> visited(snap.nodes.size(), false);
    return detail::check_snapshot_node_impl(snap, idx, expected_depth, visited);
  }
}

#endif // TESTS_R_TREE_DEBUG_SNAPSHOT_TEST_HELPERS_H