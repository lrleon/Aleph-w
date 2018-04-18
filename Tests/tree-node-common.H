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

# ifndef TREE_NODE_COMMON_H
# define TREE_NODE_COMMON_H

# include <gmock/gmock.h>

# include <tpl_tree_node.H>

using namespace testing;


/* Build:
                                    0
 
      1              2                3                 4                    5

6 7 8 9 10     11 12 13 14 15  16 17 18 19 20     21 22 23 24 25 26   27 28 29 30 31

*/
Tree_Node<int> * sample_tree(size_t num_nodes_by_subtree, int key)
{
  Tree_Node<int> * root = new Tree_Node<int>(key++);
  for (size_t i = 0; i < num_nodes_by_subtree; ++i, ++key)
    root->insert_rightmost_child(new Tree_Node<int>(key));

  for (Tree_Node<int>::Children_Iterator it(root); it.has_curr(); it.next())
    {
      auto ptr = it.get_curr();
      for (size_t i = 0; i < num_nodes_by_subtree; ++i, ++key)
	ptr->insert_rightmost_child(new Tree_Node<int>(key));
    }
  return root;
}


// preorder traversal of sample_tree(5, 0)
DynList<int> l = { 0, 1, 6, 7, 8, 9, 10, 2, 11, 12, 13, 14, 15, 3, 16, 17,
		   18, 19, 20, 4, 21, 22, 23, 24, 25, 5, 26, 27, 28, 29, 30 };

struct Simple_Tree : public Test
{
  Tree_Node<int> * root = nullptr;
  Simple_Tree() : root(sample_tree(5, 0)) {}
  ~Simple_Tree() { destroy_tree(root); }
};


struct Three_Trees : public Test
{
  Tree_Node<int> * root1 = nullptr;
  Tree_Node<int> * root2 = nullptr;
  Tree_Node<int> * root3 = nullptr;

  const DynList<int> l1 = { 0, 1, 6, 7, 8, 9, 10, 2, 11, 12, 13, 14, 15, 3,
			    16, 17, 18, 19, 20, 4, 21, 22, 23, 24, 25, 5, 26,
			    27, 28, 29, 30 };
  
  const DynList<int> l2 = { 31, 32, 33, 34, 35, 36, 37, 38, 39, 40, 41, 42, 43 };

  const DynList<int> l3 = { 44, 45, 46, 47, 48, 49, 50, 51, 52, 53, 54, 55, 56,
			    57, 58, 59, 60, 61, 62, 63, 64 };

  Three_Trees()
    : root1(sample_tree(5, 0)), root2(sample_tree(3, 31)),
      root3(sample_tree(4, 44)) {}
  ~Three_Trees()
  {
    destroy_tree(root1);
    destroy_tree(root2);
    destroy_tree(root3);
  }
};


# endif
