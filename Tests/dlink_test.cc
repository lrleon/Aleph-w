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
 * @file dlink_test.cc
 * @brief Exhaustive tests for Dlink class
 *
 * These tests cover all operations of Dlink including edge cases
 * and the bug fixes made to the class.
 */

#include <gtest/gtest.h>
#include <vector>
#include <algorithm>
#include <random>
#include <dlink.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Basic Construction and Initialization Tests
// =============================================================================

class DlinkBasicTest : public Test
{
protected:
  Dlink list;
};

TEST_F(DlinkBasicTest, DefaultConstructorCreatesEmptyList)
{
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.get_next(), &list);
  EXPECT_EQ(list.get_prev(), &list);
  EXPECT_TRUE(list.is_unitarian_or_empty());
  EXPECT_FALSE(list.is_unitarian());
}

TEST_F(DlinkBasicTest, CopyConstructorOfEmptyList)
{
  Dlink copy(list);
  EXPECT_TRUE(copy.is_empty());
  EXPECT_EQ(copy.get_next(), &copy);
  EXPECT_EQ(copy.get_prev(), &copy);
}

TEST_F(DlinkBasicTest, MoveConstructorOfEmptyList)
{
  Dlink moved(std::move(list));
  EXPECT_TRUE(moved.is_empty());
  EXPECT_TRUE(list.is_empty());
}

TEST_F(DlinkBasicTest, ResetClearsNode)
{
  Dlink node;
  node.reset();
  EXPECT_TRUE(node.is_empty());
  EXPECT_EQ(node.get_next(), &node);
  EXPECT_EQ(node.get_prev(), &node);
}

TEST_F(DlinkBasicTest, InitIsAliasForReset)
{
  Dlink node;
  node.init();
  EXPECT_TRUE(node.is_empty());
  EXPECT_EQ(node.get_next(), &node);
  EXPECT_EQ(node.get_prev(), &node);
}

// =============================================================================
// Stack Operations Tests (including bug fix verification)
// =============================================================================

class DlinkStackTest : public Test
{
protected:
  Dlink stack;
  Dlink n1, n2, n3, n4, n5;
};

TEST_F(DlinkStackTest, TopOnEmptyStackThrowsCorrectMessage)
{
  // This tests the bug fix: message should say "is empty" not "is not empty"
  EXPECT_TRUE(stack.is_empty());
  try
  {
    stack.top();
    FAIL() << "Expected underflow_error";
  }
  catch (const std::underflow_error &e)
  {
    std::string msg = e.what();
    EXPECT_NE(msg.find("empty"), std::string::npos)
        << "Error message should mention 'empty': " << msg;
    EXPECT_EQ(msg.find("not empty"), std::string::npos)
        << "Error message should NOT say 'not empty': " << msg;
  }
}

TEST_F(DlinkStackTest, PopOnEmptyStackThrowsCorrectMessage)
{
  // This tests the bug fix: message should say "is empty" not "is not empty"
  EXPECT_TRUE(stack.is_empty());
  try
  {
    stack.pop();
    FAIL() << "Expected underflow_error";
  }
  catch (const std::underflow_error &e)
  {
    std::string msg = e.what();
    EXPECT_NE(msg.find("empty"), std::string::npos)
        << "Error message should mention 'empty': " << msg;
    EXPECT_EQ(msg.find("not empty"), std::string::npos)
        << "Error message should NOT say 'not empty': " << msg;
  }
}

TEST_F(DlinkStackTest, PushAndPopSequence)
{
  stack.push(&n1);
  EXPECT_EQ(stack.top(), &n1);
  EXPECT_FALSE(stack.is_empty());
  EXPECT_TRUE(stack.is_unitarian());

  stack.push(&n2);
  EXPECT_EQ(stack.top(), &n2);
  EXPECT_FALSE(stack.is_unitarian());

  stack.push(&n3);
  EXPECT_EQ(stack.top(), &n3);

  EXPECT_EQ(stack.pop(), &n3);
  EXPECT_EQ(stack.top(), &n2);

  EXPECT_EQ(stack.pop(), &n2);
  EXPECT_EQ(stack.top(), &n1);

  EXPECT_EQ(stack.pop(), &n1);
  EXPECT_TRUE(stack.is_empty());
}

TEST_F(DlinkStackTest, AlternatingPushPop)
{
  stack.push(&n1);
  EXPECT_EQ(stack.pop(), &n1);
  EXPECT_TRUE(stack.is_empty());

  stack.push(&n2);
  stack.push(&n3);
  EXPECT_EQ(stack.pop(), &n3);

  stack.push(&n4);
  EXPECT_EQ(stack.pop(), &n4);
  EXPECT_EQ(stack.pop(), &n2);
  EXPECT_TRUE(stack.is_empty());
}

// =============================================================================
// Insert and Append Operations Tests
// =============================================================================

class DlinkInsertAppendTest : public Test
{
protected:
  Dlink list;
  vector<Dlink> nodes;

  void SetUp() override
  {
    nodes.resize(10);
  }
};

TEST_F(DlinkInsertAppendTest, InsertOnEmptyList)
{
  list.insert(&nodes[0]);
  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[0]);
}

TEST_F(DlinkInsertAppendTest, AppendOnEmptyList)
{
  list.append(&nodes[0]);
  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[0]);
}

TEST_F(DlinkInsertAppendTest, InsertMaintainsOrder)
{
  // Insert in reverse order: 0, 1, 2, 3, 4
  // Result should be: 4, 3, 2, 1, 0
  for (size_t i = 0; i < 5; ++i)
    list.insert(&nodes[i]);

  Dlink::Iterator it(list);
  for (int i = 4; i >= 0; --i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkInsertAppendTest, AppendMaintainsOrder)
{
  // Append in order: 0, 1, 2, 3, 4
  // Result should be: 0, 1, 2, 3, 4
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]);

  Dlink::Iterator it(list);
  for (size_t i = 0; i < 5; ++i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkInsertAppendTest, MixedInsertAndAppend)
{
  list.append(&nodes[2]); // list: 2
  list.insert(&nodes[1]); // list: 1, 2
  list.append(&nodes[3]); // list: 1, 2, 3
  list.insert(&nodes[0]); // list: 0, 1, 2, 3

  Dlink::Iterator it(list);
  EXPECT_EQ(it.get_curr(), &nodes[0]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[1]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[2]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[3]);
}

// =============================================================================
// Swap Operations Tests
// =============================================================================

class DlinkSwapTest : public Test
{
protected:
  Dlink list1, list2;
  Dlink n1, n2, n3, n4, n5, n6;
};

TEST_F(DlinkSwapTest, SwapBetweenEmptyLists)
{
  list1.swap(&list2);
  EXPECT_TRUE(list1.is_empty());
  EXPECT_TRUE(list2.is_empty());
}

TEST_F(DlinkSwapTest, SwapEmptyWithNonEmpty)
{
  list1.append(&n1);
  list1.append(&n2);
  list1.append(&n3);

  list1.swap(&list2);

  EXPECT_TRUE(list1.is_empty());
  EXPECT_FALSE(list2.is_empty());
  EXPECT_EQ(list2.get_first(), &n1);
  EXPECT_EQ(list2.get_last(), &n3);
}

TEST_F(DlinkSwapTest, SwapNonEmptyWithEmpty)
{
  list2.append(&n1);
  list2.append(&n2);

  list1.swap(&list2);

  EXPECT_FALSE(list1.is_empty());
  EXPECT_TRUE(list2.is_empty());
  EXPECT_EQ(list1.get_first(), &n1);
  EXPECT_EQ(list1.get_last(), &n2);
}

TEST_F(DlinkSwapTest, SwapBetweenNonEmptyLists)
{
  list1.append(&n1);
  list1.append(&n2);
  list1.append(&n3);

  list2.append(&n4);
  list2.append(&n5);
  list2.append(&n6);

  list1.swap(&list2);

  EXPECT_EQ(list1.get_first(), &n4);
  EXPECT_EQ(list1.get_last(), &n6);
  EXPECT_EQ(list2.get_first(), &n1);
  EXPECT_EQ(list2.get_last(), &n3);
}

TEST_F(DlinkSwapTest, SwapWithReferenceOverload)
{
  list1.append(&n1);
  list2.append(&n2);

  list1.swap(list2);

  EXPECT_EQ(list1.get_first(), &n2);
  EXPECT_EQ(list2.get_first(), &n1);
}

// =============================================================================
// List Operations Tests (concat, split, cut, insert_list, append_list)
// =============================================================================

class DlinkListOperationsTest : public Test
{
protected:
  Dlink list, aux;
  vector<Dlink> nodes;

  void SetUp() override
  {
    nodes.resize(20);
  }

  void populateList(Dlink &l, size_t start, size_t count)
  {
    for (size_t i = start; i < start + count; ++i)
      l.append(&nodes[i]);
  }
};

TEST_F(DlinkListOperationsTest, ConcatListEmptyToEmpty)
{
  list.concat_list(&aux);
  EXPECT_TRUE(list.is_empty());
  EXPECT_TRUE(aux.is_empty());
}

TEST_F(DlinkListOperationsTest, ConcatListNonEmptyToEmpty)
{
  populateList(aux, 0, 3);

  list.concat_list(&aux);

  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[2]);
}

TEST_F(DlinkListOperationsTest, ConcatListEmptyToNonEmpty)
{
  populateList(list, 0, 3);

  list.concat_list(&aux);

  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[2]);
}

TEST_F(DlinkListOperationsTest, ConcatListBothNonEmpty)
{
  populateList(list, 0, 3); // list: 0, 1, 2
  populateList(aux, 3, 3);  // aux: 3, 4, 5

  list.concat_list(&aux);

  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[5]);

  // Verify order
  Dlink::Iterator it(list);
  for (size_t i = 0; i < 6; ++i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkListOperationsTest, InsertListEmpty)
{
  populateList(list, 0, 3);

  nodes[1].insert_list(&aux); // Insert empty list after nodes[1]

  // List should be unchanged
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[2]);
}

TEST_F(DlinkListOperationsTest, InsertListInMiddle)
{
  populateList(list, 0, 3); // list: 0, 1, 2
  populateList(aux, 10, 2); // aux: 10, 11

  nodes[0].insert_list(&aux); // Insert after nodes[0]
  // Expected: 0, 10, 11, 1, 2

  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[2]);

  Dlink::Iterator it(list);
  EXPECT_EQ(it.get_curr(), &nodes[0]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[10]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[11]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[1]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[2]);
}

TEST_F(DlinkListOperationsTest, AppendListEmpty)
{
  populateList(list, 0, 3);

  nodes[1].append_list(&aux); // Append empty list before nodes[1]

  // List should be unchanged
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[2]);
}

TEST_F(DlinkListOperationsTest, SplitEmptyList)
{
  Dlink l, r;
  size_t count = list.split_list(l, r);

  EXPECT_EQ(count, 0);
  EXPECT_TRUE(l.is_empty());
  EXPECT_TRUE(r.is_empty());
}

TEST_F(DlinkListOperationsTest, SplitUnitaryList)
{
  list.append(&nodes[0]);

  Dlink l, r;
  size_t count = list.split_list(l, r);

  EXPECT_EQ(count, 1);
  EXPECT_TRUE(list.is_empty());
  // One element goes to l or r
  EXPECT_TRUE(!l.is_empty() || !r.is_empty());
}

TEST_F(DlinkListOperationsTest, SplitEvenList)
{
  populateList(list, 0, 6); // list: 0, 1, 2, 3, 4, 5

  Dlink l, r;
  size_t count = list.split_list(l, r);

  EXPECT_EQ(count, 6);
  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(l.is_empty());
  EXPECT_FALSE(r.is_empty());
}

TEST_F(DlinkListOperationsTest, SplitOddList)
{
  populateList(list, 0, 5); // list: 0, 1, 2, 3, 4

  Dlink l, r;
  size_t count = list.split_list(l, r);

  EXPECT_EQ(count, 5);
  EXPECT_TRUE(list.is_empty());
}

TEST_F(DlinkListOperationsTest, CutListAtFirst)
{
  populateList(list, 0, 5); // list: 0, 1, 2, 3, 4

  Dlink cut = list.cut_list(&nodes[0]);

  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(cut.get_first(), &nodes[0]);
  EXPECT_EQ(cut.get_last(), &nodes[4]);
}

TEST_F(DlinkListOperationsTest, CutListAtLast)
{
  populateList(list, 0, 5); // list: 0, 1, 2, 3, 4

  Dlink cut = list.cut_list(&nodes[4]);

  EXPECT_FALSE(list.is_empty());
  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[3]);
  EXPECT_TRUE(cut.is_unitarian());
  EXPECT_EQ(cut.get_first(), &nodes[4]);
}

TEST_F(DlinkListOperationsTest, CutListInMiddle)
{
  populateList(list, 0, 5); // list: 0, 1, 2, 3, 4

  Dlink cut = list.cut_list(&nodes[2]);

  EXPECT_EQ(list.get_first(), &nodes[0]);
  EXPECT_EQ(list.get_last(), &nodes[1]);
  EXPECT_EQ(cut.get_first(), &nodes[2]);
  EXPECT_EQ(cut.get_last(), &nodes[4]);
}

// =============================================================================
// Reverse Operations Tests
// =============================================================================

class DlinkReverseTest : public Test
{
protected:
  Dlink list;
  vector<Dlink> nodes;

  void SetUp() override
  {
    nodes.resize(10);
  }
};

TEST_F(DlinkReverseTest, ReverseEmptyList)
{
  size_t count = list.reverse_list();
  EXPECT_EQ(count, 0);
  EXPECT_TRUE(list.is_empty());
}

TEST_F(DlinkReverseTest, ReverseUnitaryList)
{
  list.append(&nodes[0]);
  size_t count = list.reverse_list();
  EXPECT_EQ(count, 1);
  EXPECT_EQ(list.get_first(), &nodes[0]);
}

TEST_F(DlinkReverseTest, ReverseTwoElements)
{
  list.append(&nodes[0]);
  list.append(&nodes[1]);

  list.reverse_list();

  EXPECT_EQ(list.get_first(), &nodes[1]);
  EXPECT_EQ(list.get_last(), &nodes[0]);
}

TEST_F(DlinkReverseTest, ReverseMultipleElements)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]); // list: 0, 1, 2, 3, 4

  size_t count = list.reverse_list();

  EXPECT_EQ(count, 5);
  EXPECT_EQ(list.get_first(), &nodes[4]);
  EXPECT_EQ(list.get_last(), &nodes[0]);

  // Verify order: 4, 3, 2, 1, 0
  Dlink::Iterator it(list);
  for (int i = 4; i >= 0; --i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkReverseTest, DoubleReverseRestoresOrder)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]);

  list.reverse_list();
  list.reverse_list();

  Dlink::Iterator it(list);
  for (size_t i = 0; i < 5; ++i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

// =============================================================================
// Rotation Tests
// =============================================================================

class DlinkRotationTest : public Test
{
protected:
  Dlink list;
  vector<Dlink> nodes;

  void SetUp() override
  {
    nodes.resize(10);
    for (size_t i = 0; i < 5; ++i)
      list.append(&nodes[i]); // list: 0, 1, 2, 3, 4
  }
};

TEST_F(DlinkRotationTest, RotateLeftZero)
{
  list.rotate_left(0);

  Dlink::Iterator it(list);
  for (size_t i = 0; i < 5; ++i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkRotationTest, RotateLeftOne)
{
  list.rotate_left(1); // Expected: 1, 2, 3, 4, 0

  Dlink::Iterator it(list);
  EXPECT_EQ(it.get_curr(), &nodes[1]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[2]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[3]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[4]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[0]);
}

TEST_F(DlinkRotationTest, RotateLeftFullCycle)
{
  list.rotate_left(5); // Full rotation should restore original order

  Dlink::Iterator it(list);
  for (size_t i = 0; i < 5; ++i, it.next())
    EXPECT_EQ(it.get_curr(), &nodes[i]);
}

TEST_F(DlinkRotationTest, RotateRightOne)
{
  list.rotate_right(1); // Expected: 4, 0, 1, 2, 3

  Dlink::Iterator it(list);
  EXPECT_EQ(it.get_curr(), &nodes[4]);
  it.next();
  EXPECT_EQ(it.get_curr(), &nodes[0]);
}

TEST_F(DlinkRotationTest, RotateEmptyListThrows)
{
  Dlink empty;
  EXPECT_THROW(empty.rotate_left(1), std::domain_error);
  EXPECT_THROW(empty.rotate_right(1), std::domain_error);
  EXPECT_NO_THROW(empty.rotate_left(0));
  EXPECT_NO_THROW(empty.rotate_right(0));
}

// =============================================================================
// Iterator Tests
// =============================================================================

class DlinkIteratorTest : public Test
{
protected:
  Dlink list;
  vector<Dlink> nodes;

  void SetUp() override
  {
    nodes.resize(10);
  }
};

TEST_F(DlinkIteratorTest, IteratorOnEmptyList)
{
  Dlink::Iterator it(list);
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_THROW(it.next(), std::overflow_error);
  EXPECT_THROW(it.prev(), std::underflow_error);
}

TEST_F(DlinkIteratorTest, IteratorForwardTraversal)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]);

  size_t count = 0;
  for (Dlink::Iterator it(list); it.has_curr(); it.next(), ++count)
    EXPECT_EQ(it.get_curr(), &nodes[count]);

  EXPECT_EQ(count, 5);
}

TEST_F(DlinkIteratorTest, IteratorBackwardTraversal)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]);

  Dlink::Iterator it(list);
  it.reset_last();

  int count = 4;
  while (it.has_curr())
  {
    EXPECT_EQ(it.get_curr(), &nodes[count]);
    it.prev();
    --count;
  }
  EXPECT_EQ(count, -1);
}

TEST_F(DlinkIteratorTest, IteratorDeleteAllElements)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]);

  Dlink::Iterator it(list);
  while (it.has_curr())
    it.del();

  EXPECT_TRUE(list.is_empty());
}

TEST_F(DlinkIteratorTest, IteratorDeleteMiddleElement)
{
  for (size_t i = 0; i < 5; ++i)
    list.append(&nodes[i]); // list: 0, 1, 2, 3, 4

  Dlink::Iterator it(list);
  it.next();
  it.next(); // Now at nodes[2]

  Dlink *deleted = it.del();
  EXPECT_EQ(deleted, &nodes[2]);

  // Iterator should now be at nodes[3]
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), &nodes[3]);
}

TEST_F(DlinkIteratorTest, IteratorIsFirstAndIsLast)
{
  for (size_t i = 0; i < 3; ++i)
    list.append(&nodes[i]);

  Dlink::Iterator it(list);
  EXPECT_TRUE(it.is_in_first());
  EXPECT_FALSE(it.is_last());

  it.next();
  EXPECT_FALSE(it.is_in_first());
  EXPECT_FALSE(it.is_last());

  it.next();
  EXPECT_FALSE(it.is_in_first());
  EXPECT_TRUE(it.is_last());
}

TEST_F(DlinkIteratorTest, IteratorEqualityOperators)
{
  for (size_t i = 0; i < 3; ++i)
    list.append(&nodes[i]);

  Dlink::Iterator it1(list);
  Dlink::Iterator it2(list);

  EXPECT_TRUE(it1 == it2);
  EXPECT_FALSE(it1 != it2);

  it1.next();
  EXPECT_FALSE(it1 == it2);
  EXPECT_TRUE(it1 != it2);
}

// =============================================================================
// Check Consistency Tests
// =============================================================================

TEST(DlinkConsistency, CheckEmptyList)
{
  Dlink list;
  EXPECT_TRUE(list.check());
}

TEST(DlinkConsistency, CheckAfterOperations)
{
  Dlink list;
  vector<Dlink> nodes(10);

  for (size_t i = 0; i < 10; ++i)
    list.append(&nodes[i]);

  EXPECT_TRUE(list.check());

  list.reverse_list();
  EXPECT_TRUE(list.check());

  list.rotate_left(3);
  EXPECT_TRUE(list.check());

  Dlink l, r;
  list.split_list(l, r);
  EXPECT_TRUE(l.check());
  EXPECT_TRUE(r.check());
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DlinkStress, LargeListOperations)
{
  constexpr size_t N = 10000;
  Dlink list;
  vector<Dlink> nodes(N);

  // Build large list
  for (size_t i = 0; i < N; ++i)
    list.append(&nodes[i]);

  EXPECT_TRUE(list.check());

  // Reverse
  size_t count = list.reverse_list();
  EXPECT_EQ(count, N);
  EXPECT_TRUE(list.check());

  // Split
  Dlink l, r;
  list.split_list(l, r);
  EXPECT_TRUE(list.is_empty());
  EXPECT_TRUE(l.check());
  EXPECT_TRUE(r.check());

  // Concat back
  l.concat_list(&r);
  EXPECT_TRUE(l.check());
}

TEST(DlinkStress, RandomOperations)
{
  constexpr size_t N = 1000;
  constexpr size_t OPS = 5000;

  Dlink list;
  vector<Dlink> nodes(N);
  vector<bool> inList(N, false);

  std::mt19937 rng(42);
  std::uniform_int_distribution<size_t> nodeDist(0, N - 1);
  std::uniform_int_distribution<int> opDist(0, 3);

  for (size_t op = 0; op < OPS; ++op)
  {
    int operation = opDist(rng);
    size_t nodeIdx = nodeDist(rng);

    switch (operation)
    {
    case 0: // Insert
      if (!inList[nodeIdx])
      {
        list.insert(&nodes[nodeIdx]);
        inList[nodeIdx] = true;
      }
      break;
    case 1: // Append
      if (!inList[nodeIdx])
      {
        list.append(&nodes[nodeIdx]);
        inList[nodeIdx] = true;
      }
      break;
    case 2: // Remove first
      if (!list.is_empty())
      {
        Dlink *removed = list.remove_first();
        for (size_t i = 0; i < N; ++i)
        {
          if (&nodes[i] == removed)
          {
            inList[i] = false;
            break;
          }
        }
      }
      break;
    case 3: // Remove last
      if (!list.is_empty())
      {
        Dlink *removed = list.remove_last();
        for (size_t i = 0; i < N; ++i)
        {
          if (&nodes[i] == removed)
          {
            inList[i] = false;
            break;
          }
        }
      }
      break;
    }
  }

  EXPECT_TRUE(list.check());
}

// =============================================================================
// Move Semantics Tests
// =============================================================================

TEST(DlinkMoveSemantics, MoveConstructor)
{
  Dlink list;
  Dlink n1, n2, n3;
  list.append(&n1);
  list.append(&n2);
  list.append(&n3);

  Dlink moved(std::move(list));

  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(moved.is_empty());
  EXPECT_EQ(moved.get_first(), &n1);
  EXPECT_EQ(moved.get_last(), &n3);
}

TEST(DlinkMoveSemantics, MoveAssignment)
{
  Dlink list1, list2;
  Dlink n1, n2, n3;
  list1.append(&n1);
  list1.append(&n2);
  list1.append(&n3);

  list2 = std::move(list1);

  EXPECT_TRUE(list1.is_empty());
  EXPECT_FALSE(list2.is_empty());
  EXPECT_EQ(list2.get_first(), &n1);
  EXPECT_EQ(list2.get_last(), &n3);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(DlinkEdgeCases, SelfSwap)
{
  Dlink list;
  Dlink n1, n2;
  list.append(&n1);
  list.append(&n2);

  // Self swap should work without issues
  list.swap(&list);

  EXPECT_FALSE(list.is_empty());
  EXPECT_EQ(list.get_first(), &n1);
  EXPECT_EQ(list.get_last(), &n2);
}

TEST(DlinkEdgeCases, RemoveOnlyElement)
{
  Dlink list;
  Dlink n1;
  list.append(&n1);

  EXPECT_TRUE(list.is_unitarian());

  Dlink *removed = list.remove_first();
  EXPECT_EQ(removed, &n1);
  EXPECT_TRUE(list.is_empty());
}

TEST(DlinkEdgeCases, IteratorOnUnitaryList)
{
  Dlink list;
  Dlink n1;
  list.append(&n1);

  Dlink::Iterator it(list);
  EXPECT_TRUE(it.has_curr());
  EXPECT_TRUE(it.is_in_first());
  EXPECT_TRUE(it.is_last());
  EXPECT_EQ(it.get_curr(), &n1);

  it.next();
  EXPECT_FALSE(it.has_curr());
}

