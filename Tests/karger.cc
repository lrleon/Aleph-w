#include <gtest/gtest.h>

#include <tpl_sgraph.H>
#include <Karger.H>

using namespace Aleph;
using namespace std;

// Graph types for testing
using Grafo = List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>;
using Node = Grafo::Node;
using Arc = Grafo::Arc;

namespace {

// Create a simple triangle graph (3 nodes, 3 edges)
// Minimum cut is 2 (remove any node leaves 2 edges)
Grafo create_triangle()
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n0, n2, 1);

  return g;
}

// Create a square graph (4 nodes, 4 edges)
// Minimum cut is 2
Grafo create_square()
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);
  g.insert_arc(n3, n0, 1);

  return g;
}

// Create a graph with an obvious minimum cut of 1
// Two clusters connected by a single edge
Grafo create_barbell()
{
  Grafo g;
  // Left cluster
  auto a0 = g.insert_node(0);
  auto a1 = g.insert_node(1);
  auto a2 = g.insert_node(2);

  g.insert_arc(a0, a1, 1);
  g.insert_arc(a1, a2, 1);
  g.insert_arc(a0, a2, 1);

  // Right cluster
  auto b0 = g.insert_node(10);
  auto b1 = g.insert_node(11);
  auto b2 = g.insert_node(12);

  g.insert_arc(b0, b1, 1);
  g.insert_arc(b1, b2, 1);
  g.insert_arc(b0, b2, 1);

  // Bridge (single edge connecting clusters)
  g.insert_arc(a0, b0, 1);

  return g;
}

// Create a complete graph K_n
Grafo create_complete_graph(int n)
{
  Grafo g;
  vector<Node*> nodes;

  for (int i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
      g.insert_arc(nodes[i], nodes[j], 1);

  return g;
}

// Create a path graph (n nodes connected in a line)
// Minimum cut is 1
Grafo create_path(int n)
{
  Grafo g;
  vector<Node*> nodes;

  for (int i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < n - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  return g;
}

// Create a cycle graph (n nodes in a ring)
// Minimum cut is 2
Grafo create_cycle(int n)
{
  Grafo g;
  vector<Node*> nodes;

  for (int i = 0; i < n; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < n; ++i)
    g.insert_arc(nodes[i], nodes[(i + 1) % n], 1);

  return g;
}

} // namespace

// Test basic construction
TEST(KargerConstruction, constructs_with_default_seed)
{
  EXPECT_NO_THROW(Karger_Min_Cut<Grafo> karger);
}

TEST(KargerConstruction, constructs_with_explicit_seed)
{
  EXPECT_NO_THROW(Karger_Min_Cut<Grafo> karger(12345));
}

// Test error handling
TEST(KargerErrors, throws_on_empty_graph)
{
  Grafo g;
  g.insert_node(0);
  g.insert_node(1);
  // No arcs

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  EXPECT_THROW(karger(g, vs, vt, cut), std::domain_error);
}

TEST(KargerErrors, throws_on_single_node_graph)
{
  Grafo g;
  g.insert_node(0);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  EXPECT_THROW(karger(g, vs, vt, cut), std::domain_error);
}

TEST(KargerErrors, throws_on_single_node_with_self_loop)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  g.insert_arc(n0, n0, 1); // Self-loop

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  // Should throw because graph has only 1 node
  EXPECT_THROW(karger(g, vs, vt, cut), std::domain_error);
}

// Test basic minimum cut on simple graphs
TEST(KargerMinCut, finds_cut_on_triangle)
{
  auto g = create_triangle();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Minimum cut of triangle is 2
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(cut.size(), 2u);

  // Partitions should cover all nodes
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());

  // Each partition should be non-empty
  EXPECT_GT(vs.size(), 0u);
  EXPECT_GT(vt.size(), 0u);
}

TEST(KargerMinCut, finds_cut_on_square)
{
  auto g = create_square();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Minimum cut of square is 2
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(cut.size(), 2u);

  // Partitions should be balanced (2 nodes each for min cut)
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());
}

TEST(KargerMinCut, finds_obvious_cut_on_barbell)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 200);

  // The barbell has a single bridge, so min cut is 1
  EXPECT_EQ(min_cut, 1);
  EXPECT_EQ(cut.size(), 1u);

  // Each partition should have 3 nodes (one cluster each)
  EXPECT_EQ(vs.size(), 3u);
  EXPECT_EQ(vt.size(), 3u);
}

TEST(KargerMinCut, finds_cut_on_path)
{
  auto g = create_path(5);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Path has minimum cut of 1 (any edge)
  EXPECT_EQ(min_cut, 1);
  EXPECT_EQ(cut.size(), 1u);
}

TEST(KargerMinCut, finds_cut_on_cycle)
{
  auto g = create_cycle(6);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Cycle has minimum cut of 2
  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(cut.size(), 2u);
}

// Test complete graphs
TEST(KargerMinCut, finds_cut_on_k4)
{
  auto g = create_complete_graph(4);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // K4 minimum cut: removing 1 node requires cutting 3 edges
  // Optimal is splitting 2-2: needs 4 edges (each pair connects to 2 in other)
  // Actually min cut of K4 = 3 (isolate one vertex)
  EXPECT_EQ(min_cut, 3);
}

TEST(KargerMinCut, finds_cut_on_k5)
{
  auto g = create_complete_graph(5);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 200);

  // K5 minimum cut = 4 (isolate one vertex)
  EXPECT_EQ(min_cut, 4);
}

// Test reproducibility with same seed
TEST(KargerReproducibility, same_seed_same_result)
{
  auto g = create_barbell();

  Karger_Min_Cut<Grafo> karger1(12345);
  DynList<Node*> vs1, vt1;
  DynList<Arc*> cut1;
  size_t result1 = karger1(g, vs1, vt1, cut1, 50);

  g = create_barbell();  // recreate graph
  Karger_Min_Cut<Grafo> karger2(12345);
  DynList<Node*> vs2, vt2;
  DynList<Arc*> cut2;
  size_t result2 = karger2(g, vs2, vt2, cut2, 50);

  EXPECT_EQ(result1, result2);
  EXPECT_EQ(cut1.size(), cut2.size());
}

// Test that more iterations don't make result worse
TEST(KargerIterations, more_iterations_not_worse)
{
  auto g = create_complete_graph(6);

  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs1, vt1;
  DynList<Arc*> cut1;
  size_t result_few = karger(g, vs1, vt1, cut1, 10);

  g = create_complete_graph(6);  // recreate
  DynList<Node*> vs2, vt2;
  DynList<Arc*> cut2;
  size_t result_many = karger(g, vs2, vt2, cut2, 100);

  // More iterations should find at least as good a cut
  EXPECT_LE(result_many, result_few);
}

// Test default iteration count
TEST(KargerDefaultIterations, works_with_default_iterations)
{
  auto g = create_triangle();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  // Use default iteration count
  size_t min_cut = karger(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 2);
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());
}

// Test partition validity
TEST(KargerPartitions, partitions_are_valid)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  karger(g, vs, vt, cut, 100);

  // Check that all nodes in partitions are from the original graph
  DynSetTree<Node*> all_nodes;
  for (Grafo::Node_Iterator it(g); it.has_curr(); it.next_ne())
    all_nodes.insert(it.get_curr());

  for (auto it = vs.get_it(); it.has_curr(); it.next())
    EXPECT_TRUE(all_nodes.has(it.get_curr()));

  for (auto it = vt.get_it(); it.has_curr(); it.next())
    EXPECT_TRUE(all_nodes.has(it.get_curr()));

  // Check no overlap between partitions
  DynSetTree<Node*> vs_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());

  for (auto it = vt.get_it(); it.has_curr(); it.next())
    EXPECT_FALSE(vs_set.has(it.get_curr()));
}

// Test cut edges validity
TEST(KargerCutEdges, cut_edges_are_valid)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  karger(g, vs, vt, cut, 100);

  // Check that all cut edges are from the original graph
  DynSetTree<Arc*> all_arcs;
  for (Grafo::Arc_Iterator it(g); it.has_curr(); it.next_ne())
    all_arcs.insert(it.get_curr());

  for (auto it = cut.get_it(); it.has_curr(); it.next())
    EXPECT_TRUE(all_arcs.has(it.get_curr()));
}

// Test two-node graph (smallest possible)
TEST(KargerEdgeCases, handles_two_node_graph)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 10);

  EXPECT_EQ(min_cut, 1);
  EXPECT_EQ(vs.size(), 1u);
  EXPECT_EQ(vt.size(), 1u);
}

// Test graph with multiple parallel edges (multigraph behavior)
TEST(KargerMultiEdge, handles_multiple_edges_between_nodes)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  // Multiple edges between n0 and n1
  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n1, 2);
  g.insert_arc(n0, n1, 3);

  // Single edge to n2
  g.insert_arc(n1, n2, 4);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Min cut should be 1 (separate n2 from the rest)
  EXPECT_EQ(min_cut, 1u);
}

// Stress test with larger graph
TEST(KargerStress, handles_larger_graph)
{
  auto g = create_complete_graph(10);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 50);

  // K10 minimum cut = 9 (isolate one vertex)
  EXPECT_EQ(min_cut, 9);
  EXPECT_EQ(vs.size() + vt.size(), 10u);
}

// Custom arc filter that excludes arcs with weight > threshold
template <class GT>
struct Weight_Filter
{
  int threshold;

  Weight_Filter(int t = 5) : threshold(t) {}

  bool operator()(typename GT::Arc * a) const
  {
    return a->get_info() <= threshold;
  }
};

// Test arc filter functionality
TEST(KargerArcFilter, filters_arcs_by_weight)
{
  // Create two clusters connected by both low and high weight edges
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Cluster 1: n0-n1 (low weight)
  g.insert_arc(n0, n1, 1);

  // Cluster 2: n2-n3 (low weight)
  g.insert_arc(n2, n3, 2);

  // Cross-cluster connections:
  // - Low weight bridge (will be included): n1-n2
  g.insert_arc(n1, n2, 3);
  // - High weight bridge (will be filtered): n0-n3
  g.insert_arc(n0, n3, 10);

  // Without filter: min cut could use either bridge
  // With filter (threshold=5): only see low weight edges
  // Graph becomes: n0-n1-n2-n3 path, min cut = 1

  Weight_Filter<Grafo> filter(5);
  Karger_Min_Cut<Grafo, Weight_Filter<Grafo>> karger_filtered(42, filter);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger_filtered(g, vs, vt, cut, 100);

  // Path graph has min cut = 1
  EXPECT_EQ(min_cut, 1u);

  // All cut edges should have weight <= threshold
  for (auto it = cut.get_it(); it.has_curr(); it.next())
    EXPECT_LE(it.get_curr()->get_info(), 5);
}

// Test that default filter includes all arcs
TEST(KargerArcFilter, default_filter_includes_all)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 100);
  g.insert_arc(n1, n2, 100);
  g.insert_arc(n0, n2, 100);

  // Default filter should include all arcs regardless of weight
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Triangle min cut = 2
  EXPECT_EQ(min_cut, 2u);
}

// Test arc filter with explicit SA template parameter
TEST(KargerArcFilter, works_with_explicit_template)
{
  Grafo g;
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  // Explicit default filter
  Dft_Show_Arc<Grafo> default_filter;
  Karger_Min_Cut<Grafo, Dft_Show_Arc<Grafo>> karger(42, default_filter);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 10);

  EXPECT_EQ(min_cut, 1u);
}

// Test that cut edges actually cross the partition
TEST(KargerCutValidity, cut_edges_cross_partition)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  karger(g, vs, vt, cut, 100);

  // Build sets for fast lookup
  DynSetTree<Node*> vs_set, vt_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());
  for (auto it = vt.get_it(); it.has_curr(); it.next())
    vt_set.insert(it.get_curr());

  // Every cut edge should have one endpoint in vs and one in vt
  for (auto it = cut.get_it(); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);

      bool src_in_vs = vs_set.has(src);
      bool tgt_in_vs = vs_set.has(tgt);
      bool src_in_vt = vt_set.has(src);
      bool tgt_in_vt = vt_set.has(tgt);

      // Edge crosses partition: one end in vs, other in vt
      EXPECT_TRUE((src_in_vs && tgt_in_vt) || (src_in_vt && tgt_in_vs));
    }
}

// Test graph with disconnected component would still work if filter excludes it
TEST(KargerDisconnected, handles_star_graph)
{
  // Star graph: center connected to all others
  Grafo g;
  auto center = g.insert_node(0);
  vector<Node*> leaves;

  for (int i = 1; i <= 5; ++i)
    {
      auto leaf = g.insert_node(i);
      leaves.push_back(leaf);
      g.insert_arc(center, leaf, 1);
    }

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger(g, vs, vt, cut, 100);

  // Star graph min cut is 1 (separate any leaf)
  EXPECT_EQ(min_cut, 1u);
}

// Test with zero iterations returns max value (no cut found)
TEST(KargerIterations, zero_iterations_behavior)
{
  auto g = create_triangle();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  // With 0 iterations, should return max size_t (no cut found)
  size_t result = karger(g, vs, vt, cut, 0);

  // Result should be very large (max size_t)
  EXPECT_GT(result, 1000000u);

  // Partitions and cut should be empty (no iteration ran)
  EXPECT_TRUE(vs.is_empty());
  EXPECT_TRUE(vt.is_empty());
  EXPECT_TRUE(cut.is_empty());
}

// Test that class is not copyable (compile-time check would fail if tried)
// This is a documentation test - actual copy would fail to compile
TEST(KargerNonCopyable, class_is_moveable)
{
  // Move construction should work
  Karger_Min_Cut<Grafo> karger1(42);
  // Cannot test move directly as it's implicitly deleted when copy is deleted
  // This test just verifies the basic construction works
  EXPECT_NO_THROW(Karger_Min_Cut<Grafo>(42));
}

// ============= Tests for fast() (Karger-Stein algorithm) =============
// Note: Karger-Stein is probabilistic and may not find optimal in one run

TEST(KargerFast, finds_valid_cut_on_barbell)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger.fast(g, vs, vt, cut);

  // Fast version may not find optimal, but should find a valid cut
  // Barbell has min cut = 1, but algorithm may return more
  EXPECT_GE(min_cut, 1u);  // At least 1
  EXPECT_LE(min_cut, 7u);  // At most all edges except internal cluster edges
  EXPECT_EQ(cut.size(), min_cut);
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());
}

TEST(KargerFast, finds_cut_on_path)
{
  auto g = create_path(8);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger.fast(g, vs, vt, cut);

  // Path has min cut = 1
  EXPECT_EQ(min_cut, 1u);
}

TEST(KargerFast, finds_cut_on_cycle)
{
  auto g = create_cycle(8);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger.fast(g, vs, vt, cut);

  // Cycle has min cut = 2
  EXPECT_EQ(min_cut, 2u);
}

TEST(KargerFast, finds_cut_on_complete_graph)
{
  auto g = create_complete_graph(8);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger.fast(g, vs, vt, cut);

  // K8 has min cut = 7 (isolate one vertex)
  EXPECT_EQ(min_cut, 7u);
}

TEST(KargerFast, partitions_are_valid)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  karger.fast(g, vs, vt, cut);

  // Check partitions cover all nodes
  EXPECT_EQ(vs.size() + vt.size(), g.get_num_nodes());

  // Check no overlap
  DynSetTree<Node*> vs_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());

  for (auto it = vt.get_it(); it.has_curr(); it.next())
    EXPECT_FALSE(vs_set.has(it.get_curr()));
}

TEST(KargerFast, cut_edges_are_valid)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  karger.fast(g, vs, vt, cut);

  // Build sets for validation
  DynSetTree<Node*> vs_set, vt_set;
  for (auto it = vs.get_it(); it.has_curr(); it.next())
    vs_set.insert(it.get_curr());
  for (auto it = vt.get_it(); it.has_curr(); it.next())
    vt_set.insert(it.get_curr());

  // Every cut edge should cross the partition
  for (auto it = cut.get_it(); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = g.get_src_node(arc);
      auto tgt = g.get_tgt_node(arc);

      bool crosses = (vs_set.has(src) && vt_set.has(tgt)) ||
                     (vt_set.has(src) && vs_set.has(tgt));
      EXPECT_TRUE(crosses);
    }
}

TEST(KargerFast, respects_num_iter_parameter)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  // Test with explicit num_iter
  size_t min_cut = karger.fast(g, vs, vt, cut, 10);

  EXPECT_GE(min_cut, 1u);
  EXPECT_FALSE(vs.is_empty());
  EXPECT_FALSE(vt.is_empty());
}

// ================================================================
// Tests for new features: get_seed(), move semantics, early termination
// ================================================================

TEST(KargerSeed, get_seed_returns_construction_seed)
{
  Karger_Min_Cut<Grafo> karger1(12345);
  EXPECT_EQ(karger1.get_seed(), 12345ul);

  Karger_Min_Cut<Grafo> karger2(99999);
  EXPECT_EQ(karger2.get_seed(), 99999ul);
}

TEST(KargerMoveSemantics, move_constructor_works)
{
  Karger_Min_Cut<Grafo> original(42);
  unsigned long seed = original.get_seed();

  Karger_Min_Cut<Grafo> moved(std::move(original));

  // Moved object has the seed
  EXPECT_EQ(moved.get_seed(), seed);

  // Can use moved object
  auto g = create_triangle();
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;
  EXPECT_NO_THROW(moved(g, vs, vt, cut, 1));
}

TEST(KargerMoveSemantics, move_assignment_works)
{
  Karger_Min_Cut<Grafo> karger1(111);
  Karger_Min_Cut<Grafo> karger2(222);

  karger1 = std::move(karger2);

  EXPECT_EQ(karger1.get_seed(), 222ul);

  // Can use karger1 after move assignment
  auto g = create_triangle();
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;
  EXPECT_NO_THROW(karger1(g, vs, vt, cut, 1));
}

TEST(KargerEarlyTermination, terminates_early_on_cut_of_one)
{
  // Create a path graph (line): A - B - C - D
  // The minimum cut is 1 (any edge)
  Grafo g;
  auto a = g.insert_node(1);
  auto b = g.insert_node(2);
  auto c = g.insert_node(3);
  auto d = g.insert_node(4);

  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, d);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  // Should find min cut of 1 and terminate early
  size_t min_cut = karger(g, vs, vt, cut, 100);

  EXPECT_EQ(min_cut, 1u);
  EXPECT_EQ(cut.size(), 1u);
  EXPECT_EQ(vs.size() + vt.size(), 4u);
}

TEST(KargerEarlyTermination, fast_terminates_early_on_cut_of_one)
{
  // Path graph
  Grafo g;
  auto a = g.insert_node(1);
  auto b = g.insert_node(2);
  auto c = g.insert_node(3);
  auto d = g.insert_node(4);

  g.insert_arc(a, b);
  g.insert_arc(b, c);
  g.insert_arc(c, d);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t min_cut = karger.fast(g, vs, vt, cut);

  EXPECT_EQ(min_cut, 1u);
  EXPECT_EQ(cut.size(), 1u);
}

// ================================================================
// Tests for reseed() and compute_min_cut_size()
// ================================================================

TEST(KargerReseed, reseed_changes_behavior)
{
  auto g = create_complete_graph(6);

  Karger_Min_Cut<Grafo> karger(42);
  DynList<Node*> vs1, vt1;
  DynList<Arc*> cut1;
  karger(g, vs1, vt1, cut1, 5);

  // Reseed and run again
  karger.reseed(12345);
  EXPECT_EQ(karger.get_seed(), 12345ul);

  DynList<Node*> vs2, vt2;
  DynList<Arc*> cut2;
  karger(g, vs2, vt2, cut2, 5);

  // Both should find valid cuts (we can't guarantee different results
  // due to randomness, but both should work)
  EXPECT_FALSE(vs1.is_empty());
  EXPECT_FALSE(vs2.is_empty());
}

TEST(KargerReseed, reseed_allows_reproducibility)
{
  auto g = create_barbell();

  Karger_Min_Cut<Grafo> karger1(999);
  Karger_Min_Cut<Grafo> karger2(0);

  // Reseed karger2 to same seed as karger1
  karger2.reseed(999);

  DynList<Node*> vs1, vt1, vs2, vt2;
  DynList<Arc*> cut1, cut2;

  size_t result1 = karger1(g, vs1, vt1, cut1, 10);
  size_t result2 = karger2(g, vs2, vt2, cut2, 10);

  EXPECT_EQ(result1, result2);
}

TEST(KargerSizeOnly, compute_min_cut_size_works)
{
  auto g = create_barbell();
  Karger_Min_Cut<Grafo> karger(42);

  size_t min_cut = karger.compute_min_cut_size(g, 50);

  // Barbell has min cut of 1
  EXPECT_EQ(min_cut, 1u);
}

TEST(KargerSizeOnly, compute_min_cut_size_with_default_iterations)
{
  auto g = create_triangle();
  Karger_Min_Cut<Grafo> karger(42);

  // Call with default iterations (0 means use n^2)
  size_t min_cut = karger.compute_min_cut_size(g);

  // Triangle has min cut of 2
  EXPECT_EQ(min_cut, 2u);
}

TEST(KargerSizeOnly, compute_min_cut_size_throws_on_invalid_graph)
{
  Grafo empty_graph;
  Karger_Min_Cut<Grafo> karger(42);

  EXPECT_THROW(karger.compute_min_cut_size(empty_graph), std::domain_error);
}

TEST(KargerSizeOnly, size_only_matches_full_computation)
{
  auto g = create_complete_graph(5);
  Karger_Min_Cut<Grafo> karger1(42);
  Karger_Min_Cut<Grafo> karger2(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  size_t full_result = karger1(g, vs, vt, cut, 50);
  size_t size_only = karger2.compute_min_cut_size(g, 50);

  // With same seed and iterations, should get same result
  EXPECT_EQ(full_result, size_only);
}
