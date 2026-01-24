

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


# ifndef TREE_NODE_COMMON_H
# define TREE_NODE_COMMON_H

# include <gtest/gtest.h>

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
