
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
 * @file graph_kinds_algorithms_test.cc
 * @brief The searches, the connectivity and the shortest paths on each
 * kind of graph.
 *
 * Most tests of the algorithms build List_Graph or List_Digraph; the order
 * of the arcs of List_SGraph, which keeps them in a tree, made Kruskal give
 * a wrong tree with it. These tests run the same algorithms, on the same
 * graph, with each of the six kinds of graph, and check what they give.
 */

#include <gtest/gtest.h>

#include <vector>

#include <Bellman_Ford.H>
#include <Dijkstra.H>
#include <Floyd_Warshall.H>
#include <Johnson.H>
#include <Tarjan.H>
#include <kosaraju.H>
#include <topological_sort.H>
#include <tpl_agraph.H>
#include <tpl_components.H>
#include <tpl_cut_nodes.H>
#include <tpl_find_path.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <tpl_sgraph.H>
#include <tpl_test_acyclique.H>
#include <tpl_test_cycle.H>
#include <tpl_test_path.H>

using namespace Aleph;

namespace
{
  // The weight of the arcs of a path
  template <class GT>
  int weight_of(const Path<GT> & path)
  {
    int w = 0;
    for (typename Path<GT>::Iterator it(path); it.has_curr(); it.next_ne())
      if (it.has_current_arc())
        w += it.get_current_arc_ne()->get_info();
    return w;
  }

  // A cycle 0..5, a tail 5-6-7 and a triangle 7-8-9, with the weights of
  // the arcs; the arc 0-5 weighs more than the rest of the cycle
  template <class GT>
  std::vector<typename GT::Node *> build(GT & g)
  {
    std::vector<typename GT::Node *> v;
    for (int i = 0; i < 10; ++i)
      v.push_back(g.insert_node(i));
    for (int i = 0; i < 5; ++i)
      g.insert_arc(v[i], v[i + 1], 1);
    g.insert_arc(v[5], v[0], 10);
    g.insert_arc(v[5], v[6], 2);
    g.insert_arc(v[6], v[7], 2);
    g.insert_arc(v[7], v[8], 3);
    g.insert_arc(v[8], v[9], 3);
    g.insert_arc(v[9], v[7], 3);
    return v;
  }

  template <class GT>
  struct UndirectedKinds : public ::testing::Test {};

  template <class GT>
  struct DirectedKinds : public ::testing::Test {};

  using Undirected =
    ::testing::Types<List_Graph<Graph_Node<int>, Graph_Arc<int>>,
                     List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
                     Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  using Directed =
    ::testing::Types<List_Digraph<Graph_Node<int>, Graph_Arc<int>>,
                     List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>,
                     Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>>;
}

TYPED_TEST_SUITE(UndirectedKinds, Undirected);
TYPED_TEST_SUITE(DirectedKinds, Directed);

TYPED_TEST(UndirectedKinds, CutNodesAndBridges)
{
  using GT = TypeParam;
  GT g;
  auto v = build(g);

  DynDlist<typename GT::Node *> cuts;
  Compute_Cut_Nodes<GT> cut_nodes(g);
  cut_nodes(v[0], cuts);
  EXPECT_EQ(cuts.size(), 3u); // 5, 6 and 7
  EXPECT_EQ(compute_cut_nodes(g, v[0]).size(), 3u);
  EXPECT_EQ(find_bridges(g, v[0]).size(), 2u); // 5-6 and 6-7
}

TYPED_TEST(UndirectedKinds, TraversalsAndComponents)
{
  using GT = TypeParam;
  GT g;
  auto v = build(g);
  g.insert_node(10); // alone

  EXPECT_EQ(depth_first_traversal<GT>(g, v[0], nullptr), 10u);
  EXPECT_EQ(Depth_First_Traversal<GT>()(g, v[0]), 10u);
  EXPECT_EQ(breadth_first_traversal<GT>(g, v[0], nullptr), 10u);
  EXPECT_EQ(Unconnected_Components<GT>().count_components(g), 2u);
  EXPECT_EQ(inconnected_components(g).size(), 2u);
}

TYPED_TEST(UndirectedKinds, PathsAndCycles)
{
  using GT = TypeParam;
  GT g;
  auto v = build(g);

  Path<GT> dfs_path(g);
  ASSERT_TRUE(Find_Path_Depth_First<GT>()(g, v[1], v[9], dfs_path));
  EXPECT_EQ(dfs_path.get_first_node(), v[1]);
  EXPECT_EQ(dfs_path.get_last_node(), v[9]);
  Path<GT> bfs_path(g);
  ASSERT_TRUE(Find_Path_Breadth_First<GT>()(g, v[1], v[9], bfs_path));
  EXPECT_EQ(bfs_path.size(), 6u); // 1 0 5 6 7 9

  EXPECT_TRUE(test_for_path(g, v[1], v[9]));
  EXPECT_TRUE(test_for_cycle(g, v[0]));
  EXPECT_FALSE(test_for_cycle(g, v[6]));
  EXPECT_FALSE(is_graph_acyclique(g));
  EXPECT_TRUE(Test_For_Cycle<GT>()(g, v[8]));
  EXPECT_TRUE(Test_For_Path<GT>()(g, v[1], v[9]));
}

TYPED_TEST(UndirectedKinds, ShortestPaths)
{
  using GT = TypeParam;
  GT g;
  auto v = build(g);

  Path<GT> path(g);
  EXPECT_EQ(Dijkstra_Min_Paths<GT>().find_min_path(g, v[0], v[9], path), 12);
  EXPECT_EQ(weight_of(path), 12); // 0 1 2 3 4 5 6 7 9

  Floyd_All_Shortest_Paths<GT> floyd(g);
  EXPECT_EQ(weight_of(floyd.get_min_path(v[0], v[9])), 12);
}

TYPED_TEST(DirectedKinds, StronglyConnectedComponents)
{
  using GT = TypeParam;
  GT g;
  build(g);

  EXPECT_EQ(Tarjan_Connected_Components<GT>().num_connected_components(g), 3u);
  EXPECT_TRUE(Tarjan_Connected_Components<GT>().has_cycle(g));
  EXPECT_EQ(kosaraju_connected_components(g).size(), 3u);
  DynList<GT> blocks;
  DynList<typename GT::Arc *> between;
  Tarjan_Connected_Components<GT>().connected_components(g, blocks, between);
  EXPECT_EQ(blocks.size(), 3u);
  EXPECT_EQ(between.size(), 2u);
}

TYPED_TEST(DirectedKinds, TopologicalSort)
{
  using GT = TypeParam;
  GT g;
  std::vector<typename GT::Node *> v;
  for (int i = 0; i < 6; ++i)
    v.push_back(g.insert_node(i));
  g.insert_arc(v[0], v[1], 1);
  g.insert_arc(v[0], v[2], 1);
  g.insert_arc(v[1], v[3], 1);
  g.insert_arc(v[2], v[3], 1);
  g.insert_arc(v[3], v[4], 1);
  g.insert_arc(v[5], v[4], 1);

  auto check = [&g, &v](const auto & order)
    {
      ASSERT_EQ(order.size(), v.size());
      std::vector<size_t> position(v.size());
      size_t i = 0;
      for (auto it = order.get_it(); it.has_curr(); it.next_ne())
        position[size_t(it.get_curr()->get_info())] = i++;
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        EXPECT_LT(position[size_t(g.get_src_node(it.get_curr())->get_info())],
                  position[size_t(g.get_tgt_node(it.get_curr())->get_info())]);
    };
  check(Topological_Sort<GT>().template perform<DynList>(g));
  check(Q_Topological_Sort<GT>().template perform<DynList>(g));
}

TYPED_TEST(DirectedKinds, ShortestPaths)
{
  using GT = TypeParam;
  GT g;
  auto v = build(g);

  Path<GT> path(g);
  EXPECT_EQ(Dijkstra_Min_Paths<GT>().find_min_path(g, v[0], v[9], path), 15);
  EXPECT_EQ(weight_of(path), 15); // 0 1 2 3 4 5 6 7 8 9

  Bellman_Ford<GT> bf(g);
  ASSERT_FALSE(bf.paint_spanning_tree(v[0])); // no negative cycle
  Path<GT> bf_path(g);
  EXPECT_EQ(bf.get_min_path(v[9], bf_path), 15);

  Floyd_All_Shortest_Paths<GT> floyd(g);
  EXPECT_EQ(weight_of(floyd.get_min_path(v[0], v[9])), 15);

  Johnson<GT> johnson(g);
  EXPECT_EQ(johnson.get_distance(v[0], v[9]), 15);
}
