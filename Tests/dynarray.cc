
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
 * @file dynarray.cc
 * @brief Tests for Dynarray
 */

#include <gtest/gtest.h>

#include <tpl_dynArray.H>
#include <ah-unique.H>

#include <new>
#include <stdexcept>
#include <string>
#include <type_traits>
#include <utility>

using namespace Aleph;
using namespace std;

namespace {

TEST(DynArrayBasics, construction_and_size)
{
  DynArray<int> arr;
  EXPECT_TRUE(arr.is_empty());
  EXPECT_EQ(arr.size(), 0u);

  arr.append(42);
  EXPECT_EQ(arr.size(), 1u);
  EXPECT_FALSE(arr.is_empty());
  EXPECT_EQ(arr.access(0), 42);

  arr.append(7);
  EXPECT_EQ(arr.size(), 2u);
  EXPECT_EQ(arr.access(1), 7);
}

TEST(DynArrayFunctional, ToArrayPreservesOrder)
{
  DynArray<int> src;
  for (int v : {3, 6, 9})
    src.append(v);

  auto copy = src.to_array();

  ASSERT_EQ(copy.size(), src.size());
  for (size_t i = 0; i < copy.size(); ++i)
    EXPECT_EQ(copy(i), src(i));
}

TEST(DynArrayBasics, default_values_and_touch)
{
  DynArray<int> arr;
  arr.set_default_initial_value(123);

  arr.reserve(0, 3);
  ASSERT_EQ(arr.size(), 4u);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr.access(i), 123);

  arr.access(2) = 77;
  EXPECT_EQ(arr.access(2), 77);

  auto & ref = arr.touch(10);
  ref = 99;
  EXPECT_EQ(arr.size(), 11u);
  EXPECT_EQ(arr.access(10), 99);
}

TEST(DynArrayAlgorithms, InPlaceUnique)
{
  DynArray<int> arr;
  arr.append(1);
  arr.append(2);
  arr.append(1);
  arr.append(3);
  arr.append(2);
  arr.append(4);
  arr.append(4);

  in_place_unique(arr);

  ASSERT_EQ(arr.size(), 4u);
  EXPECT_EQ(arr.access(0), 1);
  EXPECT_EQ(arr.access(1), 2);
  EXPECT_EQ(arr.access(2), 3);
  EXPECT_EQ(arr.access(3), 4);
}

TEST(DynArrayErrors, stack_operations_on_empty)
{
  DynArray<int> arr;

  EXPECT_THROW(arr.pop(), std::underflow_error);
  EXPECT_THROW(arr.top(), std::underflow_error);
  EXPECT_THROW(arr.get_first(), std::underflow_error);
  EXPECT_THROW(arr.get_last(), std::underflow_error);

  int dummy = 0;
  EXPECT_THROW(arr.remove(dummy), std::underflow_error);
}

TEST(DynArrayReserve, invalid_range_throws)
{
  DynArray<int> arr;
  EXPECT_THROW(arr.reserve(5, 4), std::domain_error);
}

TEST(DynArrayReserve, reserve_and_access)
{
  DynArray<int> arr;
  arr.reserve(5);
  ASSERT_EQ(arr.size(), 5u);
  for (size_t i = 0; i < arr.size(); ++i)
    arr.access(i) = static_cast<int>(i * 2);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr.access(i), static_cast<int>(i * 2));
}

TEST(DynArrayIterator, get_it_position)
{
  DynArray<int> arr;
  for (int i = 0; i < 6; ++i)
    arr.append(i);

  auto it = arr.get_it(3);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), 3);
  it.next();
  EXPECT_EQ(it.get_curr(), 4);

  const auto & carr = arr;
  auto cit = carr.get_it(5);
  EXPECT_EQ(cit.get_curr(), 5);
  EXPECT_THROW(carr.get_it(6), std::out_of_range);
}

TEST(DynArrayIterator, IsLastRequiresCurrentItem)
{
  DynArray<int>::Iterator singular;
  EXPECT_FALSE(singular.is_last());
  singular.reset_last();
  EXPECT_FALSE(singular.has_curr());
  EXPECT_FALSE(singular.is_last());

  DynArray<int> empty;
  auto it = empty.get_it();
  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  EXPECT_FALSE(it.is_last());

  empty.append(7);
  it.reset_last();
  EXPECT_TRUE(it.has_curr());
  EXPECT_TRUE(it.is_last());
  it.end();
  EXPECT_FALSE(it.is_last());
}

TEST(DynArrayIterator, CheckedAccessRejectsInvalidPositions)
{
  DynArray<int>::Iterator singular;
  EXPECT_THROW(singular.get_curr(), std::overflow_error);
  EXPECT_THROW(singular.next(), std::overflow_error);
  singular.reset_last();
  EXPECT_THROW(singular.get_curr(), std::underflow_error);
  EXPECT_THROW(singular.next(), std::overflow_error);

  DynArray<int> arr;
  auto it = arr.get_it();
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  it.reset_last();
  EXPECT_THROW(it.get_curr(), std::underflow_error);

  arr.append(42);
  it.reset_first();
  EXPECT_EQ(it.get_curr(), 42);
  it.next();
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  it.set_pos(-1);
  EXPECT_THROW(it.get_curr(), std::underflow_error);
  it.next();
  EXPECT_EQ(it.get_curr(), 42);
}

TEST(DynArrayReserve, adjust_and_cut)
{
  DynArray<int> arr;
  arr.adjust(10);
  EXPECT_EQ(arr.size(), 10u);
  arr.cut(3);
  EXPECT_EQ(arr.size(), 3u);
  arr.empty();
  EXPECT_TRUE(arr.is_empty());
  arr.append(1);
  arr.append(2);
  arr.cut(2);
  EXPECT_EQ(arr.size(), 2u);

  arr.clear();
  EXPECT_TRUE(arr.is_empty());
  EXPECT_EQ(arr.size(), 0u);

  arr.append(10);
  EXPECT_TRUE(arr.contains(10));
  EXPECT_FALSE(arr.contains(20));
}

TEST(DynArrayReserve, reserve_invalid_order)
{
  DynArray<int> arr;
  EXPECT_THROW(arr.reserve(4, 3), std::domain_error);
  EXPECT_EQ(arr.size(), 0u);
}

TEST(DynArrayReserve, reserve_touch_consistency)
{
  DynArray<int> arr;
  arr.reserve(2, 5);
  ASSERT_EQ(arr.size(), 6u);
  arr.touch(20) = 100;
  EXPECT_EQ(arr.size(), 21u);
  arr.cut(6);
  EXPECT_EQ(arr.size(), 6u);
}

// Default constructor that throws std::bad_alloc once `budget` constructions
// have succeeded. A negative budget never fails. Used to make a block
// allocation fail at a chosen point inside DynArray::reserve().
struct Fails_After_Budget
{
  static inline long budget = -1;
  int value = 0;

  Fails_After_Budget()
  {
    if (budget == 0)
      throw std::bad_alloc();
    if (budget > 0)
      --budget;
  }
};

// Restores the unlimited budget even if an ASSERT leaves the test early.
struct Budget_Guard
{
  ~Budget_Guard() { Fails_After_Budget::budget = -1; }
};

TEST(DynArrayReserve, reserve_fills_missing_blocks_of_existing_segment)
{
  DynArray<int> arr(4, 2, 2);  // segments of 4 blocks of 4 entries
  arr.touch(0) = 7;            // segment 0 exists, but only its block 0
  ASSERT_EQ(arr.get_num_blocks(), 1u);

  arr.reserve(0, 15);  // must allocate blocks 1..3 of the existing segment
  EXPECT_EQ(arr.get_num_blocks(), 4u);
  EXPECT_EQ(arr.size(), 16u);
  for (size_t i = 0; i <= 15; ++i)
    EXPECT_TRUE(arr.exist(i)) << "entry " << i;
  EXPECT_EQ(arr.access(0), 7);  // existing data is preserved
}

TEST(DynArrayReserve, failed_reserve_releases_only_what_it_allocated)
{
  Budget_Guard guard;
  // 16 segments of 4 blocks of 4 entries: [0, 47] spans segments 0, 1 and 2.
  DynArray<Fails_After_Budget> arr(4, 2, 2);
  const size_t block = arr.get_block_size();
  ASSERT_EQ(block, 4u);

  arr.touch(0);  // segment 0 and its block 0 exist before the reserve
  ASSERT_EQ(arr.get_num_blocks(), 1u);

  // Blocks 1..3 of segment 0 and block 0 of segment 1 succeed; block 1 of
  // segment 1 fails, after a whole new segment has been allocated.
  Fails_After_Budget::budget = static_cast<long>(4 * block);
  EXPECT_THROW(arr.reserve(0, 47), std::bad_alloc);
  Fails_After_Budget::budget = -1;

  EXPECT_EQ(arr.get_num_blocks(), 1u);
  EXPECT_EQ(arr.size(), 1u);
  for (size_t i = 0; i < block; ++i)
    EXPECT_TRUE(arr.exist(i)) << "pre-existing entry " << i;
  for (size_t i = block; i <= 47; ++i)
    EXPECT_FALSE(arr.exist(i)) << "entry " << i << " was not rolled back";

  arr.reserve(0, 47);  // the array remains fully usable
  EXPECT_EQ(arr.get_num_blocks(), 12u);
  EXPECT_EQ(arr.size(), 48u);
  for (size_t i = 0; i <= 47; ++i)
    EXPECT_TRUE(arr.exist(i));
}

TEST(DynArrayReserve, failure_on_first_block_releases_new_segment)
{
  Budget_Guard guard;
  DynArray<Fails_After_Budget> arr(4, 2, 2);

  Fails_After_Budget::budget = 0;  // the very first block allocation fails
  EXPECT_THROW(arr.reserve(20, 40), std::bad_alloc);
  Fails_After_Budget::budget = -1;

  EXPECT_EQ(arr.get_num_blocks(), 0u);
  EXPECT_EQ(arr.size(), 0u);
  for (size_t i = 0; i <= 47; ++i)
    EXPECT_FALSE(arr.exist(i));
}

TEST(DynArrayReserve, reserve_of_existing_range_constructs_nothing)
{
  Budget_Guard guard;
  DynArray<Fails_After_Budget> arr(4, 2, 2);
  arr.reserve(0, 47);
  ASSERT_EQ(arr.get_num_blocks(), 12u);

  Fails_After_Budget::budget = 0;  // any element construction would throw
  EXPECT_NO_THROW(arr.reserve(0, 47));
  EXPECT_NO_THROW(arr.reserve(5, 30));
  EXPECT_EQ(arr.get_num_blocks(), 12u);
  EXPECT_EQ(arr.size(), 48u);
}

TEST(DynArrayQueueStack, push_pop_fifo_lifo)
{
  DynArray<int> arr;
  for (int i = 0; i < 5; ++i)
    arr.push(i);
  EXPECT_EQ(arr.get_first(), 0);
  EXPECT_EQ(arr.get_last(), 4);

  arr.insert(-1);
  EXPECT_EQ(arr.get_first(), -1);

  EXPECT_EQ(arr.pop(), 4);
  EXPECT_EQ(arr.size(), 5u);
  EXPECT_EQ(arr.top(), arr.get_last());
}

// An integer gives a lazy logical size only through explicit construction.
static_assert(std::is_constructible_v<DynArray<int>, size_t>);
static_assert(not std::is_convertible_v<int, DynArray<int>>);
static_assert(not std::is_convertible_v<size_t, DynArray<int>>);
static_assert(std::is_constructible_v<DynArray<int>, size_t, int>);

/// Aggregate whose DynArray member may be omitted from its initializer.
struct With_Dyn_Array_Member
{
  int key = 0;
  DynArray<int> values;
};

TEST(DynArrayCtors, DefaultConstructorIsNotExplicit)
{
  const DynArray<int> braced = {};
  EXPECT_TRUE(braced.is_empty());

  const With_Dyn_Array_Member aggregate{7};
  EXPECT_EQ(aggregate.key, 7);
  EXPECT_TRUE(aggregate.values.is_empty());
}

TEST(DynArrayCtors, LazySizeIsCoveredByTheDirectory)
{
  const size_t n = size_t{1} << 30;
  const DynArray<int> lazy(n);
  EXPECT_EQ(lazy.size(), n);
  EXPECT_GE(lazy.max_size(), lazy.size());
  EXPECT_EQ(lazy.get_num_blocks(), 0u);
  EXPECT_FALSE(lazy.exist(n - 1));

  EXPECT_THROW((void) DynArray<int>(DynArray<int>::Max_Dim_Allowed + 1),
               std::length_error);
}

TEST(DynArrayCtors, CountValueAllocatesEveryEntry)
{
  const DynArray<int> sevens(5, 7);
  ASSERT_EQ(sevens.size(), 5u);
  int sum = 0;
  sevens.for_each([&sum] (int x) { sum += x; });
  EXPECT_EQ(sum, 35);
  for (size_t i = 0; i < sevens.size(); ++i)
    {
      EXPECT_TRUE(sevens.exist(i));
      EXPECT_EQ(sevens.access(i), 7);
    }

  const DynArray<size_t> same_type(size_t{3}, size_t{8});
  ASSERT_EQ(same_type.size(), 3u);
  EXPECT_EQ(same_type.access(2), 8u);

  const DynArray<std::string> strings(2, "ab");
  ASSERT_EQ(strings.size(), 2u);
  EXPECT_EQ(strings.access(1), "ab");

  const DynArray<int> none(0, 7);
  EXPECT_TRUE(none.is_empty());
}

TEST(DynArrayCopy, CopyKeepsTheLazySizeAndItsOwnDefaultValue)
{
  const DynArray<int> lazy(10);
  const DynArray<int> lazy_copy(lazy);
  EXPECT_EQ(lazy_copy.size(), 10u);
  EXPECT_EQ(lazy_copy.get_num_blocks(), 0u);

  auto * src = new DynArray<int>;
  src->set_default_initial_value(42);
  src->touch(2) = 5;
  DynArray<int> copy(*src);
  delete src;  // the copy must not refer to the original's default value
  EXPECT_EQ(copy.access(2), 5);
  EXPECT_EQ(copy.touch(5000), 42);
}

TEST(DynArrayCopy, CopyAssignmentReplacesEverything)
{
  DynArray<int> dst;
  for (int i = 1; i <= 10; ++i)
    dst.append(i);

  DynArray<int> src(10);
  src.set_default_initial_value(42);
  dst = src;
  EXPECT_EQ(dst.size(), 10u);
  EXPECT_FALSE(dst.exist(0));  // no stale value from the previous contents
  EXPECT_EQ(dst.touch(5000), 42);

  const DynArray<int> & same = dst;
  dst = same;
  EXPECT_EQ(dst.size(), 5001u);
  EXPECT_EQ(dst.access(5000), 42);
}

/// Element that counts its live objects and whose copy assignment throws
/// when `budget` reaches zero (a negative budget never throws).
struct Copy_Bomb
{
  static inline long live = 0;
  static inline long budget = -1;
  int value = 0;

  Copy_Bomb() noexcept { ++live; }
  Copy_Bomb(const Copy_Bomb & other) noexcept : value(other.value) { ++live; }
  Copy_Bomb(Copy_Bomb && other) noexcept : value(other.value) { ++live; }
  ~Copy_Bomb() { --live; }

  Copy_Bomb & operator = (const Copy_Bomb & other)
  {
    if (budget >= 0 and budget-- == 0)
      throw std::runtime_error("Copy_Bomb: copy budget exhausted");
    value = other.value;
    return *this;
  }

  Copy_Bomb & operator = (Copy_Bomb && other) noexcept
  {
    value = other.value;
    return *this;
  }
};

// The copy constructor delegates to the geometry constructor, so when copying
// an entry throws, the destructor releases every segment and block allocated
// so far. The count of live elements shows any block that leaked.
TEST(DynArrayCopy, ThrowingCopyLeaksNothing)
{
  DynArray<Copy_Bomb> src;
  src.touch(10).value = 1;       // first block
  src.touch(9000).value = 2;     // a later block, leaving a gap
  src.touch(1100000).value = 3;  // another segment
  ASSERT_EQ(src.get_num_blocks(), 3u);

  const long live_before = Copy_Bomb::live;
  const long block = static_cast<long>(src.get_block_size());

  // Each block costs 2 * block assignments: the default fill and the copy.
  // The first assignment copies the default value itself.
  for (const long at : {0L, 1L, block - 1, block, block + 5, 2 * block + 7,
                        3 * block + 1, 5 * block})
    {
      Copy_Bomb::budget = at;
      EXPECT_THROW(DynArray<Copy_Bomb> copy(src), std::runtime_error)
        << "copy did not throw at assignment " << at;
      Copy_Bomb::budget = -1;
      EXPECT_EQ(Copy_Bomb::live, live_before)
        << "elements leaked when throwing at assignment " << at;
    }

  // Copy assignment gives the strong guarantee: the target is unchanged.
  DynArray<Copy_Bomb> dst;
  dst.touch(3).value = 7;
  const long live_with_dst = Copy_Bomb::live;
  Copy_Bomb::budget = block + 3;
  EXPECT_THROW(dst = src, std::runtime_error);
  Copy_Bomb::budget = -1;
  EXPECT_EQ(Copy_Bomb::live, live_with_dst);
  ASSERT_EQ(dst.size(), 4u);
  EXPECT_EQ(dst.read(3).value, 7);
}

// Reading an entry that was never written yields the default value and never
// allocates memory; a constant array reads through const T &, not a proxy.
static_assert(std::is_same_v<decltype(std::declval<const DynArray<int> &>()[0]),
                             const int &>);

TEST(DynArrayLazyReads, ReadingUnwrittenEntriesAllocatesNothing)
{
  DynArray<int> lazy(10);
  const DynArray<int> & view = lazy;

  EXPECT_EQ(lazy.read(3), 0);
  EXPECT_EQ(view[3], 0);

  int sum = 0;
  lazy.for_each([&sum] (int x) { sum += x; });
  for (const int x : view)
    sum += x;
  EXPECT_EQ(sum, 0);

  EXPECT_EQ(lazy, DynArray<int>(10, 0));
  EXPECT_EQ(lazy.to_array().size(), 10u);
  EXPECT_EQ(lazy.get_num_blocks(), 0u);

  EXPECT_THROW((void) lazy.read(10), std::out_of_range);
  EXPECT_THROW((void) view[10], std::out_of_range);
}

TEST(DynArrayLazyReads, ReadsUseTheDefaultValue)
{
  DynArray<int> lazy(5);
  lazy.set_default_initial_value(7);
  EXPECT_EQ(lazy.read(4), 7);

  int sum = 0;
  lazy.for_each([&sum] (int x) { sum += x; });
  EXPECT_EQ(sum, 35);
  EXPECT_EQ(lazy.get_num_blocks(), 0u);
}

TEST(DynArrayLazyReads, TraversalVisitsGapsBetweenWrittenBlocks)
{
  DynArray<int> sparse;
  sparse.touch(10000) = 1;
  ASSERT_EQ(sparse.size(), 10001u);

  size_t visited = 0;
  int sum = 0;
  sparse.for_each([&] (int x) { ++visited; sum += x; });
  EXPECT_EQ(visited, 10001u);
  EXPECT_EQ(sum, 1);
}

TEST(DynArrayLazyReads, ModifiableAccessAllocatesTheBlock)
{
  DynArray<int> lazy(10);
  lazy[3] += 1;
  EXPECT_EQ(lazy.read(3), 1);
  EXPECT_EQ(lazy.get_num_blocks(), 1u);

  DynArray<int> other(10);
  other[0] = other[5];  // an unwritten right-hand entry reads as the default
  EXPECT_EQ(other.read(0), 0);

  DynArray<int> queue(3);
  EXPECT_EQ(queue.get_last(), 0);
  queue.get_first() = 4;
  EXPECT_EQ(queue.read(0), 4);
  EXPECT_EQ(queue.top(), 0);
}

TEST(DynArrayLazyReads, WritesThroughCopiesOfUnwrittenEntriesHaveNoEffect)
{
  DynArray<int> lazy(3);
  lazy.traverse([] (int & x) { x = 5; return true; });
  for (auto & x : lazy)
    x = 6;
  EXPECT_EQ(lazy.read(0), 0);
  EXPECT_EQ(lazy.get_num_blocks(), 0u);

  DynArray<int> written(3, 1);  // real entries are modified in place
  written.traverse([] (int & x) { x = 5; return true; });
  EXPECT_EQ(written.read(2), 5);
}

TEST(DynArrayLazyReads, StackAndQueueOperationsOnLazyEntries)
{
  DynArray<int> lazy(3);
  lazy.insert(9);
  ASSERT_EQ(lazy.size(), 4u);
  EXPECT_EQ(lazy.read(0), 9);
  EXPECT_EQ(lazy.read(3), 0);

  DynArray<int> popped(3);
  EXPECT_EQ(popped.pop(), 0);
  EXPECT_EQ(popped.size(), 2u);
}

TEST(DynArrayLazyReads, CutDropsTheEntriesItRemoves)
{
  DynArray<int> arr;
  for (int i = 1; i <= 10; ++i)
    arr.append(i);

  arr.cut(5);
  arr.touch(9) = 99;

  int expected[] = {1, 2, 3, 4, 5, 0, 0, 0, 0, 99};
  ASSERT_EQ(arr.size(), 10u);
  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr.read(i), expected[i]) << "at index " << i;
}

TEST(DynArrayMovedFrom, StaysEmptyAndReusable)
{
  DynArray<int> source;
  source.append(1);
  DynArray<int> target(std::move(source));
  ASSERT_EQ(target.read(0), 1);

  EXPECT_TRUE(source.is_empty());
  EXPECT_FALSE(source.exist(0));
  EXPECT_EQ(source.test(0), nullptr);
  EXPECT_TRUE(DynArray<int>(source).is_empty());

  size_t visited = 0;
  source.for_each([&visited] (int) { ++visited; });
  EXPECT_EQ(visited, 0u);
  source.empty();
  EXPECT_THROW((void) source.read(0), std::out_of_range);

  source.append(2);
  EXPECT_EQ(source.read(0), 2);

  DynArray<int> by_index(std::move(target));
  target[4] = 7;
  EXPECT_EQ(target.size(), 5u);
  EXPECT_EQ(target.read(4), 7);

  DynArray<int> by_reserve(std::move(by_index));
  by_index.reserve(3);
  EXPECT_EQ(by_index.size(), 3u);
  by_index = by_reserve;
  EXPECT_EQ(by_index.read(0), 1);
}

} // namespace
