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
 * @file ah_concepts_test.cc
 * @brief Unit tests for C++20 concepts: BinaryPredicate, StrictWeakOrder,
 *        EqualityComparator.
 */

# include <gtest/gtest.h>

# include <ah-concepts.H>
# include <ahFunction.H>
# include <tpl_avl.H>
# include <tpl_splay_tree.H>
# include <tpl_odhash.H>
# include <functional>

// ====================================================================
// static_assert positive: functors that MUST satisfy the concepts
// ====================================================================

// Aleph functors
static_assert(Aleph::BinaryPredicate<Aleph::less<int>, int>);
static_assert(Aleph::BinaryPredicate<Aleph::greater<int>, int>);
static_assert(Aleph::BinaryPredicate<Aleph::equal_to<int>, int>);

static_assert(Aleph::StrictWeakOrder<Aleph::less<int>, int>);
static_assert(Aleph::StrictWeakOrder<Aleph::greater<int>, int>);

static_assert(Aleph::EqualityComparator<Aleph::equal_to<int>, int>);

// std functors
static_assert(Aleph::StrictWeakOrder<std::less<int>, int>);
static_assert(Aleph::StrictWeakOrder<std::greater<int>, int>);
static_assert(Aleph::EqualityComparator<std::equal_to<int>, int>);

// Lambda (via decltype)
constexpr auto my_less = [](const int & a, const int & b) { return a < b; };
static_assert(Aleph::StrictWeakOrder<decltype(my_less), int>);

// Function pointer
using BoolBinFn = bool (*)(const int &, const int &);
static_assert(Aleph::BinaryPredicate<BoolBinFn, int>);

// ====================================================================
// static_assert negative: types that must NOT satisfy
// ====================================================================

struct VoidBinary {
  void operator()(int, int) const {}
};
static_assert(!Aleph::BinaryPredicate<VoidBinary, int>);

struct Unary {
  bool operator()(int) const { return true; }
};
static_assert(!Aleph::BinaryPredicate<Unary, int>);

struct NotCallable {};
static_assert(!Aleph::BinaryPredicate<NotCallable, int>);

// ====================================================================
// BSTPolicy: positive and negative
// ====================================================================

static_assert(Aleph::BSTPolicy<Avl_Tree<int>, int>);
static_assert(Aleph::BSTPolicy<Splay_Tree<int>, int>);
static_assert(!Aleph::BSTPolicy<NotCallable, int>);

// ====================================================================
// Compilation tests: constrained classes instantiate correctly
// ====================================================================

TEST(AhConceptsTest, AvlTreeInstantiates)
{
  Avl_Tree<int> tree;
  auto * node = new Avl_Tree<int>::Node(42);
  tree.insert(node);
  EXPECT_NE(tree.search(42), nullptr);
  delete tree.remove(42);
}

TEST(AhConceptsTest, ODhashTableInstantiates)
{
  ODhashTable<int> table;
  EXPECT_EQ(table.search(42), nullptr); // empty table search
  table.insert(42);
  EXPECT_NE(table.search(42), nullptr);
  EXPECT_NO_THROW(table.remove(42));
  EXPECT_EQ(table.search(42), nullptr);
}
