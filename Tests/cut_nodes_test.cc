
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
 * @file cut_nodes_test.cc
 * @brief Tests for Cut Nodes
 */

#include <gtest/gtest.h>

#include <set>
#include <vector>

#include <tpl_cut_nodes.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

  std::vector<Graph::Node *> make_nodes(Graph & g, int n)
  {
    std::vector<Graph::Node *> nodes;
    nodes.reserve(n);
    for (int i = 0; i < n; ++i)
      nodes.push_back(g.insert_node(i));
    return nodes;
  }

  std::set<int> cut_infos(const DynDlist<Graph::Node *> & list)
  {
    std::set<int> s;
    for (typename DynDlist<Graph::Node *>::Iterator it(list); it.has_curr(); it.next_ne())
      s.insert(it.get_curr()->get_info());
    return s;
  }

  size_t count_nodes_with_color(const Graph & g, long color)
  {
    size_t n = 0;
    for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        if (get_color<Graph>(p) == color)
          ++n;
      }
    return n;
  }

  size_t count_arcs_with_color(const Graph & g, long color)
  {
    size_t n = 0;
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        if (get_color<Graph>(a) == color)
          ++n;
      }
    return n;
  }

  size_t count_cut_arcs(const Graph & g)
  {
    size_t n = 0;
    for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
      if (is_an_cut_arc<Graph>(it.get_curr()))
        ++n;
    return n;
  }

} // namespace

TEST(CutNodes, PathGraphHasInternalCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{1, 2}));
}

TEST(CutNodes, CycleGraphHasNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[0], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, StarGraphHasCenterAsCutNode)
{
  Graph g;
  auto nodes = make_nodes(g, 6);
  const int center = 0;
  for (int i = 1; i < 6; ++i)
    g.insert_arc(nodes[center], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[center], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{0}));
}

TEST(CutNodes, PaintSubgraphsRequiresCutNodesComputed)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);

  EXPECT_THROW((void)alg.paint_subgraphs(), std::logic_error);
}

TEST(CutNodes, PaintSubgraphsAndMapCutGraphOnStar)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  const long next_color = alg.paint_subgraphs();
  EXPECT_EQ(next_color, 5L); // 4 leaf blocks => colors 1..4, next=5

  EXPECT_TRUE(is_a_cut_node<Graph>(nodes[0]));
  EXPECT_EQ(get_color<Graph>(nodes[0]), 0L);
  for (int i = 1; i < 5; ++i)
    EXPECT_GT(get_color<Graph>(nodes[i]), 0L);

  Graph cut_graph;
  DynDlist<Graph::Arc *> cross;
  alg.map_cut_graph(cut_graph, cross);

  EXPECT_EQ(cut_graph.get_num_nodes(), 1U);
  EXPECT_EQ(cut_graph.get_num_arcs(), 0U);

  EXPECT_EQ(cross.size(), g.get_num_arcs());
  for (typename DynDlist<Graph::Arc *>::Iterator it(cross); it.has_curr(); it.next_ne())
    EXPECT_TRUE(is_a_cross_arc<Graph>(it.get_curr()));
}

TEST(CutNodes, MapSubgraphMatchesColorCounts)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  (void)alg.paint_subgraphs();

  // In this star, colors 1..4 correspond to single-node blocks.
  for (long color = 1; color <= 4; ++color)
    {
      Graph sg;
      alg.map_subgraph(sg, color);

      EXPECT_EQ(sg.get_num_nodes(), count_nodes_with_color(g, color));
      EXPECT_EQ(sg.get_num_arcs(), count_arcs_with_color(g, color));
    }
}

TEST(CutNodes, MapCutGraphMatchesCutArcCountOnPath)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  (void)alg.paint_subgraphs();

  Graph cut_graph;
  DynDlist<Graph::Arc *> cross;
  alg.map_cut_graph(cut_graph, cross);

  EXPECT_EQ(cut_graph.get_num_nodes(), cuts.size());
  EXPECT_EQ(cut_graph.get_num_arcs(), count_cut_arcs(g));
}

// ============================================================================
// Additional Graph Topologies
// ============================================================================

TEST(CutNodes, CompleteGraphHasNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  // Complete graph K4
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
      g.insert_arc(nodes[i], nodes[j], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, TreeAllInternalNodesAreCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 7);
  /*
   *       0
   *      / \
   *     1   2
   *    /|   |\
   *   3 4   5 6
   */
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[3], 1);
  g.insert_arc(nodes[1], nodes[4], 1);
  g.insert_arc(nodes[2], nodes[5], 1);
  g.insert_arc(nodes[2], nodes[6], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{0, 1, 2}));
}

TEST(CutNodes, BridgeGraphHasMultipleCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 6);
  // Two triangles connected by a bridge (nodes[2]--nodes[3])
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  g.insert_arc(nodes[2], nodes[3], 1); // bridge
  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{2, 3}));
}

TEST(CutNodes, BiconnectedGraphHasNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  // 4-cycle (biconnected)
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[0], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, ArticulationAtRootNode)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  // Root connects two separate components
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[2], 1); // triangle
  g.insert_arc(nodes[0], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1); // separate component

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cut_infos(cuts).count(0) > 0);
  EXPECT_TRUE(cut_infos(cuts).count(3) > 0);
}

// ============================================================================
// State Machine Tests
// ============================================================================

TEST(CutNodes, MapSubgraphWithoutPaintingThrows)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  Graph sg;
  EXPECT_THROW(alg.map_subgraph(sg, 1L), std::logic_error);
}

TEST(CutNodes, MapCutGraphWithoutPaintingThrows)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  Graph cg;
  DynDlist<Graph::Arc *> cross;
  EXPECT_THROW(alg.map_cut_graph(cg, cross), std::logic_error);
}

TEST(CutNodes, CorrectSequenceCutNodesPaintMap)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);

  // Step 1: compute cut nodes
  alg(nodes[0], cuts);
  EXPECT_FALSE(cuts.is_empty());

  // Step 2: paint
  long num_colors = alg.paint_subgraphs();
  EXPECT_GT(num_colors, 1L);

  // Step 3: map subgraph (should not throw)
  Graph sg;
  EXPECT_NO_THROW(alg.map_subgraph(sg, 1L));
}

TEST(CutNodes, MultipleCutNodesCallsReinitialize)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  Compute_Cut_Nodes<Graph> alg(g);

  DynDlist<Graph::Node *> cuts1;
  alg(nodes[0], cuts1);
  size_t first_count = cuts1.size();

  DynDlist<Graph::Node *> cuts2;
  alg(nodes[0], cuts2);

  EXPECT_EQ(cuts1.size(), cuts2.size());
  EXPECT_EQ(first_count, cuts2.size());
}

// ============================================================================
// Operator() Overload Tests
// ============================================================================

TEST(CutNodes, OperatorWithoutStartUsesFirstNode)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(cuts); // No start node specified

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{1, 2}));
}

TEST(CutNodes, OperatorWithSpecificStart)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[3], cuts); // Start from end

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{1, 2}));
}

// ============================================================================
// Painting Tests
// ============================================================================

TEST(CutNodes, PaintAssignsCorrectColors)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  // Star: center is cut node, leaves are separate blocks
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  long num_colors = alg.paint_subgraphs();
  EXPECT_GT(num_colors, 0);

  // Center (cut node) should have color 0
  EXPECT_EQ(get_color<Graph>(nodes[0]), 0L);

  // Each leaf should have a unique color > 0
  std::set<long> leaf_colors;
  for (int i = 1; i < 5; ++i)
    {
      long c = get_color<Graph>(nodes[i]);
      EXPECT_GT(c, 0L);
      leaf_colors.insert(c);
    }
  EXPECT_EQ(leaf_colors.size(), 4U);
}

TEST(CutNodes, CrossArcsIdentifiedCorrectly)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1); // cross arc from cut node 1
  g.insert_arc(nodes[1], nodes[2], 1); // cross arc from cut node 1

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  // All arcs in this path should be cross arcs
  size_t cross_count = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (is_a_cross_arc<Graph>(it.get_curr()))
      ++cross_count;

  EXPECT_EQ(cross_count, g.get_num_arcs());
}

TEST(CutNodes, CutArcsIdentifiedInBridge)
{
  Graph g;
  auto nodes = make_nodes(g, 6);
  // Two triangles with a bridge
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  auto bridge = g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  EXPECT_TRUE(is_an_cut_arc<Graph>(bridge));
}

TEST(CutNodes, NumberOfColorsMatchesComponents)
{
  Graph g;
  auto nodes = make_nodes(g, 7);
  // Tree with 4 leaf components
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[3], 1);
  g.insert_arc(nodes[1], nodes[4], 1);
  g.insert_arc(nodes[2], nodes[5], 1);
  g.insert_arc(nodes[2], nodes[6], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  long num_colors = alg.paint_subgraphs();

  // 4 leaves, each a separate block
  EXPECT_EQ(num_colors, 5L); // colors 1-4 used, next is 5
}

// ============================================================================
// Subgraph Mapping Tests
// ============================================================================

TEST(CutNodes, MapSubgraphWithInvalidColorThrows)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  Graph sg;
  EXPECT_THROW(alg.map_subgraph(sg, 999L), std::domain_error);
}

TEST(CutNodes, MappedSubgraphIsCorrect)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  // Each leaf is a single-node component
  for (long color = 1; color <= 4; ++color)
    {
      Graph sg;
      alg.map_subgraph(sg, color);

      EXPECT_EQ(sg.get_num_nodes(), 1U);
      EXPECT_EQ(sg.get_num_arcs(), 0U);
    }
}

TEST(CutNodes, BidirectionalMappingEstablished)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  Graph sg;
  alg.map_subgraph(sg, 1L);

  // Verify nodes are mapped
  for (auto it = sg.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto sg_node = it.get_curr();
      auto orig_node = mapped_node<Graph>(sg_node);
      EXPECT_NE(orig_node, nullptr);
    }
}

// ============================================================================
// Cut Graph Tests
// ============================================================================

TEST(CutNodes, CutGraphContainsOnlyCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  Graph cg;
  DynDlist<Graph::Arc *> cross;
  alg.map_cut_graph(cg, cross);

  EXPECT_EQ(cg.get_num_nodes(), cuts.size());
}

TEST(CutNodes, CrossArcsListMatchesGraph)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  Graph cg;
  DynDlist<Graph::Arc *> cross;
  alg.map_cut_graph(cg, cross);

  // Count cross arcs in original graph
  size_t expected_cross = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (is_a_cross_arc<Graph>(it.get_curr()))
      ++expected_cross;

  EXPECT_EQ(cross.size(), expected_cross);
}

// ============================================================================
// compute_blocks() Tests
// ============================================================================

TEST(CutNodes, ComputeBlocksWithoutCutNodesThrows)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1], 1);

  Compute_Cut_Nodes<Graph> alg(g);

  DynDlist<Graph> blocks;
  Graph cg;
  DynDlist<Graph::Arc *> cross;

  EXPECT_THROW(alg.compute_blocks(blocks, cg, cross), std::logic_error);
}

TEST(CutNodes, ComputeBlocksAutoPaints)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  DynDlist<Graph> blocks;
  Graph cg;
  DynDlist<Graph::Arc *> cross;

  // compute_blocks should auto-paint
  EXPECT_NO_THROW(alg.compute_blocks(blocks, cg, cross));
  EXPECT_GE(blocks.size(), 4U); // at least 4 leaf blocks (may have empty slots)
}

TEST(CutNodes, ComputeBlocksGeneratesAllBlocks)
{
  Graph g;
  auto nodes = make_nodes(g, 7);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[3], 1);
  g.insert_arc(nodes[1], nodes[4], 1);
  g.insert_arc(nodes[2], nodes[5], 1);
  g.insert_arc(nodes[2], nodes[6], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  DynDlist<Graph> blocks;
  Graph cg;
  DynDlist<Graph::Arc *> cross;
  alg.compute_blocks(blocks, cg, cross);

  EXPECT_GE(blocks.size(), 4U); // at least 4 leaf components (may have empty slots)
}

TEST(CutNodes, ComputeBlocksGeneratesCutGraph)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  DynDlist<Graph> blocks;
  Graph cg;
  DynDlist<Graph::Arc *> cross;
  alg.compute_blocks(blocks, cg, cross);

  EXPECT_EQ(cg.get_num_nodes(), cuts.size());
}

TEST(CutNodes, ComputeBlocksGeneratesCrossArcList)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  DynDlist<Graph> blocks;
  Graph cg;
  DynDlist<Graph::Arc *> cross;
  alg.compute_blocks(blocks, cg, cross);

  EXPECT_FALSE(cross.is_empty());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(CutNodes, SingleNodeHasNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, TwoNodesWithArcNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, TriangleNoCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_TRUE(cuts.is_empty());
}

TEST(CutNodes, SelfLoopDoesNotAffectCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[1], 99); // self-loop

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{1}));
}

TEST(CutNodes, ParallelArcsDoNotAffectCutNodes)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[1], 2); // parallel arc
  g.insert_arc(nodes[1], nodes[2], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cut_infos(cuts), (std::set<int>{1}));
}

// ============================================================================
// Integration Tests
// ============================================================================

TEST(CutNodes, CompleteWorkflowDetectPaintMapAll)
{
  Graph g;
  auto nodes = make_nodes(g, 5);
  for (int i = 1; i < 5; ++i)
    g.insert_arc(nodes[0], nodes[i], 1);

  Compute_Cut_Nodes<Graph> alg(g);

  // Step 1: Detect cut nodes
  DynDlist<Graph::Node *> cuts;
  alg(nodes[0], cuts);
  EXPECT_FALSE(cuts.is_empty());

  // Step 2: Paint
  long num_colors = alg.paint_subgraphs();
  EXPECT_GT(num_colors, 1L);

  // Step 3: Map all subgraphs
  for (long color = 1; color < num_colors; ++color)
    {
      Graph sg;
      EXPECT_NO_THROW(alg.map_subgraph(sg, color));
    }

  // Step 4: Map cut graph
  Graph cg;
  DynDlist<Graph::Arc *> cross;
  EXPECT_NO_THROW(alg.map_cut_graph(cg, cross));
}

TEST(CutNodes, MultipleIterationsOnSameGraph)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  Compute_Cut_Nodes<Graph> alg(g);

  for (int iter = 0; iter < 3; ++iter)
    {
      DynDlist<Graph::Node *> cuts;
      alg(nodes[0], cuts);
      EXPECT_EQ(cut_infos(cuts), (std::set<int>{1, 2}));
    }
}

TEST(CutNodes, LargeGraphStressTest)
{
  Graph g;
  const int N = 100;
  auto nodes = make_nodes(g, N);

  // Create a path graph with N-2 cut nodes
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);

  EXPECT_EQ(cuts.size(), static_cast<size_t>(N - 2));
}

TEST(CutNodes, ExceptionDuringMapSubgraphClearsGraph)
{
  Graph g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1], 1);

  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  alg.paint_subgraphs();

  Graph sg;
  // Attempt with invalid color should throw and clear sg
  try
    {
      alg.map_subgraph(sg, 999L);
    }
  catch (const std::domain_error &)
    {
      // Graph should be cleared on exception
      EXPECT_EQ(sg.get_num_nodes(), 0U);
      EXPECT_EQ(sg.get_num_arcs(), 0U);
    }
}

// ============================================================================
// Bridges (Compute_Bridges / find_bridges)
// ============================================================================

namespace
{
  // Collect bridge arcs into a set for easy membership testing.
  DynSetTree<Graph::Arc *> bridge_set(const DynList<Graph::Arc *> & lst)
  {
    DynSetTree<Graph::Arc *> s;
    for (typename DynList<Graph::Arc *>::Iterator it(lst); it.has_curr(); it.next_ne())
      s.insert(it.get_curr());
    return s;
  }
} // namespace

TEST(Bridges, PathGraphAllEdgesAreBridges)
{
  // 0 -- 1 -- 2 -- 3 : all 3 arcs are bridges
  Graph g;
  auto nodes = make_nodes(g, 4);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 1);
  auto a23 = g.insert_arc(nodes[2], nodes[3], 1);

  Compute_Bridges<Graph> cb(g);
  auto bridges = cb.find_bridges(nodes[0]);

  auto bs = bridge_set(bridges);
  EXPECT_EQ(bs.size(), 3U);
  EXPECT_TRUE(bs.exist(a01));
  EXPECT_TRUE(bs.exist(a12));
  EXPECT_TRUE(bs.exist(a23));
}

TEST(Bridges, CycleHasNoBridges)
{
  // 0 -- 1 -- 2 -- 3 -- 0 : no bridges
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[0], 1);

  auto bridges = find_bridges(g);
  EXPECT_TRUE(bridges.is_empty());
}

TEST(Bridges, TwoTrianglesOneBridge)
{
  // Triangles 0-1-2-0 and 3-4-5-3, connected by bridge 2--3
  Graph g;
  auto nodes = make_nodes(g, 6);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  auto bridge = g.insert_arc(nodes[2], nodes[3], 1); // bridge
  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  auto bridges = find_bridges(g, nodes[0]);
  EXPECT_EQ(bridges.size(), 1U);
  EXPECT_TRUE(bridge_set(bridges).exist(bridge));
}

TEST(Bridges, TreeAllEdgesAreBridges)
{
  /*
   *       0
   *      / \
   *     1   2
   *    / \
   *   3   4
   */
  Graph g;
  auto nodes = make_nodes(g, 5);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a02 = g.insert_arc(nodes[0], nodes[2], 1);
  auto a13 = g.insert_arc(nodes[1], nodes[3], 1);
  auto a14 = g.insert_arc(nodes[1], nodes[4], 1);

  auto bridges = find_bridges(g);
  auto bs = bridge_set(bridges);
  EXPECT_EQ(bs.size(), 4U);
  EXPECT_TRUE(bs.exist(a01));
  EXPECT_TRUE(bs.exist(a02));
  EXPECT_TRUE(bs.exist(a13));
  EXPECT_TRUE(bs.exist(a14));
}

TEST(Bridges, CompleteK4NoBridges)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
      g.insert_arc(nodes[i], nodes[j], 1);

  auto bridges = find_bridges(g);
  EXPECT_TRUE(bridges.is_empty());
}

TEST(Bridges, SingleNodeNoBridges)
{
  Graph g;
  auto nodes = make_nodes(g, 1);
  auto bridges = find_bridges(g, nodes[0]);
  EXPECT_TRUE(bridges.is_empty());
}

TEST(Bridges, TwoNodesOneBridge)
{
  Graph g;
  auto nodes = make_nodes(g, 2);
  auto b = g.insert_arc(nodes[0], nodes[1], 1);

  auto bridges = find_bridges(g);
  EXPECT_EQ(bridges.size(), 1U);
  EXPECT_TRUE(bridge_set(bridges).exist(b));
}

TEST(Bridges, ParallelArcsNotBridges)
{
  // Two parallel arcs between nodes 0 and 1: neither is a bridge
  Graph g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[0], nodes[1], 2); // parallel arc

  auto bridges = find_bridges(g);
  EXPECT_TRUE(bridges.is_empty());
}

TEST(Bridges, FreeFunctionConsistentWithClass)
{
  // Triangle + pendant edge: only pendant is a bridge
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  auto pend = g.insert_arc(nodes[2], nodes[3], 1); // bridge

  Compute_Bridges<Graph> cb(g);
  auto b_class = cb.find_bridges(nodes[0]);
  auto b_free  = find_bridges(g, nodes[0]);

  EXPECT_EQ(b_class.size(), b_free.size());
  EXPECT_EQ(b_class.size(), 1U);
  EXPECT_TRUE(bridge_set(b_class).exist(pend));
}

TEST(Bridges, OperatorCallConsistentWithMethod)
{
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  Compute_Bridges<Graph> cb(g);
  auto b1 = cb.find_bridges();
  auto b2 = cb(); // operator()()

  EXPECT_EQ(b1.size(), b2.size());
  EXPECT_EQ(b1.size(), 3U);
}

TEST(Bridges, MultipleCallsSameResult)
{
  // Two triangles joined by a single bridge
  Graph g;
  auto nodes = make_nodes(g, 6);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  g.insert_arc(nodes[2], nodes[3], 1); // bridge
  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  Compute_Bridges<Graph> cb(g);
  auto b1 = cb.find_bridges();
  auto b2 = cb.find_bridges();

  EXPECT_EQ(b1.size(), 1U);
  EXPECT_EQ(b2.size(), 1U);
}

TEST(Bridges, StressTestPathGraph)
{
  // Path of 100 nodes: all 99 edges are bridges
  Graph g;
  const int N = 100;
  auto nodes = make_nodes(g, N);
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  auto bridges = find_bridges(g);
  EXPECT_EQ(bridges.size(), static_cast<size_t>(N - 1));
}

TEST(Bridges, ConsistentWithCutArcOnPathGraph)
{
  // Path 0-1-2-3: every edge is also a "cut arc" after compute_cut_nodes+paint
  Graph g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[3], 1);

  // Bridges via Compute_Bridges
  auto bridges = find_bridges(g, nodes[0]);
  EXPECT_EQ(bridges.size(), 3U);

  // Articulation points via Compute_Cut_Nodes: nodes 1 and 2
  DynDlist<Graph::Node *> cuts;
  Compute_Cut_Nodes<Graph> alg(g);
  alg(nodes[0], cuts);
  EXPECT_EQ(cuts.size(), 2U); // nodes 1 and 2
}

TEST(Bridges, StarGraphAllEdgesBridges)
{
  // Star: center (0) -- leaves 1..4; all 4 arcs are bridges
  Graph g;
  auto nodes = make_nodes(g, 5);
  DynList<Graph::Arc *> arcs;
  for (int i = 1; i < 5; ++i)
    arcs.append(g.insert_arc(nodes[0], nodes[i], 1));

  auto bridges = find_bridges(g, nodes[0]);
  auto bs = bridge_set(bridges);
  EXPECT_EQ(bs.size(), 4U);
  arcs.for_each([&](Graph::Arc * a) { EXPECT_TRUE(bs.exist(a)); });
}

TEST(Bridges, SelfLoopDoesNotAffectBridges)
{
  // Path 0-1-2 with a self-loop on node 1: bridges are still 0-1 and 1-2
  Graph g;
  auto nodes = make_nodes(g, 3);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[1], nodes[1], 99); // self-loop on 1

  auto bridges = find_bridges(g, nodes[0]);
  auto bs = bridge_set(bridges);
  EXPECT_EQ(bs.size(), 2U);
  EXPECT_TRUE(bs.exist(a01));
  EXPECT_TRUE(bs.exist(a12));
}

TEST(Bridges, ResultIsIndependentOfStartNode)
{
  // Two triangles connected by a bridge (2--3).
  // Regardless of where the DFS starts, only that arc is a bridge.
  Graph g;
  auto nodes = make_nodes(g, 6);
  g.insert_arc(nodes[0], nodes[1], 1);
  g.insert_arc(nodes[1], nodes[2], 1);
  g.insert_arc(nodes[2], nodes[0], 1);
  auto bridge = g.insert_arc(nodes[2], nodes[3], 1);
  g.insert_arc(nodes[3], nodes[4], 1);
  g.insert_arc(nodes[4], nodes[5], 1);
  g.insert_arc(nodes[5], nodes[3], 1);

  for (int start = 0; start < 6; ++start)
    {
      auto bridges = find_bridges(g, nodes[start]);
      auto bs = bridge_set(bridges);
      EXPECT_EQ(bs.size(), 1U) << "start=" << start;
      EXPECT_TRUE(bs.exist(bridge)) << "start=" << start;
    }
}

TEST(Bridges, NonBridgeEdgesAbsentFromList)
{
  // Triangle 0-1-2-0 + pendant 2-3.
  // The 3 triangle arcs must NOT appear; only the pendant is a bridge.
  Graph g;
  auto nodes = make_nodes(g, 4);
  auto t01 = g.insert_arc(nodes[0], nodes[1], 1); // triangle, not bridge
  auto t12 = g.insert_arc(nodes[1], nodes[2], 1); // triangle, not bridge
  auto t20 = g.insert_arc(nodes[2], nodes[0], 1); // triangle, not bridge
  auto pend = g.insert_arc(nodes[2], nodes[3], 1); // bridge

  auto bridges = find_bridges(g);
  auto bs = bridge_set(bridges);

  EXPECT_EQ(bs.size(), 1U);
  EXPECT_TRUE(bs.exist(pend));
  EXPECT_FALSE(bs.exist(t01));
  EXPECT_FALSE(bs.exist(t12));
  EXPECT_FALSE(bs.exist(t20));
}

TEST(Bridges, EmptyGraphNoArgReturnsEmpty)
{
  // Empty graph: find_bridges() must return empty list without throwing.
  Graph g;
  EXPECT_TRUE(find_bridges(g).is_empty());
}

TEST(Bridges, DisconnectedGraphBridgesFromAllComponents)
{
  // Component A: path 0-1-2 (both arcs are bridges).
  // Component B: path 3-4-5 (both arcs are bridges).
  // The two components are not connected, so there are 4 bridges total.
  Graph g;
  auto nodes = make_nodes(g, 6);
  auto a01 = g.insert_arc(nodes[0], nodes[1], 1);
  auto a12 = g.insert_arc(nodes[1], nodes[2], 1);
  auto a34 = g.insert_arc(nodes[3], nodes[4], 1);
  auto a45 = g.insert_arc(nodes[4], nodes[5], 1);

  // Start from a node in component A; component B must still be covered.
  auto bridges = find_bridges(g, nodes[0]);
  auto bs = bridge_set(bridges);
  EXPECT_EQ(bs.size(), 4U);
  EXPECT_TRUE(bs.exist(a01));
  EXPECT_TRUE(bs.exist(a12));
  EXPECT_TRUE(bs.exist(a34));
  EXPECT_TRUE(bs.exist(a45));
}
