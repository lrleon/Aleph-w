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

/**
 * @file r_tree_test.cc
 * @brief Tests for Aleph::RTree.
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <concepts>
#include <memory>
#include <random>
#include <type_traits>
#include <utility>
#include <vector>

#include <tpl_r_tree.H>

using namespace Aleph;

namespace
{
  Rectangle rect(const int x1, const int y1, const int x2, const int y2)
  {
    return Rectangle(Geom_Number(x1), Geom_Number(y1),
                     Geom_Number(x2), Geom_Number(y2));
  }

  Point pt(const int x, const int y)
  {
    return Point(Geom_Number(x), Geom_Number(y));
  }

  template <typename Tree>
  std::vector<int> sorted_intersects(const Tree &tree, const Rectangle &q)
  {
    Array<int> hits = tree.search_intersects(q);
    std::vector<int> out;
    for (size_t i = 0; i < hits.size(); ++i)
      out.push_back(hits(i));
    std::sort(out.begin(), out.end());
    return out;
  }

  template <typename Tree>
  std::vector<int> sorted_contains(const Tree &tree, const Point &p)
  {
    Array<int> hits = tree.search_contains(p);
    std::vector<int> out;
    for (size_t i = 0; i < hits.size(); ++i)
      out.push_back(hits(i));
    std::sort(out.begin(), out.end());
    return out;
  }

  std::vector<int> brute_intersects(const std::vector<std::pair<Rectangle, int>> &ref,
                                    const Rectangle &q)
  {
    std::vector<int> out;
    for (const auto &[b, id] : ref)
      if (b.intersects(q))
        out.push_back(id);
    std::sort(out.begin(), out.end());
    return out;
  }

  std::vector<int> brute_contains(const std::vector<std::pair<Rectangle, int>> &ref,
                                  const Point &p)
  {
    std::vector<int> out;
    for (const auto &[b, id] : ref)
      if (b.contains(p))
        out.push_back(id);
    std::sort(out.begin(), out.end());
    return out;
  }

  struct CopyConstructibleNonCopyAssignablePayload
  {
    int value = 0;

    CopyConstructibleNonCopyAssignablePayload() = default;
    explicit CopyConstructibleNonCopyAssignablePayload(const int v) : value(v) {}

    CopyConstructibleNonCopyAssignablePayload(
      const CopyConstructibleNonCopyAssignablePayload &) = default;
    CopyConstructibleNonCopyAssignablePayload(
      CopyConstructibleNonCopyAssignablePayload &&) noexcept = default;

    CopyConstructibleNonCopyAssignablePayload &operator = (
      const CopyConstructibleNonCopyAssignablePayload &) = delete;
    CopyConstructibleNonCopyAssignablePayload &operator = (
      CopyConstructibleNonCopyAssignablePayload &&) noexcept = default;
  };

  static_assert(std::default_initializable<CopyConstructibleNonCopyAssignablePayload>);
  static_assert(std::movable<CopyConstructibleNonCopyAssignablePayload>);
  static_assert(std::is_copy_constructible_v<CopyConstructibleNonCopyAssignablePayload>);
  static_assert(not std::is_copy_assignable_v<CopyConstructibleNonCopyAssignablePayload>);
}

TEST(RTree, EmptyTree)
{
  RTree<int> tree;
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.height(), 0u);
  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.search_intersects(rect(0, 0, 10, 10)).size(), 0u);
  EXPECT_EQ(tree.search_contains(pt(1, 1)).size(), 0u);
  EXPECT_FALSE(tree.erase(rect(0, 0, 1, 1), 5));
}

TEST(RTree, SingletonInsertAndQuery)
{
  RTree<int> tree;
  tree.insert(rect(0, 0, 2, 2), 42);

  EXPECT_FALSE(tree.is_empty());
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(tree.height(), 1u);
  EXPECT_TRUE(tree.verify());

  EXPECT_EQ(sorted_intersects(tree, rect(1, 1, 5, 5)), (std::vector<int>{42}));
  EXPECT_EQ(sorted_intersects(tree, rect(10, 10, 20, 20)), (std::vector<int>{}));
  EXPECT_EQ(sorted_contains(tree, pt(1, 1)), (std::vector<int>{42}));
  EXPECT_EQ(sorted_contains(tree, pt(9, 9)), (std::vector<int>{}));
}

TEST(RTree, EraseSingletonEmptiesTree)
{
  RTree<int> tree;
  tree.insert(rect(0, 0, 2, 2), 1);
  EXPECT_TRUE(tree.erase(rect(0, 0, 2, 2), 1));
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.height(), 0u);
  EXPECT_TRUE(tree.verify());
}

TEST(RTree, EraseNonexistentReturnsFalse)
{
  RTree<int> tree;
  tree.insert(rect(0, 0, 2, 2), 1);
  EXPECT_FALSE(tree.erase(rect(0, 0, 2, 2), 999));   // wrong value
  EXPECT_FALSE(tree.erase(rect(5, 5, 6, 6), 1));     // wrong bbox
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
}

TEST(RTree, GrowsInHeightWithManyInserts)
{
  RTree<int, 4, 2> tree;   // tiny fanout forces splits early
  for (int i = 0; i < 200; ++i)
    {
      tree.insert(rect(i, i, i + 1, i + 1), i);
      ASSERT_TRUE(tree.verify());
    }
  EXPECT_EQ(tree.size(), 200u);
  EXPECT_GT(tree.height(), 1u);

  // Every inserted box must be findable at its own corner.
  for (int i = 0; i < 200; ++i)
    {
      const std::vector<int> hits = sorted_contains(tree, pt(i, i));
      EXPECT_TRUE(std::find(hits.begin(), hits.end(), i) != hits.end());
    }
}

TEST(RTree, CoincidentRectangles)
{
  RTree<int, 4, 2> tree;
  const Rectangle box = rect(3, 3, 6, 6);
  for (int i = 0; i < 20; ++i)
    tree.insert(box, i);   // identical bbox, distinct payloads

  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 20u);
  EXPECT_EQ(sorted_intersects(tree, rect(4, 4, 5, 5)).size(), 20u);

  // Removing one leaves the other nineteen.
  ASSERT_TRUE(tree.erase(box, 7));
  ASSERT_TRUE(tree.verify());
  const std::vector<int> hits = sorted_intersects(tree, box);
  EXPECT_EQ(hits.size(), 19u);
  EXPECT_TRUE(std::find(hits.begin(), hits.end(), 7) == hits.end());
}

TEST(RTree, DegenerateRectangles)
{
  RTree<int, 4, 2> tree;
  // Points (zero area) and lines (zero width or height).
  for (int i = 0; i < 30; ++i)
    tree.insert(rect(i, 5, i, 5), i);          // points on a horizontal line
  for (int i = 0; i < 30; ++i)
    tree.insert(rect(0, i, 40, i), 100 + i);   // horizontal segments

  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 60u);

  // The point (10,5) is a stored point and lies on segment y=5.
  const std::vector<int> hits = sorted_contains(tree, pt(10, 5));
  EXPECT_TRUE(std::find(hits.begin(), hits.end(), 10) != hits.end());
  EXPECT_TRUE(std::find(hits.begin(), hits.end(), 105) != hits.end());
}

TEST(RTree, CopyIsIndependentDeepClone)
{
  RTree<int, 4, 2> original;
  for (int i = 0; i < 50; ++i)
    original.insert(rect(i, i, i + 2, i + 2), i);

  RTree<int, 4, 2> copy = original;   // deep copy
  ASSERT_TRUE(copy.verify());
  EXPECT_EQ(copy.size(), original.size());

  // Mutate the original; the copy must not change.
  for (int i = 0; i < 25; ++i)
    ASSERT_TRUE(original.erase(rect(i, i, i + 2, i + 2), i));

  ASSERT_TRUE(original.verify());
  ASSERT_TRUE(copy.verify());
  EXPECT_EQ(original.size(), 25u);
  EXPECT_EQ(copy.size(), 50u);
  EXPECT_EQ(sorted_contains(copy, pt(1, 1)).empty(), false);
  EXPECT_TRUE(sorted_contains(original, pt(1, 1)).empty());
}

TEST(RTree, MoveConstructionTransfersOwnership)
{
  RTree<int, 4, 2> a;
  for (int i = 0; i < 30; ++i)
    a.insert(rect(i, 0, i + 1, 1), i);

  RTree<int, 4, 2> b = std::move(a);
  EXPECT_EQ(b.size(), 30u);
  EXPECT_TRUE(b.verify());
  EXPECT_TRUE(a.is_empty());   // NOLINT(bugprone-use-after-move)
  EXPECT_TRUE(a.verify());
}

TEST(RTree, SupportsMoveOnlyPayload)
{
  RTree<std::unique_ptr<int>, 4, 2> tree;
  for (int i = 0; i < 20; ++i)
    tree.insert(rect(i, i, i + 1, i + 1), std::make_unique<int>(i));

  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 20u);

  int seen = 0;
  int sum = 0;
  tree.for_each_intersecting(rect(0, 0, 100, 100),
    [&](const Rectangle &, const std::unique_ptr<int> &v)
    {
      ++seen;
      sum += *v;
    });
  EXPECT_EQ(seen, 20);
  EXPECT_EQ(sum, 190);   // 0 + 1 + ... + 19
}

TEST(RTree, CopyConstructiblePayloadNeedNotBeCopyAssignable)
{
  using Payload = CopyConstructibleNonCopyAssignablePayload;

  RTree<Payload, 4, 2> tree;
  const Payload first(7);
  tree.insert(rect(0, 0, 2, 2), first);
  tree.insert(rect(3, 3, 5, 5), Payload(11));

  const RTree<Payload, 4, 2> copy(tree);
  ASSERT_TRUE(copy.verify());

  Array<Payload> intersecting = copy.search_intersects(rect(1, 1, 4, 4));
  ASSERT_EQ(intersecting.size(), 2u);
  EXPECT_EQ(intersecting(0).value + intersecting(1).value, 18);

  Array<Payload> containing = copy.search_contains(pt(1, 1));
  ASSERT_EQ(containing.size(), 1u);
  EXPECT_EQ(containing(0).value, 7);
}

TEST(RTree, AssignmentClearAndDrainRemainValid)
{
  RTree<int, 4, 2> source;
  std::vector<std::pair<Rectangle, int>> ref;
  for (int i = 0; i < 90; ++i)
    {
      const Rectangle b = rect(i % 15, i / 15, i % 15 + 2, i / 15 + 2);
      source.insert(b, i);
      ref.emplace_back(b, i);
    }
  ASSERT_TRUE(source.verify());

  RTree<int, 4, 2> assigned;
  assigned = source;
  EXPECT_EQ(assigned.size(), source.size());
  EXPECT_TRUE(assigned.verify());

  for (int i = 0; i < 30; ++i)
    ASSERT_TRUE(source.erase(ref[i].first, ref[i].second));
  EXPECT_EQ(source.size(), 60u);
  EXPECT_EQ(assigned.size(), 90u);
  EXPECT_TRUE(source.verify());
  EXPECT_TRUE(assigned.verify());

  RTree<int, 4, 2> moved;
  moved = std::move(assigned);
  EXPECT_EQ(moved.size(), 90u);
  EXPECT_TRUE(moved.verify());
  EXPECT_TRUE(assigned.is_empty());   // NOLINT(bugprone-use-after-move)
  EXPECT_TRUE(assigned.verify());

  moved.clear();
  EXPECT_TRUE(moved.is_empty());
  EXPECT_EQ(moved.height(), 0u);
  EXPECT_TRUE(moved.verify());

  for (const auto &[b, id] : ref)
    {
      ASSERT_TRUE(source.erase(b, id) or id < 30);
      ASSERT_TRUE(source.verify());
    }
  EXPECT_TRUE(source.is_empty());
  EXPECT_EQ(source.height(), 0u);
}

TEST(RTree, RandomizedParityAgainstBruteForce)
{
  std::mt19937 rng(20260714u);   // fixed seed
  std::uniform_int_distribution<int> op_dist(0, 2);
  std::uniform_int_distribution<int> coord(0, 80);
  std::uniform_int_distribution<int> extent(0, 12);

  RTree<int, 6, 3> tree;
  std::vector<std::pair<Rectangle, int>> ref;
  int next_id = 0;

  auto random_rect = [&]()
  {
    const int x1 = coord(rng);
    const int y1 = coord(rng);
    return rect(x1, y1, x1 + extent(rng), y1 + extent(rng));
  };

  for (int iter = 0; iter < 4000; ++iter)
    {
      const int op = ref.empty() ? 0 : op_dist(rng);
      if (op == 0)   // insert
        {
          const Rectangle b = random_rect();
          const int id = next_id++;
          tree.insert(b, id);
          ref.emplace_back(b, id);
        }
      else if (op == 1)   // erase a random existing entry
        {
          std::uniform_int_distribution<size_t> pick(0, ref.size() - 1);
          const size_t k = pick(rng);
          ASSERT_TRUE(tree.erase(ref[k].first, ref[k].second));
          ref.erase(ref.begin() + k);
        }
      else   // query and compare against brute force
        {
          const Rectangle q = random_rect();
          EXPECT_EQ(sorted_intersects(tree, q), brute_intersects(ref, q));
          const Point p = pt(coord(rng), coord(rng));
          EXPECT_EQ(sorted_contains(tree, p), brute_contains(ref, p));
        }

      ASSERT_TRUE(tree.verify()) << "invariant broken at iter " << iter;
      ASSERT_EQ(tree.size(), ref.size());
    }

  // Final exhaustive parity over a grid of query rectangles.
  for (int x = 0; x < 90; x += 7)
    for (int y = 0; y < 90; y += 7)
      {
        const Rectangle q = rect(x, y, x + 10, y + 10);
        ASSERT_EQ(sorted_intersects(tree, q), brute_intersects(ref, q));
      }
}
