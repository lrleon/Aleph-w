
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
 * @file ah-dry-mixin_test.cc
 * @brief Tests for Ah Dry Mixin
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
// Aggregation Tests (sum, product, min, max)
// =============================================================================

TEST(FunctionalMixin, Sum)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(v.sum(), 15);
  EXPECT_EQ(v.sum(10), 25);  // with initial value
}

TEST(FunctionalMixin, SumEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.sum(), 0);
  EXPECT_EQ(v.sum(100), 100);
}

TEST(FunctionalMixin, SumStrings)
{
  MixinVector<string> v = {"Hello", " ", "World"};
  EXPECT_EQ(v.sum(), "Hello World");
  EXPECT_EQ(v.sum(string("Prefix: ")), "Prefix: Hello World");
}

TEST(FunctionalMixin, Product)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};
  EXPECT_EQ(v.product(1), 120);
  EXPECT_EQ(v.product(2), 240);
}

TEST(FunctionalMixin, ProductEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.product(1), 1);
  EXPECT_EQ(v.product(0), 0);
}

TEST(FunctionalMixin, Min)
{
  MixinVector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

  const int* min_ptr = v.min();
  ASSERT_NE(min_ptr, nullptr);
  EXPECT_EQ(*min_ptr, 1);
}

TEST(FunctionalMixin, MinEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.min(), nullptr);
}

TEST(FunctionalMixin, MinSingle)
{
  MixinVector<int> v = {42};
  ASSERT_NE(v.min(), nullptr);
  EXPECT_EQ(*v.min(), 42);
}

TEST(FunctionalMixin, Max)
{
  MixinVector<int> v = {3, 1, 4, 1, 5, 9, 2, 6};

  const int* max_ptr = v.max();
  ASSERT_NE(max_ptr, nullptr);
  EXPECT_EQ(*max_ptr, 9);
}

TEST(FunctionalMixin, MaxEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.max(), nullptr);
}

TEST(FunctionalMixin, MaxSingle)
{
  MixinVector<int> v = {42};
  ASSERT_NE(v.max(), nullptr);
  EXPECT_EQ(*v.max(), 42);
}

TEST(FunctionalMixin, MinByCustomComparator)
{
  MixinVector<string> v = {"apple", "pie", "a", "banana"};

  const string* shortest = v.min_by([](const string& a, const string& b) {
    return a.length() < b.length();
  });

  ASSERT_NE(shortest, nullptr);
  EXPECT_EQ(*shortest, "a");
}

TEST(FunctionalMixin, MaxByCustomComparator)
{
  MixinVector<string> v = {"apple", "pie", "a", "banana"};

  const string* longest = v.max_by([](const string& a, const string& b) {
    return a.length() < b.length();
  });

  ASSERT_NE(longest, nullptr);
  EXPECT_EQ(*longest, "banana");
}

TEST(FunctionalMixin, MinMaxWithNegatives)
{
  MixinVector<int> v = {-5, 3, -10, 7, 0};

  EXPECT_EQ(*v.min(), -10);
  EXPECT_EQ(*v.max(), 7);
}


// =============================================================================
// Search and Counting Tests (contains, none, count_if)
// =============================================================================

TEST(FunctionalMixin, HasValue)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  EXPECT_TRUE(v.has_value(3));
  EXPECT_TRUE(v.has_value(1));
  EXPECT_TRUE(v.has_value(5));
  EXPECT_FALSE(v.has_value(0));
  EXPECT_FALSE(v.has_value(6));
  EXPECT_FALSE(v.has_value(100));
}

TEST(FunctionalMixin, HasValueEmpty)
{
  MixinVector<int> v;
  EXPECT_FALSE(v.has_value(1));
}

TEST(FunctionalMixin, HasValueStrings)
{
  MixinVector<string> v = {"apple", "banana", "cherry"};

  EXPECT_TRUE(v.has_value("banana"));
  EXPECT_FALSE(v.has_value("Banana"));  // case sensitive
  EXPECT_FALSE(v.has_value("grape"));
}

TEST(FunctionalMixin, None)
{
  MixinVector<int> v = {2, 4, 6, 8, 10};

  EXPECT_TRUE(v.none([](int x) { return x % 2 != 0; }));  // no odds
  EXPECT_FALSE(v.none([](int x) { return x > 5; }));       // some > 5
}

TEST(FunctionalMixin, NoneEmpty)
{
  MixinVector<int> v;
  EXPECT_TRUE(v.none([](int) { return true; }));  // vacuously true
}

TEST(FunctionalMixin, NoneLValuePredicate)
{
  MixinVector<int> v = {2, 4, 6};

  auto is_odd = [](int x) { return x % 2 != 0; };
  EXPECT_TRUE(v.none(is_odd));
}

TEST(FunctionalMixin, CountIf)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  EXPECT_EQ(v.count_if([](int x) { return x % 2 == 0; }), 5);  // evens
  EXPECT_EQ(v.count_if([](int x) { return x > 5; }), 5);        // > 5
  EXPECT_EQ(v.count_if([](int x) { return x > 100; }), 0);      // none
  EXPECT_EQ(v.count_if([](int) { return true; }), 10);           // all
}

TEST(FunctionalMixin, CountIfEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.count_if([](int) { return true; }), 0);
}


// =============================================================================
// First/Last Element Tests
// =============================================================================

TEST(FunctionalMixin, First)
{
  MixinVector<int> v = {10, 20, 30};

  const int* ptr = v.first();
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 10);
}

TEST(FunctionalMixin, FirstEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.first(), nullptr);
}

TEST(FunctionalMixin, FirstOr)
{
  MixinVector<int> v = {10, 20, 30};
  EXPECT_EQ(v.first_or(-1), 10);

  MixinVector<int> empty;
  EXPECT_EQ(empty.first_or(-1), -1);
}

TEST(FunctionalMixin, Last)
{
  MixinVector<int> v = {10, 20, 30};

  const int* ptr = v.last();
  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 30);
}

TEST(FunctionalMixin, LastEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.last(), nullptr);
}

TEST(FunctionalMixin, LastOr)
{
  MixinVector<int> v = {10, 20, 30};
  EXPECT_EQ(v.last_or(-1), 30);

  MixinVector<int> empty;
  EXPECT_EQ(empty.last_or(-1), -1);
}

TEST(FunctionalMixin, FirstLastSingleElement)
{
  MixinVector<int> v = {42};

  EXPECT_EQ(*v.first(), 42);
  EXPECT_EQ(*v.last(), 42);
  EXPECT_EQ(v.first_or(-1), 42);
  EXPECT_EQ(v.last_or(-1), 42);
}


// =============================================================================
// Enumeration and Indexing Tests
// =============================================================================

TEST(FunctionalMixin, Enumerate)
{
  MixinVector<string> v = {"a", "b", "c"};

  auto enumerated = v.enumerate();
  EXPECT_EQ(enumerated.size(), 3);

  size_t expected_idx = 0;
  string expected_vals[] = {"a", "b", "c"};
  enumerated.for_each([&](const std::pair<size_t, string>& p) {
    EXPECT_EQ(p.first, expected_idx);
    EXPECT_EQ(p.second, expected_vals[expected_idx]);
    ++expected_idx;
  });
}

TEST(FunctionalMixin, EnumerateEmpty)
{
  MixinVector<int> v;
  auto enumerated = v.enumerate();
  EXPECT_TRUE(enumerated.is_empty());
}

TEST(FunctionalMixin, FindIndex)
{
  MixinVector<int> v = {10, 20, 30, 40, 50};

  EXPECT_EQ(v.find_index([](int x) { return x == 30; }), 2);
  EXPECT_EQ(v.find_index([](int x) { return x > 35; }), 3);  // first > 35 is 40
  EXPECT_EQ(v.find_index([](int x) { return x > 100; }), static_cast<size_t>(-1));
}

TEST(FunctionalMixin, FindIndexEmpty)
{
  MixinVector<int> v;
  EXPECT_EQ(v.find_index([](int) { return true; }), static_cast<size_t>(-1));
}

TEST(FunctionalMixin, IndexOf)
{
  MixinVector<int> v = {10, 20, 30, 40, 50};

  EXPECT_EQ(v.index_of(10), 0);
  EXPECT_EQ(v.index_of(30), 2);
  EXPECT_EQ(v.index_of(50), 4);
  EXPECT_EQ(v.index_of(99), static_cast<size_t>(-1));
}

TEST(FunctionalMixin, IndexOfStrings)
{
  MixinVector<string> v = {"apple", "banana", "cherry"};

  EXPECT_EQ(v.index_of("banana"), 1);
  EXPECT_EQ(v.index_of("grape"), static_cast<size_t>(-1));
}


// =============================================================================
// Combined Operations with New Methods
// =============================================================================

TEST(ChainedOps, SumAfterFoldl)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};

  // Sum of evens using filter + foldl (since filter returns DynList)
  auto evens = v.filter([](int x) { return x % 2 == 0; });
  int sum = evens.foldl<int>(0, [](int acc, int x) { return acc + x; });
  EXPECT_EQ(sum, 2 + 4 + 6 + 8 + 10);

  // Direct sum on MixinVector
  EXPECT_EQ(v.sum(), 55);
}

TEST(ChainedOps, MinMaxOnMixinVector)
{
  MixinVector<int> v = {1, 3, 5, 7, 9};  // odds only

  EXPECT_EQ(*v.min(), 1);
  EXPECT_EQ(*v.max(), 9);
}

TEST(ChainedOps, EnumerateThenCountElements)
{
  MixinVector<string> v = {"a", "bb", "ccc", "dd"};

  auto enumerated = v.enumerate();
  EXPECT_EQ(enumerated.size(), 4);

  // Count even indices using the mixin's count_if
  // (enumerated is DynList, so we verify via foldl)
  int even_count = enumerated.foldl<int>(0, [](int acc, const std::pair<size_t, string>& p) {
    return acc + (p.first % 2 == 0 ? 1 : 0);
  });
  EXPECT_EQ(even_count, 2);  // indices 0 and 2
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

  // Sum using foldl
  size_t sum = v.foldl<size_t>(0, [](size_t acc, size_t x) { return acc + x; });
  EXPECT_EQ(sum, N * (N - 1) / 2);

  // Sum using sum()
  EXPECT_EQ(v.sum(), N * (N - 1) / 2);

  // Count evens
  auto evens = v.filter([](size_t x) { return x % 2 == 0; });
  EXPECT_EQ(evens.size(), N / 2);

  // count_if
  EXPECT_EQ(v.count_if([](size_t x) { return x % 2 == 0; }), N / 2);

  // All positive
  EXPECT_TRUE(v.all([](size_t x) { return x < N; }));

  // Min/Max
  EXPECT_EQ(*v.min(), 0);
  EXPECT_EQ(*v.max(), N - 1);

  // has_value
  EXPECT_TRUE(v.has_value(N / 2));
  EXPECT_FALSE(v.has_value(N));

  // first/last
  EXPECT_EQ(*v.first(), 0);
  EXPECT_EQ(*v.last(), N - 1);
}

TEST(StressTest, LargeEnumerate)
{
  constexpr size_t N = 1000;
  MixinVector<int> v;

  for (size_t i = 0; i < N; ++i)
    v.append(static_cast<int>(i * 2));

  auto enumerated = v.enumerate();
  EXPECT_EQ(enumerated.size(), N);

  // Verify all indices are correct
  size_t idx = 0;
  enumerated.for_each([&idx](const std::pair<size_t, int>& p) {
    EXPECT_EQ(p.first, idx);
    EXPECT_EQ(p.second, static_cast<int>(idx * 2));
    ++idx;
  });
}


// =============================================================================
// Constraint Tests - verify methods are properly constrained
// =============================================================================

// Type without arithmetic operators
struct NoOpType
{
  int value;
  // No operator+, operator*, operator<, operator==
};

TEST(ConstraintTest, MethodsWithoutConstraintsWork)
{
  MixinVector<NoOpType> v;
  v.append({1});
  v.append({2});
  v.append({3});

  // These should compile - they don't require operators
  EXPECT_EQ(v.length(), 3);
  EXPECT_NE(v.first(), nullptr);
  EXPECT_NE(v.last(), nullptr);
  EXPECT_EQ(v.first()->value, 1);
  EXPECT_EQ(v.last()->value, 3);

  auto enumerated = v.enumerate();
  EXPECT_EQ(enumerated.size(), 3);

  // count_if with custom predicate works
  EXPECT_EQ(v.count_if([](const NoOpType& x) { return x.value > 1; }), 2);

  // find_index with custom predicate works
  EXPECT_EQ(v.find_index([](const NoOpType& x) { return x.value == 2; }), 1);

  // The following would NOT compile (correctly!) due to requires constraints:
  // v.sum();        // requires operator+
  // v.product({0}); // requires operator*
  // v.min();        // requires operator<
  // v.max();        // requires operator<
  // v.has_value({1}); // requires operator==
  // v.index_of({1});  // requires operator==
}

// Type with only equality
struct EqOnlyType
{
  int value;
  bool operator==(const EqOnlyType& other) const { return value == other.value; }
};

TEST(ConstraintTest, EqualityOnlyType)
{
  MixinVector<EqOnlyType> v;
  v.append({1});
  v.append({2});
  v.append({3});

  // has_value and index_of work (require operator==)
  EXPECT_TRUE(v.has_value({2}));
  EXPECT_FALSE(v.has_value({99}));
  EXPECT_EQ(v.index_of({2}), 1);

  // The following would NOT compile:
  // v.sum();  // requires operator+
  // v.min();  // requires operator<
}

// Type with only less-than
struct LtOnlyType
{
  int value;
  bool operator<(const LtOnlyType& other) const { return value < other.value; }
};

TEST(ConstraintTest, LessThanOnlyType)
{
  MixinVector<LtOnlyType> v;
  v.append({3});
  v.append({1});
  v.append({2});

  // min and max work (require operator<)
  EXPECT_EQ(v.min()->value, 1);
  EXPECT_EQ(v.max()->value, 3);

  // The following would NOT compile:
  // v.sum();        // requires operator+
  // v.has_value({1}); // requires operator==
}


// =============================================================================
// Advanced Transformation Tests (unique, intersperse)
// =============================================================================

TEST(FunctionalMixin, Unique)
{
  MixinVector<int> v = {1, 1, 2, 2, 2, 3, 1, 1};

  auto u = v.unique();
  EXPECT_EQ(u.size(), 4);

  int expected[] = {1, 2, 3, 1};
  int i = 0;
  u.for_each([&](int x) { EXPECT_EQ(x, expected[i++]); });
}

TEST(FunctionalMixin, UniqueEmpty)
{
  MixinVector<int> v;
  auto u = v.unique();
  EXPECT_TRUE(u.is_empty());
}

TEST(FunctionalMixin, UniqueSingleElement)
{
  MixinVector<int> v = {42};
  auto u = v.unique();
  EXPECT_EQ(u.size(), 1);

  // Verify first element via iteration
  int first_val = 0;
  u.traverse([&first_val](int x) { first_val = x; return false; });
  EXPECT_EQ(first_val, 42);
}

TEST(FunctionalMixin, UniqueNoDuplicates)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};
  auto u = v.unique();
  EXPECT_EQ(u.size(), 5);
}

TEST(FunctionalMixin, UniqueBy)
{
  MixinVector<string> v = {"a", "A", "b", "B", "c"};

  auto u = v.unique_by([](const string& a, const string& b) {
    return tolower(a[0]) == tolower(b[0]);
  });

  EXPECT_EQ(u.size(), 3);  // "a", "b", "c"
}

TEST(FunctionalMixin, Intersperse)
{
  MixinVector<int> v = {1, 2, 3};

  auto i = v.intersperse(0);
  EXPECT_EQ(i.size(), 5);  // 1, 0, 2, 0, 3

  int expected[] = {1, 0, 2, 0, 3};
  int idx = 0;
  i.for_each([&](int x) { EXPECT_EQ(x, expected[idx++]); });
}

TEST(FunctionalMixin, IntersperseEmpty)
{
  MixinVector<int> v;
  auto i = v.intersperse(0);
  EXPECT_TRUE(i.is_empty());
}

TEST(FunctionalMixin, IntersperseSingleElement)
{
  MixinVector<int> v = {42};
  auto i = v.intersperse(0);
  EXPECT_EQ(i.size(), 1);

  // Verify first element via iteration
  int first_val = 0;
  i.traverse([&first_val](int x) { first_val = x; return false; });
  EXPECT_EQ(first_val, 42);
}

TEST(FunctionalMixin, IntersperseStrings)
{
  MixinVector<string> v = {"a", "b", "c"};
  auto i = v.intersperse(string("-"));
  EXPECT_EQ(i.size(), 5);
}


// =============================================================================
// Chunking and Windowing Tests
// =============================================================================

TEST(FunctionalMixin, Chunk)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto chunks = v.chunk(2);
  EXPECT_EQ(chunks.size(), 3);  // {1,2}, {3,4}, {5}

  // Verify first chunk size via iteration
  size_t first_chunk_size = 0;
  chunks.traverse([&first_chunk_size](const auto& c) {
    first_chunk_size = c.size();
    return false;
  });
  EXPECT_EQ(first_chunk_size, 2);
}

TEST(FunctionalMixin, ChunkExactDivision)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  auto chunks = v.chunk(2);
  EXPECT_EQ(chunks.size(), 3);  // {1,2}, {3,4}, {5,6}
}

TEST(FunctionalMixin, ChunkEmpty)
{
  MixinVector<int> v;
  auto chunks = v.chunk(2);
  EXPECT_TRUE(chunks.is_empty());
}

TEST(FunctionalMixin, ChunkSizeZero)
{
  MixinVector<int> v = {1, 2, 3};
  auto chunks = v.chunk(0);
  EXPECT_TRUE(chunks.is_empty());
}

TEST(FunctionalMixin, ChunkLargerThanContainer)
{
  MixinVector<int> v = {1, 2};
  auto chunks = v.chunk(10);
  EXPECT_EQ(chunks.size(), 1);  // single chunk with all elements
}

TEST(FunctionalMixin, Sliding)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  auto windows = v.sliding(3);
  EXPECT_EQ(windows.size(), 3);  // {1,2,3}, {2,3,4}, {3,4,5}

  // Verify each window has 3 elements
  windows.for_each([](const auto& w) { EXPECT_EQ(w.size(), 3); });
}

TEST(FunctionalMixin, SlidingWithStep)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  auto windows = v.sliding(2, 2);
  EXPECT_EQ(windows.size(), 3);  // {1,2}, {3,4}, {5,6}
}

TEST(FunctionalMixin, SlidingEmpty)
{
  MixinVector<int> v;
  auto windows = v.sliding(2);
  EXPECT_TRUE(windows.is_empty());
}

TEST(FunctionalMixin, SlidingSizeZero)
{
  MixinVector<int> v = {1, 2, 3};
  auto windows = v.sliding(0);
  EXPECT_TRUE(windows.is_empty());
}

TEST(FunctionalMixin, SlidingLargerThanContainer)
{
  MixinVector<int> v = {1, 2};
  auto windows = v.sliding(3);
  EXPECT_TRUE(windows.is_empty());
}


// =============================================================================
// Conversion Tests (to_vector, join)
// =============================================================================

TEST(FunctionalMixin, ToVector)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  std::vector<int> vec = v.to_vector();
  EXPECT_EQ(vec.size(), 5);
  EXPECT_EQ(vec[0], 1);
  EXPECT_EQ(vec[4], 5);
}

TEST(FunctionalMixin, ToVectorEmpty)
{
  MixinVector<int> v;
  std::vector<int> vec = v.to_vector();
  EXPECT_TRUE(vec.empty());
}

TEST(FunctionalMixin, ToDynlist)
{
  MixinVector<int> v = {1, 2, 3, 4, 5};

  DynList<int> list = v.to_dynlist();
  EXPECT_EQ(list.size(), 5);
  
  // Verify order is preserved
  auto it = list.get_it();
  EXPECT_EQ(it.get_curr(), 1);
  it.next_ne();
  EXPECT_EQ(it.get_curr(), 2);
}

TEST(FunctionalMixin, ToDynlistEmpty)
{
  MixinVector<int> v;
  DynList<int> list = v.to_dynlist();
  EXPECT_TRUE(list.is_empty());
}

TEST(FunctionalMixin, ToDynlistRoundTrip)
{
  // to_vector -> to_dynlist -> verify same content
  MixinVector<int> v = {10, 20, 30};
  
  std::vector<int> vec = v.to_vector();
  DynList<int> list = v.to_dynlist();
  
  // Both should have same size and content
  EXPECT_EQ(vec.size(), list.size());
  
  size_t i = 0;
  list.for_each([&vec, &i](int x) {
    EXPECT_EQ(x, vec[i++]);
  });
}

TEST(FunctionalMixin, Join)
{
  MixinVector<int> v = {1, 2, 3};

  string s = v.join(string(", "));
  EXPECT_EQ(s, "1, 2, 3");
}

TEST(FunctionalMixin, JoinSingleElement)
{
  MixinVector<int> v = {42};
  string s = v.join(string("-"));
  EXPECT_EQ(s, "42");
}

TEST(FunctionalMixin, JoinEmpty)
{
  MixinVector<int> v;
  string s = v.join(string(", "));
  EXPECT_EQ(s, "");
}

TEST(FunctionalMixin, JoinStr)
{
  MixinVector<string> v = {"a", "b", "c"};

  string s = v.join_str("-");
  EXPECT_EQ(s, "a-b-c");
}

TEST(FunctionalMixin, JoinStrEmpty)
{
  MixinVector<string> v;
  string s = v.join_str("-");
  EXPECT_EQ(s, "");
}


// =============================================================================
// Zip Tests
// =============================================================================

TEST(FunctionalMixin, ZipWith)
{
  MixinVector<int> a = {1, 2, 3};
  MixinVector<string> b;
  b.append("a");
  b.append("b");
  b.append("c");

  auto zipped = a.zip_with(b);
  EXPECT_EQ(zipped.size(), 3);

  // Verify pairs
  auto it = zipped.get_it();
  EXPECT_EQ(it.get_curr().first, 1);
  EXPECT_EQ(it.get_curr().second, "a");
}

TEST(FunctionalMixin, ZipWithDifferentLengths)
{
  MixinVector<int> a = {1, 2, 3, 4, 5};
  MixinVector<int> b = {10, 20};

  auto zipped = a.zip_with(b);
  EXPECT_EQ(zipped.size(), 2);  // stops at shorter
}

TEST(FunctionalMixin, ZipWithEmpty)
{
  MixinVector<int> a = {1, 2, 3};
  MixinVector<int> b;

  auto zipped = a.zip_with(b);
  EXPECT_TRUE(zipped.is_empty());
}

TEST(FunctionalMixin, ZipWithBothEmpty)
{
  MixinVector<int> a;
  MixinVector<int> b;

  auto zipped = a.zip_with(b);
  EXPECT_TRUE(zipped.is_empty());
}


// =============================================================================
// Combined Advanced Operations
// =============================================================================

TEST(ChainedOps, UniqueVerify)
{
  MixinVector<int> v = {1, 1, 2, 2, 3};

  auto unique_result = v.unique();
  EXPECT_EQ(unique_result.size(), 3);  // 1, 2, 3

  // Verify values via iteration
  int expected[] = {1, 2, 3};
  int idx = 0;
  unique_result.for_each([&](int x) { EXPECT_EQ(x, expected[idx++]); });
}

TEST(ChainedOps, ChunkAndSum)
{
  MixinVector<int> v = {1, 2, 3, 4, 5, 6};

  auto chunks = v.chunk(2);
  // Each chunk: {1,2}, {3,4}, {5,6} -> sums: 3, 7, 11

  int total = 0;
  chunks.for_each([&total](const Aleph::DynList<int>& chunk) {
    total += chunk.foldl<int>(0, [](int acc, int x) { return acc + x; });
  });
  EXPECT_EQ(total, 21);
}


// =============================================================================
// Tests with DynList (Real Aleph Container) - uses existing methods
// =============================================================================

TEST(DynListIntegration, ExistingMethodsWork)
{
  // DynList uses legacy ahDry.H macros, so only test existing methods
  Aleph::DynList<int> list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  // foldl exists in DynList
  int sum = list.foldl<int>(0, [](int acc, int x) { return acc + x; });
  EXPECT_EQ(sum, 15);

  // filter exists
  auto evens = list.filter([](int x) { return x % 2 == 0; });
  EXPECT_EQ(evens.size(), 2);

  // all exists
  EXPECT_TRUE(list.all([](int x) { return x <= 5; }));

  // exists works
  EXPECT_TRUE(list.exists([](int x) { return x == 3; }));
}

