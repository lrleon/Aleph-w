
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
 * @file tree_allocation_test.cc
 * @brief The balanced trees and their iterators allocate nothing but their
 *        nodes: construction, move, iteration and the path stacks of the
 *        operations never allocate, so their noexcept specifications hold.
 *
 * The allocations are counted by replacing the global allocation functions.
 * A sanitizer that brings its own allocator defines them in its runtime
 * (ThreadSanitizer and MemorySanitizer link them into the program, so a
 * second definition does not link): under one, the replacement is left out
 * and the tests that count are skipped.
 */

#include <gtest/gtest.h>

#include <tpl_avl.H>
#include <tpl_avlRk.H>
#include <tpl_rb_tree.H>
#include <tpl_rbRk.H>
#include <tpl_hRbTree.H>
#include <tpl_hRbTreeRk.H>
#include <tpl_treap.H>
#include <tpl_rand_tree.H>
#include <tpl_splay_tree.H>
#include <tpl_binNodeUtils.H>
#include <tpl_dynBinHeap.H>
#include <tpl_dynSetTree.H>
#include <tpl_dynMapTree.H>

#include <cstdlib>
#include <memory>
#include <new>
#include <type_traits>
#include <utility>
#include <vector>

using namespace Aleph;

#if defined(__SANITIZE_ADDRESS__) || defined(__SANITIZE_THREAD__) \
    || defined(__SANITIZE_HWADDRESS__)
# define COUNT_ALLOCATIONS 0
#elif defined(__has_feature)
# if __has_feature(address_sanitizer) || __has_feature(thread_sanitizer) \
     || __has_feature(memory_sanitizer) || __has_feature(hwaddress_sanitizer)
#   define COUNT_ALLOCATIONS 0
# endif
#endif
#ifndef COUNT_ALLOCATIONS
# define COUNT_ALLOCATIONS 1
#endif

// Skip a test that counts allocations when they cannot be counted.
#define SKIP_UNLESS_COUNTING_ALLOCATIONS()                                   \
  if (not COUNT_ALLOCATIONS)                                                 \
    GTEST_SKIP() << "a sanitizer provides the allocation functions"

#if defined(__GNUC__) || defined(__clang__)
# define ALEPH_TEST_NOINLINE [[gnu::noinline]]
#else
# define ALEPH_TEST_NOINLINE
#endif

namespace
{
  // Calls to the global operator new. Replacing it in this translation unit
  // replaces it for the whole program; only the deltas matter. Every variant
  // is replaced, so that all the memory comes from malloc() and goes to
  // free(). The helpers are not inlined, so that the compiler does not pair
  // malloc() and free() with the operators and warn about a mismatch.
  long allocations = 0;

#if COUNT_ALLOCATIONS
  ALEPH_TEST_NOINLINE void *allocate(std::size_t size)
  {
    ++allocations;
    if (void *p = std::malloc(size == 0 ? 1 : size))
      return p;
    throw std::bad_alloc();
  }

  ALEPH_TEST_NOINLINE void release(void *p) noexcept
  {
    std::free(p);
  }
#endif

  template <class Action>
  long allocations_of(Action action)
  {
    const long before = allocations;
    action();
    return allocations - before;
  }
}

#if COUNT_ALLOCATIONS
void *operator new(std::size_t size) { return allocate(size); }
void *operator new[](std::size_t size) { return allocate(size); }

void *operator new(std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void *operator new[](std::size_t size, const std::nothrow_t &) noexcept
{
  try
    {
      return allocate(size);
    }
  catch (...)
    {
      return nullptr;
    }
}

void operator delete(void *p) noexcept { release(p); }
void operator delete[](void *p) noexcept { release(p); }
void operator delete(void *p, std::size_t) noexcept { release(p); }
void operator delete[](void *p, std::size_t) noexcept { release(p); }
void operator delete(void *p, const std::nothrow_t &) noexcept { release(p); }
void operator delete[](void *p, const std::nothrow_t &) noexcept { release(p); }
#endif

TEST(TreeAllocation, constructing_a_tree_does_not_allocate)
{
  SKIP_UNLESS_COUNTING_ALLOCATIONS();

  EXPECT_EQ(allocations_of([] { Avl_Tree<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Avl_Tree_Rk<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Rb_Tree<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Rb_Tree_Rk<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { HtdRbTree<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { HtdRbTreeRk<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Treap<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Rand_Tree<int> t; (void) t; }), 0);
  EXPECT_EQ(allocations_of([] { Splay_Tree<int> t; (void) t; }), 0);

  EXPECT_EQ(allocations_of([] { DynSetTree<int> s; (void) s; }), 0);
  EXPECT_EQ(allocations_of([] { DynSetTree<int, Rb_Tree> s; (void) s; }), 0);
  EXPECT_EQ(allocations_of([] { DynSetTree<int, Avl_Tree_Rk> s; (void) s; }), 0);
  EXPECT_EQ(allocations_of([] { DynSetTree<int, Rb_Tree_Rk> s; (void) s; }), 0);
  EXPECT_EQ(allocations_of([] { DynMapTree<int, int> m; (void) m; }), 0);
}

TEST(TreeAllocation, moving_a_tree_does_not_allocate)
{
  SKIP_UNLESS_COUNTING_ALLOCATIONS();

  DynMapTree<int, int> m;
  for (int i = 0; i < 100; ++i)
    m.insert(i, i);
  EXPECT_EQ(allocations_of([&m]
    {
      DynMapTree<int, int> moved(std::move(m));
      EXPECT_EQ(moved.size(), 100u);
    }), 0);

  DynSetTree<int, Rb_Tree_Rk> s;
  for (int i = 0; i < 100; ++i)
    s.insert(i);
  EXPECT_EQ(allocations_of([&s]
    {
      DynSetTree<int, Rb_Tree_Rk> moved(std::move(s));
      EXPECT_EQ(moved.size(), 100u);
    }), 0);

  EXPECT_EQ(allocations_of([] { Rb_Tree_Rk<int> t, moved(std::move(t)); (void) moved; }), 0);
  EXPECT_EQ(allocations_of([] { HtdRbTree<int> t, moved(std::move(t)); (void) moved; }), 0);
  EXPECT_EQ(allocations_of([] { HtdRbTreeRk<int> t, moved(std::move(t)); (void) moved; }), 0);
}

namespace
{
  // Inserting allocates the node and nothing else; removing allocates nothing.
  template <class Set>
  void check_insert_and_remove_allocate_only_the_node()
  {
    Set s;
    for (int i = 0; i < 100; ++i)
      s.insert(i);
    EXPECT_EQ(allocations_of([&s] { s.insert(1000); }), 1);
    EXPECT_EQ(allocations_of([&s] { s.remove(1000); }), 0);
    EXPECT_EQ(allocations_of([&s] { s.remove(50); }), 0);
    EXPECT_EQ(allocations_of([&s] { s.insert(50); }), 1);
    EXPECT_EQ(s.size(), 100u);
  }
}

TEST(TreeAllocation, insert_and_remove_allocate_only_the_node)
{
  SKIP_UNLESS_COUNTING_ALLOCATIONS();

  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Avl_Tree>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Avl_Tree_Rk>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Rb_Tree>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Rb_Tree_Rk>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Treap>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Rand_Tree>>();
  check_insert_and_remove_allocate_only_the_node<DynSetTree<int, Splay_Tree>>();

  // The trees with a head node, through their raw node interface.
  using Htd = HtdRbTree<int>;
  std::vector<std::unique_ptr<Htd::Node>> nodes;  // outlives the tree
  for (int i = 0; i < 100; ++i)
    nodes.emplace_back(new Htd::Node(i));
  Htd t;
  for (auto & n : nodes)
    (void) t.insert(n.get());
  EXPECT_EQ(allocations_of([&t] { (void) t.remove(50); }), 0);
  EXPECT_EQ(allocations_of([&t] { (void) t.remove(0); }), 0);
}

TEST(TreeAllocation, iterating_does_not_allocate)
{
  SKIP_UNLESS_COUNTING_ALLOCATIONS();

  DynSetTree<int> s;
  for (int i = 0; i < 100; ++i)
    s.insert(i);
  EXPECT_EQ(allocations_of([&s]
    {
      long sum = 0;
      for (DynSetTree<int>::Iterator it(s); it.has_curr(); it.next_ne())
        sum += it.get_curr();
      EXPECT_EQ(sum, 4950);
      DynSetTree<int>::Iterator it(s), copy(it);  // copying and exchanging
      it.next();
      copy.swap(it);
      EXPECT_EQ(copy.get_curr(), 1);
      EXPECT_EQ(it.get_curr(), 0);
    }), 0);

  // A splay tree uses the generic node, whose height bound is the largest.
  DynSetTree<int, Splay_Tree> splay;
  for (int i = 0; i < 100; ++i)
    splay.insert(i);
  EXPECT_EQ(allocations_of([&splay]
    {
      long sum = 0;
      for (DynSetTree<int, Splay_Tree>::Iterator it(splay); it.has_curr(); it.next_ne())
        sum += it.get_curr();
      EXPECT_EQ(sum, 4950);
    }), 0);

  using Tree = Avl_Tree<int>;
  std::vector<std::unique_ptr<Tree::Node>> nodes;  // outlives the tree
  for (int i = 0; i < 100; ++i)
    nodes.emplace_back(new Tree::Node(i));
  Tree t;
  for (auto & n : nodes)
    (void) t.insert(n.get());
  EXPECT_EQ(allocations_of([&t]
    {
      long count = 0;
      for (BinNodePrefixIterator<Tree::Node> it(t.getRoot()); it.has_curr(); it.next_ne())
        ++count;
      for (BinNodeInfixIterator<Tree::Node> it(t.getRoot()); it.has_curr(); it.next_ne())
        ++count;
      EXPECT_EQ(count, 200);
    }), 0);

  DynBinHeap<int> heap;
  for (int i = 0; i < 100; ++i)
    heap.insert(i);
  EXPECT_EQ(allocations_of([&heap]
    {
      long count = 0;
      for (DynBinHeap<int>::Iterator it(heap); it.has_curr(); it.next_ne())
        ++count;
      EXPECT_EQ(count, 100);
    }), 0);
}

TEST(TreeAllocation, noexcept_specifications_hold)
{
  static_assert(std::is_nothrow_default_constructible_v<Avl_Tree<int>>);
  static_assert(std::is_nothrow_default_constructible_v<Avl_Tree_Rk<int>>);
  static_assert(std::is_nothrow_default_constructible_v<Rb_Tree<int>>);
  static_assert(std::is_nothrow_default_constructible_v<HtdRbTree<int>>);
  static_assert(std::is_nothrow_default_constructible_v<HtdRbTreeRk<int>>);
  static_assert(std::is_nothrow_move_constructible_v<Rb_Tree<int>>);
  static_assert(std::is_nothrow_move_constructible_v<Rb_Tree_Rk<int>>);
  static_assert(std::is_nothrow_move_constructible_v<HtdRbTree<int>>);
  static_assert(std::is_nothrow_move_constructible_v<HtdRbTreeRk<int>>);
  static_assert(std::is_nothrow_move_constructible_v<DynSetTree<int>>);
  static_assert(std::is_nothrow_move_constructible_v<DynMapTree<int, int>>);
  using Node = AvlNode<int>;
  // The inorder iterator pushes the left spine when it is built, and both
  // iterators push while they advance: past Node::MaxHeight pending nodes
  // they move to the heap, so those operations may throw bad_alloc.
  static_assert(not std::is_nothrow_constructible_v<BinNodeInfixIterator<Node>, Node *>);
  static_assert(std::is_nothrow_constructible_v<BinNodePrefixIterator<Node>, Node *>);
  static_assert(not noexcept(std::declval<BinNodeInfixIterator<Node> &>().next_ne()));
  static_assert(not noexcept(std::declval<BinNodePrefixIterator<Node> &>().next_ne()));
}
