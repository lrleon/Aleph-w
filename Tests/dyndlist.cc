
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
