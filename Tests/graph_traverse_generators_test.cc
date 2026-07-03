/*
                          Aleph_w

  Data structures & Algorithms
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
 * @file graph_traverse_generators_test.cc
 * @brief Tests for lazy graph traversal (graph-traverse-generators.H).
 *
 * Property tests comparing Graph_Traverse_BFS_Generator/
 * Graph_Traverse_DFS_Generator against the eager Graph_Traverse_BFS/
 * Graph_Traverse_DFS from graph-traverse.H: same set of visited nodes,
 * same count, and (for BFS) the same level-by-level order. Also covers a
 * single-node graph, a disconnected graph (unreachable nodes are not
 * visited), early break, and a digraph.
 */

#include <set>
#include <stdexcept>
#include <type_traits>
#include <vector>

#include <gtest/gtest.h>

#include <graph-traverse-generators.H>
#include <graph-traverse.H>
#include <tpl_graph.H>

using namespace Aleph;

namespace
{
using TestGraph = List_Graph<Graph_Node<int>, Graph_Arc<double>>;
using TestDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using Itor = Node_Arc_Iterator<TestGraph>;
using DigraphItor = Node_Arc_Iterator<TestDigraph>;

template <class GT>
std::vector<int> node_keys(const std::vector<typename GT::Node *> &nodes)
{
  std::vector<int> keys;
  keys.reserve(nodes.size());
  for (typename GT::Node *n : nodes)
    keys.push_back(n->get_info());
  return keys;
}

// Diamond-shaped connected graph:
//     0 --- 1
//     |     |
//     2 --- 3 --- 4
TestGraph build_diamond(std::vector<TestGraph::Node *> &nodes)
{
  TestGraph g;
  for (int i = 0; i < 5; ++i)
    nodes.push_back(g.insert_node(i));
  g.insert_arc(nodes[0], nodes[1], 1.0);
  g.insert_arc(nodes[0], nodes[2], 2.0);
  g.insert_arc(nodes[1], nodes[3], 3.0);
  g.insert_arc(nodes[2], nodes[3], 4.0);
  g.insert_arc(nodes[3], nodes[4], 5.0);
  return g;
}
}  // namespace

// Compile-time regression: traverse() is &-ref-qualified specifically so
// that calling it on a temporary traverser — the dangerous one-liner
// `Graph_Traverse_BFS_Generator<GT,Itor>(g).traverse(start)`, whose
// coroutine frame would otherwise read a dangling `this` on first resume —
// is a compile error instead of undefined behavior. `std::is_invocable_v`
// (not a bare `requires{...}` expression: a ref-qualifier mismatch is a
// hard error, not a SFINAE-friendly substitution failure, so it would
// abort the whole translation unit instead of just making the requirement
// unsatisfied) checks this without ever attempting the invalid call.
using BfsGen = Graph_Traverse_BFS_Generator<TestGraph, Itor>;
static_assert(
  not std::is_invocable_v<decltype(&BfsGen::traverse), BfsGen, TestGraph::Node *>,
  "Graph_Traverse_Generator::traverse() must reject rvalue (temporary) receivers");
static_assert(
  std::is_invocable_v<decltype(&BfsGen::traverse), BfsGen &, TestGraph::Node *>,
  "Graph_Traverse_Generator::traverse() must still accept lvalue receivers");

TEST(GraphTraverseGenerators, SingleNodeGraph)
{
  TestGraph g;
  TestGraph::Node *n = g.insert_node(42);

  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  std::vector<int> seen;
  for (TestGraph::Node *v : bfs.traverse(n))
    seen.push_back(v->get_info());
  EXPECT_EQ(seen, (std::vector<int>{42}));
}

TEST(GraphTraverseGenerators, NullStartThrowsInvalidArgument)
{
  TestGraph g;

  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  EXPECT_THROW(
    {
      for (TestGraph::Node *v : bfs.traverse(nullptr))
        (void) v;
    },
    std::invalid_argument);
}

TEST(GraphTraverseGenerators, BfsVisitsAllReachableNodesExactlyOnce)
{
  std::vector<TestGraph::Node *> nodes;
  TestGraph g = build_diamond(nodes);

  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  std::vector<int> seen;
  for (TestGraph::Node *v : bfs.traverse(nodes[0]))
    seen.push_back(v->get_info());

  ASSERT_EQ(seen.size(), 5u);
  EXPECT_EQ(std::set<int>(seen.begin(), seen.end()),
            (std::set<int>{0, 1, 2, 3, 4}));
  EXPECT_EQ(seen[0], 0);  // start node is always first
}

TEST(GraphTraverseGenerators, BfsOrderMatchesEagerBfsOrder)
{
  std::vector<TestGraph::Node *> nodes;
  TestGraph g = build_diamond(nodes);

  Graph_Traverse_BFS_Generator<TestGraph, Itor> lazy_bfs(g);
  std::vector<int> lazy_seen;
  for (TestGraph::Node *v : lazy_bfs.traverse(nodes[0]))
    lazy_seen.push_back(v->get_info());

  Graph_Traverse_BFS<TestGraph, Itor> eager_bfs(g);
  std::vector<int> eager_seen;
  eager_bfs(nodes[0], [&](TestGraph::Node *v) {
    eager_seen.push_back(v->get_info());
    return true;
  });

  EXPECT_EQ(lazy_seen, eager_seen);
}

TEST(GraphTraverseGenerators, DfsOrderMatchesEagerDfsOrder)
{
  std::vector<TestGraph::Node *> nodes;
  TestGraph g = build_diamond(nodes);

  Graph_Traverse_DFS_Generator<TestGraph, Itor> lazy_dfs(g);
  std::vector<int> lazy_seen;
  for (TestGraph::Node *v : lazy_dfs.traverse(nodes[0]))
    lazy_seen.push_back(v->get_info());

  Graph_Traverse_DFS<TestGraph, Itor> eager_dfs(g);
  std::vector<int> eager_seen;
  eager_dfs(nodes[0], [&](TestGraph::Node *v) {
    eager_seen.push_back(v->get_info());
    return true;
  });

  EXPECT_EQ(lazy_seen, eager_seen);
}

TEST(GraphTraverseGenerators, DisconnectedGraphOnlyVisitsReachableComponent)
{
  // Component 1: 0 - 1 - 2      Component 2: 3 - 4
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;
  for (int i = 0; i < 5; ++i)
    nodes.push_back(g.insert_node(i));
  g.insert_arc(nodes[0], nodes[1], 1.0);
  g.insert_arc(nodes[1], nodes[2], 2.0);
  g.insert_arc(nodes[3], nodes[4], 3.0);

  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  std::vector<int> seen;
  for (TestGraph::Node *v : bfs.traverse(nodes[0]))
    seen.push_back(v->get_info());

  EXPECT_EQ(std::set<int>(seen.begin(), seen.end()), (std::set<int>{0, 1, 2}));
}

TEST(GraphTraverseGenerators, EarlyBreakStopsTraversal)
{
  std::vector<TestGraph::Node *> nodes;
  TestGraph g = build_diamond(nodes);

  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  int visited = 0;
  for (TestGraph::Node *v : bfs.traverse(nodes[0]))
    {
      (void) v;
      if (++visited == 2)
        break;
    }
  EXPECT_EQ(visited, 2);  // stopped well before all 5 nodes were visited
}

// Regression test: a self-loop at the start node must not be yielded
// twice, and must not prevent other reachable nodes from being visited.
// The initial frontier-seeding loop used to unconditionally overwrite the
// target node's state to Processing, which for a self-loop (tgt == start)
// clobbered start's already-Processed state.
TEST(GraphTraverseGenerators, SelfLoopAtStartNodeYieldsOnce)
{
  TestGraph g;
  TestGraph::Node *a = g.insert_node(1);
  TestGraph::Node *b = g.insert_node(2);
  g.insert_arc(a, a, 0.0);  // self-loop at the start node
  g.insert_arc(a, b, 1.0);

  std::vector<int> bfs_seen;
  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  for (TestGraph::Node *n : bfs.traverse(a))
    bfs_seen.push_back(n->get_info());
  EXPECT_EQ(bfs_seen, (std::vector<int>{1, 2}));

  std::vector<int> dfs_seen;
  Graph_Traverse_DFS_Generator<TestGraph, Itor> dfs(g);
  for (TestGraph::Node *n : dfs.traverse(a))
    dfs_seen.push_back(n->get_info());
  EXPECT_EQ(dfs_seen, (std::vector<int>{1, 2}));
}

// Same regression, but the self-loop is on a node discovered mid-traversal
// rather than on the start node itself (already handled correctly by the
// main loop's own guard; kept as a sibling test for symmetry/coverage).
TEST(GraphTraverseGenerators, SelfLoopMidTraversalYieldsOnce)
{
  TestGraph g;
  TestGraph::Node *a = g.insert_node(1);
  TestGraph::Node *b = g.insert_node(2);
  TestGraph::Node *c = g.insert_node(3);
  g.insert_arc(a, b, 1.0);
  g.insert_arc(b, b, 0.0);  // self-loop at a non-start node
  g.insert_arc(b, c, 1.0);

  std::vector<int> seen;
  Graph_Traverse_BFS_Generator<TestGraph, Itor> bfs(g);
  for (TestGraph::Node *n : bfs.traverse(a))
    seen.push_back(n->get_info());
  EXPECT_EQ(seen, (std::vector<int>{1, 2, 3}));
}

TEST(GraphTraverseGenerators, CyclicGraphTerminates)
{
  // 0 - 1 - 2 - 3 - 0 (cycle)
  TestGraph g;
  std::vector<TestGraph::Node *> nodes;
  for (int i = 0; i < 4; ++i)
    nodes.push_back(g.insert_node(i));
  g.insert_arc(nodes[0], nodes[1], 1.0);
  g.insert_arc(nodes[1], nodes[2], 2.0);
  g.insert_arc(nodes[2], nodes[3], 3.0);
  g.insert_arc(nodes[3], nodes[0], 4.0);

  Graph_Traverse_DFS_Generator<TestGraph, Itor> dfs(g);
  std::vector<int> seen;
  for (TestGraph::Node *v : dfs.traverse(nodes[0]))
    seen.push_back(v->get_info());

  ASSERT_EQ(seen.size(), 4u);
  EXPECT_EQ(std::set<int>(seen.begin(), seen.end()), (std::set<int>{0, 1, 2, 3}));
}

TEST(GraphTraverseGenerators, DigraphRespectsArcDirection)
{
  // 0 -> 1 -> 2 ; note: no arc back, and no arc from 2 anywhere.
  TestDigraph g;
  std::vector<TestDigraph::Node *> nodes;
  for (int i = 0; i < 3; ++i)
    nodes.push_back(g.insert_node(i));
  g.insert_arc(nodes[0], nodes[1], 1.0);
  g.insert_arc(nodes[1], nodes[2], 2.0);

  Graph_Traverse_BFS_Generator<TestDigraph, DigraphItor> bfs(g);
  std::vector<int> from0;
  for (TestDigraph::Node *v : bfs.traverse(nodes[0]))
    from0.push_back(v->get_info());
  EXPECT_EQ(from0, (std::vector<int>{0, 1, 2}));

  // Starting from node 2 (a sink), only node 2 itself is reachable.
  std::vector<int> from2;
  for (TestDigraph::Node *v : bfs.traverse(nodes[2]))
    from2.push_back(v->get_info());
  EXPECT_EQ(from2, (std::vector<int>{2}));
}
