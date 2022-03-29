

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

struct List_of_5_nodes : public Test
{
  Snodenc<int> list;
  List_of_5_nodes()
  {
    list.insert(new Snodenc<int>(5));
    list.insert(new Snodenc<int>(4));
    list.insert(new Snodenc<int>(3));
    list.insert(new Snodenc<int>(2));
    list.insert(new Snodenc<int>(1));
  }
  ~List_of_5_nodes()
  {
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
    delete list.remove_next();
  }
};

TEST(Snodenc, conversion_from_slinknc)
{
  {
    Snodenc<int> node = 10;
    Slinknc * ptr = &node;
    EXPECT_THAT(ptr->to_snodenc<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }

  {
    const Snodenc<int> node = 10;
    const Slinknc * ptr = &node;
    EXPECT_THAT(ptr->to_snodenc<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }
}

TEST_F(List_of_5_nodes, Basic_operations)
{
  EXPECT_THAT(list.get_next()->get_data(), 1);

  int i = 1;
  for (Snodenc<int>::Iterator it = list; it.has_curr(); it.next())
    {
      auto ptr = static_cast<Snodenc<int>*>(it.get_curr());
      EXPECT_EQ(ptr->get_data(), i++);
    }

  auto ptr = list.remove_first();
  EXPECT_EQ(ptr->get_data(), 1);
  EXPECT_TRUE(ptr->is_empty());
  list.insert(ptr);
  EXPECT_EQ(list.get_next()->get_data(), 1);
}
