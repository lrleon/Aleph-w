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
#include <map>
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
