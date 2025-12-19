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

// Test basic minimum cut on simple graphs
TEST(KargerMinCut, finds_cut_on_triangle)
{
  auto g = create_triangle();
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  int min_cut = karger(g, vs, vt, cut, 100);

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

  int min_cut = karger(g, vs, vt, cut, 100);

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

  int min_cut = karger(g, vs, vt, cut, 200);

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

  int min_cut = karger(g, vs, vt, cut, 100);

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

  int min_cut = karger(g, vs, vt, cut, 100);

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

  int min_cut = karger(g, vs, vt, cut, 100);

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

  int min_cut = karger(g, vs, vt, cut, 200);

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
  int result1 = karger1(g, vs1, vt1, cut1, 50);

  g = create_barbell();  // recreate graph
  Karger_Min_Cut<Grafo> karger2(12345);
  DynList<Node*> vs2, vt2;
  DynList<Arc*> cut2;
  int result2 = karger2(g, vs2, vt2, cut2, 50);

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
  int result_few = karger(g, vs1, vt1, cut1, 10);

  g = create_complete_graph(6);  // recreate
  DynList<Node*> vs2, vt2;
  DynList<Arc*> cut2;
  int result_many = karger(g, vs2, vt2, cut2, 100);

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
  int min_cut = karger(g, vs, vt, cut);

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

  int min_cut = karger(g, vs, vt, cut, 10);

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

  int min_cut = karger(g, vs, vt, cut, 100);

  // Min cut should be 1 (separate n2 from the rest)
  EXPECT_EQ(min_cut, 1);
}

// Stress test with larger graph
TEST(KargerStress, handles_larger_graph)
{
  auto g = create_complete_graph(10);
  Karger_Min_Cut<Grafo> karger(42);

  DynList<Node*> vs, vt;
  DynList<Arc*> cut;

  int min_cut = karger(g, vs, vt, cut, 50);

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

  int min_cut = karger_filtered(g, vs, vt, cut, 100);

  // Path graph has min cut = 1
  EXPECT_EQ(min_cut, 1);

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

  int min_cut = karger(g, vs, vt, cut, 100);

  // Triangle min cut = 2
  EXPECT_EQ(min_cut, 2);
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

  int min_cut = karger(g, vs, vt, cut, 10);

  EXPECT_EQ(min_cut, 1);
}
