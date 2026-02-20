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
# include <ahFunctional.H>
# include <tpl_avl.H>
# include <tpl_splay_tree.H>
# include <tpl_odhash.H>
# include <htlist.H>
# include <tpl_dynDlist.H>
# include <tpl_array.H>
# include <tpl_dynArray.H>
# include <functional>

using namespace Aleph;

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

// ====================================================================
// AlephSequentialContainer: static_assert positive cases
// ====================================================================

static_assert(Aleph::AlephSequentialContainer<DynList<int>>);
static_assert(Aleph::AlephSequentialContainer<DynDlist<int>>);
static_assert(Aleph::AlephSequentialContainer<Array<int>>);
static_assert(Aleph::AlephSequentialContainer<DynArray<int>>);

static_assert(Aleph::AlephSequentialContainer<DynList<double>>);
static_assert(Aleph::AlephSequentialContainer<Array<std::string>>);

// Negative: plain types and non-Aleph types are not sequential containers
static_assert(not Aleph::AlephSequentialContainer<int>);
static_assert(not Aleph::AlephSequentialContainer<std::vector<int>>);
static_assert(not Aleph::AlephSequentialContainer<double>);

// ====================================================================
// fill(): set all existing elements to a constant
// ====================================================================

TEST(AlephFillTest, FillDynList)
{
  DynList<int> lst = {1, 2, 3, 4, 5};
  fill(lst, 0);

  lst.for_each([](int v) { EXPECT_EQ(v, 0); });
  EXPECT_EQ(lst.size(), 5u);
}

TEST(AlephFillTest, FillArray)
{
  Array<int> arr;
  for (int i = 0; i < 5; ++i) arr.append(i);
  fill(arr, 7);

  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr[i], 7);
  EXPECT_EQ(arr.size(), 5u);
}

TEST(AlephFillTest, FillDynDlist)
{
  DynDlist<double> lst;
  for (int i = 0; i < 4; ++i) lst.append(static_cast<double>(i));
  fill(lst, 3.14);

  lst.for_each([](double v) { EXPECT_DOUBLE_EQ(v, 3.14); });
}

TEST(AlephFillTest, FillDynArray)
{
  DynArray<int> arr;
  for (int i = 0; i < 6; ++i) arr.append(i);
  fill(arr, -1);

  arr.for_each([](int v) { EXPECT_EQ(v, -1); });
  EXPECT_EQ(arr.size(), 6u);
}

TEST(AlephFillTest, FillEmptyContainerIsNoOp)
{
  DynList<int> lst;
  EXPECT_NO_THROW(fill(lst, 42));
  EXPECT_TRUE(lst.is_empty());
}

TEST(AlephFillTest, FillDoesNotChangeSize)
{
  auto lst = rep(8, 0);
  fill(lst, 99);
  EXPECT_EQ(lst.size(), 8u);
}

// ====================================================================
// iota(): fill with unit-step sequential values
// ====================================================================

TEST(AlephIotaTest, IotaDynList)
{
  auto lst = rep(5, 0);
  iota(lst, 1);

  int expected = 1;
  lst.for_each([&expected](int v) { EXPECT_EQ(v, expected++); });
}

TEST(AlephIotaTest, IotaArray)
{
  Array<int> arr;
  for (int i = 0; i < 5; ++i) arr.append(0);
  iota(arr, 10);

  for (size_t i = 0; i < arr.size(); ++i)
    EXPECT_EQ(arr[i], static_cast<int>(10 + i));
}

TEST(AlephIotaTest, IotaDynArray)
{
  DynArray<int> arr;
  for (int i = 0; i < 4; ++i) arr.append(0);
  iota(arr, 0);

  int expected = 0;
  arr.for_each([&expected](int v) { EXPECT_EQ(v, expected++); });
}

TEST(AlephIotaTest, IotaDynDlist)
{
  DynDlist<int> lst;
  for (int i = 0; i < 3; ++i) lst.append(0);
  iota(lst, 5);

  int expected = 5;
  lst.for_each([&expected](int v) { EXPECT_EQ(v, expected++); });
}

TEST(AlephIotaTest, IotaEmptyIsNoOp)
{
  DynList<int> lst;
  EXPECT_NO_THROW(iota(lst, 0));
  EXPECT_TRUE(lst.is_empty());
}

// ====================================================================
// iota(container, start, step): custom step
// ====================================================================

TEST(AlephIotaStepTest, EvenNumbers)
{
  auto lst = rep(5, 0);
  iota(lst, 0, 2);  // {0, 2, 4, 6, 8}

  int expected = 0;
  lst.for_each([&expected](int v) {
    EXPECT_EQ(v, expected);
    expected += 2;
  });
}

TEST(AlephIotaStepTest, DoubleStep)
{
  Array<double> arr;
  for (int i = 0; i < 4; ++i) arr.append(0.0);
  iota(arr, 1.0, 0.5);  // {1.0, 1.5, 2.0, 2.5}

  double expected = 1.0;
  for (size_t i = 0; i < arr.size(); ++i)
    {
      EXPECT_DOUBLE_EQ(arr[i], expected);
      expected += 0.5;
    }
}

TEST(AlephIotaStepTest, NegativeStep)
{
  auto lst = rep(4, 0);
  iota(lst, 10, -3);  // {10, 7, 4, 1}

  int expected = 10;
  lst.for_each([&expected](int v) {
    EXPECT_EQ(v, expected);
    expected -= 3;
  });
}

TEST(AlephIotaStepTest, SingleElement)
{
  auto lst = rep(1, 0);
  iota(lst, 42, 100);
  EXPECT_EQ(lst.get_first(), 42);
}

// ====================================================================
// Interaction: fill then iota
// ====================================================================

TEST(AlephFillIotaTest, FillThenIota)
{
  auto lst = rep(5, 99);
  fill(lst, 0);
  iota(lst, 1);

  int expected = 1;
  lst.for_each([&expected](int v) { EXPECT_EQ(v, expected++); });
}
