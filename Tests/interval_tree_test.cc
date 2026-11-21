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
 * @file interval_tree_test.cc
 * @brief Comprehensive tests for Interval<T>, Interval_Tree, and DynIntervalTree.
 */

# include <gtest/gtest.h>
# include <tpl_interval_tree.H>
# include <random>
# include <algorithm>
# include <string>
# include <vector>
# include <type_traits>

using namespace Aleph;
using namespace testing;

static_assert(not std::is_copy_constructible_v<Interval_Tree<int>>);
static_assert(not std::is_copy_assignable_v<Interval_Tree<int>>);
static_assert(not std::is_move_constructible_v<Interval_Tree<int>>);
static_assert(not std::is_move_assignable_v<Interval_Tree<int>>);

struct CountingLess
{
  int *counter;

  explicit CountingLess(int *c = nullptr) noexcept : counter(c) {}

  bool operator ()(const int &a, const int &b) const noexcept
  {
    if (counter != nullptr)
      ++*counter;
    return a < b;
  }
};

// ─────────────────────────────────────────────────────────────────
// Interval<T> value type tests
// ─────────────────────────────────────────────────────────────────

class IntervalTypeTest : public Test {};

TEST_F(IntervalTypeTest, Construction)
{
  Interval<int> a(1, 5);
  EXPECT_EQ(a.low, 1);
  EXPECT_EQ(a.high, 5);

  auto pt = Interval<int>::point(3);
  EXPECT_EQ(pt.low, 3);
  EXPECT_EQ(pt.high, 3);
}

TEST_F(IntervalTypeTest, Validity)
{
  Interval<int> valid(1, 5);
  EXPECT_TRUE(valid.is_valid());

  Interval<int> point(3, 3);
  EXPECT_TRUE(point.is_valid());

  Interval<int> invalid(5, 1);
  EXPECT_FALSE(invalid.is_valid());
}

TEST_F(IntervalTypeTest, OverlapDetection)
{
  Interval<int> a(1, 5);
  Interval<int> b(3, 7);
  Interval<int> c(6, 10);
  Interval<int> d(1, 5);
  Interval<int> e(10, 15);

  EXPECT_TRUE(a.overlaps(b));   // partial overlap
  EXPECT_FALSE(a.overlaps(c));  // a.high=5 < c.low=6
  EXPECT_TRUE(a.overlaps(d));   // identical
  EXPECT_FALSE(a.overlaps(e));  // disjoint

  // touching endpoints count as overlap for closed intervals
  Interval<int> f(5, 10);
  EXPECT_TRUE(a.overlaps(f));
}

TEST_F(IntervalTypeTest, Containment)
{
  Interval<int> a(1, 5);
  EXPECT_TRUE(a.contains(1));
  EXPECT_TRUE(a.contains(3));
  EXPECT_TRUE(a.contains(5));
  EXPECT_FALSE(a.contains(0));
  EXPECT_FALSE(a.contains(6));
}

TEST_F(IntervalTypeTest, PointIntervals)
{
  auto pt = Interval<int>::point(5);
  EXPECT_TRUE(pt.is_valid());
  EXPECT_TRUE(pt.contains(5));
  EXPECT_FALSE(pt.contains(4));
  EXPECT_FALSE(pt.contains(6));

  Interval<int> a(3, 7);
  EXPECT_TRUE(pt.overlaps(a));

  Interval<int> b(6, 10);
  EXPECT_FALSE(pt.overlaps(b));
}

TEST_F(IntervalTypeTest, Ordering)
{
  Interval_Less<int> cmp;
  Interval<int> a(1, 5);
  Interval<int> b(2, 3);
  Interval<int> c(1, 7);

  EXPECT_TRUE(cmp(a, b));   // 1 < 2
  EXPECT_FALSE(cmp(b, a));
  EXPECT_TRUE(cmp(a, c));   // same low, 5 < 7
  EXPECT_FALSE(cmp(c, a));
  EXPECT_FALSE(cmp(a, a));  // irreflexive
}

// ─────────────────────────────────────────────────────────────────
// Low-level Interval_Tree tests
// ─────────────────────────────────────────────────────────────────

class IntervalTreeRawTest : public Test
{
protected:
  using IvTree = Interval_Tree<int>;
  using Node = IvTree::Node;
  using Key = Interval<int>;

  IvTree tree;

  void SetUp() override { tree.set_seed(42); }

  void TearDown() override
  {
    destroyRec(tree.getRoot());
  }

  Node * make_node(int lo, int hi)
  {
    return new Node(Key(lo, hi));
  }
};

TEST_F(IntervalTreeRawTest, EmptyTree)
{
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.search(Key(0, 0)), nullptr);
  EXPECT_EQ(tree.overlap_search(Key(0, 10)), nullptr);
}

TEST_F(IntervalTreeRawTest, SingleInsert)
{
  auto * p = make_node(1, 5);
  EXPECT_NE(tree.insert(p), nullptr);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
  EXPECT_NE(tree.search(Key(1, 5)), nullptr);
}

TEST_F(IntervalTreeRawTest, MultipleInserts)
{
  tree.insert(make_node(15, 20));
  tree.insert(make_node(10, 30));
  tree.insert(make_node(17, 19));
  tree.insert(make_node(5, 20));
  tree.insert(make_node(12, 15));
  tree.insert(make_node(30, 40));

  EXPECT_EQ(tree.size(), 6u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(IntervalTreeRawTest, DuplicateInsert)
{
  auto * p1 = make_node(1, 5);
  auto * p2 = make_node(1, 5);
  EXPECT_NE(tree.insert(p1), nullptr);
  EXPECT_EQ(tree.insert(p2), nullptr);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
  delete p2;
}

TEST_F(IntervalTreeRawTest, InsertDup)
{
  auto * p1 = make_node(1, 5);
  auto * p2 = make_node(1, 5);
  tree.insert_dup(p1);
  tree.insert_dup(p2);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(IntervalTreeRawTest, Remove)
{
  tree.insert(make_node(1, 5));
  tree.insert(make_node(10, 20));
  tree.insert(make_node(3, 7));

  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());

  auto * removed = tree.remove(Key(10, 20));
  EXPECT_NE(removed, nullptr);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());
  delete removed;
}

TEST_F(IntervalTreeRawTest, RemoveNonexistent)
{
  tree.insert(make_node(1, 5));
  auto * removed = tree.remove(Key(99, 100));
  EXPECT_EQ(removed, nullptr);
  EXPECT_EQ(tree.size(), 1u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(IntervalTreeRawTest, OverlapSearchFound)
{
  tree.insert(make_node(15, 20));
  tree.insert(make_node(10, 30));
  tree.insert(make_node(17, 19));
  tree.insert(make_node(5, 20));
  tree.insert(make_node(12, 15));
  tree.insert(make_node(30, 40));

  auto * result = tree.overlap_search(Key(14, 16));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(KEY(result).overlaps(Key(14, 16)));
}

TEST_F(IntervalTreeRawTest, OverlapSearchNotFound)
{
  tree.insert(make_node(1, 5));
  tree.insert(make_node(10, 15));
  tree.insert(make_node(20, 25));

  auto * result = tree.overlap_search(Key(6, 9));
  EXPECT_EQ(result, nullptr);
}

TEST_F(IntervalTreeRawTest, AllOverlaps)
{
  tree.insert(make_node(1, 5));
  tree.insert(make_node(3, 8));
  tree.insert(make_node(7, 10));
  tree.insert(make_node(15, 20));

  DynList<Key> results;
  tree.all_overlaps(Key(4, 7), [&results](const Key & iv) {
    results.append(iv);
  });

  // [1,5] overlaps [4,7]: yes (5>=4 and 1<=7)
  // [3,8] overlaps [4,7]: yes
  // [7,10] overlaps [4,7]: yes (7<=7)
  // [15,20] overlaps [4,7]: no
  EXPECT_EQ(results.size(), 3u);
}

TEST_F(IntervalTreeRawTest, StabQuery)
{
  tree.insert(make_node(1, 5));
  tree.insert(make_node(3, 8));
  tree.insert(make_node(7, 10));
  tree.insert(make_node(15, 20));

  auto results = tree.find_stab(4);
  // [1,5] contains 4: yes
  // [3,8] contains 4: yes
  // [7,10] contains 4: no
  // [15,20] contains 4: no
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(IntervalTreeRawTest, PointIntervalQueries)
{
  tree.insert(make_node(5, 5));
  tree.insert(make_node(3, 7));

  auto * result = tree.overlap_search(Key(5, 5));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(KEY(result).overlaps(Key(5, 5)));
}

TEST_F(IntervalTreeRawTest, MaxEndpointInvariant)
{
  tree.insert(make_node(1, 10));
  EXPECT_TRUE(tree.verify());

  tree.insert(make_node(5, 20));
  EXPECT_TRUE(tree.verify());

  tree.insert(make_node(15, 25));
  EXPECT_TRUE(tree.verify());

  auto * r1 = tree.remove(Key(5, 20));
  EXPECT_TRUE(tree.verify());
  delete r1;

  auto * r2 = tree.remove(Key(1, 10));
  EXPECT_TRUE(tree.verify());
  delete r2;
}

TEST_F(IntervalTreeRawTest, IteratorOrdering)
{
  tree.insert(make_node(10, 20));
  tree.insert(make_node(5, 15));
  tree.insert(make_node(1, 3));
  tree.insert(make_node(20, 30));

  Interval_Less<int> cmp;
  Key prev_key;
  bool first = true;
  for (IvTree::Iterator it(tree); it.has_curr(); it.next())
    {
      Key k = KEY(it.get_curr());
      if (not first)
        EXPECT_TRUE(cmp(prev_key, k) or (not cmp(k, prev_key) and not cmp(prev_key, k)));
      prev_key = k;
      first = false;
    }
}

TEST_F(IntervalTreeRawTest, CLRSTextbookExample)
{
  // CLRS 3rd ed, Figure 14.4 intervals
  tree.insert(make_node(0, 3));
  tree.insert(make_node(5, 8));
  tree.insert(make_node(6, 10));
  tree.insert(make_node(8, 9));
  tree.insert(make_node(15, 23));
  tree.insert(make_node(16, 21));
  tree.insert(make_node(17, 19));
  tree.insert(make_node(19, 20));
  tree.insert(make_node(25, 30));
  tree.insert(make_node(26, 26));

  EXPECT_EQ(tree.size(), 10u);
  EXPECT_TRUE(tree.verify());

  // Query [22, 25]: should overlap [15,23] and [25,30]
  auto overlaps = tree.find_all_overlaps(Key(22, 25));
  EXPECT_GE(overlaps.size(), 2u);
  overlaps.for_each([](const Key & iv) {
    EXPECT_TRUE(iv.overlaps(Key(22, 25)));
  });

  // Query [11, 14]: should not overlap anything
  auto * result = tree.overlap_search(Key(11, 14));
  EXPECT_EQ(result, nullptr);
}

TEST_F(IntervalTreeRawTest, LargeRandomizedVerification)
{
  std::mt19937 gen(12345);
  std::uniform_int_distribution<int> dist(0, 10000);

  const size_t N = 5000;
  std::vector<Key> intervals;
  intervals.reserve(N);

  for (size_t i = 0; i < N; ++i)
    {
      int a = dist(gen), b = dist(gen);
      if (a > b) std::swap(a, b);
      tree.insert_dup(make_node(a, b));
      intervals.push_back(Key(a, b));
    }

  EXPECT_EQ(tree.size(), N);
  EXPECT_TRUE(tree.verify());

  // Verify overlap_search against brute force for a sample of queries
  for (int q = 0; q < 100; ++q)
    {
      int a = dist(gen), b = dist(gen);
      if (a > b) std::swap(a, b);
      Key query(a, b);

      auto * found = tree.overlap_search(query);
      bool brute_force_found = false;
      for (const auto & iv : intervals)
        if (iv.overlaps(query))
          {
            brute_force_found = true;
            break;
          }

      if (brute_force_found)
        EXPECT_NE(found, nullptr) << "BF found overlap for ["
          << a << "," << b << "] but tree didn't";
      else
        EXPECT_EQ(found, nullptr) << "BF found no overlap for ["
          << a << "," << b << "] but tree did";
    }
}

// ─────────────────────────────────────────────────────────────────
// DynIntervalTree tests
// ─────────────────────────────────────────────────────────────────

class DynIntervalTreeTest : public Test
{
protected:
  using Key = Interval<int>;
  DynIntervalTree<int> tree;
};

TEST_F(DynIntervalTreeTest, Construction)
{
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  EXPECT_TRUE(tree.empty());
  EXPECT_TRUE(tree.verify());
}

TEST_F(DynIntervalTreeTest, InsertAndSearch)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());

  auto * found = tree.search(Key(1, 5));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(found->low, 1);
  EXPECT_EQ(found->high, 5);

  EXPECT_EQ(tree.search(Key(2, 3)), nullptr);
}

TEST_F(DynIntervalTreeTest, InsertByEndpoints)
{
  tree.insert(1, 5);
  tree.insert(10, 20);
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());
}

TEST_F(DynIntervalTreeTest, Remove)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));

  EXPECT_TRUE(tree.remove(Key(10, 20)));
  EXPECT_EQ(tree.size(), 2u);
  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.search(Key(10, 20)), nullptr);
}

TEST_F(DynIntervalTreeTest, RemoveByEndpoints)
{
  tree.insert(1, 5);
  EXPECT_TRUE(tree.remove(1, 5));
  EXPECT_TRUE(tree.is_empty());
}

TEST_F(DynIntervalTreeTest, RemoveNonexistent)
{
  tree.insert(Key(1, 5));
  EXPECT_FALSE(tree.remove(Key(99, 100)));
  EXPECT_EQ(tree.size(), 1u);
}

TEST_F(DynIntervalTreeTest, InvalidIntervalThrows)
{
  EXPECT_THROW(tree.insert(Key(5, 1)), std::domain_error);
  EXPECT_TRUE(tree.is_empty());
}

TEST_F(DynIntervalTreeTest, OverlapSearch)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 15));
  tree.insert(Key(20, 25));

  auto * result = tree.overlap_search(Key(4, 11));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(result->overlaps(Key(4, 11)));

  EXPECT_EQ(tree.overlap_search(Key(6, 9)), nullptr);
}

TEST_F(DynIntervalTreeTest, OverlapSearchByEndpoints)
{
  tree.insert(Key(1, 5));
  auto * result = tree.overlap_search(3, 4);
  ASSERT_NE(result, nullptr);
}

TEST_F(DynIntervalTreeTest, AllOverlaps)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(3, 8));
  tree.insert(Key(7, 10));
  tree.insert(Key(15, 20));

  auto results = tree.find_all_overlaps(Key(4, 7));
  EXPECT_EQ(results.size(), 3u);

  results.for_each([](const Key & iv) {
    EXPECT_TRUE(iv.overlaps(Key(4, 7)));
  });
}

TEST_F(DynIntervalTreeTest, FindAllOverlapsByEndpoints)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(3, 8));
  auto results = tree.find_all_overlaps(2, 4);
  EXPECT_EQ(results.size(), 2u);
}

TEST_F(DynIntervalTreeTest, Stab)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(3, 8));
  tree.insert(Key(7, 10));

  size_t count = 0;
  tree.stab(4, [&count](const Key &) { ++count; });
  EXPECT_EQ(count, 2u);
}

TEST_F(DynIntervalTreeTest, FindStab)
{
  tree.insert(Key(1, 10));
  tree.insert(Key(5, 15));
  tree.insert(Key(20, 25));

  auto results = tree.find_stab(7);
  EXPECT_EQ(results.size(), 2u);

  results = tree.find_stab(20);
  EXPECT_EQ(results.size(), 1u);

  results = tree.find_stab(16);
  EXPECT_EQ(results.size(), 0u);
}

TEST_F(DynIntervalTreeTest, CopyConstruction)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));

  DynIntervalTree<int> copy(tree);
  EXPECT_EQ(copy.size(), 3u);
  EXPECT_TRUE(copy.verify());
  EXPECT_NE(copy.search(Key(1, 5)), nullptr);
  EXPECT_NE(copy.search(Key(10, 20)), nullptr);
  EXPECT_NE(copy.search(Key(3, 7)), nullptr);
}

TEST_F(DynIntervalTreeTest, MoveConstruction)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));

  DynIntervalTree<int> moved(std::move(tree));
  EXPECT_EQ(moved.size(), 2u);
  EXPECT_TRUE(moved.verify());
  EXPECT_TRUE(tree.is_empty());
}

TEST_F(DynIntervalTreeTest, CopyAssignment)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));

  DynIntervalTree<int> other;
  other.insert(Key(100, 200));

  other = tree;
  EXPECT_EQ(other.size(), 2u);
  EXPECT_TRUE(other.verify());
}

TEST_F(DynIntervalTreeTest, MoveAssignment)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));

  DynIntervalTree<int> other;
  other = std::move(tree);
  EXPECT_EQ(other.size(), 2u);
  EXPECT_TRUE(other.verify());
  EXPECT_TRUE(tree.is_empty());
}

TEST(IntervalTreeComparatorStateTest, CopyAssignmentPropagatesComparatorState)
{
  int left_counter = 0;
  int right_counter = 0;

  DynIntervalTree<int, CountingLess> left{CountingLess(&left_counter)};
  DynIntervalTree<int, CountingLess> right{CountingLess(&right_counter)};

  left.insert(Interval<int>(1, 3));
  right.insert(Interval<int>(10, 20));

  left = right;

  left_counter = 0;
  right_counter = 0;

  const auto *found = left.search(Interval<int>(10, 20));
  ASSERT_NE(found, nullptr);
  EXPECT_EQ(left_counter, 0);
  EXPECT_GT(right_counter, 0);
}

TEST_F(DynIntervalTreeTest, FunctionalForEach)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));

  size_t count = 0;
  tree.for_each([&count](const Key &) { ++count; });
  EXPECT_EQ(count, 3u);
}

TEST_F(DynIntervalTreeTest, FunctionalExists)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));

  EXPECT_TRUE(tree.exists([](const Key & iv) { return iv.low == 10; }));
  EXPECT_FALSE(tree.exists([](const Key & iv) { return iv.low == 99; }));
}

TEST_F(DynIntervalTreeTest, FunctionalFilter)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));

  auto wide = tree.filter([](const Key & iv) {
    return (iv.high - iv.low) >= 5;
  });
  EXPECT_EQ(wide.size(), 1u); // [1,5] has width 4, [10,20] has 10, [3,7] has 4
}

TEST_F(DynIntervalTreeTest, InitializerList)
{
  DynIntervalTree<int> t = {Key(1, 5), Key(10, 20), Key(3, 7)};
  EXPECT_EQ(t.size(), 3u);
  EXPECT_TRUE(t.verify());
}

TEST_F(DynIntervalTreeTest, Verify)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));
  tree.insert(Key(25, 30));
  tree.insert(Key(0, 2));

  EXPECT_TRUE(tree.verify());
}

TEST_F(DynIntervalTreeTest, StlRangeFor)
{
  tree.insert(Key(1, 5));
  tree.insert(Key(10, 20));
  tree.insert(Key(3, 7));

  size_t count = 0;
  for (const auto & iv : tree)
    {
      EXPECT_TRUE(iv.is_valid());
      ++count;
    }
  EXPECT_EQ(count, 3u);
}

// ─────────────────────────────────────────────────────────────────
// Type tests
// ─────────────────────────────────────────────────────────────────

TEST(IntervalTreeTypeTest, IntIntervals)
{
  DynIntervalTree<int> tree;
  tree.insert(Interval<int>(1, 5));
  tree.insert(Interval<int>(3, 8));

  auto * result = tree.overlap_search(Interval<int>(4, 6));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(tree.verify());
}

TEST(IntervalTreeTypeTest, DoubleIntervals)
{
  DynIntervalTree<double> tree;
  tree.insert(Interval<double>(1.0, 5.5));
  tree.insert(Interval<double>(3.2, 8.1));
  tree.insert(Interval<double>(10.0, 20.0));

  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());

  auto * result = tree.overlap_search(Interval<double>(4.0, 6.0));
  ASSERT_NE(result, nullptr);
  EXPECT_TRUE(result->overlaps(Interval<double>(4.0, 6.0)));

  EXPECT_EQ(tree.overlap_search(Interval<double>(8.2, 9.9)), nullptr);
}

TEST(IntervalTreeTypeTest, StringIntervals)
{
  DynIntervalTree<std::string> tree;
  tree.insert(Interval<std::string>("apple", "cherry"));
  tree.insert(Interval<std::string>("banana", "grape"));
  tree.insert(Interval<std::string>("mango", "peach"));

  EXPECT_EQ(tree.size(), 3u);
  EXPECT_TRUE(tree.verify());

  auto * result = tree.overlap_search(
    Interval<std::string>("blueberry", "cantaloupe"));
  ASSERT_NE(result, nullptr);
}

// ─────────────────────────────────────────────────────────────────
// Stress test: insertions + removals + queries stay consistent
// ─────────────────────────────────────────────────────────────────

TEST(IntervalTreeStressTest, InsertRemoveVerify)
{
  DynIntervalTree<int> tree;
  std::mt19937 gen(54321);
  std::uniform_int_distribution<int> dist(0, 1000);

  DynList<Interval<int>> inserted;

  // Insert 1000 intervals
  for (int i = 0; i < 1000; ++i)
    {
      int a = dist(gen), b = dist(gen);
      if (a > b) std::swap(a, b);
      Interval<int> iv(a, b);
      tree.insert_dup(iv);
      inserted.append(iv);
    }

  EXPECT_EQ(tree.size(), 1000u);
  EXPECT_TRUE(tree.verify());

  // Remove half
  size_t removed = 0;
  DynList<Interval<int>> remaining;
  inserted.for_each([&](const Interval<int> & iv) {
    if (removed < 500)
      {
        tree.remove(iv);
        ++removed;
      }
    else
      remaining.append(iv);
  });

  EXPECT_TRUE(tree.verify());

  // Verify remaining intervals are still findable
  remaining.for_each([&](const Interval<int> & iv) {
    auto * found = tree.search(iv);
    EXPECT_NE(found, nullptr)
      << "Lost interval [" << iv.low << "," << iv.high << "]";
  });
}

TEST(IntervalTreeStressTest, AllOverlapsBruteForce)
{
  std::mt19937 gen(99999);
  std::uniform_int_distribution<int> dist(0, 100);

  DynIntervalTree<int> tree;
  std::vector<Interval<int>> intervals;

  for (int i = 0; i < 200; ++i)
    {
      int a = dist(gen), b = dist(gen);
      if (a > b) std::swap(a, b);
      Interval<int> iv(a, b);
      tree.insert_dup(iv);
      intervals.push_back(iv);
    }

  EXPECT_TRUE(tree.verify());

  // Compare all_overlaps against brute force
  for (int q = 0; q < 50; ++q)
    {
      int a = dist(gen), b = dist(gen);
      if (a > b) std::swap(a, b);
      Interval<int> query(a, b);

      auto tree_results = tree.find_all_overlaps(query);

      size_t bf_count = 0;
      for (const auto & iv : intervals)
        if (iv.overlaps(query))
          ++bf_count;

      EXPECT_EQ(tree_results.size(), bf_count)
        << "Mismatch for query [" << a << "," << b << "]";
    }
}

// ─────────────────────────────────────────────────────────────────
// key_comp() with a comparator whose copy may throw
// ─────────────────────────────────────────────────────────────────

namespace
{
  struct Copy_Error {};

  // Compares as less<int> does; its copy throws when it is told to fail.
  struct LessWithFallibleCopy
  {
    static inline bool copy_fails = false;

    LessWithFallibleCopy() = default;

    LessWithFallibleCopy(const LessWithFallibleCopy &)
    {
      if (copy_fails)
        throw Copy_Error();
    }

    LessWithFallibleCopy & operator = (const LessWithFallibleCopy &) = default;

    bool operator ()(const int a, const int b) const noexcept { return a < b; }
  };

  // While one lives, copying a LessWithFallibleCopy throws.
  struct Failing_Copies
  {
    Failing_Copies() noexcept { LessWithFallibleCopy::copy_fails = true; }
    ~Failing_Copies() { LessWithFallibleCopy::copy_fails = false; }
  };
}

// The comparator of intervals that key_comp() returns holds a copy of the
// comparator of endpoints. What that copy throws has to reach the caller: a
// noexcept key_comp() would end the program instead.
TEST(IntervalTreeComparator, KeyCompThrowsWhatCopyingTheComparatorThrows)
{
  static_assert(noexcept(std::declval<const Interval_Tree<int> &>().key_comp()));
  static_assert(noexcept(std::declval<const DynIntervalTree<int> &>().key_comp()));

  using Raw_Tree = Interval_Tree<int, LessWithFallibleCopy>;
  using Tree = DynIntervalTree<int, LessWithFallibleCopy>;
  static_assert(not noexcept(std::declval<const Raw_Tree &>().key_comp()));
  static_assert(not noexcept(std::declval<const Tree &>().key_comp()));

  Tree tree;
  tree.insert(1, 5);
  tree.insert(3, 8);

  const auto less = tree.key_comp();
  EXPECT_TRUE(less(Interval<int>(1, 5), Interval<int>(3, 8)));
  EXPECT_FALSE(less(Interval<int>(3, 8), Interval<int>(1, 5)));

  {
    const Failing_Copies failing;
    EXPECT_THROW((void) tree.key_comp(), Copy_Error);
  }

  EXPECT_TRUE(tree.verify());
  EXPECT_EQ(tree.size(), 2u);
}

// ─────────────────────────────────────────────────────────────────
// A comparison, or a copy of an endpoint, that throws
// ─────────────────────────────────────────────────────────────────

namespace
{
  struct Call_Error : std::runtime_error
  {
    Call_Error() : std::runtime_error("the call that throws") {}
  };

  long calls_left = -1; // the calls until the one that throws; -1 for none
  long calls_made = 0;

  // A call of the comparison of Throwing_Less, or a copy of a Fragile
  void a_call()
  {
    ++calls_made;
    if (calls_left == 0)
      {
        calls_left = -1;
        throw Call_Error();
      }
    if (calls_left > 0)
      --calls_left;
  }

  struct Throwing_Less
  {
    bool operator ()(const int & a, const int & b) const
    {
      a_call();
      return a < b;
    }
  };

  // An endpoint whose copies may throw, and whose moves do not
  struct Fragile
  {
    int value = 0;

    Fragile() noexcept = default;

    explicit Fragile(const int v) noexcept : value(v) {}

    Fragile(const Fragile & f) : value(f.value) { a_call(); }

    Fragile(Fragile && f) noexcept : value(f.value) {}

    Fragile & operator =(const Fragile & f)
    {
      a_call();
      value = f.value;
      return *this;
    }

    Fragile & operator =(Fragile && f) noexcept
    {
      value = f.value;
      return *this;
    }
  };

  static_assert(not std::is_nothrow_copy_assignable_v<Fragile>);
  static_assert(std::is_nothrow_move_assignable_v<Fragile>);
  static_assert(std::is_nothrow_copy_assignable_v<int>);

  struct Fragile_Less
  {
    bool operator ()(const Fragile & a, const Fragile & b) const noexcept
    {
      return a.value < b.value;
    }
  };

  // An endpoint whose copies may throw, and whose move assignment too
  struct Brittle
  {
    int value = 0;

    Brittle() noexcept = default;

    explicit Brittle(const int v) noexcept : value(v) {}

    Brittle(const Brittle & b) : value(b.value) { a_call(); }

    Brittle(Brittle && b) noexcept : value(b.value) {}

    Brittle & operator =(const Brittle & b)
    {
      a_call();
      value = b.value;
      return *this;
    }

    Brittle & operator =(Brittle && b)
    {
      a_call();
      value = b.value;
      return *this;
    }
  };

  static_assert(not std::is_nothrow_copy_assignable_v<Brittle>);
  static_assert(not std::is_nothrow_move_assignable_v<Brittle>);

  struct Brittle_Less
  {
    bool operator ()(const Brittle & a, const Brittle & b) const noexcept
    {
      return a.value < b.value;
    }
  };

  int value_of(const int v) noexcept { return v; }

  int value_of(const Fragile & f) noexcept { return f.value; }

  int value_of(const Brittle & b) noexcept { return b.value; }

  // A node as it is and where it hangs, to tell a tree that is as it was
  template <class Node>
  struct Seen
  {
    const Node *node;
    const Node *left;
    const Node *right;
    int low;
    int high;
    int max;
    unsigned long priority;

    bool operator ==(const Seen &) const = default;
  };

  template <class Node>
  void see(Node *p, std::vector<Seen<Node>> & nodes)
  {
    if (p == Node::NullPtr)
      return;
    nodes.push_back({p, LLINK(p), RLINK(p), value_of(KEY(p).low), value_of(KEY(p).high),
                     value_of(MAX_EP(p)), PRIO(p)});
    see(LLINK(p), nodes);
    see(RLINK(p), nodes);
  }

  template <class Node>
  std::vector<Seen<Node>> seen(Node *root)
  {
    std::vector<Seen<Node>> nodes;
    see(root, nodes);
    return nodes;
  }

  // The nodes of a tree and where they hang, whatever their maxima are
  template <class Node>
  std::vector<Seen<Node>> links(Node *root)
  {
    std::vector<Seen<Node>> nodes = seen(root);
    for (auto & node : nodes)
      node.max = 0;
    return nodes;
  }

  // The intervals, the maxima, the priorities and the form of a tree,
  // whatever its nodes are
  template <class Node>
  std::string shape(Node *p)
  {
    if (p == Node::NullPtr)
      return ".";
    return "(" + std::to_string(value_of(KEY(p).low)) + "," + std::to_string(value_of(KEY(p).high)) +
           " max " + std::to_string(value_of(MAX_EP(p))) + " prio " + std::to_string(PRIO(p)) + " " +
           shape(LLINK(p)) + " " + shape(RLINK(p)) + ")";
  }

  // True if two trees have the same intervals, maxima and priorities at the
  // same places
  template <class Node>
  bool same(Node *a, Node *b)
  {
    if (a == Node::NullPtr or b == Node::NullPtr)
      return a == b;
    return value_of(KEY(a).low) == value_of(KEY(b).low) and
           value_of(KEY(a).high) == value_of(KEY(b).high) and
           value_of(MAX_EP(a)) == value_of(MAX_EP(b)) and PRIO(a) == PRIO(b) and
           same(LLINK(a), LLINK(b)) and same(RLINK(a), RLINK(b));
  }

  // The intervals of a tree, in order
  template <class Node>
  std::string intervals(Node *p)
  {
    if (p == Node::NullPtr)
      return "";
    return intervals(LLINK(p)) + "[" + std::to_string(value_of(KEY(p).low)) + "," +
           std::to_string(value_of(KEY(p).high)) + "]" + intervals(RLINK(p));
  }

  // The insertions and the removal of the tree through 7.7.5: rotations, and
  // the maximum of each node computed again from its children on the way
  // back, after the links
  template <class Node, class Compare>
  struct Old_Tree
  {
    using Key = typename Node::key_type;
    using Interval_Cmp = Interval_Less<typename Node::Endpoint, Compare>;

    Node *root = Node::NullPtr;
    Compare cmp;

    ~Old_Tree() { destroyRec(root); }

    static void update_max(Node *p, const Compare & cmp)
    {
      auto & mx = MAX_EP(p);
      mx = KEY(p).high;
      if (LLINK(p) != Node::NullPtr)
        if (cmp(mx, MAX_EP(LLINK(p))))
          mx = MAX_EP(LLINK(p));
      if (RLINK(p) != Node::NullPtr)
        if (cmp(mx, MAX_EP(RLINK(p))))
          mx = MAX_EP(RLINK(p));
    }

    static Node * rotate_to_right(Node *p, const Compare & cmp)
    {
      Node *q = LLINK(p);
      LLINK(p) = RLINK(q);
      RLINK(q) = p;
      update_max(p, cmp);
      update_max(q, cmp);
      return q;
    }

    static Node * rotate_to_left(Node *p, const Compare & cmp)
    {
      Node *q = RLINK(p);
      RLINK(p) = LLINK(q);
      LLINK(q) = p;
      update_max(p, cmp);
      update_max(q, cmp);
      return q;
    }

    Node * insert(Node *root, Node *p)
    {
      if (root == Node::NullPtr)
        return p;

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      const Interval_Cmp interval_cmp(cmp);
      Node *result = nullptr;
      if (interval_cmp(pk, rk))
        {
          result = insert(LLINK(root), p);
          if (result == Node::NullPtr)
            return Node::NullPtr;
          LLINK(root) = result;
          update_max(root, cmp);
          if (PRIO(LLINK(root)) < PRIO(root))
            return rotate_to_right(root, cmp);
          return root;
        }
      if (interval_cmp(rk, pk))
        {
          result = insert(RLINK(root), p);
          if (result == Node::NullPtr)
            return Node::NullPtr;
          RLINK(root) = result;
          update_max(root, cmp);
          if (PRIO(RLINK(root)) < PRIO(root))
            return rotate_to_left(root, cmp);
          return root;
        }

      return Node::NullPtr; // duplicate
    }

    Node * insert_dup(Node *root, Node *p)
    {
      if (root == Node::NullPtr)
        return p;

      const Key & pk = KEY(p);
      const Key & rk = KEY(root);
      const Interval_Cmp interval_cmp(cmp);
      if (interval_cmp(pk, rk))
        {
          LLINK(root) = insert_dup(LLINK(root), p);
          update_max(root, cmp);
          if (PRIO(LLINK(root)) < PRIO(root))
            return rotate_to_right(root, cmp);
          return root;
        }
      RLINK(root) = insert_dup(RLINK(root), p);
      update_max(root, cmp);
      if (PRIO(RLINK(root)) < PRIO(root))
        return rotate_to_left(root, cmp);
      return root;
    }

    static Node * join_exclusive(Node *t1, Node *t2, const Compare & cmp)
    {
      if (t1 == Node::NullPtr)
        return t2;
      if (t2 == Node::NullPtr)
        return t1;

      if (PRIO(t1) < PRIO(t2))
        {
          RLINK(t1) = join_exclusive(RLINK(t1), t2, cmp);
          update_max(t1, cmp);
          return t1;
        }
      LLINK(t2) = join_exclusive(t1, LLINK(t2), cmp);
      update_max(t2, cmp);
      return t2;
    }

    Node * remove(Node *& root, const Key & key)
    {
      if (root == Node::NullPtr)
        return Node::NullPtr;

      const Key & rk = KEY(root);
      const Interval_Cmp interval_cmp(cmp);
      Node *ret_val;
      if (interval_cmp(key, rk))
        ret_val = remove(LLINK(root), key);
      else if (interval_cmp(rk, key))
        ret_val = remove(RLINK(root), key);
      else
        {
          ret_val = root;
          root = join_exclusive(LLINK(root), RLINK(root), cmp);
          return ret_val;
        }

      if (ret_val == Node::NullPtr)
        return Node::NullPtr;

      update_max(root, cmp);
      return ret_val;
    }
  };

  // The same operations, at random and with many intervals the same, on a
  // tree and on an old one whose nodes take the same priorities. With
  // `priorities` other than zero, the nodes take one of that many
  // priorities, so that most of them are equal.
  template <class E, class Less>
  void same_trees_as_the_old_ones(const unsigned seed, const int operations, const unsigned priorities)
  {
    using Tree = Interval_Tree<E, Less>;
    using Node = typename Tree::Node;

    Tree tree(seed);
    Old_Tree<Node, Less> old;
    std::mt19937 gen(seed);
    for (int i = 0; i < operations; ++i)
      {
        const int lo = static_cast<int>(gen() % 40);
        const int hi = lo + static_cast<int>(gen() % 12);
        const Interval<E> key{E(lo), E(hi)};
        if (priorities != 0)
          tree.set_seed(gen() % priorities);
        switch (gen() % 4)
          {
          case 0:
            {
              Node *p = new Node(key);
              Node *q = new Node(key);
              if (tree.insert(p) == nullptr)
                {
                  ASSERT_EQ(old.insert(old.root, q), Node::NullPtr);
                  delete p;
                  delete q;
                  break;
                }
              PRIO(q) = PRIO(p);
              Node *root = old.insert(old.root, q);
              ASSERT_NE(root, Node::NullPtr);
              old.root = root;
              break;
            }
          case 1:
            {
              Node *p = new Node(key);
              Node *q = new Node(key);
              tree.insert_dup(p);
              PRIO(q) = PRIO(p);
              old.root = old.insert_dup(old.root, q);
              break;
            }
          default:
            {
              Node *p = tree.remove(key);
              Node *q = old.remove(old.root, key);
              ASSERT_EQ(p == nullptr, q == Node::NullPtr);
              if (p != nullptr) // it goes out as its reset() leaves it
                {
                  EXPECT_EQ(LLINK(p), Node::NullPtr);
                  EXPECT_EQ(RLINK(p), Node::NullPtr);
                  EXPECT_EQ(value_of(MAX_EP(p)), value_of(KEY(p).high));
                }
              delete p;
              if (q != Node::NullPtr)
                delete q;
            }
          }
        ASSERT_TRUE(same(tree.getRoot(), old.root))
          << "after the operation " << i << ": " << shape(tree.getRoot()) << " and " << shape(old.root);
      }
    EXPECT_TRUE(tree.verify());
    EXPECT_GT(tree.size(), 0u);
    destroyRec(tree.getRoot());
    tree.reset_num_nodes();
  }

  // A tree of n intervals with even endpoints, some of them the same and
  // some that reach far, which frees its nodes
  template <class E, class Less>
  struct Grove
  {
    using Tree = Interval_Tree<E, Less>;
    using Node = typename Tree::Node;

    Tree tree;

    static Interval<E> interval(const int i)
    {
      const int lo = 2 * ((i * 7) % 23);
      return {E(lo), E(lo + 2 * ((i * 5) % 11) + (i % 4 == 0 ? 80 : 0))};
    }

    Grove(const unsigned long seed, const int n) : tree(seed)
    {
      for (int i = 0; i < n; ++i)
        tree.insert_dup(new Node(interval(i)));
    }

    ~Grove()
    {
      destroyRec(tree.getRoot());
      tree.reset_num_nodes();
    }
  };

  enum class Kind { Insert, Insert_Dup, Remove };

  // One operation on a tree, whose call number `arm` throws: none if it is
  // -1. It gives the calls that the operation made
  template <class E, class Less>
  long apply(Interval_Tree<E, Less> & tree, const Kind kind, const int lo, const int hi, const long arm)
  {
    using Node = typename Interval_Tree<E, Less>::Node;

    const Interval<E> key{E(lo), E(hi)};
    Node *p = kind == Kind::Remove ? nullptr : new Node(key);
    calls_made = 0;
    calls_left = arm;
    try
      {
        switch (kind)
          {
          case Kind::Insert:
            if (tree.insert(p) != nullptr)
              p = nullptr;
            break;
          case Kind::Insert_Dup:
            tree.insert_dup(p);
            p = nullptr;
            break;
          case Kind::Remove:
            p = tree.remove(key);
            break;
          }
      }
    catch (...)
      {
        calls_left = -1;
        delete p; // the node that did not enter
        throw;
      }
    calls_left = -1;
    const long made = calls_made;
    delete p; // the duplicate that did not enter, or the node taken out
    return made;
  }

  // An operation on a tree of n intervals, with each of its calls throwing
  // in turn: the tree must stay as it was, to the pointer, and then take
  // the operation. If not `whole`, only its nodes must stay where they
  // were: their maxima may change
  template <class E, class Less>
  void every_failed_call(const unsigned long seed, const int n, const Kind kind, const int lo, const int hi,
                         const bool whole)
  {
    SCOPED_TRACE("seed " + std::to_string(seed) + ", " + std::to_string(n) + " intervals, operation " +
                 std::to_string(static_cast<int>(kind)) + " of [" + std::to_string(lo) + "," +
                 std::to_string(hi) + "]");
    long total = 0;
    std::string expected_shape;
    std::string expected_intervals;
    {
      Grove<E, Less> grove(seed, n);
      total = apply(grove.tree, kind, lo, hi, -1);
      ASSERT_TRUE(grove.tree.verify());
      expected_shape = shape(grove.tree.getRoot());
      expected_intervals = intervals(grove.tree.getRoot());
    }
    for (long k = 0; k < total; ++k)
      {
        SCOPED_TRACE("call " + std::to_string(k) + " of " + std::to_string(total));
        Grove<E, Less> grove(seed, n);
        const auto before = whole ? seen(grove.tree.getRoot()) : links(grove.tree.getRoot());
        const size_t size = grove.tree.size();
        EXPECT_THROW(apply(grove.tree, kind, lo, hi, k), Call_Error);
        EXPECT_TRUE((whole ? seen(grove.tree.getRoot()) : links(grove.tree.getRoot())) == before);
        EXPECT_EQ(grove.tree.size(), size);
        if (not whole)
          continue;
        EXPECT_TRUE(grove.tree.verify());

        apply(grove.tree, kind, lo, hi, -1);
        EXPECT_TRUE(grove.tree.verify());
        EXPECT_EQ(intervals(grove.tree.getRoot()), expected_intervals);
        if (kind == Kind::Remove) // an insertion takes another priority
          EXPECT_EQ(shape(grove.tree.getRoot()), expected_shape);
      }
  }

  // Every removal, and insertions before, among and after the intervals of
  // trees of up to `sizes` of them, with a high endpoint below the maxima
  // and with one above them all
  template <class E, class Less>
  long every_failed_call_of_every_operation(const int sizes, const bool whole = true)
  {
    long operations = 0;
    for (unsigned long seed = 1; seed <= 3; ++seed)
      for (int n = 0; n <= sizes; ++n)
        {
          for (int i = 0; i < n; ++i, ++operations)
            {
              const auto iv = Grove<E, Less>::interval(i);
              every_failed_call<E, Less>(seed, n, Kind::Remove, value_of(iv.low), value_of(iv.high), whole);
            }
          every_failed_call<E, Less>(seed, n, Kind::Remove, 15, 17, whole); // not in the tree
          ++operations;
          for (const Kind kind : {Kind::Insert, Kind::Insert_Dup})
            {
              for (const int lo : {-1, 15, 47})
                for (const int hi : {lo, lo + 9, 999}) // odd: not in the tree
                  {
                    every_failed_call<E, Less>(seed, n, kind, lo, hi, whole);
                    ++operations;
                  }
              if (n == 0)
                continue;
              for (const int i : {0, n / 2}) // in the tree
                {
                  const auto iv = Grove<E, Less>::interval(i);
                  every_failed_call<E, Less>(seed, n, kind, value_of(iv.low), value_of(iv.high), whole);
                  ++operations;
                }
            }
        }
    return operations;
  }
}

// The tree that an insertion or a removal leaves is the one that the
// rotations of 7.7.5 left: the same nodes at the same places, with the same
// maxima. Also with priorities that are equal, where a rotation is not made.
TEST(IntervalTreeInsertionAndRemoval, SameTreesAsTheRotations)
{
  for (const unsigned seed : {1u, 2u, 3u})
    {
      same_trees_as_the_old_ones<int, Aleph::less<int>>(seed, 5000, 0);
      same_trees_as_the_old_ones<int, Aleph::less<int>>(seed, 1500, 1);
      same_trees_as_the_old_ones<int, Aleph::less<int>>(seed, 1500, 3);
      same_trees_as_the_old_ones<Fragile, Fragile_Less>(seed, 2000, 0);
      same_trees_as_the_old_ones<Fragile, Fragile_Less>(seed, 1000, 3);
    }
}

// Through 7.7.5 the maxima were computed with the comparison after the
// links, in the rotations and in the join of a removal: when it threw, an
// insertion left the node in the tree, which DynIntervalTree then freed, and
// a removal left nodes that hung twice, or took the node out without giving
// it back.
TEST(IntervalTreeThrowingCompare, EveryFailedCallLeavesTheTreeAsItWas)
{
  const long operations = every_failed_call_of_every_operation<int, Throwing_Less>(12);
  EXPECT_GT(operations, 900);
}

// The copies of the endpoints are made before the tree changes too: the
// maxima that change are copied first, and then moved to their nodes.
TEST(IntervalTreeThrowingCopy, EveryFailedCopyLeavesTheTreeAsItWas)
{
  const long operations = every_failed_call_of_every_operation<Fragile, Fragile_Less>(12);
  EXPECT_GT(operations, 900);
}

// The limit that the documentation states. A move assignment of an endpoint
// that throws, when its copy assignment may throw too, may leave the maxima
// of some nodes changed; but every node stays where it was, the node of an
// insertion out of the tree and that of a removal in it: the maxima are
// given before any link changes.
TEST(IntervalTreeThrowingMove, AFailedMoveLeavesEveryNodeWhereItWas)
{
  const long operations = every_failed_call_of_every_operation<Brittle, Brittle_Less>(12, false);
  EXPECT_GT(operations, 900);
}

// DynIntervalTree frees the node of an insertion that throws: the tree must
// not have it. And its removal must not lose the node that it does not free.
TEST(IntervalTreeThrowingCompare, DynIntervalTreeKeepsItsIntervals)
{
  using Tree = DynIntervalTree<int, Throwing_Less>;
  const auto fill = [](Tree & tree, const int n)
  {
    for (int i = 0; i < n; ++i)
      tree.insert_dup(Grove<int, Throwing_Less>::interval(i));
  };
  const auto listed = [](const Tree & tree)
  {
    std::string s;
    tree.for_each([&s](const Interval<int> & iv)
                  {
                    s += "[" + std::to_string(iv.low) + "," + std::to_string(iv.high) + "]";
                  });
    return s;
  };
  const auto every = [&](const int n, const auto & operation)
  {
    long total = 0;
    {
      Tree tree(7);
      fill(tree, n);
      calls_made = 0;
      operation(tree);
      total = calls_made;
    }
    for (long k = 0; k < total; ++k)
      {
        Tree tree(7);
        fill(tree, n);
        const std::string before = listed(tree);
        calls_left = k;
        EXPECT_THROW(operation(tree), Call_Error) << "call " << k << " of " << total;
        calls_left = -1;
        EXPECT_EQ(listed(tree), before);
        EXPECT_EQ(tree.size(), static_cast<size_t>(n));
        EXPECT_TRUE(tree.verify());
        operation(tree);
        EXPECT_TRUE(tree.verify());
      }
    return total;
  };

  long calls = 0;
  for (int n = 0; n <= 14; ++n)
    {
      calls += every(n, [](Tree & tree) { tree.insert(15, 999); });
      calls += every(n, [](Tree & tree) { tree.insert(Interval<int>(-1, 3)); });
      calls += every(n, [](Tree & tree) { tree.insert_dup(Interval<int>(15, 21)); });
      calls += every(n, [](Tree & tree) { tree.append(Interval<int>(0, 80)); });
      if (n > 0)
        {
          calls += every(n, [](Tree & tree) { EXPECT_TRUE(tree.remove(0, 80)); });
          calls += every(n, [n](Tree & tree)
                         {
                           EXPECT_TRUE(tree.remove(Grove<int, Throwing_Less>::interval(n / 2)));
                         });
        }
    }
  EXPECT_GT(calls, 500);
}

// search() runs the comparison and copies it: it is noexcept only when
// neither throws. It was noexcept without a condition in DynIntervalTree.
TEST(IntervalTreeThrowingCompare, SearchThrowsWhatTheComparisonThrows)
{
  static_assert(noexcept(std::declval<const Interval_Tree<int> &>().search(Interval<int>())));
  static_assert(noexcept(std::declval<const DynIntervalTree<int> &>().search(Interval<int>())));
  static_assert(not noexcept(std::declval<const Interval_Tree<int, Throwing_Less> &>()
                               .search(Interval<int>())));
  static_assert(not noexcept(std::declval<const DynIntervalTree<int, Throwing_Less> &>()
                               .search(Interval<int>())));
  static_assert(not noexcept(std::declval<const DynIntervalTree<int, LessWithFallibleCopy> &>()
                               .search(Interval<int>())));

  DynIntervalTree<int, Throwing_Less> tree(7);
  tree.insert(1, 5);
  tree.insert(3, 8);
  ASSERT_NE(tree.search(Interval<int>(3, 8)), nullptr);
  calls_left = 0;
  EXPECT_THROW((void) tree.search(Interval<int>(3, 8)), Call_Error);
  calls_left = -1;
  EXPECT_EQ(tree.search(Interval<int>(4, 8)), nullptr);
  EXPECT_TRUE(tree.verify());
}

// Every member of DynIntervalTree and of its iterator compiles. The
// iterator had a prev() that called one that the iterator of the tree does
// not have: it compiled only because nothing called it.
template class Aleph::DynIntervalTree<int>;
