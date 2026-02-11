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
 * @file tpl_cartesian_tree_test.cc
 * @brief Unit tests for Gen_Cartesian_Tree, Gen_Euler_Tour_LCA,
 *        and Gen_Cartesian_Tree_RMQ.
 */

# include <gtest/gtest.h>

# include <tpl_cartesian_tree.H>
# include <tpl_sparse_table.H>

# include <algorithm>
# include <cstddef>
# include <numeric>
# include <random>
# include <vector>

using namespace Aleph;
using namespace testing;

namespace
{
  // ================================================================
  //  Helper: brute-force range minimum for verification
  // ================================================================

  template <typename T>
  T brute_min(const std::vector<T> & v, size_t l, size_t r)
  {
    T result = v[l];
    for (size_t i = l + 1; i <= r; ++i)
      if (v[i] < result)
        result = v[i];
    return result;
  }

  template <typename T>
  size_t brute_min_idx(const std::vector<T> & v, size_t l, size_t r)
  {
    size_t idx = l;
    for (size_t i = l + 1; i <= r; ++i)
      if (v[i] < v[idx])
        idx = i;
    return idx;
  }

  // Verify heap property: for every node i, comp(data[parent], data[i])
  template <typename T, class Comp>
  void verify_heap(const Gen_Cartesian_Tree<T, Comp> & ct, Comp c = Comp())
  {
    for (size_t i = 0; i < ct.size(); ++i)
      {
        size_t p = ct.parent_of(i);
        if (p != Gen_Cartesian_Tree<T, Comp>::NONE)
          EXPECT_TRUE(c(ct.data_at(p), ct.data_at(i)) ||
                      ct.data_at(p) == ct.data_at(i))
            << "Heap violation at node " << i
            << ": parent=" << p
            << " data[parent]=" << ct.data_at(p)
            << " data[i]=" << ct.data_at(i);
      }
  }

  // Verify inorder = {0, 1, ..., n-1}
  template <typename T, class Comp>
  void verify_inorder(const Gen_Cartesian_Tree<T, Comp> & ct)
  {
    auto io = ct.inorder();
    ASSERT_EQ(io.size(), ct.size());
    for (size_t i = 0; i < ct.size(); ++i)
      EXPECT_EQ(io(i), i) << "Inorder mismatch at position " << i;
  }

  // Brute-force LCA: walk from u and v to root, find common ancestor
  template <typename T, class Comp>
  size_t brute_lca(const Gen_Cartesian_Tree<T, Comp> & ct,
                   size_t u, size_t v)
  {
    constexpr size_t NONE = Gen_Cartesian_Tree<T, Comp>::NONE;

    // Collect ancestors of u
    std::vector<bool> is_ancestor(ct.size(), false);
    size_t curr = u;
    while (curr != NONE)
      {
        is_ancestor[curr] = true;
        curr = ct.parent_of(curr);
      }

    // Walk from v upward until hitting an ancestor of u
    curr = v;
    while (!is_ancestor[curr])
      curr = ct.parent_of(curr);

    return curr;
  }

} // anonymous namespace


// ================================================================
//  GenCartesianTree test suite
// ================================================================

TEST(GenCartesianTree, Empty)
{
  std::vector<int> empty;
  Cartesian_Tree<int> ct(empty);
  EXPECT_EQ(ct.size(), 0u);
  EXPECT_TRUE(ct.is_empty());
  EXPECT_EQ(ct.root(), Cartesian_Tree<int>::NONE);
  auto io = ct.inorder();
  EXPECT_EQ(io.size(), 0u);
  EXPECT_EQ(ct.height(), 0u);
}

TEST(GenCartesianTree, SingleElement)
{
  Cartesian_Tree<int> ct = {42};
  EXPECT_EQ(ct.size(), 1u);
  EXPECT_FALSE(ct.is_empty());
  EXPECT_EQ(ct.root(), 0u);
  EXPECT_EQ(ct.data_at(0), 42);
  EXPECT_TRUE(ct.is_leaf(0));
  EXPECT_TRUE(ct.is_root(0));
  EXPECT_EQ(ct.left_child(0), Cartesian_Tree<int>::NONE);
  EXPECT_EQ(ct.right_child(0), Cartesian_Tree<int>::NONE);
  EXPECT_EQ(ct.parent_of(0), Cartesian_Tree<int>::NONE);
  EXPECT_EQ(ct.height(), 1u);
  verify_inorder(ct);
}

TEST(GenCartesianTree, TwoElements)
{
  // {5, 3} — min is 3 at index 1, so root = 1
  Cartesian_Tree<int> ct = {5, 3};
  EXPECT_EQ(ct.root(), 1u);
  EXPECT_EQ(ct.left_child(1), 0u);
  EXPECT_EQ(ct.right_child(1), Cartesian_Tree<int>::NONE);
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, HeapProperty)
{
  Cartesian_Tree<int> ct = {3, 2, 6, 1, 9, 0, 7};
  verify_heap(ct);
}

TEST(GenCartesianTree, InorderProperty)
{
  Cartesian_Tree<int> ct = {3, 2, 6, 1, 9, 0, 7};
  verify_inorder(ct);
}

TEST(GenCartesianTree, ManualSmall)
{
  // Array: {3, 2, 6, 1, 9}
  // Minimum is 1 at index 3 -> root = 3
  // Left subtree of 3 is indices {0,1,2}, min is 2 at idx 1
  // Right subtree of 3 is index {4}
  Cartesian_Tree<int> ct = {3, 2, 6, 1, 9};

  EXPECT_EQ(ct.root(), 3u);
  EXPECT_EQ(ct.data_at(3), 1);

  // Node 1 (value 2) is left child of root 3
  EXPECT_EQ(ct.left_child(3), 1u);
  EXPECT_EQ(ct.right_child(3), 4u);

  // Node 1 (value 2): left child is 0, right child is 2
  EXPECT_EQ(ct.left_child(1), 0u);
  EXPECT_EQ(ct.right_child(1), 2u);

  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, DuplicateValues)
{
  Cartesian_Tree<int> ct = {3, 1, 4, 1, 5};
  verify_heap(ct);
  verify_inorder(ct);
  // Root should be one of the 1s (index 1 because of left-to-right processing)
  EXPECT_EQ(ct.data_at(ct.root()), 1);
}

TEST(GenCartesianTree, Sorted)
{
  // Sorted array: Cartesian tree degenerates to left-spine
  Cartesian_Tree<int> ct = {1, 2, 3, 4, 5};
  EXPECT_EQ(ct.root(), 0u);
  verify_heap(ct);
  verify_inorder(ct);
  EXPECT_EQ(ct.height(), 5u); // chain
}

TEST(GenCartesianTree, ReverseSorted)
{
  // Reverse sorted: degenerates to right-spine
  Cartesian_Tree<int> ct = {5, 4, 3, 2, 1};
  EXPECT_EQ(ct.root(), 4u);
  verify_heap(ct);
  verify_inorder(ct);
  EXPECT_EQ(ct.height(), 5u); // chain
}

TEST(GenCartesianTree, AllEqual)
{
  Cartesian_Tree<int> ct = {7, 7, 7, 7, 7};
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, CustomComparatorMax)
{
  // Max-heap Cartesian Tree
  Max_Cartesian_Tree<int> ct = {3, 2, 6, 1, 9};
  EXPECT_EQ(ct.data_at(ct.root()), 9);
  verify_heap(ct, Aleph::greater<int>());
  verify_inorder(ct);
}

TEST(GenCartesianTree, FromArray)
{
  Array<int> arr = {5, 1, 3, 2, 4};
  Cartesian_Tree<int> ct(arr);
  EXPECT_EQ(ct.size(), 5u);
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, FromVector)
{
  std::vector<int> vec = {5, 1, 3, 2, 4};
  Cartesian_Tree<int> ct(vec);
  EXPECT_EQ(ct.size(), 5u);
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, FromInitList)
{
  Cartesian_Tree<int> ct = {5, 1, 3, 2, 4};
  EXPECT_EQ(ct.size(), 5u);
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, FromDynList)
{
  DynList<int> dl = {5, 1, 3, 2, 4};
  Cartesian_Tree<int> ct(dl);
  EXPECT_EQ(ct.size(), 5u);
  verify_heap(ct);
  verify_inorder(ct);
}

TEST(GenCartesianTree, Height)
{
  // Single element: height 1
  Cartesian_Tree<int> ct1 = {42};
  EXPECT_EQ(ct1.height(), 1u);

  // Balanced-ish tree
  Cartesian_Tree<int> ct2 = {2, 1, 3};
  EXPECT_GE(ct2.height(), 2u);
  EXPECT_LE(ct2.height(), 3u);
}

TEST(GenCartesianTree, CopyMoveSwap)
{
  Cartesian_Tree<int> ct1 = {3, 1, 4, 1, 5};
  Cartesian_Tree<int> ct2(ct1); // copy
  EXPECT_EQ(ct2.root(), ct1.root());
  EXPECT_EQ(ct2.size(), ct1.size());

  Cartesian_Tree<int> ct3(std::move(ct2)); // move
  EXPECT_EQ(ct3.root(), ct1.root());
  EXPECT_EQ(ct3.size(), ct1.size());

  Cartesian_Tree<int> ct4 = {10, 20};
  ct4.swap(ct3);
  EXPECT_EQ(ct4.size(), 5u);
  EXPECT_EQ(ct3.size(), 2u);
}

TEST(GenCartesianTree, BoundsChecking)
{
  Cartesian_Tree<int> ct = {3, 1, 4};
  EXPECT_THROW(ct.data_at(3), std::out_of_range);
  EXPECT_THROW(ct.left_child(5), std::out_of_range);
  EXPECT_THROW(ct.right_child(100), std::out_of_range);
  EXPECT_THROW(ct.parent_of(3), std::out_of_range);
  EXPECT_THROW(ct.is_leaf(10), std::out_of_range);
  EXPECT_THROW(ct.is_root(3), std::out_of_range);
}

TEST(GenCartesianTree, StressRandom)
{
  constexpr size_t N = 1000;
  std::mt19937 rng(42);
  std::uniform_int_distribution<int> dist(0, 10000);

  std::vector<int> v(N);
  for (auto & x : v)
    x = dist(rng);

  Cartesian_Tree<int> ct(v);
  EXPECT_EQ(ct.size(), N);
  verify_heap(ct);
  verify_inorder(ct);
}


// ================================================================
//  EulerTourLCA test suite
// ================================================================

TEST(EulerTourLCA, Empty)
{
  std::vector<int> empty;
  Euler_Tour_LCA<int> lca(empty);
  EXPECT_EQ(lca.size(), 0u);
  EXPECT_TRUE(lca.is_empty());
}

TEST(EulerTourLCA, SingleElement)
{
  Euler_Tour_LCA<int> lca = {42};
  EXPECT_EQ(lca.size(), 1u);
  EXPECT_EQ(lca.lca(0, 0), 0u);
  EXPECT_EQ(lca.depth_of(0), 0u);
  EXPECT_EQ(lca.distance(0, 0), 0u);
}

TEST(EulerTourLCA, EulerTourSize)
{
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};
  // Euler tour size should be 2n - 1
  EXPECT_EQ(lca.euler_tour_size(), 2 * 5 - 1);
}

TEST(EulerTourLCA, LcaSelf)
{
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};
  for (size_t i = 0; i < lca.size(); ++i)
    EXPECT_EQ(lca.lca(i, i), i) << "lca(i,i) should be i for i=" << i;
}

TEST(EulerTourLCA, LcaRootChildren)
{
  // {3, 2, 6, 1, 9} — root is 3 (value 1)
  // lca of any two nodes on different sides of root = root
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};
  size_t r = lca.tree().root();
  EXPECT_EQ(r, 3u);

  // 0 is in left subtree, 4 is in right subtree
  EXPECT_EQ(lca.lca(0, 4), r);
  EXPECT_EQ(lca.lca(2, 4), r);
}

TEST(EulerTourLCA, ManualSmall)
{
  // {3, 2, 6, 1, 9}
  // Tree structure:
  //        3(1)
  //       /    \
  //     1(2)   4(9)
  //    /    \
  //  0(3)  2(6)
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};

  // LCA(0, 2) = 1 (node with value 2, parent of both)
  EXPECT_EQ(lca.lca(0, 2), 1u);

  // LCA(0, 1) = 1 (1 is parent of 0)
  EXPECT_EQ(lca.lca(0, 1), 1u);

  // LCA(1, 4) = 3 (root)
  EXPECT_EQ(lca.lca(1, 4), 3u);
}

TEST(EulerTourLCA, DepthOf)
{
  // {3, 2, 6, 1, 9}
  // Root=3 at depth 0, children 1 and 4 at depth 1, etc.
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};

  EXPECT_EQ(lca.depth_of(3), 0u); // root
  EXPECT_EQ(lca.depth_of(1), 1u); // child of root
  EXPECT_EQ(lca.depth_of(4), 1u); // child of root
  EXPECT_EQ(lca.depth_of(0), 2u); // grandchild
  EXPECT_EQ(lca.depth_of(2), 2u); // grandchild
}

TEST(EulerTourLCA, Distance)
{
  Euler_Tour_LCA<int> lca = {3, 2, 6, 1, 9};

  // Distance(0, 2) = depth(0) + depth(2) - 2*depth(lca(0,2))
  //                = 2 + 2 - 2*1 = 2
  EXPECT_EQ(lca.distance(0, 2), 2u);

  // Distance(0, 4) = 2 + 1 - 2*0 = 3
  EXPECT_EQ(lca.distance(0, 4), 3u);

  // Distance(i, i) = 0
  for (size_t i = 0; i < lca.size(); ++i)
    EXPECT_EQ(lca.distance(i, i), 0u);
}

TEST(EulerTourLCA, LcaSymmetry)
{
  Euler_Tour_LCA<int> lca = {5, 1, 8, 3, 7, 2, 9};
  for (size_t u = 0; u < lca.size(); ++u)
    for (size_t v = u; v < lca.size(); ++v)
      EXPECT_EQ(lca.lca(u, v), lca.lca(v, u))
        << "Symmetry failed for u=" << u << " v=" << v;
}

TEST(EulerTourLCA, LcaBoundsCheck)
{
  Euler_Tour_LCA<int> lca = {3, 1, 4};
  EXPECT_THROW(lca.lca(0, 5), std::out_of_range);
  EXPECT_THROW(lca.lca(10, 0), std::out_of_range);
  EXPECT_THROW(lca.depth_of(3), std::out_of_range);
}

TEST(EulerTourLCA, StressVsBruteForce)
{
  constexpr size_t N = 500;
  constexpr size_t Q = 2000;
  std::mt19937 rng(123);
  std::uniform_int_distribution<int> dist(0, 10000);

  std::vector<int> v(N);
  for (auto & x : v)
    x = dist(rng);

  Euler_Tour_LCA<int> lca(v);
  const auto & ct = lca.tree();

  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  for (size_t q = 0; q < Q; ++q)
    {
      size_t u = idx_dist(rng);
      size_t w = idx_dist(rng);
      size_t expected = brute_lca(ct, u, w);
      EXPECT_EQ(lca.lca(u, w), expected)
        << "LCA mismatch for u=" << u << " v=" << w;
    }
}


// ================================================================
//  CartesianTreeRMQ test suite
// ================================================================

TEST(CartesianTreeRMQ, Empty)
{
  std::vector<int> empty;
  Cartesian_Tree_RMQ<int> rmq(empty);
  EXPECT_EQ(rmq.size(), 0u);
  EXPECT_TRUE(rmq.is_empty());
}

TEST(CartesianTreeRMQ, SingleElement)
{
  Cartesian_Tree_RMQ<int> rmq = {42};
  EXPECT_EQ(rmq.size(), 1u);
  EXPECT_EQ(rmq.query(0, 0), 42);
  EXPECT_EQ(rmq.query_idx(0, 0), 0u);
  EXPECT_EQ(rmq.get(0), 42);
}

TEST(CartesianTreeRMQ, QueryEntireRange)
{
  Cartesian_Tree_RMQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};
  EXPECT_EQ(rmq.query(0, 6), 1);
  EXPECT_EQ(rmq.query_idx(0, 6), 4u);
}

TEST(CartesianTreeRMQ, QuerySingleElement)
{
  Cartesian_Tree_RMQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};
  for (size_t i = 0; i < rmq.size(); ++i)
    EXPECT_EQ(rmq.query(i, i), rmq.get(i));
}

TEST(CartesianTreeRMQ, QueryVsSparseTable)
{
  constexpr size_t N = 100;
  std::mt19937 rng(99);
  std::uniform_int_distribution<int> dist(-1000, 1000);

  std::vector<int> v(N);
  for (auto & x : v)
    x = dist(rng);

  Cartesian_Tree_RMQ<int> rmq(v);
  Sparse_Table<int> st(v);

  // Compare all O(N^2) pairs
  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      EXPECT_EQ(rmq.query(l, r), st.query(l, r))
        << "Mismatch at [" << l << ", " << r << "]";
}

TEST(CartesianTreeRMQ, QueryIdx)
{
  Cartesian_Tree_RMQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};

  // query_idx should return position of the minimum
  size_t idx = rmq.query_idx(0, 3);
  EXPECT_EQ(rmq.get(idx), 2);
  EXPECT_EQ(idx, 1u);

  idx = rmq.query_idx(2, 6);
  EXPECT_EQ(rmq.get(idx), 1);
  EXPECT_EQ(idx, 4u);
}

TEST(CartesianTreeRMQ, MaxVariant)
{
  Cartesian_Tree_RMaxQ<int> rmq = {5, 2, 4, 7, 1, 3, 6};
  EXPECT_EQ(rmq.query(0, 6), 7);
  EXPECT_EQ(rmq.query(0, 2), 5);
  EXPECT_EQ(rmq.query(4, 6), 6);
}

TEST(CartesianTreeRMQ, StressRandomVsBruteForce)
{
  constexpr size_t N = 1000;
  constexpr size_t Q = 5000;
  std::mt19937 rng(777);
  std::uniform_int_distribution<int> dist(-50000, 50000);

  std::vector<int> v(N);
  for (auto & x : v)
    x = dist(rng);

  Cartesian_Tree_RMQ<int> rmq(v);

  std::uniform_int_distribution<size_t> idx_dist(0, N - 1);
  for (size_t q = 0; q < Q; ++q)
    {
      size_t l = idx_dist(rng);
      size_t r = idx_dist(rng);
      if (l > r)
        std::swap(l, r);

      int expected = brute_min(v, l, r);
      EXPECT_EQ(rmq.query(l, r), expected)
        << "Mismatch at [" << l << ", " << r << "] query " << q;
    }
}

TEST(CartesianTreeRMQ, BoundsCheck)
{
  Cartesian_Tree_RMQ<int> rmq = {3, 1, 4};
  EXPECT_THROW(rmq.query(0, 5), std::out_of_range);
  EXPECT_THROW(rmq.query(2, 1), std::out_of_range);
  EXPECT_THROW(rmq.get(3), std::out_of_range);
}
