/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# include <gmock/gmock.h>

# include <dlink.H>

using namespace std;
using namespace testing;

TEST(Dlink, Well_initialized)
{
  Dlink l;
  EXPECT_TRUE(l.is_empty());
  EXPECT_EQ(l.get_next(), &l);
  EXPECT_EQ(l.get_prev(), &l);
}

TEST(Dlink, Operations_with_empty_list)
{
  Dlink l, laux;
  l.swap(laux);                              // swap between empty list
  EXPECT_TRUE(l.is_empty());
  EXPECT_EQ(l.get_next(), &l);
  EXPECT_EQ(l.get_prev(), &l);
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(laux.get_next(), &laux);
  EXPECT_EQ(laux.get_prev(), &laux);

  l = move(laux);                           // assignment of rvalue empty list
  EXPECT_TRUE(l.is_empty());
  EXPECT_EQ(l.get_next(), &l);
  EXPECT_EQ(l.get_prev(), &l);
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(laux.get_next(), &laux);
  EXPECT_EQ(laux.get_prev(), &laux);

  l = laux;                            // assignment of empty list
  EXPECT_TRUE(l.is_empty());
  EXPECT_EQ(l.get_next(), &l);
  EXPECT_EQ(l.get_prev(), &l);
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(laux.get_next(), &laux);
  EXPECT_EQ(laux.get_prev(), &laux);

  l = laux;                            // assignment of rvalue empty list
  EXPECT_TRUE(l.is_empty());
  EXPECT_EQ(l.get_next(), &l);
  EXPECT_EQ(l.get_prev(), &l);
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(laux.get_next(), &laux);
  EXPECT_EQ(laux.get_prev(), &laux);

  {
    Dlink llaux(move(laux));       // copy constructor of empty rvalue list
    EXPECT_TRUE(laux.is_empty());
    EXPECT_EQ(laux.get_next(), &laux);
    EXPECT_EQ(laux.get_prev(), &laux);
    EXPECT_TRUE(llaux.is_empty());
    EXPECT_EQ(llaux.get_next(), &llaux);
    EXPECT_EQ(llaux.get_prev(), &llaux);
  }

  {
    Dlink llaux(laux);   // copy constructor of empty list
    EXPECT_TRUE(laux.is_empty());
    EXPECT_EQ(laux.get_next(), &laux);
    EXPECT_EQ(laux.get_prev(), &laux);
    EXPECT_TRUE(llaux.is_empty());
    EXPECT_EQ(llaux.get_next(), &llaux);
    EXPECT_EQ(llaux.get_prev(), &llaux);
  }

  l.append_list(&laux);
  EXPECT_TRUE(l.is_empty());
  EXPECT_TRUE(laux.is_empty());

  l.insert_list(&laux);
  EXPECT_TRUE(l.is_empty());
  EXPECT_TRUE(laux.is_empty());
}

TEST(Dlink, Basic_operations)
{
  Dlink l, l1, l2;

  l.reverse_list();               // revese of empty list
  EXPECT_TRUE(l.is_empty());

  l.append(&l2);
  EXPECT_FALSE(l.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_TRUE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l2);
  EXPECT_EQ(l.get_first(), l.get_next());
  EXPECT_EQ(l.get_last(), &l2);
  EXPECT_EQ(l.get_last(), l.get_prev());

  l.reverse_list();                // reverse with one item
  EXPECT_EQ(l.get_first(), &l2);
  EXPECT_EQ(l.get_first(), l.get_next());
  EXPECT_EQ(l.get_last(), &l2);
  EXPECT_EQ(l.get_last(), l.get_prev());
  

  l.insert(&l1);
  EXPECT_FALSE(l.is_unitarian());
  EXPECT_FALSE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l2);

  l.reverse_list();                // reverse of list of two items
  EXPECT_EQ(l.get_first(), &l2);
  EXPECT_EQ(l.get_last(), &l1);

  l.reverse_list();  

  EXPECT_EQ(l.remove_first(), &l1);
  EXPECT_EQ(l.get_first(), &l2);
  EXPECT_EQ(l.get_last(), &l2);
  EXPECT_FALSE(l.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_TRUE(l.is_unitarian_or_empty());

  l.insert(&l1);
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_FALSE(l.is_empty());
  EXPECT_FALSE(l.is_unitarian());
  EXPECT_FALSE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l2);

  EXPECT_EQ(l.remove_last(), &l2);
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l1);
  EXPECT_FALSE(l.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_TRUE(l.is_unitarian_or_empty());

  l.append(&l2);
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_FALSE(l.is_empty());
  EXPECT_FALSE(l.is_unitarian());
  EXPECT_FALSE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l2);

  l1.del();
  EXPECT_TRUE(l1.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_EQ(l.get_first(), &l2);

  l.insert(&l1);
  EXPECT_FALSE(l.is_empty());
  EXPECT_FALSE(l.is_unitarian());
  EXPECT_FALSE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l2);

  l2.del();
  EXPECT_TRUE(l2.is_empty());
  EXPECT_TRUE(l.is_unitarian());
  EXPECT_EQ(l.get_first(), &l1);

  l.append(&l2);
  EXPECT_FALSE(l.is_empty());
  EXPECT_FALSE(l.is_unitarian());
  EXPECT_FALSE(l.is_unitarian_or_empty());
  EXPECT_EQ(l.get_first(), &l1);
  EXPECT_EQ(l.get_last(), &l2);
}

TEST(Dlink, Very_simple_iterator_operations)
{
  Dlink l;
  Dlink::Iterator it = l;

  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_FALSE(it.is_last());
  EXPECT_FALSE(it.is_in_first());

  it.reset_first();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_FALSE(it.is_last());
  EXPECT_FALSE(it.is_in_first());

  it.reset_last();
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), overflow_error);
  EXPECT_FALSE(it.is_last());
  EXPECT_FALSE(it.is_in_first());
}

struct List_of_5_nodes : public testing::Test
{
  Dlink list, l1, l2, l3, l4, l5;
  List_of_5_nodes()
  {
    list.append(&l1);
    list.append(&l2);
    list.append(&l3);
    list.append(&l4);
    list.append(&l5);
  }
};

TEST_F(List_of_5_nodes, Iterator)
{
  Dlink::Iterator it = list;

  for (size_t i = 0; i < 2; ++i)
    {
      EXPECT_TRUE(it.has_curr());
      EXPECT_TRUE(it.is_in_first());
      EXPECT_EQ(it.get_curr(), &l1);
      it.next();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l2);
      it.next();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l3);
      it.next();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l4);
      EXPECT_FALSE(it.is_last());
      it.next();

      EXPECT_TRUE(it.has_curr());
      EXPECT_TRUE(it.is_in_last());
      EXPECT_EQ(it.get_curr(), &l5);
      EXPECT_TRUE(it.is_last());
      it.next();

      EXPECT_FALSE(it.has_curr());
      EXPECT_THROW(it.get_curr(), overflow_error);

      it.reset_first();
    }

  it.reset_last();
  for (size_t i = 0; i < 2; ++i)
    {
      EXPECT_TRUE(it.has_curr());
      EXPECT_TRUE(it.is_in_last());
      EXPECT_EQ(it.get_curr(), &l5);
      it.prev();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l4);
      it.prev();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l3);
      it.prev();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l2);
      EXPECT_FALSE(it.is_last());
      it.prev();

      EXPECT_TRUE(it.has_curr());
      EXPECT_EQ(it.get_curr(), &l1);
      EXPECT_TRUE(it.is_in_first());
      it.prev();

      EXPECT_FALSE(it.has_curr());
      EXPECT_THROW(it.get_curr(), overflow_error);

      it.reset_last();
    }

  it.reset_first();
  EXPECT_EQ(it.del(), &l1);
  EXPECT_TRUE(it.has_curr());

  EXPECT_EQ(it.del(), &l2);
  EXPECT_TRUE(it.has_curr());

  EXPECT_EQ(it.del(), &l3);
  EXPECT_TRUE(it.has_curr());

  EXPECT_EQ(it.del(), &l4);
  EXPECT_TRUE(it.has_curr());

  EXPECT_EQ(it.del(), &l5);
  EXPECT_FALSE(it.has_curr());
  EXPECT_TRUE(list.is_empty());
}

TEST_F(List_of_5_nodes, swap)
{
  Dlink laux;
  list.swap(&laux);

  EXPECT_TRUE(list.is_empty());
  EXPECT_FALSE(laux.is_empty());
  EXPECT_EQ(laux.get_first(), &l1);
  EXPECT_EQ(laux.get_last(), &l5);

  laux.swap(&list);

  EXPECT_TRUE(laux.is_empty());
  EXPECT_FALSE(list.is_empty());
  EXPECT_EQ(list.get_first(), &l1);
  EXPECT_EQ(list.get_last(), &l5);
}

TEST_F(List_of_5_nodes, append_insert_cut_splice_concat_of_lists)
{
  Dlink laux, n1, n2, n3;
  laux.append(&n1);
  laux.append(&n2);
  laux.append(&n3);                // laux = { n1, n2, n3 }

  list.append_list(&laux);         // l = { l1, l2, l3, l4, l5, n1, n2 ,n3 }
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(list.get_first(), &l1);
  EXPECT_EQ(list.get_last(), &n3);

  laux = list.cut_list(&n1);       // l = { l1, l2, l3, l4, l5 }, laux = { n1, n2, n3 }
  EXPECT_EQ(laux.get_first(), &n1);
  EXPECT_EQ(laux.get_last(), &n3);
  EXPECT_EQ(list.get_first(), &l1);
  EXPECT_EQ(list.get_last(), &l5);

  Dlink lr = list.cut_list(&l1);   // l = {}, lr = { l1, l2, l3, l4, l5 }
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(lr.get_first(), &l1);
  EXPECT_EQ(lr.get_last(), &l5);

  list.insert_list(&lr);            // list = { l1, l2, l3, l4, l5 }
  EXPECT_TRUE(lr.is_empty());
  EXPECT_EQ(list.get_first(), &l1);
  EXPECT_EQ(list.get_last(), &l5);
  EXPECT_FALSE(list.is_empty());

  list.insert_list(&laux);        // list = { n1, n2, n3, l1, l2, l3, l4, l5 }
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(list.get_first(), &n1);
  EXPECT_EQ(list.get_last(), &l5);

  Dlink n0;
  list.insert(&n0);     // list = { n0, n1, n2, n3, l1, l2, l3, l4, l5 }
  EXPECT_EQ(list.get_first(), &n0);
  EXPECT_EQ(list.get_last(), &l5);

  Dlink m1, m2, m3; // laux = { m1, m2, m3 }
  laux.append(&m1);
  laux.append(&m2);
  laux.append(&m3);
  EXPECT_EQ(laux.get_first(), &m1);
  EXPECT_EQ(laux.get_first()->get_next(), &m2);
  EXPECT_EQ(laux.get_last()->get_prev(), &m2);
  EXPECT_EQ(laux.get_last(), &m3);

  EXPECT_EQ(laux.remove_last(), &m3);
  EXPECT_EQ(laux.remove_first(), &m1);
  EXPECT_EQ(laux.remove_first(), &m2);
  laux.append(&m2);
  EXPECT_EQ(laux.get_first(), &m2);
  EXPECT_EQ(laux.get_last(), &m2);
  EXPECT_TRUE(laux.is_unitarian());
  EXPECT_EQ(laux.remove_last(), &m2);

  laux.append(&m1); // laux = { m1, m2, m3 }
  laux.append(&m2);
  laux.append(&m3);    

  n3.append_list(&laux); // list = { n0, n1, n2, n3, m1, m2, m3, l1, l2, l3, l4, l5 }
  EXPECT_TRUE(laux.is_empty());
  EXPECT_EQ(list.get_first(), &n0);
  EXPECT_EQ(list.get_first()->get_next()->get_next()->get_next(), &m1);
  EXPECT_EQ(list.get_last(), &l5);

  laux.append(m1.del());
  laux.append(m2.del());
  laux.append(m3.del());     // laux = { m1, m2, m3 }
  EXPECT_EQ(laux.get_first(), &m1);
  EXPECT_EQ(laux.get_first()->get_next(), &m2);
  EXPECT_EQ(laux.get_last(), &m3);
  EXPECT_EQ(laux.get_last()->get_prev(), &m2);
  EXPECT_EQ(m2.get_prev(), &m1);
  EXPECT_EQ(m2.get_next(), &m3);

  Dlink l, r;
  list.split_list(l, r); // l = { n0, n1, n2, n3, l1 } r = { l2, l3, l4, l5 }
  EXPECT_TRUE(list.is_empty());
  EXPECT_EQ(l.get_first(), &n0);
  EXPECT_EQ(l.get_last(), &l1);
  EXPECT_EQ(r.get_first(), &l2);
  EXPECT_EQ(r.get_last(), &l5);
}

TEST(Dlink, As_stack)
{
  Dlink stack;
  EXPECT_TRUE(stack.is_empty());
  EXPECT_THROW(stack.top(), underflow_error);
  EXPECT_THROW(stack.pop(), underflow_error);
  EXPECT_TRUE(stack.is_unitarian_or_empty());
  Dlink n1, n2, n3;
  stack.push(&n3);

  EXPECT_FALSE(stack.is_empty());
  EXPECT_TRUE(stack.is_unitarian());

  stack.push(&n2);
  stack.push(&n1);
  EXPECT_EQ(stack.top(), &n1);
  EXPECT_EQ(stack.pop(), &n1);
  EXPECT_EQ(stack.top(), &n2);
  EXPECT_EQ(stack.pop(), &n2);

  EXPECT_TRUE(stack.is_unitarian());

  EXPECT_EQ(stack.top(), &n3);
  EXPECT_EQ(stack.pop(), &n3);
  EXPECT_FALSE(stack.is_unitarian());
  EXPECT_TRUE(stack.is_empty());
}

TEST_F(List_of_5_nodes, reverse)
{
  list.reverse_list();
  EXPECT_EQ(list.get_first(), &l5);
  EXPECT_EQ(list.get_last(), &l1);
}
