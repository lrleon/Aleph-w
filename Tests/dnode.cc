
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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

# include <tpl_dnode.H>

using namespace testing;

TEST(Dnode, conversion_from_slinknc)
{
  {
    Dnode<int> node = 10;
    Dlink * ptr = &node;
    EXPECT_THAT(ptr->to_dnode<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }

  {
    const Dnode<int> node = 10;
    const Dlink * ptr = &node;
    EXPECT_THAT(ptr->to_dnode<int>(), &node);
    EXPECT_THAT(ptr->to_data<int>(), 10);
  }
}

struct List_of_5_nodes : public Test
{
  Dnode<int> list;
  List_of_5_nodes()
  {
    list.insert(new Dnode<int>(5));
    list.insert(new Dnode<int>(4));
    list.insert(new Dnode<int>(3));
    list.insert(new Dnode<int>(2));
    list.insert(new Dnode<int>(1));
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

TEST_F(List_of_5_nodes, Basic_operations)
{
  EXPECT_THAT(list.get_next()->get_data(), 1);
  EXPECT_THAT(list.get_prev()->get_data(), 5);

  int i = 1;
  for (Dnode<int>::Iterator it = list; it.has_curr(); it.next(), ++i)
    EXPECT_EQ(it.get_curr()->to_data<int>(), i);

  auto ptr = list.remove_first();
  EXPECT_EQ(ptr->get_data(), 1);
  EXPECT_TRUE(ptr->is_empty());
  list.insert(ptr);
  EXPECT_EQ(list.get_next()->get_data(), 1);
}
