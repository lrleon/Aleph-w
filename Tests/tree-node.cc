
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
 * @file tree-node.cc
 * @brief Tests for Tree Node
 */
# include <gtest/gtest.h>

# include <algorithm>
# include <map>
# include <random>
# include <stdexcept>
# include <vector>

# include <tpl_binNode.H>
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

TEST(Tree_Node, get_parent_tracks_multi_level_tree)
{
  Tree_Node<int> root(0);
  Tree_Node<int> left(1);
  Tree_Node<int> right(2);
  Tree_Node<int> left_left(3);
  Tree_Node<int> left_right(4);
  Tree_Node<int> deep(5);

  root.insert_rightmost_child(&left);
  root.insert_rightmost_child(&right);
  left.insert_rightmost_child(&left_left);
  left.insert_rightmost_child(&left_right);
  left_left.insert_rightmost_child(&deep);

  ASSERT_EQ(root.get_parent(), nullptr);
  ASSERT_EQ(left.get_parent(), &root);
  ASSERT_EQ(right.get_parent(), &root);
  ASSERT_EQ(left_left.get_parent(), &left);
  ASSERT_EQ(left_right.get_parent(), &left);
  ASSERT_EQ(deep.get_parent(), &left_left);
}

TEST(Tree_Node, insert_leftmost_child_preserves_existing_subtree_parent_links)
{
  Tree_Node<int> root(0);
  Tree_Node<int> old_left(1);
  Tree_Node<int> old_right(2);
  Tree_Node<int> old_left_child(3);
  Tree_Node<int> new_left(4);

  root.insert_rightmost_child(&old_left);
  root.insert_rightmost_child(&old_right);
  old_left.insert_rightmost_child(&old_left_child);

  root.insert_leftmost_child(&new_left);

  ASSERT_EQ(root.get_left_child(), &new_left);
  ASSERT_EQ(root.get_child(0), &new_left);
  ASSERT_EQ(root.get_child(1), &old_left);
  ASSERT_EQ(root.get_child(2), &old_right);

  ASSERT_EQ(new_left.get_parent(), &root);
  ASSERT_EQ(old_left.get_parent(), &root);
  ASSERT_EQ(old_right.get_parent(), &root);
  ASSERT_EQ(old_left_child.get_parent(), &old_left);

  ASSERT_TRUE(new_left.is_leftmost());
  ASSERT_FALSE(new_left.is_rightmost());
  ASSERT_FALSE(old_left.is_leftmost());
  ASSERT_FALSE(old_right.is_leftmost());
  ASSERT_TRUE(old_right.is_rightmost());
}

TEST(Tree_Node, insert_left_sibling_before_leftmost_child_updates_spine)
{
  Tree_Node<int> root(0);
  Tree_Node<int> old_left(1);
  Tree_Node<int> right(2);
  Tree_Node<int> old_left_child(3);
  Tree_Node<int> new_left(4);

  root.insert_rightmost_child(&old_left);
  root.insert_rightmost_child(&right);
  old_left.insert_rightmost_child(&old_left_child);

  old_left.insert_left_sibling(&new_left);

  ASSERT_EQ(root.get_left_child(), &new_left);
  ASSERT_EQ(root.get_child(0), &new_left);
  ASSERT_EQ(root.get_child(1), &old_left);
  ASSERT_EQ(root.get_child(2), &right);

  ASSERT_EQ(new_left.get_parent(), &root);
  ASSERT_EQ(old_left.get_parent(), &root);
  ASSERT_EQ(right.get_parent(), &root);
  ASSERT_EQ(old_left_child.get_parent(), &old_left);

  ASSERT_TRUE(new_left.is_leftmost());
  ASSERT_FALSE(new_left.is_rightmost());
  ASSERT_FALSE(old_left.is_leftmost());
  ASSERT_FALSE(right.is_leftmost());
  ASSERT_TRUE(right.is_rightmost());
}

TEST(Tree_Node, bin_to_forest_preserves_each_forest_root_flag)
{
  BinNode<int> n1(1);
  BinNode<int> n2(2);
  BinNode<int> n3(3);

  RLINK(&n1) = &n2;
  RLINK(&n2) = &n3;

  auto * forest = bin_to_forest<Tree_Node<int>, BinNode<int>>(&n1);
  ASSERT_NE(forest, nullptr);

  size_t count = 0;
  for (auto * t = forest; t != nullptr; t = t->get_right_sibling(), ++count)
    {
      EXPECT_TRUE(t->is_root()) << "tree key " << t->get_key()
                                << " must remain a forest root";
      EXPECT_EQ(t->get_parent(), nullptr);
    }

  EXPECT_EQ(count, 3);
  destroy_forest(forest);
}

TEST(Tree_Node, insert_right_sibling_on_root_links_forest_roots)
{
  Tree_Node<int> tree1(1);
  Tree_Node<int> tree2(2);
  Tree_Node<int> tree3(3);

  tree1.insert_right_sibling(&tree2);
  tree2.insert_right_sibling(&tree3);

  ASSERT_EQ(tree1.get_right_tree(), &tree2);
  ASSERT_EQ(tree2.get_right_tree(), &tree3);
  ASSERT_EQ(tree3.get_right_tree(), nullptr);

  ASSERT_TRUE(tree1.is_root());
  ASSERT_TRUE(tree2.is_root());
  ASSERT_TRUE(tree3.is_root());

  ASSERT_EQ(tree1.get_parent(), nullptr);
  ASSERT_EQ(tree2.get_parent(), nullptr);
  ASSERT_EQ(tree3.get_parent(), nullptr);
}

TEST(Tree_Node, unique_sibling_macro_tracks_sibling_links)
{
  Tree_Node<int> isolated(0);
  ASSERT_TRUE(IS_UNIQUE_SIBLING(&isolated));

  Tree_Node<int> root(1);
  Tree_Node<int> only_child(2);
  root.insert_rightmost_child(&only_child);
  ASSERT_TRUE(IS_UNIQUE_SIBLING(&only_child));

  Tree_Node<int> other_child(3);
  root.insert_rightmost_child(&other_child);
  ASSERT_FALSE(IS_UNIQUE_SIBLING(&only_child));
  ASSERT_FALSE(IS_UNIQUE_SIBLING(&other_child));

  Tree_Node<int> tree1(4);
  Tree_Node<int> tree2(5);
  tree1.insert_tree_to_right(&tree2);
  ASSERT_FALSE(IS_UNIQUE_SIBLING(&tree1));
  ASSERT_FALSE(IS_UNIQUE_SIBLING(&tree2));
}

TEST(Tree_Node, destroy_tree_on_non_leftmost_child_regression)
{
  Tree_Node<int>* root = new Tree_Node<int>(1);
  Tree_Node<int>* child1 = new Tree_Node<int>(2);
  Tree_Node<int>* child2 = new Tree_Node<int>(3);
  
  root->insert_rightmost_child(child1);
  root->insert_rightmost_child(child2);

  // Layout: child1 -> child2 (rightmost)
  // Destroying a non-leftmost sibling with a left sibling
  destroy_tree(child2);

  ASSERT_EQ(root->get_left_child(), child1);
  ASSERT_EQ(root->get_right_child(), child1);
  ASSERT_TRUE(IS_UNIQUE_SIBLING(child1));
  ASSERT_TRUE(child1->is_rightmost());
  ASSERT_EQ(child1->get_right_sibling(), nullptr);

  destroy_tree(root);
}

TEST(Tree_Node, destroy_tree_restores_is_rightmost_on_surviving_sibling)
{
  // Regression: destroy_tree() unlinked a node from its sibling list
  // without restoring is_rightmost()/is_leftmost() on the neighbor taking
  // its place. get_right_child()/get_left_child() use the raw (flag-blind)
  // Dlink links and so cannot detect this; get_right_sibling() and anything
  // built on it (for_each_child(), Children_Iterator) are flag-gated and
  // did loop forever on the surviving sibling before this fix.
  auto *root = new Tree_Node<int>(0);
  auto *a = new Tree_Node<int>(1);
  auto *b = new Tree_Node<int>(2);
  auto *c = new Tree_Node<int>(3);
  root->insert_rightmost_child(a);
  root->insert_rightmost_child(b);
  root->insert_rightmost_child(c);  // root -> a, b, c (c is rightmost)

  destroy_tree(c);

  EXPECT_TRUE(b->is_rightmost());
  EXPECT_EQ(b->get_right_sibling(), nullptr);

  int count = 0;
  root->for_each_child([&count](auto) { ++count; });
  EXPECT_EQ(count, 2);  // a, b -- would loop forever before the fix

  destroy_tree(root);
}

TEST(Tree_Node, destroy_tree_restores_is_leftmost_on_surviving_sibling)
{
  // Symmetric case: destroying the current leftmost child must promote
  // its former right neighbor to is_leftmost().
  auto *root = new Tree_Node<int>(0);
  auto *a = new Tree_Node<int>(1);
  auto *b = new Tree_Node<int>(2);
  auto *c = new Tree_Node<int>(3);
  root->insert_rightmost_child(a);
  root->insert_rightmost_child(b);
  root->insert_rightmost_child(c);  // root -> a (leftmost), b, c

  destroy_tree(a);

  EXPECT_TRUE(b->is_leftmost());
  EXPECT_EQ(b->get_left_sibling(), nullptr);
  EXPECT_EQ(root->get_left_child(), b);

  int count = 0;
  root->for_each_child([&count](auto) { ++count; });
  EXPECT_EQ(count, 2);  // b, c

  destroy_tree(root);
}

// destroy_tree() gave the place of a leftmost child to its right sibling
// only if that sibling was a leaf: the children of one with children were
// lost, and the next traversal of the parent did not end
TEST(Tree_Node, destroy_tree_of_a_leftmost_child_whose_sibling_has_children)
{
  auto *root = new Tree_Node<int>(0);
  auto *a = new Tree_Node<int>(1);
  auto *b = new Tree_Node<int>(2);
  auto *b1 = new Tree_Node<int>(21);
  auto *b2 = new Tree_Node<int>(22);
  root->insert_rightmost_child(a);
  root->insert_rightmost_child(b);
  a->insert_rightmost_child(new Tree_Node<int>(11));
  b->insert_rightmost_child(b1);
  b->insert_rightmost_child(b2);

  destroy_tree(a);

  EXPECT_EQ(root->get_left_child(), b);
  EXPECT_EQ(root->get_right_child(), b);
  EXPECT_TRUE(b->is_leftmost());
  EXPECT_EQ(b->get_parent(), root);
  EXPECT_EQ(b->get_left_child(), b1);
  EXPECT_EQ(b->get_right_child(), b2);
  EXPECT_EQ(b1->get_parent(), b);
  EXPECT_EQ(b2->get_parent(), b);

  destroy_tree(root);
}

// The parent of the only child was left as a node with children
TEST(Tree_Node, destroy_tree_of_the_only_child_makes_the_parent_a_leaf)
{
  auto *root = new Tree_Node<int>(0);
  auto *a = new Tree_Node<int>(1);
  root->insert_rightmost_child(a);
  a->insert_rightmost_child(new Tree_Node<int>(11));

  destroy_tree(a);

  EXPECT_TRUE(root->is_leaf());
  EXPECT_EQ(root->get_left_child(), nullptr);

  root->insert_rightmost_child(new Tree_Node<int>(2)); // the root still works
  ASSERT_NE(root->get_left_child(), nullptr);
  EXPECT_EQ(root->get_left_child()->get_key(), 2);
  EXPECT_EQ(root->get_left_child()->get_parent(), root);

  destroy_tree(root);
}

TEST(Tree_Node, destroy_tree_of_a_child_with_children_that_is_not_leftmost)
{
  auto *root = new Tree_Node<int>(0);
  auto *a = new Tree_Node<int>(1);
  auto *b = new Tree_Node<int>(2);
  auto *c = new Tree_Node<int>(3);
  root->insert_rightmost_child(a);
  root->insert_rightmost_child(b);
  root->insert_rightmost_child(c);
  b->insert_rightmost_child(new Tree_Node<int>(21));
  b->get_left_child()->insert_rightmost_child(new Tree_Node<int>(211));

  destroy_tree(b);

  EXPECT_EQ(a->get_right_sibling(), c);
  EXPECT_EQ(c->get_left_sibling(), a);
  EXPECT_EQ(c->get_parent(), root);

  destroy_tree(root);
}

TEST(Tree_Node, destroy_tree_of_a_tree_of_a_forest)
{
  auto *t1 = new Tree_Node<int>(1);
  auto *t2 = new Tree_Node<int>(2);
  auto *t3 = new Tree_Node<int>(3);
  t1->insert_right_sibling(t2);
  t2->insert_right_sibling(t3);
  t2->insert_rightmost_child(new Tree_Node<int>(21));

  destroy_tree(t2);

  EXPECT_EQ(t1->get_right_tree(), t3);
  EXPECT_EQ(t3->get_left_tree(), t1);

  destroy_tree(t1); // the last tree of the forest is t3 then
  EXPECT_TRUE(t3->is_leftmost());
  EXPECT_EQ(t3->get_left_tree(), nullptr);
  destroy_tree(t3);
}

// destroy_tree() recursed as deep as the tree
TEST(Tree_Node, destroy_tree_of_a_deep_chain)
{
  auto *root = new Tree_Node<int>(0);
  Tree_Node<int> *last = root;
  for (int i = 1; i < 1000000; ++i)
    {
      auto *p = new Tree_Node<int>(i);
      last->insert_rightmost_child(p);
      last = p;
    }

  destroy_tree(root);
}

namespace
{
  using Model_Node = Tree_Node<int>;

  // The parent and the children, in their order, of each node of a tree
  struct Tree_Model
  {
    std::map<Model_Node *, Model_Node *> parent;
    std::map<Model_Node *, std::vector<Model_Node *>> children;

    void add(Model_Node *p, Model_Node *par)
    {
      parent[p] = par;
      children[p];
    }

    // Takes p, and the nodes below it, out of the model
    void erase(Model_Node *p)
    {
      auto &siblings = children[parent[p]];
      siblings.erase(std::find(siblings.begin(), siblings.end(), p));
      std::vector<Model_Node *> stack = {p};
      while (not stack.empty())
        {
          auto *q = stack.back();
          stack.pop_back();
          for (auto *c : children[q])
            stack.push_back(c);
          children.erase(q);
          parent.erase(q);
        }
    }
  };

  // The tree of root is that of the model
  void expect_model(Model_Node *root, const Tree_Model &model)
  {
    for (const auto &[p, kids] : model.children)
      {
        if (p != root)
          {
            ASSERT_EQ(p->get_parent(), model.parent.at(p));
          }
        std::vector<Model_Node *> seen;
        for (auto *c = p->get_left_child(); c != nullptr and seen.size() <= kids.size();
             c = c->get_right_sibling())
          seen.push_back(c);
        ASSERT_EQ(seen, kids);
        ASSERT_EQ(p->is_leaf(), kids.empty());
        if (not kids.empty())
          {
            ASSERT_EQ(p->get_right_child(), kids.back());
          }
        for (size_t i = 0; i < kids.size(); ++i)
          {
            ASSERT_EQ(kids[i]->is_leftmost(), i == 0);
            ASSERT_EQ(kids[i]->is_rightmost(), i + 1 == kids.size());
          }
      }
  }
}

// Random insertions, and destroy_tree() of random nodes, which must leave
// the rest of the tree as if they had not been inserted
TEST(Tree_Node, destroy_tree_keeps_the_rest_of_the_tree)
{
  std::mt19937 gen(7);
  int key = 0;
  for (int round = 0; round < 20; ++round)
    {
      SCOPED_TRACE(round);
      Tree_Model model;
      auto *root = new Model_Node(key++);
      model.add(root, nullptr);
      for (int op = 0; op < 300; ++op)
        {
          std::vector<Model_Node *> nodes;
          for (const auto &kv : model.parent)
            nodes.push_back(kv.first);
          auto *p = nodes[gen() % nodes.size()];
          if (p == root or gen() % 10 < 6)
            {
              auto *q = new Model_Node(key++);
              const auto how = p == root ? gen() % 2 : gen() % 4;
              if (how < 2)
                {
                  auto &kids = model.children[p];
                  if (how == 0)
                    {
                      p->insert_rightmost_child(q);
                      kids.push_back(q);
                    }
                  else
                    {
                      p->insert_leftmost_child(q);
                      kids.insert(kids.begin(), q);
                    }
                  model.add(q, p);
                  continue;
                }

              auto *par = model.parent[p];
              auto &kids = model.children[par];
              auto pos = std::find(kids.begin(), kids.end(), p);
              if (how == 2)
                {
                  p->insert_left_sibling(q);
                  kids.insert(pos, q);
                }
              else
                {
                  p->insert_right_sibling(q);
                  kids.insert(pos + 1, q);
                }
              model.add(q, par);
            }
          else
            {
              model.erase(p);
              destroy_tree(p);
            }
          expect_model(root, model);
          if (HasFatalFailure())
            return;
        }
      destroy_tree(root);
    }
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
  ASSERT_TRUE(root->level_traverse([&i] (auto p)
			       {
				 return p->get_key() == i++;
			       }));
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
    ASSERT_EQ(root, nullptr);
  }
  {
    Tree_Node<int> root(5);
    size_t k = 0;
    ASSERT_TRUE(root.traverse([&k] (auto p) { k++; return p->get_key() == 5; }));
    ASSERT_EQ(k, 1);
  }
}

TEST(Tree_Node, are_tree_equal_uses_custom_comparator_recursively)
{
  using Node = Tree_Node<std::string>;

  auto * t1 = new Node("root");
  t1->insert_rightmost_child(new Node("A"));
  t1->insert_rightmost_child(new Node("B"));

  auto * t2 = new Node("ROOT");
  t2->insert_rightmost_child(new Node("a"));
  t2->insert_rightmost_child(new Node("b"));

  auto nocase = [] (const std::string & a, const std::string & b)
  {
    if (a.size() != b.size())
      return false;
    for (size_t i = 0; i < a.size(); ++i)
      if (std::tolower(static_cast<unsigned char>(a[i])) !=
          std::tolower(static_cast<unsigned char>(b[i])))
        return false;
    return true;
  };

  ASSERT_TRUE((are_tree_equal<Node, decltype(nocase)>(t1, t2, nocase)));

  destroy_tree(t1);
  destroy_tree(t2);
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

// =============================================================================
// Deep and wide trees, and the functions that recursed over them
// =============================================================================

namespace
{
  // A chain of n nodes, keyed 0 to n - 1: a tree of n levels
  Tree_Node<int> *chain_of(const int n)
  {
    auto *root = new Tree_Node<int>(0);
    Tree_Node<int> *last = root;
    for (int i = 1; i < n; ++i)
      {
        auto *p = new Tree_Node<int>(i);
        last->insert_rightmost_child(p);
        last = p;
      }
    return root;
  }

  // A root, keyed 0, with n - 1 children, keyed 1 to n - 1
  Tree_Node<int> *star_of(const int n)
  {
    auto *root = new Tree_Node<int>(0);
    for (int i = 1; i < n; ++i)
      root->insert_rightmost_child(new Tree_Node<int>(i));
    return root;
  }

  // The deepest node of the first children
  Tree_Node<int> *deepest_of(Tree_Node<int> *p)
  {
    while (p->get_left_child() != nullptr)
      p = p->get_left_child();
    return p;
  }

  // Frees a binary tree without recursion: a node with a left child
  // turns into the right child of that child first
  template <class BNode>
  void free_binary(BNode *root)
  {
    while (root != BNode::NullPtr)
      if (LLINK(root) != BNode::NullPtr)
        {
          BNode *left = LLINK(root);
          LLINK(root) = RLINK(left);
          RLINK(left) = root;
          root = left;
        }
      else
        {
          BNode *right = RLINK(root);
          delete root;
          root = right;
        }
  }

  long visit_sum = 0;
  long visit_count = 0;

  void sum_visit(Tree_Node<int> *p, int level, int pos)
  {
    visit_sum += long(p->get_key()) + level + pos;
    ++visit_count;
  }
}

// These functions recursed as deep as the tree, which a chain of a million
// nodes overflows
TEST(Tree_Node, the_functions_on_a_deep_chain_need_no_recursion)
{
  constexpr int n = 1000000;
  Tree_Node<int> *root = chain_of(n);
  Tree_Node<int> *deepest = deepest_of(root);

  long count = 0;
  EXPECT_TRUE(root->traverse([&count](Tree_Node<int> *) { ++count; return true; }));
  EXPECT_EQ(count, n);

  const long sum = 2 * (long(n) * (n - 1) / 2); // keys and levels; positions are 0
  visit_sum = visit_count = 0;
  tree_preorder_traversal(root, sum_visit);
  EXPECT_EQ(visit_count, n);
  EXPECT_EQ(visit_sum, sum);

  visit_sum = visit_count = 0;
  tree_postorder_traversal(root, sum_visit);
  EXPECT_EQ(visit_count, n);
  EXPECT_EQ(visit_sum, sum);

  EXPECT_EQ(compute_height(root), size_t(n));

  Tree_Node<int> *copy = clone_tree(root);
  EXPECT_TRUE(are_tree_equal(root, copy));
  deepest_of(copy)->get_key() = -1;
  EXPECT_FALSE(are_tree_equal(root, copy));
  destroy_tree(copy);

  std::vector<int> dewey(n + 1, -7);
  size_t len = 0;
  EXPECT_EQ(search_deway(root, n - 1, dewey.data(), dewey.size(), len), deepest);
  EXPECT_EQ(len, size_t(n));
  EXPECT_TRUE(std::all_of(dewey.begin(), dewey.begin() + n, [](int i) { return i == 0; }));

  std::vector<int> path(n + 1, 0);
  path[n] = -1;
  EXPECT_EQ(deway_search(root, path.data(), path.size()), deepest);

  auto *bin = forest_to_bin<Tree_Node<int>, BinNode<int>>(root);
  Tree_Node<int> *back = bin_to_forest<Tree_Node<int>, BinNode<int>>(bin);
  EXPECT_TRUE(are_tree_equal(root, back));
  destroy_forest(back);
  free_binary(bin);

  destroy_tree(root);
}

// forest_to_bin() and bin_to_forest() recursed along the siblings too, as
// many as the children of a node
TEST(Tree_Node, the_conversions_of_a_wide_tree_need_no_recursion)
{
  constexpr int n = 1000000;
  Tree_Node<int> *root = star_of(n);

  auto *bin = forest_to_bin<Tree_Node<int>, BinNode<int>>(root);
  Tree_Node<int> *back = bin_to_forest<Tree_Node<int>, BinNode<int>>(bin);
  EXPECT_TRUE(are_tree_equal(root, back));
  EXPECT_EQ(compute_height(back), 2u);

  visit_sum = visit_count = 0;
  tree_preorder_traversal(back, sum_visit);
  EXPECT_EQ(visit_count, n);
  EXPECT_EQ(visit_sum, long(n) * (n - 1)); // keys, levels and positions

  destroy_tree(back);
  free_binary(bin);
  destroy_tree(root);
}

// The operations of FunctionalMixin did not compile on a Tree_Node: the
// traversal gave them a const Tree_Node *
TEST(Tree_Node, the_functional_operations_take_the_nodes)
{
  auto *root = new Tree_Node<int>(1);
  root->insert_rightmost_child(new Tree_Node<int>(2));
  root->insert_rightmost_child(new Tree_Node<int>(3));
  root->get_left_child()->insert_rightmost_child(new Tree_Node<int>(4));

  // in preorder: 1, 2, 4, 3
  EXPECT_EQ(root->foldl<long>(0, [](long acc, Tree_Node<int> *p) { return acc * 10 + p->get_key(); }),
            1243);
  EXPECT_TRUE(root->exists([](Tree_Node<int> *p) { return p->get_key() == 4; }));
  EXPECT_TRUE(root->all([](Tree_Node<int> *p) { return p->get_key() > 0; }));
  EXPECT_EQ(root->length(), 4u);

  int sum = 0;
  root->for_each([&sum](Tree_Node<int> *p) { sum += p->get_key(); });
  EXPECT_EQ(sum, 10);
  EXPECT_EQ(root->maps<int>([](Tree_Node<int> *p) { return p->get_key(); }).size(), 4u);
  EXPECT_EQ(root->filter([](Tree_Node<int> *p) { return p->get_key() % 2 == 1; }).size(), 2u);

  const Tree_Node<int> *c = root;
  EXPECT_TRUE(c->traverse([](const Tree_Node<int> *p) { return p->get_key() < 10; }));

  destroy_tree(root);
}

namespace
{
  // A key whose copies can throw, and that counts those alive
  struct Counted_Key
  {
    static inline int alive = 0;
    static inline int copies_left = 0; // the copy that throws, when it gets to 0

    int value = 0;

    Counted_Key(int v = 0) : value(v) { ++alive; }

    Counted_Key(const Counted_Key &other) : value(other.value)
    {
      if (copies_left > 0 and --copies_left == 0)
        throw std::runtime_error("the copy of a key");
      ++alive;
    }

    Counted_Key &operator=(const Counted_Key &) = default;

    ~Counted_Key() { --alive; }
  };
}

// clone_tree(), forest_to_bin() and bin_to_forest() lost what they had made
// when the copy of a key threw
TEST(Tree_Node, the_copies_of_a_tree_leave_nothing_when_a_key_throws)
{
  using Node = Tree_Node<Counted_Key>;
  using Bin = BinNode<Counted_Key>;

  std::mt19937 gen(3);
  std::vector<Node *> nodes{new Node(Counted_Key(0))};
  for (int i = 1; i < 200; ++i)
    {
      auto *q = new Node(Counted_Key(i));
      nodes[gen() % nodes.size()]->insert_rightmost_child(q);
      nodes.push_back(q);
    }
  Node *root = nodes[0];
  const int before = Counted_Key::alive;

  for (const int k : {1, 2, 50, 200})
    {
      Counted_Key::copies_left = k;
      EXPECT_THROW((void) clone_tree(root), std::runtime_error);
      EXPECT_EQ(Counted_Key::alive, before);

      Counted_Key::copies_left = k;
      EXPECT_THROW((void) (forest_to_bin<Node, Bin>(root)), std::runtime_error);
      EXPECT_EQ(Counted_Key::alive, before);
    }

  Counted_Key::copies_left = 0;
  Bin *bin = forest_to_bin<Node, Bin>(root);
  const int with_bin = Counted_Key::alive;
  for (const int k : {1, 2, 50, 200})
    {
      Counted_Key::copies_left = k;
      EXPECT_THROW((void) (bin_to_forest<Node, Bin>(bin)), std::runtime_error);
      EXPECT_EQ(Counted_Key::alive, with_bin);
    }

  Counted_Key::copies_left = 0;
  free_binary(bin);
  destroy_tree(root);
  EXPECT_EQ(Counted_Key::alive, 0);
}

// get_parent() goes at once from the last child to the first, around the
// circle of the siblings
TEST(Tree_Node, get_parent_of_every_child)
{
  std::mt19937 gen(11);
  std::vector<Tree_Node<int> *> nodes{new Tree_Node<int>(0)};
  std::vector<Tree_Node<int> *> parents{nullptr};
  for (int i = 1; i < 3000; ++i)
    {
      auto *q = new Tree_Node<int>(i);
      Tree_Node<int> *p = nodes[gen() % nodes.size()];
      if (gen() % 3 == 0)
        p->insert_leftmost_child(q);
      else
        p->insert_rightmost_child(q);
      nodes.push_back(q);
      parents.push_back(p);
    }

  for (size_t i = 0; i < nodes.size(); ++i)
    EXPECT_EQ(nodes[i]->get_parent(), parents[i]);

  destroy_tree(nodes[0]);
}
