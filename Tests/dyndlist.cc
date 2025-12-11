
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

# include <gtest/gtest.h>

# include <tpl_dynDlist.H>

using namespace std;
using namespace testing;
using namespace Aleph;

# define Declare_list_n_items(num)            \
  struct List_of_##num##_items : public Test	\
  {                                           \
    size_t n = 0;                             \
    DynDlist<int> list##num;                  \
    DynDlist<int> rlist##num;                 \
    List_of_##num##_items()                   \
      {                                       \
        for (size_t i = 0; i < num; ++i, ++n)	\
          list##num.append(i + 1);            \
                                              \
        const DynDlist<int> l = list##num;    \
        rlist##num = l.rev();                 \
      }                                       \
  };

Declare_list_n_items(25);
Declare_list_n_items(10);

TEST(DynDlist, Basic_operations)
{
  DynDlist<int> list;
  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());

  list.append(2);
  EXPECT_FALSE(list.is_empty());
  EXPECT_TRUE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());
  EXPECT_EQ(list.get_first(), list.get_last());

  list.insert(1);
  EXPECT_FALSE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_FALSE(list.is_unitarian_or_empty());

  EXPECT_EQ(list.get_first(), 1);
  EXPECT_EQ(list.get_last(), 2);

  list.insert(0);
  list.append(3);
  EXPECT_EQ(list.get_first(), 0);
  EXPECT_EQ(list.get_last(), 3);

  EXPECT_EQ(list.remove_first(), 0);
  EXPECT_EQ(list.size(), 3);
  EXPECT_EQ(list.get_last(), 3);

  EXPECT_EQ(list.remove_first(), 1);
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.get_last(), 3);

  EXPECT_EQ(list.remove_first(), 2);
  EXPECT_EQ(list.size(), 1);
  EXPECT_EQ(list.get_last(), 3);

  EXPECT_TRUE(list.is_unitarian());
  EXPECT_TRUE(list.is_unitarian_or_empty());

  EXPECT_EQ(list.remove_first(), 3);
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.size(), 0);

  EXPECT_THROW(list.rotate_left(1), std::domain_error);
  EXPECT_NO_THROW(list.rotate_left(0));
}

TEST(DynDlist, Simple_append_and_insert_of_list)
{
  {                                // by copy
    DynDlist<int> laux, list;
    laux.insert(2);                // laux = { 2 }
    list.append(laux);             // list = { 2 }

    EXPECT_FALSE(laux.is_empty());
    EXPECT_FALSE(list.is_empty());
    EXPECT_EQ(list.get_first(), 2);
    EXPECT_EQ(list.get_first(), laux.get_first());
    EXPECT_TRUE(list.is_unitarian());

    laux.insert(1);                // laux = { 1, 2 }
    list.insert(laux);             // list = { 1, 2, 2 }
    EXPECT_FALSE(laux.is_empty());
    EXPECT_EQ(laux.get_first(), 1);
    EXPECT_EQ(laux.get_last(), 2);
    EXPECT_EQ(list.size(), 3);
    EXPECT_EQ(list.get_first(), 1);
    EXPECT_EQ(list.get_last(), 2);
  }

  {                                // by move
    DynDlist<int> laux, list;
    laux.insert(2);
    list.append(move(laux));

    EXPECT_TRUE(laux.is_empty());
    EXPECT_FALSE(list.is_empty());
    EXPECT_EQ(list.get_first(), 2);
    EXPECT_EQ(list.get_last(), 2);
    EXPECT_TRUE(list.is_unitarian());
    EXPECT_TRUE(list.is_unitarian_or_empty());

    laux.insert(1);
    list.insert(move(laux));
    EXPECT_TRUE(laux.is_empty());
    EXPECT_EQ(list.size(), 2);
    EXPECT_EQ(list.get_first(), 1);
    EXPECT_EQ(list.get_last(), 2);
  }
}

TEST_F(List_of_10_items, copy_and_assignment)
{
  {                    // copy constructor
    auto tmp = list10;

    EXPECT_EQ(tmp.size(), list10.size());
    EXPECT_EQ(tmp.size(), 10);

    auto it1 = list10.get_it();
    auto it2 = tmp.get_it();
    for (; it1.has_curr() and it2.has_curr(); it1.next(), it2.next())
      ASSERT_EQ(it1.get_curr(), it2.get_curr());

    EXPECT_FALSE(it1.has_curr());
    EXPECT_FALSE(it2.has_curr());
  }

  EXPECT_FALSE(list10.is_empty());

  {                // move constructor
    auto tmp = move(list10);

    EXPECT_TRUE(list10.is_empty());
    EXPECT_EQ(list10.size(), 0);
    EXPECT_EQ(tmp.size(), 10);
    int i = 1;
    for (auto it = tmp.get_it(); it.has_curr(); it.next(), ++i)
      ASSERT_EQ(it.get_curr(), i);
    EXPECT_EQ(i, 11);
    list10.swap(tmp);
  }

  EXPECT_FALSE(list10.is_empty());
  DynDlist<int> aux;
  aux = list10;
  EXPECT_EQ(aux.size(), 10);
  int i = 1;
  for (auto it = aux.get_it(); it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_EQ(i, 11);

  aux.empty();
  EXPECT_TRUE(aux.is_empty());

  aux = move(list10);
  EXPECT_TRUE(list10.is_empty());
  EXPECT_EQ(aux.size(), 10);

  i = 1;
  for (auto it = aux.get_it(); it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_EQ(i, 11);

  list10 = move(aux);

  EXPECT_TRUE(aux.is_empty());
  EXPECT_EQ(list10.size(), 10);
  i = 1;
  for (auto it = list10.get_it(); it.has_curr(); it.next(), ++i)
    ASSERT_EQ(it.get_curr(), i);
  EXPECT_EQ(i, 11);
}

TEST_F(List_of_25_items, Basic_operations)
{
  EXPECT_EQ(list25.get_first(), 1);
  EXPECT_EQ(list25.get_last(), 25);
  EXPECT_EQ(list25.size(), 25);
  EXPECT_FALSE(list25.is_empty());
  EXPECT_FALSE(list25.is_unitarian());
  EXPECT_FALSE(list25.is_unitarian_or_empty());
}

TEST_F(List_of_25_items, Iterator_operations)
{
  list25.for_each([] (auto i) { cout << i << " "; }); cout << endl;
  int i = 1;
  auto it = list25.get_it();
  for (; it.has_curr(); it.next(), ++i)
    {
      EXPECT_EQ(it.get_curr(), i);
      EXPECT_EQ(it.get_pos(), i - 1);
    }
  EXPECT_EQ(i, 26);

  i = 25;
  it.reset_last();
  for (; it.has_curr(); it.prev(), --i)
    {
      EXPECT_EQ(it.get_curr(), i);
      EXPECT_EQ(it.get_pos(), i - 1);
    }
  EXPECT_EQ(i, 0);
}

TEST_F(List_of_25_items, Split_and_concat)
{
  DynDlist<int> l, r;
  list25.split(l, r);

  EXPECT_TRUE(list25.is_empty());
  EXPECT_EQ(l.size(), 13);
  EXPECT_EQ(r.size(), 12);
  EXPECT_EQ(l.get_first(), 1);
  EXPECT_EQ(l.get_last(), 13);
  EXPECT_EQ(r.get_first(), 14);
  EXPECT_EQ(r.get_last(), 25);

  int i = 1;
  for (auto it = l.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  for (auto it = r.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  list25.append(r);
  list25.insert(l);
  i = 1;
  for (auto it = l.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, swap)
{
  DynDlist<int> laux;
  laux.swap(list25);

  EXPECT_TRUE(list25.is_empty());
  EXPECT_EQ(list25.size(), 0);
  EXPECT_FALSE(laux.is_empty());

  int i = 1;
  for (auto it = laux.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, reverse)
{
  list25.reverse();
  int i = 25;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), --i)
    EXPECT_EQ(it.get_curr(), i);

  list25.reverse();

  i = 1;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  DynDlist<int> l, r;
  list25.split(l, r);

  EXPECT_TRUE(list25.is_empty());

  l.reverse();
  r.reverse();
  list25.insert(l);
  list25.insert(r);

  list25.reverse();
  i = 1;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, rotate_left)
{
  list25.rotate_left(3);

  {
    auto it = list25.get_it();
    for (int i = 0, n = 4; i < 3; ++i, ++n, it.next())
      EXPECT_EQ(it.get_curr(), n);
  }
  
  list25.rotate_left(22);
  int i = 1;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, rotate_right)
{
  list25.rotate_right(3);

  {
    auto it = list25.get_it();
    for (int i = 0, n = 23; i < 3; ++i, ++n, it.next())
      EXPECT_EQ(it.get_curr(), n);
  }
  
  list25.rotate_right(22);
  int i = 1;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, append_and_insert_by_moving)
{
  DynDlist<int> ll = { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9 };
  DynDlist<int> lg = { 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };

  ll.reverse();

  list25.insert(std::move(ll));
  list25.append(std::move(lg));

  EXPECT_TRUE(ll.is_empty());
  EXPECT_TRUE(lg.is_empty());

  int i = -9;
  for (auto it = list25.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST(DynDlist, traverse_on_empty_container)
{
  DynDlist<int> m;
  size_t n = 0;
  auto ret = m.traverse([&n] (int) { ++n; return true; });
  EXPECT_TRUE(ret);
  EXPECT_EQ(n, 0);
}

TEST_F(List_of_25_items, traverse)
{
  EXPECT_TRUE(list25.size() > 0);
  EXPECT_EQ(list25.size(), n);
  int N = 0;
  auto ret = list25.traverse([&N, this] (int i) { ++N; return i < n/2; });
  EXPECT_FALSE(ret);
  EXPECT_EQ(N, n/2);
}

TEST(DynDlist, RemoveAndEraseByReference)
{
  DynDlist<int> list;
  for (int i = 1; i <= 4; ++i)
    list.append(i);

  int &middle = list[2];
  list.remove(middle);
  ASSERT_EQ(list.size(), 3u);
  int expected_after_remove[] = {1, 2, 4};
  int idx = 0;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++idx)
    EXPECT_EQ(it.get_curr(), expected_after_remove[idx]);

  int &first = list.get_first();
  list.erase(first);
  EXPECT_EQ(list.size(), 2u);
  EXPECT_EQ(list.get_first(), 2);
  EXPECT_EQ(list.get_last(), 4);
}

TEST(DynDlist, OperatorIndexEnforcesBounds)
{
  DynDlist<int> list;
  for (int i = 0; i < 5; ++i)
    list.append(i);

  list[2] = 99;
  EXPECT_EQ(list[2], 99);

  const DynDlist<int> &clist = list;
  EXPECT_EQ(clist[0], 0);

  EXPECT_THROW(list[5], std::out_of_range);
  EXPECT_THROW(list[10], std::out_of_range);

  DynDlist<int> empty;
  EXPECT_THROW(empty[0], std::out_of_range);
}

TEST_F(List_of_10_items, ConstReverseReturnsIndependentCopy)
{
  const DynDlist<int> &cref = list10;
  auto reversed = cref.reverse();

  int expected_desc = 10;
  for (auto it = reversed.get_it(); it.has_curr(); it.next(), --expected_desc)
    EXPECT_EQ(it.get_curr(), expected_desc);
  EXPECT_EQ(expected_desc, 0);

  int expected_orig = 1;
  for (auto it = cref.get_it(); it.has_curr(); it.next(), ++expected_orig)
    EXPECT_EQ(it.get_curr(), expected_orig);
  EXPECT_EQ(expected_orig, 11);

  auto reversed_alias = cref.rev();
  int expected_desc_alias = 10;
  for (auto it = reversed_alias.get_it(); it.has_curr(); it.next(), --expected_desc_alias)
    EXPECT_EQ(it.get_curr(), expected_desc_alias);
  EXPECT_EQ(expected_desc_alias, 0);
}

TEST(DynDlist, IteratorInsertAndAppendOperations)
{
  DynDlist<int> list = {1, 3};
  auto it = list.get_it();
  ASSERT_TRUE(it.has_curr());

  it.insert(2);
  int expected_after_insert[] = {1, 2, 3};
  int idx = 0;
  for (auto iter = list.get_it(); iter.has_curr(); iter.next(), ++idx)
    EXPECT_EQ(iter.get_curr(), expected_after_insert[idx]);

  it.reset_first();
  it.append(0);
  int expected_after_append[] = {0, 1, 2, 3};
  idx = 0;
  for (auto iter = list.get_it(); iter.has_curr(); iter.next(), ++idx)
    EXPECT_EQ(iter.get_curr(), expected_after_append[idx]);

  auto at_end = list.get_it();
  at_end.end();
  EXPECT_THROW(at_end.insert(42), std::overflow_error);
  EXPECT_THROW(at_end.append(42), std::overflow_error);
}

TEST(DynDlist, IteratorListOperations)
{
  DynDlist<int> base = {1, 4};
  DynDlist<int> middle = {2, 3};
  DynDlist<int> head = {-1, 0};

  auto it = base.get_it();
  ASSERT_TRUE(it.has_curr());
  it.insert_list(middle);
  EXPECT_TRUE(middle.is_empty());

  int expected_after_insert_list[] = {1, 2, 3, 4};
  int idx = 0;
  for (auto iter = base.get_it(); iter.has_curr(); iter.next(), ++idx)
    EXPECT_EQ(iter.get_curr(), expected_after_insert_list[idx]);

  it.reset_first();
  it.append_list(head);
  EXPECT_TRUE(head.is_empty());

  int expected_after_append_list[] = {-1, 0, 1, 2, 3, 4};
  idx = 0;
  for (auto iter = base.get_it(); iter.has_curr(); iter.next(), ++idx)
    EXPECT_EQ(iter.get_curr(), expected_after_append_list[idx]);
}

TEST(DynDlist, IteratorListOperationsRequireCurrent)
{
  DynDlist<int> base = {1, 2, 3};
  DynDlist<int> extra = {4, 5};

  auto it = base.get_it();
  it.end();
  EXPECT_THROW(it.insert_list(extra), std::overflow_error);
  EXPECT_THROW(it.append_list(extra), std::overflow_error);

  // ni la lista base ni la extra deberían alterarse tras los errores
  int expected_base[] = {1, 2, 3};
  int idx = 0;
  for (auto iter = base.get_it(); iter.has_curr(); iter.next(), ++idx)
    EXPECT_EQ(iter.get_curr(), expected_base[idx]);
  EXPECT_FALSE(extra.is_empty());
  EXPECT_EQ(extra.size(), 2u);
}

TEST(DynDlist, SplitRequiresEmptyDestination)
{
  DynDlist<int> source = {1, 2, 3};
  DynDlist<int> left;
  DynDlist<int> right;

  left.append(42);
  EXPECT_THROW(source.split(left, right), std::domain_error);

  left.empty();
  right.append(99);
  EXPECT_THROW(source.split(left, right), std::domain_error);
}

TEST(DynDlist, StackAndQueueAliases)
{
  DynDlist<int> list;
  list.push(3);
  list.push(2);
  list.push(1);

  EXPECT_EQ(list.top(), 1);
  EXPECT_EQ(list.pop(), 1);
  EXPECT_EQ(list.top(), 2);

  list.put(99);
  EXPECT_EQ(list.rear(), 99);
  EXPECT_EQ(list.front(), 2);
  EXPECT_EQ(list.get(), 2);
  EXPECT_EQ(list.front(), 3);

  list.pop();
  list.pop();
  EXPECT_TRUE(list.is_empty());
  EXPECT_THROW(list.pop(), std::underflow_error);
}
