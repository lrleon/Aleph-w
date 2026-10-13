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
 * @file flat_set_test.cc
 * @brief Tests for Aleph::FlatSet (tpl_flat_set.H).
 *
 * Covers construction (empty, range, initializer list with duplicates),
 * lookup (find/contains/bounds), mutation (insert/emplace/erase), iteration
 * order, Aleph conventions (is_empty/empty/traverse) and a randomized parity
 * check against std::set as the reference implementation.
 */

#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_flat_set.H>

using Aleph::FlatSet;

namespace
{
// A key whose move assignment throws once a budget of them is spent, to
// interrupt a shift or a relocation half way. Nothing else throws.
struct Throwing_Move_Key
{
  static inline int moves_left = -1; // -1 means that it never throws

  int value = 0;

  Throwing_Move_Key() = default;
  Throwing_Move_Key(const int v) : value(v) {}
  Throwing_Move_Key(const Throwing_Move_Key &) = default;
  Throwing_Move_Key(Throwing_Move_Key &&) = default;
  Throwing_Move_Key &operator = (const Throwing_Move_Key &) = default;
  Throwing_Move_Key &operator = (Throwing_Move_Key &&other)
  {
    if (moves_left == 0)
      throw std::runtime_error("move assignment");
    if (moves_left > 0)
      --moves_left;
    value = other.value;
    return *this;
  }

  bool operator < (const Throwing_Move_Key &other) const noexcept
  {
    return value < other.value;
  }
};

// A key that moves without throwing but whose copies throw on demand.
struct Throwing_Copy_Key
{
  static inline bool fail = false;

  int value = 0;

  Throwing_Copy_Key() = default;
  Throwing_Copy_Key(const int v) : value(v) {}
  Throwing_Copy_Key(const Throwing_Copy_Key &other) : value(other.value)
  {
    if (fail)
      throw std::runtime_error("copy");
  }
  Throwing_Copy_Key(Throwing_Copy_Key &&) noexcept = default;
  Throwing_Copy_Key &operator = (const Throwing_Copy_Key &other)
  {
    if (fail)
      throw std::runtime_error("copy assignment");
    value = other.value;
    return *this;
  }
  Throwing_Copy_Key &operator = (Throwing_Copy_Key &&) noexcept = default;

  bool operator < (const Throwing_Copy_Key &other) const noexcept
  {
    return value < other.value;
  }
};

template <class Set>
::testing::AssertionResult is_strictly_sorted(const Set &s)
{
  for (size_t i = 1; i < s.size(); ++i)
    if (not (s.nth(i - 1) < s.nth(i)))
      return ::testing::AssertionFailure()
        << "keys " << s.nth(i - 1).value << " and " << s.nth(i).value
        << " at " << i - 1 << " are not strictly increasing";
  return ::testing::AssertionSuccess();
}
} // namespace

TEST(FlatSet, EmptySetBasics)
{
  FlatSet<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0u);
  EXPECT_FALSE(s.contains(42));
  EXPECT_EQ(s.find(42), s.end());
  EXPECT_EQ(s.begin(), s.end());
  EXPECT_EQ(s.count(42), 0u);
}

TEST(FlatSet, InsertKeepsSortedOrderAndRejectsDuplicates)
{
  FlatSet<int> s;
  EXPECT_TRUE(s.insert(5).second);
  EXPECT_TRUE(s.insert(1).second);
  EXPECT_TRUE(s.insert(3).second);
  EXPECT_FALSE(s.insert(3).second);  // duplicate

  ASSERT_EQ(s.size(), 3u);
  EXPECT_EQ(s.nth(0), 1);
  EXPECT_EQ(s.nth(1), 3);
  EXPECT_EQ(s.nth(2), 5);

  // The iterator returned on a duplicate points to the existing element.
  auto [it, inserted] = s.insert(1);
  EXPECT_FALSE(inserted);
  EXPECT_EQ(*it, 1);
  EXPECT_EQ(it, s.begin());
}

TEST(FlatSet, InitializerListDropsDuplicates)
{
  FlatSet<int> s = {5, 1, 3, 1, 5, 5};
  ASSERT_EQ(s.size(), 3u);
  EXPECT_EQ(s.nth(0), 1);
  EXPECT_EQ(s.nth(1), 3);
  EXPECT_EQ(s.nth(2), 5);
}

TEST(FlatSet, RangeConstructor)
{
  const std::vector<int> v = {9, 7, 7, 8, 1};
  FlatSet<int> s(v.begin(), v.end());
  ASSERT_EQ(s.size(), 4u);
  EXPECT_TRUE(std::is_sorted(s.begin(), s.end()));
  EXPECT_TRUE(s.contains(7));
  EXPECT_TRUE(s.contains(9));
  EXPECT_FALSE(s.contains(2));
}

TEST(FlatSet, LookupBoundsAndEqualRange)
{
  const FlatSet<int> s = {10, 20, 30, 40};

  EXPECT_EQ(*s.lower_bound(20), 20);
  EXPECT_EQ(*s.lower_bound(25), 30);
  EXPECT_EQ(*s.upper_bound(20), 30);
  EXPECT_EQ(s.lower_bound(50), s.end());
  EXPECT_EQ(s.upper_bound(40), s.end());
  EXPECT_EQ(*s.lower_bound(5), 10);

  auto [lo, hi] = s.equal_range(30);
  ASSERT_EQ(hi - lo, 1);
  EXPECT_EQ(*lo, 30);

  auto [lo2, hi2] = s.equal_range(35);
  EXPECT_EQ(lo2, hi2);
}

TEST(FlatSet, EraseByKeyAndIterator)
{
  FlatSet<int> s = {1, 2, 3, 4, 5};

  EXPECT_EQ(s.erase(3), 1u);
  EXPECT_EQ(s.erase(3), 0u);  // already gone
  EXPECT_EQ(s.size(), 4u);
  EXPECT_FALSE(s.contains(3));

  auto it = s.find(2);
  ASSERT_NE(it, s.end());
  it = s.erase(it);
  EXPECT_EQ(*it, 4);  // element following the erased one
  EXPECT_EQ(s.size(), 3u);

  // Erasing an end iterator must throw out_of_range.
  EXPECT_THROW((void) s.erase(s.end()), std::out_of_range);
}

TEST(FlatSet, MinMaxAndCheckedAccess)
{
  FlatSet<int> s = {7, 3, 9};
  EXPECT_EQ(s.get_first(), 3);
  EXPECT_EQ(s.get_last(), 9);
  EXPECT_EQ(s.min(), 3);
  EXPECT_EQ(s.max(), 9);
  EXPECT_THROW((void) s.nth(3), std::out_of_range);

  const FlatSet<int> e;
  EXPECT_THROW((void) e.get_first(), std::underflow_error);
  EXPECT_THROW((void) e.get_last(), std::underflow_error);
}

TEST(FlatSet, AlephConventionsEmptyClearTraverse)
{
  FlatSet<int> s = {1, 2, 3};

  // Aleph convention: empty() clears, is_empty() tests.
  s.empty();
  EXPECT_TRUE(s.is_empty());

  s = {4, 5, 6, 7};
  s.clear();
  EXPECT_TRUE(s.is_empty());

  s = {1, 2, 3, 4};
  int sum = 0;
  EXPECT_TRUE(s.traverse([&sum] (const int &x) { sum += x; return true; }));
  EXPECT_EQ(sum, 10);

  // Early stop: visit only until (and including) the first even key.
  int visited = 0;
  EXPECT_FALSE(s.traverse([&visited] (const int &x)
  {
    ++visited;
    return x % 2 != 0;
  }));
  EXPECT_EQ(visited, 2);  // 1 (odd, continue), 2 (even, stop)
}

TEST(FlatSet, CopyMoveAndEquality)
{
  FlatSet<std::string> s = {"beta", "alpha", "gamma"};
  FlatSet<std::string> copy = s;
  EXPECT_EQ(copy, s);

  FlatSet<std::string> moved = std::move(copy);
  EXPECT_EQ(moved, s);

  moved.insert("delta");
  EXPECT_NE(moved, s);

  FlatSet<std::string> assigned;
  assigned = s;
  EXPECT_EQ(assigned, s);

  FlatSet<std::string> move_assigned;
  move_assigned = std::move(assigned);
  EXPECT_EQ(move_assigned, s);
}

TEST(FlatSet, CustomComparatorDescendingOrder)
{
  FlatSet<int, std::greater<int>> s = {1, 5, 3};
  EXPECT_EQ(s.nth(0), 5);
  EXPECT_EQ(s.nth(1), 3);
  EXPECT_EQ(s.nth(2), 1);
  EXPECT_TRUE(s.contains(3));
  EXPECT_FALSE(s.insert(5).second);
}

TEST(FlatSet, ReserveAndCapacity)
{
  FlatSet<int> s;
  s.reserve(1000);
  const size_t cap = s.capacity();
  EXPECT_GE(cap, 1000u);
  for (int i = 0; i < 1000; ++i)
    s.insert(i);
  EXPECT_EQ(s.capacity(), cap);  // no reallocation after reserve
  EXPECT_EQ(s.size(), 1000u);
}

TEST(FlatSet, DataIsSortedContiguousStorage)
{
  FlatSet<int> s = {4, 2, 8, 6};
  const int *p = s.data();
  ASSERT_EQ(s.size(), 4u);
  EXPECT_EQ(p[0], 2);
  EXPECT_EQ(p[3], 8);
  EXPECT_EQ(s.begin(), p);
  EXPECT_EQ(s.end(), p + 4);
}

TEST(FlatSet, SwapExchangesContents)
{
  FlatSet<int> a = {1, 2};
  FlatSet<int> b = {9};
  a.swap(b);
  EXPECT_EQ(a.size(), 1u);
  EXPECT_TRUE(a.contains(9));
  EXPECT_EQ(b.size(), 2u);
  EXPECT_TRUE(b.contains(1) and b.contains(2));
}

// Randomized parity test: FlatSet must behave exactly like std::set under
// an arbitrary interleaving of insertions, removals and lookups.
TEST(FlatSet, RandomizedParityWithStdSet)
{
  std::mt19937 rng(20260702);
  std::uniform_int_distribution<int> key_dist(0, 200);
  std::uniform_int_distribution<int> op_dist(0, 2);

  FlatSet<int> fs;
  std::set<int> ref;

  for (int step = 0; step < 4000; ++step)
    {
      const int k = key_dist(rng);
      switch (op_dist(rng))
        {
        case 0:
          EXPECT_EQ(fs.insert(k).second, ref.insert(k).second);
          break;
        case 1:
          EXPECT_EQ(fs.erase(k), ref.erase(k));
          break;
        default:
          EXPECT_EQ(fs.contains(k), ref.count(k) == 1);
          break;
        }
    }

  ASSERT_EQ(fs.size(), ref.size());
  size_t i = 0;
  for (const int k : ref)
    EXPECT_EQ(fs.nth(i++), k);
}

TEST(FlatSet, CopyThatThrowsLeavesTheSetUntouched)
{
  FlatSet<Throwing_Copy_Key> s;
  for (const int k : {1, 3, 5, 7})
    s.insert(Throwing_Copy_Key(k));
  const Throwing_Copy_Key first(0), middle(4);

  Throwing_Copy_Key::fail = true;
  EXPECT_THROW(s.insert(first), std::runtime_error);
  EXPECT_THROW(s.insert(middle), std::runtime_error);
  Throwing_Copy_Key::fail = false;

  ASSERT_EQ(s.size(), 4u);
  EXPECT_TRUE(is_strictly_sorted(s));
  EXPECT_FALSE(s.contains(first));
  EXPECT_FALSE(s.contains(middle));
  EXPECT_TRUE(s.contains(Throwing_Copy_Key(5)));
}

// If moving a key can throw, an insertion, a removal or a reservation
// interrupted at any of its moves must leave a valid set: either the
// operation took effect or the set was emptied, never a duplicated key.
TEST(FlatSet, MoveThatThrowsAtAnyPointLeavesAValidSet)
{
  const auto build = []
    {
      FlatSet<Throwing_Move_Key> s;
      for (const int k : {1, 3, 5, 7, 9})
        s.insert(Throwing_Move_Key(k));
      return s;
    };

  enum class Operation { insert_first, insert_middle, erase_first, reserve };
  for (const auto operation : {Operation::insert_first, Operation::insert_middle,
                               Operation::erase_first, Operation::reserve})
    for (int budget = 0; budget < 12; ++budget)
      {
        SCOPED_TRACE(testing::Message() << "operation " << static_cast<int>(operation)
                                        << ", moves allowed " << budget);
        auto s = build();
        bool thrown = false;
        Throwing_Move_Key::moves_left = budget;
        try
          {
            switch (operation)
              {
              case Operation::insert_first: s.insert(Throwing_Move_Key(0)); break;
              case Operation::insert_middle: s.insert(Throwing_Move_Key(4)); break;
              case Operation::erase_first: s.erase(Throwing_Move_Key(1)); break;
              case Operation::reserve: s.reserve(4 * s.capacity()); break;
              }
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        Throwing_Move_Key::moves_left = -1;

        EXPECT_TRUE(is_strictly_sorted(s));
        if (thrown)
          EXPECT_TRUE(s.is_empty());
        else if (operation == Operation::erase_first)
          EXPECT_EQ(s.size(), 4u);
        else if (operation == Operation::reserve)
          EXPECT_EQ(s.size(), 5u);
        else
          EXPECT_EQ(s.size(), 6u);

        // the set remains usable in both cases
        s.insert(Throwing_Move_Key(100));
        EXPECT_TRUE(s.contains(Throwing_Move_Key(100)));
        EXPECT_TRUE(is_strictly_sorted(s));
      }

  // the sweep reached the point where nothing throws
  auto s = build();
  Throwing_Move_Key::moves_left = 11;
  EXPECT_NO_THROW(s.insert(Throwing_Move_Key(0)));
  Throwing_Move_Key::moves_left = -1;
}
