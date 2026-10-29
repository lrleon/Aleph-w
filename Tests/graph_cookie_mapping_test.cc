
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
 * @file graph_cookie_mapping_test.cc
 * @brief The copies that the graph algorithms map to the graph.
 *
 * map_nodes(p, q) and map_arcs(p, q) compose with the mapping that the
 * cookie of p holds, if it holds one. An algorithm that gives a copy of
 * a part of g mapped to g must then clear the cookies first: those that
 * it used for something else, as the low values of the cut nodes, and
 * those that a former run left, which point to a copy that may be gone.
 * Each test checks that every node and arc of the copy maps to one of g
 * that maps back to it, after a run and after another one, once the first
 * copy is gone.
 */

#include <gtest/gtest.h>

#include <vector>

#include <Bellman_Ford.H>
#include <Dominators.H>
#include <Tarjan.H>
#include <kosaraju.H>
#include <tpl_cut_nodes.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <tpl_spanning_tree.H>

using namespace Aleph;

namespace
{
  using Mapped_Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Mapped_Digraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  // Whether p is one of the nodes of g
  template <class GT>
  bool has_node(const GT & g, typename GT::Node * p)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      if (it.get_curr() == p)
        return true;
    return false;
  }

  // Whether a is one of the arcs of g
  template <class GT>
  bool has_arc(const GT & g, typename GT::Arc * a)
  {
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      if (it.get_curr() == a)
        return true;
    return false;
  }

  // Every node and arc of c maps to one of g that maps back to it; the
  // cookie of what the copy maps to is read only if it is in g
  template <class GT>
  void expect_mapped(const GT & g, const GT & c)
  {
    for (typename GT::Node_Iterator it(c); it.has_curr(); it.next_ne())
      {
        auto q = it.get_curr();
        auto p = static_cast<typename GT::Node *>(NODE_COOKIE(q));
        ASSERT_TRUE(has_node(g, p)) << "node " << q->get_info();
        EXPECT_EQ(NODE_COOKIE(p), q) << "node " << q->get_info();
      }
    for (typename GT::Arc_Iterator it(c); it.has_curr(); it.next_ne())
      {
        auto b = it.get_curr();
        auto a = static_cast<typename GT::Arc *>(ARC_COOKIE(b));
        ASSERT_TRUE(has_arc(g, a)) << "arc " << b->get_info();
        EXPECT_EQ(ARC_COOKIE(a), b) << "arc " << b->get_info();
      }
  }

  // A cycle 0..5, a tail 5-6-7 and a triangle 7-8-9; the cut nodes are 5,
  // 6 and 7
  template <class GT>
  std::vector<typename GT::Node *> build(GT & g)
  {
    std::vector<typename GT::Node *> v;
    for (int i = 0; i < 10; ++i)
      v.push_back(g.insert_node(i));
    int k = 0;
    for (int i = 0; i < 6; ++i)
      g.insert_arc(v[i], v[(i + 1) % 6], k++);
    g.insert_arc(v[5], v[6], k++);
    g.insert_arc(v[6], v[7], k++);
    g.insert_arc(v[7], v[8], k++);
    g.insert_arc(v[8], v[9], k++);
    g.insert_arc(v[9], v[7], k++);
    return v;
  }
}

// The cookies of the nodes held their low values when Compute_Cut_Nodes
// copied them, so that the copies mapped to those values
TEST(CookieMapping, ComputeCutNodesMapsItsCopies)
{
  Mapped_Graph g;
  auto v = build(g);
  Compute_Cut_Nodes<Mapped_Graph> cc(g);
  DynDlist<Mapped_Graph::Node *> cuts;
  cc(v[0], cuts);
  EXPECT_EQ(cuts.size(), 3u);
  cc.paint_subgraphs();
  {
    Mapped_Graph block;
    cc.map_subgraph(block, NODE_COUNTER(v[0]));
    EXPECT_EQ(block.get_num_nodes(), 5u);
    expect_mapped(g, block);
  }
  Mapped_Graph block; // again, once the first copy is gone
  cc.map_subgraph(block, NODE_COUNTER(v[0]));
  EXPECT_EQ(block.get_num_nodes(), 5u);
  expect_mapped(g, block);
  {
    Mapped_Graph cut_graph;
    DynDlist<Mapped_Graph::Arc *> cross;
    cc.map_cut_graph(cut_graph, cross);
    EXPECT_EQ(cut_graph.get_num_nodes(), 3u);
    expect_mapped(g, cut_graph);
  }

  // compute_blocks() maps the cut graph again, and all the blocks
  DynDlist<Mapped_Graph> blocks;
  Mapped_Graph cut_graph;
  DynDlist<Mapped_Graph::Arc *> cross;
  cc.compute_blocks(blocks, cut_graph, cross);
  size_t nodes = 0;
  for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
    {
      nodes += it.get_curr().get_num_nodes();
      expect_mapped(g, it.get_curr());
    }
  EXPECT_EQ(nodes, 7u); // all but the cut nodes
  EXPECT_EQ(cut_graph.get_num_nodes(), 3u);
  expect_mapped(g, cut_graph);
}

// map_subgraph() reset the bits but not the cookies, and map_cut_graph()
// gave a copy of the graph that it had mapped
TEST(CookieMapping, CutNodeFunctionsMapTheirCopies)
{
  Mapped_Graph g;
  auto v = build(g);
  auto cuts = compute_cut_nodes(g, v[0]);
  EXPECT_EQ(cuts.size(), 3u);
  paint_subgraphs(g, cuts);
  const long color = NODE_COUNTER(v[0]);
  for (int run = 0; run < 2; ++run)
    {
      SCOPED_TRACE(run);
      Mapped_Graph block = map_subgraph(g, color);
      EXPECT_EQ(block.get_num_nodes(), 5u);
      expect_mapped(g, block);
      auto [cut_graph, cross] = map_cut_graph(g, cuts);
      EXPECT_EQ(cut_graph.get_num_nodes(), 3u);
      expect_mapped(g, cut_graph);
    }
}

// The breadth-first trees reset the bits but not the cookies
TEST(CookieMapping, BreadthFirstTreesMapTheirCopies)
{
  Mapped_Graph g;
  auto v = build(g);
  for (int run = 0; run < 2; ++run)
    {
      SCOPED_TRACE(run);
      Mapped_Graph tree;
      Find_Breadth_First_Spanning_Tree<Mapped_Graph>()(g, v[0], tree);
      EXPECT_EQ(tree.get_num_arcs(), 9u);
      expect_mapped(g, tree);
      Mapped_Graph tree2 = find_breadth_first_spanning_tree(g, v[0]);
      EXPECT_EQ(tree2.get_num_arcs(), 9u);
      expect_mapped(g, tree2);
    }
}

// Tarjan_Connected_Components did not clear the cookies of the arcs
TEST(CookieMapping, TarjanMapsItsBlocks)
{
  Mapped_Digraph g;
  build(g);
  for (int run = 0; run < 2; ++run)
    {
      SCOPED_TRACE(run);
      DynList<Mapped_Digraph> blocks;
      DynList<Mapped_Digraph::Arc *> between;
      Tarjan_Connected_Components<Mapped_Digraph>().connected_components(g, blocks, between);
      EXPECT_EQ(blocks.size(), 3u);
      for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
        expect_mapped(g, it.get_curr());
    }
}

// kosaraju_connected_components() mapped the nodes of g, and the blocks of
// the arcs, to the transposed graph, which it destroys
TEST(CookieMapping, KosarajuMapsItsBlocks)
{
  Mapped_Digraph g;
  build(g);
  for (int run = 0; run < 2; ++run)
    {
      SCOPED_TRACE(run);
      DynList<Mapped_Digraph> blocks;
      DynList<Mapped_Digraph::Arc *> between;
      kosaraju_connected_components(g, blocks, between);
      EXPECT_EQ(blocks.size(), 3u);
      for (auto it = blocks.get_it(); it.has_curr(); it.next_ne())
        expect_mapped(g, it.get_curr());
      for (auto it = between.get_it(); it.has_curr(); it.next_ne())
        EXPECT_EQ(ARC_COOKIE(it.get_curr()), nullptr); // not to the transposed graph
    }
  auto lists = kosaraju_connected_components(g);
  EXPECT_EQ(lists.size(), 3u);
  for (Mapped_Digraph::Node_Iterator it(g); it.has_curr(); it.next_ne())
    EXPECT_EQ(NODE_COOKIE(it.get_curr()), nullptr); // not to the transposed graph
}

// Bellman_Ford::build_tree() mapped the nodes when their cookies still
// held their parents in the tree
TEST(CookieMapping, BellmanFordMapsItsTree)
{
  Mapped_Digraph g;
  auto v = build(g);
  Bellman_Ford<Mapped_Digraph> bf(g);
  ASSERT_FALSE(bf.paint_spanning_tree(v[0])); // no negative cycle
  Mapped_Digraph tree;
  bf.build_tree(tree);
  EXPECT_EQ(tree.get_num_arcs(), 9u);
  expect_mapped(g, tree);
}

// build_dominator_tree() did not clear the cookies, unlike the tree of the
// post-dominators
TEST(CookieMapping, DominatorTreeMapsItsNodes)
{
  Mapped_Digraph g;
  auto v = build(g);
  for (int run = 0; run < 2; ++run)
    {
      SCOPED_TRACE(run);
      Mapped_Digraph tree;
      build_dominator_tree(g, v[0], tree);
      EXPECT_EQ(tree.get_num_nodes(), 10u);
      for (Mapped_Digraph::Node_Iterator it(tree); it.has_curr(); it.next_ne())
        {
          auto q = it.get_curr();
          auto p = static_cast<Mapped_Digraph::Node *>(NODE_COOKIE(q));
          ASSERT_TRUE(has_node(g, p));
          EXPECT_EQ(NODE_COOKIE(p), q);
        }
    }
}
