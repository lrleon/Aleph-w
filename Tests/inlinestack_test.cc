
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
 * @file inlinestack_test.cc
 * @brief Tests for InlineStack: a stack of fixed capacity whose items live in
 *        the object, so it never allocates.
 */

#include <gtest/gtest.h>

#include <tpl_arrayStack.H>

#include <cstddef>
#include <type_traits>
#include <utility>

using namespace Aleph;

TEST(InlineStack, never_allocates_and_cannot_throw)
{
  using S = InlineStack<int *, 8>;
  static_assert(S::capacity() == 8);
  static_assert(sizeof(S) >= 8 * sizeof(int *));  // the storage is inside
  static_assert(std::is_nothrow_default_constructible_v<S>);
  static_assert(std::is_nothrow_copy_constructible_v<S>);
  static_assert(std::is_nothrow_copy_assignable_v<S>);
  static_assert(std::is_nothrow_move_constructible_v<S>);
  static_assert(std::is_nothrow_move_assignable_v<S>);
  static_assert(std::is_nothrow_swappable_v<S>);
}

TEST(InlineStack, push_pop_top_and_size)
{
  int a = 1, b = 2, c = 3;
  InlineStack<int *, 4> s;
  EXPECT_TRUE(s.is_empty());
  EXPECT_EQ(s.size(), 0u);

  EXPECT_EQ(s.push(&a), &a);
  s.push(&b);
  s.push(&c);
  EXPECT_FALSE(s.is_empty());
  EXPECT_EQ(s.size(), 3u);
  EXPECT_EQ(s.top(), &c);
  EXPECT_EQ(s.top(0), &c);
  EXPECT_EQ(s.top(1), &b);
  EXPECT_EQ(s.top(2), &a);
  EXPECT_EQ(s.base(), &a);

  const InlineStack<int *, 4> & cs = s;
  EXPECT_EQ(cs.top(), &c);
  EXPECT_EQ(cs.top(1), &b);
  EXPECT_EQ(cs.base(), &a);

  EXPECT_EQ(s.pop(), &c);
  EXPECT_EQ(s.top(), &b);
  s.top() = &c;  // the top is a modifiable reference
  EXPECT_EQ(s.pop(), &c);
  EXPECT_EQ(s.pop(), &a);
  EXPECT_TRUE(s.is_empty());
}

TEST(InlineStack, popn_returns_the_last_popped_item)
{
  InlineStack<int, 8> s;
  for (int i = 1; i <= 5; ++i)
    s.push(i);
  EXPECT_EQ(s.popn(3), 3);  // 5, 4 and 3 are popped
  EXPECT_EQ(s.size(), 2u);
  EXPECT_EQ(s.top(), 2);
  EXPECT_EQ(s.popn(2), 1);
  EXPECT_TRUE(s.is_empty());
}

TEST(InlineStack, fills_up_to_its_capacity)
{
  InlineStack<int, 3> s;
  s.push(1);
  s.push(2);
  s.push(3);
  EXPECT_EQ(s.size(), s.capacity());
  EXPECT_EQ(s.top(), 3);
  s.empty();
  EXPECT_TRUE(s.is_empty());
  s.push(4);
  EXPECT_EQ(s.top(), 4);
  s.clear();
  EXPECT_TRUE(s.is_empty());
}

TEST(InlineStack, copies_only_the_items_in_use)
{
  InlineStack<int, 4> a;
  a.push(1);
  a.push(2);

  InlineStack<int, 4> b(a);
  EXPECT_EQ(b.size(), 2u);
  EXPECT_EQ(b.pop(), 2);
  EXPECT_EQ(a.size(), 2u);  // b is independent of a
  EXPECT_EQ(a.top(), 2);

  InlineStack<int, 4> c;
  c.push(9);
  c = a;
  EXPECT_EQ(c.size(), 2u);
  EXPECT_EQ(c.top(1), 1);
  c = c;  // self-assignment is harmless
  EXPECT_EQ(c.size(), 2u);
}

TEST(InlineStack, swap_exchanges_the_items)
{
  InlineStack<int, 4> a, b;
  a.push(1);
  a.push(2);
  a.push(3);
  b.push(9);

  a.swap(b);
  EXPECT_EQ(a.size(), 1u);
  EXPECT_EQ(a.top(), 9);
  EXPECT_EQ(b.size(), 3u);
  EXPECT_EQ(b.top(), 3);
  EXPECT_EQ(b.base(), 1);

  b.swap(b);  // with itself: unchanged
  EXPECT_EQ(b.size(), 3u);
  EXPECT_EQ(b.top(), 3);

  InlineStack<int, 4> empty;
  b.swap(empty);
  EXPECT_TRUE(b.is_empty());
  EXPECT_EQ(empty.size(), 3u);
}
