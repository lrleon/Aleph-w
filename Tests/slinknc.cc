

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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


# include <gmock/gmock.h>

# include <htlist.H>

using namespace testing;

TEST(Slinknc, Initialized_in_nullptr)
{
  Slinknc l;
  EXPECT_THAT(l.get_next(), nullptr);
}

TEST(Slinknc, Basic_operations)
{
  Slinknc l;
  EXPECT_TRUE(l.is_empty());
  EXPECT_THAT(l.get_next(), nullptr);

  Slinknc n1;
  l.insert(&n1);
  EXPECT_THAT(l.get_next(), &n1);

  Slinknc c1(l);
  EXPECT_THAT(c1.get_next(), nullptr);

  Slinknc c2;
  EXPECT_THAT(c2.get_next(), nullptr);
  c2 = l;
  EXPECT_THAT(c2.get_next(), nullptr);

  Slinknc n2;
  l.insert(&n2);
  EXPECT_THAT(l.get_next(), &n2);
  EXPECT_THAT(l.remove_next(), &n2);
  EXPECT_THAT(l.get_next(), &n1);
  EXPECT_THAT(l.remove_next(), &n1);
  EXPECT_THAT(l.get_next(), nullptr);
}

struct Slinknc_of_5_items : public testing::Test
{
  Slinknc list;
  Slinknc n1, n2, n3, n4, n5;
  Slinknc_of_5_items()
  {
    list.insert(&n5);
    list.insert(&n4);
    list.insert(&n3);
    list.insert(&n2);
    list.insert(&n1);    
  }
};

TEST_F(Slinknc_of_5_items, Complex_operations)
{
  EXPECT_FALSE(list.is_empty());
  EXPECT_THAT(list.get_next(), &n1);
  EXPECT_THAT(list.get_next()->get_next(), &n2);
  EXPECT_THAT(list.get_next()->get_next()->get_next(), &n3);
  EXPECT_THAT(list.get_next()->get_next()->get_next()->get_next(), &n4);
  EXPECT_THAT(list.get_next()->get_next()->get_next()->get_next()->get_next(), &n5);

  Slinknc laux = list;
  EXPECT_TRUE(laux.is_empty());
  EXPECT_THAT(laux.get_next(), nullptr);
  laux = list;
  EXPECT_THAT(laux.get_next(), nullptr);

  Slinknc n6;
  list.insert(&n6);
  EXPECT_THAT(list.get_next(), &n6);
  EXPECT_FALSE(list.is_empty());
  
  EXPECT_THAT(list.remove_next(), &n6);
  EXPECT_TRUE(n6.is_empty());
  EXPECT_FALSE(list.is_empty());

  EXPECT_THAT(list.remove_next(), &n1);
  EXPECT_TRUE(n1.is_empty());
  EXPECT_FALSE(list.is_empty());

  EXPECT_THAT(list.remove_next(), &n2);
  EXPECT_TRUE(n2.is_empty());
  EXPECT_FALSE(list.is_empty());

  EXPECT_THAT(list.remove_next(), &n3);
  EXPECT_TRUE(n3.is_empty());
  EXPECT_FALSE(list.is_empty());

  EXPECT_THAT(list.remove_next(), &n4);
  EXPECT_TRUE(n4.is_empty());
  EXPECT_FALSE(list.is_empty());

  EXPECT_THAT(list.remove_next(), &n5);
  EXPECT_TRUE(n5.is_empty());
  EXPECT_TRUE(list.is_empty());
}
  
TEST(Slinknc, Iterator_on_empty)
{
  Slinknc l;
  Slinknc::Iterator it(l);
  EXPECT_THAT(it.has_curr(), false);
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

TEST_F(Slinknc_of_5_items, Iterator_operations)
{
  Slinknc::Iterator it = list;

  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n1);
  it.next();

  it.reset_first();
  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n1);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n2);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n3);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n4);
  it.next();
  
  EXPECT_TRUE(it.has_curr());
  EXPECT_THAT(it.get_curr(), &n5);
  it.next();
  
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

