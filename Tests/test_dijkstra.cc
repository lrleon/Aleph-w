/**
 * GoogleTest suite for Dijkstra.H
 *
 * This file contains exhaustive tests to verify the correctness
 * of Dijkstra's shortest path algorithm implementation in Aleph-w.
 */

#include <gtest/gtest.h>

#include <functional>
#include <limits>
#include <queue>
#include <random>
#include <unordered_map>
#include <utility>
#include <vector>

#include <Dijkstra.H>
#include <tpl_graph.H>

using namespace Aleph;

// Graph type for tests (undirected graph with int node info and int arc weights)
using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using Node = GT::Node;
using Arc = GT::Arc;

// Directed graph type for some tests
using DGT = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using DNode = DGT::Node;
using DArc = DGT::Arc;

namespace {

template <class G>
size_t count_painted_nodes(const G &g)
{
  size_t n = 0;
  for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
    if (IS_NODE_VISITED(it.get_current_node_ne(), Aleph::Spanning_Tree))
      ++n;
  return n;
}

template <class G>
size_t count_painted_arcs(const G &g)
{
  size_t n = 0;
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    if (IS_ARC_VISITED(it.get_current_arc_ne(), Aleph::Spanning_Tree))
      ++n;
  return n;
}

template <class G>
int path_cost_by_info(const Path<G> &path)
{
  int total = 0;
  path.for_each_arc([&total](auto *a) { total += a->get_info(); });
  return total;
}

template <class G>
bool path_is_consistent(const Path<G> &path)
{
  if (path.is_empty())
    return true;

  const auto &g = path.get_graph();
  auto it = path.get_it();

  while (it.has_current_arc())
    {
      auto *src = it.get_current_node_ne();
      auto *arc = it.get_current_arc_ne();
      it.next_ne();
      auto *tgt = it.get_current_node_ne();

      if (g.is_digraph())
        {
          if (g.get_src_node(arc) != src || g.get_tgt_node(arc) != tgt)
            return false;
        }
      else if (g.get_connected_node(arc, src) != tgt)
        return false;
    }

  return true;
}

template <class G>
struct Scaled_Dist
{
  using Distance_Type = int;

  int factor = 1;
  Scaled_Dist() = default;
  explicit Scaled_Dist(int f) : factor(f) {}

  int operator()(typename G::Arc *a) const noexcept
  {
    return a->get_info() * factor;
  }
};

template <class G>
std::unordered_map<typename G::Node *, size_t>
index_nodes(const std::vector<typename G::Node *> &nodes)
{
  std::unordered_map<typename G::Node *, size_t> idx;
  idx.reserve(nodes.size());
  for (size_t i = 0; i < nodes.size(); ++i)
    idx.emplace(nodes[i], i);
  return idx;
}

template <class G>
std::vector<long long>
reference_dijkstra(const G &g,
                   const std::vector<typename G::Node *> &nodes,
                   typename G::Node *start)
{
  const auto idx = index_nodes<G>(nodes);
  const long long INF = std::numeric_limits<long long>::max() / 4;

  std::vector<std::vector<std::pair<size_t, int>>> adj(nodes.size());
  for (auto it = g.get_arc_it(); it.has_curr(); it.next_ne())
    {
      auto *a = it.get_current_arc_ne();
      auto *src = g.get_src_node(a);
      auto *tgt = g.get_tgt_node(a);
      const int w = a->get_info();
      const size_t u = idx.at(src);
      const size_t v = idx.at(tgt);
      adj[u].push_back({v, w});
      if (not g.is_digraph())
        adj[v].push_back({u, w});
    }

  std::vector<long long> dist(nodes.size(), INF);
  using Item = std::pair<long long, size_t>;
  std::priority_queue<Item, std::vector<Item>, std::greater<Item>> pq;

  dist[idx.at(start)] = 0;
  pq.push({0, idx.at(start)});

  while (not pq.empty())
    {
      const auto [d, u] = pq.top();
      pq.pop();
      if (d != dist[u])
        continue;

      for (const auto &[v, w] : adj[u])
        {
          const long long nd = d + static_cast<long long>(w);
          if (nd < dist[v])
            {
              dist[v] = nd;
              pq.push({nd, v});
            }
        }
    }

  return dist;
}

} // namespace

// ========== TEST 1: Simple Graph Shortest Paths ==========
TEST(DijkstraTest, SimpleGraphShortestPaths) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  // Tree should have same number of nodes as g
  ASSERT_EQ(tree.get_num_nodes(), g.get_num_nodes());

  // Tree should have n-1 arcs (spanning tree)
  ASSERT_EQ(tree.get_num_arcs(), g.get_num_nodes() - 1);
}

// ========== TEST 2: Find Shortest Path ==========
TEST(DijkstraTest, FindShortestPath) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 6);
  g.insert_arc(n2, n3, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n3, path);

  // Shortest path from n0 to n3: n0 -> n1 -> n2 -> n3 = 1 + 2 + 1 = 4
  ASSERT_EQ(cost, 4);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 3: Compute Partial Min Paths Tree ==========
TEST(DijkstraTest, ComputePartialMinPathsTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n3, n4, 1);
  g.insert_arc(n0, n4, 10); // Long direct path

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_partial_min_paths_tree(g, n0, n2, tree);

  // Tree should contain at least n0, n1, n2
  ASSERT_GE(tree.get_num_nodes(), 3u);
}

// ========== TEST 4: Paint Min Paths Tree ==========
TEST(DijkstraTest, PaintMinPathsTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 4);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n1, n3, 5);
  g.insert_arc(n2, n3, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  dijkstra.paint_min_paths_tree(g, n0);

  // Verify that spanning tree arcs are marked
  int painted_arcs = 0;
  for (Arc_Iterator<GT> it(g); it.has_curr(); it.next()) {
    if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree)) {
      painted_arcs++;
    }
  }

  // The spanning tree must have n-1 arcs
  ASSERT_EQ(painted_arcs, 3);
}

// ========== TEST 5: Paint Partial Min Paths Tree ==========
TEST(DijkstraTest, PaintPartialMinPathsTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Dijkstra_Min_Paths<GT> dijkstra;
  bool found = dijkstra.paint_partial_min_paths_tree(g, n0, n3);

  ASSERT_TRUE(found);
}

// ========== TEST 6: Paint Partial - End Node Not Reachable ==========
TEST(DijkstraTest, PaintPartialEndNotReachable) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Isolated node

  g.insert_arc(n0, n1, 1);
  // n2 is not connected

  Dijkstra_Min_Paths<GT> dijkstra;
  bool found = dijkstra.paint_partial_min_paths_tree(g, n0, n2);

  ASSERT_FALSE(found);
}

// ========== TEST 7: Get Min Path After Painting ==========
TEST(DijkstraTest, GetMinPathAfterPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);

  Dijkstra_Min_Paths<GT> dijkstra;
  dijkstra.paint_min_paths_tree(g, n0);

  Path<GT> path(g);
  auto cost = dijkstra.get_min_path(n2, path);

  ASSERT_EQ(cost, 5);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 8: Single Node Graph ==========
TEST(DijkstraTest, SingleNodeGraph) {

  GT g;
  Node* n0 = g.insert_node(0);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  ASSERT_EQ(tree.get_num_nodes(), 1u);
  ASSERT_EQ(tree.get_num_arcs(), 0u);
}

// ========== TEST 9: Linear Graph ==========
TEST(DijkstraTest, LinearGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n3, path);

  ASSERT_EQ(cost, 3);
}

// ========== TEST 10: Complete Graph ==========
TEST(DijkstraTest, CompleteGraph) {

  GT g;
  const int num_nodes = 5;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(g.insert_node(i));
  }

  // Create complete graph with varying weights
  for (int i = 0; i < num_nodes; ++i) {
    for (int j = i + 1; j < num_nodes; ++j) {
      g.insert_arc(nodes[i], nodes[j], (i + 1) * (j + 1));
    }
  }

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, nodes[0], tree);

  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_nodes - 1));
}

// ========== TEST 11: Zero Weight Edges ==========
TEST(DijkstraTest, ZeroWeightEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 0);
  g.insert_arc(n1, n2, 0);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n2, path);

  ASSERT_EQ(cost, 0);
}

// ========== TEST 12: Large Graph ==========
TEST(DijkstraTest, LargeGraph) {

  GT g;
  const int num_nodes = 100;
  std::vector<Node*> nodes;

  for (int i = 0; i < num_nodes; ++i) {
    nodes.push_back(g.insert_node(i));
  }

  // Create a chain
  for (int i = 0; i < num_nodes - 1; ++i) {
    g.insert_arc(nodes[i], nodes[i+1], 1);
  }

  // Add some shortcuts
  for (int i = 0; i < num_nodes - 10; i += 10) {
    g.insert_arc(nodes[i], nodes[i+10], 5);
  }

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, nodes[0], tree);

  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_nodes));
}

// ========== TEST 13: Nullptr Validation - Start ==========
TEST(DijkstraTest, NullptrValidationStart) {

  GT g;
  g.insert_node(0);
  g.insert_node(1);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  EXPECT_THROW(dijkstra.compute_min_paths_tree(g, nullptr, tree), std::domain_error);
}

// ========== TEST 14: Nullptr Validation - End ==========
TEST(DijkstraTest, NullptrValidationEnd) {

  GT g;
  Node* n0 = g.insert_node(0);
  g.insert_node(1);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  EXPECT_THROW(dijkstra.compute_partial_min_paths_tree(g, n0, nullptr, tree),
               std::domain_error);
}

// ========== TEST 15: Copy Painted Tree Requires Painting ==========
TEST(DijkstraTest, CopyPaintedMinPathsTreeRequiresPainting) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  EXPECT_THROW(dijkstra.copy_painted_min_paths_tree(g, tree), std::domain_error);
}

// ========== TEST 16: Operator() for Tree Computation ==========
TEST(DijkstraTest, OperatorTreeComputation) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra(g, n0, tree);

  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);
}

// ========== TEST 17: Operator() for Path Finding ==========
TEST(DijkstraTest, OperatorPathFinding) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra(g, n0, n2, path);

  ASSERT_EQ(cost, 3);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 18: Multiple Paths Same Weight ==========
TEST(DijkstraTest, MultiplePathsSameWeight) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Two paths from n0 to n3 with same weight
  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n3, 2);
  g.insert_arc(n0, n2, 2);
  g.insert_arc(n2, n3, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n3, path);

  // Both paths have cost 4
  ASSERT_EQ(cost, 4);
  ASSERT_FALSE(path.is_empty());
}

// ========== TEST 19: Star Graph ==========
TEST(DijkstraTest, StarGraph) {

  GT g;
  Node* center = g.insert_node(0);
  const int num_leaves = 10;
  std::vector<Node*> leaves;

  for (int i = 1; i <= num_leaves; ++i) {
    auto leaf = g.insert_node(i);
    leaves.push_back(leaf);
    g.insert_arc(center, leaf, i);
  }

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, center, tree);

  ASSERT_EQ(tree.get_num_nodes(), static_cast<size_t>(num_leaves + 1));
  ASSERT_EQ(tree.get_num_arcs(), static_cast<size_t>(num_leaves));
}

// ========== TEST 20: Directed Graph Shortest Paths ==========
TEST(DijkstraTest, DirectedGraphShortestPaths) {

  DGT g;
  DNode* n0 = g.insert_node(0);
  DNode* n1 = g.insert_node(1);
  DNode* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n0, n2, 5); // Direct but longer

  Dijkstra_Min_Paths<DGT> dijkstra;
  Path<DGT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n2, path);

  // Shortest is n0 -> n1 -> n2 = 3
  ASSERT_EQ(cost, 3);
}

// ========== TEST 21: Get Min Path Without Painting First ==========
TEST(DijkstraTest, GetMinPathWithoutPaintingFirst) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  EXPECT_THROW(dijkstra.get_min_path(n1, path), std::domain_error);
}

// ========== TEST 22: Triangle Graph ==========
TEST(DijkstraTest, TriangleGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n0, n2, 3); // Longer direct path

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n2, path);

  // Shortest is n0 -> n1 -> n2 = 2
  ASSERT_EQ(cost, 2);
}

// ========== TEST 23: Self-Loop (should be ignored) ==========
TEST(DijkstraTest, SelfLoop) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n0, 1); // Self-loop
  g.insert_arc(n0, n1, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  ASSERT_EQ(tree.get_num_nodes(), 2u);
}

// ========== TEST 24: Path Cost Verification ==========
TEST(DijkstraTest, PathCostVerification) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  Node* n4 = g.insert_node(4);

  // Create a graph with known shortest paths
  g.insert_arc(n0, n1, 10);
  g.insert_arc(n0, n2, 3);
  g.insert_arc(n1, n3, 2);
  g.insert_arc(n2, n1, 1);
  g.insert_arc(n2, n3, 8);
  g.insert_arc(n2, n4, 2);
  g.insert_arc(n3, n4, 7);

  Dijkstra_Min_Paths<GT> dijkstra;

  // Test path from n0 to n4
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n4, path);

  // Shortest path n0 -> n2 -> n4 = 3 + 2 = 5
  ASSERT_EQ(cost, 5);
}

// ========== TEST 25: High Weight Variation ==========
TEST(DijkstraTest, HighWeightVariation) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1000000);
  g.insert_arc(n0, n2, 999999);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n2, path);

  // Direct path is shorter
  ASSERT_EQ(cost, 999999);
}

// ========== TEST 26: Disconnected Graph (partial tree via paint) ==========
TEST(DijkstraTest, DisconnectedGraph) {

  GT g;
  // Component 1
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  // Component 2 (disconnected)
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);
  g.insert_arc(n2, n3, 1);

  // Use paint_partial which handles disconnected graphs gracefully
  Dijkstra_Min_Paths<GT> dijkstra;
  bool found = dijkstra.paint_partial_min_paths_tree(g, n0, n2);

  // n2 is not reachable from n0
  ASSERT_FALSE(found);
}

// ========== TEST 27: Get Min Path From Tree ==========
TEST(DijkstraTest, GetMinPathFromTree) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);
  g.insert_arc(n2, n3, 3);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  // Verify tree was computed correctly
  ASSERT_EQ(tree.get_num_nodes(), 4u);
  ASSERT_EQ(tree.get_num_arcs(), 3u);

  Path<GT> path;
  const auto cost = dijkstra.get_min_path(tree, n3, path);
  ASSERT_EQ(cost, 6); // 1 + 2 + 3
  ASSERT_FALSE(path.is_empty());
  EXPECT_EQ(path_cost_by_info(path), cost);
  EXPECT_TRUE(path_is_consistent(path));
}

// ========== TEST 28: Node Mapping Verification ==========
TEST(DijkstraTest, NodeMappingVerification) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  // Verify tree structure
  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);

  // Verify nodes are properly mapped (tree nodes have correct info)
  bool found_0 = false, found_1 = false, found_2 = false;
  for (GT::Node_Iterator it(tree); it.has_curr(); it.next()) {
    int info = it.get_curr()->get_info();
    if (info == 0) found_0 = true;
    if (info == 1) found_1 = true;
    if (info == 2) found_2 = true;
  }
  ASSERT_TRUE(found_0);
  ASSERT_TRUE(found_1);
  ASSERT_TRUE(found_2);
}

// ========== TEST 29: Arc Mapping Verification ==========
TEST(DijkstraTest, ArcMappingVerification) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);

  g.insert_arc(n0, n1, 5);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);

  // Verify arc weight is preserved in tree
  ASSERT_EQ(tree.get_num_arcs(), 1u);

  GT::Arc_Iterator it(tree);
  ASSERT_TRUE(it.has_curr());
  ASSERT_EQ(it.get_curr()->get_info(), 5);
}

// ========== TEST 30: Partial Path Not Found Returns Max ==========
TEST(DijkstraTest, PartialPathNotFoundReturnsMax) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2); // Isolated

  g.insert_arc(n0, n1, 1);
  // n2 is not connected

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n2, path);

  // Should return max value when path not found
  ASSERT_EQ(cost, std::numeric_limits<int>::max());
  ASSERT_TRUE(path.is_empty());
}

// ========== TEST 31: Bi-directional Edges ==========
TEST(DijkstraTest, BidirectionalEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Undirected graph edges (both directions have same weight)
  g.insert_arc(n0, n1, 2);
  g.insert_arc(n1, n2, 3);

  Dijkstra_Min_Paths<GT> dijkstra;

  // Path from n0 to n2
  Path<GT> path1(g);
  auto cost1 = dijkstra.find_min_path(g, n0, n2, path1);
  ASSERT_EQ(cost1, 5);

  // Path from n2 to n0 (reverse)
  Path<GT> path2(g);
  auto cost2 = dijkstra.find_min_path(g, n2, n0, path2);
  ASSERT_EQ(cost2, 5);
}

// ========== TEST 32: Diamond Graph ==========
TEST(DijkstraTest, DiamondGraph) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);
  Node* n3 = g.insert_node(3);

  // Diamond graph:
  // n0--1--n1
  // n0--2--n2
  // n1--3--n3
  // n2--1--n3
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 2);
  g.insert_arc(n1, n3, 3);
  g.insert_arc(n2, n3, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path(g);
  auto cost = dijkstra.find_min_path(g, n0, n3, path);

  // Shortest: n0 -> n2 -> n3 = 2 + 1 = 3
  ASSERT_EQ(cost, 3);
}

// ========== Additional edge-case and regression tests ==========
TEST(DijkstraTest, ComputeMinPathsTreeDisconnectedGraphBuildsReachableSubtree) {

  GT g;
  // Reachable component
  Node* a0 = g.insert_node(0);
  Node* a1 = g.insert_node(1);
  Node* a2 = g.insert_node(2);
  g.insert_arc(a0, a1, 1);
  g.insert_arc(a1, a2, 1);

  // Unreachable component
  Node* b0 = g.insert_node(3);
  Node* b1 = g.insert_node(4);
  g.insert_arc(b0, b1, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  GT tree;
  dijkstra.compute_min_paths_tree(g, a0, tree);

  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);
  ASSERT_EQ(mapped_node<GT>(b0), nullptr);
  ASSERT_EQ(mapped_node<GT>(b1), nullptr);
}

TEST(DijkstraTest, PaintMinPathsTreeDisconnectedGraphPaintsOnlyReachableNodes) {

  GT g;
  // Reachable component
  Node* a0 = g.insert_node(0);
  Node* a1 = g.insert_node(1);
  Node* a2 = g.insert_node(2);
  g.insert_arc(a0, a1, 1);
  g.insert_arc(a1, a2, 2);

  // Unreachable component
  Node* b0 = g.insert_node(3);
  Node* b1 = g.insert_node(4);
  g.insert_arc(b0, b1, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  dijkstra.paint_min_paths_tree(g, a0);

  ASSERT_EQ(count_painted_nodes(g), 3u);
  ASSERT_EQ(count_painted_arcs(g), 2u);
  ASSERT_FALSE(IS_NODE_VISITED(b0, Aleph::Spanning_Tree));
  ASSERT_FALSE(IS_NODE_VISITED(b1, Aleph::Spanning_Tree));
}

TEST(DijkstraTest, PaintPartialStartEqualsEndReturnsTrueAndZeroCostPath) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 7);

  Dijkstra_Min_Paths<GT> dijkstra;
  ASSERT_TRUE(dijkstra.paint_partial_min_paths_tree(g, n0, n0));

  Path<GT> path;
  const auto cost = dijkstra.get_min_path(n0, path);
  ASSERT_EQ(cost, 0);
  ASSERT_FALSE(path.is_empty());
  ASSERT_EQ(path.get_first_node(), n0);
  ASSERT_EQ(path.get_last_node(), n0);
  ASSERT_EQ(path_cost_by_info(path), 0);
}

TEST(DijkstraTest, NegativeWeightThrowsDomainError) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  g.insert_arc(n0, n1, -1);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path;
  EXPECT_THROW(dijkstra.find_min_path(g, n0, n1, path), std::domain_error);
}

TEST(DijkstraTest, IntegerOverflowThrowsOverflowError) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  const int near_max = std::numeric_limits<int>::max() - 1;
  g.insert_arc(n0, n1, near_max);
  g.insert_arc(n1, n2, 10);

  Dijkstra_Min_Paths<GT> dijkstra;
  Path<GT> path;
  EXPECT_THROW(dijkstra.find_min_path(g, n0, n2, path), std::overflow_error);
}

TEST(DijkstraTest, StatefulDistanceIsUsedForPaintingAndTreePaths) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  using Dist = Scaled_Dist<GT>;
  Dijkstra_Min_Paths<GT, Dist> dijkstra(Dist(2));

  // Painting mode
  dijkstra.paint_min_paths_tree(g, n0);
  Path<GT> painted_path;
  ASSERT_EQ(dijkstra.get_min_path(n2, painted_path), 6);

  // Tree-building mode
  GT tree;
  dijkstra.compute_min_paths_tree(g, n0, tree);
  Path<GT> tree_path;
  ASSERT_EQ(dijkstra.get_min_path(tree, n2, tree_path), 6);
}

TEST(DijkstraTest, CopyPaintedTreeCopiesOnlyPaintedNodesAndKeepsCookies) {

  GT g;
  // Reachable component
  Node* a0 = g.insert_node(0);
  Node* a1 = g.insert_node(1);
  Node* a2 = g.insert_node(2);
  g.insert_arc(a0, a1, 1);
  g.insert_arc(a1, a2, 2);

  // Unreachable component
  Node* b0 = g.insert_node(3);
  Node* b1 = g.insert_node(4);
  g.insert_arc(b0, b1, 1);

  Dijkstra_Min_Paths<GT> dijkstra;
  dijkstra.paint_min_paths_tree(g, a0);

  GT tree;
  const auto total = dijkstra.copy_painted_min_paths_tree(g, tree);
  ASSERT_EQ(total, 3);
  ASSERT_EQ(tree.get_num_nodes(), 3u);
  ASSERT_EQ(tree.get_num_arcs(), 2u);

  // Copy must not clobber predecessor cookies.
  Path<GT> path;
  ASSERT_EQ(dijkstra.get_min_path(a2, path), 3);
}

TEST(DijkstraTest, GetMinPathAfterPaintingUsesPaintedArcAmongParallelEdges) {

  GT g;
  Node* n0 = g.insert_node(0);
  Node* n1 = g.insert_node(1);
  Node* n2 = g.insert_node(2);

  // Insert the heavier parallel edge first to stress reconstruction.
  g.insert_arc(n0, n1, 100);
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 2);

  Dijkstra_Min_Paths<GT> dijkstra;
  dijkstra.paint_min_paths_tree(g, n0);

  Path<GT> path;
  const auto cost = dijkstra.get_min_path(n2, path);
  ASSERT_EQ(cost, 3);
  ASSERT_EQ(path_cost_by_info(path), 3);
}

TEST(DijkstraTest, RandomGraphsMatchReferenceDijkstra) {

  std::mt19937 rng(123456);
  std::uniform_int_distribution<int> wdist(0, 20);

  for (int iter = 0; iter < 20; ++iter)
    {
      GT g;
      constexpr int N = 15;
      std::vector<Node*> nodes;
      nodes.reserve(N);
      for (int i = 0; i < N; ++i)
        nodes.push_back(g.insert_node(i));

      std::uniform_int_distribution<int> ndist(0, N - 1);
      constexpr int E = 40;
      for (int e = 0; e < E; ++e)
        {
          const int u = ndist(rng);
          const int v = ndist(rng);
          if (u == v)
            continue;
          g.insert_arc(nodes[u], nodes[v], wdist(rng));
        }

      Dijkstra_Min_Paths<GT> dijkstra;
      for (int q = 0; q < 10; ++q)
        {
          const int s = ndist(rng);
          const int t = ndist(rng);

          const auto ref = reference_dijkstra(g, nodes, nodes[s]);
          const long long rd = ref[t];

          Path<GT> path;
          const auto cost = dijkstra.find_min_path(g, nodes[s], nodes[t], path);

          const long long INF = std::numeric_limits<long long>::max() / 4;
          if (rd >= INF)
            {
              ASSERT_EQ(cost, std::numeric_limits<int>::max());
              ASSERT_TRUE(path.is_empty());
            }
          else
            {
              ASSERT_EQ(cost, static_cast<int>(rd));
              ASSERT_FALSE(path.is_empty());
              ASSERT_EQ(path_cost_by_info(path), cost);
              ASSERT_TRUE(path_is_consistent(path));
            }
        }
    }
}

// ========== GoogleTest main ==========
int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
