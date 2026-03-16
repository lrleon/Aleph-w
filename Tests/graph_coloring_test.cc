/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  MIT License (see Graph_Coloring.H for full text)
*/

# include <gtest/gtest.h>
# include <Graph_Coloring.H>
# include <tpl_sgraph.H>
# include <tpl_agraph.H>

using namespace Aleph;

// ----- Graph type aliases -----
using Graph = List_Graph<Graph_Node<std::string>, Graph_Arc<Empty_Class>>;
using DGraph = List_Digraph<Graph_Node<std::string>, Graph_Arc<Empty_Class>>;
using SGraph = List_SGraph<Graph_Snode<int>, Graph_Sarc<Empty_Class>>;
using AGraph = Array_Graph<Graph_Anode<std::string>, Graph_Aarc<Empty_Class>>;

// ===================================================================
// Helper: build a complete graph K_n
// ===================================================================
static Graph build_complete_graph(size_t n)
{
  Graph g;
  DynList<Graph::Node *> nodes;
  for (size_t i = 0; i < n; ++i)
    nodes.append(g.insert_node("v" + std::to_string(i)));

  // For each pair (i, j) with i < j, insert an edge
  size_t i = 0;
  for (auto it_i = nodes.get_it(); it_i.has_curr(); it_i.next_ne(), ++i)
    {
      size_t j = 0;
      for (auto it_j = nodes.get_it(); it_j.has_curr(); it_j.next_ne(), ++j)
        if (j > i)
          g.insert_arc(it_i.get_curr(), it_j.get_curr());
    }

  return g;
}

// ===================================================================
// Helper: build a cycle graph C_n
// ===================================================================
static Graph build_cycle(size_t n)
{
  Graph g;
  DynList<Graph::Node *> nodes;
  for (size_t i = 0; i < n; ++i)
    nodes.append(g.insert_node("v" + std::to_string(i)));

  Graph::Node *prev = nullptr;
  Graph::Node *first = nullptr;
  for (auto it = nodes.get_it(); it.has_curr(); it.next_ne())
    {
      Graph::Node *curr = it.get_curr();
      if (prev != nullptr)
        g.insert_arc(prev, curr);
      else
        first = curr;
      prev = curr;
    }
  if (n > 2)
    g.insert_arc(prev, first);

  return g;
}

// ===================================================================
// Helper: build a path graph P_n
// ===================================================================
static Graph build_path(size_t n)
{
  Graph g;
  Graph::Node *prev = nullptr;
  for (size_t i = 0; i < n; ++i)
    {
      auto *curr = g.insert_node("v" + std::to_string(i));
      if (prev != nullptr)
        g.insert_arc(prev, curr);
      prev = curr;
    }
  return g;
}

// ===================================================================
// Helper: build a star graph S_n (one center, n-1 leaves)
// ===================================================================
static Graph build_star(size_t n)
{
  Graph g;
  if (n == 0) return g;
  auto *center = g.insert_node("center");
  for (size_t i = 1; i < n; ++i)
    {
      auto *leaf = g.insert_node("leaf" + std::to_string(i));
      g.insert_arc(center, leaf);
    }
  return g;
}

// ===================================================================
// Helper: build the Petersen graph (chromatic number = 3)
// ===================================================================
static Graph build_petersen()
{
  Graph g;
  // Outer ring: 0..4, inner pentagram: 5..9
  Graph::Node *nodes[10];
  for (int i = 0; i < 10; ++i)
    nodes[i] = g.insert_node("p" + std::to_string(i));

  // Outer cycle: 0-1-2-3-4-0
  for (int i = 0; i < 5; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % 5]);

  // Inner pentagram: 5-7-9-6-8-5
  g.insert_arc(nodes[5], nodes[7]);
  g.insert_arc(nodes[7], nodes[9]);
  g.insert_arc(nodes[9], nodes[6]);
  g.insert_arc(nodes[6], nodes[8]);
  g.insert_arc(nodes[8], nodes[5]);

  // Spokes: i -- i+5
  for (int i = 0; i < 5; ++i)
    g.insert_arc(nodes[i], nodes[i + 5]);

  return g;
}

// ===================================================================
// Helper: build a wheel graph W_n (cycle of n-1 + central hub)
// ===================================================================
static Graph build_wheel(size_t n)
{
  Graph g;
  if (n < 4) return g;

  auto *hub = g.insert_node("hub");
  DynList<Graph::Node *> ring;
  for (size_t i = 1; i < n; ++i)
    {
      auto *v = g.insert_node("v" + std::to_string(i));
      ring.append(v);
      g.insert_arc(hub, v);
    }

  Graph::Node *prev = nullptr;
  Graph::Node *first = nullptr;
  for (auto it = ring.get_it(); it.has_curr(); it.next_ne())
    {
      auto *curr = it.get_curr();
      if (prev != nullptr)
        g.insert_arc(prev, curr);
      else
        first = curr;
      prev = curr;
    }
  g.insert_arc(prev, first);

  return g;
}

// ===================================================================
// Helper: build K_{m,n} complete bipartite graph
// ===================================================================
static Graph build_complete_bipartite(size_t m, size_t n)
{
  Graph g;
  DynList<Graph::Node *> left, right;
  for (size_t i = 0; i < m; ++i)
    left.append(g.insert_node("L" + std::to_string(i)));
  for (size_t i = 0; i < n; ++i)
    right.append(g.insert_node("R" + std::to_string(i)));

  for (auto li = left.get_it(); li.has_curr(); li.next_ne())
    for (auto ri = right.get_it(); ri.has_curr(); ri.next_ne())
      g.insert_arc(li.get_curr(), ri.get_curr());

  return g;
}

// ===================================================================
// Count distinct colors in a coloring map
// ===================================================================
static size_t count_distinct_colors(
    const DynMapTree<Graph::Node *, size_t> & colors)
{
  DynSetTree<size_t> s;
  colors.for_each([&](const auto & p) { s.insert(p.second); });
  return s.size();
}

// ===================================================================
// Tests: trivial cases
// ===================================================================

TEST(GraphColoring, EmptyGraph)
{
  Graph g;
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 0u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(welsh_powell_coloring(g, colors), 0u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(dsatur_coloring(g, colors), 0u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, SingleNode)
{
  Graph g;
  g.insert_node("v0");
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(welsh_powell_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(dsatur_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, IsolatedNodes)
{
  Graph g;
  for (int i = 0; i < 10; ++i)
    g.insert_node("v" + std::to_string(i));

  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(welsh_powell_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(dsatur_coloring(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: complete graphs K_n (need exactly n colors)
// ===================================================================

TEST(GraphColoring, CompleteK2)
{
  auto g = build_complete_graph(2);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(welsh_powell_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(dsatur_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, CompleteK3)
{
  auto g = build_complete_graph(3);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, CompleteK4)
{
  auto g = build_complete_graph(4);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, CompleteK5)
{
  auto g = build_complete_graph(5);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 5u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: bipartite graphs and trees (chromatic number = 2)
// ===================================================================

TEST(GraphColoring, CompleteBipartiteK33)
{
  auto g = build_complete_bipartite(3, 3);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, Path5)
{
  auto g = build_path(5);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, Star6)
{
  auto g = build_star(6);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, TreeChain)
{
  // A simple tree (chain = path) needs 2 colors
  auto g = build_path(10);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: cycles (even = 2 colors, odd = 3 colors)
// ===================================================================

TEST(GraphColoring, EvenCycle)
{
  auto g = build_cycle(6);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, OddCycle)
{
  auto g = build_cycle(5);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, Triangle)
{
  auto g = build_cycle(3);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: Petersen graph (chromatic number = 3)
// ===================================================================

TEST(GraphColoring, PetersenGraph)
{
  auto g = build_petersen();
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: disconnected graphs
// ===================================================================

TEST(GraphColoring, DisconnectedComponents)
{
  // Two disconnected triangles
  Graph g;
  auto *a1 = g.insert_node("a1");
  auto *a2 = g.insert_node("a2");
  auto *a3 = g.insert_node("a3");
  g.insert_arc(a1, a2);
  g.insert_arc(a2, a3);
  g.insert_arc(a3, a1);

  auto *b1 = g.insert_node("b1");
  auto *b2 = g.insert_node("b2");
  auto *b3 = g.insert_node("b3");
  g.insert_arc(b1, b2);
  g.insert_arc(b2, b3);
  g.insert_arc(b3, b1);

  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, DisconnectedMixed)
{
  // A triangle (needs 3 colors) + isolated node
  Graph g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  auto *c = g.insert_node("c");
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, a);
  g.insert_node("isolated");

  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: wheel graphs
// ===================================================================

TEST(GraphColoring, WheelW5EvenRing)
{
  // W5 = hub + C4; C4 is even cycle → 3 colors needed
  auto g = build_wheel(5);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, WheelW6OddRing)
{
  // W6 = hub + C5; C5 is odd cycle → 4 colors needed
  auto g = build_wheel(6);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: algorithm comparison — all produce valid colorings
// ===================================================================

TEST(GraphColoring, AllAlgorithmsProduceValidColorings)
{
  auto g = build_petersen();
  DynMapTree<Graph::Node *, size_t> c1, c2, c3;

  size_t k1 = greedy_coloring(g, c1);
  size_t k2 = welsh_powell_coloring(g, c2);
  size_t k3 = dsatur_coloring(g, c3);

  EXPECT_TRUE(is_valid_coloring(g, c1));
  EXPECT_TRUE(is_valid_coloring(g, c2));
  EXPECT_TRUE(is_valid_coloring(g, c3));

  // All should use at most Delta+1 colors
  // Petersen graph is 3-regular, so Delta+1 = 4
  EXPECT_LE(k1, 4u);
  EXPECT_LE(k2, 4u);
  EXPECT_LE(k3, 4u);
}

TEST(GraphColoring, GreedyBoundDeltaPlusOne)
{
  // Greedy coloring guarantees at most Delta+1 colors
  auto g = build_complete_bipartite(4, 4);
  DynMapTree<Graph::Node *, size_t> colors;

  size_t k = greedy_coloring(g, colors);
  // K_{4,4}: Delta = 4, so k <= 5
  EXPECT_LE(k, 5u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: validation
// ===================================================================

TEST(GraphColoring, ValidationRejectsInvalidColoring)
{
  auto g = build_complete_graph(3);
  DynMapTree<Graph::Node *, size_t> colors;

  // Assign the same color to all nodes
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    colors.insert(it.get_curr(), 0);

  EXPECT_FALSE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ValidationRejectsMissingNode)
{
  Graph g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  g.insert_arc(a, b);

  // Only color node a, not b
  DynMapTree<Graph::Node *, size_t> colors;
  colors.insert(a, 0);

  EXPECT_FALSE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ValidationRejectsSelfLoop)
{
  Graph g;
  auto *a = g.insert_node("a");
  g.insert_arc(a, a);

  DynMapTree<Graph::Node *, size_t> colors;
  colors.insert(a, 0);

  EXPECT_FALSE(is_valid_coloring(g, colors));
  EXPECT_THROW(greedy_coloring(g, colors), std::domain_error);
  EXPECT_THROW(welsh_powell_coloring(g, colors), std::domain_error);
  EXPECT_THROW(dsatur_coloring(g, colors), std::domain_error);
  EXPECT_THROW(chromatic_number(g, colors), std::domain_error);
}

// ===================================================================
// Tests: chromatic number (exact, small graphs)
// ===================================================================

TEST(GraphColoring, ChromaticNumberEmptyGraph)
{
  Graph g;
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 0u);
}

TEST(GraphColoring, ChromaticNumberIsolatedNodes)
{
  Graph g;
  for (int i = 0; i < 5; ++i)
    g.insert_node("v" + std::to_string(i));

  DynMapTree<Graph::Node *, size_t> colors;
  EXPECT_EQ(chromatic_number(g, colors), 1u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberK4)
{
  auto g = build_complete_graph(4);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
  EXPECT_EQ(count_distinct_colors(colors), 4u);
}

TEST(GraphColoring, ChromaticNumberPetersen)
{
  auto g = build_petersen();
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberOddCycle)
{
  auto g = build_cycle(5);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberEvenCycle)
{
  auto g = build_cycle(6);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberBipartite)
{
  auto g = build_complete_bipartite(3, 3);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberTooManyNodes)
{
  Graph g;
  for (int i = 0; i < 65; ++i)
    g.insert_node("v" + std::to_string(i));

  DynMapTree<Graph::Node *, size_t> colors;
  EXPECT_THROW(chromatic_number(g, colors), std::domain_error);
}

// ===================================================================
// Tests: List_SGraph type
// ===================================================================

TEST(GraphColoring, SGraphBasicColoring)
{
  SGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  auto *c = g.insert_node(3);
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, a);

  DynMapTree<SGraph::Node *, size_t> colors;

  size_t k = dsatur_coloring(g, colors);
  EXPECT_EQ(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, SGraphGreedy)
{
  SGraph g;
  auto *a = g.insert_node(1);
  auto *b = g.insert_node(2);
  auto *c = g.insert_node(3);
  auto *d = g.insert_node(4);
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, d);
  g.insert_arc(d, a);

  DynMapTree<SGraph::Node *, size_t> colors;

  size_t k = greedy_coloring(g, colors);
  EXPECT_LE(k, 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, DigraphSingleArcUsesTwoColors)
{
  DGraph g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  g.insert_arc(a, b);

  DynMapTree<DGraph::Node *, size_t> colors;

  EXPECT_EQ(greedy_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(welsh_powell_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(dsatur_coloring(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  EXPECT_EQ(chromatic_number(g, colors), 2u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, AGraphTriangle)
{
  AGraph g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  auto *c = g.insert_node("c");
  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, a);

  DynMapTree<AGraph::Node *, size_t> colors;

  EXPECT_EQ(dsatur_coloring(g, colors), 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
  EXPECT_EQ(chromatic_number(g, colors), 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: functor wrappers
// ===================================================================

TEST(GraphColoring, FunctorWrappers)
{
  auto g = build_complete_graph(4);
  DynMapTree<Graph::Node *, size_t> colors;

  Greedy_Coloring<Graph> greedy;
  size_t k1 = greedy(g, colors);
  EXPECT_EQ(k1, 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  Welsh_Powell_Coloring<Graph> wp;
  size_t k2 = wp(g, colors);
  EXPECT_EQ(k2, 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));

  DSatur_Coloring<Graph> dsat;
  size_t k3 = dsat(g, colors);
  EXPECT_EQ(k3, 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

// ===================================================================
// Tests: cookie preservation
// ===================================================================

TEST(GraphColoring, CookiesPreservedAfterColoring)
{
  Graph g;
  auto *a = g.insert_node("a");
  auto *b = g.insert_node("b");
  g.insert_arc(a, b);

  // Set cookies to known values
  int sentinel_a = 42;
  int sentinel_b = 99;
  NODE_COOKIE(a) = &sentinel_a;
  NODE_COOKIE(b) = &sentinel_b;

  DynMapTree<Graph::Node *, size_t> colors;
  dsatur_coloring(g, colors);

  // Cookies should be restored
  EXPECT_EQ(NODE_COOKIE(a), &sentinel_a);
  EXPECT_EQ(NODE_COOKIE(b), &sentinel_b);
}

// ===================================================================
// Illustrative example: map coloring (South America)
// ===================================================================

TEST(GraphColoring, SouthAmericaMapColoring)
{
  // Simplified South America adjacency
  Graph g;
  auto *br = g.insert_node("Brazil");
  auto *ar = g.insert_node("Argentina");
  auto *uy = g.insert_node("Uruguay");
  auto *py = g.insert_node("Paraguay");
  auto *bo = g.insert_node("Bolivia");
  auto *pe = g.insert_node("Peru");
  auto *cl = g.insert_node("Chile");
  auto *ec = g.insert_node("Ecuador");
  auto *co = g.insert_node("Colombia");
  auto *ve = g.insert_node("Venezuela");
  auto *gy = g.insert_node("Guyana");
  auto *sr = g.insert_node("Suriname");
  auto *gf = g.insert_node("French Guiana");

  // Borders
  g.insert_arc(br, uy);
  g.insert_arc(br, ar);
  g.insert_arc(br, py);
  g.insert_arc(br, bo);
  g.insert_arc(br, pe);
  g.insert_arc(br, co);
  g.insert_arc(br, ve);
  g.insert_arc(br, gy);
  g.insert_arc(br, sr);
  g.insert_arc(br, gf);
  g.insert_arc(ar, uy);
  g.insert_arc(ar, py);
  g.insert_arc(ar, bo);
  g.insert_arc(ar, cl);
  g.insert_arc(py, bo);
  g.insert_arc(bo, pe);
  g.insert_arc(bo, cl);
  g.insert_arc(pe, cl);
  g.insert_arc(pe, ec);
  g.insert_arc(pe, co);
  g.insert_arc(ec, co);
  g.insert_arc(co, ve);
  g.insert_arc(ve, gy);
  g.insert_arc(gy, sr);
  g.insert_arc(sr, gf);

  DynMapTree<Graph::Node *, size_t> colors;
  size_t k = dsatur_coloring(g, colors);

  EXPECT_TRUE(is_valid_coloring(g, colors));
  EXPECT_LE(k, 4u); // Four Color Theorem guarantees <= 4 for planar graphs
}

// ===================================================================
// Illustrative example: exam scheduling
// ===================================================================

TEST(GraphColoring, ExamScheduling)
{
  // Courses that share students cannot be scheduled at the same time.
  // Courses: Math, Physics, CS, Chemistry, Biology, English
  // Conflicts: Math-Physics, Math-CS, Physics-CS, Physics-Chemistry,
  //            Chemistry-Biology, Biology-English
  Graph g;
  auto *math    = g.insert_node("Math");
  auto *physics = g.insert_node("Physics");
  auto *cs      = g.insert_node("CS");
  auto *chem    = g.insert_node("Chemistry");
  auto *bio     = g.insert_node("Biology");
  auto *eng     = g.insert_node("English");

  g.insert_arc(math, physics);
  g.insert_arc(math, cs);
  g.insert_arc(physics, cs);
  g.insert_arc(physics, chem);
  g.insert_arc(chem, bio);
  g.insert_arc(bio, eng);

  DynMapTree<Graph::Node *, size_t> colors;
  size_t num_slots = dsatur_coloring(g, colors);

  EXPECT_TRUE(is_valid_coloring(g, colors));
  // The chromatic number for this graph is 3
  EXPECT_LE(num_slots, 3u);
}

// ===================================================================
// Tests: chromatic number for wheel graphs
// ===================================================================

TEST(GraphColoring, ChromaticNumberWheelEvenRing)
{
  // W5 = hub + C4; chromatic number = 3
  auto g = build_wheel(5);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 3u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}

TEST(GraphColoring, ChromaticNumberWheelOddRing)
{
  // W6 = hub + C5; chromatic number = 4
  auto g = build_wheel(6);
  DynMapTree<Graph::Node *, size_t> colors;

  EXPECT_EQ(chromatic_number(g, colors), 4u);
  EXPECT_TRUE(is_valid_coloring(g, colors));
}
