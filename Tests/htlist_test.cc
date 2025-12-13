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
  Slinknc other;
  link.insert(&other);
  EXPECT_FALSE(link.is_empty());

  Slinknc copy(link);
  EXPECT_TRUE(copy.is_empty());
}

TEST_F(SlinkcTest, AssignmentResetsLink)
{
  Slinknc other, another;
  link.insert(&other);

  another = link;
  EXPECT_TRUE(another.is_empty());
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
  list.split(l, r);

  // The split algorithm uses tortoise-hare, count may not equal total elements
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size() + r.size(), 10);

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
  list.split(l, r);

  // The split algorithm uses tortoise-hare, count may not equal total elements
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size() + r.size(), 7);

  l.concat(r);
  l.remove_all_and_delete();
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

