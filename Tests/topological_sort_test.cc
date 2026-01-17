
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/**
 * @file topological_sort_test.cc
 * @brief Tests for Topological Sort
 */

#include <gtest/gtest.h>
#include <set>
#include <map>
#include <string>

#include <topological_sort.H>
#include <tpl_graph.H>

using TestDigraph = Aleph::List_Digraph<Aleph::Graph_Node<std::string>,
                                    Aleph::Graph_Arc<int>>;

template <class List>
bool is_valid_topological_order(const TestDigraph & g, const List & order)
{
  // Build position map
  std::map<TestDigraph::Node*, size_t> pos;
  size_t idx = 0;
  for (auto it = order.get_it(); it.has_curr(); it.next_ne(), ++idx)
    pos[it.get_curr()] = idx;

  // Check all arcs: src must come before tgt
  for (Aleph::Arc_Iterator<TestDigraph> it(g); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);

      if (pos.find(src) == pos.end() || pos.find(tgt) == pos.end())
        return false;

      if (pos[src] >= pos[tgt])
        return false;
    }

  return true;
}

// ==================== Topological_Sort (DFS) Tests ====================

TEST(TopologicalSortDFS, EmptyGraph)
{
  TestDigraph g;
  Aleph::Topological_Sort<TestDigraph> sorter;

  auto result = sorter.perform<Aleph::DynList>(g);
  EXPECT_TRUE(result.is_empty());
}

TEST(TopologicalSortDFS, SingleNode)
{
  TestDigraph g;
  auto n = g.insert_node("A");

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result.get_first(), n);
}

TEST(TopologicalSortDFS, TwoNodesOneArc)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 2);
  EXPECT_TRUE(is_valid_topological_order(g, result));
  // A must come before B
  EXPECT_EQ(result.get_first(), a);
  EXPECT_EQ(result.get_last(), b);
}

TEST(TopologicalSortDFS, LinearChain)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, d);

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 4);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(TopologicalSortDFS, DiamondDAG)
{
  //     A
  //    / \
  //   B   C
  //    \ /
  //     D
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(c, d);

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 4);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(TopologicalSortDFS, DisconnectedComponents)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);  // Component 1
  g.insert_arc(c, d);  // Component 2

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 4);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(TopologicalSortDFS, ComplexDAG)
{
  //   A -> B -> D
  //   |    |    |
  //   v    v    v
  //   C -> E -> F
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  auto f = g.insert_node("F");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(b, e);
  g.insert_arc(c, e);
  g.insert_arc(d, f);
  g.insert_arc(e, f);

  Aleph::Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 6);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(TopologicalSortDFS, OperatorOverload)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Topological_Sort<TestDigraph> sorter;
  Aleph::DynDlist<TestDigraph::Node*> result;
  sorter(g, result);

  ASSERT_EQ(result.size(), 2);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

// ==================== Q_Topological_Sort (BFS/Kahn) Tests ====================

TEST(QTopologicalSortBFS, EmptyGraph)
{
  TestDigraph g;
  Aleph::Q_Topological_Sort<TestDigraph> sorter;

  auto result = sorter.perform<Aleph::DynList>(g);
  EXPECT_TRUE(result.is_empty());
}

TEST(QTopologicalSortBFS, SingleNode)
{
  TestDigraph g;
  auto n = g.insert_node("A");

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 1);
  EXPECT_EQ(result.get_first(), n);
}

TEST(QTopologicalSortBFS, TwoNodesOneArc)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 2);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(QTopologicalSortBFS, LinearChain)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, d);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 4);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

TEST(QTopologicalSortBFS, DiamondDAG)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(c, d);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.perform<Aleph::DynList>(g);

  ASSERT_EQ(result.size(), 4);
  EXPECT_TRUE(is_valid_topological_order(g, result));
}

// ==================== Ranks Tests ====================

TEST(QTopologicalSortRanks, EmptyGraph)
{
  TestDigraph g;
  Aleph::Q_Topological_Sort<TestDigraph> sorter;

  auto result = sorter.ranks(g);
  EXPECT_TRUE(result.is_empty());
}

TEST(QTopologicalSortRanks, SingleNode)
{
  TestDigraph g;
  auto n = g.insert_node("A");

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.ranks(g);

  ASSERT_EQ(result.size(), 1);
  ASSERT_EQ(result.get_first().size(), 1);
  EXPECT_EQ(result.get_first().get_first(), n);
}

TEST(QTopologicalSortRanks, LinearChain)
{
  TestDigraph g;
  g.insert_node("A");
  g.insert_node("B");
  g.insert_node("C");

  auto nodes = g.nodes();
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.ranks(g);

  // Each node should be in its own rank
  ASSERT_EQ(result.size(), 3);
  for (auto it = result.get_it(); it.has_curr(); it.next_ne())
    EXPECT_EQ(it.get_curr().size(), 1);
}

TEST(QTopologicalSortRanks, ParallelNodes)
{
  //   A
  //  /|\
  // B C D
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(a, d);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.ranks(g);

  ASSERT_EQ(result.size(), 2);

  // First rank should have A
  EXPECT_EQ(result.get_first().size(), 1);
  EXPECT_EQ(result.get_first().get_first(), a);

  // Second rank should have B, C, D (in some order)
  EXPECT_EQ(result.get_last().size(), 3);
}

TEST(QTopologicalSortRanks, DiamondRanks)
{
  //     A       <- rank 0
  //    / \
  //   B   C     <- rank 1
  //    \ /
  //     D       <- rank 2
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(c, d);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.ranks(g);

  ASSERT_EQ(result.size(), 3);

  auto it = result.get_it();

  // Rank 0: A
  EXPECT_EQ(it.get_curr().size(), 1);
  EXPECT_EQ(it.get_curr().get_first(), a);
  it.next_ne();

  // Rank 1: B, C
  EXPECT_EQ(it.get_curr().size(), 2);
  it.next_ne();

  // Rank 2: D
  EXPECT_EQ(it.get_curr().size(), 1);
  EXPECT_EQ(it.get_curr().get_first(), d);
}

TEST(QTopologicalSortRanks, DisconnectedComponents)
{
  // Two separate chains: A->B and C->D
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b);
  g.insert_arc(c, d);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto result = sorter.ranks(g);

  ASSERT_EQ(result.size(), 2);

  // Rank 0: A, C (both sources)
  EXPECT_EQ(result.get_first().size(), 2);

  // Rank 1: B, D
  EXPECT_EQ(result.get_last().size(), 2);
}

// ==================== Operator Overloads Tests ====================

TEST(QTopologicalSortOperators, PerformOverload)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  Aleph::DynDlist<TestDigraph::Node*> result;
  sorter(g, result);

  ASSERT_EQ(result.size(), 2);
}

TEST(QTopologicalSortOperators, RanksOverloadDynDlist)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  Aleph::DynDlist<Aleph::DynList<TestDigraph::Node*>> result;
  sorter(g, result);

  ASSERT_EQ(result.size(), 2);
}

TEST(QTopologicalSortOperators, RanksOverloadDynList)
{
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b);

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  Aleph::DynList<Aleph::DynList<TestDigraph::Node*>> result;
  sorter(g, result);

  ASSERT_EQ(result.size(), 2);
}

// ==================== Stress Tests ====================

TEST(TopologicalSortStress, LargeDAG)
{
  TestDigraph g;
  constexpr size_t N = 1000;

  // Create a long chain
  Aleph::DynArray<TestDigraph::Node*> nodes;
  for (size_t i = 0; i < N; ++i)
    nodes.append(g.insert_node(std::to_string(i)));

  for (size_t i = 0; i < N - 1; ++i)
    g.insert_arc(nodes(i), nodes(i + 1));

  // DFS version
  {
    Aleph::Topological_Sort<TestDigraph> sorter;
    auto result = sorter.perform<Aleph::DynList>(g);
    ASSERT_EQ(result.size(), N);
    EXPECT_TRUE(is_valid_topological_order(g, result));
  }

  // BFS version
  {
    Aleph::Q_Topological_Sort<TestDigraph> sorter;
    auto result = sorter.perform<Aleph::DynList>(g);
    ASSERT_EQ(result.size(), N);
    EXPECT_TRUE(is_valid_topological_order(g, result));
  }
}

TEST(TopologicalSortStress, WideDAG)
{
  TestDigraph g;
  constexpr size_t WIDTH = 100;

  // Single source with many sinks
  auto source = g.insert_node("source");
  for (size_t i = 0; i < WIDTH; ++i)
    {
      auto sink = g.insert_node(std::to_string(i));
      g.insert_arc(source, sink);
    }

  Aleph::Q_Topological_Sort<TestDigraph> sorter;
  auto ranks = sorter.ranks(g);

  ASSERT_EQ(ranks.size(), 2);
  EXPECT_EQ(ranks.get_first().size(), 1);
  EXPECT_EQ(ranks.get_last().size(), WIDTH);
}

// ==================== Comparison Tests ====================

TEST(TopologicalSortComparison, BothAlgorithmsProduceValidOrder)
{
  // Complex DAG
  TestDigraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  auto e = g.insert_node("E");
  auto f = g.insert_node("F");

  g.insert_arc(a, b);
  g.insert_arc(a, c);
  g.insert_arc(b, d);
  g.insert_arc(c, d);
  g.insert_arc(d, e);
  g.insert_arc(d, f);

  Aleph::Topological_Sort<TestDigraph> dfs_sorter;
  auto dfs_result = dfs_sorter.perform<Aleph::DynList>(g);

  Aleph::Q_Topological_Sort<TestDigraph> bfs_sorter;
  auto bfs_result = bfs_sorter.perform<Aleph::DynList>(g);

  // Both should have all nodes
  EXPECT_EQ(dfs_result.size(), 6);
  EXPECT_EQ(bfs_result.size(), 6);

  // Both should produce valid orderings (may differ)
  EXPECT_TRUE(is_valid_topological_order(g, dfs_result));
  EXPECT_TRUE(is_valid_topological_order(g, bfs_result));
}

