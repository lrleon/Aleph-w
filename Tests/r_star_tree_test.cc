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
 * @file r_star_tree_test.cc
 * @brief Tests for Aleph::RStarTree (R*-tree heuristics).
 */

#include <gtest/gtest.h>

#include <algorithm>
#include <memory>
#include <random>
#include <utility>
#include <vector>

#include <tpl_r_star_tree.H>

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

  // Randomized parity + per-step invariant check for a given fanout.
  template <size_t Max, size_t Min>
  void randomized_parity(const unsigned seed, const int iters, const int space,
                         const int ext)
  {
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> op_dist(0, 2);
    std::uniform_int_distribution<int> coord(0, space);
    std::uniform_int_distribution<int> extent(0, ext);

    RStarTree<int, Max, Min> tree;
    std::vector<std::pair<Rectangle, int>> ref;
    int next_id = 0;

    auto random_rect = [&]()
    {
      const int x1 = coord(rng);
      const int y1 = coord(rng);
      return rect(x1, y1, x1 + extent(rng), y1 + extent(rng));
    };

    for (int iter = 0; iter < iters; ++iter)
      {
        const int op = ref.empty() ? 0 : op_dist(rng);
        if (op == 0)
          {
            const Rectangle b = random_rect();
            const int id = next_id++;
            tree.insert(b, id);
            ref.emplace_back(b, id);
          }
        else if (op == 1)
          {
            std::uniform_int_distribution<size_t> pick(0, ref.size() - 1);
            const size_t k = pick(rng);
            ASSERT_TRUE(tree.erase(ref[k].first, ref[k].second));
            ref.erase(ref.begin() + k);
          }
        else
          {
            const Rectangle q = random_rect();
            EXPECT_EQ(sorted_intersects(tree, q), brute_intersects(ref, q));
            const Point p = pt(coord(rng), coord(rng));
            EXPECT_EQ(sorted_contains(tree, p), brute_contains(ref, p));
          }

        ASSERT_TRUE(tree.verify()) << "invariant broken at iter " << iter;
        ASSERT_EQ(tree.size(), ref.size());
      }

    for (int x = 0; x <= space; x += 9)
      for (int y = 0; y <= space; y += 9)
        {
          const Rectangle q = rect(x, y, x + ext, y + ext);
          ASSERT_EQ(sorted_intersects(tree, q), brute_intersects(ref, q));
          ASSERT_EQ(sorted_contains(tree, pt(x, y)), brute_contains(ref, pt(x, y)));
        }
  }
}

TEST(RStarTree, EmptyTree)
{
  RStarTree<int> tree;
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_EQ(tree.height(), 0u);
  EXPECT_TRUE(tree.verify());
  EXPECT_FALSE(tree.erase(rect(0, 0, 1, 1), 5));
}

TEST(RStarTree, SingletonAndErase)
{
  RStarTree<int> tree;
  tree.insert(rect(0, 0, 2, 2), 42);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_EQ(sorted_intersects(tree, rect(1, 1, 5, 5)), (std::vector<int>{42}));
  EXPECT_TRUE(tree.erase(rect(0, 0, 2, 2), 42));
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.height(), 0u);
  EXPECT_TRUE(tree.verify());
}

TEST(RStarTree, ForcedReinsertionKeepsTreeValid)
{
  // Default fanout (16/8): forced reinsertion of ~30% of a leaf triggers on the
  // first leaf overflow of each insert once the tree has depth >= 2.
  RStarTree<int> tree;   // Max=16, Min=8
  for (int i = 0; i < 500; ++i)
    {
      tree.insert(rect(i % 50, i / 50, i % 50 + 3, i / 50 + 3), i);
      ASSERT_TRUE(tree.verify());
    }
  EXPECT_EQ(tree.size(), 500u);
  EXPECT_GT(tree.height(), 1u);

  // Every inserted box must still be found by an intersecting query.
  for (int i = 0; i < 500; ++i)
    {
      const Rectangle b = rect(i % 50, i / 50, i % 50 + 3, i / 50 + 3);
      const std::vector<int> hits = sorted_intersects(tree, b);
      EXPECT_TRUE(std::find(hits.begin(), hits.end(), i) != hits.end());
    }
}

TEST(RStarTree, CoincidentRectangles)
{
  RStarTree<int, 4, 2> tree;
  const Rectangle box = rect(3, 3, 6, 6);
  for (int i = 0; i < 25; ++i)
    tree.insert(box, i);
  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(sorted_intersects(tree, rect(4, 4, 5, 5)).size(), 25u);

  ASSERT_TRUE(tree.erase(box, 10));
  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(sorted_intersects(tree, box).size(), 24u);
}

TEST(RStarTree, DegenerateRectangles)
{
  RStarTree<int, 4, 2> tree;
  for (int i = 0; i < 40; ++i)
    tree.insert(rect(i, 5, i, 5), i);            // points
  for (int i = 0; i < 20; ++i)
    tree.insert(rect(0, i, 40, i), 100 + i);     // horizontal segments
  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 60u);

  const std::vector<int> hits = sorted_contains(tree, pt(10, 5));
  EXPECT_TRUE(std::find(hits.begin(), hits.end(), 10) != hits.end());
  EXPECT_TRUE(std::find(hits.begin(), hits.end(), 105) != hits.end());
}

TEST(RStarTree, CopyAndMove)
{
  RStarTree<int, 6, 3> original;
  for (int i = 0; i < 60; ++i)
    original.insert(rect(i, i, i + 2, i + 2), i);

  RStarTree<int, 6, 3> copy = original;
  ASSERT_TRUE(copy.verify());
  EXPECT_EQ(copy.size(), original.size());
  for (int i = 0; i < 30; ++i)
    ASSERT_TRUE(original.erase(rect(i, i, i + 2, i + 2), i));
  EXPECT_EQ(original.size(), 30u);
  EXPECT_EQ(copy.size(), 60u);
  ASSERT_TRUE(original.verify());
  ASSERT_TRUE(copy.verify());

  RStarTree<int, 6, 3> moved = std::move(copy);
  EXPECT_EQ(moved.size(), 60u);
  EXPECT_TRUE(moved.verify());
  EXPECT_TRUE(copy.is_empty());   // NOLINT(bugprone-use-after-move)
  EXPECT_TRUE(copy.verify());
}

TEST(RStarTree, SupportsMoveOnlyPayload)
{
  RStarTree<std::unique_ptr<int>> tree;
  for (int i = 0; i < 40; ++i)
    tree.insert(rect(i, i, i + 1, i + 1), std::make_unique<int>(i));
  ASSERT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 40u);

  int seen = 0;
  tree.for_each_intersecting(rect(0, 0, 100, 100),
    [&](const Rectangle &, const std::unique_ptr<int> &) { ++seen; });
  EXPECT_EQ(seen, 40);
}

TEST(RStarTree, AssignmentClearAndDrainRemainValid)
{
  RStarTree<int, 4, 2> source;
  std::vector<std::pair<Rectangle, int>> ref;
  for (int i = 0; i < 100; ++i)
    {
      const Rectangle b = rect(i % 20, i / 20, i % 20 + 3, i / 20 + 2);
      source.insert(b, i);
      ref.emplace_back(b, i);
    }
  ASSERT_TRUE(source.verify());

  RStarTree<int, 4, 2> assigned;
  assigned = source;
  EXPECT_EQ(assigned.size(), source.size());
  EXPECT_TRUE(assigned.verify());

  RStarTree<int, 4, 2> moved;
  moved = std::move(assigned);
  EXPECT_EQ(moved.size(), 100u);
  EXPECT_TRUE(moved.verify());
  EXPECT_TRUE(assigned.is_empty());   // NOLINT(bugprone-use-after-move)
  EXPECT_TRUE(assigned.verify());

  for (const auto &[b, id] : ref)
    {
      ASSERT_TRUE(moved.erase(b, id));
      ASSERT_TRUE(moved.verify());
    }
  EXPECT_TRUE(moved.is_empty());
  EXPECT_EQ(moved.height(), 0u);

  source.clear();
  EXPECT_TRUE(source.is_empty());
  EXPECT_TRUE(source.verify());
}

TEST(RStarTree, RandomizedParityDefaultFanout)
{
  randomized_parity<16, 8>(20260714u, 4000, 90, 12);
}

TEST(RStarTree, RandomizedParitySmallFanout)
{
  randomized_parity<4, 2>(913u, 3000, 60, 8);
}
