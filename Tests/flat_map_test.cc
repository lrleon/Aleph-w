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
 * @file flat_map_test.cc
 * @brief Tests for Aleph::FlatMap (tpl_flat_map.H).
 *
 * Covers construction, operator[]/at, insert/insert_or_assign/emplace,
 * erase, proxy iterators (structured bindings, value mutation through
 * iterators), keys()/values(), traverse, and a randomized parity check
 * against std::map as the reference implementation.
 */

#include <iterator>
#include <limits>
#include <map>
#include <new>
#include <random>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_flat_map.H>

using Aleph::FlatMap;

namespace
{
// A value whose move assignment throws once a budget of them is spent, to
// interrupt a shift or a relocation half way. Nothing else throws.
struct Throwing_Move
{
  static inline int moves_left = -1; // -1 means that it never throws

  int value = 0;

  Throwing_Move() = default;
  Throwing_Move(const int v) : value(v) {}
  Throwing_Move(const Throwing_Move &) = default;
  Throwing_Move(Throwing_Move &&) = default;
  Throwing_Move &operator = (const Throwing_Move &) = default;
  Throwing_Move &operator = (Throwing_Move &&other)
  {
    if (moves_left == 0)
      throw std::runtime_error("move assignment");
    if (moves_left > 0)
      --moves_left;
    value = other.value;
    return *this;
  }
};

// A value that moves without throwing but whose copies throw on demand.
struct Throwing_Copy
{
  static inline bool fail = false;

  int value = 0;

  Throwing_Copy() = default;
  Throwing_Copy(const int v) : value(v) {}
  Throwing_Copy(const Throwing_Copy &other) : value(other.value)
  {
    if (fail)
      throw std::runtime_error("copy");
  }
  Throwing_Copy(Throwing_Copy &&) noexcept = default;
  Throwing_Copy &operator = (const Throwing_Copy &other)
  {
    if (fail)
      throw std::runtime_error("copy assignment");
    value = other.value;
    return *this;
  }
  Throwing_Copy &operator = (Throwing_Copy &&) noexcept = default;
};

// A value whose default construction fails on demand, as the allocation of
// a buffer does: a new buffer is filled with default values before any
// entry is moved to it. Its moves throw or not, by declaration only.
template <bool Nothrow_Move>
struct Failing_Default
{
  static inline bool fail = false;

  int value = 0;

  Failing_Default()
  {
    if (fail)
      throw std::bad_alloc();
  }
  Failing_Default(const int v) : value(v) {}
  Failing_Default(const Failing_Default &) = default;
  Failing_Default(Failing_Default &&other) noexcept(Nothrow_Move) : value(other.value) {}
  Failing_Default &operator = (const Failing_Default &) = default;
  Failing_Default &operator = (Failing_Default &&other) noexcept(Nothrow_Move)
  {
    value = other.value;
    return *this;
  }
};

// The keys are strictly increasing and each one still has its own value,
// which every test below sets to ten times the key.
template <class Map>
::testing::AssertionResult is_consistent(const Map &m)
{
  for (size_t i = 0; i < m.size(); ++i)
    {
      if (i > 0 and not (m.nth_key(i - 1) < m.nth_key(i)))
        return ::testing::AssertionFailure()
          << "keys " << m.nth_key(i - 1) << " and " << m.nth_key(i)
          << " at " << i - 1 << " are not strictly increasing";
      if (m.nth_value(i).value != 10 * m.nth_key(i))
        return ::testing::AssertionFailure()
          << "key " << m.nth_key(i) << " holds " << m.nth_value(i).value;
    }
  return ::testing::AssertionSuccess();
}

class Pair_Input_Iterator
{
  const std::vector<std::pair<int, std::string>> *items_ = nullptr;
  size_t pos_ = 0;

public:

  using iterator_category = std::input_iterator_tag;
  using value_type = std::pair<int, std::string>;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = const value_type &;

  Pair_Input_Iterator() = default;

  Pair_Input_Iterator(const std::vector<std::pair<int, std::string>> &items,
                      size_t pos) noexcept
    : items_(&items), pos_(pos) {}

  reference operator*() const noexcept { return (*items_)[pos_]; }

  Pair_Input_Iterator &operator++() noexcept
  {
    ++pos_;
    return *this;
  }

  Pair_Input_Iterator operator++(int) noexcept
  {
    Pair_Input_Iterator ret = *this;
    ++(*this);
    return ret;
  }

  bool operator==(const Pair_Input_Iterator &it) const noexcept
  {
    return items_ == it.items_ and pos_ == it.pos_;
  }

  bool operator!=(const Pair_Input_Iterator &it) const noexcept
  {
    return not (*this == it);
  }
};
}  // namespace

TEST(FlatMap, EmptyMapBasics)
{
  FlatMap<std::string, int> m;
  EXPECT_TRUE(m.is_empty());
  EXPECT_EQ(m.size(), 0u);
  EXPECT_FALSE(m.contains("x"));
  EXPECT_EQ(m.find("x"), m.end());
  EXPECT_EQ(m.begin(), m.end());
}

TEST(FlatMap, SubscriptInsertsAndUpdates)
{
  FlatMap<std::string, int> m;
  m["b"] = 2;
  m["a"] = 1;
  m["c"] = 3;
  ASSERT_EQ(m.size(), 3u);

  m["a"] += 10;  // update through subscript
  EXPECT_EQ(m.at("a"), 11);

  // Absent key gets a default-constructed value.
  EXPECT_EQ(m["zz"], 0);
  EXPECT_EQ(m.size(), 4u);

  // Keys must come out sorted.
  EXPECT_EQ(m.nth_key(0), "a");
  EXPECT_EQ(m.nth_key(1), "b");
  EXPECT_EQ(m.nth_key(2), "c");
  EXPECT_EQ(m.nth_key(3), "zz");
}

TEST(FlatMap, AtThrowsOnMissingKey)
{
  FlatMap<std::string, int> m = {{"a", 1}};
  EXPECT_EQ(m.at("a"), 1);
  EXPECT_THROW((void) m.at("nope"), std::out_of_range);

  const FlatMap<std::string, int> &cm = m;
  EXPECT_EQ(cm.at("a"), 1);
  EXPECT_THROW((void) cm.at("nope"), std::out_of_range);
}

TEST(FlatMap, InitializerListDropsDuplicateKeysFirstWins)
{
  FlatMap<int, std::string> m = {{3, "three"}, {1, "one"},
                                  {3, "THREE"}, {2, "two"}};
  ASSERT_EQ(m.size(), 3u);
  EXPECT_EQ(m.at(3), "three");  // first occurrence wins
  EXPECT_EQ(m.nth_key(0), 1);
  EXPECT_EQ(m.nth_key(2), 3);
}

TEST(FlatMap, RangeConstructorAcceptsIteratorsWithoutArrow)
{
  const std::vector<std::pair<int, std::string>> items =
    {{3, "three"}, {1, "one"}, {2, "two"}};

  FlatMap<int, std::string> m(Pair_Input_Iterator(items, 0),
                              Pair_Input_Iterator(items, items.size()));

  ASSERT_EQ(m.size(), 3u);
  EXPECT_EQ(m.at(1), "one");
  EXPECT_EQ(m.at(2), "two");
  EXPECT_EQ(m.at(3), "three");
}

// The map's own iterators yield a (key, value) proxy with no common
// reference with std::pair, so they do not model std::input_iterator; the
// range constructor must still accept them.
TEST(FlatMap, RangeConstructorAcceptsOwnIteratorAndConstIterator)
{
  FlatMap<int, std::string> src = {{2, "two"}, {1, "one"}, {3, "three"}};
  const FlatMap<int, std::string> &csrc = src;

  FlatMap<int, std::string> from_it(src.begin(), src.end());
  FlatMap<int, std::string> from_cit(csrc.begin(), csrc.end());
  FlatMap<int, std::string> from_tail(src.begin() + 1, src.end());

  EXPECT_EQ(from_it, src);
  EXPECT_EQ(from_cit, src);
  ASSERT_EQ(from_tail.size(), 2u);
  EXPECT_FALSE(from_tail.contains(1));
  EXPECT_EQ(from_tail.at(3), "three");
}

TEST(FlatMap, InsertRejectsDuplicateInsertOrAssignOverwrites)
{
  FlatMap<int, int> m;
  EXPECT_TRUE(m.insert(1, 100).second);
  EXPECT_FALSE(m.insert(1, 999).second);
  EXPECT_EQ(m.at(1), 100);  // original value kept

  EXPECT_FALSE(m.insert_or_assign(1, 999).second);
  EXPECT_EQ(m.at(1), 999);  // overwritten

  EXPECT_TRUE(m.insert_or_assign(2, 200).second);
  EXPECT_EQ(m.at(2), 200);

  // Pair-based insert.
  EXPECT_TRUE(m.insert(std::pair<int, int>(3, 300)).second);
  EXPECT_EQ(m.at(3), 300);
}

TEST(FlatMap, EraseByKeyAndIterator)
{
  FlatMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}, {4, 40}};

  EXPECT_EQ(m.erase(2), 1u);
  EXPECT_EQ(m.erase(2), 0u);
  EXPECT_EQ(m.size(), 3u);

  auto it = m.find(3);
  ASSERT_NE(it, m.end());
  it = m.erase(it);
  EXPECT_EQ((*it).first, 4);  // entry following the erased one
  EXPECT_EQ(m.size(), 2u);

  EXPECT_THROW((void) m.erase(m.end()), std::out_of_range);
}

TEST(FlatMap, ProxyIterationAndStructuredBindings)
{
  FlatMap<std::string, int> m = {{"b", 2}, {"a", 1}, {"c", 3}};

  std::vector<std::string> ks;
  int sum = 0;
  for (auto [k, v] : m)
    {
      ks.push_back(k);
      sum += v;
    }
  EXPECT_EQ(ks, (std::vector<std::string>{"a", "b", "c"}));
  EXPECT_EQ(sum, 6);

  // Mutation through the iterator proxy.
  for (auto [k, v] : m)
    v *= 10;
  EXPECT_EQ(m.at("a"), 10);
  EXPECT_EQ(m.at("b"), 20);
  EXPECT_EQ(m.at("c"), 30);

  // Arrow access.
  auto it = m.find("b");
  ASSERT_NE(it, m.end());
  EXPECT_EQ(it->first, "b");
  it->second = 7;
  EXPECT_EQ(m.at("b"), 7);
}

TEST(FlatMap, IteratorRandomAccessOperations)
{
  FlatMap<int, int> m = {{1, 10}, {2, 20}, {3, 30}, {4, 40}};

  auto it = m.begin();
  EXPECT_EQ(m.end() - it, 4);
  EXPECT_EQ((it + 2)->first, 3);
  EXPECT_EQ(it[3].second, 40);
  ++it;
  --it;
  EXPECT_EQ(it->first, 1);
  EXPECT_TRUE(it < m.end());

  // Mutable-to-const iterator conversion.
  FlatMap<int, int>::const_iterator cit = m.begin();
  EXPECT_EQ(cit->second, 10);
}

TEST(FlatMap, BoundsAndEqualRange)
{
  const FlatMap<int, int> m = {{10, 1}, {20, 2}, {30, 3}};

  EXPECT_EQ(m.lower_bound(20)->first, 20);
  EXPECT_EQ(m.lower_bound(25)->first, 30);
  EXPECT_EQ(m.upper_bound(20)->first, 30);
  EXPECT_EQ(m.lower_bound(31), m.end());

  auto [lo, hi] = m.equal_range(20);
  EXPECT_EQ(hi - lo, 1);
  auto [lo2, hi2] = m.equal_range(15);
  EXPECT_EQ(lo2, hi2);
}

TEST(FlatMap, KeysAndValuesLists)
{
  FlatMap<std::string, int> m = {{"b", 2}, {"a", 1}, {"c", 3}};

  auto ks = m.keys();
  auto vs = m.values();
  ASSERT_EQ(ks.size(), 3u);
  ASSERT_EQ(vs.size(), 3u);
  EXPECT_EQ(ks.get_first(), "a");
  EXPECT_EQ(ks.get_last(), "c");
  EXPECT_EQ(vs.get_first(), 1);
  EXPECT_EQ(vs.get_last(), 3);
}

TEST(FlatMap, TraverseMutatingAndEarlyStop)
{
  FlatMap<int, int> m = {{1, 1}, {2, 2}, {3, 3}};

  EXPECT_TRUE(m.traverse([] (const int &, int &v) { v += 100; return true; }));
  EXPECT_EQ(m.at(1), 101);

  int visited = 0;
  const FlatMap<int, int> &cm = m;
  EXPECT_FALSE(cm.traverse([&visited] (const int &k, const int &)
  {
    ++visited;
    return k < 2;
  }));
  EXPECT_EQ(visited, 2);
}

TEST(FlatMap, CopyMoveEqualityAndSwap)
{
  FlatMap<std::string, int> m = {{"x", 1}, {"y", 2}};
  FlatMap<std::string, int> copy = m;
  EXPECT_EQ(copy, m);

  FlatMap<std::string, int> moved = std::move(copy);
  EXPECT_EQ(moved, m);

  moved["z"] = 3;
  EXPECT_NE(moved, m);

  FlatMap<std::string, int> other = {{"q", 9}};
  m.swap(other);
  EXPECT_EQ(m.size(), 1u);
  EXPECT_EQ(m.at("q"), 9);
  EXPECT_EQ(other.size(), 2u);
}

TEST(FlatMap, CustomComparatorDescendingOrder)
{
  FlatMap<int, std::string, std::greater<int>> m
    = {{1, "one"}, {5, "five"}, {3, "three"}};
  EXPECT_EQ(m.nth_key(0), 5);
  EXPECT_EQ(m.nth_key(1), 3);
  EXPECT_EQ(m.nth_key(2), 1);
  EXPECT_EQ(m.at(3), "three");
}

TEST(FlatMap, ParallelStorageAccessors)
{
  FlatMap<int, int> m = {{2, 20}, {1, 10}, {3, 30}};
  const int *kp = m.keys_data();
  const int *vp = m.values_data();
  for (size_t i = 0; i < m.size(); ++i)
    EXPECT_EQ(vp[i], kp[i] * 10);
}

// Randomized parity test: FlatMap must behave exactly like std::map under
// an arbitrary interleaving of subscripts, inserts, erases and lookups.
TEST(FlatMap, RandomizedParityWithStdMap)
{
  std::mt19937 rng(20260702);
  std::uniform_int_distribution<int> key_dist(0, 150);
  std::uniform_int_distribution<int> val_dist(0, 1000000);
  std::uniform_int_distribution<int> op_dist(0, 3);

  FlatMap<int, int> fm;
  std::map<int, int> ref;

  for (int step = 0; step < 4000; ++step)
    {
      const int k = key_dist(rng);
      const int v = val_dist(rng);
      switch (op_dist(rng))
        {
        case 0:
          fm[k] = v;
          ref[k] = v;
          break;
        case 1:
          EXPECT_EQ(fm.insert(k, v).second, ref.insert({k, v}).second);
          break;
        case 2:
          EXPECT_EQ(fm.erase(k), ref.erase(k));
          break;
        default:
          {
            const auto it = ref.find(k);
            if (it == ref.end())
              EXPECT_FALSE(fm.contains(k));
            else
              EXPECT_EQ(fm.at(k), it->second);
          }
          break;
        }
    }

  ASSERT_EQ(fm.size(), ref.size());
  size_t i = 0;
  for (const auto &[k, v] : ref)
    {
      EXPECT_EQ(fm.nth_key(i), k);
      EXPECT_EQ(fm.nth_value(i), v);
      ++i;
    }
}

TEST(FlatMap, InsertedValueMayBeAnEntryOfTheSameMap)
{
  FlatMap<int, std::string> m;
  const std::string thirty(40, 'c'); // longer than any small-string buffer
  m.insert(1, std::string(40, 'a'));
  m.insert(3, thirty);
  m.insert(5, std::string(40, 'e'));

  // without growth: the shift moves the value of 3 before it is read
  ASSERT_LT(m.size(), m.capacity());
  EXPECT_TRUE(m.insert(0, m.at(3)).second);
  EXPECT_EQ(m.at(0), thirty);
  EXPECT_EQ(m.at(3), thirty);

  // with growth: the reallocation frees the buffer that holds the value
  int next = 100;
  while (m.size() < m.capacity())
    m.insert(next++, std::string(40, 'x'));
  EXPECT_TRUE(m.insert(next, m.at(3)).second);
  EXPECT_EQ(m.at(next), thirty);

  while (m.size() < m.capacity())
    m.insert(++next, std::string(40, 'x'));
  EXPECT_TRUE(m.insert_or_assign(-1, m.at(3)).second);
  EXPECT_EQ(m.at(-1), thirty);

  while (m.size() < m.capacity())
    m.insert(++next, std::string(40, 'x'));
  EXPECT_TRUE(m.insert(-2, std::move(m.at(0))).second);
  EXPECT_EQ(m.at(-2), thirty);
}

TEST(FlatMap, InsertedKeyMayBeAValueOfTheSameMap)
{
  FlatMap<int, int> m;
  m.insert(1, 10);
  m.insert(3, 30);
  m.insert(5, 50);

  ASSERT_LT(m.size(), m.capacity());
  EXPECT_TRUE(m.insert(m.at(1), 7).second); // the key is the value 10
  EXPECT_EQ(m.at(10), 7);

  m[m.at(3)] = 8; // the key is the value 30
  EXPECT_EQ(m.at(30), 8);

  int next = 100;
  while (m.size() < m.capacity())
    m.insert(next++, 0);
  m[m.at(5)] = 9; // the key is the value 50, and the arrays grow
  EXPECT_EQ(m.at(50), 9);
  EXPECT_EQ(m.at(5), 50);
}

TEST(FlatMap, CopyThatThrowsLeavesTheMapUntouched)
{
  FlatMap<int, Throwing_Copy> m;
  for (const int k : {1, 3, 5, 7})
    m.insert(k, Throwing_Copy(10 * k));
  const Throwing_Copy value(0);

  Throwing_Copy::fail = true;
  EXPECT_THROW(m.insert(0, value), std::runtime_error);
  EXPECT_THROW(m.insert(4, value), std::runtime_error);
  EXPECT_THROW(m.insert_or_assign(9, value), std::runtime_error);
  Throwing_Copy::fail = false;

  ASSERT_EQ(m.size(), 4u);
  EXPECT_TRUE(is_consistent(m));
  EXPECT_FALSE(m.contains(0));
  EXPECT_FALSE(m.contains(4));
  EXPECT_FALSE(m.contains(9));
}

TEST(FlatMap, CopyAssignmentThatThrowsLeavesTheTargetUntouched)
{
  FlatMap<int, Throwing_Copy> source;
  for (const int k : {2, 4, 6, 8, 10})
    source.insert(k, Throwing_Copy(10 * k));
  FlatMap<int, Throwing_Copy> target;
  target.insert(1, Throwing_Copy(10));

  Throwing_Copy::fail = true;
  EXPECT_THROW(target = source, std::runtime_error);
  Throwing_Copy::fail = false;

  ASSERT_EQ(target.size(), 1u);
  EXPECT_EQ(target.nth_key(0), 1);
  EXPECT_EQ(target.nth_value(0).value, 10);

  target = source;
  ASSERT_EQ(target.size(), 5u);
  EXPECT_TRUE(is_consistent(target));
}

// If moving a value can throw, an insertion, a removal or a reservation
// interrupted at any of its moves must leave a valid map: either the
// operation took effect or the map was emptied, never a duplicated key or a
// key apart from its value.
TEST(FlatMap, MoveThatThrowsAtAnyPointLeavesAValidMap)
{
  const auto build = []
    {
      FlatMap<int, Throwing_Move> m;
      for (const int k : {1, 3, 5, 7, 9})
        m.insert(k, Throwing_Move(10 * k));
      return m;
    };

  enum class Operation { insert_first, insert_middle, erase_first, reserve };
  for (const auto operation : {Operation::insert_first, Operation::insert_middle,
                               Operation::erase_first, Operation::reserve})
    for (int budget = 0; budget < 12; ++budget)
      {
        SCOPED_TRACE(testing::Message() << "operation " << static_cast<int>(operation)
                                        << ", moves allowed " << budget);
        auto m = build();
        bool thrown = false;
        Throwing_Move::moves_left = budget;
        try
          {
            switch (operation)
              {
              case Operation::insert_first: m.insert(0, Throwing_Move(0)); break;
              case Operation::insert_middle: m.insert(4, Throwing_Move(40)); break;
              case Operation::erase_first: m.erase(1); break;
              case Operation::reserve: m.reserve(4 * m.capacity()); break;
              }
          }
        catch (const std::runtime_error &)
          {
            thrown = true;
          }
        Throwing_Move::moves_left = -1;

        EXPECT_TRUE(is_consistent(m));
        if (thrown)
          EXPECT_TRUE(m.is_empty());
        else if (operation == Operation::erase_first)
          EXPECT_EQ(m.size(), 4u);
        else if (operation == Operation::reserve)
          EXPECT_EQ(m.size(), 5u);
        else
          EXPECT_EQ(m.size(), 6u);

        // the map remains usable in both cases
        m.insert(100, Throwing_Move(1000));
        EXPECT_TRUE(m.contains(100));
        EXPECT_TRUE(is_consistent(m));
      }

  // the sweep reached the point where nothing throws
  auto m = build();
  Throwing_Move::moves_left = 11;
  EXPECT_NO_THROW(m.insert(0, Throwing_Move(0)));
  Throwing_Move::moves_left = -1;
}

// The map is emptied only when a move may have left an entry half way. A
// failure that comes before, while the new buffers are obtained, must not
// cost the entries, whatever the moves of their types declare.
TEST(FlatMap, FailureBeforeAnyEntryMovesKeepsTheMap)
{
  using Value = Failing_Default<false>;
  ASSERT_FALSE(std::is_nothrow_move_assignable_v<Value>);

  FlatMap<int, Value> m;
  for (int k = 0; k < 4; ++k)
    m.insert(k, Value(10 * k));
  ASSERT_EQ(m.size(), m.capacity()); // the next insertion has to grow

  // a capacity that cannot be rounded up
  EXPECT_THROW(m.reserve(std::numeric_limits<size_t>::max()), std::overflow_error);
  EXPECT_EQ(m.size(), 4u);
  EXPECT_TRUE(is_consistent(m));

  // the buffer of the values cannot be built, with that of the keys already in place
  Value::fail = true;
  EXPECT_THROW(m.reserve(64), std::bad_alloc);
  EXPECT_THROW(m.insert(9, Value(90)), std::bad_alloc);
  EXPECT_THROW(m.insert_or_assign(9, Value(90)), std::bad_alloc);
  Value::fail = false;
  EXPECT_EQ(m.size(), 4u);
  EXPECT_TRUE(is_consistent(m));
  EXPECT_FALSE(m.contains(9));

  m.insert(9, Value(90));
  m.reserve(64);
  EXPECT_EQ(m.size(), 5u);
  EXPECT_GE(m.capacity(), 64u);
  EXPECT_TRUE(is_consistent(m));
}

// An insertion that grows relocates the entries before it shifts them. A
// move that throws during that relocation empties the map, as one that
// throws during the shift does.
TEST(FlatMap, MoveThatThrowsWhileAnInsertionGrowsLeavesAValidMap)
{
  bool completed = false;
  for (int budget = 0; not completed; ++budget)
    {
      ASSERT_LT(budget, 64);
      SCOPED_TRACE(testing::Message() << "moves allowed " << budget);
      FlatMap<int, Throwing_Move> m;
      for (int k = 1; k <= 4; ++k)
        m.insert(k, Throwing_Move(10 * k));
      ASSERT_EQ(m.size(), m.capacity());

      Throwing_Move::moves_left = budget;
      try
        {
          m.insert(0, Throwing_Move(0));
          completed = true;
        }
      catch (const std::runtime_error &)
        {
          EXPECT_TRUE(m.is_empty());
        }
      Throwing_Move::moves_left = -1;

      EXPECT_TRUE(is_consistent(m));
      if (completed)
        {
          EXPECT_EQ(m.size(), 5u);
          EXPECT_GT(budget, 4); // the four entries were relocated and then shifted
        }
      m.insert(100, Throwing_Move(1000));
      EXPECT_TRUE(m.contains(100));
      EXPECT_TRUE(is_consistent(m));
    }
}

// A map emptied by a move has no storage. If only one of its two arrays
// gets a buffer, the map must not be left with a key and no value.
TEST(FlatMap, FailedGrowthOfAMapEmptiedByAMoveLeavesItEmpty)
{
  for (const bool nothrow_move : {true, false})
    {
      SCOPED_TRACE(testing::Message() << "nothrow move " << nothrow_move);
      const auto check = [] <class Value> (Value)
        {
          FlatMap<int, Value> source;
          source.insert(1, Value(10));
          FlatMap<int, Value> taken(std::move(source));
          ASSERT_EQ(source.capacity(), 0u);

          Value::fail = true; // the keys get their buffer, the values do not
          EXPECT_THROW(source.insert(2, Value(20)), std::bad_alloc);
          Value::fail = false;
          ASSERT_TRUE(source.is_empty());
          EXPECT_TRUE(source.begin() == source.end());

          source.insert(2, Value(20));
          EXPECT_EQ(source.size(), 1u);
          EXPECT_TRUE(is_consistent(source));
          EXPECT_EQ(taken.size(), 1u);
          EXPECT_TRUE(is_consistent(taken));
        };
      if (nothrow_move)
        check(Failing_Default<true>(0));
      else
        check(Failing_Default<false>(0));
    }
}
