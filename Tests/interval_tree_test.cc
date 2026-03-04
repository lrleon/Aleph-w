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
