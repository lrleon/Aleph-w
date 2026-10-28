
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
 * @file generate_tree.cc
 * @brief Tests for Generate Tree
 */
# include <gtest/gtest.h>

# include <algorithm>

# include <generate_tree.H>
# include "tree-node-common.H"

using namespace std;
using namespace testing;
using namespace Aleph;

TEST_F(Simple_Tree, Generate_ntreepic)
{
  const string result =
    "Root \"0\" \n"				\
    "Node 0 \"1\" \n"				\
    "Node 0.0 \"6\" \n"				\
    "Node 0.0 \"7\" \n"				\
    "Node 0.0 \"8\" \n"				\
    "Node 0.0 \"9\" \n"				\
    "Node 0.0 \"10\" \n"			\
    "Node 0 \"2\" \n"				\
    "Node 0.1 \"11\" \n"			\
    "Node 0.1 \"12\" \n"			\
    "Node 0.1 \"13\" \n"			\
    "Node 0.1 \"14\" \n"			\
    "Node 0.1 \"15\" \n"			\
    "Node 0 \"3\" \n"				\
    "Node 0.2 \"16\" \n"			\
    "Node 0.2 \"17\" \n"			\
    "Node 0.2 \"18\" \n"			\
    "Node 0.2 \"19\" \n"			\
    "Node 0.2 \"20\" \n"			\
    "Node 0 \"4\" \n"				\
    "Node 0.3 \"21\" \n"			\
    "Node 0.3 \"22\" \n"			\
    "Node 0.3 \"23\" \n"			\
    "Node 0.3 \"24\" \n"			\
    "Node 0.3 \"25\" \n"			\
    "Node 0 \"5\" \n"				\
    "Node 0.4 \"26\" \n"			\
    "Node 0.4 \"27\" \n"			\
    "Node 0.4 \"28\" \n"			\
    "Node 0.4 \"29\" \n"			\
    "Node 0.4 \"30\" \n";

  ostringstream ss;
  generate_tree(root, ss);

  ASSERT_EQ(ss.str(), result);
}

// A chain of n nodes, a tree of n levels
static Tree_Node<int> * chain(size_t n)
{
  auto root = new Tree_Node<int>(0);
  auto last = root;
  for (size_t i = 1; i < n; ++i)
    {
      auto p = new Tree_Node<int>(int(i));
      last->insert_rightmost_child(p);
      last = p;
    }
  return root;
}

// generate_tree() refused the trees of more than Max_Tree_Node_Depth levels
TEST(Generate_Tree, a_tree_of_any_depth_is_written)
{
  for (const size_t n : {Max_Tree_Node_Depth, Max_Tree_Node_Depth + 1, size_t(3000)})
    {
      Tree_Node<int> * deep = chain(n);
      ostringstream out;
      EXPECT_NO_THROW(generate_tree(deep, out));

      // a line for each node; that of the deepest one has the Dewey number
      // of its parent, of n - 1 parts
      const string s = out.str();
      EXPECT_EQ(size_t(count(s.begin(), s.end(), '\n')), n);
      const size_t last = s.rfind("Node ");
      ASSERT_NE(last, string::npos);
      const string dewey = s.substr(last + 5, s.find(' ', last + 5) - last - 5);
      EXPECT_EQ(size_t(count(dewey.begin(), dewey.end(), '.')), n - 2);

      destroy_tree(deep);
    }
}

// generate_tree_graphviz() recursed as deep as the tree
TEST(Generate_Tree, graphviz_of_a_deep_tree_needs_no_recursion)
{
  constexpr size_t n = 100000;
  Tree_Node<int> * deep = chain(n);
  ostringstream out;
  generate_tree_graphviz(deep, out);

  // three lines of header, a label for each node, an arc for each one but
  // the root, and the closing brace
  const string s = out.str();
  EXPECT_EQ(size_t(count(s.begin(), s.end(), '\n')), 3 + n + (n - 1) + 1);
  EXPECT_EQ(s.substr(s.size() - 2), "}\n");

  destroy_tree(deep);
}

