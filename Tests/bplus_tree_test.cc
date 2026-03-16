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
 * @file bplus_tree_test.cc
 * @brief Tests for B+ Tree (tpl_bplus_tree.H)
 */

#include <algorithm>
#include <random>
#include <set>
#include <stdexcept>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_bplus_tree.H>

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
    EXPECT_EQ(tree_keys, ref_keys);
    EXPECT_EQ(tree.size(), ref.size());
    EXPECT_TRUE(tree.verify());

    if (ref.empty())
      {
        EXPECT_FALSE(tree.min_key().has_value());
        EXPECT_FALSE(tree.max_key().has_value());
      }
    else
      {
        ASSERT_TRUE(tree.min_key().has_value());
        ASSERT_TRUE(tree.max_key().has_value());
        EXPECT_EQ(*tree.min_key(), *ref.begin());
        EXPECT_EQ(*tree.max_key(), *ref.rbegin());
      }
  }

  void expect_optional_eq(const std::optional<int> & got,
                          const std::optional<int> & expected)
  {
    ASSERT_EQ(got.has_value(), expected.has_value());
    if (got.has_value())
      EXPECT_EQ(*got, *expected);
  }

  std::vector<int> ref_range(const std::set<int> & ref, int first, int last)
  {
    std::vector<int> out;
    if (last < first)
      return out;

    for (auto it = ref.lower_bound(first);
         it != ref.end() and *it <= last;
         ++it)
      out.push_back(*it);
    return out;
  }
}

class BPlusTreeTest : public Test
{
protected:
  using Tree = BPlus_Tree<int, Aleph::less<int>, 3>;
};

TEST_F(BPlusTreeTest, EmptyTreeReportsNoKeys)
{
  Tree tree;

  EXPECT_TRUE(tree.empty());
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
  EXPECT_EQ(tree.height(), 0);
  EXPECT_TRUE(to_vector(tree).empty());
  EXPECT_TRUE(to_vector(tree.range(0, 10)).empty());
  EXPECT_FALSE(tree.min_key().has_value());
  EXPECT_FALSE(tree.max_key().has_value());
  EXPECT_FALSE(tree.lower_bound(10).has_value());
  EXPECT_FALSE(tree.upper_bound(10).has_value());
  EXPECT_TRUE(tree.verify());
}

TEST_F(BPlusTreeTest, InitializerListSortsDeduplicatesAndSupportsRangeQueries)
{
  Tree tree = {40, 10, 90, 10, 20, 70, 40, 60, 30, 50};

  EXPECT_EQ(to_vector(tree), (std::vector<int>{10, 20, 30, 40, 50, 60, 70, 90}));
  EXPECT_EQ(to_vector(tree.range(25, 65)), (std::vector<int>{30, 40, 50, 60}));
  EXPECT_EQ(to_vector(tree.range(10, 10)), (std::vector<int>{10}));
  EXPECT_TRUE(tree.verify());
}

TEST_F(BPlusTreeTest, LowerAndUpperBoundCrossLeafBoundaries)
{
  Tree tree;
  for (int i = 1; i <= 40; ++i)
    EXPECT_TRUE(tree.insert(i * 3));

  EXPECT_GT(tree.height(), 2);
  expect_optional_eq(tree.lower_bound(1), 3);
  expect_optional_eq(tree.lower_bound(58), 60);
  expect_optional_eq(tree.lower_bound(120), 120);
  expect_optional_eq(tree.lower_bound(121), std::nullopt);

  expect_optional_eq(tree.upper_bound(57), 60);
  expect_optional_eq(tree.upper_bound(120), std::nullopt);
  EXPECT_EQ(to_vector(tree.range(27, 42)), (std::vector<int>{27, 30, 33, 36, 39, 42}));
  EXPECT_TRUE(tree.verify());
}

TEST_F(BPlusTreeTest, LazyIteratorsTraverseLeafChain)
{
  Tree tree;
  for (int value : {105, 110, 115, 120, 125, 130, 135, 140, 145, 150})
    EXPECT_TRUE(tree.insert(value));

  std::vector<int> all_keys;
  for (auto it = tree.get_it(); it.has_curr(); it.next_ne())
    all_keys.push_back(it.get_curr());
  EXPECT_EQ(all_keys,
            (std::vector<int>{105, 110, 115, 120, 125, 130, 135, 140, 145, 150}));

  std::vector<int> band;
  auto band_it = tree.get_range_it(118, 141);
  ASSERT_TRUE(band_it.has_curr());
  EXPECT_EQ(*band_it, 120);
  band.push_back(band_it.get_curr());
  band_it.next();
  for (; band_it.has_curr(); band_it.next_ne())
    band.push_back(band_it.get_curr());

  EXPECT_EQ(band, (std::vector<int>{120, 125, 130, 135, 140}));
}

TEST_F(BPlusTreeTest, RemoveMaintainsLeafLinksAndOrder)
{
  Tree tree;
  std::set<int> ref;

  for (int i = 1; i <= 60; ++i)
    {
      tree.insert(i);
      ref.insert(i);
    }

  for (int value : {1, 2, 3, 6, 7, 8, 17, 18, 19, 28, 29, 30, 42, 43, 44, 60, 59})
    {
      EXPECT_TRUE(tree.remove(value));
      ref.erase(value);
      expect_matches_reference(tree, ref);
    }

  EXPECT_FALSE(tree.remove(999));
  EXPECT_EQ(to_vector(tree.range(20, 27)), ref_range(ref, 20, 27));
  expect_matches_reference(tree, ref);
}

TEST_F(BPlusTreeTest, CopyAndMovePreserveContents)
{
  Tree tree = {50, 20, 70, 10, 30, 60, 80, 25, 27};

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

TEST_F(BPlusTreeTest, InvalidRangeThrows)
{
  Tree tree = {1, 2, 3};
  EXPECT_THROW((void) tree.range(10, 5), std::invalid_argument);
}

TEST_F(BPlusTreeTest, RandomizedOperationsMatchStdSetOracle)
{
  Tree tree;
  std::set<int> ref;
  std::mt19937 rng(0xBEEFBEEF);
  std::uniform_int_distribution<int> op_dist(0, 6);
  std::uniform_int_distribution<int> value_dist(-80, 180);

  for (size_t step = 0; step < 1400; ++step)
    {
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

        case 5:
          {
            const auto it = ref.upper_bound(value);
            const std::optional<int> expected =
                it == ref.end() ? std::nullopt : std::optional<int>(*it);
            expect_optional_eq(tree.upper_bound(value), expected);
          }
          break;

        default:
          {
            int a = value_dist(rng);
            int b = value_dist(rng);
            if (a > b)
              std::swap(a, b);
            EXPECT_EQ(to_vector(tree.range(a, b)), ref_range(ref, a, b));
          }
          break;
        }

      expect_matches_reference(tree, ref);
    }
}
