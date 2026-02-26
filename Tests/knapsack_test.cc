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
 * @file knapsack_test.cc
 * @brief Tests for Knapsack.H.
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <cstdint>
# include <limits>
# include <random>

# include <Knapsack.H>

using namespace Aleph;

using Item = Knapsack_Item<int, int>;

namespace
{
  int selected_value(const Array<Item> & items,
                     const Array<size_t> & selected)
  {
    int total = 0;
    for (size_t i = 0; i < selected.size(); ++i)
      total += items[selected[i]].value;
    return total;
  }

  int selected_weight(const Array<Item> & items,
                      const Array<size_t> & selected)
  {
    int total = 0;
    for (size_t i = 0; i < selected.size(); ++i)
      total += items[selected[i]].weight;
    return total;
  }

  int brute_force_knapsack_01(const Array<Item> & items, const int capacity)
  {
    const size_t n = items.size();
    const uint64_t total_masks = uint64_t(1) << n;
    int best = 0;

    for (uint64_t mask = 0; mask < total_masks; ++mask)
      {
        int w = 0, v = 0;
        for (size_t i = 0; i < n; ++i)
          if (mask & (uint64_t(1) << i))
            {
              w += items[i].weight;
              v += items[i].value;
            }
        if (w <= capacity and v > best)
          best = v;
      }

    return best;
  }

  int brute_force_bounded(const Array<Item> & items, const Array<size_t> & counts,
                          const int capacity, const size_t i = 0)
  {
    if (i == items.size())
      return 0;

    int best = brute_force_bounded(items, counts, capacity, i + 1);
    const int wi = items[i].weight;
    const int vi = items[i].value;

    if (wi == 0)
      {
        if (vi > 0 and counts[i] > 0)
          best = std::max(best,
                          static_cast<int>(counts[i]) * vi
                          + brute_force_bounded(items, counts, capacity, i + 1));
        return best;
      }

    for (size_t take = 1; take <= counts[i]; ++take)
      {
        const int weight = static_cast<int>(take) * wi;
        if (weight > capacity)
          break;
        best = std::max(best,
                        static_cast<int>(take) * vi
                        + brute_force_bounded(items, counts,
                                              capacity - weight, i + 1));
      }

    return best;
  }

  int brute_force_unbounded(const Array<Item> & items,
                            const int capacity, const size_t i = 0)
  {
    if (i == items.size())
      return 0;

    int best = brute_force_unbounded(items, capacity, i + 1);
    const int wi = items[i].weight;
    const int vi = items[i].value;

    if (wi <= 0)
      return best;

    for (int take = 1; take * wi <= capacity; ++take)
      best = std::max(best,
                      take * vi + brute_force_unbounded(items,
                                                        capacity - take * wi,
                                                        i + 1));

    return best;
  }
} // namespace


TEST(Knapsack01, EmptyItems)
{
  Array<Item> items;
  auto r = knapsack_01(items, 10);
  EXPECT_EQ(r.optimal_value, 0);
  EXPECT_EQ(r.selected_items.size(), 0u);
}

TEST(Knapsack01, ZeroCapacity)
{
  Array<Item> items = {{5, 10}, {3, 7}};
  auto r = knapsack_01(items, 0);
  EXPECT_EQ(r.optimal_value, 0);
}

TEST(Knapsack01, ClassicExample)
{
  // Items: (weight, value): (2,3), (3,4), (4,5), (5,6)
  // Capacity = 8
  // Best: items 1,2,3 (w=3+4+5=12 too heavy), items 0,1,2 (w=2+3+4=9 too heavy)
  // items 0,2 (w=2+4=6, v=3+5=8), items 1,3 (w=3+5=8, v=4+6=10)
  Array<Item> items = {{2, 3}, {3, 4}, {4, 5}, {5, 6}};
  auto r = knapsack_01(items, 8);
  EXPECT_EQ(r.optimal_value, 10);

  // Verify selected items have correct total weight and value
  int total_w = 0, total_v = 0;
  for (size_t k = 0; k < r.selected_items.size(); ++k)
    {
      const auto & it = items[r.selected_items[k]];
      total_w += it.weight;
      total_v += it.value;
    }
  EXPECT_EQ(total_v, r.optimal_value);
  EXPECT_LE(total_w, 8);
}

TEST(Knapsack01, AllFit)
{
  Array<Item> items = {{1, 10}, {1, 20}, {1, 30}};
  auto r = knapsack_01(items, 100);
  EXPECT_EQ(r.optimal_value, 60);
  EXPECT_EQ(r.selected_items.size(), 3u);
}

TEST(Knapsack01, ValueOnly)
{
  Array<Item> items = {{2, 3}, {3, 4}, {4, 5}, {5, 6}};
  EXPECT_EQ(knapsack_01_value(items, 8), 10);
}

TEST(Knapsack01, NegativeCapacity)
{
  Array<Item> items = {{1, 1}};
  EXPECT_THROW(knapsack_01(items, -1), std::domain_error);
  EXPECT_THROW(knapsack_01_value(items, -1), std::domain_error);
}

TEST(Knapsack01, ZeroWeightItemsAtZeroCapacity)
{
  Array<Item> items = {{0, 5}, {0, 2}, {2, 7}};
  const auto r = knapsack_01(items, 0);
  EXPECT_EQ(r.optimal_value, 7);
  EXPECT_EQ(selected_weight(items, r.selected_items), 0);
  EXPECT_EQ(selected_value(items, r.selected_items), 7);
  EXPECT_EQ(knapsack_01_value(items, 0), 7);
}

TEST(Knapsack, NegativeWeightRejected)
{
  Array<Item> bad = {{-1, 10}, {2, 3}};
  Array<size_t> counts = {1, 1};

  EXPECT_THROW(knapsack_01(bad, 10), std::domain_error);
  EXPECT_THROW(knapsack_01_value(bad, 10), std::domain_error);
  EXPECT_THROW(knapsack_unbounded(bad, 10), std::domain_error);
  EXPECT_THROW(knapsack_bounded(bad, counts, 10), std::domain_error);
}

TEST(KnapsackUnbounded, Classic)
{
  // Items: (weight, value): (1,1), (3,4), (4,5)
  // Capacity = 7
  // Best unbounded: 2 * item1 (w=6,v=8) + 1 * item0 (w=1,v=1) = v=9
  // Or: 1 * item2 (w=4,v=5) + 1 * item1 (w=3,v=4) = v=9
  Array<Item> items = {{1, 1}, {3, 4}, {4, 5}};
  auto r = knapsack_unbounded(items, 7);
  EXPECT_EQ(r.optimal_value, 9);

  // Verify weight
  int total_w = 0;
  for (size_t k = 0; k < r.selected_items.size(); ++k)
    total_w += items[r.selected_items[k]].weight;
  EXPECT_LE(total_w, 7);
}

TEST(KnapsackUnbounded, ZeroWeightPositiveValueRejected)
{
  Array<Item> items = {{0, 1}, {3, 4}};
  EXPECT_THROW(knapsack_unbounded(items, 7), std::domain_error);
}

TEST(KnapsackUnbounded, Empty)
{
  Array<Item> items;
  auto r = knapsack_unbounded(items, 10);
  EXPECT_EQ(r.optimal_value, 0);
}

TEST(KnapsackBounded, Classic)
{
  // Item 0: w=2, v=3, count=2
  // Item 1: w=3, v=5, count=1
  // Capacity = 7
  // Best: 2 * item0 (w=4,v=6) + 1 * item1 (w=3,v=5) = w=7, v=11
  Array<Item> items = {{2, 3}, {3, 5}};
  Array<size_t> counts = {2, 1};
  auto r = knapsack_bounded(items, counts, 7);
  EXPECT_EQ(r.optimal_value, 11);

  // Verify weight and count constraints
  int total_w = 0, total_v = 0;
  Array<size_t> used = Array<size_t>::create(items.size());
  for (size_t i = 0; i < items.size(); ++i)
    used(i) = 0;

  for (size_t k = 0; k < r.selected_items.size(); ++k)
    {
      const size_t idx = r.selected_items[k];
      total_w += items[idx].weight;
      total_v += items[idx].value;
      used(idx)++;
    }
  EXPECT_EQ(total_v, r.optimal_value);
  EXPECT_LE(total_w, 7);
  for (size_t i = 0; i < items.size(); ++i)
    EXPECT_LE(used(i), counts[i]);
}

TEST(KnapsackBounded, SizeMismatch)
{
  Array<Item> items = {{1, 1}};
  Array<size_t> counts = {1, 2};
  EXPECT_THROW(knapsack_bounded(items, counts, 10), std::invalid_argument);
}

TEST(Knapsack01, RandomVsBruteForce)
{
  std::mt19937 rng(123456);
  for (int trial = 0; trial < 80; ++trial)
    {
      const size_t n = 1 + rng() % 10;
      const int capacity = static_cast<int>(rng() % 14);
      Array<Item> items;
      items.reserve(n);

      for (size_t i = 0; i < n; ++i)
        {
          const int w = static_cast<int>(rng() % 7);      // includes zero
          const int v = static_cast<int>(rng() % 19) - 4; // may be negative
          items.append(Item{w, v});
        }

      const auto r = knapsack_01(items, capacity);
      const int brute = brute_force_knapsack_01(items, capacity);
      const int value_only = knapsack_01_value(items, capacity);

      EXPECT_EQ(r.optimal_value, brute);
      EXPECT_EQ(value_only, brute);
      EXPECT_EQ(selected_value(items, r.selected_items), r.optimal_value);
      EXPECT_LE(selected_weight(items, r.selected_items), capacity);
    }
}

TEST(KnapsackUnbounded, RandomVsBruteForce)
{
  std::mt19937 rng(424242);
  for (int trial = 0; trial < 60; ++trial)
    {
      const size_t n = 1 + rng() % 7;
      const int capacity = 1 + static_cast<int>(rng() % 16);
      Array<Item> items;
      items.reserve(n);

      for (size_t i = 0; i < n; ++i)
        {
          const int w = 1 + static_cast<int>(rng() % 6);  // strictly positive
          const int v = static_cast<int>(rng() % 16) - 3;
          items.append(Item{w, v});
        }

      const auto r = knapsack_unbounded(items, capacity);
      const int brute = brute_force_unbounded(items, capacity);
      EXPECT_EQ(r.optimal_value, brute);
      EXPECT_EQ(selected_value(items, r.selected_items), r.optimal_value);
      EXPECT_LE(selected_weight(items, r.selected_items), capacity);
    }
}

TEST(KnapsackBounded, RandomVsBruteForce)
{
  std::mt19937 rng(777);
  for (int trial = 0; trial < 60; ++trial)
    {
      const size_t n = 1 + rng() % 8;
      const int capacity = static_cast<int>(rng() % 15);
      Array<Item> items;
      Array<size_t> counts;
      items.reserve(n);
      counts.reserve(n);

      for (size_t i = 0; i < n; ++i)
        {
          const int w = static_cast<int>(rng() % 5);      // includes zero
          const int v = static_cast<int>(rng() % 17) - 2;
          const size_t c = static_cast<size_t>(rng() % 4);
          items.append(Item{w, v});
          counts.append(c);
        }

      const auto r = knapsack_bounded(items, counts, capacity);
      const int brute = brute_force_bounded(items, counts, capacity);
      EXPECT_EQ(r.optimal_value, brute);
      EXPECT_EQ(selected_value(items, r.selected_items), r.optimal_value);
      EXPECT_LE(selected_weight(items, r.selected_items), capacity);

      Array<size_t> used = Array<size_t>::create(n);
      for (size_t i = 0; i < n; ++i)
        used(i) = 0;
      for (size_t i = 0; i < r.selected_items.size(); ++i)
        ++used(r.selected_items[i]);
      for (size_t i = 0; i < n; ++i)
        EXPECT_LE(used(i), counts[i]);
    }
}
