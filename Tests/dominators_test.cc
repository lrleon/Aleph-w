
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
 * @file dominators_test.cc
 * @brief Tests for Lengauer-Tarjan dominator tree algorithm.
 */

#include <gtest/gtest.h>
#include <chrono>
#include <cstdlib>
#include <map>
#include <string>
#include <random>

#include <Dominators.H>
#include <tpl_dynArray.H>
#include <tpl_dynMapTree.H>
#include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using DG = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  DynArray<DG::Node *> make_nodes(DG & g, int n)
  {
    DynArray<DG::Node *> nodes;
    nodes.reserve(static_cast<size_t>(n));
    for (int i = 0; i < n; ++i)
      nodes(static_cast<size_t>(i)) = g.insert_node(i);
    return nodes;
  }

  // Build idom map: node_info -> idom_info (-1 if root/no idom)
  DynMapTree<int, int> idom_map(const DynList<std::pair<DG::Node *,
                                                         DG::Node *>> & idoms)
  {
    DynMapTree<int, int> m;
    for (auto it = idoms.get_it(); it.has_curr(); it.next_ne())
      {
        auto [node, idom] = it.get_curr();
        m.insert(node->get_info(), idom ? idom->get_info() : -1);
      }
    return m;
  }

} // namespace


// ============================================================================
// Basic Structure Tests
// ============================================================================

TEST(Dominators, SingleNode)
{
  DG g;
  auto nodes = make_nodes(g, 1);

  auto idoms = compute_dominators(g, nodes[0]);
  auto m = idom_map(idoms);

  EXPECT_EQ(m.size(), 1U);
  EXPECT_EQ(m[0], -1); // root has no idom
}

TEST(Dominators, LinearChain)
{
  // 0 -> 1 -> 2 -> 3 -> 4
  // idom: 0=root, 1<-0, 2<-1, 3<-2, 4<-3
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);
  g.insert_arc(nodes[3], nodes[4]);

  auto m = idom_map(compute_dominators(g, nodes[0]));

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 1);
  EXPECT_EQ(m[3], 2);
  EXPECT_EQ(m[4], 3);
}

TEST(Dominators, DiamondGraph)
{
  //     0
  //    / |
  //   1   2
  //    \ /
  //     3
  // idom(1) = 0, idom(2) = 0, idom(3) = 0
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  auto m = idom_map(compute_dominators(g, nodes[0]));

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 0);
  EXPECT_EQ(m[3], 0);
}

TEST(Dominators, IfThenElseCFG)
{
  // Classic if-then-else CFG:
  //   0 (entry)
  //  / |
  // 1   2  (then / else)
  //  \ /
  //   3    (join)
  //   |
  //   4    (exit)
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);
  g.insert_arc(nodes[3], nodes[4]);

  auto m = idom_map(compute_dominators(g, nodes[0]));

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 0);
  EXPECT_EQ(m[3], 0);
  EXPECT_EQ(m[4], 3);
}

TEST(Dominators, LoopCFG)
{
  // CFG with a loop:
  //   0 (entry)
  //   |
  //   1 (loop header) <---+
  //  / |                   |
  // 2   3 (loop body) ----+
  // |
  // 4 (exit)
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[3], nodes[1]); // back-edge
  g.insert_arc(nodes[2], nodes[4]);

  auto m = idom_map(compute_dominators(g, nodes[0]));

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 1);
  EXPECT_EQ(m[3], 1);
  EXPECT_EQ(m[4], 2);
}

TEST(Dominators, ComplexTextbookCFG)
{
  // Classic textbook example (Appel "Modern Compiler Implementation")
  // 13 nodes (0..12), root = 0
  //
  // Edges:
  // 0->1, 0->5, 0->9
  // 1->2, 2->3, 3->4, 3->1 (back), 4->12
  // 5->6, 6->7, 6->8, 7->4, 8->12
  // 9->10, 10->11, 11->9 (back), 11->12
  DG g;
  auto n = make_nodes(g, 13);
  g.insert_arc(n[0], n[1]);
  g.insert_arc(n[0], n[5]);
  g.insert_arc(n[0], n[9]);
  g.insert_arc(n[1], n[2]);
  g.insert_arc(n[2], n[3]);
  g.insert_arc(n[3], n[4]);
  g.insert_arc(n[3], n[1]); // back-edge
  g.insert_arc(n[4], n[12]);
  g.insert_arc(n[5], n[6]);
  g.insert_arc(n[6], n[7]);
  g.insert_arc(n[6], n[8]);
  g.insert_arc(n[7], n[4]);
  g.insert_arc(n[8], n[12]);
  g.insert_arc(n[9], n[10]);
  g.insert_arc(n[10], n[11]);
  g.insert_arc(n[11], n[9]); // back-edge
  g.insert_arc(n[11], n[12]);

  auto m = idom_map(compute_dominators(g, n[0]));

  // Root
  EXPECT_EQ(m[0], -1);

  // Branch 1: 0->1->2->3
  EXPECT_EQ(m[1], 0);
  EXPECT_EQ(m[2], 1);
  EXPECT_EQ(m[3], 2);

  // Node 4: reachable from both 3 and 7; idom = 0
  EXPECT_EQ(m[4], 0);

  // Branch 2: 0->5->6->{7,8}
  EXPECT_EQ(m[5], 0);
  EXPECT_EQ(m[6], 5);
  EXPECT_EQ(m[7], 6);
  EXPECT_EQ(m[8], 6);

  // Branch 3: 0->9->10->11
  EXPECT_EQ(m[9], 0);
  EXPECT_EQ(m[10], 9);
  EXPECT_EQ(m[11], 10);

  // Node 12: reachable from 4, 8, 11; idom = 0
  EXPECT_EQ(m[12], 0);
}


// ============================================================================
// Unreachable Nodes and Edge Cases
// ============================================================================

TEST(Dominators, UnreachableNodes)
{
  // 0 -> 1 -> 2, but 3 is disconnected
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  // Node 3 is unreachable

  auto idoms = compute_dominators(g, nodes[0]);
  auto m = idom_map(idoms);

  // Only 3 reachable nodes
  EXPECT_EQ(m.size(), 3U);
  EXPECT_EQ(m.search(3), nullptr); // node 3 not in result
}

TEST(Dominators, SelfLoop)
{
  // 0 -> 1 -> 1 (self-loop)
  DG g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[1]); // self-loop

  auto m = idom_map(compute_dominators(g, nodes[0]));

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(m[1], 0);
}

TEST(Dominators, EmptyGraph)
{
  DG g;
  // Create algorithm instance for empty graph
  Lengauer_Tarjan_Dominators<DG> alg;
  // Empty graph has no nodes, so we can't call with a root.
  // The algorithm should handle this gracefully.
  EXPECT_TRUE(g.get_num_nodes() == 0);
}

TEST(Dominators, ErrorHandlingNullRoot)
{
  DG g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);

  EXPECT_THROW(compute_dominators(g, nullptr), std::domain_error);
}


// ============================================================================
// Dominator Tree Structure
// ============================================================================

TEST(Dominators, DominatorTreeStructure)
{
  //     0
  //    / |
  //   1   2
  //    \ /
  //     3
  //     |
  //     4
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);
  g.insert_arc(nodes[3], nodes[4]);

  DG tree;
  build_dominator_tree(g, nodes[0], tree);

  // Tree should have 5 nodes and 4 arcs (n-1 for a tree)
  EXPECT_EQ(tree.get_num_nodes(), 5U);
  EXPECT_EQ(tree.get_num_arcs(), 4U);

  // Verify node mapping
  for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto tn = it.get_curr();
      auto orig = mapped_node<DG>(tn);
      EXPECT_NE(orig, nullptr);
      EXPECT_EQ(tn->get_info(), orig->get_info());
    }
}

TEST(Dominators, DominatorTreeLinearChain)
{
  // 0->1->2->3: dom tree is the same chain
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);

  DG tree;
  build_dominator_tree(g, nodes[0], tree);

  EXPECT_EQ(tree.get_num_nodes(), 4U);
  EXPECT_EQ(tree.get_num_arcs(), 3U);
}


// ============================================================================
// Dominance Checks
// ============================================================================

TEST(Dominators, DominanceReflexive)
{
  DG g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);

  Lengauer_Tarjan_Dominators<DG> alg;

  // Every reachable node dominates itself
  for (int i = 0; i < 3; ++i)
    EXPECT_TRUE(alg.dominates(g, nodes[0], nodes[i], nodes[i]));
}

TEST(Dominators, RootDominatesAll)
{
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[4]);

  Lengauer_Tarjan_Dominators<DG> alg;

  for (int i = 0; i < 5; ++i)
    EXPECT_TRUE(alg.dominates(g, nodes[0], nodes[0], nodes[i]));
}

TEST(Dominators, NonDominance)
{
  //   0
  //  / |
  // 1   2
  //  \ /
  //   3
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  Lengauer_Tarjan_Dominators<DG> alg;

  // Node 1 does NOT dominate 3 (path 0->2->3 bypasses 1)
  EXPECT_FALSE(alg.dominates(g, nodes[0], nodes[1], nodes[3]));
  // Node 2 does NOT dominate 3 (path 0->1->3 bypasses 2)
  EXPECT_FALSE(alg.dominates(g, nodes[0], nodes[2], nodes[3]));
}


// ============================================================================
// Dominance Frontiers
// ============================================================================

TEST(Dominators, DominanceFrontiersIfThenElse)
{
  //   0 (entry)
  //  / |
  // 1   2
  //  \ /
  //   3 (join)
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  auto df = compute_dominance_frontiers(g, nodes[0]);

  // DF(0) = {} (root dominates everything reachable)
  EXPECT_TRUE(df.find(nodes[0]).is_empty());

  // DF(1) = {3} (1 dominates itself. 3 has predecessors 1 and 2.
  // 1 dominates pred 1 of 3, but 1 does NOT strictly dominate 3 (idom(3)=0).
  // So 3 is in DF(1).
  auto & df1 = df.find(nodes[1]);
  EXPECT_EQ(df1.size(), 1U);
  EXPECT_EQ(df1.get_first()->get_info(), 3);

  // DF(2) = {3}
  auto & df2 = df.find(nodes[2]);
  EXPECT_EQ(df2.size(), 1U);
  EXPECT_EQ(df2.get_first()->get_info(), 3);

  // DF(3) = {} (3 has only one predecessor in the idom sense)
  EXPECT_TRUE(df.find(nodes[3]).is_empty());
}

TEST(Dominators, DominanceFrontiersLoop)
{
  //   0 -> 1 -> 2
  //        ^    |
  //        +----+  (back-edge 2->1)
  DG g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[1]); // back-edge

  auto df = compute_dominance_frontiers(g, nodes[0]);

  // Node 1 is a loop header with predecessors 0 and 2.
  // DF(1) = {1} (1 dominates 2 which is pred of 1, but 1 doesn't strictly dom 1)
  auto & df1 = df.find(nodes[1]);
  EXPECT_EQ(df1.size(), 1U);
  EXPECT_EQ(df1.get_first()->get_info(), 1);

  // DF(2) = {1}
  auto & df2 = df.find(nodes[2]);
  EXPECT_EQ(df2.size(), 1U);
  EXPECT_EQ(df2.get_first()->get_info(), 1);
}


// ============================================================================
// get_idom tests
// ============================================================================

TEST(Dominators, GetIdomSpecificNode)
{
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);

  Lengauer_Tarjan_Dominators<DG> alg;

  DG::Node * n0 = nodes[0];
  DG::Node * n1 = nodes[1];
  DG::Node * n2 = nodes[2];
  DG::Node * n3 = nodes[3];

  EXPECT_EQ(alg.get_idom(g, n0, n0), nullptr); // root
  EXPECT_EQ(alg.get_idom(g, n0, n1), n0);
  EXPECT_EQ(alg.get_idom(g, n0, n2), n1);
  EXPECT_EQ(alg.get_idom(g, n0, n3), n2);
}

TEST(Dominators, GetIdomNullNodeThrows)
{
  DG g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);

  Lengauer_Tarjan_Dominators<DG> alg;
  EXPECT_THROW(alg.get_idom(g, nodes[0], nullptr), std::domain_error);
}


// ============================================================================
// Large / Random Graph
// ============================================================================

TEST(Dominators, LargeRandomDAG)
{
  // Build a random DAG with ~500 nodes and verify structural properties
  const int N = 500;
  DG g;
  auto nodes = make_nodes(g, N);

  std::mt19937 rng(42);

  // Add random forward edges (i -> j where j > i to ensure DAG)
  for (int i = 0; i < N - 1; ++i)
    {
      // Each node has at least one outgoing edge
      int j = i + 1 + static_cast<int>(rng() % std::min(5, N - i - 1));
      if (j < N)
        g.insert_arc(nodes[i], nodes[j]);

      // Add a few more random edges
      for (int k = 0; k < 2; ++k)
        {
          int t = i + 1 + static_cast<int>(rng() % (N - i - 1));
          if (t < N)
            g.insert_arc(nodes[i], nodes[t]);
        }
    }

  auto idoms = compute_dominators(g, nodes[0]);
  auto m = idom_map(idoms);

  // Root should have no idom
  EXPECT_EQ(m[0], -1);

  // Every non-root reachable node should have an idom
  for (auto it = m.get_it(); it.has_curr(); it.next_ne())
    {
      const auto & [node_id, idom_id] = it.get_curr();
      if (node_id == 0)
        continue;
      EXPECT_GE(idom_id, 0) << "Node " << node_id << " has no idom";
    }

  // Verify dominator tree is a proper tree
  DG tree;
  build_dominator_tree(g, nodes[0], tree);
  EXPECT_EQ(tree.get_num_nodes(), m.size());
  EXPECT_EQ(tree.get_num_arcs(), m.size() - 1);
}

TEST(Dominators, PerformanceLargeRandomDAG)
{
  const char * run_perf = std::getenv("ALEPH_RUN_DOMINATORS_PERF");
  if (run_perf == nullptr or std::string(run_perf) != "1")
    GTEST_SKIP() << "Set ALEPH_RUN_DOMINATORS_PERF=1 to run dominators perf test";

  constexpr int N = 5000;
# ifdef NDEBUG
  constexpr long long kBudgetMs = 9000;
# else
  constexpr long long kBudgetMs = 20000;
# endif

  DG g;
  auto nodes = make_nodes(g, N);
  std::mt19937 rng(0xD01A70u);

  for (int i = 0; i < N - 1; ++i)
    {
      const int max_step = std::min(12, N - i - 1);
      const int j = i + 1 + static_cast<int>(rng() % max_step);
      g.insert_arc(nodes[i], nodes[j]);

      for (int k = 0; k < 4; ++k)
        {
          const int t = i + 1 + static_cast<int>(rng() % (N - i - 1));
          g.insert_arc(nodes[i], nodes[t]);
        }
    }

  const auto t0 = std::chrono::steady_clock::now();
  auto idoms = compute_dominators(g, nodes[0]);
  auto m = idom_map(idoms);
  DG tree;
  build_dominator_tree(g, nodes[0], tree);
  const auto elapsed_ms = std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - t0).count();

  EXPECT_EQ(m[0], -1);
  EXPECT_EQ(tree.get_num_nodes(), m.size());
  EXPECT_EQ(tree.get_num_arcs(), m.size() - 1);
  EXPECT_LT(elapsed_ms, kBudgetMs)
      << "Dominators perf regression: N=" << N
      << ", elapsed_ms=" << elapsed_ms
      << ", budget_ms=" << kBudgetMs;
}


// ============================================================================
// Operator() alias
// ============================================================================

TEST(Dominators, OperatorAlias)
{
  DG g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);

  DG tree1, tree2;
  Lengauer_Tarjan_Dominators<DG> alg;

  alg.build_tree(g, nodes[0], tree1);
  alg(g, nodes[0], tree2);

  EXPECT_EQ(tree1.get_num_nodes(), tree2.get_num_nodes());
  EXPECT_EQ(tree1.get_num_arcs(), tree2.get_num_arcs());
}


// ============================================================================
// Cache reuse test
// ============================================================================

TEST(Dominators, CacheReuse)
{
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  Lengauer_Tarjan_Dominators<DG> alg;

  // First call computes
  auto m1 = idom_map(alg.compute_idom(g, nodes[0]));
  // Second call should reuse cache
  auto m2 = idom_map(alg.compute_idom(g, nodes[0]));

  EXPECT_EQ(m1, m2);
  EXPECT_TRUE(alg.has_computation());
  EXPECT_EQ(alg.get_graph(), &g);
  DG::Node * root = nodes[0];
  EXPECT_EQ(alg.get_root(), root);
}


// ============================================================================
// String-typed graph
// ============================================================================

TEST(Dominators, StringTypedGraph)
{
  using SDG = List_Digraph<Graph_Node<std::string>, Graph_Arc<int>>;

  SDG g;
  auto entry = g.insert_node("entry");
  auto body = g.insert_node("body");
  auto exit_node = g.insert_node("exit");

  g.insert_arc(entry, body);
  g.insert_arc(body, exit_node);

  auto idoms = compute_dominators(g, entry);

  std::map<std::string, std::string> m;
  for (auto it = idoms.get_it(); it.has_curr(); it.next_ne())
    {
      auto [node, idom] = it.get_curr();
      m[node->get_info()] = idom ? idom->get_info() : "none";
    }

  EXPECT_EQ(m["entry"], "none");
  EXPECT_EQ(m["body"], "entry");
  EXPECT_EQ(m["exit"], "body");
}


// ============================================================================
// Post-Dominator Tests
// ============================================================================

namespace
{
  // Build ipdom map: node_info -> ipdom_info (-1 if exit/no ipdom)
  std::map<int, int> ipdom_map(const DynList<std::pair<DG::Node*,
                                                        DG::Node*>> & ipdoms)
  {
    std::map<int, int> m;
    for (auto it = ipdoms.get_it(); it.has_curr(); it.next_ne())
      {
        auto [node, ipdom] = it.get_curr();
        m[node->get_info()] = ipdom ? ipdom->get_info() : -1;
      }
    return m;
  }
} // namespace

TEST(PostDominators, SingleNode)
{
  DG g;
  auto nodes = make_nodes(g, 1);

  auto ipdoms = compute_post_dominators(g, nodes[0]);
  auto m = ipdom_map(ipdoms);

  EXPECT_EQ(m.size(), 1U);
  EXPECT_EQ(m[0], -1); // exit has no ipdom
}

TEST(PostDominators, LinearChain)
{
  // 0 -> 1 -> 2 -> 3; exit = 3
  // ipdom(2) = 3, ipdom(1) = 2, ipdom(0) = 1
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);

  auto m = ipdom_map(compute_post_dominators(g, nodes[3]));

  EXPECT_EQ(m[3], -1);  // exit
  EXPECT_EQ(m[2], 3);
  EXPECT_EQ(m[1], 2);
  EXPECT_EQ(m[0], 1);
}

TEST(PostDominators, DiamondGraph)
{
  //     0
  //    / |
  //   1   2
  //    \ /
  //     3
  // exit = 3; ipdom(1) = 3, ipdom(2) = 3, ipdom(0) = 3
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  auto m = ipdom_map(compute_post_dominators(g, nodes[3]));

  EXPECT_EQ(m[3], -1);
  EXPECT_EQ(m[1], 3);
  EXPECT_EQ(m[2], 3);
  EXPECT_EQ(m[0], 3);
}

TEST(PostDominators, IfThenElseCFG)
{
  // Classic if-then-else CFG:
  //   0 (entry)
  //  / |
  // 1   2  (then / else)
  //  \ /
  //   3    (join)
  //   |
  //   4    (exit)
  // ipdom(3) = 4, ipdom(1) = 3, ipdom(2) = 3, ipdom(0) = 3
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);
  g.insert_arc(nodes[3], nodes[4]);

  auto m = ipdom_map(compute_post_dominators(g, nodes[4]));

  EXPECT_EQ(m[4], -1);
  EXPECT_EQ(m[3], 4);
  EXPECT_EQ(m[1], 3);
  EXPECT_EQ(m[2], 3);
  EXPECT_EQ(m[0], 3);
}

TEST(PostDominators, LoopCFG)
{
  // CFG with a loop:
  //   0 (entry)
  //   |
  //   1 (loop header) <---+
  //  / |                   |
  // 2   3 (loop body) ----+
  // |
  // 4 (exit)
  // exit = 4; ipdom(2) = 4, ipdom(1) = 2, ipdom(0) = 1
  // Node 3 cannot reach 4 (it only loops back to 1), so it may or may
  // not appear in the post-dominator result depending on reachability
  // in the reversed graph.
  DG g;
  auto nodes = make_nodes(g, 5);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[3], nodes[1]); // back-edge
  g.insert_arc(nodes[2], nodes[4]);

  auto m = ipdom_map(compute_post_dominators(g, nodes[4]));

  EXPECT_EQ(m[4], -1);
  EXPECT_EQ(m[2], 4);
  EXPECT_EQ(m[1], 2);
  EXPECT_EQ(m[0], 1);
}

TEST(PostDominators, PostDominanceReflexive)
{
  // Every node post-dominates itself
  //   0 -> 1 -> 2
  DG g;
  auto nodes = make_nodes(g, 3);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);

  Lengauer_Tarjan_Post_Dominators<DG> alg;

  for (int i = 0; i < 3; ++i)
    EXPECT_TRUE(alg.post_dominates(g, nodes[2], nodes[i], nodes[i]));
}

TEST(PostDominators, ExitPostDominatesAll)
{
  // The exit node post-dominates every node that can reach it
  //   0 -> 1 -> 2 -> 3
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);

  Lengauer_Tarjan_Post_Dominators<DG> alg;

  for (int i = 0; i < 4; ++i)
    EXPECT_TRUE(alg.post_dominates(g, nodes[3], nodes[3], nodes[i]));
}

TEST(PostDominators, PostDominanceFrontiers)
{
  // Diamond graph for PDF:
  //     0
  //    / |
  //   1   2
  //    \ /
  //     3
  // exit = 3
  // In reversed graph: 3->{1,2}, 1->0, 2->0. Root=3.
  // Dom on reversed: idom(1)=3, idom(2)=3, idom(0)=3
  // DF on reversed at node 0: {0 has preds 1,2 in rev. Runner from 1: 1!=idom(0)=3 → DF(1)+=0,
  //   runner=idom(1)=3=idom(0) stop. Runner from 2: 2!=3 → DF(2)+=0, runner=3 stop.}
  // PDF(1) = DF_rev(1) mapped back = {0}
  // PDF(2) = DF_rev(2) mapped back = {0}
  // PDF(0) = {} (only one "predecessor" in rev)
  // PDF(3) = {}
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[0], nodes[2]);
  g.insert_arc(nodes[1], nodes[3]);
  g.insert_arc(nodes[2], nodes[3]);

  auto pdf = compute_post_dominance_frontiers(g, nodes[3]);

  // PDF(3) = {} (exit)
  EXPECT_TRUE(pdf.find(nodes[3]).is_empty());

  // PDF(1) = {0}
  auto & pdf1 = pdf.find(nodes[1]);
  EXPECT_EQ(pdf1.size(), 1U);
  EXPECT_EQ(pdf1.get_first()->get_info(), 0);

  // PDF(2) = {0}
  auto & pdf2 = pdf.find(nodes[2]);
  EXPECT_EQ(pdf2.size(), 1U);
  EXPECT_EQ(pdf2.get_first()->get_info(), 0);

  // PDF(0) = {}
  EXPECT_TRUE(pdf.find(nodes[0]).is_empty());
}

TEST(PostDominators, PostDominatorTreeStructure)
{
  //   0 -> 1 -> 2 -> 3
  // exit = 3; post-dom tree: 3->2->1->0 (chain)
  DG g;
  auto nodes = make_nodes(g, 4);
  g.insert_arc(nodes[0], nodes[1]);
  g.insert_arc(nodes[1], nodes[2]);
  g.insert_arc(nodes[2], nodes[3]);

  DG tree;
  build_post_dominator_tree(g, nodes[3], tree);

  // Tree should have 4 nodes and 3 arcs (n-1)
  EXPECT_EQ(tree.get_num_nodes(), 4U);
  EXPECT_EQ(tree.get_num_arcs(), 3U);

  // Verify node mapping
  for (auto it = tree.get_node_it(); it.has_curr(); it.next_ne())
    {
      auto tn = it.get_curr();
      auto orig = mapped_node<DG>(tn);
      EXPECT_NE(orig, nullptr);
      EXPECT_EQ(tn->get_info(), orig->get_info());
    }
}

TEST(PostDominators, ErrorHandlingNullExit)
{
  DG g;
  auto nodes = make_nodes(g, 2);
  g.insert_arc(nodes[0], nodes[1]);

  EXPECT_THROW(compute_post_dominators(g, nullptr), std::domain_error);
}
