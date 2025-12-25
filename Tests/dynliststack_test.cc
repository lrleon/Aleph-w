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

/**
 * @file dynliststack_test.cc
 * @brief Comprehensive test suite for DynListStack<T>
 *
 * This test file provides exhaustive coverage of DynListStack functionality:
 * - Basic operations (push, pop, top, size, is_empty)
 * - Constructors (default, copy, move, initializer_list)
 * - Assignment operators (copy, move)
 * - Memory management and exception safety
 * - Iterator functionality
 * - Functional methods (traverse, maps, filter, foldl, etc.)
 * - Edge cases and boundary conditions
 * - Performance characteristics
 */

# include <gtest/gtest.h>

# include <string>
# include <memory>
# include <vector>
# include <stdexcept>
# include <sstream>

# include <tpl_dynListStack.H>
# include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

constexpr size_t N = 17;
constexpr size_t LARGE_N = 10000;

/// Fixture with a stack of integers
struct SimpleStack : public Test
{
  size_t n = 0;
  DynListStack<int> s;

  SimpleStack()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push(i);
  }
};

/// Fixture with a stack of complex objects (DynList<int>)
struct ComplexStack : public Test
{
  size_t n = 0;
  DynListStack<DynList<int>> s;

  ComplexStack()
  {
    for (size_t i = 0; i < N; ++i, ++n)
      s.push({ int(i), 0, 1, 2, int(i) });
  }
};

/// Fixture with a stack of strings
struct StringStack : public Test
{
  DynListStack<string> s;

  StringStack()
  {
    s.push("first");
    s.push("second");
    s.push("third");
  }
};

/// Move-only type for testing move semantics
struct MoveOnly
{
  int value;
  bool moved_from = false;

  explicit MoveOnly(int v) : value(v) {}
  MoveOnly(const MoveOnly&) = delete;
  MoveOnly& operator=(const MoveOnly&) = delete;
  MoveOnly(MoveOnly&& other) noexcept : value(other.value)
  {
    other.moved_from = true;
  }
  MoveOnly& operator=(MoveOnly&& other) noexcept
  {
    value = other.value;
    other.moved_from = true;
    return *this;
  }

  bool operator==(const MoveOnly& other) const { return value == other.value; }
};

/// Type that counts constructions/destructions
struct Counted
{
  static int constructions;
  static int destructions;
  static int copies;
  static int moves;

  int value;

  static void reset()
  {
    constructions = destructions = copies = moves = 0;
  }

  explicit Counted(int v = 0) : value(v) { ++constructions; }
  Counted(const Counted& other) : value(other.value)
  {
    ++constructions;
    ++copies;
  }
  Counted(Counted&& other) noexcept : value(other.value)
  {
    ++constructions;
    ++moves;
  }
  ~Counted() { ++destructions; }

  Counted& operator=(const Counted& other)
  {
    value = other.value;
    ++copies;
    return *this;
  }
  Counted& operator=(Counted&& other) noexcept
  {
    value = other.value;
    ++moves;
    return *this;
  }

  bool operator==(const Counted& other) const { return value == other.value; }
};

int Counted::constructions = 0;
int Counted::destructions = 0;
int Counted::copies = 0;
int Counted::moves = 0;

// =============================================================================
// Basic Construction Tests
// =============================================================================

TEST(DynListStack, default_constructor_creates_empty_stack)
{
  DynListStack<int> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(DynListStack, default_constructor_with_various_types)
{
  DynListStack<int> si;
  DynListStack<double> sd;
  DynListStack<string> ss;
  DynListStack<vector<int>> sv;

  EXPECT_TRUE(si.is_empty());
  EXPECT_TRUE(sd.is_empty());
  EXPECT_TRUE(ss.is_empty());
  EXPECT_TRUE(sv.is_empty());
}

TEST(DynListStack, initializer_list_constructor)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), 5);

  // Elements should be in LIFO order - last pushed is first out
  // With initializer list {1,2,3,4,5}, append is called in order
  // So 1 is at bottom, 5 is at top
  EXPECT_EQ(s.pop(), 5);
  EXPECT_EQ(s.pop(), 4);
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStack, initializer_list_empty)
{
  DynListStack<int> s = {};
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(DynListStack, initializer_list_with_strings)
{
  DynListStack<string> s = {"hello", "world", "test"};

  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s.top(), "test");
}

// =============================================================================
// Copy Constructor Tests
// =============================================================================

TEST_F(SimpleStack, copy_constructor_creates_independent_copy)
{
  DynListStack<int> copy(s);

  EXPECT_EQ(copy.size(), s.size());
  EXPECT_EQ(copy.top(), s.top());

  // Modify original, copy should be unchanged
  (void) s.pop();
  EXPECT_NE(copy.size(), s.size());
  EXPECT_EQ(copy.size(), n);
}

TEST_F(SimpleStack, copy_constructor_preserves_order)
{
  DynListStack<int> copy(s);

  for (size_t i = 0; i < n; ++i)
    {
      ASSERT_EQ(copy.top(), s.top());
      (void) copy.pop();
      (void) s.pop();
    }
}

TEST(DynListStack, copy_constructor_empty_stack)
{
  DynListStack<int> empty;
  DynListStack<int> copy(empty);

  EXPECT_TRUE(copy.is_empty());
  EXPECT_EQ(copy.size(), 0);
}

TEST_F(ComplexStack, copy_constructor_deep_copies_elements)
{
  DynListStack<DynList<int>> copy(s);

  // Modify element in original
  s.top().append(999);

  // Copy should be unchanged
  EXPECT_FALSE(copy.top().exists([](int x) { return x == 999; }));
}

// =============================================================================
// Move Constructor Tests
// =============================================================================

TEST_F(SimpleStack, move_constructor_transfers_ownership)
{
  size_t original_size = s.size();
  int original_top = s.top();

  DynListStack<int> moved(std::move(s));

  EXPECT_EQ(moved.size(), original_size);
  EXPECT_EQ(moved.top(), original_top);
  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStack, move_constructor_empty_stack)
{
  DynListStack<int> empty;
  DynListStack<int> moved(std::move(empty));

  EXPECT_TRUE(moved.is_empty());
  EXPECT_TRUE(empty.is_empty());
}

TEST(DynListStack, move_constructor_with_move_only_type)
{
  DynListStack<unique_ptr<int>> s;
  s.push(make_unique<int>(42));
  s.push(make_unique<int>(43));

  DynListStack<unique_ptr<int>> moved(std::move(s));

  EXPECT_EQ(moved.size(), 2);
  EXPECT_EQ(*moved.top(), 43);
  EXPECT_TRUE(s.is_empty());
}

// =============================================================================
// Copy Assignment Tests
// =============================================================================

TEST_F(SimpleStack, copy_assignment_replaces_contents)
{
  DynListStack<int> other;
  other.push(100);
  other.push(200);

  other = s;

  EXPECT_EQ(other.size(), n);
  EXPECT_EQ(other.top(), s.top());
}

TEST_F(SimpleStack, copy_assignment_self_assignment_is_safe)
{
  s = s;

  EXPECT_EQ(s.size(), n);
  EXPECT_EQ(s.top(), int(n - 1));
}

TEST(DynListStack, copy_assignment_to_empty_stack)
{
  DynListStack<int> source = {1, 2, 3};
  DynListStack<int> dest;

  dest = source;

  EXPECT_EQ(dest.size(), 3);
  EXPECT_EQ(dest.top(), 3);
}

TEST(DynListStack, copy_assignment_from_empty_stack)
{
  DynListStack<int> source;
  DynListStack<int> dest = {1, 2, 3};

  dest = source;

  EXPECT_TRUE(dest.is_empty());
}

// =============================================================================
// Move Assignment Tests
// =============================================================================

TEST_F(SimpleStack, move_assignment_transfers_ownership)
{
  size_t original_size = s.size();
  DynListStack<int> dest;
  dest.push(999);

  dest = std::move(s);

  EXPECT_EQ(dest.size(), original_size);
  EXPECT_TRUE(s.is_empty());
}

TEST_F(SimpleStack, move_assignment_self_assignment_is_safe)
{
  size_t original_size = s.size();
  s = std::move(s);

  // Self-move should leave object in valid state
  // (actual behavior may vary, but should not crash)
  EXPECT_TRUE(s.size() == original_size || s.is_empty());
}

// =============================================================================
// Swap Tests
// =============================================================================

TEST_F(SimpleStack, swap_exchanges_contents)
{
  DynListStack<int> other = {100, 200, 300};

  size_t s_size = s.size();
  size_t other_size = other.size();
  int s_top = s.top();
  int other_top = other.top();

  s.swap(other);

  EXPECT_EQ(s.size(), other_size);
  EXPECT_EQ(other.size(), s_size);
  EXPECT_EQ(s.top(), other_top);
  EXPECT_EQ(other.top(), s_top);
}

TEST(DynListStack, swap_with_empty_stack)
{
  DynListStack<int> s = {1, 2, 3};
  DynListStack<int> empty;

  s.swap(empty);

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(empty.size(), 3);
}

TEST(DynListStack, swap_is_noexcept)
{
  DynListStack<int> s1, s2;
  EXPECT_TRUE(noexcept(s1.swap(s2)));
}

// =============================================================================
// Push Operation Tests
// =============================================================================

TEST(DynListStack, push_by_copy)
{
  DynListStack<string> s;
  string value = "test";

  s.push(value);

  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s.top(), "test");
  EXPECT_EQ(value, "test");  // Original unchanged
}

TEST(DynListStack, push_by_move)
{
  DynListStack<string> s;
  string value = "test";

  s.push(std::move(value));

  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s.top(), "test");
  EXPECT_TRUE(value.empty());  // Moved-from string is empty
}

TEST(DynListStack, push_returns_reference_to_inserted_element)
{
  DynListStack<int> s;

  int& ref = s.push(42);

  EXPECT_EQ(ref, 42);
  EXPECT_EQ(&ref, &s.top());

  ref = 100;
  EXPECT_EQ(s.top(), 100);
}

TEST(DynListStack, push_multiple_elements_maintains_lifo_order)
{
  DynListStack<int> s;

  for (int i = 0; i < 10; ++i)
    s.push(i);

  for (int i = 9; i >= 0; --i)
    ASSERT_EQ(s.pop(), i);
}

// =============================================================================
// Emplace Tests
// =============================================================================

TEST(DynListStack, emplace_constructs_in_place)
{
  DynListStack<pair<int, string>> s;

  s.emplace(42, "hello");

  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s.top().first, 42);
  EXPECT_EQ(s.top().second, "hello");
}

TEST(DynListStack, emplace_with_multiple_arguments)
{
  struct ThreeArgs
  {
    int a;
    double b;
    string c;
    ThreeArgs(int a_, double b_, string c_) : a(a_), b(b_), c(c_) {}
  };

  DynListStack<ThreeArgs> s;
  s.emplace(1, 2.5, "test");

  EXPECT_EQ(s.top().a, 1);
  EXPECT_DOUBLE_EQ(s.top().b, 2.5);
  EXPECT_EQ(s.top().c, "test");
}

TEST(DynListStack, emplace_returns_reference)
{
  DynListStack<int> s;

  int& ref = s.emplace(42);

  EXPECT_EQ(ref, 42);
  ref = 100;
  EXPECT_EQ(s.top(), 100);
}

// =============================================================================
// Pop Operation Tests
// =============================================================================

TEST_F(SimpleStack, pop_removes_and_returns_top)
{
  int top_value = s.top();

  int popped = s.pop();

  EXPECT_EQ(popped, top_value);
  EXPECT_EQ(s.size(), n - 1);
}

TEST(DynListStack, pop_on_empty_stack_throws)
{
  DynListStack<int> s;

  EXPECT_THROW(s.pop(), underflow_error);
}

TEST(DynListStack, pop_until_empty)
{
  DynListStack<int> s = {1, 2, 3};

  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
  EXPECT_TRUE(s.is_empty());
  EXPECT_THROW(s.pop(), underflow_error);
}

TEST(DynListStack, pop_with_move_only_type)
{
  DynListStack<unique_ptr<int>> s;
  s.push(make_unique<int>(42));

  unique_ptr<int> ptr = s.pop();

  EXPECT_EQ(*ptr, 42);
  EXPECT_TRUE(s.is_empty());
}

// =============================================================================
// Top/Peek Tests
// =============================================================================

TEST_F(SimpleStack, top_returns_reference_to_top_element)
{
  EXPECT_EQ(s.top(), int(n - 1));

  s.top() = 999;
  EXPECT_EQ(s.top(), 999);
}

TEST_F(SimpleStack, top_const_returns_const_reference)
{
  const DynListStack<int>& cs = s;

  EXPECT_EQ(cs.top(), int(n - 1));
}

TEST(DynListStack, top_on_empty_stack_throws)
{
  DynListStack<int> s;

  EXPECT_THROW(s.top(), underflow_error);
}

TEST(DynListStack, top_const_on_empty_stack_throws)
{
  const DynListStack<int> s;

  EXPECT_THROW(s.top(), underflow_error);
}

TEST_F(SimpleStack, peek_is_alias_for_top)
{
  EXPECT_EQ(s.peek(), s.top());
  EXPECT_EQ(&s.peek(), &s.top());
}

TEST_F(SimpleStack, peek_const_is_alias_for_top_const)
{
  const DynListStack<int>& cs = s;
  EXPECT_EQ(cs.peek(), cs.top());
}

// =============================================================================
// Get (alias for pop) Tests
// =============================================================================

TEST_F(SimpleStack, get_is_alias_for_pop)
{
  int pop_result = s.pop();
  int get_result = s.get();

  EXPECT_EQ(s.size(), n - 2);
  EXPECT_NE(pop_result, get_result);
}

TEST(DynListStack, get_on_empty_stack_throws)
{
  DynListStack<int> s;
  EXPECT_THROW(s.get(), underflow_error);
}

// =============================================================================
// Size and Empty Tests
// =============================================================================

TEST(DynListStack, size_is_zero_for_empty_stack)
{
  DynListStack<int> s;
  EXPECT_EQ(s.size(), 0);
}

TEST_F(SimpleStack, size_reflects_number_of_elements)
{
  EXPECT_EQ(s.size(), n);

  s.push(999);
  EXPECT_EQ(s.size(), n + 1);

  (void) s.pop();
  (void) s.pop();
  EXPECT_EQ(s.size(), n - 1);
}

TEST(DynListStack, is_empty_true_for_new_stack)
{
  DynListStack<int> s;
  EXPECT_TRUE(s.is_empty());
}

TEST_F(SimpleStack, is_empty_false_for_non_empty_stack)
{
  EXPECT_FALSE(s.is_empty());
}

TEST(DynListStack, is_empty_after_all_elements_removed)
{
  DynListStack<int> s = {1, 2, 3};

  (void) s.pop();
  (void) s.pop();
  (void) s.pop();

  EXPECT_TRUE(s.is_empty());
}

// =============================================================================
// Empty/Clear Tests
// =============================================================================

TEST_F(SimpleStack, empty_removes_all_elements)
{
  EXPECT_FALSE(s.is_empty());

  s.empty();

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(DynListStack, empty_on_empty_stack_is_safe)
{
  DynListStack<int> s;

  s.empty();  // Should not throw

  EXPECT_TRUE(s.is_empty());
}

TEST_F(SimpleStack, clear_is_alias_for_empty)
{
  s.clear();

  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0);
}

TEST(DynListStack, empty_is_noexcept)
{
  DynListStack<int> s;
  EXPECT_TRUE(noexcept(s.empty()));
}

TEST(DynListStack, clear_is_noexcept)
{
  DynListStack<int> s;
  EXPECT_TRUE(noexcept(s.clear()));
}

// =============================================================================
// Alias Methods Tests (put, insert, append)
// =============================================================================

TEST(DynListStack, put_is_alias_for_push)
{
  DynListStack<int> s;

  s.put(1);
  s.push(2);
  s.put(3);

  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s.pop(), 3);
  EXPECT_EQ(s.pop(), 2);
  EXPECT_EQ(s.pop(), 1);
}

TEST(DynListStack, insert_is_alias_for_push)
{
  DynListStack<int> s;

  s.insert(1);
  s.push(2);
  s.insert(3);

  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s.pop(), 3);
}

TEST(DynListStack, append_is_alias_for_push)
{
  DynListStack<int> s;

  s.append(1);
  s.push(2);
  s.append(3);

  EXPECT_EQ(s.size(), 3);
  EXPECT_EQ(s.pop(), 3);
}

// =============================================================================
// Search Tests
// =============================================================================

TEST_F(SimpleStack, search_finds_existing_element)
{
  int* ptr = s.search(5);

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 5);
}

TEST_F(SimpleStack, search_returns_nullptr_for_missing_element)
{
  int* ptr = s.search(999);

  EXPECT_EQ(ptr, nullptr);
}

TEST(DynListStack, search_on_empty_stack_returns_nullptr)
{
  DynListStack<int> s;

  EXPECT_EQ(s.search(1), nullptr);
}

TEST_F(SimpleStack, search_const_version)
{
  const DynListStack<int>& cs = s;

  const int* ptr = cs.search(5);

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, 5);
}

TEST_F(StringStack, search_with_string_type)
{
  string* ptr = s.search("second");

  ASSERT_NE(ptr, nullptr);
  EXPECT_EQ(*ptr, "second");

  EXPECT_EQ(s.search("missing"), nullptr);
}

// =============================================================================
// Contains/Has Tests
// =============================================================================

TEST_F(SimpleStack, contains_returns_true_for_existing_element)
{
  EXPECT_TRUE(s.contains(5));
  EXPECT_TRUE(s.contains(0));
  EXPECT_TRUE(s.contains(int(n - 1)));
}

TEST_F(SimpleStack, contains_returns_false_for_missing_element)
{
  EXPECT_FALSE(s.contains(-1));
  EXPECT_FALSE(s.contains(int(n)));
  EXPECT_FALSE(s.contains(999));
}

TEST(DynListStack, contains_on_empty_stack)
{
  DynListStack<int> s;
  EXPECT_FALSE(s.contains(1));
}

TEST_F(SimpleStack, has_is_alias_for_contains)
{
  EXPECT_EQ(s.has(5), s.contains(5));
  EXPECT_EQ(s.has(999), s.contains(999));
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST(DynListStack, iterator_on_empty_stack)
{
  DynListStack<int> s;
  auto it = s.get_it();

  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_THROW(it.next(), overflow_error);
}

TEST_F(SimpleStack, iterator_traverses_in_lifo_order)
{
  auto it = s.get_it();

  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), int(n - i - 1));
}

TEST_F(SimpleStack, iterator_count_matches_size)
{
  size_t count = 0;
  for (auto it = s.get_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, n);
}

TEST_F(SimpleStack, range_based_for_loop)
{
  vector<int> elements;

  for (const auto& item : s)
    elements.push_back(item);

  ASSERT_EQ(elements.size(), n);
  for (size_t i = 0; i < n; ++i)
    EXPECT_EQ(elements[i], int(n - i - 1));
}

TEST_F(ComplexStack, iterator_with_complex_type)
{
  auto it = s.get_it();

  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    {
      const auto& list = it.get_curr();
      ASSERT_EQ(list.get_first(), int(n - i - 1));
      ASSERT_EQ(list.get_last(), int(n - i - 1));
    }
}

// =============================================================================
// Traverse Tests
// =============================================================================

TEST_F(SimpleStack, traverse_visits_all_elements)
{
  size_t count = 0;
  bool result = s.traverse([&count](int) { ++count; return true; });

  EXPECT_TRUE(result);
  EXPECT_EQ(count, n);
}

TEST_F(SimpleStack, traverse_can_stop_early)
{
  size_t count = 0;
  bool result = s.traverse([&count](int)
  {
    ++count;
    return count < 5;
  });

  EXPECT_FALSE(result);
  EXPECT_EQ(count, 5);
}

TEST_F(SimpleStack, traverse_visits_in_lifo_order)
{
  size_t i = 0;
  s.traverse([&i, this](int value)
  {
    EXPECT_EQ(value, int(n - i - 1));
    ++i;
    return true;
  });
}

TEST(DynListStack, traverse_on_empty_stack)
{
  DynListStack<int> s;
  bool called = false;

  bool result = s.traverse([&called](int) { called = true; return true; });

  EXPECT_TRUE(result);
  EXPECT_FALSE(called);
}

TEST_F(SimpleStack, traverse_const_version)
{
  const DynListStack<int>& cs = s;
  size_t count = 0;

  cs.traverse([&count](const int&) { ++count; return true; });

  EXPECT_EQ(count, n);
}

// =============================================================================
// Functional Methods Tests (inherited from FunctionalMethods mixin)
// =============================================================================

TEST_F(SimpleStack, maps_transforms_elements)
{
  auto doubled = s.maps([](int x) { return x * 2; });

  EXPECT_EQ(doubled.size(), n);

  auto it = doubled.get_it();
  for (size_t i = 0; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), int((n - i - 1) * 2));
}

TEST_F(SimpleStack, maps_to_different_type)
{
  auto strings = s.template maps<string>([](int x)
  {
    return to_string(x);
  });

  EXPECT_EQ(strings.size(), n);
  EXPECT_EQ(strings.get_first(), to_string(int(n - 1)));
}

TEST_F(SimpleStack, filter_selects_matching_elements)
{
  auto evens = s.filter([](int x) { return x % 2 == 0; });

  for (auto it = evens.get_it(); it.has_curr(); it.next())
    EXPECT_EQ(it.get_curr() % 2, 0);
}

TEST_F(SimpleStack, filter_returns_empty_when_none_match)
{
  auto result = s.filter([](int x) { return x > 1000; });

  EXPECT_TRUE(result.is_empty());
}

TEST_F(SimpleStack, foldl_accumulates_values)
{
  int sum = s.foldl(0, [](int acc, int x) { return acc + x; });

  int expected = (n - 1) * n / 2;  // Sum of 0 to n-1
  EXPECT_EQ(sum, expected);
}

TEST_F(SimpleStack, foldl_with_different_accumulator_type)
{
  string result = s.template foldl<string>("", [](const string& acc, int x)
  {
    return acc + to_string(x) + ",";
  });

  EXPECT_FALSE(result.empty());
}

TEST_F(SimpleStack, all_returns_true_when_all_match)
{
  bool result = s.all([](int x) { return x >= 0; });
  EXPECT_TRUE(result);
}

TEST_F(SimpleStack, all_returns_false_when_any_fails)
{
  bool result = s.all([](int x) { return x < 10; });
  EXPECT_FALSE(result);  // n > 10, so some elements are >= 10
}

TEST_F(SimpleStack, exists_returns_true_when_any_matches)
{
  bool result = s.exists([](int x) { return x == 5; });
  EXPECT_TRUE(result);
}

TEST_F(SimpleStack, exists_returns_false_when_none_match)
{
  bool result = s.exists([](int x) { return x == 999; });
  EXPECT_FALSE(result);
}

TEST_F(SimpleStack, for_each_visits_all_elements)
{
  int sum = 0;
  s.for_each([&sum](int x) { sum += x; });

  int expected = (n - 1) * n / 2;
  EXPECT_EQ(sum, expected);
}

// =============================================================================
// Type Aliases Tests
// =============================================================================

TEST(DynListStack, type_aliases_are_correct)
{
  using Stack = DynListStack<int>;

  static_assert(is_same_v<Stack::Item_Type, int>);
  static_assert(is_same_v<Stack::Key_Type, int>);
  static_assert(is_same_v<Stack::Set_Type, Stack>);
}

// =============================================================================
// Memory Management Tests
// =============================================================================

TEST(DynListStack, destructor_frees_all_memory)
{
  Counted::reset();

  {
    DynListStack<Counted> s;
    for (int i = 0; i < 100; ++i)
      s.emplace(i);
  }

  // All elements should be destroyed
  EXPECT_EQ(Counted::constructions, Counted::destructions);
}

TEST(DynListStack, clear_destroys_all_elements)
{
  Counted::reset();

  DynListStack<Counted> s;
  for (int i = 0; i < 50; ++i)
    s.emplace(i);

  int constructions_before = Counted::constructions;
  s.clear();

  EXPECT_EQ(Counted::destructions, constructions_before);
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST(DynListStack, push_move_only_type)
{
  DynListStack<unique_ptr<int>> s;

  s.push(make_unique<int>(42));
  s.push(make_unique<int>(43));

  EXPECT_EQ(s.size(), 2);
  EXPECT_EQ(*s.top(), 43);

  auto ptr = s.pop();
  EXPECT_EQ(*ptr, 43);
}

TEST(DynListStack, move_semantics_avoid_copies)
{
  Counted::reset();

  DynListStack<Counted> s;
  Counted c(42);

  s.push(std::move(c));

  // Should use move, not copy
  EXPECT_GE(Counted::moves, 1);
}

// =============================================================================
// Edge Cases and Boundary Tests
// =============================================================================

TEST(DynListStack, single_element_stack)
{
  DynListStack<int> s;
  s.push(42);

  EXPECT_EQ(s.size(), 1);
  EXPECT_EQ(s.top(), 42);
  EXPECT_FALSE(s.is_empty());

  EXPECT_EQ(s.pop(), 42);
  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStack, repeated_push_pop_cycles)
{
  DynListStack<int> s;

  for (int cycle = 0; cycle < 10; ++cycle)
    {
      for (int i = 0; i < 100; ++i)
        s.push(i);

      EXPECT_EQ(s.size(), 100);

      for (int i = 99; i >= 0; --i)
        ASSERT_EQ(s.pop(), i);

      EXPECT_TRUE(s.is_empty());
    }
}

TEST(DynListStack, interleaved_push_pop)
{
  DynListStack<int> s;

  s.push(1);
  s.push(2);
  EXPECT_EQ(s.pop(), 2);
  s.push(3);
  s.push(4);
  EXPECT_EQ(s.pop(), 4);
  EXPECT_EQ(s.pop(), 3);
  s.push(5);
  EXPECT_EQ(s.pop(), 5);
  EXPECT_EQ(s.pop(), 1);
  EXPECT_TRUE(s.is_empty());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynListStack, stress_large_number_of_elements)
{
  DynListStack<int> s;

  for (size_t i = 0; i < LARGE_N; ++i)
    s.push(i);

  EXPECT_EQ(s.size(), LARGE_N);

  for (size_t i = 0; i < LARGE_N; ++i)
    ASSERT_EQ(s.pop(), int(LARGE_N - i - 1));

  EXPECT_TRUE(s.is_empty());
}

TEST(DynListStack, stress_with_strings)
{
  DynListStack<string> s;

  for (size_t i = 0; i < 1000; ++i)
    s.push("string_" + to_string(i));

  EXPECT_EQ(s.size(), 1000);

  for (size_t i = 999; i != size_t(-1); --i)
    ASSERT_EQ(s.pop(), "string_" + to_string(i));
}

TEST(DynListStack, stress_copy_large_stack)
{
  DynListStack<int> original;

  for (size_t i = 0; i < LARGE_N; ++i)
    original.push(i);

  DynListStack<int> copy(original);

  EXPECT_EQ(copy.size(), LARGE_N);
  EXPECT_EQ(original.size(), LARGE_N);

  // Verify contents match
  auto it1 = original.get_it();
  auto it2 = copy.get_it();
  while (it1.has_curr() && it2.has_curr())
    {
      ASSERT_EQ(it1.get_curr(), it2.get_curr());
      it1.next();
      it2.next();
    }
}

// =============================================================================
// Exception Safety Tests
// =============================================================================

struct ThrowOnCopy
{
  int value;
  static bool should_throw;

  ThrowOnCopy(int v) : value(v) {}
  ThrowOnCopy(const ThrowOnCopy& other) : value(other.value)
  {
    if (should_throw)
      throw runtime_error("Copy failed");
  }
  ThrowOnCopy(ThrowOnCopy&&) = default;
  ThrowOnCopy& operator=(const ThrowOnCopy&) = default;
  ThrowOnCopy& operator=(ThrowOnCopy&&) = default;
};

bool ThrowOnCopy::should_throw = false;

TEST(DynListStack, exception_safety_on_push_copy)
{
  DynListStack<ThrowOnCopy> s;
  s.push(ThrowOnCopy(1));
  s.push(ThrowOnCopy(2));

  ThrowOnCopy::should_throw = true;
  ThrowOnCopy item(3);

  EXPECT_THROW(s.push(item), runtime_error);

  ThrowOnCopy::should_throw = false;

  // Stack should still be usable
  EXPECT_EQ(s.size(), 2);
  EXPECT_EQ(s.top().value, 2);
}

// =============================================================================
// Comparison with STL stack Tests
// =============================================================================

TEST(DynListStack, compatible_with_algorithm_patterns)
{
  DynListStack<int> s = {5, 3, 8, 1, 9, 2};

  // Find max using foldl
  int max_val = s.foldl(numeric_limits<int>::min(),
                        [](int acc, int x) { return std::max(acc, x); });

  EXPECT_EQ(max_val, 9);

  // Find min using foldl
  int min_val = s.foldl(numeric_limits<int>::max(),
                        [](int acc, int x) { return std::min(acc, x); });

  EXPECT_EQ(min_val, 1);
}

// =============================================================================
// Regression Tests
// =============================================================================

TEST(DynListStack, regression_size_after_move)
{
  DynListStack<int> s1 = {1, 2, 3};
  DynListStack<int> s2 = std::move(s1);

  EXPECT_EQ(s2.size(), 3);
  EXPECT_EQ(s1.size(), 0);
  EXPECT_TRUE(s1.is_empty());
}

TEST(DynListStack, regression_iterator_after_modification)
{
  DynListStack<int> s = {1, 2, 3, 4, 5};

  // Get count before modification
  size_t count_before = 0;
  for (auto it = s.get_it(); it.has_curr(); it.next())
    ++count_before;

  s.push(6);
  (void) s.pop();

  // Count should be same
  size_t count_after = 0;
  for (auto it = s.get_it(); it.has_curr(); it.next())
    ++count_after;

  EXPECT_EQ(count_before, count_after);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}