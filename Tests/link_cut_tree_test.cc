
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
 * @file link_cut_tree_test.cc
 * @brief Comprehensive tests for Link-Cut Tree (tpl_link_cut_tree.H).
 *
 * Tests structural operations (link, cut, connected, make_root, find_root,
 * lca), path aggregates (sum, min, max, xor), lazy path updates,
 * error-handling contracts, and randomised stress against a naive oracle.
 */

#include <algorithm>
#include <numeric>
#include <random>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_link_cut_tree.H>

using namespace Aleph;
using namespace testing;

// ===================================================================
//  A. Structural correctness — connectivity-only Link_Cut_Tree
// ===================================================================

class LinkCutTreeStructTest : public Test
{
protected:
  Link_Cut_Tree lct;
};

TEST_F(LinkCutTreeStructTest, SingleNodeIsItsOwnRoot)
{
  auto * x = lct.make_vertex(42);
  EXPECT_EQ(lct.find_root(x), x);
  EXPECT_TRUE(lct.connected(x, x));
  EXPECT_EQ(lct.size(), 1u);
}

TEST_F(LinkCutTreeStructTest, LinkTwoNodesConnectsThem)
{
  auto * u = lct.make_vertex(1);
  auto * v = lct.make_vertex(2);

  EXPECT_FALSE(lct.connected(u, v));

  lct.link(u, v);
  EXPECT_TRUE(lct.connected(u, v));
  EXPECT_EQ(lct.find_root(u), lct.find_root(v));
  EXPECT_EQ(lct.path_size(u, v), 2u);
}

TEST_F(LinkCutTreeStructTest, CutDisconnects)
{
  auto * u = lct.make_vertex(1);
  auto * v = lct.make_vertex(2);

  lct.link(u, v);
  EXPECT_TRUE(lct.connected(u, v));

  lct.cut(u, v);
  EXPECT_FALSE(lct.connected(u, v));
}

TEST_F(LinkCutTreeStructTest, BuildPath)
{
  constexpr int N = 10;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);

  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  for (int i = 0; i < N; ++i)
    for (int j = i; j < N; ++j)
      {
        EXPECT_TRUE(lct.connected(nd[i], nd[j]));
        EXPECT_EQ(lct.path_size(nd[i], nd[j]),
                  static_cast<size_t>(j - i + 1));
      }
}

TEST_F(LinkCutTreeStructTest, CutMiddleOfPath)
{
  constexpr int N = 6;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);

  // path: 0 - 1 - 2 - 3 - 4 - 5
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // cut edge (2,3)
  lct.cut(nd[2], nd[3]);
  EXPECT_TRUE(lct.connected(nd[0], nd[2]));
  EXPECT_TRUE(lct.connected(nd[3], nd[5]));
  EXPECT_FALSE(lct.connected(nd[0], nd[5]));

  // re-link
  lct.link(nd[2], nd[3]);
  EXPECT_TRUE(lct.connected(nd[0], nd[5]));
}

TEST_F(LinkCutTreeStructTest, BuildStar)
{
  auto * center = lct.make_vertex(0);
  constexpr int N = 8;
  std::vector<Link_Cut_Tree::Node *> leaves(N);
  for (int i = 0; i < N; ++i)
    {
      leaves[i] = lct.make_vertex(i + 1);
      lct.link(leaves[i], center);
    }

  for (int i = 0; i < N; ++i)
    EXPECT_TRUE(lct.connected(leaves[i], center));

  for (int i = 0; i < N; ++i)
    for (int j = i + 1; j < N; ++j)
      EXPECT_TRUE(lct.connected(leaves[i], leaves[j]));

  // cut each leaf
  for (int i = 0; i < N; ++i)
    {
      lct.cut(leaves[i], center);
      EXPECT_FALSE(lct.connected(leaves[i], center));
    }
}

// ===================================================================
//  B. Rerooting (make_root)
// ===================================================================

TEST_F(LinkCutTreeStructTest, RerootPath)
{
  // path 0-1-2-3-4
  constexpr int N = 5;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // reroot at node 3
  lct.make_root(nd[3]);
  EXPECT_EQ(lct.find_root(nd[0]), nd[3]);
  EXPECT_EQ(lct.find_root(nd[4]), nd[3]);
}

TEST_F(LinkCutTreeStructTest, MultipleReroots)
{
  constexpr int N = 7;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  for (int r = 0; r < N; ++r)
    {
      lct.make_root(nd[r]);
      EXPECT_EQ(lct.find_root(nd[0]), nd[r]);
      for (int i = 0; i < N; ++i)
        EXPECT_TRUE(lct.connected(nd[i], nd[r]));
    }
}

// ===================================================================
//  C. LCA
// ===================================================================

TEST_F(LinkCutTreeStructTest, LcaSameNode)
{
  auto * x = lct.make_vertex(1);
  EXPECT_EQ(lct.lca(x, x), x);
}

TEST_F(LinkCutTreeStructTest, LcaOnPath)
{
  // path 0-1-2-3-4, root at 0
  constexpr int N = 5;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  lct.make_root(nd[0]);
  // on a path rooted at 0, lca(i, j) = min(i, j)
  for (int i = 0; i < N; ++i)
    for (int j = i; j < N; ++j)
      EXPECT_EQ(lct.lca(nd[i], nd[j]), nd[i]);
}

TEST_F(LinkCutTreeStructTest, LcaOnBinaryTree)
{
  //         0
  //        / \          .
  //       1   2
  //      / \            .
  //     3   4
  auto * n0 = lct.make_vertex(0);
  auto * n1 = lct.make_vertex(1);
  auto * n2 = lct.make_vertex(2);
  auto * n3 = lct.make_vertex(3);
  auto * n4 = lct.make_vertex(4);

  lct.make_root(n0);
  lct.link(n1, n0);
  lct.link(n2, n0);
  lct.link(n3, n1);
  lct.link(n4, n1);

  EXPECT_EQ(lct.lca(n3, n4), n1);
  EXPECT_EQ(lct.lca(n3, n2), n0);
  EXPECT_EQ(lct.lca(n1, n2), n0);
  EXPECT_EQ(lct.lca(n0, n4), n0);
}

// ===================================================================
//  D. Error handling
// ===================================================================

TEST_F(LinkCutTreeStructTest, LinkAlreadyConnectedThrows)
{
  auto * u = lct.make_vertex(1);
  auto * v = lct.make_vertex(2);
  lct.link(u, v);
  EXPECT_THROW(lct.link(u, v), std::domain_error);
}

TEST_F(LinkCutTreeStructTest, SelfLinkThrows)
{
  auto * x = lct.make_vertex(1);
  EXPECT_THROW(lct.link(x, x), std::invalid_argument);
}

TEST_F(LinkCutTreeStructTest, CutNonExistentEdgeThrows)
{
  auto * u = lct.make_vertex(1);
  auto * v = lct.make_vertex(2);
  auto * w = lct.make_vertex(3);

  // not connected at all
  EXPECT_THROW(lct.cut(u, v), std::domain_error);

  // connected but no direct edge: u-v-w, try to cut u-w
  lct.link(u, v);
  lct.link(v, w);
  EXPECT_THROW(lct.cut(u, w), std::domain_error);
}

TEST_F(LinkCutTreeStructTest, NullHandleThrows)
{
  EXPECT_THROW(lct.make_root(nullptr), std::invalid_argument);
  EXPECT_THROW(lct.find_root(nullptr), std::invalid_argument);
  EXPECT_THROW(lct.connected(nullptr, nullptr), std::invalid_argument);
  auto * x = lct.make_vertex(1);
  EXPECT_THROW(lct.link(nullptr, x), std::invalid_argument);
  EXPECT_THROW(lct.cut(nullptr, x), std::invalid_argument);
  EXPECT_THROW(lct.lca(nullptr, x), std::invalid_argument);
}

TEST_F(LinkCutTreeStructTest, LcaDisconnectedThrows)
{
  auto * u = lct.make_vertex(1);
  auto * v = lct.make_vertex(2);
  EXPECT_THROW(lct.lca(u, v), std::domain_error);
}

// ===================================================================
//  D2. num_components, tree_size, depth, parent
// ===================================================================

TEST_F(LinkCutTreeStructTest, NumComponents)
{
  EXPECT_EQ(lct.num_components(), 0u);

  auto * a = lct.make_vertex(1);
  auto * b = lct.make_vertex(2);
  auto * c = lct.make_vertex(3);
  EXPECT_EQ(lct.num_components(), 3u);

  lct.link(a, b);
  EXPECT_EQ(lct.num_components(), 2u);

  lct.link(b, c);
  EXPECT_EQ(lct.num_components(), 1u);

  lct.cut(a, b);
  EXPECT_EQ(lct.num_components(), 2u);
}

TEST_F(LinkCutTreeStructTest, TreeSize)
{
  auto * a = lct.make_vertex(1);
  auto * b = lct.make_vertex(2);
  auto * c = lct.make_vertex(3);

  EXPECT_EQ(lct.tree_size(a), 1u);

  lct.link(a, b);
  EXPECT_EQ(lct.tree_size(a), 2u);
  EXPECT_EQ(lct.tree_size(b), 2u);

  lct.link(b, c);
  EXPECT_EQ(lct.tree_size(a), 3u);

  lct.cut(a, b);
  EXPECT_EQ(lct.tree_size(a), 1u);
  EXPECT_EQ(lct.tree_size(b), 2u);
}

TEST_F(LinkCutTreeStructTest, DepthOnPath)
{
  // path 0-1-2-3-4, root at 0
  constexpr int N = 5;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  lct.make_root(nd[0]);
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(lct.depth(nd[i]), static_cast<size_t>(i));

  // reroot at nd[2]: depths become |i - 2|
  lct.make_root(nd[2]);
  for (int i = 0; i < N; ++i)
    EXPECT_EQ(lct.depth(nd[i]), static_cast<size_t>(std::abs(i - 2)));
}

TEST_F(LinkCutTreeStructTest, ParentOnPath)
{
  // path 0-1-2-3, root at 0
  constexpr int N = 4;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  lct.make_root(nd[0]);
  EXPECT_EQ(lct.parent(nd[0]), nullptr);
  EXPECT_EQ(lct.parent(nd[1]), nd[0]);
  EXPECT_EQ(lct.parent(nd[2]), nd[1]);
  EXPECT_EQ(lct.parent(nd[3]), nd[2]);

  // reroot at nd[3]
  lct.make_root(nd[3]);
  EXPECT_EQ(lct.parent(nd[3]), nullptr);
  EXPECT_EQ(lct.parent(nd[2]), nd[3]);
  EXPECT_EQ(lct.parent(nd[0]), nd[1]);
}

TEST_F(LinkCutTreeStructTest, ParentOnStar)
{
  //       0
  //     / | \            .
  //    1  2  3
  auto * center = lct.make_vertex(0);
  auto * l1 = lct.make_vertex(1);
  auto * l2 = lct.make_vertex(2);
  auto * l3 = lct.make_vertex(3);

  lct.make_root(center);
  lct.link(l1, center);
  lct.link(l2, center);
  lct.link(l3, center);

  EXPECT_EQ(lct.parent(center), nullptr);
  EXPECT_EQ(lct.parent(l1), center);
  EXPECT_EQ(lct.parent(l2), center);
  EXPECT_EQ(lct.parent(l3), center);
}

// ===================================================================
//  D3. for_each_on_path, for_each_node
// ===================================================================

TEST_F(LinkCutTreeStructTest, ForEachOnPath)
{
  constexpr int N = 5;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // collect values on path from nd[1] to nd[3]
  std::vector<int> collected;
  lct.for_each_on_path(nd[1], nd[3],
    [&](Link_Cut_Tree::Node * n) { collected.push_back(lct.get_val(n)); });

  ASSERT_EQ(collected.size(), 3u);
  EXPECT_EQ(collected[0], 1);
  EXPECT_EQ(collected[1], 2);
  EXPECT_EQ(collected[2], 3);
}

TEST_F(LinkCutTreeStructTest, ForEachNode)
{
  lct.make_vertex(10);
  lct.make_vertex(20);
  lct.make_vertex(30);

  int sum = 0;
  lct.for_each_node([&](Link_Cut_Tree::Node * n) { sum += n->get_val(); });
  EXPECT_EQ(sum, 60);
}

// ===================================================================
//  D4. Batch construction: make_path, make_vertices, link_edges
// ===================================================================

TEST_F(LinkCutTreeStructTest, MakePath)
{
  auto path = lct.make_path({10, 20, 30, 40});
  EXPECT_EQ(lct.size(), 4u);
  EXPECT_EQ(lct.num_components(), 1u);
  EXPECT_TRUE(lct.connected(path(0), path(3)));
  EXPECT_EQ(lct.path_size(path(0), path(3)), 4u);
  EXPECT_EQ(lct.get_val(path(0)), 10);
  EXPECT_EQ(lct.get_val(path(3)), 40);
}

TEST_F(LinkCutTreeStructTest, MakeVertices)
{
  auto verts = lct.make_vertices({1, 2, 3});
  EXPECT_EQ(lct.size(), 3u);
  EXPECT_EQ(lct.num_components(), 3u);
  EXPECT_FALSE(lct.connected(verts(0), verts(1)));
}

TEST_F(LinkCutTreeStructTest, LinkEdges)
{
  auto verts = lct.make_vertices({1, 2, 3, 4});

  std::vector<std::pair<Link_Cut_Tree::Node *, Link_Cut_Tree::Node *>> edges = {
    {verts(0), verts(1)},
    {verts(1), verts(2)},
    {verts(2), verts(3)}
  };
  lct.link_edges(edges);

  EXPECT_EQ(lct.num_components(), 1u);
  EXPECT_TRUE(lct.connected(verts(0), verts(3)));
}

// ===================================================================
//  E. Path aggregates — SumMonoid
// ===================================================================

class LinkCutTreeSumTest : public Test
{
protected:
  Gen_Link_Cut_Tree<int, SumMonoid<int>> lct;
};

TEST_F(LinkCutTreeSumTest, SingleNodeQuery)
{
  auto * x = lct.make_vertex(7);
  EXPECT_EQ(lct.path_query(x, x), 7);
}

TEST_F(LinkCutTreeSumTest, PathSum)
{
  // path: 1 - 2 - 3 - 4 - 5
  constexpr int N = 5;
  using NodePtr = Gen_Link_Cut_Tree<int, SumMonoid<int>>::Node *;
  std::vector<NodePtr> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i + 1);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // sum of path from nd[0] to nd[4] = 1+2+3+4+5 = 15
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 15);

  // sub-paths
  EXPECT_EQ(lct.path_query(nd[1], nd[3]), 2 + 3 + 4);
  EXPECT_EQ(lct.path_query(nd[0], nd[0]), 1);
}

TEST_F(LinkCutTreeSumTest, SetValUpdatesAggregate)
{
  auto * u = lct.make_vertex(10);
  auto * v = lct.make_vertex(20);
  lct.link(u, v);

  EXPECT_EQ(lct.path_query(u, v), 30);

  lct.set_val(u, 100);
  EXPECT_EQ(lct.path_query(u, v), 120);
}

TEST_F(LinkCutTreeSumTest, SumAfterCutAndRelink)
{
  auto * a = lct.make_vertex(1);
  auto * b = lct.make_vertex(2);
  auto * c = lct.make_vertex(3);

  lct.link(a, b);
  lct.link(b, c);
  EXPECT_EQ(lct.path_query(a, c), 6);

  lct.cut(a, b);
  EXPECT_EQ(lct.path_query(b, c), 5);

  lct.link(a, c);
  EXPECT_EQ(lct.path_query(a, b), 6);
}

// ===================================================================
//  E2. Path aggregates — MinMonoid
// ===================================================================

class LinkCutTreeMinTest : public Test
{
protected:
  Gen_Link_Cut_Tree<int, MinMonoid<int>> lct;
};

TEST_F(LinkCutTreeMinTest, PathMin)
{
  using NodePtr = Gen_Link_Cut_Tree<int, MinMonoid<int>>::Node *;
  constexpr int N = 5;
  std::vector<NodePtr> nd(N);
  int vals[] = {7, 2, 9, 1, 5};
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(vals[i]);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 1);
  EXPECT_EQ(lct.path_query(nd[0], nd[2]), 2);
  EXPECT_EQ(lct.path_query(nd[2], nd[4]), 1);
}

// ===================================================================
//  E3. Path aggregates — MaxMonoid
// ===================================================================

class LinkCutTreeMaxTest : public Test
{
protected:
  Gen_Link_Cut_Tree<int, MaxMonoid<int>> lct;
};

TEST_F(LinkCutTreeMaxTest, PathMax)
{
  using NodePtr = Gen_Link_Cut_Tree<int, MaxMonoid<int>>::Node *;
  constexpr int N = 5;
  std::vector<NodePtr> nd(N);
  int vals[] = {3, 8, 1, 6, 4};
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(vals[i]);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 8);
  EXPECT_EQ(lct.path_query(nd[2], nd[4]), 6);
}

// ===================================================================
//  E4. Path aggregates — XorMonoid
// ===================================================================

class LinkCutTreeXorTest : public Test
{
protected:
  Gen_Link_Cut_Tree<int, XorMonoid<int>> lct;
};

TEST_F(LinkCutTreeXorTest, PathXor)
{
  auto * a = lct.make_vertex(5);  // 101
  auto * b = lct.make_vertex(3);  // 011
  auto * c = lct.make_vertex(6);  // 110

  lct.link(a, b);
  lct.link(b, c);

  EXPECT_EQ(lct.path_query(a, c), 5 ^ 3 ^ 6);
  EXPECT_EQ(lct.path_query(a, b), 5 ^ 3);
}

// ===================================================================
//  E5. Path aggregates — GcdMonoid
// ===================================================================

class LinkCutTreeGcdTest : public Test
{
protected:
  Gen_Link_Cut_Tree<int, GcdMonoid<int>> lct;
};

TEST_F(LinkCutTreeGcdTest, PathGcd)
{
  auto * a = lct.make_vertex(12);
  auto * b = lct.make_vertex(18);
  auto * c = lct.make_vertex(24);

  lct.link(a, b);
  lct.link(b, c);

  EXPECT_EQ(lct.path_query(a, c), 6);   // gcd(12, 18, 24) = 6
  EXPECT_EQ(lct.path_query(a, b), 6);   // gcd(12, 18) = 6
  EXPECT_EQ(lct.path_query(b, c), 6);   // gcd(18, 24) = 6

  lct.set_val(b, 15);
  EXPECT_EQ(lct.path_query(a, c), 3);   // gcd(12, 15, 24) = 3
}

// ===================================================================
//  E6. Path aggregates — ProductMonoid
// ===================================================================

class LinkCutTreeProductTest : public Test
{
protected:
  Gen_Link_Cut_Tree<long long, ProductMonoid<long long>> lct;
};

TEST_F(LinkCutTreeProductTest, PathProduct)
{
  auto * a = lct.make_vertex(2LL);
  auto * b = lct.make_vertex(3LL);
  auto * c = lct.make_vertex(5LL);

  lct.link(a, b);
  lct.link(b, c);

  EXPECT_EQ(lct.path_query(a, c), 30LL);   // 2*3*5
  EXPECT_EQ(lct.path_query(a, b), 6LL);    // 2*3
}

// ===================================================================
//  F. Lazy path updates — AddLazyTag + SumMonoid
// ===================================================================

class LinkCutTreeLazyTest : public Test
{
protected:
  Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                    AddLazyTag<long long>> lct;
};

TEST_F(LinkCutTreeLazyTest, PathApplyAndQuery)
{
  using LCT = Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                                AddLazyTag<long long>>;
  using NodePtr = LCT::Node *;

  constexpr int N = 5;
  std::vector<NodePtr> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(0LL);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // add 10 to every node on path 0..4  (5 nodes, sum = 50)
  lct.path_apply(nd[0], nd[4], 10LL);
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 50LL);

  // add 5 to sub-path 1..3  (3 nodes)
  lct.path_apply(nd[1], nd[3], 5LL);
  // nd[0]=10, nd[1]=15, nd[2]=15, nd[3]=15, nd[4]=10  => total = 65
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 65LL);

  // query sub-path 1..3 => 15+15+15 = 45
  EXPECT_EQ(lct.path_query(nd[1], nd[3]), 45LL);
}

// ===================================================================
//  F2. Lazy path updates — AssignLazyTag + SumMonoid
// ===================================================================

class LinkCutTreeAssignTest : public Test
{
protected:
  Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                    AssignLazyTag<long long>> lct;
};

TEST_F(LinkCutTreeAssignTest, AssignAndQuery)
{
  using LCT = Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                                AssignLazyTag<long long>>;
  using NodePtr = LCT::Node *;

  constexpr int N = 5;
  std::vector<NodePtr> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(static_cast<long long>(i + 1));
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // initial sum = 1+2+3+4+5 = 15
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 15LL);

  // assign 10 to all 5 nodes → sum = 50
  typename AssignLazyTag<long long>::tag_type assign_10 = {10LL, true};
  lct.path_apply(nd[0], nd[4], assign_10);
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 50LL);

  // assign 3 to sub-path 1..3 (3 nodes) → nd[0]=10, 1..3=3, nd[4]=10
  // sum = 10 + 3 + 3 + 3 + 10 = 29
  typename AssignLazyTag<long long>::tag_type assign_3 = {3LL, true};
  lct.path_apply(nd[1], nd[3], assign_3);
  EXPECT_EQ(lct.path_query(nd[0], nd[4]), 29LL);
  EXPECT_EQ(lct.path_query(nd[1], nd[3]), 9LL);
}

// ===================================================================
//  G. Randomised stress test vs naive oracle
// ===================================================================

namespace
{

// Simple naive forest using parent array + DFS for connected queries
struct NaiveForest
{
  int n;
  std::vector<std::vector<int>> adj;

  explicit NaiveForest(int n) : n(n), adj(n) {}

  void link(int u, int v)
  {
    adj[u].push_back(v);
    adj[v].push_back(u);
  }

  void cut(int u, int v)
  {
    adj[u].erase(std::find(adj[u].begin(), adj[u].end(), v));
    adj[v].erase(std::find(adj[v].begin(), adj[v].end(), u));
  }

  bool connected(int u, int v) const
  {
    if (u == v)
      return true;
    std::vector<bool> visited(n, false);
    std::vector<int> stk = {u};
    visited[u] = true;
    while (not stk.empty())
      {
        int cur = stk.back();
        stk.pop_back();
        for (int nb : adj[cur])
          if (not visited[nb])
            {
              if (nb == v)
                return true;
              visited[nb] = true;
              stk.push_back(nb);
            }
      }
    return false;
  }

  bool has_edge(int u, int v) const
  {
    return std::find(adj[u].begin(), adj[u].end(), v) != adj[u].end();
  }
};

} // anonymous namespace

TEST(LinkCutTreeStress, RandomLinkCutConnected)
{
  constexpr int N = 200;
  constexpr int OPS = 2000;

  std::mt19937 rng(12345);

  Link_Cut_Tree lct;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);

  NaiveForest oracle(N);

  // track edges for cut
  std::vector<std::pair<int, int>> edges;

  for (int op = 0; op < OPS; ++op)
    {
      int choice = std::uniform_int_distribution<int>(0, 2)(rng);

      if (choice == 0 and edges.size() < static_cast<size_t>(N - 1))
        {
          // try to link two random disconnected nodes
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          if (u != v and not oracle.connected(u, v))
            {
              lct.link(nd[u], nd[v]);
              oracle.link(u, v);
              edges.push_back({u, v});
            }
        }
      else if (choice == 1 and not edges.empty())
        {
          // cut a random existing edge
          size_t idx = std::uniform_int_distribution<size_t>(
            0, edges.size() - 1)(rng);
          auto [u, v] = edges[idx];
          edges[idx] = edges.back();
          edges.pop_back();

          lct.cut(nd[u], nd[v]);
          oracle.cut(u, v);
        }
      else
        {
          // query connected
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          EXPECT_EQ(lct.connected(nd[u], nd[v]),
                    oracle.connected(u, v))
            << "mismatch at op=" << op << " u=" << u << " v=" << v;
        }
    }
}

// ===================================================================
//  G2. Randomised path-query stress (SumMonoid)
// ===================================================================

namespace
{

struct NaiveSumForest
{
  int n;
  std::vector<int> val;
  std::vector<std::vector<int>> adj;

  explicit NaiveSumForest(int n) : n(n), val(n, 0), adj(n) {}

  void link(int u, int v) { adj[u].push_back(v); adj[v].push_back(u); }
  void cut(int u, int v)
  {
    adj[u].erase(std::find(adj[u].begin(), adj[u].end(), v));
    adj[v].erase(std::find(adj[v].begin(), adj[v].end(), u));
  }

  bool connected(int u, int v) const
  {
    if (u == v) return true;
    std::vector<bool> vis(n, false);
    std::vector<int> stk = {u};
    vis[u] = true;
    while (not stk.empty())
      {
        int c = stk.back(); stk.pop_back();
        for (int nb : adj[c])
          if (not vis[nb])
            { if (nb == v) return true; vis[nb] = true; stk.push_back(nb); }
      }
    return false;
  }

  int path_sum(int u, int v) const
  {
    // BFS to find path, then sum
    std::vector<int> par(n, -1);
    std::vector<bool> vis(n, false);
    std::vector<int> q = {u};
    vis[u] = true;
    while (not q.empty())
      {
        int c = q.back(); q.pop_back();
        if (c == v) break;
        for (int nb : adj[c])
          if (not vis[nb])
            { vis[nb] = true; par[nb] = c; q.push_back(nb); }
      }
    int s = 0;
    for (int c = v; c != -1; c = par[c])
      s += val[c];
    return s;
  }
};

} // anonymous namespace

TEST(LinkCutTreeStress, RandomPathSum)
{
  constexpr int N = 100;
  constexpr int OPS = 1000;

  std::mt19937 rng(54321);

  Gen_Link_Cut_Tree<int, SumMonoid<int>> lct;
  using NodePtr = Gen_Link_Cut_Tree<int, SumMonoid<int>>::Node *;
  std::vector<NodePtr> nd(N);

  NaiveSumForest oracle(N);

  for (int i = 0; i < N; ++i)
    {
      int v = std::uniform_int_distribution<int>(1, 100)(rng);
      nd[i] = lct.make_vertex(v);
      oracle.val[i] = v;
    }

  // build a random tree on N nodes (random Prüfer-like sequence)
  std::vector<std::pair<int, int>> edges;
  std::vector<int> perm(N);
  std::iota(perm.begin(), perm.end(), 0);
  std::shuffle(perm.begin(), perm.end(), rng);
  for (int i = 1; i < N; ++i)
    {
      int par = perm[std::uniform_int_distribution<int>(0, i - 1)(rng)];
      int cur = perm[i];
      lct.link(nd[cur], nd[par]);
      oracle.link(cur, par);
      edges.push_back({cur, par});
    }

  for (int op = 0; op < OPS; ++op)
    {
      int choice = std::uniform_int_distribution<int>(0, 3)(rng);

      if (choice == 0)
        {
          // query path sum between two connected nodes
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          if (oracle.connected(u, v))
            {
              int expected = oracle.path_sum(u, v);
              int got = lct.path_query(nd[u], nd[v]);
              EXPECT_EQ(got, expected)
                << "path_sum mismatch at op=" << op
                << " u=" << u << " v=" << v;
            }
        }
      else if (choice == 1)
        {
          // update a value
          int idx = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int nv = std::uniform_int_distribution<int>(1, 100)(rng);
          lct.set_val(nd[idx], nv);
          oracle.val[idx] = nv;
        }
      else if (choice == 2 and not edges.empty())
        {
          // cut a random edge
          size_t ei = std::uniform_int_distribution<size_t>(
            0, edges.size() - 1)(rng);
          auto [u, v] = edges[ei];
          edges[ei] = edges.back();
          edges.pop_back();
          lct.cut(nd[u], nd[v]);
          oracle.cut(u, v);
        }
      else if (choice == 3 and edges.size() < static_cast<size_t>(N - 1))
        {
          // try to link two disconnected nodes
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          if (u != v and not oracle.connected(u, v))
            {
              lct.link(nd[u], nd[v]);
              oracle.link(u, v);
              edges.push_back({u, v});
            }
        }
    }
}

// ===================================================================
//  H. Performance sanity
// ===================================================================

TEST(LinkCutTreePerf, LargePathStaysReasonable)
{
  constexpr int N = 50000;
  Link_Cut_Tree lct;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);

  // build a long path
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  // various queries should not blow up
  EXPECT_TRUE(lct.connected(nd[0], nd[N - 1]));
  EXPECT_EQ(lct.path_size(nd[0], nd[N - 1]), static_cast<size_t>(N));

  // reroot and query again
  lct.make_root(nd[N / 2]);
  EXPECT_EQ(lct.find_root(nd[0]), nd[N / 2]);

  // cut and re-link near the middle
  lct.cut(nd[N / 2], nd[N / 2 + 1]);
  EXPECT_FALSE(lct.connected(nd[0], nd[N - 1]));

  lct.link(nd[N / 2], nd[N / 2 + 1]);
  EXPECT_TRUE(lct.connected(nd[0], nd[N - 1]));
}

// ===================================================================
//  I. Edge-as-Node Link-Cut Tree
// ===================================================================

#include <tpl_link_cut_tree_with_edges.H>

class LinkCutTreeEdgesTest : public Test
{
protected:
  Gen_Link_Cut_Tree_WithEdges<int, int, MaxMonoid<int>> lct;
};

TEST_F(LinkCutTreeEdgesTest, SimpleLink)
{
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);

  lct.link(u, v, 10);  // edge weight = 10
  EXPECT_TRUE(lct.connected(u, v));
  EXPECT_EQ(lct.size(), 2u);  // only 2 vertices, not the edge node
}

TEST_F(LinkCutTreeEdgesTest, PathQueryEdges)
{
  // Build: u --(5)-- v --(7)-- w
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);
  auto * w = lct.make_vertex(2);

  lct.link(u, v, 5);
  lct.link(v, w, 7);

  // Max edge weight on path u-w = max(5, 7) = 7
  EXPECT_EQ(lct.path_query(u, w), 7);
  EXPECT_EQ(lct.path_query(u, v), 5);
  EXPECT_EQ(lct.path_query(v, w), 7);
}

TEST_F(LinkCutTreeEdgesTest, EdgeValueUpdate)
{
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);

  lct.link(u, v, 10);

  // Get the edge node (it's the only edge)
  // After link, the structure is: u ← edge_node → v
  // We need to access the edge through the internal structure
  // For now, test that path_query works
  EXPECT_EQ(lct.path_query(u, v), 10);
}

TEST_F(LinkCutTreeEdgesTest, Cut)
{
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);

  lct.link(u, v, 100);
  EXPECT_TRUE(lct.connected(u, v));

  lct.cut(u, v);
  EXPECT_FALSE(lct.connected(u, v));
}

TEST_F(LinkCutTreeEdgesTest, DynamicMSTSimulation)
{
  // Simple MST: 3 vertices, try edges with weights
  //     1
  //   /   \
  //  0     2

  auto * v0 = lct.make_vertex(0);
  auto * v1 = lct.make_vertex(1);
  auto * v2 = lct.make_vertex(2);

  // Add edges: (0,1) weight 2, (1,2) weight 3, (0,2) weight 5
  lct.link(v0, v1, 2);
  lct.link(v1, v2, 3);

  // If we add (0,2) with weight 5, it creates a cycle
  // The max weight on path 0-2 is max(2, 3) = 3
  // So we could remove (1,2) if we add (0,2)
  EXPECT_EQ(lct.path_query(v0, v2), 3);  // max of {2, 3}
}

TEST_F(LinkCutTreeEdgesTest, MultipleEdges)
{
  //     a
  //    /|\
  //   b c d

  auto * a = lct.make_vertex(100);
  auto * b = lct.make_vertex(101);
  auto * c = lct.make_vertex(102);
  auto * d = lct.make_vertex(103);

  lct.link(b, a, 10);
  lct.link(c, a, 20);
  lct.link(d, a, 30);

  EXPECT_EQ(lct.num_components(), 1u);
  EXPECT_EQ(lct.size(), 4u);

  // Paths from b to other nodes
  EXPECT_EQ(lct.path_query(b, c), 20);  // max(10, 20)
  EXPECT_EQ(lct.path_query(b, d), 30);  // max(10, 30)
}

// ===================================================================
//  J. Edge-as-Node: edge value update and stress test
// ===================================================================

TEST_F(LinkCutTreeEdgesTest, SetEdgeVal)
{
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);
  auto * w = lct.make_vertex(2);

  lct.link(u, v, 5);
  lct.link(v, w, 3);

  EXPECT_EQ(lct.path_query(u, w), 5);  // max(5, 3) = 5

  lct.set_edge_val(u, v, 1);
  EXPECT_EQ(lct.path_query(u, w), 3);  // max(1, 3) = 3

  EXPECT_EQ(lct.get_edge_val(u, v), 1);
  EXPECT_EQ(lct.get_edge_val(v, w), 3);
}

TEST_F(LinkCutTreeEdgesTest, CutAndRelink)
{
  auto * a = lct.make_vertex(0);
  auto * b = lct.make_vertex(1);
  auto * c = lct.make_vertex(2);

  lct.link(a, b, 10);
  lct.link(b, c, 20);
  EXPECT_EQ(lct.path_query(a, c), 20);

  lct.cut(a, b);
  EXPECT_FALSE(lct.connected(a, b));
  EXPECT_TRUE(lct.connected(b, c));

  lct.link(a, c, 5);
  EXPECT_EQ(lct.path_query(a, b), 20);  // a-c(5)-b... wait: a--5--c--20--b => max(5, 20) = 20
}

TEST_F(LinkCutTreeEdgesTest, ErrorHandling)
{
  auto * u = lct.make_vertex(0);
  auto * v = lct.make_vertex(1);

  // self-loop
  EXPECT_THROW(lct.link(u, u, 1), std::invalid_argument);

  // null handle
  EXPECT_THROW(lct.link(nullptr, v, 1), std::invalid_argument);

  // link then double-link
  lct.link(u, v, 5);
  EXPECT_THROW(lct.link(u, v, 10), std::domain_error);

  // cut non-existent edge
  auto * w = lct.make_vertex(2);
  EXPECT_THROW(lct.cut(u, w), std::domain_error);
}

// ===================================================================
//  K. Edge-as-Node: randomised stress vs naive oracle
// ===================================================================

namespace
{

struct NaiveEdgeForest
{
  int n;
  struct Edge { int u, v, w; };
  std::vector<Edge> edges;
  std::vector<std::vector<std::pair<int, int>>> adj; // adj[u] = {(v, edge_idx)}

  explicit NaiveEdgeForest(int n) : n(n), adj(n) {}

  void link(int u, int v, int w)
  {
    int idx = static_cast<int>(edges.size());
    edges.push_back({u, v, w});
    adj[u].push_back({v, idx});
    adj[v].push_back({u, idx});
  }

  void cut(int u, int v)
  {
    for (auto it = adj[u].begin(); it != adj[u].end(); ++it)
      if (it->first == v) { adj[u].erase(it); break; }
    for (auto it = adj[v].begin(); it != adj[v].end(); ++it)
      if (it->first == u) { adj[v].erase(it); break; }
  }

  bool connected(int u, int v) const
  {
    if (u == v) return true;
    std::vector<bool> vis(n, false);
    std::vector<int> stk = {u};
    vis[u] = true;
    while (not stk.empty())
      {
        int c = stk.back(); stk.pop_back();
        for (auto [nb, _] : adj[c])
          if (not vis[nb])
            { if (nb == v) return true; vis[nb] = true; stk.push_back(nb); }
      }
    return false;
  }

  bool has_edge(int u, int v) const
  {
    for (auto [nb, _] : adj[u])
      if (nb == v) return true;
    return false;
  }

  int path_max(int u, int v) const
  {
    std::vector<int> par(n, -1);
    std::vector<int> par_edge(n, -1);
    std::vector<bool> vis(n, false);
    std::vector<int> q = {u};
    vis[u] = true;
    while (not q.empty())
      {
        int c = q.back(); q.pop_back();
        if (c == v) break;
        for (auto [nb, eidx] : adj[c])
          if (not vis[nb])
            { vis[nb] = true; par[nb] = c; par_edge[nb] = eidx; q.push_back(nb); }
      }
    int mx = std::numeric_limits<int>::lowest();
    for (int c = v; par[c] != -1; c = par[c])
      mx = std::max(mx, edges[par_edge[c]].w);
    return mx;
  }
};

} // anonymous namespace

TEST(LinkCutTreeEdgeStress, RandomLinkCutPathMax)
{
  constexpr int N = 100;
  constexpr int OPS = 1500;

  std::mt19937 rng(99887);

  Gen_Link_Cut_Tree_WithEdges<int, int, MaxMonoid<int>> lct;
  using NodePtr = Gen_Link_Cut_Tree_WithEdges<int, int, MaxMonoid<int>>::Node *;
  std::vector<NodePtr> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i);

  NaiveEdgeForest oracle(N);
  std::vector<std::pair<int, int>> user_edges;

  for (int op = 0; op < OPS; ++op)
    {
      int choice = std::uniform_int_distribution<int>(0, 2)(rng);

      if (choice == 0 and user_edges.size() < static_cast<size_t>(N - 1))
        {
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          if (u != v and not oracle.connected(u, v))
            {
              int w = std::uniform_int_distribution<int>(1, 1000)(rng);
              lct.link(nd[u], nd[v], w);
              oracle.link(u, v, w);
              user_edges.push_back({u, v});
            }
        }
      else if (choice == 1 and not user_edges.empty())
        {
          size_t idx = std::uniform_int_distribution<size_t>(
            0, user_edges.size() - 1)(rng);
          auto [u, v] = user_edges[idx];
          user_edges[idx] = user_edges.back();
          user_edges.pop_back();

          lct.cut(nd[u], nd[v]);
          oracle.cut(u, v);
        }
      else
        {
          int u = std::uniform_int_distribution<int>(0, N - 1)(rng);
          int v = std::uniform_int_distribution<int>(0, N - 1)(rng);
          if (u != v and oracle.connected(u, v))
            {
              int got = lct.path_query(nd[u], nd[v]);
              int expected = oracle.path_max(u, v);
              EXPECT_EQ(got, expected)
                << "path_max mismatch at op=" << op
                << " u=" << u << " v=" << v;
            }
          else
            {
              EXPECT_EQ(lct.connected(nd[u], nd[v]),
                        oracle.connected(u, v));
            }
        }
    }
}

// ===================================================================
//  L. export_to_tree_node — vertex-only LCT
// ===================================================================

TEST_F(LinkCutTreeStructTest, ExportToTreeNodePath)
{
  // path: 0-1-2-3-4
  constexpr int N = 5;
  std::vector<Link_Cut_Tree::Node *> nd(N);
  for (int i = 0; i < N; ++i)
    nd[i] = lct.make_vertex(i * 10);
  for (int i = 0; i + 1 < N; ++i)
    lct.link(nd[i], nd[i + 1]);

  auto * tree = lct.export_to_tree_node(nd[0]);

  // root should have value 0
  EXPECT_EQ(tree->get_key(), 0);
  EXPECT_TRUE(tree->is_root());

  // root has one child (1), which has one child (2), etc.
  auto * c = tree->get_left_child();
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->get_key(), 10);

  c = c->get_left_child();
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->get_key(), 20);

  c = c->get_left_child();
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->get_key(), 30);

  c = c->get_left_child();
  ASSERT_NE(c, nullptr);
  EXPECT_EQ(c->get_key(), 40);
  EXPECT_TRUE(c->is_leaf());

  destroy_tree(tree);
}

TEST_F(LinkCutTreeStructTest, ExportToTreeNodeStar)
{
  //       0
  //     / | \          .
  //    1  2  3
  auto * center = lct.make_vertex(100);
  auto * l1 = lct.make_vertex(10);
  auto * l2 = lct.make_vertex(20);
  auto * l3 = lct.make_vertex(30);

  lct.make_root(center);
  lct.link(l1, center);
  lct.link(l2, center);
  lct.link(l3, center);

  auto * tree = lct.export_to_tree_node(center);
  EXPECT_EQ(tree->get_key(), 100);
  EXPECT_TRUE(tree->is_root());
  EXPECT_FALSE(tree->is_leaf());

  // Collect children values
  std::vector<int> child_vals;
  for (auto * c = tree->get_left_child(); c != nullptr;
       c = c->get_right_sibling())
    {
      child_vals.push_back(c->get_key());
      EXPECT_TRUE(c->is_leaf());
    }

  std::sort(child_vals.begin(), child_vals.end());
  ASSERT_EQ(child_vals.size(), 3u);
  EXPECT_EQ(child_vals[0], 10);
  EXPECT_EQ(child_vals[1], 20);
  EXPECT_EQ(child_vals[2], 30);

  destroy_tree(tree);
}

// ===================================================================
//  M. export_to_tree_node — edge-weighted LCT
// ===================================================================

TEST_F(LinkCutTreeEdgesTest, ExportToTreeNodeEdges)
{
  //  a --(5)-- b --(3)-- c --(7)-- d
  auto * a = lct.make_vertex(0);
  auto * b = lct.make_vertex(1);
  auto * c = lct.make_vertex(2);
  auto * d = lct.make_vertex(3);

  lct.link(a, b, 5);
  lct.link(b, c, 3);
  lct.link(c, d, 7);

  auto * tree = lct.export_to_tree_node(a);

  // Root = a, parent_edge = identity (INT_MIN for MaxMonoid)
  EXPECT_EQ(tree->get_key().lct_node, a);
  EXPECT_TRUE(tree->is_root());

  // a's only child is b with edge weight 5
  auto * cb = tree->get_left_child();
  ASSERT_NE(cb, nullptr);
  EXPECT_EQ(cb->get_key().lct_node, b);
  EXPECT_EQ(cb->get_key().parent_edge, 5);

  // b's only child is c with edge weight 3
  auto * cc = cb->get_left_child();
  ASSERT_NE(cc, nullptr);
  EXPECT_EQ(cc->get_key().lct_node, c);
  EXPECT_EQ(cc->get_key().parent_edge, 3);

  // c's only child is d with edge weight 7
  auto * cd = cc->get_left_child();
  ASSERT_NE(cd, nullptr);
  EXPECT_EQ(cd->get_key().lct_node, d);
  EXPECT_EQ(cd->get_key().parent_edge, 7);
  EXPECT_TRUE(cd->is_leaf());

  destroy_tree(tree);
}

TEST_F(LinkCutTreeEdgesTest, ExportToTreeNodeStar)
{
  //     center
  //    /  |  \          .
  //   x   y   z
  //  (10)(20)(30)
  auto * center = lct.make_vertex(0);
  auto * x = lct.make_vertex(1);
  auto * y = lct.make_vertex(2);
  auto * z = lct.make_vertex(3);

  lct.link(x, center, 10);
  lct.link(y, center, 20);
  lct.link(z, center, 30);

  auto * tree = lct.export_to_tree_node(center);
  EXPECT_EQ(tree->get_key().lct_node, center);

  std::vector<int> child_weights;
  for (auto * c = tree->get_left_child(); c != nullptr;
       c = c->get_right_sibling())
    child_weights.push_back(c->get_key().parent_edge);

  std::sort(child_weights.begin(), child_weights.end());
  ASSERT_EQ(child_weights.size(), 3u);
  EXPECT_EQ(child_weights[0], 10);
  EXPECT_EQ(child_weights[1], 20);
  EXPECT_EQ(child_weights[2], 30);

  destroy_tree(tree);
}

int main(int argc, char * argv[])
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}