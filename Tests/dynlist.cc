
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
 * @file dynlist.cc
 * @brief Tests for Dynlist
 */
# include <gtest/gtest.h>

# include <htlist.H>
# include <ah-unique.H>

using namespace testing;
using namespace Aleph;

struct List_of_25_items : public Test
{
  size_t n = 0;
  DynList<int> list;
  DynList<int> rlist;
  List_of_25_items()
  {
    for (size_t i = 0; i < 25; ++i, ++n)
      list.append(i + 1);

    const DynList<int> l = list;
    rlist = l.rev();
  }
};

TEST(DynList, Basic_operations)
{
  DynList<int> list;
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

TEST(DynList, Simple_append_and_insert_of_list)
{
  DynList<int> laux, list;
  laux.insert(2);
  list.append(std::move(laux));

  EXPECT_TRUE(laux.is_empty());
  EXPECT_TRUE(list.is_unitarian());

  laux.insert(1);
  list.insert(std::move(laux));
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(list.size(), 2);
  EXPECT_EQ(list.get_first(), 1);
  EXPECT_EQ(list.get_last(), 2);
}

TEST(DynListAlgorithms, InPlaceUnique)
{
  DynList<int> list;
  list.append(1);
  list.append(2);
  list.append(1);
  list.append(3);
  list.append(2);
  list.append(4);
  list.append(4);

  in_place_unique(list);

  ASSERT_EQ(list.size(), 4u);
  auto it = list.get_it();
  ASSERT_TRUE(it.has_curr()); EXPECT_EQ(it.get_curr(), 1); it.next();
  ASSERT_TRUE(it.has_curr()); EXPECT_EQ(it.get_curr(), 2); it.next();
  ASSERT_TRUE(it.has_curr()); EXPECT_EQ(it.get_curr(), 3); it.next();
  ASSERT_TRUE(it.has_curr()); EXPECT_EQ(it.get_curr(), 4); it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST_F(List_of_25_items, Basic_operations)
{
  EXPECT_EQ(list.get_first(), 1);
  EXPECT_EQ(list.get_last(), 25);
  EXPECT_EQ(list.size(), 25);
  EXPECT_FALSE(list.is_empty());
  EXPECT_FALSE(list.is_unitarian());
  EXPECT_FALSE(list.is_unitarian_or_empty());
}

TEST_F(List_of_25_items, Iterator_operations)
{
  int i = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++i)
    {
      EXPECT_EQ(it.get_curr(), i);
      EXPECT_EQ(it.get_pos(), i - 1);
    }
}

TEST_F(List_of_25_items, Split_and_concat)
{
  DynList<int> l, r;
  list.split(l, r);

  EXPECT_TRUE(list.is_empty());
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

  list.append(r);
  list.insert(l);
  i = 1;
  for (auto it = l.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, swap)
{
  DynList<int> laux;
  laux.swap(list);

  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(list.size(), 0);
  EXPECT_FALSE(laux.is_empty());

  int i = 1;
  for (auto it = laux.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, reverse)
{
  list.reverse();
  int i = 25;
  for (auto it = list.get_it(); it.has_curr(); it.next(), --i)
    EXPECT_EQ(it.get_curr(), i);

  list.reverse();

  i = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);

  const DynList<int> list_cpy = list;
  const DynList<int> rlist_cpy = rlist;

  EXPECT_TRUE(eq(list_cpy.rev(), rlist));
  EXPECT_TRUE(eq(rlist_cpy.rev(), list));

  DynList<int> l, r;
  list.split(l, r);

  EXPECT_TRUE(list.is_empty());

  l.reverse();
  r.reverse();
  list.insert(l);
  list.insert(r);

  list.reverse();
  i = 1;
  for (HTList::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);
  
}

TEST_F(List_of_25_items, rotate_left)
{
  list.rotate_left(3);

  {
    auto it = list.get_it();
    for (int i = 0, n = 4; i < 3; ++i, ++n, it.next())
      EXPECT_EQ(it.get_curr(), n);
  }
  
  list.rotate_left(22);
  int i = 1;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST_F(List_of_25_items, append_and_insert_by_moving)
{
  DynList<int> ll = { 0, -1, -2, -3, -4, -5, -6, -7, -8, -9 };
  DynList<int> lg = { 26, 27, 28, 29, 30, 31, 32, 33, 34, 35 };

  ll.reverse();

  list.insert(std::move(ll));
  list.append(std::move(lg));

  EXPECT_TRUE(ll.is_empty());
  EXPECT_TRUE(lg.is_empty());

  int i = -9;
  for (auto it = list.get_it(); it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr(), i);
}

TEST(DynList, traverse_on_empty_container)
{
  DynList<int> m;
  size_t n = 0;
  auto ret = m.traverse([&n] (int) { ++n; return true; });
  EXPECT_TRUE(ret);
  EXPECT_EQ(n, 0);
}

TEST_F(List_of_25_items, traverse)
{
  EXPECT_TRUE(list.size() > 0);
  EXPECT_EQ(list.size(), n);
  int N = 0;
  auto ret = list.traverse([&N, this] (int i) { ++N; return i < n/2; });
  EXPECT_FALSE(ret);
  EXPECT_EQ(N, n/2);
}
