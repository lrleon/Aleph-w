
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
 * @file htlist.cc
 * @brief Tests for Htlist
 */
# include <gtest/gtest.h>

# include <htlist.H>
# include <stdexcept>
# include <utility>

using namespace std;
using namespace testing;
using namespace Aleph;

struct List_of_25_nodes : public Test
{
  HTList list;
  List_of_25_nodes()
  {
    for (size_t i = 0; i < 25; ++i)
      list.append(new Snodenc<int>(i + 1));
  }
  ~List_of_25_nodes()
  {
    list.remove_all_and_delete();
  }
};

TEST(HTList, Basic_operations)
{
  HTList list;
  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());
  EXPECT_EQ(list.get_head(), list.get_first());
  EXPECT_EQ(list.get_tail(), list.get_last());

  list.append(new Snodenc<int>(2));
  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());
  EXPECT_EQ(list.get_head(), list.get_first());
  EXPECT_EQ(list.get_tail(), list.get_last());
  EXPECT_EQ(list.get_first(), list.get_last());

  list.insert(new Snodenc<int>(1));
  EXPECT_FALSE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_FALSE(list.is_unitarian_or_empty());
  EXPECT_EQ(list.get_head(), list.get_first());
  EXPECT_EQ(list.get_tail(), list.get_last());

  // list = { 1, 2} 

  EXPECT_EQ(static_cast<Snodenc<int>*>(list.get_first())->get_data(), 1);
  EXPECT_EQ(static_cast<Snodenc<int>*>(list.get_last())->get_data(), 2);

  auto p1 = new Snodenc<int>(0);
  auto p2 = new Snodenc<int>(3);
  list.insert(p1);
  list.append(p2);
  EXPECT_EQ(list.get_first()->to_snodenc<int>()->get_data(), 0);
  EXPECT_EQ(list.get_last()->to_snodenc<int>()->get_data(), 3);

  // list = { 0, 1, 2, 3} 

  auto fst = list.remove_first(); // remove 0
  EXPECT_EQ(fst, p1);
  EXPECT_EQ(fst->to_snodenc<int>()->get_data(), 0);
  delete fst;
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);

  fst = list.remove_first(); // remove 1
  EXPECT_EQ(fst->to_data<int>(), 1);
  delete fst;
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);

  fst = list.remove_first(); // remove 2
  EXPECT_EQ(fst->to_data<int>(), 2);
  delete fst;
  EXPECT_EQ(list.size(), 1);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);

  //list = { 3 }

  EXPECT_EQ(list.get_first()->to_data<int>(), 3);
  EXPECT_EQ(list.get_last()->to_data<int>(), 3);
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());

  fst = list.remove_first(); // remove 3
  EXPECT_EQ(fst->to_data<int>(), 3);
  delete fst;
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.size(), 0);

  EXPECT_THROW(list.rotate_left(1), std::domain_error);
  EXPECT_NO_THROW(list.rotate_left(0));
}

TEST(HTList, Simple_append_and_insert_of_list)
{
  HTList laux, list;
  laux.insert(new Snodenc<int>(2));
  list.append(laux);

  EXPECT_TRUE(laux.is_empty());
  EXPECT_TRUE(list.is_unitarian());

  laux.insert(new Snodenc<int>(1));
  list.insert(laux);
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.get_first()->to_data<int>(), 1);
  EXPECT_EQ(list.get_last()->to_data<int>(), 2);

  list.remove_all_and_delete();
}

TEST_F(List_of_25_nodes, Basic_operations)
{
  EXPECT_EQ(static_cast<Snodenc<int>*>(list.get_first())->get_data(), 1);
  EXPECT_EQ(static_cast<Snodenc<int>*>(list.get_last())->get_data(), 25);
  EXPECT_EQ(list.get_first(), list.get_head());
  EXPECT_EQ(list.get_last(), list.get_tail());
  EXPECT_EQ(list.size(), 25);
  EXPECT_FALSE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_FALSE(list.is_unitarian_or_empty());
}

TEST(HTList, Iterator_on_empty_list)
{
  HTList l;
  HTList::Iterator it(l);
  ASSERT_EQ(it.has_curr(), false);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_FALSE(it.is_in_last());
  EXPECT_FALSE(it.is_in_first());

  it.reset_first();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
  EXPECT_FALSE(it.is_last());
  EXPECT_FALSE(it.is_in_first());
}

TEST_F(List_of_25_nodes, Iterator_operations)
{
  int i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    {
      EXPECT_EQ(it.get_curr()->to_snodenc<int>()->get_data(), i);
      EXPECT_EQ(it.get_pos(), i - 1);
    }
}

TEST_F(List_of_25_nodes, Split_and_concat)
{
  HTList l, r;
  list.split(l, r);

  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.size(), 13);
  EXPECT_EQ(r.size(), 12);
  EXPECT_EQ(l.get_first()->to_data<int>(), 1);
  EXPECT_EQ(l.get_last()->to_data<int>(), 13);
  EXPECT_EQ(r.get_first()->to_data<int>(), 14);
  EXPECT_EQ(r.get_last()->to_data<int>(), 25);

  int i = 1;
  for (HTList::Iterator it = l; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  for (HTList::Iterator it = r; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  list.append(r);
  list.insert(l);
  i = 1;
  for (HTList::Iterator it = l; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
}

TEST_F(List_of_25_nodes, swap)
{
  HTList laux;
  laux.swap(list);

  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.size(), 0);
  EXPECT_FALSE(laux.is_empty());

  int i = 1;
  for (HTList::Iterator it = laux; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  laux.remove_all_and_delete();
}

TEST_F(List_of_25_nodes, cut_and_concat)
{
  HTList::Iterator it = list;
  for (size_t i = 1; i < 13; ++i, it.next())
    ;

  HTList laux;
  list.cut(it.get_curr(), laux);

  EXPECT_FALSE(laux.is_empty());

  int i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  for (HTList::Iterator it = laux; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  list.concat(laux);
  i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
}

TEST_F(List_of_25_nodes, reverse)
{
  EXPECT_EQ(list.reverse(), 25);
  int i = 25;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), --i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  EXPECT_EQ(list.reverse(), 25);

  i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  HTList l, r;
  list.split(l, r);

  EXPECT_TRUE(list.is_empty());

  l.reverse();
  r.reverse();
  list.insert(l);
  list.insert(r);
  EXPECT_TRUE(l.is_empty());
  EXPECT_TRUE(r.is_empty());

  list.reverse();
  i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
}

TEST_F(List_of_25_nodes, rotate_left)
{
  list.rotate_left(3);

  {
    HTList::Iterator it = list;
    for (int i = 0, n = 4; i < 3; ++i, ++n, it.next())
      EXPECT_EQ(it.get_curr()->to_data<int>(), n);
  }
  
  list.rotate_left(22);
  int i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
}

TEST(HTList, As_stack)
{
  HTList stack;
  ASSERT_TRUE(stack.is_empty());
  ASSERT_THROW(stack.top(), underflow_error);
  ASSERT_THROW(stack.pop(), underflow_error);
  ASSERT_TRUE(stack.is_unitarian_or_empty());
  Slinknc n1, n2, n3;
  stack.push(&n3);

  ASSERT_FALSE(stack.is_empty());
  ASSERT_TRUE(stack.is_unitarian());

  stack.push(&n2);
  stack.push(&n1);
  ASSERT_EQ(stack.top(), &n1);
  ASSERT_EQ(stack.pop(), &n1);
  ASSERT_EQ(stack.top(), &n2);
  ASSERT_EQ(stack.pop(), &n2);

  ASSERT_TRUE(stack.is_unitarian());

  ASSERT_EQ(stack.top(), &n3);
  ASSERT_EQ(stack.pop(), &n3);
  ASSERT_FALSE(stack.is_unitarian());
  ASSERT_TRUE(stack.is_empty());
}

namespace
{
  // Its move throws while fail is true; moves counts them
  struct Fragile_Move
  {
    static inline bool fail = false;
    static inline int moves = 0;
    int value = 0;

    Fragile_Move(int v = 0) noexcept : value(v) {}
    Fragile_Move(const Fragile_Move &) = default;
    Fragile_Move(Fragile_Move && c) : value(c.value)
    {
      ++moves;
      if (fail)
        throw std::runtime_error("move failed");
    }
    Fragile_Move & operator = (const Fragile_Move &) = default;
    Fragile_Move & operator = (Fragile_Move &&) = default;
  };
} // namespace

// remove_ne() copied nothing, but it was noexcept, so that a move that threw
// ended the program
static_assert(noexcept(std::declval<DynList<int> &>().remove_ne()));
static_assert(not noexcept(std::declval<DynList<Fragile_Move> &>().remove_ne()));

// remove() unlinked the node before it moved the item out: a move that threw
// lost the item and its node
TEST(DynList, AMoveThatThrowsLeavesTheItemInTheList)
{
  DynList<Fragile_Move> list;
  for (int i = 1; i <= 3; ++i)
    list.append(Fragile_Move(i));

  Fragile_Move::fail = true;
  EXPECT_THROW((void) list.remove_ne(), std::runtime_error);
  EXPECT_THROW((void) list.remove(), std::runtime_error);
  EXPECT_THROW((void) list.remove_first(), std::runtime_error);
  EXPECT_THROW((void) list.pop(), std::runtime_error);
  EXPECT_THROW((void) list.get(), std::runtime_error);
  Fragile_Move::fail = false;

  ASSERT_EQ(list.size(), 3u);
  EXPECT_EQ(list.get_first().value, 1);
  EXPECT_EQ(list.remove().value, 1);
  EXPECT_EQ(list.size(), 2u);
  EXPECT_THROW((void) DynList<Fragile_Move>().remove(), std::underflow_error);
}

// The item moves once, straight into the result: a second move, as that of
// a named result that the compiler does not elide, would come after the
// node left, and if it threw the item would be lost
TEST(DynList, ARemoveMovesTheItemOnce)
{
  DynList<Fragile_Move> list;
  for (int i = 1; i <= 3; ++i)
    list.append(Fragile_Move(i));

  Fragile_Move::moves = 0;
  const Fragile_Move first = list.remove_ne();
  EXPECT_EQ(Fragile_Move::moves, 1);
  const Fragile_Move second = list.remove();
  EXPECT_EQ(Fragile_Move::moves, 2);
  EXPECT_EQ(first.value, 1);
  EXPECT_EQ(second.value, 2);
  EXPECT_EQ(list.size(), 1u);
}

// Iterator::del() also unlinked the node before it moved the item out. Now a
// move that throws leaves the item, and the iterator on it, on the first, a
// middle and the last item
TEST(DynList, AMoveThatThrowsInDelLeavesTheItemInTheList)
{
  for (int pos = 0; pos < 3; ++pos)
    {
      DynList<Fragile_Move> list;
      for (int i = 1; i <= 3; ++i)
        list.append(Fragile_Move(i));

      DynList<Fragile_Move>::Iterator it(list);
      for (int k = 0; k < pos; ++k)
        it.next();

      Fragile_Move::fail = true;
      EXPECT_THROW((void) it.del(), std::runtime_error);
      Fragile_Move::fail = false;

      ASSERT_EQ(list.size(), 3u);
      ASSERT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr().value, pos + 1);

      Fragile_Move::moves = 0;
      EXPECT_EQ(it.del().value, pos + 1); // one move, straight into the result
      EXPECT_EQ(Fragile_Move::moves, 1);
      ASSERT_EQ(list.size(), 2u);
      EXPECT_EQ(list.get_first().value, pos == 0 ? 2 : 1);
      EXPECT_EQ(list.get_last().value, pos == 2 ? 2 : 3);
      ASSERT_EQ(it.has_curr(), pos < 2);
      if (pos < 2)
        {
          EXPECT_EQ(it.get_curr().value, pos + 2);
        }
    }

  DynList<Fragile_Move> empty;
  DynList<Fragile_Move>::Iterator it(empty);
  EXPECT_THROW((void) it.del(), std::overflow_error);
}
