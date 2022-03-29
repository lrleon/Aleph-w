

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


TEST_F(Three_Trees, insertion_of_trees)
{
  auto t1 = clone_tree(root1); 
  auto t2 = clone_tree(root2);
  auto t3 = clone_tree(root3);

  t1->insert_tree_to_right(t3);
  t1->insert_tree_to_right(t2);

  DynList<Tree_Node<int>*> tlist = { t1, t2, t3 };
  DynList<Tree_Node<int>*> flist = t1->trees();

  zip_for_each([] (auto t) { ASSERT_EQ(get<0>(t), get<1>(t)); }, tlist, flist);
  
  destroy_forest(t1);
}

TEST_F(Three_Trees, join)
{
  auto t = clone_tree(root1);
  auto t2 = clone_tree(root2);

  t->join(t2);

  DynList<int> l = { 0, 1, 2, 3, 4, 5, 31, 6, 7, 8, 9, 10, 11, 12, 13, 14, 15,
		     16, 17, 18, 19, 20, 21, 22, 23, 24, 25, 26, 27, 28, 29,
		     30, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 };

  DynList<int> order;
  t->level_traverse([&order] (auto p) { order.append(p->get_key()); return true; });

  ASSERT_TRUE(eq(l, order));

  destroy_forest(t);
}


