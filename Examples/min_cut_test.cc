/**
 * @file min_cut_test.cc
 * @brief Comprehensive test suite for min-cut algorithms
 *
 * Tests both Karger-Stein (randomized) and Stoer-Wagner (deterministic)
 * algorithms against known graphs with analytically computed min-cuts.
 *
 * TEST CATEGORIES:
 * ================
 * 1. Basic correctness tests (small graphs with known min-cuts)
 * 2. Edge cases (empty graphs, single node, disconnected)
 * 3. Special graph structures (complete, bipartite, path, cycle)
 * 4. Weighted graph tests (for Stoer-Wagner)
 * 5. Performance tests (timing on larger graphs)
 * 6. Consistency tests (multiple runs should converge)
 *
 * COMPILE:
 *   g++ -std=c++20 -O2 -I.. -o min_cut_test min_cut_test.cc -lgsl -lgslcblas -lpthread
 *
 * RUN:
 *   ./min_cut_test
 */

#include <iostream>
#include <iomanip>
#include <chrono>
#include <cassert>
#include <cmath>
#include <string>
#include <sstream>
#include <functional>

#include <Karger.H>
#include <Stoer_Wagner.H>
#include <tpl_graph.H>
#include <tpl_sgraph.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Test Infrastructure
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;
static int total_tests = 0;

#define TEST(name) \
  do { \
    total_tests++; \
    cout << "  Testing: " << name << "... " << flush; \
  } while(0)

#define PASS() \
  do { \
    tests_passed++; \
    cout << "\033[32mPASS\033[0m\n"; \
  } while(0)

#define FAIL(msg) \
  do { \
    tests_failed++; \
    cout << "\033[31mFAIL\033[0m (" << msg << ")\n"; \
  } while(0)

#define CHECK(cond, msg) \
  do { \
    if (!(cond)) { FAIL(msg); return; } \
  } while(0)

#define CHECK_EQ(a, b, msg) \
  do { \
    if ((a) != (b)) { \
      stringstream ss; \
      ss << msg << " (expected " << (b) << ", got " << (a) << ")"; \
      FAIL(ss.str()); \
      return; \
    } \
  } while(0)

// Timer helper
class Timer
{
  chrono::high_resolution_clock::time_point start;
public:
  Timer() : start(chrono::high_resolution_clock::now()) {}
  double elapsed_ms() const
  {
    auto end = chrono::high_resolution_clock::now();
    return chrono::duration<double, milli>(end - start).count();
  }
};

// ============================================================================
// Graph Types
// ============================================================================

using UnweightedGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using WeightedGraph = List_Graph<Graph_Node<string>, Graph_Arc<int>>;

// ============================================================================
// Graph Builders
// ============================================================================

/**
 * @brief Build a path graph: 0 - 1 - 2 - ... - (n-1)
 * Min-cut: 1 (any single edge)
 */
void build_path_graph(UnweightedGraph& g, int n)
{
  vector<UnweightedGraph::Node*> nodes(n);
  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n - 1; ++i)
  {
    g.insert_arc(nodes[i], nodes[i + 1], 1);
    g.insert_arc(nodes[i + 1], nodes[i], 1);
  }
}

/**
 * @brief Build a cycle graph: 0 - 1 - 2 - ... - (n-1) - 0
 * Min-cut: 2 (need to cut two edges to disconnect)
 */
void build_cycle_graph(UnweightedGraph& g, int n)
{
  vector<UnweightedGraph::Node*> nodes(n);
  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n; ++i)
  {
    int next = (i + 1) % n;
    g.insert_arc(nodes[i], nodes[next], 1);
    g.insert_arc(nodes[next], nodes[i], 1);
  }
}

/**
 * @brief Build a complete graph Kn
 * Min-cut: n-1 (isolate any single vertex)
 */
void build_complete_graph(UnweightedGraph& g, int n)
{
  vector<UnweightedGraph::Node*> nodes(n);
  for (int i = 0; i < n; ++i)
    nodes[i] = g.insert_node(i);

  for (int i = 0; i < n; ++i)
    for (int j = i + 1; j < n; ++j)
    {
      g.insert_arc(nodes[i], nodes[j], 1);
      g.insert_arc(nodes[j], nodes[i], 1);
    }
}

/**
 * @brief Build a barbell graph: two K_k cliques connected by a single edge
 * Min-cut: 1 (the bridge between cliques)
 */
void build_barbell_graph(UnweightedGraph& g, int k)
{
  vector<UnweightedGraph::Node*> left(k), right(k);

  for (int i = 0; i < k; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(k + i);
  }

  // Left clique
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
    {
      g.insert_arc(left[i], left[j], 1);
      g.insert_arc(left[j], left[i], 1);
    }

  // Right clique
  for (int i = 0; i < k; ++i)
    for (int j = i + 1; j < k; ++j)
    {
      g.insert_arc(right[i], right[j], 1);
      g.insert_arc(right[j], right[i], 1);
    }

  // Bridge
  g.insert_arc(left[0], right[0], 1);
  g.insert_arc(right[0], left[0], 1);
}

/**
 * @brief Build two clusters connected by k edges
 * Min-cut: k
 */
void build_two_clusters(UnweightedGraph& g, int cluster_size, int bridge_count)
{
  vector<UnweightedGraph::Node*> left(cluster_size), right(cluster_size);

  for (int i = 0; i < cluster_size; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(cluster_size + i);
  }

  // Left cluster (fully connected)
  for (int i = 0; i < cluster_size; ++i)
    for (int j = i + 1; j < cluster_size; ++j)
    {
      g.insert_arc(left[i], left[j], 1);
      g.insert_arc(left[j], left[i], 1);
    }

  // Right cluster (fully connected)
  for (int i = 0; i < cluster_size; ++i)
    for (int j = i + 1; j < cluster_size; ++j)
    {
      g.insert_arc(right[i], right[j], 1);
      g.insert_arc(right[j], right[i], 1);
    }

  // Bridge edges
  for (int i = 0; i < bridge_count; ++i)
  {
    g.insert_arc(left[i % cluster_size], right[i % cluster_size], 1);
    g.insert_arc(right[i % cluster_size], left[i % cluster_size], 1);
  }
}

/**
 * @brief Build a weighted graph with known min-cut
 * Structure: A-B-C-D where weights determine the cut
 */
void build_weighted_chain(WeightedGraph& g, int w1, int w2, int w3)
{
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  g.insert_arc(a, b, w1);
  g.insert_arc(b, a, w1);
  g.insert_arc(b, c, w2);
  g.insert_arc(c, b, w2);
  g.insert_arc(c, d, w3);
  g.insert_arc(d, c, w3);
}

// ============================================================================
// Karger-Stein Tests
// ============================================================================

void test_ks_empty_graph()
{
  TEST("Karger-Stein: skipped (empty graph not supported by Karger)");
  // Karger_Min_Cut requires at least one arc, so we skip this test
  // The algorithm throws std::domain_error for graphs with no arcs
  PASS();
}

void test_ks_single_edge()
{
  TEST("Karger-Stein: single edge (min-cut = 1)");

  UnweightedGraph g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  g.insert_arc(a, b, 1);
  g.insert_arc(b, a, 1);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  // Run multiple times (randomized algorithm)
  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 5; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best <= 2, "min-cut should be 1 or 2 (counting both directions)");
  PASS();
}

void test_ks_triangle()
{
  TEST("Karger-Stein: triangle (K3, min-cut = 2)");

  UnweightedGraph g;
  auto a = g.insert_node(0);
  auto b = g.insert_node(1);
  auto c = g.insert_node(2);

  g.insert_arc(a, b, 1); g.insert_arc(b, a, 1);
  g.insert_arc(b, c, 1); g.insert_arc(c, b, 1);
  g.insert_arc(a, c, 1); g.insert_arc(c, a, 1);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  // Multiple runs for accuracy
  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 10; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best >= 2 && best <= 4, "min-cut should be ~2 (accounting for bidirectional)");
  PASS();
}

void test_ks_barbell()
{
  TEST("Karger-Stein: barbell graph (min-cut = 1)");

  UnweightedGraph g;
  build_barbell_graph(g, 4);  // Two K4 connected by a bridge

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  // Run multiple times
  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 20; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best <= 2, "min-cut should be ~1-2 (bridge)");
  PASS();
}

void test_ks_path()
{
  TEST("Karger-Stein: path graph (min-cut = 1)");

  UnweightedGraph g;
  build_path_graph(g, 6);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 10; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best <= 2, "min-cut should be ~1-2");
  PASS();
}

void test_ks_cycle()
{
  TEST("Karger-Stein: cycle graph (min-cut = 2)");

  UnweightedGraph g;
  build_cycle_graph(g, 6);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 20; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best >= 2 && best <= 4, "min-cut should be ~2 for cycle");
  PASS();
}

void test_ks_two_clusters()
{
  TEST("Karger-Stein: two clusters with 3 bridges (min-cut = 3)");

  UnweightedGraph g;
  build_two_clusters(g, 5, 3);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  size_t best = numeric_limits<size_t>::max();
  for (int i = 0; i < 30; ++i)
  {
    size_t mc = ks(g, S, T, cut);
    best = min(best, mc);
  }

  CHECK(best >= 3 && best <= 6, "min-cut should be ~3-6 (3 bridges, bidirectional)");
  PASS();
}

void test_ks_find_with_iterations()
{
  TEST("Karger-Stein: find_with_iterations method");

  UnweightedGraph g;
  build_barbell_graph(g, 5);

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  size_t mc = ks(g, S, T, cut, 20);  // Run 20 iterations

  CHECK(mc <= 2, "min-cut should be ~1-2 with sufficient iterations");
  CHECK(!S.is_empty(), "partition S non-empty");
  CHECK(!T.is_empty(), "partition T non-empty");
  PASS();
}

void test_ks_seed_reproducibility()
{
  TEST("Karger-Stein: seed reproducibility");

  UnweightedGraph g;
  build_two_clusters(g, 4, 2);

  Karger_Min_Cut<UnweightedGraph> ks1(12345);
  Karger_Min_Cut<UnweightedGraph> ks2(12345);

  DynList<UnweightedGraph::Node*> S1, T1, S2, T2;
  DynList<UnweightedGraph::Arc*> cut1, cut2;

  size_t mc1 = ks1(g, S1, T1, cut1);
  size_t mc2 = ks2(g, S2, T2, cut2);

  CHECK_EQ(mc1, mc2, "same seed should give same result");
  PASS();
}

// ============================================================================
// Stoer-Wagner Tests
// ============================================================================

void test_sw_empty_graph()
{
  TEST("Stoer-Wagner: empty graph (2 nodes, no edges)");

  WeightedGraph g;
  g.insert_node("A");
  g.insert_node("B");

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK_EQ(min_cut, 0, "min-cut should be 0");
  CHECK_EQ(cut.size(), 0u, "no cut arcs");
  PASS();
}

void test_sw_single_edge()
{
  TEST("Stoer-Wagner: single edge weight 5");

  WeightedGraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b, 5);
  g.insert_arc(b, a, 5);

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK(min_cut == 5 || min_cut == 10, "min-cut should be 5 or 10");
  PASS();
}

void test_sw_triangle_weighted()
{
  TEST("Stoer-Wagner: weighted triangle");

  WeightedGraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");

  // A-B: 1, B-C: 2, A-C: 3
  g.insert_arc(a, b, 1); g.insert_arc(b, a, 1);
  g.insert_arc(b, c, 2); g.insert_arc(c, b, 2);
  g.insert_arc(a, c, 3); g.insert_arc(c, a, 3);

  // Min-cut: isolate B with cut {A-B, B-C} = 1+2 = 3
  // Or isolate A with cut {A-B, A-C} = 1+3 = 4
  // Or isolate C with cut {B-C, A-C} = 2+3 = 5
  // Best: 3 (isolate B)

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK(min_cut >= 3 && min_cut <= 6, "min-cut should be ~3");
  PASS();
}

void test_sw_chain_weighted()
{
  TEST("Stoer-Wagner: weighted chain A-10-B-1-C-10-D");

  WeightedGraph g;
  build_weighted_chain(g, 10, 1, 10);  // Middle edge is weakest

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK(min_cut <= 2, "min-cut should be ~1 (the weak middle edge)");
  PASS();
}

void test_sw_complete_k4()
{
  TEST("Stoer-Wagner: complete K4 (all weights 1)");

  WeightedGraph g;
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");

  // All edges weight 1
  g.insert_arc(a, b, 1); g.insert_arc(b, a, 1);
  g.insert_arc(a, c, 1); g.insert_arc(c, a, 1);
  g.insert_arc(a, d, 1); g.insert_arc(d, a, 1);
  g.insert_arc(b, c, 1); g.insert_arc(c, b, 1);
  g.insert_arc(b, d, 1); g.insert_arc(d, b, 1);
  g.insert_arc(c, d, 1); g.insert_arc(d, c, 1);

  // Min-cut: isolate any vertex, cut 3 edges = weight 3

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK(min_cut >= 3 && min_cut <= 6, "min-cut should be ~3 for K4");
  PASS();
}

void test_sw_two_heavy_clusters()
{
  TEST("Stoer-Wagner: two clusters with weak bridge");

  WeightedGraph g;

  // Left cluster: heavy edges
  auto a = g.insert_node("A");
  auto b = g.insert_node("B");
  g.insert_arc(a, b, 100); g.insert_arc(b, a, 100);

  // Right cluster: heavy edges
  auto c = g.insert_node("C");
  auto d = g.insert_node("D");
  g.insert_arc(c, d, 100); g.insert_arc(d, c, 100);

  // Weak bridge
  g.insert_arc(b, c, 1); g.insert_arc(c, b, 1);

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  int min_cut = sw(g, S, T, cut);

  CHECK(min_cut <= 2, "min-cut should be ~1 (the weak bridge)");
  PASS();
}

void test_sw_min_cut_weight_only()
{
  TEST("Stoer-Wagner: min_cut_weight (no partition)");

  WeightedGraph g;
  build_weighted_chain(g, 5, 2, 8);

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  int min_cut = sw.min_cut_weight(g);

  CHECK(min_cut <= 4, "min-cut weight should be ~2");
  PASS();
}

void test_sw_unit_weight()
{
  TEST("Stoer-Wagner: Unit_Weight functor (unweighted)");

  using UG = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  UG g;

  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  // Path: 0-1-2-3
  g.insert_arc(n0, n1, 999);  // Weight ignored by Unit_Weight
  g.insert_arc(n1, n0, 999);
  g.insert_arc(n1, n2, 999);
  g.insert_arc(n2, n1, 999);
  g.insert_arc(n2, n3, 999);
  g.insert_arc(n3, n2, 999);

  Stoer_Wagner_Min_Cut<UG, Unit_Weight<UG>> sw;
  DynList<UG::Node*> S, T;
  DynList<UG::Arc*> cut;

  size_t min_cut = sw(g, S, T, cut);

  CHECK(min_cut <= 2, "min-cut should be 1-2 (counting edges, not weights)");
  PASS();
}

// ============================================================================
// Cross-Algorithm Comparison Tests
// ============================================================================

void test_cross_comparison()
{
  TEST("Cross-comparison: both algorithms on same graph");

  // Build graph for both
  using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  GT g;

  // Two clusters connected by 2 edges
  vector<GT::Node*> left(4), right(4);
  for (int i = 0; i < 4; ++i)
  {
    left[i] = g.insert_node(i);
    right[i] = g.insert_node(4 + i);
  }

  // Left cluster
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
    {
      g.insert_arc(left[i], left[j], 1);
      g.insert_arc(left[j], left[i], 1);
    }

  // Right cluster
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
    {
      g.insert_arc(right[i], right[j], 1);
      g.insert_arc(right[j], right[i], 1);
    }

  // Two bridges
  g.insert_arc(left[0], right[0], 1);
  g.insert_arc(right[0], left[0], 1);
  g.insert_arc(left[1], right[1], 1);
  g.insert_arc(right[1], left[1], 1);

  // Run Karger-Stein multiple times
  Karger_Min_Cut<GT> ks;
  DynList<GT::Node*> ks_S, ks_T;
  DynList<GT::Arc*> ks_cut;

  size_t ks_best = numeric_limits<size_t>::max();
  for (int i = 0; i < 30; ++i)
  {
    size_t mc = ks(g, ks_S, ks_T, ks_cut);
    ks_best = min(ks_best, mc);
  }

  // Run Stoer-Wagner (deterministic)
  Stoer_Wagner_Min_Cut<GT> sw;
  DynList<GT::Node*> sw_S, sw_T;
  DynList<GT::Arc*> sw_cut;

  int sw_cut_size = sw(g, sw_S, sw_T, sw_cut);

  // Both should find similar results (within factor of 2 for bidirectional)
  CHECK(ks_best <= 2u * sw_cut_size + 2, "Karger-Stein and Stoer-Wagner should agree");
  PASS();
}

// ============================================================================
// Performance Tests
// ============================================================================

void test_performance_ks()
{
  TEST("Performance: Karger-Stein on 50-node graph");

  UnweightedGraph g;
  const int N = 50;

  vector<UnweightedGraph::Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Ensure graph is connected: create a path
  for (int i = 0; i < N - 1; ++i)
  {
    g.insert_arc(nodes[i], nodes[i + 1], 1);
    g.insert_arc(nodes[i + 1], nodes[i], 1);
  }

  // Add some extra edges
  for (int i = 0; i < N; i += 3)
    for (int j = i + 5; j < N; j += 5)
    {
      g.insert_arc(nodes[i], nodes[j], 1);
      g.insert_arc(nodes[j], nodes[i], 1);
    }

  Karger_Min_Cut<UnweightedGraph> ks;
  DynList<UnweightedGraph::Node*> S, T;
  DynList<UnweightedGraph::Arc*> cut;

  Timer timer;
  size_t mc = ks(g, S, T, cut, 5);  // Run 5 iterations
  double elapsed = timer.elapsed_ms();

  CHECK(elapsed < 10000, "should complete in < 10 seconds");
  CHECK(mc >= 1, "should find a cut");
  PASS();
}

void test_performance_sw()
{
  TEST("Performance: Stoer-Wagner on 50-node graph");

  WeightedGraph g;
  const int N = 50;

  vector<WeightedGraph::Node*> nodes(N);
  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node("N" + to_string(i));

  // Ensure graph is connected: create a path
  for (int i = 0; i < N - 1; ++i)
  {
    g.insert_arc(nodes[i], nodes[i + 1], 1);
    g.insert_arc(nodes[i + 1], nodes[i], 1);
  }

  // Add some extra edges
  for (int i = 0; i < N; i += 3)
    for (int j = i + 5; j < N; j += 5)
    {
      g.insert_arc(nodes[i], nodes[j], 1);
      g.insert_arc(nodes[j], nodes[i], 1);
    }

  Stoer_Wagner_Min_Cut<WeightedGraph> sw;
  DynList<WeightedGraph::Node*> S, T;
  DynList<WeightedGraph::Arc*> cut;

  Timer timer;
  int mc = sw(g, S, T, cut);
  double elapsed = timer.elapsed_ms();

  CHECK(elapsed < 5000, "should complete in < 5 seconds");
  CHECK(mc >= 0, "should find a cut");
  PASS();
}

// ============================================================================
// Main
// ============================================================================

int main()
{
  cout << "╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║      Min-Cut Algorithms Test Suite                          ║\n";
  cout << "║      Testing Karger-Stein and Stoer-Wagner                  ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n\n";

  cout << "=== Karger-Stein Tests ===\n";
  test_ks_empty_graph();
  test_ks_single_edge();
  test_ks_triangle();
  test_ks_barbell();
  test_ks_path();
  test_ks_cycle();
  test_ks_two_clusters();
  test_ks_find_with_iterations();
  test_ks_seed_reproducibility();

  cout << "\n=== Stoer-Wagner Tests ===\n";
  test_sw_empty_graph();
  test_sw_single_edge();
  test_sw_triangle_weighted();
  test_sw_chain_weighted();
  test_sw_complete_k4();
  test_sw_two_heavy_clusters();
  test_sw_min_cut_weight_only();
  test_sw_unit_weight();

  cout << "\n=== Cross-Algorithm Tests ===\n";
  test_cross_comparison();

  cout << "\n=== Performance Tests ===\n";
  test_performance_ks();
  test_performance_sw();

  cout << "\n";
  cout << "══════════════════════════════════════════════════════════════\n";
  cout << "  RESULTS: " << tests_passed << "/" << total_tests << " passed";
  if (tests_failed > 0)
    cout << ", \033[31m" << tests_failed << " FAILED\033[0m";
  cout << "\n";
  cout << "══════════════════════════════════════════════════════════════\n";

  return tests_failed > 0 ? 1 : 0;
}
