
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

# include <tpl_tree_node.H>
# include <ah-zip.H>
# include <ah-string-utils.H>

# include "tree-node-common.H"

using namespace std;
using namespace Aleph;

TEST(Tree_Node, on_isolated_node)
{
  Tree_Node<int> p;

  ASSERT_TRUE(p.is_root());
  ASSERT_TRUE(p.is_leaf());
  ASSERT_TRUE(p.is_leftmost());
  ASSERT_TRUE(p.is_rightmost());
  ASSERT_TRUE(p.get_child_list()->is_empty());
  ASSERT_TRUE(p.get_sibling_list()->is_empty());

  ASSERT_EQ(p.get_right_child(), nullptr);
  ASSERT_EQ(p.get_left_child(), nullptr);

  ASSERT_EQ(p.get_child(0), nullptr);
  ASSERT_EQ(p.get_parent(), nullptr);

  ASSERT_EQ(p.get_left_tree(), nullptr);
  ASSERT_EQ(p.get_right_tree(), nullptr);

  ASSERT_TRUE(p.children().is_empty());

  ASSERT_TRUE(p.traverse([] (auto) { return true; }));
  ASSERT_FALSE(p.traverse([] (auto) { return false; }));

  Tree_Node<int>::Iterator it = p.get_it();
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &p);
  ASSERT_NO_THROW(it.next());
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);
  it.reset_first();
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr(), &p);
  ASSERT_NO_THROW(it.next());
  ASSERT_FALSE(it.has_curr());
  ASSERT_THROW(it.get_curr(), overflow_error);
  ASSERT_THROW(it.next(), overflow_error);

  Tree_Node<int>::Children_Iterator cit(p);
  ASSERT_FALSE(cit.has_curr());
  ASSERT_THROW(cit.get_curr(), overflow_error);
  ASSERT_THROW(cit.next(), overflow_error);
}

TEST(Tree_Node, simple_tree_construction_and_destruction)
{
  Tree_Node<int> p1 = 1;
  Tree_Node<int> p2 = 2;
  Tree_Node<int> p3 = 3;
  Tree_Node<int> p4 = 4;
  Tree_Node<int> p5 = 5;

  /* 1  insert_leftmost_child() test
     |
     2
  */
  p1.insert_leftmost_child(&p2); 
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_TRUE(p2.is_rightmost());

  /*     1   insert_rightmost_child() test
	/\
       2 3
   */
  p1.insert_rightmost_child(&p3);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_TRUE(p3.is_rightmost());

  /*        0
          / | \
         2  3 5
   */
  p3.insert_right_sibling(&p5);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());
  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_FALSE(p3.is_rightmost());
  ASSERT_TRUE(p3.is_leaf());
  ASSERT_FALSE(p5.is_leftmost());
  ASSERT_TRUE(p5.is_rightmost());
  ASSERT_FALSE(p5.is_root());
  ASSERT_TRUE(p5.is_leaf());

  /*            1
           /  /  |  | 
	   2 3   4  5
   */
  p5.insert_left_sibling(&p4);
  ASSERT_TRUE(p1.is_root());
  ASSERT_TRUE(p1.is_rightmost());
  ASSERT_TRUE(p1.is_leftmost());
  ASSERT_FALSE(p1.is_leaf());
  ASSERT_FALSE(p2.is_root());
  ASSERT_TRUE(p2.is_leftmost());
  ASSERT_FALSE(p2.is_rightmost());
  ASSERT_TRUE(p2.is_leaf());

  ASSERT_FALSE(p3.is_root());
  ASSERT_FALSE(p3.is_leftmost());
  ASSERT_FALSE(p3.is_rightmost());
  ASSERT_TRUE(p3.is_leaf());

  ASSERT_FALSE(p4.is_root());
  ASSERT_FALSE(p4.is_leftmost());
  ASSERT_FALSE(p4.is_rightmost());
  ASSERT_TRUE(p4.is_leaf());

  ASSERT_FALSE(p5.is_leftmost());
  ASSERT_TRUE(p5.is_rightmost());
  ASSERT_FALSE(p5.is_root());
  ASSERT_TRUE(p5.is_leaf());

  ASSERT_EQ(p1.get_left_child(), &p2);
  ASSERT_EQ(p1.get_right_child(), &p5);

  ASSERT_EQ(p2.get_left_sibling(), nullptr);
  ASSERT_EQ(p2.get_right_sibling(), &p3);

  ASSERT_EQ(p3.get_left_sibling(), &p2);
  ASSERT_EQ(p3.get_right_sibling(), &p4);

  ASSERT_EQ(p4.get_left_sibling(), &p3);
  ASSERT_EQ(p4.get_right_sibling(), &p5);

  ASSERT_EQ(p5.get_left_sibling(), &p4);
  ASSERT_EQ(p5.get_right_sibling(), nullptr);

  ASSERT_EQ(p1.get_child(0), &p2);
  ASSERT_EQ(p1.get_child(1), &p3);
  ASSERT_EQ(p1.get_child(2), &p4);
  ASSERT_EQ(p1.get_child(3), &p5);

  int k = 0;
  ASSERT_TRUE(p1.traverse([&k] (auto p) { return p->get_key() == ++k; }));
  ASSERT_EQ(k, 5);
  k = 1;
  ASSERT_TRUE(p1.children_nodes().traverse([&k] (auto p)
					   { return p->get_key() == ++k; }));
  ASSERT_EQ(k, 5);
  k = 1;
  ASSERT_TRUE(p1.children().traverse([&k] (auto i) { return i == ++k; }));
  ASSERT_EQ(k, 5);
}

TEST(Tree_Node, Iterator_on_extreme_cases)
{
  {
    Tree_Node<int>::Iterator it = nullptr;
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }

  {
    Tree_Node<int> p(0);
    auto it = p.get_it();
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_pos(), 0);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_EQ(it.get_pos(), 1);
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }

  {
    Tree_Node<int> p0(0);
    Tree_Node<int> p1(1);
    p0.insert_leftmost_child(&p1);
    auto it = p0.get_it();
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p0);
    ASSERT_NO_THROW(it.next());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p1);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);

    ASSERT_NO_THROW(it.reset_first());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p0);
    ASSERT_NO_THROW(it.next());
    ASSERT_TRUE(it.has_curr());
    ASSERT_EQ(it.get_curr(), &p1);
    ASSERT_NO_THROW(it.next());
    ASSERT_FALSE(it.has_curr());
    ASSERT_THROW(it.get_curr(), overflow_error);
    ASSERT_THROW(it.next(), overflow_error);
  }
}

TEST_F(Simple_Tree, Iterators)
{
  auto itl = l.get_it();
  size_t k = 0;
  for (auto it = root->get_it(); it.has_curr(); it.next(), itl.next(), ++k)
    ASSERT_EQ(it.get_curr()->get_key(), itl.get_curr());
  ASSERT_GT(k, 0);
}

TEST(Tree_Node, clone_on_extreme_cases)
{
  {
    Tree_Node<int> * root = nullptr;
    ASSERT_EQ(clone_tree(root), nullptr);
  }
  {
    Tree_Node<int> root(5);
    auto rootp = clone_tree(&root);
    ASSERT_NE(rootp, nullptr);
    ASSERT_EQ(root.get_key(), rootp->get_key());
    destroy_tree(rootp);
  }
}

TEST_F(Simple_Tree, level_traversal)
{
  int i = 0;
  root->level_traverse([&i] (auto p)
		       {
			 return p->get_key() == i++;
		       });
  ASSERT_EQ(i, 31);
}

TEST_F(Simple_Tree, clone)
{
  Tree_Node<int> * clone = clone_tree(root);
  using It = Tree_Node<int>::Iterator;
  using Pit = Pair_Iterator<It>;
  for (Pit it{It(root), It(clone)}; it.has_curr(); it.next())
    {
      auto p = it.get_curr();
      ASSERT_EQ(p.first->get_key(), p.second->get_key());
    }
  destroy_tree(clone);
}

TEST(Tree_Node, traverse_on_extreme_cases)
{
  {
    Tree_Node<int> * root = nullptr;
    ASSERT_TRUE(root->traverse([] (auto) { return false; }));
  }
  {
    Tree_Node<int> root(5);
    size_t k = 0;
    ASSERT_TRUE(root.traverse([&k] (auto p) { k++; return p->get_key() == 5; }));
    ASSERT_EQ(k, 1);
  }
}

TEST_F(Simple_Tree, traverse)
{
  auto it = l.get_it();
  size_t k = 0;
  auto ret = root->traverse([&it, &k] (auto p)
    {
      bool r = p->get_key() == it.get_curr();
      it.next(); ++k;
      return r;
    });
  ASSERT_TRUE(ret);
  ASSERT_EQ(k, l.size());
}

TEST_F(Simple_Tree, deway)
{
  int d[100];
  size_t sz;
  auto p = search_deway(root, 14, d, 100, sz);
  ASSERT_EQ(p->get_key(), 14);
  ASSERT_EQ(sz, 3);
  ASSERT_EQ(d[0], 0);
  ASSERT_EQ(d[1], 1);
  ASSERT_EQ(d[2], 3);
}

/*                                    0
 
      1              2                3                 4                    5

6 7 8 9 10     11 12 13 14 15  16 17 18 19 20     21 22 23 24 25 26   27 28 29 30 31

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


