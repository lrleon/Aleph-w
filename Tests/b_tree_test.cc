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

/**
 * @file b_tree_test.cc
 * @brief Tests for B-Tree (tpl_b_tree.H)
 */

#include <algorithm>
#include <random>
#include <set>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_b_tree.H>

using namespace Aleph;
using namespace testing;

namespace
{
  template <typename T>
  std::vector<T> to_vector(const Array<T> & arr)
  {
    std::vector<T> out;
    out.reserve(arr.size());
    for (size_t i = 0; i < arr.size(); ++i)
      out.push_back(arr[i]);
    return out;
  }

  template <class Tree>
  std::vector<typename Tree::key_type> to_vector(const Tree & tree)
  {
    return to_vector(tree.keys());
  }

  template <class Tree>
  void expect_matches_reference(const Tree & tree, const std::set<int> & ref)
  {
    const std::vector<int> tree_keys = to_vector(tree);
    const std::vector<int> ref_keys(ref.begin(), ref.end());
    const auto min_key = tree.min_key();
    const auto max_key = tree.max_key();
    EXPECT_EQ(tree_keys, ref_keys);
    EXPECT_EQ(tree.size(), ref.size());
    EXPECT_TRUE(tree.verify());

    if (ref.empty())
      {
        EXPECT_FALSE(min_key.has_value());
        EXPECT_FALSE(max_key.has_value());
      }
    else
      {
        ASSERT_TRUE(min_key.has_value());
        ASSERT_TRUE(max_key.has_value());
        EXPECT_EQ(*min_key, *ref.begin());
        EXPECT_EQ(*max_key, *ref.rbegin());
      }
  }

  void expect_optional_eq(const std::optional<int> & got,
                          const std::optional<int> & expected)
  {
    ASSERT_EQ(got.has_value(), expected.has_value());
    if (got.has_value())
      EXPECT_EQ(*got, *expected);
  }
}

class BTreeTest : public Test
{
protected:
  using Tree = B_Tree<int, Aleph::less<int>, 3>;
};

TEST_F(BTreeTest, EmptyTreeReportsNoKeys)
{
  Tree tree;

  EXPECT_TRUE(tree.empty());
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.height(), 0);
  EXPECT_TRUE(to_vector(tree).empty());
  EXPECT_FALSE(tree.min_key().has_value());
  EXPECT_FALSE(tree.max_key().has_value());
  EXPECT_FALSE(tree.lower_bound(10).has_value());
  EXPECT_FALSE(tree.upper_bound(10).has_value());
  EXPECT_TRUE(tree.verify());
}

TEST_F(BTreeTest, InitializerListAndRangeConstructorSortAndDeduplicate)
{
  Tree from_init = {40, 10, 90, 10, 20, 70, 40};
  std::vector<int> src = {8, 3, 5, 8, 2, 9, 1};
  Tree from_range(src.begin(), src.end());

  EXPECT_EQ(to_vector(from_init), (std::vector<int>{10, 20, 40, 70, 90}));
  EXPECT_EQ(to_vector(from_range), (std::vector<int>{1, 2, 3, 5, 8, 9}));
  EXPECT_TRUE(from_init.verify());
  EXPECT_TRUE(from_range.verify());
}

TEST_F(BTreeTest, InsertSearchAndBoundsWorkAcrossInternalNodes)
{
  Tree tree;
  for (int value : {40, 10, 90, 20, 70, 60, 30, 50, 80})
    EXPECT_TRUE(tree.insert(value));

  EXPECT_FALSE(tree.insert(70));
  EXPECT_TRUE(tree.contains(10));
  EXPECT_TRUE(tree.search(80));
  EXPECT_FALSE(tree.contains(11));

  EXPECT_EQ(to_vector(tree), (std::vector<int>{10, 20, 30, 40, 50, 60, 70, 80, 90}));
  EXPECT_EQ(tree.height(), 2);

  expect_optional_eq(tree.lower_bound(55), 60);
  expect_optional_eq(tree.lower_bound(60), 60);
  expect_optional_eq(tree.lower_bound(91), std::nullopt);

  expect_optional_eq(tree.upper_bound(60), 70);
  expect_optional_eq(tree.upper_bound(89), 90);
  expect_optional_eq(tree.upper_bound(90), std::nullopt);

  EXPECT_TRUE(tree.verify());
}

TEST_F(BTreeTest, RemoveMaintainsOrderAndInvariants)
{
  Tree tree;
  std::set<int> ref;

  for (int i = 1; i <= 40; ++i)
    {
      tree.insert(i);
      ref.insert(i);
    }

  for (int value : {1, 2, 3, 7, 8, 9, 16, 17, 18, 25, 26, 27, 40, 31, 24, 12})
    {
      SCOPED_TRACE(value);
      EXPECT_TRUE(tree.remove(value));
      ref.erase(value);
      expect_matches_reference(tree, ref);
    }

  EXPECT_FALSE(tree.remove(999));
  expect_matches_reference(tree, ref);
}

TEST_F(BTreeTest, CopyAndMovePreserveKeys)
{
  Tree tree = {50, 20, 70, 10, 30, 60, 80};

  Tree copy(tree);
  Tree assigned;
  assigned = tree;

  EXPECT_EQ(to_vector(copy), to_vector(tree));
  EXPECT_EQ(to_vector(assigned), to_vector(tree));
  EXPECT_TRUE(copy.verify());
  EXPECT_TRUE(assigned.verify());

  Tree moved(std::move(copy));
  Tree move_assigned;
  move_assigned = std::move(assigned);

  EXPECT_EQ(to_vector(moved), to_vector(tree));
  EXPECT_EQ(to_vector(move_assigned), to_vector(tree));
  EXPECT_TRUE(moved.verify());
  EXPECT_TRUE(move_assigned.verify());
}

TEST_F(BTreeTest, RandomizedOperationsMatchStdSetOracle)
{
  Tree tree;
  std::set<int> ref;
  std::mt19937 rng(0xBADA55);
  std::uniform_int_distribution<int> op_dist(0, 5);
  std::uniform_int_distribution<int> value_dist(-50, 150);

  for (size_t step = 0; step < 1200; ++step)
    {
      SCOPED_TRACE(step);
      const int value = value_dist(rng);

      switch (op_dist(rng))
        {
        case 0:
        case 1:
          EXPECT_EQ(tree.insert(value), ref.insert(value).second);
          break;

        case 2:
          EXPECT_EQ(tree.remove(value), ref.erase(value) == 1);
          break;

        case 3:
          EXPECT_EQ(tree.contains(value), ref.contains(value));
          break;

        case 4:
          {
            const auto it = ref.lower_bound(value);
            const std::optional<int> expected =
                it == ref.end() ? std::nullopt : std::optional<int>(*it);
            expect_optional_eq(tree.lower_bound(value), expected);
          }
          break;

        default:
          {
            const auto it = ref.upper_bound(value);
            const std::optional<int> expected =
                it == ref.end() ? std::nullopt : std::optional<int>(*it);
            expect_optional_eq(tree.upper_bound(value), expected);
          }
          break;
        }

      expect_matches_reference(tree, ref);
    }
}
