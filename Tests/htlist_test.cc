
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
 * @file htlist_test.cc
 * @brief Exhaustive tests for Slinknc, Snodenc, HTList, and DynList classes
 *
 * These tests cover all operations including edge cases and the bug fixes
 * made to the classes, particularly:
 * - Bug fix in insert(Slinknc*, HTList&) that was losing elements
 * - Bug fix in remove_ne(Equal) that had infinite loop potential
 */

#include <gtest/gtest.h>
#include <vector>
#include <string>
#include <algorithm>
#include <random>
#include <memory>
#include <htlist.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Slinknc Tests
// =============================================================================

class SlinkcTest : public Test
{
protected:
  Slinknc link;
};

TEST_F(SlinkcTest, DefaultConstructor)
{
  EXPECT_TRUE(link.is_empty());
  EXPECT_EQ(link.get_next(), nullptr);
}

TEST_F(SlinkcTest, CopyConstructorResetsLink)
{
  auto other = std::make_unique<Slinknc>();
  link.insert(other.get());
  EXPECT_FALSE(link.is_empty());

  Slinknc copy(link);
  EXPECT_TRUE(copy.is_empty());

  link.reset();
}

TEST_F(SlinkcTest, AssignmentResetsLink)
{
  auto other = std::make_unique<Slinknc>();
  Slinknc another;
  link.insert(other.get());

  another = link;
  EXPECT_TRUE(another.is_empty());

  link.reset();
}

TEST_F(SlinkcTest, InsertAndRemove)
{
  Slinknc n1, n2, n3;

  link.insert(&n1);
  EXPECT_FALSE(link.is_empty());
  EXPECT_EQ(link.get_next(), &n1);

  link.insert(&n2);
  EXPECT_EQ(link.get_next(), &n2);
  EXPECT_EQ(n2.get_next(), &n1);

  Slinknc *removed = link.remove_next();
  EXPECT_EQ(removed, &n2);
  EXPECT_TRUE(n2.is_empty());
  EXPECT_EQ(link.get_next(), &n1);

  link.reset();
}

TEST_F(SlinkcTest, Reset)
{
  Slinknc other;
  link.insert(&other);
  EXPECT_FALSE(link.is_empty());

  link.reset();
  EXPECT_TRUE(link.is_empty());
}

// =============================================================================
// Snodenc Tests
// =============================================================================

TEST(SnodencTest, DefaultConstructor)
{
  Snodenc<int> node;
  EXPECT_TRUE(node.is_empty());
}

TEST(SnodencTest, CopyConstructor)
{
  Snodenc<int> node(42);
  EXPECT_EQ(node.get_data(), 42);
}

TEST(SnodencTest, MoveConstructor)
{
  string str = "Hello World";
  Snodenc<string> node(std::move(str));
  EXPECT_EQ(node.get_data(), "Hello World");
}

TEST(SnodencTest, GetDataModification)
{
  Snodenc<int> node(10);
  node.get_data() = 20;
  EXPECT_EQ(node.get_data(), 20);
}

TEST(SnodencTest, ChainOfNodes)
{
  Snodenc<int> n1(1), n2(2), n3(3);

  n1.insert(&n2);
  n2.insert(&n3);

  EXPECT_EQ(n1.get_next()->get_data(), 2);
  EXPECT_EQ(n1.get_next()->get_next()->get_data(), 3);
}

TEST(SnodencTest, ToSnodencConversion)
{
  Snodenc<int> node(42);
  Slinknc *link = &node;

  Snodenc<int> *converted = link->to_snodenc<int>();
  EXPECT_EQ(converted, &node);
  EXPECT_EQ(converted->get_data(), 42);
}

TEST(SnodencTest, ToDataConversion)
{
  Snodenc<int> node(42);
  Slinknc *link = &node;

  EXPECT_EQ(link->to_data<int>(), 42);

  link->to_data<int>() = 100;
  EXPECT_EQ(node.get_data(), 100);
}

// =============================================================================
// HTList Basic Tests
// =============================================================================

class HTListBasicTest : public Test
{
protected:
  HTList list;
};

TEST_F(HTListBasicTest, DefaultConstructor)
{
  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());
  EXPECT_EQ(list.get_head(), nullptr);
  EXPECT_EQ(list.get_tail(), nullptr);
}

TEST_F(HTListBasicTest, InsertSingleElement)
{
  auto *node = new Snodenc<int>(1);
  list.insert(node);

  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first(), node);
  EXPECT_EQ(list.get_last(), node);

  delete list.remove_first();
}

TEST_F(HTListBasicTest, AppendSingleElement)
{
  auto *node = new Snodenc<int>(1);
  list.append(node);

  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first(), node);
  EXPECT_EQ(list.get_last(), node);

  delete list.remove_first();
}

TEST_F(HTListBasicTest, InsertMaintainsOrder)
{
  // Insert 3, 2, 1 -> list should be 1, 2, 3
  list.insert(new Snodenc<int>(3));
  list.insert(new Snodenc<int>(2));
  list.insert(new Snodenc<int>(1));

  EXPECT_EQ(list.get_first()->to_data<int>(), 1);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);
  EXPECT_EQ(list.size(), 3);

  list.remove_all_and_delete();
}

TEST_F(HTListBasicTest, AppendMaintainsOrder)
{
  // Append 1, 2, 3 -> list should be 1, 2, 3
  list.append(new Snodenc<int>(1));
  list.append(new Snodenc<int>(2));
  list.append(new Snodenc<int>(3));

  EXPECT_EQ(list.get_first()->to_data<int>(), 1);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);
  EXPECT_EQ(list.size(), 3);

  list.remove_all_and_delete();
}

// =============================================================================
// HTList Insert with Link Bug Fix Test
// =============================================================================

class HTListInsertBugFixTest : public Test
{
protected:
  HTList list;

  void SetUp() override
  {
    // Create list: 1, 2, 3, 4, 5
    for (int i = 1; i <= 5; ++i)
      list.append(new Snodenc<int>(i));
  }

  void TearDown() override
  {
    list.remove_all_and_delete();
  }
};

TEST_F(HTListInsertBugFixTest, InsertListAfterFirstElement)
{
  // This tests the bug fix: insert(link, list) should preserve elements after link
  // list: 1, 2, 3, 4, 5
  // Insert {10, 11, 12} after element 1
  // Expected: 1, 10, 11, 12, 2, 3, 4, 5

  HTList toInsert;
  toInsert.append(new Snodenc<int>(10));
  toInsert.append(new Snodenc<int>(11));
  toInsert.append(new Snodenc<int>(12));

  Slinknc *first = list.get_first();
  list.insert(first, toInsert);

  EXPECT_TRUE(toInsert.is_empty());
  EXPECT_EQ(list.size(), 8);

  // Verify order: 1, 10, 11, 12, 2, 3, 4, 5
  vector<int> expected = {1, 10, 11, 12, 2, 3, 4, 5};
  HTList::Iterator it(list);
  for (int val : expected)
  {
    ASSERT_TRUE(it.has_curr()) << "List ended prematurely";
    EXPECT_EQ(it.get_curr()->to_data<int>(), val);
    it.next();
  }
  EXPECT_FALSE(it.has_curr());
}

TEST_F(HTListInsertBugFixTest, InsertListInMiddle)
{
  // list: 1, 2, 3, 4, 5
  // Insert {20, 21} after element 3
  // Expected: 1, 2, 3, 20, 21, 4, 5

  HTList toInsert;
  toInsert.append(new Snodenc<int>(20));
  toInsert.append(new Snodenc<int>(21));

  // Find element 3
  HTList::Iterator it(list);
  while (it.get_curr()->to_data<int>() != 3)
    it.next();

  Slinknc *link = it.get_curr();
  list.insert(link, toInsert);

  EXPECT_TRUE(toInsert.is_empty());
  EXPECT_EQ(list.size(), 7);

  // Verify order
  vector<int> expected = {1, 2, 3, 20, 21, 4, 5};
  it.reset();
  for (int val : expected)
  {
    ASSERT_TRUE(it.has_curr());
    EXPECT_EQ(it.get_curr()->to_data<int>(), val);
    it.next();
  }
}

TEST_F(HTListInsertBugFixTest, InsertListAtEnd)
{
  // list: 1, 2, 3, 4, 5
  // Insert {30, 31} after element 5 (tail)
  // Expected: 1, 2, 3, 4, 5, 30, 31

  HTList toInsert;
  toInsert.append(new Snodenc<int>(30));
  toInsert.append(new Snodenc<int>(31));

  Slinknc *last = list.get_last();
  list.insert(last, toInsert);

  EXPECT_TRUE(toInsert.is_empty());
  EXPECT_EQ(list.size(), 7);
  EXPECT_EQ(list.get_last()->to_data<int>(), 31);

  // Verify order
  vector<int> expected = {1, 2, 3, 4, 5, 30, 31};
  HTList::Iterator it(list);
  for (int val : expected)
  {
    ASSERT_TRUE(it.has_curr());
    EXPECT_EQ(it.get_curr()->to_data<int>(), val);
    it.next();
  }
}

TEST_F(HTListInsertBugFixTest, InsertEmptyList)
{
  HTList emptyList;

  Slinknc *first = list.get_first();
  list.insert(first, emptyList);

  // List should be unchanged
  EXPECT_EQ(list.size(), 5);

  HTList::Iterator it(list);
  for (int i = 1; i <= 5; ++i, it.next())
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
}

// =============================================================================
// HTList Stack Operations Tests
// =============================================================================

TEST(HTListStack, PushPopSequence)
{
  HTList stack;
  Slinknc n1, n2, n3;

  stack.push(&n1);
  stack.push(&n2);
  stack.push(&n3);

  EXPECT_EQ(stack.top(), &n3);
  EXPECT_EQ(stack.pop(), &n3);
  EXPECT_EQ(stack.top(), &n2);
  EXPECT_EQ(stack.pop(), &n2);
  EXPECT_EQ(stack.top(), &n1);
  EXPECT_EQ(stack.pop(), &n1);
  EXPECT_TRUE(stack.is_empty());
}

TEST(HTListStack, EmptyStackThrows)
{
  HTList stack;
  EXPECT_THROW(stack.top(), std::underflow_error);
  EXPECT_THROW(stack.pop(), std::underflow_error);
}

// =============================================================================
// HTList Split and Concat Tests
// =============================================================================

class HTListSplitTest : public Test
{
protected:
  HTList list;

  void TearDown() override
  {
    list.remove_all_and_delete();
  }
};

TEST_F(HTListSplitTest, SplitEmptyList)
{
  HTList l, r;
  size_t count = list.split(l, r);

  EXPECT_EQ(count, 0);
  EXPECT_TRUE(l.is_empty());
  EXPECT_TRUE(r.is_empty());
}

TEST_F(HTListSplitTest, SplitUnitaryList)
{
  list.append(new Snodenc<int>(1));

  HTList l, r;
  size_t count = list.split(l, r);

  EXPECT_EQ(count, 1);
  EXPECT_TRUE(list.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_TRUE(r.is_empty());

  l.remove_all_and_delete();
}

TEST_F(HTListSplitTest, SplitEvenList)
{
  for (int i = 1; i <= 10; ++i)
    list.append(new Snodenc<int>(i));

  HTList l, r;
  const size_t count = list.split(l, r);

  // split() gave one less than the number of items
  EXPECT_EQ(count, 10u);
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size(), 5u);
  EXPECT_EQ(r.size(), 5u);

  // Verify that concatenating restores the list
  l.concat(r);
  EXPECT_TRUE(r.is_empty());
  EXPECT_EQ(l.size(), 10);

  l.remove_all_and_delete();
}

TEST_F(HTListSplitTest, SplitOddList)
{
  for (int i = 1; i <= 7; ++i)
    list.append(new Snodenc<int>(i));

  HTList l, r;
  const size_t count = list.split(l, r);

  EXPECT_EQ(count, 7u);
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size(), 4u); // the first takes the middle item
  EXPECT_EQ(r.size(), 3u);

  l.concat(r);
  l.remove_all_and_delete();
}

TEST_F(HTListSplitTest, SplitGivesTheNumberOfItemsAndKeepsTheirOrder)
{
  for (int n = 0; n <= 9; ++n)
    {
      SCOPED_TRACE(n);
      for (int i = 1; i <= n; ++i)
        list.append(new Snodenc<int>(i));

      HTList l, r;
      EXPECT_EQ(list.split(l, r), size_t(n));
      EXPECT_TRUE(list.is_empty());
      EXPECT_EQ(l.size(), size_t(n - n / 2));
      EXPECT_EQ(r.size(), size_t(n / 2));

      l.concat(r);
      int expected = 1;
      for (HTList::Iterator it(l); it.has_curr(); it.next_ne(), ++expected)
        EXPECT_EQ(static_cast<Snodenc<int> *>(it.get_curr())->get_data(), expected);
      EXPECT_EQ(expected, n + 1);
      l.remove_all_and_delete();
    }
}

// =============================================================================
// HTList Reverse Tests
// =============================================================================

TEST(HTListReverse, ReverseEmptyList)
{
  HTList list;
  size_t count = list.reverse();
  EXPECT_EQ(count, 0);
  EXPECT_TRUE(list.is_empty());
}

TEST(HTListReverse, ReverseUnitaryList)
{
  HTList list;
  list.append(new Snodenc<int>(1));

  size_t count = list.reverse();
  EXPECT_EQ(count, 1);
  EXPECT_EQ(list.get_first()->to_data<int>(), 1);

  list.remove_all_and_delete();
}

TEST(HTListReverse, ReverseMultipleElements)
{
  HTList list;
  for (int i = 1; i <= 5; ++i)
    list.append(new Snodenc<int>(i));

  list.reverse();

  int expected = 5;
  for (HTList::Iterator it(list); it.has_curr(); it.next(), --expected)
    EXPECT_EQ(it.get_curr()->to_data<int>(), expected);

  list.remove_all_and_delete();
}

// =============================================================================
// HTList Rotation Tests
// =============================================================================

TEST(HTListRotation, RotateEmptyListThrows)
{
  HTList list;
  EXPECT_THROW(list.rotate_left(1), std::domain_error);
  EXPECT_NO_THROW(list.rotate_left(0));
}

TEST(HTListRotation, RotateLeftOne)
{
  HTList list;
  for (int i = 1; i <= 5; ++i)
    list.append(new Snodenc<int>(i));

  list.rotate_left(1);

  // Expected: 2, 3, 4, 5, 1
  vector<int> expected = {2, 3, 4, 5, 1};
  HTList::Iterator it(list);
  for (int val : expected)
  {
    EXPECT_EQ(it.get_curr()->to_data<int>(), val);
    it.next();
  }

  list.remove_all_and_delete();
}

// =============================================================================
// DynList Tests
// =============================================================================

class DynListBasicTest : public Test
{
protected:
  DynList<int> list;
};

TEST_F(DynListBasicTest, DefaultConstructor)
{
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.size(), 0);
}

TEST_F(DynListBasicTest, InsertAndRemove)
{
  list.insert(1);
  list.insert(2);
  list.insert(3);

  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.get_first(), 3);
  EXPECT_EQ(list.get_last(), 1);

  EXPECT_EQ(list.remove_first(), 3);
  EXPECT_EQ(list.remove_first(), 2);
  EXPECT_EQ(list.remove_first(), 1);
  EXPECT_TRUE(list.is_empty());
}

TEST_F(DynListBasicTest, AppendAndRemove)
{
  list.append(1);
  list.append(2);
  list.append(3);

  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.get_first(), 1);
  EXPECT_EQ(list.get_last(), 3);
}

TEST_F(DynListBasicTest, PushAndPop)
{
  list.push(1);
  list.push(2);
  list.push(3);

  EXPECT_EQ(list.top(), 3);
  EXPECT_EQ(list.pop(), 3);
  EXPECT_EQ(list.pop(), 2);
  EXPECT_EQ(list.pop(), 1);
  EXPECT_TRUE(list.is_empty());
}

TEST_F(DynListBasicTest, GetByIndex)
{
  for (int i = 0; i < 5; ++i)
    list.append(i);

  for (int i = 0; i < 5; ++i)
    EXPECT_EQ(list[i], i);
}

TEST_F(DynListBasicTest, GetByIndexThrowsOnOverflow)
{
  list.append(1);
  list.append(2);

  EXPECT_NO_THROW(list[0]);
  EXPECT_NO_THROW(list[1]);
  EXPECT_THROW(list[2], std::overflow_error);
}

// =============================================================================
// DynList Remove with Predicate Bug Fix Test
// =============================================================================

class DynListRemovePredicateTest : public Test
{
protected:
  DynList<int> list;

  void SetUp() override
  {
    for (int i = 1; i <= 10; ++i)
      list.append(i);
  }
};

TEST_F(DynListRemovePredicateTest, RemoveExistingElement)
{
  // Remove element equal to 5
  int removed = list.remove([](const int &x)
                            { return x == 5; });

  EXPECT_EQ(removed, 5);
  EXPECT_EQ(list.size(), 9);

  // Verify 5 is not in the list
  for (auto it = list.get_it(); it.has_curr(); it.next())
    EXPECT_NE(it.get_curr(), 5);
}

TEST_F(DynListRemovePredicateTest, RemoveFirstElement)
{
  int removed = list.remove([](const int &x)
                            { return x == 1; });

  EXPECT_EQ(removed, 1);
  EXPECT_EQ(list.get_first(), 2);
}

TEST_F(DynListRemovePredicateTest, RemoveLastElement)
{
  int removed = list.remove([](const int &x)
                            { return x == 10; });

  EXPECT_EQ(removed, 10);
  EXPECT_EQ(list.get_last(), 9);
}

TEST_F(DynListRemovePredicateTest, RemoveNonExistentThrows)
{
  // This tests the bug fix: should throw, not infinite loop
  EXPECT_THROW(
      list.remove([](const int &x)
                  { return x == 100; }),
      std::domain_error);
}

TEST_F(DynListRemovePredicateTest, RemoveNeNonExistentReturnsDefault)
{
  // This tests the bug fix: remove_ne should return default value, not infinite loop
  int result = list.remove_ne([](const int &x)
                              { return x == 100; });

  // Should return default-constructed int (0), not hang
  EXPECT_EQ(result, 0);
}

TEST_F(DynListRemovePredicateTest, RemoveNeExistingElement)
{
  int removed = list.remove_ne([](const int &x)
                               { return x == 5; });

  EXPECT_EQ(removed, 5);
  EXPECT_EQ(list.size(), 9);
}

TEST_F(DynListRemovePredicateTest, RemoveAllMatchingElements)
{
  // Remove all even numbers one by one
  for (int i = 2; i <= 10; i += 2)
  {
    int target = i;
    list.remove([target](const int &x)
                { return x == target; });
  }

  EXPECT_EQ(list.size(), 5);

  // Verify only odd numbers remain
  for (auto it = list.get_it(); it.has_curr(); it.next())
    EXPECT_TRUE(it.get_curr() % 2 == 1);
}

// =============================================================================
// DynList Copy and Move Semantics Tests
// =============================================================================

TEST(DynListSemantics, CopyConstructor)
{
  DynList<int> original;
  for (int i = 1; i <= 5; ++i)
    original.append(i);

  DynList<int> copy(original);

  EXPECT_EQ(copy.size(), original.size());

  auto it1 = original.get_it();
  auto it2 = copy.get_it();
  while (it1.has_curr())
  {
    EXPECT_EQ(it1.get_curr(), it2.get_curr());
    it1.next();
    it2.next();
  }
}

TEST(DynListSemantics, MoveConstructor)
{
  DynList<int> original;
  for (int i = 1; i <= 5; ++i)
    original.append(i);

  DynList<int> moved(std::move(original));

  EXPECT_TRUE(original.is_empty());
  EXPECT_EQ(moved.size(), 5);
}

TEST(DynListSemantics, CopyAssignment)
{
  DynList<int> original;
  for (int i = 1; i <= 5; ++i)
    original.append(i);

  DynList<int> copy;
  copy.append(100); // Add something to ensure it gets cleared

  copy = original;

  EXPECT_EQ(copy.size(), original.size());
  EXPECT_EQ(copy.get_first(), 1);
}

TEST(DynListSemantics, MoveAssignment)
{
  DynList<int> original;
  for (int i = 1; i <= 5; ++i)
    original.append(i);

  DynList<int> moved;
  moved = std::move(original);

  EXPECT_TRUE(original.is_empty());
  EXPECT_EQ(moved.size(), 5);
}

TEST(DynListSemantics, SelfAssignment)
{
  DynList<int> list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  list = list;

  EXPECT_EQ(list.size(), 5);
}

// =============================================================================
// DynList Append and Insert Lists Tests
// =============================================================================

TEST(DynListConcatenation, AppendByMove)
{
  DynList<int> list1, list2;

  for (int i = 1; i <= 3; ++i)
    list1.append(i);
  for (int i = 4; i <= 6; ++i)
    list2.append(i);

  list1.append(std::move(list2));

  EXPECT_TRUE(list2.is_empty());
  EXPECT_EQ(list1.size(), 6);

  int expected = 1;
  for (auto it = list1.get_it(); it.has_curr(); it.next(), ++expected)
    EXPECT_EQ(it.get_curr(), expected);
}

TEST(DynListConcatenation, AppendByCopy)
{
  DynList<int> list1, list2;

  for (int i = 1; i <= 3; ++i)
    list1.append(i);
  for (int i = 4; i <= 6; ++i)
    list2.append(i);

  list1.append(list2);

  EXPECT_FALSE(list2.is_empty()); // list2 should not be modified
  EXPECT_EQ(list1.size(), 6);
}

TEST(DynListConcatenation, InsertByMove)
{
  DynList<int> list1, list2;

  for (int i = 4; i <= 6; ++i)
    list1.append(i);
  for (int i = 1; i <= 3; ++i)
    list2.append(i);

  list1.insert(std::move(list2));

  EXPECT_TRUE(list2.is_empty());
  EXPECT_EQ(list1.size(), 6);
  EXPECT_EQ(list1.get_first(), 1);
}

// =============================================================================
// DynList Reverse Tests
// =============================================================================

TEST(DynListReverse, ReverseMutating)
{
  DynList<int> list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  list.reverse();

  int expected = 5;
  for (auto it = list.get_it(); it.has_curr(); it.next(), --expected)
    EXPECT_EQ(it.get_curr(), expected);
}

TEST(DynListReverse, ReverseConst)
{
  DynList<int> list;
  for (int i = 1; i <= 5; ++i)
    list.append(i);

  const DynList<int> &constRef = list;
  DynList<int> reversed = constRef.reverse();

  // Original should be unchanged
  int expected = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++expected)
    EXPECT_EQ(it.get_curr(), expected);

  // Reversed should be in reverse order
  expected = 5;
  for (auto it = reversed.get_it(); it.has_curr(); it.next(), --expected)
    EXPECT_EQ(it.get_curr(), expected);
}

// =============================================================================
// DynList Iterator Tests
// =============================================================================

class DynListIteratorTest : public Test
{
protected:
  DynList<int> list;

  void SetUp() override
  {
    for (int i = 1; i <= 5; ++i)
      list.append(i);
  }
};

TEST_F(DynListIteratorTest, ForwardTraversal)
{
  int expected = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++expected)
    EXPECT_EQ(it.get_curr(), expected);
}

TEST_F(DynListIteratorTest, IteratorDelete)
{
  DynList<int>::Iterator it(list);

  // Delete first element
  int deleted = it.del();
  EXPECT_EQ(deleted, 1);
  EXPECT_EQ(list.size(), 4);
  EXPECT_EQ(list.get_first(), 2);
}

TEST_F(DynListIteratorTest, IteratorDeleteMiddle)
{
  DynList<int>::Iterator it(list);
  it.next();
  it.next(); // Now at element 3

  int deleted = it.del();
  EXPECT_EQ(deleted, 3);
  EXPECT_EQ(list.size(), 4);
}

TEST_F(DynListIteratorTest, EmptyListIterator)
{
  DynList<int> empty;
  DynList<int>::Iterator it(empty);

  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

// =============================================================================
// DynList with Complex Types Tests
// =============================================================================

TEST(DynListComplexTypes, StringList)
{
  DynList<string> list;

  list.append("Hello");
  list.append("World");
  list.append("!");

  EXPECT_EQ(list.get_first(), "Hello");
  EXPECT_EQ(list.get_last(), "!");
  EXPECT_EQ(list.size(), 3);
}

TEST(DynListComplexTypes, VectorList)
{
  DynList<vector<int>> list;

  list.append({1, 2, 3});
  list.append({4, 5, 6});

  EXPECT_EQ(list.get_first().size(), 3);
  EXPECT_EQ(list.get_first()[0], 1);
  EXPECT_EQ(list.get_last()[0], 4);
}

struct TestStruct
{
  int x;
  string name;

  bool operator==(const TestStruct &other) const
  {
    return x == other.x && name == other.name;
  }
};

TEST(DynListComplexTypes, StructList)
{
  DynList<TestStruct> list;

  list.append({1, "one"});
  list.append({2, "two"});

  EXPECT_EQ(list.get_first().x, 1);
  EXPECT_EQ(list.get_first().name, "one");
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynListStress, LargeList)
{
  constexpr size_t N = 100000;
  DynList<int> list;

  for (size_t i = 0; i < N; ++i)
    list.append(static_cast<int>(i));

  EXPECT_EQ(list.size(), N);
  EXPECT_EQ(list.get_first(), 0);
  EXPECT_EQ(list.get_last(), static_cast<int>(N - 1));

  // Reverse and verify
  list.reverse();
  EXPECT_EQ(list.get_first(), static_cast<int>(N - 1));
  EXPECT_EQ(list.get_last(), 0);
}

TEST(DynListStress, RandomOperations)
{
  constexpr size_t OPS = 10000;

  DynList<int> list;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> opDist(0, 3);
  std::uniform_int_distribution<int> valDist(0, 1000);

  for (size_t i = 0; i < OPS; ++i)
  {
    int op = opDist(rng);

    switch (op)
    {
    case 0: // Insert
      list.insert(valDist(rng));
      break;
    case 1: // Append
      list.append(valDist(rng));
      break;
    case 2: // Remove first
      if (!list.is_empty())
        list.remove_first();
      break;
    case 3: // Pop
      if (!list.is_empty())
        list.pop();
      break;
    }
  }

  // Just verify the list is in a consistent state
  size_t count = 0;
  for (auto it = list.get_it(); it.has_curr(); it.next())
    ++count;

  EXPECT_EQ(count, list.size());
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(DynListEdgeCases, EmptyListOperations)
{
  DynList<int> list;

  EXPECT_THROW(list.get_first(), std::underflow_error);
  EXPECT_THROW(list.get_last(), std::underflow_error);
  EXPECT_THROW(list.remove(), std::underflow_error);
  EXPECT_THROW(list.pop(), std::underflow_error);
  EXPECT_THROW(list.top(), std::underflow_error);
}

TEST(DynListEdgeCases, UnitaryListOperations)
{
  DynList<int> list;
  list.append(42);

  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first(), list.get_last());
  EXPECT_EQ(list.top(), 42);

  int removed = list.pop();
  EXPECT_EQ(removed, 42);
  EXPECT_TRUE(list.is_empty());
}

TEST(DynListEdgeCases, SplitAndMerge)
{
  DynList<int> list;
  for (int i = 1; i <= 10; ++i)
    list.append(i);

  DynList<int> l, r;
  list.split_list(l, r);

  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size() + r.size(), 10);

  // Merge back
  l.append(std::move(r));
  EXPECT_EQ(l.size(), 10);
}

TEST(HTListRemoveByPointer, HeadMiddleTailAndNotFound)
{
  HTList list;
  auto * n1 = new Snodenc<int>(1);
  auto * n2 = new Snodenc<int>(2);
  auto * n3 = new Snodenc<int>(3);

  list.append(n1);
  list.append(n2);
  list.append(n3);

  EXPECT_TRUE(list.remove(n1));
  delete n1;
  EXPECT_EQ(list.get_first()->to_data<int>(), 2);

  EXPECT_TRUE(list.remove(n2));
  delete n2;
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first()->to_data<int>(), 3);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);

  EXPECT_TRUE(list.remove(n3));
  delete n3;
  EXPECT_TRUE(list.is_empty());

  auto * stray = new Snodenc<int>(99);
  EXPECT_THROW(list.remove(stray), std::underflow_error);
  delete stray;
}

TEST(HTListIterator, ResetLastEndAndAssignmentCopiesPos)
{
  HTList list;
  list.append(new Snodenc<int>(1));
  list.append(new Snodenc<int>(2));
  list.append(new Snodenc<int>(3));

  HTList::Iterator it(list);
  it.reset_last();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr()->to_data<int>(), 3);
  EXPECT_TRUE(it.is_last());
  EXPECT_EQ(it.get_pos(), 2);

  it.end();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);

  HTList::Iterator a(list);
  a.next();
  ASSERT_TRUE(a.has_curr());
  EXPECT_EQ(a.get_curr()->to_data<int>(), 2);
  EXPECT_EQ(a.get_pos(), 1);

  HTList::Iterator b(list);
  b.reset_last();
  b = a;
  EXPECT_TRUE(b.has_curr());
  EXPECT_EQ(b.get_curr()->to_data<int>(), 2);
  EXPECT_EQ(b.get_pos(), 1);

  list.remove_all_and_delete();
}

TEST(HTListSynonyms, PutConcatListSplitListNeReverseListCutList)
{
  HTList list;
  list.put(new Snodenc<int>(1));
  list.put(new Snodenc<int>(2));
  list.put(new Snodenc<int>(3));
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.get_first()->to_data<int>(), 1);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);

  HTList l, r;
  list.split_list_ne(l, r);
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size() + r.size(), 3);

  l.concat_list(r);
  EXPECT_TRUE(r.is_empty());
  EXPECT_EQ(l.size(), 3);

  EXPECT_EQ(l.reverse_list(), 3u);
  EXPECT_EQ(l.get_first()->to_data<int>(), 3);
  EXPECT_EQ(l.get_last()->to_data<int>(), 1);

  HTList::Iterator it(l);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr()->to_data<int>(), 2);

  HTList cut;
  l.cut_list(it.get_curr(), cut);
  EXPECT_EQ(l.get_last()->to_data<int>(), 2);
  EXPECT_EQ(cut.get_first()->to_data<int>(), 1);

  l.remove_all_and_delete();
  cut.remove_all_and_delete();
}

TEST(SlinkncIterator, TraversalAndOverflow)
{
  Slinknc head;
  Slinknc n1;
  Slinknc n2;

  head.insert(&n1);
  head.insert(&n2);

  Slinknc::Iterator it(head);
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), &n2);
  it.next();
  ASSERT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), &n1);
  it.next();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

// =============================================================================
// DynList keeps the number of its items
// =============================================================================

namespace
{
  // The number of items of l, counted by walking its nodes
  template <typename T>
  size_t recount(const DynList<T> & l)
  {
    return static_cast<const HTList &>(l).HTList::size();
  }

  template <typename T>
  vector<T> items_of(const DynList<T> & l)
  {
    vector<T> out;
    l.for_each([&out] (const T & x) { out.push_back(x); });
    return out;
  }
}

TEST(DynListCount, EveryOperationKeepsTheCount)
{
  DynList<string> a = {"a", "b", "c"};
  EXPECT_EQ(a.size(), 3u);
  a.insert("x");
  a.append(string("y"));
  a.push("z");
  a.put("w");
  EXPECT_EQ(a.size(), 7u);
  (void) a.remove_first();
  (void) a.remove();
  (void) a.pop();
  (void) a.get();
  (void) a.remove_ne();
  EXPECT_EQ(a.size(), 2u);
  EXPECT_EQ(a.size(), recount(a));

  vector<string> v = {"1", "2", "3"};
  DynList<string> range(v.begin(), v.end());
  EXPECT_EQ(range.size(), 3u);
  DynList<string> from_list(range);
  EXPECT_EQ(from_list.size(), 3u);

  a.append(std::move(range));
  EXPECT_EQ(a.size(), 5u);
  EXPECT_EQ(range.size(), 0u);
  a.insert(std::move(from_list));
  EXPECT_EQ(a.size(), 8u);
  EXPECT_EQ(from_list.size(), 0u);
  const DynList<string> d = {"4", "5"};
  a.append(d);
  a.insert(d);
  EXPECT_EQ(a.size(), 12u);
  EXPECT_EQ(a.size(), recount(a));

  DynList<string> b = a;
  EXPECT_EQ(b.size(), 12u);
  DynList<string> c = std::move(b);
  EXPECT_EQ(c.size(), 12u);
  EXPECT_EQ(b.size(), 0u);
  b = c;
  EXPECT_EQ(b.size(), 12u);
  b = std::move(a);
  EXPECT_EQ(b.size(), 12u);
  b.swap(a);
  EXPECT_EQ(a.size(), 12u);
  EXPECT_EQ(b.size(), 12u);

  {
    DynList<string>::Iterator it(a);
    (void) it.del();
    it.next();
    delete static_cast<Snodenc<string> *>(it.del_ne());
  }
  EXPECT_EQ(a.size(), 10u);
  (void) a.remove([] (const string & s) { return s == "4"; });
  (void) a.remove_ne([] (const string & s) { return s == "5"; });
  EXPECT_EQ(a.size(), 8u);
  EXPECT_EQ(a.size(), recount(a));

  a.concat(b);
  EXPECT_EQ(a.size(), 20u);
  EXPECT_EQ(b.size(), 0u);
  c.concat_list(a);
  EXPECT_EQ(c.size(), 32u);

  DynList<string> rest;
  Slinknc * third = static_cast<HTList &>(c).get_first()->get_next()->get_next();
  c.cut(third, rest);
  EXPECT_EQ(c.size(), 3u);
  EXPECT_EQ(rest.size(), 29u);
  EXPECT_EQ(rest.size(), recount(rest));
  rest.cut_list(static_cast<HTList &>(rest).get_last(), a);
  EXPECT_EQ(rest.size(), 29u);
  EXPECT_EQ(a.size(), 0u);

  delete static_cast<Snodenc<string> *>(rest.remove_head());
  delete static_cast<Snodenc<string> *>(rest.remove_head_ne());
  EXPECT_EQ(rest.size(), 27u);
  EXPECT_EQ(rest.size(), recount(rest));

  rest.reverse();
  rest.rotate_left(5);
  EXPECT_EQ(rest.size(), 27u);

  vector<Slinknc *> nodes;
  for (HTList::Iterator it(rest); it.has_curr(); it.next_ne())
    nodes.push_back(it.get_curr_ne());
  rest.reset();
  EXPECT_EQ(rest.size(), 0u);
  EXPECT_TRUE(rest.is_empty());
  for (Slinknc * p : nodes)
    delete static_cast<Snodenc<string> *>(p);

  c.remove_all_and_delete();
  EXPECT_EQ(c.size(), 0u);
  DynList<int> e = {1, 2, 3};
  e.empty();
  EXPECT_EQ(e.size(), 0u);
  e = {4, 5};
  e.clear();
  EXPECT_EQ(e.size(), 0u);
}

TEST(DynListCount, SplitGivesTheFirstHalfToTheLeft)
{
  for (size_t n = 0; n <= 9; ++n)
    {
      DynList<int> list, l, r;
      for (size_t i = 0; i < n; ++i)
        list.append(int(i));

      size_t count = 0;
      if (n % 3 == 0)
        count = list.split(l, r);
      else if (n % 3 == 1)
        count = list.split_list(l, r);
      else
        count = list.split_list_ne(l, r);

      EXPECT_EQ(count, n);
      EXPECT_TRUE(list.is_empty());
      EXPECT_EQ(list.size(), 0u);
      ASSERT_EQ(l.size(), n - n / 2) << "n = " << n;
      ASSERT_EQ(r.size(), n / 2) << "n = " << n;
      EXPECT_EQ(recount(l), l.size());
      EXPECT_EQ(recount(r), r.size());

      vector<int> left, right;
      for (size_t i = 0; i < n; ++i)
        (i < n - n / 2 ? left : right).push_back(int(i));
      EXPECT_EQ(items_of(l), left) << "n = " << n;
      EXPECT_EQ(items_of(r), right) << "n = " << n;
    }
}

// Random sequences of the operations that change the lists, against a
// vector for each list
TEST(DynListCount, RandomSequencesKeepTheCount)
{
  mt19937 rng(20261103);
  DynList<int> lists[3];
  vector<int> model[3];
  int next_value = 0;

  auto pick = [&rng] (size_t n) { return uniform_int_distribution<size_t>(0, n - 1)(rng); };

  for (int step = 0; step < 3000; ++step)
    {
      const size_t i = pick(3), j = (i + 1 + pick(2)) % 3;
      DynList<int> & l = lists[i];
      vector<int> & m = model[i];
      switch (pick(13))
        {
        case 0:
          l.insert(next_value);
          m.insert(m.begin(), next_value++);
          break;
        case 1:
        case 2:
          l.append(next_value);
          m.push_back(next_value++);
          break;
        case 3:
          if (not m.empty())
            {
              EXPECT_EQ(l.remove_first(), m.front());
              m.erase(m.begin());
            }
          break;
        case 4: // delete through an iterator at a random position
          if (not m.empty())
            {
              const size_t k = pick(m.size());
              DynList<int>::Iterator it(l);
              for (size_t q = 0; q < k; ++q)
                it.next_ne();
              EXPECT_EQ(it.del(), m[k]);
              m.erase(m.begin() + long(k));
            }
          break;
        case 5: // remove by equality
          if (not m.empty())
            {
              const int x = m[pick(m.size())];
              EXPECT_EQ(l.remove([x] (int y) { return y == x; }), x);
              m.erase(find(m.begin(), m.end(), x));
            }
          break;
        case 6:
          l.append(std::move(lists[j]));
          m.insert(m.end(), model[j].begin(), model[j].end());
          model[j].clear();
          break;
        case 7:
          l.insert(std::move(lists[j]));
          m.insert(m.begin(), model[j].begin(), model[j].end());
          model[j].clear();
          break;
        case 8:
          l.swap(lists[j]);
          m.swap(model[j]);
          break;
        case 9:
          lists[j] = l;
          model[j] = m;
          break;
        case 10: // split and join back
          {
            DynList<int> a, b;
            l.split(a, b);
            EXPECT_EQ(a.size() + b.size(), m.size());
            l.append(std::move(a));
            l.append(std::move(b));
          }
          break;
        case 11: // cut after a random node and join back
          if (not m.empty())
            {
              const size_t k = pick(m.size());
              Slinknc * p = static_cast<HTList &>(l).get_first();
              for (size_t q = 0; q < k; ++q)
                p = p->get_next();
              DynList<int> tail;
              l.cut(p, tail);
              EXPECT_EQ(l.size(), k + 1);
              EXPECT_EQ(tail.size(), m.size() - k - 1);
              l.concat(tail);
            }
          break;
        case 12:
          l.reverse();
          std::reverse(m.begin(), m.end());
          break;
        }

      for (size_t q = 0; q < 3; ++q)
        {
          ASSERT_EQ(lists[q].size(), model[q].size()) << "step " << step;
          ASSERT_EQ(recount(lists[q]), model[q].size()) << "step " << step;
        }
    }

  for (size_t q = 0; q < 3; ++q)
    EXPECT_EQ(items_of(lists[q]), model[q]);
}
