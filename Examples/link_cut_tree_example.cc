
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
 * @file link_cut_tree_example.cc
 * @brief Illustrative examples for Link-Cut Trees.
 *
 * Demonstrates:
 * 1. Dynamic connectivity (link, cut, connected)
 * 2. Rerooting a tree (make_root, find_root)
 * 3. Lowest common ancestor (lca)
 * 4. Path aggregates (sum, min, max)
 * 5. Lazy path updates (path_apply)
 */

#include <iostream>
#include <tpl_link_cut_tree.H>

using namespace std;
using namespace Aleph;

// ---------------------------------------------------------------
//  Example 1: Dynamic connectivity
// ---------------------------------------------------------------
static void example_connectivity()
{
  cout << "=== Example 1: Dynamic connectivity ===" << endl;

  //  Build a forest of 6 nodes:
  //
  //      0 - 1 - 2         3 - 4
  //                            |
  //                            5
  Link_Cut_Tree lct;
  auto * n0 = lct.make_vertex(0);
  auto * n1 = lct.make_vertex(1);
  auto * n2 = lct.make_vertex(2);
  auto * n3 = lct.make_vertex(3);
  auto * n4 = lct.make_vertex(4);
  auto * n5 = lct.make_vertex(5);

  lct.link(n0, n1);
  lct.link(n1, n2);
  lct.link(n3, n4);
  lct.link(n5, n4);

  cout << "  connected(0, 2) = " << boolalpha << lct.connected(n0, n2) << endl;
  cout << "  connected(0, 3) = " << boolalpha << lct.connected(n0, n3) << endl;

  // Merge the two components
  cout << "  linking node 2 to node 3..." << endl;
  lct.link(n2, n3);
  cout << "  connected(0, 5) = " << boolalpha << lct.connected(n0, n5) << endl;

  // Split them again
  cout << "  cutting edge (2, 3)..." << endl;
  lct.cut(n2, n3);
  cout << "  connected(0, 5) = " << boolalpha << lct.connected(n0, n5) << endl;

  cout << endl;
}

// ---------------------------------------------------------------
//  Example 2: Rerooting and find_root
// ---------------------------------------------------------------
static void example_rerooting()
{
  cout << "=== Example 2: Rerooting ===" << endl;

  //  Build a path:  A - B - C - D - E
  Link_Cut_Tree lct;
  auto * a = lct.make_vertex(0);
  auto * b = lct.make_vertex(1);
  auto * c = lct.make_vertex(2);
  auto * d = lct.make_vertex(3);
  auto * e = lct.make_vertex(4);

  lct.link(a, b);
  lct.link(b, c);
  lct.link(c, d);
  lct.link(d, e);

  cout << "  Before reroot: find_root(E) = node "
       << lct.get_val(lct.find_root(e)) << endl;

  lct.make_root(c);
  cout << "  After make_root(C): find_root(A) = node "
       << lct.get_val(lct.find_root(a)) << endl;
  cout << "  After make_root(C): find_root(E) = node "
       << lct.get_val(lct.find_root(e)) << endl;

  cout << "  Path size from A to E = " << lct.path_size(a, e) << endl;

  cout << endl;
}

// ---------------------------------------------------------------
//  Example 3: Lowest Common Ancestor
// ---------------------------------------------------------------
static void example_lca()
{
  cout << "=== Example 3: LCA ===" << endl;

  //  Build a tree:
  //
  //          0
  //         / \ 
  //        1   2
  //       / \   \ 
  //      3   4   5
  //
  Link_Cut_Tree lct;
  auto * n0 = lct.make_vertex(0);
  auto * n1 = lct.make_vertex(1);
  auto * n2 = lct.make_vertex(2);
  auto * n3 = lct.make_vertex(3);
  auto * n4 = lct.make_vertex(4);
  auto * n5 = lct.make_vertex(5);

  lct.make_root(n0);
  lct.link(n1, n0);
  lct.link(n2, n0);
  lct.link(n3, n1);
  lct.link(n4, n1);
  lct.link(n5, n2);

  cout << "  lca(3, 4) = " << lct.get_val(lct.lca(n3, n4)) << endl;  // 1
  cout << "  lca(3, 5) = " << lct.get_val(lct.lca(n3, n5)) << endl;  // 0
  cout << "  lca(4, 5) = " << lct.get_val(lct.lca(n4, n5)) << endl;  // 0
  cout << "  lca(0, 4) = " << lct.get_val(lct.lca(n0, n4)) << endl;  // 0

  cout << endl;
}

// ---------------------------------------------------------------
//  Example 4: Path aggregates (sum / min / max)
// ---------------------------------------------------------------
static void example_path_aggregates()
{
  cout << "=== Example 4: Path aggregates ===" << endl;

  //  Build a weighted path:
  //
  //    node weights: [10, 20, 5, 30, 15]
  //    edges:  0 - 1 - 2 - 3 - 4

  // --- Sum ---
  {
    Gen_Link_Cut_Tree<int, SumMonoid<int>> lct;
    auto * n0 = lct.make_vertex(10);
    auto * n1 = lct.make_vertex(20);
    auto * n2 = lct.make_vertex(5);
    auto * n3 = lct.make_vertex(30);
    auto * n4 = lct.make_vertex(15);

    lct.link(n0, n1);
    lct.link(n1, n2);
    lct.link(n2, n3);
    lct.link(n3, n4);

    cout << "  path_sum(0, 4) = " << lct.path_query(n0, n4) << endl;  // 80
    cout << "  path_sum(1, 3) = " << lct.path_query(n1, n3) << endl;  // 55

    lct.set_val(n2, 100);
    cout << "  after set_val(2, 100): path_sum(0, 4) = "
         << lct.path_query(n0, n4) << endl;  // 175
  }

  // --- Min ---
  {
    Gen_Link_Cut_Tree<int, MinMonoid<int>> lct;
    auto * n0 = lct.make_vertex(10);
    auto * n1 = lct.make_vertex(20);
    auto * n2 = lct.make_vertex(5);
    auto * n3 = lct.make_vertex(30);
    auto * n4 = lct.make_vertex(15);

    lct.link(n0, n1);
    lct.link(n1, n2);
    lct.link(n2, n3);
    lct.link(n3, n4);

    cout << "  path_min(0, 4) = " << lct.path_query(n0, n4) << endl;  // 5
    cout << "  path_min(3, 4) = " << lct.path_query(n3, n4) << endl;  // 15
  }

  // --- Max ---
  {
    Gen_Link_Cut_Tree<int, MaxMonoid<int>> lct;
    auto * n0 = lct.make_vertex(10);
    auto * n1 = lct.make_vertex(20);
    auto * n2 = lct.make_vertex(5);
    auto * n3 = lct.make_vertex(30);
    auto * n4 = lct.make_vertex(15);

    lct.link(n0, n1);
    lct.link(n1, n2);
    lct.link(n2, n3);
    lct.link(n3, n4);

    cout << "  path_max(0, 4) = " << lct.path_query(n0, n4) << endl;  // 30
    cout << "  path_max(0, 1) = " << lct.path_query(n0, n1) << endl;  // 20
  }

  cout << endl;
}

// ---------------------------------------------------------------
//  Example 5: Lazy path updates
// ---------------------------------------------------------------
static void example_lazy_updates()
{
  cout << "=== Example 5: Lazy path updates ===" << endl;

  //  5 nodes with initial value 0, path: 0-1-2-3-4
  using LCT = Gen_Link_Cut_Tree<long long, SumMonoid<long long>,
                                AddLazyTag<long long>>;
  LCT lct;
  auto * n0 = lct.make_vertex(0LL);
  auto * n1 = lct.make_vertex(0LL);
  auto * n2 = lct.make_vertex(0LL);
  auto * n3 = lct.make_vertex(0LL);
  auto * n4 = lct.make_vertex(0LL);

  lct.link(n0, n1);
  lct.link(n1, n2);
  lct.link(n2, n3);
  lct.link(n3, n4);

  cout << "  Initial path_sum(0, 4) = " << lct.path_query(n0, n4) << endl;

  // Add 10 to every node on path 0..4
  lct.path_apply(n0, n4, 10LL);
  cout << "  After +10 on path(0,4): sum = " << lct.path_query(n0, n4) << endl;

  // Add 5 to sub-path 1..3
  lct.path_apply(n1, n3, 5LL);
  cout << "  After +5 on path(1,3):  sum(0,4) = "
       << lct.path_query(n0, n4) << endl;
  cout << "  Sub-path sum(1,3) = " << lct.path_query(n1, n3) << endl;

  cout << "  Individual values: ";
  for (auto * nd : {n0, n1, n2, n3, n4})
    {
      // reading after access materialises the lazy value
      cout << lct.get_val(nd) << " ";
    }
  cout << endl;

  cout << endl;
}

int main()
{
  example_connectivity();
  example_rerooting();
  example_lca();
  example_path_aggregates();
  example_lazy_updates();

  return 0;
}