/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>

#include <vector>
#include <string>
#include <htlist.H>
#include <ah-dry-mixin.H>

using namespace std;
using namespace Aleph;

// =============================================================================
// Test Container using CRTP Mixins
// =============================================================================

/// Simple test container using std::vector internally but with Aleph mixins
template <typename T>
class MixinVector : public TraverseMixin<MixinVector<T>, T>,
                    public LocateMixin<MixinVector<T>, T>,
                    public FunctionalMixin<MixinVector<T>, T>,
                    public KeysMixin<MixinVector<T>, T>
{
  std::vector<T> data;

public:
  using Item_Type = T;

  MixinVector() = default;
  MixinVector(std::initializer_list<T> init) : data(init) {}

  void append(const T & item) { data.push_back(item); }
  void insert(const T & item) { data.insert(data.begin(), item); }
  size_t size() const noexcept { return data.size(); }
  bool is_empty() const noexcept { return data.empty(); }

  // Iterator compatible with Aleph requirements
  class Iterator
  {
    const MixinVector * container;
    size_t pos;

  public:
    Iterator() : container(nullptr), pos(0) {}
    Iterator(const MixinVector & c) : container(&c), pos(0) {}

    bool has_curr() const noexcept { return pos < container->data.size(); }
    T & get_curr() const { return const_cast<T&>(container->data[pos]); }
    void next() { ++pos; }
    void next_ne() noexcept { ++pos; }
  };

  Iterator get_it() const { return Iterator(*this); }

  // For testing equality
  bool operator==(const MixinVector & other) const { return data == other.data; }
};


// =============================================================================
// TraverseMixin Tests
// =============================================================================

TEST(TraverseMixin, BasicTraverse)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  int sum = 0;
  bool completed = v.traverse([&sum](int x) {
    sum += x;
    return true;
  });

  EXPECT_TRUE(completed);
  EXPECT_EQ(sum, 15);
}

TEST(TraverseMixin, TraverseEarlyStop)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  int sum = 0;
  bool completed = v.traverse([&sum](int x) {
    if (x > 3) return false;
    sum += x;
    return true;
  });

  EXPECT_FALSE(completed);
  EXPECT_EQ(sum, 6);  // 1 + 2 + 3
}

TEST(TraverseMixin, TraverseEmpty)
{
  MixinVector<int> v;

  bool completed = v.traverse([](int) { return true; });
  EXPECT_TRUE(completed);
}


// =============================================================================
// FunctionalMixin Tests
// =============================================================================

TEST(FunctionalMixin, ForEach)
{
  MixinVector<int> v = {1, 2, 3};

  int sum = 0;
  v.for_each([&sum](int x) { sum += x; });

  EXPECT_EQ(sum, 6);
}

TEST(FunctionalMixin, MutableForEach)
{
  MixinVector<int> v = {1, 2, 3};

  v.mutable_for_each([](int& x) { x *= 2; });

  int sum = 0;
  v.for_each([&sum](int x) { sum += x; });
  EXPECT_EQ(sum, 12);  // 2 + 4 + 6
}

TEST(FunctionalMixin, AllPredicate)
{
  MixinVector<int> v = {2, 4, 6, 8};

  EXPECT_TRUE(v.all([](int x) { return x % 2 == 0; }));
  EXPECT_FALSE(v.all([](int x) { return x > 5; }));
}

TEST(FunctionalMixin, AllEmpty)
{
  MixinVector<int> v;
  // Vacuous truth
  EXPECT_TRUE(v.all([](int) { return false; }));
}

TEST(FunctionalMixin, ExistsPredicate)
{
  MixinVector<int> v = {1, 3, 5, 6, 7};

  EXPECT_TRUE(v.exists([](int x) { return x % 2 == 0; }));  // 6
  EXPECT_FALSE(v.exists([](int x) { return x > 100; }));
}

TEST(FunctionalMixin, ExistsEmpty)
{
  MixinVector<int> v;
  EXPECT_FALSE(v.exists([](int) { return true; }));
}

TEST(FunctionalMixin, Maps)
{
  MixinVector<int> v = {1, 2, 3};

  auto squared = v.maps<int>([](int x) { return x * x; });

  EXPECT_EQ(squared.size(), 3);

  int expected[] = {1, 4, 9};
  int i = 0;
  squared.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });
}

TEST(FunctionalMixin, MapsTypeConversion)
{
  MixinVector<int> v = {1, 2, 3};

  auto strs = v.maps<string>([](int x) { return to_string(x); });

  EXPECT_EQ(strs.size(), 3);

  string expected[] = {"1", "2", "3"};
  int i = 0;
  strs.for_each([&](const string& s) { EXPECT_EQ(s, expected[i++]); });
}

TEST(FunctionalMixin, Filter)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  auto evens = v.filter([](int x) { return x % 2 == 0; });

  EXPECT_EQ(evens.size(), 3);

  int expected[] = {2, 4, 6};
  int i = 0;
  evens.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });
}

TEST(FunctionalMixin, FilterNone)
{
  MixinVector<int> v = {1, 3, 5, 7};

  auto evens = v.filter([](int x) { return x % 2 == 0; });
  EXPECT_TRUE(evens.is_empty());
}

TEST(FunctionalMixin, Foldl)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  int sum = v.foldl<int>(0, [](int acc, int x) { return acc + x; });
  EXPECT_EQ(sum, 15);

  int product = v.foldl<int>(1, [](int acc, int x) { return acc * x; });
  EXPECT_EQ(product, 120);
}

TEST(FunctionalMixin, FoldlEmpty)
{
  MixinVector<int> v;

  int result = v.foldl<int>(42, [](int acc, int x) { return acc + x; });
  EXPECT_EQ(result, 42);
}

TEST(FunctionalMixin, Partition)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  auto [evens, odds] = v.partition([](int x) { return x % 2 == 0; });

  EXPECT_EQ(evens.size(), 3);
  EXPECT_EQ(odds.size(), 3);

  int expected_evens[] = {2, 4, 6};
  int expected_odds[] = {1, 3, 5};

  int i = 0;
  evens.for_each([&](int x) { EXPECT_EQ(x, expected_evens[i++]); });

  i = 0;
  odds.for_each([&](int x) { EXPECT_EQ(x, expected_odds[i++]); });
}

TEST(FunctionalMixin, Length)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(v.length(), 5);

  MixinVector<int> empty;
  EXPECT_EQ(empty.length(), 0);
}

TEST(FunctionalMixin, Rev)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto reversed = v.rev();

  EXPECT_EQ(reversed.size(), 5);

  int expected[] = {5, 4, 3, 2, 1};
  int i = 0;
  reversed.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });
}

TEST(FunctionalMixin, Take)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto first3 = v.take(3);
  EXPECT_EQ(first3.size(), 3);

  int expected[] = {1, 2, 3};
  int i = 0;
  first3.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });

  auto all = v.take(100);
  EXPECT_EQ(all.size(), 5);
}

TEST(FunctionalMixin, Drop)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto last3 = v.drop(2);
  EXPECT_EQ(last3.size(), 3);

  int expected[] = {3, 4, 5};
  int i = 0;
  last3.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });

  auto empty = v.drop(100);
  EXPECT_TRUE(empty.is_empty());
}


// =============================================================================
// LocateMixin Tests
// =============================================================================

TEST(LocateMixin, Nth)
{
  MixinVector<int> v = {10, 20, 30, 40, 50};

  EXPECT_EQ(v.nth(0), 10);
  EXPECT_EQ(v.nth(2), 30);
  EXPECT_EQ(v.nth(4), 50);

  EXPECT_THROW(v.nth(5), std::out_of_range);
  EXPECT_THROW(v.nth(100), std::out_of_range);
}

TEST(LocateMixin, NthNe)
{
  MixinVector<int> v = {10, 20, 30};

  EXPECT_EQ(v.nth_ne(0), 10);
  EXPECT_EQ(v.nth_ne(1), 20);
  EXPECT_EQ(v.nth_ne(2), 30);
}

TEST(LocateMixin, NthModify)
{
  MixinVector<int> v = {10, 20, 30};

  v.nth(1) = 25;
  EXPECT_EQ(v.nth(1), 25);
}

TEST(LocateMixin, FindPtr)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  int* p = v.find_ptr([](int x) { return x == 3; });
  ASSERT_NE(p, nullptr);
  EXPECT_EQ(*p, 3);

  int* not_found = v.find_ptr([](int x) { return x == 100; });
  EXPECT_EQ(not_found, nullptr);
}

TEST(LocateMixin, FindPtrModify)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  int* p = v.find_ptr([](int x) { return x == 3; });
  ASSERT_NE(p, nullptr);
  *p = 30;

  EXPECT_EQ(v.nth(2), 30);
}

TEST(LocateMixin, FindItem)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto [found, item] = v.find_item([](int x) { return x > 3; });
  EXPECT_TRUE(found);
  EXPECT_EQ(item, 4);

  auto [not_found, default_val] = v.find_item([](int x) { return x > 100; });
  EXPECT_FALSE(not_found);
}


// =============================================================================
// KeysMixin Tests
// =============================================================================

TEST(KeysMixin, Keys)
{
  MixinVector<int> v = {1, 2, 3};

  auto k = v.keys();
  EXPECT_EQ(k.size(), 3);
}

TEST(KeysMixin, Items)
{
  MixinVector<string> v = {"a", "b", "c"};

  auto items = v.items();
  EXPECT_EQ(items.size(), 3);
}


// =============================================================================
// Chained Operations Tests
// =============================================================================

TEST(ChainedOps, FilterThenMap)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  // Filter evens, then square them
  auto evens = v.filter([](int x) { return x % 2 == 0; });
  auto squared = evens.maps<int>([](int x) { return x * x; });

  EXPECT_EQ(squared.size(), 3);

  int expected[] = {4, 16, 36};
  int i = 0;
  squared.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });
}

TEST(ChainedOps, MapThenFold)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  // Square then sum
  auto squared = v.maps<int>([](int x) { return x * x; });
  int sum = squared.foldl<int>(0, [](int acc, int x) { return acc + x; });

  EXPECT_EQ(sum, 1 + 4 + 9 + 16 + 25);
}


// =============================================================================
// Complex Type Tests
// =============================================================================

struct Person
{
  string name;
  int age;
};

TEST(ComplexTypes, StructOperations)
{
  MixinVector<Person> people;
  people.append({"Alice", 30});
  people.append({"Bob", 25});
  people.append({"Charlie", 35});

  // Find oldest
  Person* oldest = people.find_ptr([](const Person& p) {
    return p.age >= 35;
  });
  ASSERT_NE(oldest, nullptr);
  EXPECT_EQ(oldest->name, "Charlie");

  // Get ages
  auto ages = people.maps<int>([](const Person& p) { return p.age; });
  int sum = ages.foldl<int>(0, [](int acc, int a) { return acc + a; });
  EXPECT_EQ(sum, 90);

  // Filter by age
  auto over27 = people.filter([](const Person& p) { return p.age > 27; });
  EXPECT_EQ(over27.size(), 2);
}


// =============================================================================
// Stress Tests
// =============================================================================

TEST(StressTest, LargeContainer)
{
  constexpr size_t N = 10000;
  MixinVector<size_t> v;

  for (size_t i = 0; i < N; ++i)
    v.append(i);

  // Sum
  size_t sum = v.foldl<size_t>(0, [](size_t acc, size_t x) { return acc + x; });
  EXPECT_EQ(sum, N * (N - 1) / 2);

  // Count evens
  auto evens = v.filter([](size_t x) { return x % 2 == 0; });
  EXPECT_EQ(evens.size(), N / 2);

  // All positive
  EXPECT_TRUE(v.all([](size_t x) { return x < N; }));
}

