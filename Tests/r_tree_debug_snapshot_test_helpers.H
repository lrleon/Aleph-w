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

namespace Aleph::test_helpers
{
  inline Rectangle union_of(const Rectangle &a, const Rectangle &b)
  {
    return Rectangle(a.get_xmin() < b.get_xmin() ? a.get_xmin() : b.get_xmin(),
                     a.get_ymin() < b.get_ymin() ? a.get_ymin() : b.get_ymin(),
                     a.get_xmax() > b.get_xmax() ? a.get_xmax() : b.get_xmax(),
                     a.get_ymax() > b.get_ymax() ? a.get_ymax() : b.get_ymax());
  }

  // Recursively check a DebugSnapshot: every child index is in range and one
  // depth deeper, every leaf's stored bbox is the tight union of its
  // entry_boxes, every internal node's bbox is the tight union of its
  // children's bboxes, and returns the total number of leaf entries found.
  template <typename Snapshot>
  size_t check_snapshot_node(const Snapshot &snap, const size_t idx, const size_t expected_depth)
  {
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
    size_t total = check_snapshot_node(snap, node.children(0), expected_depth + 1);
    for (size_t i = 1; i < node.children.size(); ++i)
      {
        EXPECT_LT(node.children(i), snap.nodes.size());
        if (node.children(i) >= snap.nodes.size())
          return total;
        total += check_snapshot_node(snap, node.children(i), expected_depth + 1);
        acc = union_of(acc, snap.nodes(node.children(i)).bbox);
      }
    EXPECT_EQ(node.bbox, acc);
    return total;
  }
}

#endif // TESTS_R_TREE_DEBUG_SNAPSHOT_TEST_HELPERS_H