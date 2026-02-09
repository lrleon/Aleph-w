/* Tests for tpl_maxflow.H - Advanced maximum flow algorithms
 *
 * Tests cover:
 * - Dinic's algorithm
 * - Capacity scaling
 * - Flow decomposition
 * - HLPP (Highest Label Preflow-Push)
 * - Flow statistics
 */

#include <gtest/gtest.h>
#include <tpl_maxflow.H>
#include <tpl_net.H>
#include <chrono>
#include <random>
#include <stdexcept>

using namespace Aleph;

namespace
{
  // Network type for tests
  using TestNet = Net_Graph<Net_Node<int>, Net_Arc<int, double>>;
  using Node = TestNet::Node;
  using Arc = TestNet::Arc;

  // Build a simple linear network: s -> a -> b -> t
  // Returns the source node
  Node* build_linear_network(TestNet& net)
  {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto t = net.insert_node(3);

    net.insert_arc(s, a, 10);  // cap=10
    net.insert_arc(a, b, 5);   // cap=5 (bottleneck)
    net.insert_arc(b, t, 10);  // cap=10

    return s;
  }

  // Build a diamond network: s -> a,b -> t
  Node* build_diamond_network(TestNet& net)
  {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto t = net.insert_node(3);

    net.insert_arc(s, a, 10);  // cap=10
    net.insert_arc(s, b, 10);  // cap=10
    net.insert_arc(a, t, 10);  // cap=10
    net.insert_arc(b, t, 10);  // cap=10

    return s;
  }

  // Build a complex network with multiple paths
  Node* build_complex_network(TestNet& net)
  {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto c = net.insert_node(3);
    auto d = net.insert_node(4);
    auto t = net.insert_node(5);

    net.insert_arc(s, a, 16);
    net.insert_arc(s, b, 13);
    net.insert_arc(a, b, 10);
    net.insert_arc(a, c, 12);
    net.insert_arc(b, a, 4);
    net.insert_arc(b, d, 14);
    net.insert_arc(c, b, 9);
    net.insert_arc(c, t, 20);
    net.insert_arc(d, c, 7);
    net.insert_arc(d, t, 4);

    return s;
  }

  // Build a grid network with directed arcs (flow only goes right and down)
  // This ensures single source (top-left) and single sink (bottom-right)
  Node* build_grid_network(TestNet& net, int rows, int cols, double capacity)
  {
    std::vector<std::vector<Node*>> nodes(rows, std::vector<Node*>(cols));
    int id = 0;

    // Create nodes
    for (int i = 0; i < rows; ++i)
      for (int j = 0; j < cols; ++j)
        nodes[i][j] = net.insert_node(id++);

    // Create horizontal arcs (left to right only)
    for (int i = 0; i < rows; ++i)
      for (int j = 0; j < cols - 1; ++j)
        net.insert_arc(nodes[i][j], nodes[i][j+1], capacity);

    // Create vertical arcs (top to bottom only)
    for (int i = 0; i < rows - 1; ++i)
      for (int j = 0; j < cols; ++j)
        net.insert_arc(nodes[i][j], nodes[i+1][j], capacity);

    return nodes[0][0];
  }

  [[maybe_unused]] void reset_flow(TestNet& net)
  {
    for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
      it.get_curr()->flow = 0;
  }
}


//==============================================================================
// Dinic's Algorithm Tests
//==============================================================================

TEST(DinicTest, LinearNetwork)
{
  TestNet net;
  build_linear_network(net);

  const auto flow = dinic_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 5.0);  // Bottleneck is middle arc
  EXPECT_TRUE(net.check_network());
}

TEST(DinicTest, DiamondNetwork)
{
  TestNet net;
  build_diamond_network(net);

  const auto flow = dinic_maximum_flow(net);
  (void)flow;

  EXPECT_DOUBLE_EQ(flow, 20.0);  // Both paths can carry 10
  EXPECT_TRUE(net.check_network());
}

TEST(DinicTest, ComplexNetwork)
{
  TestNet net;
  build_complex_network(net);

  auto flow = dinic_maximum_flow(net);

  // Known max flow for this network is 23
  EXPECT_DOUBLE_EQ(flow, 23.0);
  EXPECT_TRUE(net.check_network());
}

TEST(DinicTest, ZeroCapacity)
{
  // Test with zero capacity arc - should give zero flow
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);

  net.insert_arc(s, t, 0);  // Zero capacity

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 0.0);
}

TEST(DinicTest, SingleArc)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);
  net.insert_arc(s, t, 42);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 42.0);
}


//==============================================================================
// Capacity Scaling Tests
//==============================================================================

TEST(CapacityScalingTest, LinearNetwork)
{
  TestNet net;
  build_linear_network(net);

  auto flow = capacity_scaling_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 5.0);
  EXPECT_TRUE(net.check_network());
}

TEST(CapacityScalingTest, DiamondNetwork)
{
  TestNet net;
  build_diamond_network(net);

  auto flow = capacity_scaling_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 20.0);
  EXPECT_TRUE(net.check_network());
}

TEST(CapacityScalingTest, ComplexNetwork)
{
  TestNet net;
  build_complex_network(net);

  auto flow = capacity_scaling_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 23.0);
  EXPECT_TRUE(net.check_network());
}

TEST(CapacityScalingTest, LargeCapacities)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);

  net.insert_arc(s, a, 1000000);
  net.insert_arc(a, t, 1000000);

  auto flow = capacity_scaling_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 1000000.0);
}


//==============================================================================
// HLPP Tests
//==============================================================================

TEST(HLPPTest, LinearNetwork)
{
  TestNet net;
  build_linear_network(net);

  auto flow = hlpp_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 5.0);
  EXPECT_TRUE(net.check_network());
}

TEST(HLPPTest, DiamondNetwork)
{
  TestNet net;
  build_diamond_network(net);

  auto flow = hlpp_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 20.0);
  EXPECT_TRUE(net.check_network());
}

TEST(HLPPTest, ComplexNetwork)
{
  TestNet net;
  build_complex_network(net);

  auto flow = hlpp_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 23.0);
  EXPECT_TRUE(net.check_network());
}


//==============================================================================
// Flow Decomposition Tests
//==============================================================================

TEST(FlowDecompositionTest, LinearFlow)
{
  TestNet net;
  build_linear_network(net);
  dinic_maximum_flow(net);

  auto decomp = decompose_flow(net);

  EXPECT_EQ(decomp.num_paths(), 1);
  EXPECT_DOUBLE_EQ(decomp.total_flow(), 5.0);
}

TEST(FlowDecompositionTest, DiamondFlow)
{
  TestNet net;
  build_diamond_network(net);
  dinic_maximum_flow(net);

  auto decomp = decompose_flow(net);

  // Should decompose into 2 paths (through a and through b)
  EXPECT_EQ(decomp.num_paths(), 2);
  EXPECT_DOUBLE_EQ(decomp.total_flow(), 20.0);
}

TEST(FlowDecompositionTest, ComplexFlow)
{
  TestNet net;
  build_complex_network(net);
  dinic_maximum_flow(net);

  auto decomp = decompose_flow(net);

  // Total flow should match
  EXPECT_DOUBLE_EQ(decomp.total_flow(), 23.0);
  // Should have multiple paths
  EXPECT_GT(decomp.num_paths(), 1);
}

TEST(FlowDecompositionTest, ZeroFlow)
{
  // Build a network with a path but zero flow
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);

  net.insert_arc(s, a, 10);
  net.insert_arc(a, t, 10);
  // Don't run max-flow, so all arcs have 0 flow

  auto decomp = decompose_flow(net);

  EXPECT_EQ(decomp.num_paths(), 0);
  EXPECT_DOUBLE_EQ(decomp.total_flow(), 0.0);
}

TEST(FlowDecompositionTest, CycleDetection)
{
  // Build a network with a cycle: s -> a -> b -> a (cycle a-b-a) -> t
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto t = net.insert_node(3);

  // Main path s -> a -> t
  auto arc_sa = net.insert_arc(s, a, 10);
  auto arc_at = net.insert_arc(a, t, 10);

  // Cycle a -> b -> a
  auto arc_ab = net.insert_arc(a, b, 5);
  auto arc_ba = net.insert_arc(b, a, 5);

  // Manually set flow to create a cycle
  // Flow goes s -> a -> t (5 units)
  // And circulates a -> b -> a (5 units)
  arc_sa->flow = 5;
  arc_at->flow = 5;
  arc_ab->flow = 5;
  arc_ba->flow = 5;

  // Verify manual flow setup is valid (conservation holds)
  // Node a: in=5(s)+5(b)=10, out=5(t)+5(b)=10. OK.
  // Node b: in=5(a), out=5(a). OK.

  auto decomp = decompose_flow(net);

  // Should have 1 path (s->a->t) and 1 cycle (a->b->a)
  EXPECT_EQ(decomp.num_paths(), 1);
  EXPECT_EQ(decomp.num_cycles(), 1);

  EXPECT_DOUBLE_EQ(decomp.total_flow(), 5.0); // Only path flow counts towards total s-t flow

  // Verify cycle details
  const auto& cycle = decomp.cycles.get_first();
  EXPECT_DOUBLE_EQ(cycle.flow, 5.0);

  // Verify path details
  const auto& path = decomp.paths.get_first();
  EXPECT_DOUBLE_EQ(path.flow, 5.0);
}


//==============================================================================
// Flow Statistics Tests
//==============================================================================

TEST(FlowStatisticsTest, BeforeFlow)
{
  TestNet net;
  build_diamond_network(net);

  auto stats = compute_flow_statistics(net);

  EXPECT_DOUBLE_EQ(stats.total_capacity, 40.0);
  EXPECT_EQ(stats.num_empty_arcs, 4);
  EXPECT_EQ(stats.num_saturated_arcs, 0);
  EXPECT_EQ(stats.num_partial_arcs, 0);
}

TEST(FlowStatisticsTest, AfterFlow)
{
  TestNet net;
  build_diamond_network(net);
  dinic_maximum_flow(net);

  auto stats = compute_flow_statistics(net);

  EXPECT_DOUBLE_EQ(stats.total_flow, 20.0);
  EXPECT_DOUBLE_EQ(stats.utilization, 0.5);  // 20/40
}


//==============================================================================
// Min-Cut Validation Tests (Max-Flow Min-Cut Theorem)
//==============================================================================

// Helper function to compute min-cut capacity by BFS from source
template <class Net>
double compute_min_cut_capacity(Net& net)
{
  using Node = typename Net::Node;
  using Arc = typename Net::Arc;
  
  auto source = net.get_source();
  (void)net.get_sink();
  
  // Find reachable nodes from source via residual network
  std::set<Node*> reachable;
  DynListQueue<Node*> q;
  
  reachable.insert(source);
  q.put(source);
  
  while (!q.is_empty())
  {
    Node* u = q.front();
    q.get();
    
    // Explore all adjacent arcs
    for (typename Net::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* v = net.get_connected_node(arc, u);
      
      if (reachable.count(v) > 0)
        continue;  // Already visited
      
      // Check if there is residual capacity
      bool forward = (net.get_src_node(arc) == u);
      double residual = forward ? (arc->cap - arc->flow) : arc->flow;
      
      if (residual > 1e-9)  // Has residual capacity
      {
        reachable.insert(v);
        q.put(v);
      }
    }
  }
  
  // Compute cut capacity: sum of capacities of arcs from reachable to non-reachable
  double cut_capacity = 0.0;
  
  for (auto u : reachable)
  {
    for (Out_Iterator<Net> it(u); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* v = net.get_tgt_node(arc);
      
      if (reachable.count(v) == 0)  // Arc crosses the cut
        cut_capacity += arc->cap;
    }
  }
  
  return cut_capacity;
}

// Helper to verify max-flow = min-cut
template <class Net>
bool verify_max_flow_min_cut(Net& net, double max_flow)
{
  double min_cut = compute_min_cut_capacity(net);
  return std::abs(max_flow - min_cut) < 1e-6;
}

TEST(MinCutTest, LinearNetwork)
{
  TestNet net;
  build_linear_network(net);
  
  auto flow = dinic_maximum_flow(net);
  
  // Verify max-flow = min-cut theorem
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
  
  // The min-cut should be at the bottleneck (middle arc with cap=5)
  double min_cut = compute_min_cut_capacity(net);
  EXPECT_DOUBLE_EQ(min_cut, 5.0);
}

TEST(MinCutTest, DiamondNetwork)
{
  TestNet net;
  build_diamond_network(net);
  
  auto flow = dinic_maximum_flow(net);
  
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
  
  double min_cut = compute_min_cut_capacity(net);
  EXPECT_DOUBLE_EQ(min_cut, 20.0);
}

TEST(MinCutTest, ComplexNetwork)
{
  TestNet net;
  build_complex_network(net);
  
  auto flow = dinic_maximum_flow(net);
  
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
  
  double min_cut = compute_min_cut_capacity(net);
  EXPECT_DOUBLE_EQ(min_cut, 23.0);
}

TEST(MinCutTest, GridNetwork)
{
  TestNet net;
  build_grid_network(net, 5, 5, 10);
  
  auto flow = edmonds_karp_maximum_flow(net);
  
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
}

TEST(MinCutTest, SingleBottleneck)
{
  // Build network where min-cut is obvious
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto t = net.insert_node(3);
  
  // Wide source side, narrow bottleneck, wide sink side
  net.insert_arc(s, a, 100);
  net.insert_arc(s, b, 100);
  net.insert_arc(a, t, 5);   // Bottleneck
  net.insert_arc(b, t, 5);   // Bottleneck
  
  auto flow = dinic_maximum_flow(net);
  
  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
  
  double min_cut = compute_min_cut_capacity(net);
  EXPECT_DOUBLE_EQ(min_cut, 10.0);
}

TEST(MinCutTest, ParallelArcs)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);
  
  // Three parallel arcs
  net.insert_arc(s, t, 10);
  net.insert_arc(s, t, 20);
  net.insert_arc(s, t, 30);
  
  auto flow = dinic_maximum_flow(net);
  
  EXPECT_DOUBLE_EQ(flow, 60.0);
  EXPECT_TRUE(verify_max_flow_min_cut(net, flow));
  
  double min_cut = compute_min_cut_capacity(net);
  EXPECT_DOUBLE_EQ(min_cut, 60.0);
}

TEST(MinCutTest, AllAlgorithmsGiveSameMinCut)
{
  // Build 4 identical networks
  TestNet net1, net2, net3, net4;
  build_complex_network(net1);
  build_complex_network(net2);
  build_complex_network(net3);
  build_complex_network(net4);
  
  auto flow_ff = ford_fulkerson_maximum_flow(net1);
  auto flow_ek = edmonds_karp_maximum_flow(net2);
  auto flow_dinic = dinic_maximum_flow(net3);
  auto flow_cs = capacity_scaling_maximum_flow(net4);
  
  // All should satisfy max-flow = min-cut
  EXPECT_TRUE(verify_max_flow_min_cut(net1, flow_ff));
  EXPECT_TRUE(verify_max_flow_min_cut(net2, flow_ek));
  EXPECT_TRUE(verify_max_flow_min_cut(net3, flow_dinic));
  EXPECT_TRUE(verify_max_flow_min_cut(net4, flow_cs));
  
  // All min-cuts should be equal
  double cut1 = compute_min_cut_capacity(net1);
  double cut2 = compute_min_cut_capacity(net2);
  double cut3 = compute_min_cut_capacity(net3);
  double cut4 = compute_min_cut_capacity(net4);
  
  EXPECT_DOUBLE_EQ(cut1, cut2);
  EXPECT_DOUBLE_EQ(cut2, cut3);
  EXPECT_DOUBLE_EQ(cut3, cut4);
}

TEST(MinCutTest, VerifyCutPartition)
{
  // Verify that the cut actually partitions the graph correctly
  TestNet net;
  build_diamond_network(net);
  
  const auto flow = dinic_maximum_flow(net);
  (void)flow;
  
  auto source = net.get_source();
  auto sink = net.get_sink();
  
  // Find reachable set from source
  std::set<Node*> S_side;  // Source side of cut
  DynListQueue<Node*> q;
  
  S_side.insert(source);
  q.put(source);
  
  while (!q.is_empty())
  {
    Node* u = q.front();
    q.get();
    
    for (typename TestNet::Node_Arc_Iterator it(u); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* v = net.get_connected_node(arc, u);
      
      if (S_side.count(v) > 0)
        continue;
      
      bool forward = (net.get_src_node(arc) == u);
      double residual = forward ? (arc->cap - arc->flow) : arc->flow;
      
      if (residual > 1e-9)
      {
        S_side.insert(v);
        q.put(v);
      }
    }
  }
  
  // Source should be in S, sink should not be in S
  EXPECT_TRUE(S_side.count(source) > 0);
  EXPECT_FALSE(S_side.count(sink) > 0);
  
  // Verify all arcs crossing the cut are saturated (forward) or empty (backward)
  for (auto u : S_side)
  {
    for (Out_Iterator<TestNet> it(u); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* v = net.get_tgt_node(arc);
      
      if (S_side.count(v) == 0)  // Arc crosses the cut
      {
        // Forward arc crossing cut should be saturated
        EXPECT_NEAR(arc->flow, arc->cap, 1e-6);
      }
    }
    
    for (In_Iterator<TestNet> it(u); it.has_curr(); it.next_ne())
    {
      Arc* arc = it.get_curr();
      Node* v = net.get_src_node(arc);
      
      if (S_side.count(v) == 0)  // Arc crosses the cut (backward)
      {
        // Backward arc crossing cut should have zero flow
        EXPECT_NEAR(arc->flow, 0.0, 1e-6);
      }
    }
  }
}


//==============================================================================
// Algorithm Comparison Tests
//==============================================================================

TEST(AlgorithmComparisonTest, AllAlgorithmsAgree)
{
  // Build 4 identical networks
  TestNet net1, net2, net3, net4;
  build_complex_network(net1);
  build_complex_network(net2);
  build_complex_network(net3);
  build_complex_network(net4);

  auto flow_ff = ford_fulkerson_maximum_flow(net1);
  auto flow_ek = edmonds_karp_maximum_flow(net2);
  auto flow_dinic = dinic_maximum_flow(net3);
  auto flow_cs = capacity_scaling_maximum_flow(net4);

  EXPECT_DOUBLE_EQ(flow_ff, flow_ek);
  EXPECT_DOUBLE_EQ(flow_ek, flow_dinic);
  EXPECT_DOUBLE_EQ(flow_dinic, flow_cs);
}

TEST(AlgorithmComparisonTest, GridNetwork)
{
  const int size = 5;

  TestNet net1, net2, net3;
  build_grid_network(net1, size, size, 10);
  build_grid_network(net2, size, size, 10);
  build_grid_network(net3, size, size, 10);

  auto flow_ek = edmonds_karp_maximum_flow(net1);
  auto flow_dinic = dinic_maximum_flow(net2);
  auto flow_cs = capacity_scaling_maximum_flow(net3);

  EXPECT_DOUBLE_EQ(flow_ek, flow_dinic);
  EXPECT_DOUBLE_EQ(flow_dinic, flow_cs);
}


//==============================================================================
// Edge Cases
//==============================================================================

TEST(EdgeCasesTest, ParallelArcs)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);

  // Multiple arcs between same nodes
  net.insert_arc(s, t, 10);
  net.insert_arc(s, t, 20);
  net.insert_arc(s, t, 30);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 60.0);  // Sum of capacities
}

TEST(EdgeCasesTest, SourceBottleneck)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);

  net.insert_arc(s, a, 5);   // Bottleneck at source
  net.insert_arc(a, t, 100);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 5.0);
}

TEST(EdgeCasesTest, SinkBottleneck)
{
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);

  net.insert_arc(s, a, 100);
  net.insert_arc(a, t, 5);   // Bottleneck at sink

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 5.0);
}


//==============================================================================
// Functor Tests
//==============================================================================

TEST(FunctorTest, DinicFunctor)
{
  TestNet net;
  build_diamond_network(net);

  Dinic_Maximum_Flow<TestNet> algo;
  auto flow = algo(net);
  EXPECT_DOUBLE_EQ(flow, 20.0);
}

TEST(FunctorTest, CapacityScalingFunctor)
{
  TestNet net;
  build_diamond_network(net);

  Capacity_Scaling_Maximum_Flow<TestNet> algo;
  auto flow = algo(net);
  EXPECT_DOUBLE_EQ(flow, 20.0);
}

TEST(FunctorTest, HLPPFunctor)
{
  TestNet net;
  build_diamond_network(net);

  HLPP_Maximum_Flow<TestNet> algo;
  auto flow = algo(net);
  EXPECT_DOUBLE_EQ(flow, 20.0);
}

TEST(FunctorTest, DecomposeFunctor)
{
  TestNet net;
  build_diamond_network(net);
  dinic_maximum_flow(net);

  Decompose_Flow<TestNet> decomposer;
  auto decomp = decomposer(net);

  EXPECT_DOUBLE_EQ(decomp.total_flow(), 20.0);
}


//==============================================================================
// Performance Benchmark
//==============================================================================

TEST(PerformanceTest, Benchmark)
{
  const int grid_size = 10;
  std::cout << "\n=== Max Flow Performance Benchmark (Grid "
            << grid_size << "x" << grid_size << ") ===\n";

  auto benchmark = [&](const std::string& name, auto algo) {
    TestNet net;
    build_grid_network(net, grid_size, grid_size, 100);

    auto start = std::chrono::high_resolution_clock::now();
    auto flow = algo(net);
    auto end = std::chrono::high_resolution_clock::now();
    double ms = std::chrono::duration<double, std::milli>(end - start).count();

    std::cout << name << ": " << ms << " ms, flow=" << flow << "\n";
    return flow;
  };

  auto flow_ek = benchmark("Edmonds-Karp    ", [](TestNet& n) {
    return edmonds_karp_maximum_flow(n);
  });

  auto flow_dinic = benchmark("Dinic           ", [](TestNet& n) {
    return dinic_maximum_flow(n);
  });

  auto flow_cs = benchmark("Capacity Scaling", [](TestNet& n) {
    return capacity_scaling_maximum_flow(n);
  });

  // All should give same result
  EXPECT_DOUBLE_EQ(flow_ek, flow_dinic);
  EXPECT_DOUBLE_EQ(flow_dinic, flow_cs);
}


// =============================================================================
// Regression Tests
// =============================================================================

TEST(RegressionTest, HLPPDisconnectedNodeRelabel)
{
  // This test covers a bug where HLPP could set height[u] = 2*n+1
  // when relabeling a node with no residual neighbors, causing
  // out-of-bounds access to buckets[2*n+1].
  //
  // Network structure that can trigger this:
  // s -> a -> t (bottleneck at a->t)
  // If 'a' has excess but all outgoing arcs are saturated and
  // all incoming arcs have no reverse capacity, relabel would
  // set height[a] = min_height+1 where min_height could be 2*n.

  TestNet net;
  auto s = net.insert_node(0);  // source (inferred)
  auto a = net.insert_node(1);  // intermediate
  auto t = net.insert_node(2);  // sink (inferred)

  // Create a very small capacity to sink, so 'a' can get excess
  net.insert_arc(s, a, 100.0);  // large capacity
  net.insert_arc(a, t, 1.0);    // small capacity (bottleneck)

  // Run HLPP - should not crash or access out of bounds
  double flow = hlpp_maximum_flow(net);

  // The max flow is limited by the bottleneck
  EXPECT_DOUBLE_EQ(flow, 1.0);

  // Verify the flow is valid (conservation at intermediate node)
  double flow_in_a = 0, flow_out_a = 0;
  for (typename TestNet::Node_Arc_Iterator it(a); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      if (net.get_tgt_node(arc) == a)
        flow_in_a += arc->flow;
      else
        flow_out_a += arc->flow;
    }
  EXPECT_DOUBLE_EQ(flow_in_a, flow_out_a);
}


//==============================================================================
// Bug-hunting tests: Anti-parallel arcs
//==============================================================================

// Helper: build network with anti-parallel arcs (A->B and B->A)
// This stresses backward-edge residual logic because there are REAL
// reverse arcs in addition to the virtual backward edges.
//
//   s --20--> a --10--> t
//             |   ^
//          7  |   | 5
//             v   |
//             b --8--> t
//
// Plus anti-parallel: a->b(7), b->a(5)
// Max flow should be 20 (limited by s->a), with paths:
//  s->a->t: 10, s->a->b->t: 7, and s->a flow via b->a back to a->t: up to 5
Node* build_antiparallel_network(TestNet& net)
{
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto t = net.insert_node(3);

  net.insert_arc(s, a, 20);
  net.insert_arc(a, t, 10);
  net.insert_arc(a, b, 7);   // Forward a->b
  net.insert_arc(b, a, 5);   // Anti-parallel b->a
  net.insert_arc(b, t, 8);

  return s;
}

TEST(AntiParallelTest, AllAlgorithmsAgreeOnAntiParallel)
{
  // All algorithms must give the same result on anti-parallel arcs
  TestNet net1, net2, net3, net4, net5;
  build_antiparallel_network(net1);
  build_antiparallel_network(net2);
  build_antiparallel_network(net3);
  build_antiparallel_network(net4);
  build_antiparallel_network(net5);

  auto flow_ff    = ford_fulkerson_maximum_flow(net1);
  auto flow_ek    = edmonds_karp_maximum_flow(net2);
  auto flow_dinic = dinic_maximum_flow(net3);
  auto flow_cs    = capacity_scaling_maximum_flow(net4);
  auto flow_hlpp  = hlpp_maximum_flow(net5);

  // Max flow is 18: s->a(20), a->t(10), a->b(7), b->t(8) => 10+8=18
  // (limited by outgoing capacity from a: 10+7=17? no, b->a adds 5 more to a)
  // Actually just verify they all agree and conservation holds
  EXPECT_DOUBLE_EQ(flow_ff, flow_ek);
  EXPECT_DOUBLE_EQ(flow_ek, flow_dinic);
  EXPECT_DOUBLE_EQ(flow_dinic, flow_cs);
  EXPECT_DOUBLE_EQ(flow_cs, flow_hlpp);

  EXPECT_TRUE(net1.check_network());
  EXPECT_TRUE(net2.check_network());
  EXPECT_TRUE(net3.check_network());
  EXPECT_TRUE(net4.check_network());
  EXPECT_TRUE(net5.check_network());

  // Verify min-cut theorem for all
  EXPECT_TRUE(verify_max_flow_min_cut(net1, flow_ff));
  EXPECT_TRUE(verify_max_flow_min_cut(net3, flow_dinic));
  EXPECT_TRUE(verify_max_flow_min_cut(net5, flow_hlpp));
}

TEST(AntiParallelTest, PureAntiParallel)
{
  // s -> a -> t  AND  s -> b -> a (with b->a anti-parallel to a->b implicit)
  // But also a -> b exists
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto t = net.insert_node(3);

  net.insert_arc(s, a, 10);
  net.insert_arc(s, b, 10);
  net.insert_arc(a, b, 5);   // a->b
  net.insert_arc(b, a, 5);   // b->a (anti-parallel)
  net.insert_arc(a, t, 15);
  net.insert_arc(b, t, 15);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 20.0);  // Both s->a and s->b push 10 each
  EXPECT_TRUE(net.check_network());
}


//==============================================================================
// Bug-hunting tests: Capacity Scaling with fractional capacities
//==============================================================================

TEST(CapacityScalingTest, FractionalCapacities)
{
  // Capacity scaling now scales down to the minimum positive capacity,
  // so fractional capacities are handled correctly.
  auto build_net = [](TestNet& net) {
    auto s = net.insert_node(0);
    auto t = net.insert_node(1);
    net.insert_arc(s, t, 0.5);
  };

  TestNet net_cs, net_dinic;
  build_net(net_cs);
  build_net(net_dinic);

  auto flow_cs = capacity_scaling_maximum_flow(net_cs);
  auto flow_dinic = dinic_maximum_flow(net_dinic);

  EXPECT_DOUBLE_EQ(flow_dinic, 0.5);
  EXPECT_DOUBLE_EQ(flow_cs, 0.5);  // Now correctly finds fractional flow
}

TEST(CapacityScalingTest, MixedIntegerFractional)
{
  // Network where integer-capacity paths exist but fractional adds more
  // Must use separate networks since algorithms modify flow in-place

  auto build_net = [](TestNet& net) {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto t = net.insert_node(3);
    net.insert_arc(s, a, 10);
    net.insert_arc(s, b, 0.7);  // Fractional
    net.insert_arc(a, t, 10);
    net.insert_arc(b, t, 0.7);  // Fractional
  };

  TestNet net_cs, net_dinic;
  build_net(net_cs);
  build_net(net_dinic);

  auto flow_cs = capacity_scaling_maximum_flow(net_cs);
  auto flow_dinic = dinic_maximum_flow(net_dinic);

  // Both should find the full flow: 10 + 0.7 = 10.7
  EXPECT_DOUBLE_EQ(flow_dinic, 10.7);
  EXPECT_DOUBLE_EQ(flow_cs, 10.7);
}


//==============================================================================
// Bug-hunting tests: HLPP stress with conservation verification
//==============================================================================

// Explicit conservation check at every intermediate node
template <class Net>
void verify_flow_conservation(const Net& net)
{
  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
    {
      auto p = it.get_curr();
      if (net.is_source(p) or net.is_sink(p))
        continue;

      double in_flow = 0, out_flow = 0;
      for (typename Net::Node_Arc_Iterator ait(p); ait.has_curr(); ait.next_ne())
        {
          auto arc = ait.get_curr();
          if (net.get_tgt_node(arc) == p)
            in_flow += arc->flow;
          else if (net.get_src_node(arc) == p)
            out_flow += arc->flow;
        }

      EXPECT_NEAR(in_flow, out_flow, 1e-6)
        << "Conservation violated at node with in=" << in_flow
        << " out=" << out_flow;
    }
}

TEST(HLPPStressTest, LadderNetwork)
{
  // Ladder graph: two parallel chains connected by rungs
  // Forces many relabeling operations
  //
  //  s -> a0 -> a1 -> a2 -> ... -> aN -> t
  //       |      |      |              |
  //       v      v      v              v
  //  s -> b0 -> b1 -> b2 -> ... -> bN -> t
  //
  const int N = 15;
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);

  auto chain_a = Array<Node *>::create(N);
  auto chain_b = Array<Node *>::create(N);
  for (int i = 0; i < N; ++i)
    {
      chain_a[i] = net.insert_node(10 + i);
      chain_b[i] = net.insert_node(30 + i);
    }

  // Connect source to first nodes
  net.insert_arc(s, chain_a[0], 100);
  net.insert_arc(s, chain_b[0], 100);

  // Chains
  for (int i = 0; i < N - 1; ++i)
    {
      net.insert_arc(chain_a[i], chain_a[i+1], 50);
      net.insert_arc(chain_b[i], chain_b[i+1], 50);
    }

  // Rungs (cross connections)
  for (int i = 0; i < N; ++i)
    {
      net.insert_arc(chain_a[i], chain_b[i], 10);
      net.insert_arc(chain_b[i], chain_a[i], 10);  // Anti-parallel rungs
    }

  // Connect last nodes to sink
  net.insert_arc(chain_a[N-1], t, 100);
  net.insert_arc(chain_b[N-1], t, 100);

  // Build identical networks (can't copy Net_Graph, so rebuild)
  auto build_ladder = [&](TestNet& n) {
    auto s2 = n.insert_node(0);
    auto t2 = n.insert_node(1);
    auto ca = Array<Node *>::create(N);
    auto cb = Array<Node *>::create(N);
    for (int i = 0; i < N; ++i)
      {
        ca[i] = n.insert_node(10 + i);
        cb[i] = n.insert_node(30 + i);
      }
    n.insert_arc(s2, ca[0], 100);
    n.insert_arc(s2, cb[0], 100);
    for (int i = 0; i < N - 1; ++i)
      {
        n.insert_arc(ca[i], ca[i+1], 50);
        n.insert_arc(cb[i], cb[i+1], 50);
      }
    for (int i = 0; i < N; ++i)
      {
        n.insert_arc(ca[i], cb[i], 10);
        n.insert_arc(cb[i], ca[i], 10);
      }
    n.insert_arc(ca[N-1], t2, 100);
    n.insert_arc(cb[N-1], t2, 100);
  };

  TestNet net2, net3;
  build_ladder(net2);
  build_ladder(net3);

  auto flow_dinic = dinic_maximum_flow(net);
  auto flow_hlpp  = hlpp_maximum_flow(net2);
  auto flow_ek    = edmonds_karp_maximum_flow(net3);

  EXPECT_DOUBLE_EQ(flow_dinic, flow_hlpp);
  EXPECT_DOUBLE_EQ(flow_dinic, flow_ek);

  // Explicit conservation check
  EXPECT_TRUE(net.check_network());
  EXPECT_TRUE(net2.check_network());
  verify_flow_conservation<TestNet>(net);
  verify_flow_conservation<TestNet>(net2);
}

TEST(HLPPStressTest, HighExcessBottleneck)
{
  // Graph designed to create large excess at intermediate nodes
  // s pushes 1000 to a, but a can only drain 1 to t
  // HLPP must push 999 back to s
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto c = net.insert_node(3);
  auto t = net.insert_node(4);

  net.insert_arc(s, a, 1000);
  net.insert_arc(s, b, 1000);
  net.insert_arc(a, c, 1);       // Tiny bottleneck
  net.insert_arc(b, c, 1);       // Tiny bottleneck
  net.insert_arc(c, t, 2);
  net.insert_arc(a, b, 500);     // Cross connection
  net.insert_arc(b, a, 500);     // Anti-parallel

  auto flow = hlpp_maximum_flow(net);

  EXPECT_DOUBLE_EQ(flow, 2.0);
  EXPECT_TRUE(net.check_network());
  verify_flow_conservation<TestNet>(net);
}


//==============================================================================
// Bug-hunting tests: Flow decomposition consistency
//==============================================================================

TEST(FlowDecompositionTest, ArcFlowConsistency)
{
  // After decomposition, for each arc:
  //   sum of (path.flow for paths using that arc) == arc.flow
  TestNet net;
  build_complex_network(net);
  dinic_maximum_flow(net);

  // Save original flows
  DynMapTree<Arc*, double> original_flow;
  for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      original_flow[arc] = arc->flow;
    }

  auto decomp = decompose_flow(net);

  // Rebuild flow from decomposition
  DynMapTree<Arc*, double> reconstructed_flow;
  for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
    reconstructed_flow[it.get_curr()] = 0.0;

  for (auto path_it = decomp.paths.get_it(); path_it.has_curr(); path_it.next_ne())
    {
      const auto& path = path_it.get_curr();
      for (auto arc_it = path.arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
        reconstructed_flow[arc_it.get_curr()] += path.flow;
    }

  for (auto cycle_it = decomp.cycles.get_it(); cycle_it.has_curr(); cycle_it.next_ne())
    {
      const auto& cycle = cycle_it.get_curr();
      for (auto arc_it = cycle.arcs.get_it(); arc_it.has_curr(); arc_it.next_ne())
        reconstructed_flow[arc_it.get_curr()] += cycle.flow;
    }

  // Every arc's flow should be fully accounted for
  for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      EXPECT_NEAR(reconstructed_flow[arc], original_flow[arc], 1e-6)
        << "Arc flow mismatch: reconstructed=" << reconstructed_flow[arc]
        << " original=" << original_flow[arc];
    }
}

TEST(FlowDecompositionTest, Phase2CycleClosingNode)
{
  // Create a flow with a cycle NOT reachable from source (Phase 2 territory)
  // After fixing the Phase-2 bug, cycle.nodes should include the closing node
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto c = net.insert_node(3);
  auto t = net.insert_node(4);

  // Main path s->a->t
  auto arc_sa = net.insert_arc(s, a, 10);
  auto arc_at = net.insert_arc(a, t, 10);

  // Isolated cycle b->c->b (not connected to source outflow)
  auto arc_bc = net.insert_arc(b, c, 5);
  auto arc_cb = net.insert_arc(c, b, 5);

  // Dummy arcs to keep b,c in the network but no flow from source to them
  net.insert_arc(s, b, 0);  // zero capacity
  net.insert_arc(c, t, 0);  // zero capacity

  // Set up flow manually: path s->a->t with flow 5, cycle b->c->b with flow 3
  arc_sa->flow = 5;
  arc_at->flow = 5;
  arc_bc->flow = 3;
  arc_cb->flow = 3;

  auto decomp = decompose_flow(net);

  EXPECT_EQ(decomp.num_paths(), 1);
  EXPECT_DOUBLE_EQ(decomp.total_flow(), 5.0);

  // Should find the b->c->b cycle in Phase 2
  EXPECT_GE(decomp.num_cycles(), 1);

  if (decomp.num_cycles() >= 1)
    {
      const auto& cycle = decomp.cycles.get_first();
      EXPECT_DOUBLE_EQ(cycle.flow, 3.0);

      // Verify closing node: cycle.nodes should have one more element than
      // cycle.arcs, and first node == last node
      size_t n_nodes = 0, n_arcs = 0;
      for (auto it = cycle.nodes.get_it(); it.has_curr(); it.next_ne())
        ++n_nodes;
      for (auto it = cycle.arcs.get_it(); it.has_curr(); it.next_ne())
        ++n_arcs;

      EXPECT_EQ(n_nodes, n_arcs + 1)
        << "Cycle should have nodes.size() == arcs.size() + 1 (closing node)";

      // First and last node should be the same (cycle is closed)
      auto first_node = cycle.nodes.get_first();
      auto last_node = cycle.nodes.get_last();
      EXPECT_EQ(first_node, last_node)
        << "Cycle first and last node should be the same";
    }
}

TEST(FlowDecompositionTest, Phase1CycleClosingNode)
{
  // Also verify Phase-1 cycles have proper format for symmetry
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);
  auto t = net.insert_node(3);

  auto arc_sa = net.insert_arc(s, a, 10);
  auto arc_at = net.insert_arc(a, t, 10);
  auto arc_ab = net.insert_arc(a, b, 5);
  auto arc_ba = net.insert_arc(b, a, 5);

  // Flow: s->a->t(5), cycle a->b->a(5)
  arc_sa->flow = 5;
  arc_at->flow = 5;
  arc_ab->flow = 5;
  arc_ba->flow = 5;

  auto decomp = decompose_flow(net);

  EXPECT_EQ(decomp.num_paths(), 1);
  EXPECT_GE(decomp.num_cycles(), 1);

  if (decomp.num_cycles() >= 1)
    {
      const auto& cycle = decomp.cycles.get_first();

      size_t n_nodes = 0, n_arcs = 0;
      for (auto it = cycle.nodes.get_it(); it.has_curr(); it.next_ne())
        ++n_nodes;
      for (auto it = cycle.arcs.get_it(); it.has_curr(); it.next_ne())
        ++n_arcs;

      EXPECT_EQ(n_nodes, n_arcs + 1)
        << "Phase-1 cycle should also have nodes.size() == arcs.size() + 1";

      auto first_node = cycle.nodes.get_first();
      auto last_node = cycle.nodes.get_last();
      EXPECT_EQ(first_node, last_node)
        << "Phase-1 cycle first and last node should be the same";
    }
}


//==============================================================================
// Bug-hunting tests: Disconnected sink
//==============================================================================

TEST(DisconnectedTest, ZeroCapacityBottleneck)
{
  // Single-source/single-sink network where all paths have zero capacity
  // This is the closest to "disconnected" that Net_Graph allows
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto t = net.insert_node(2);

  net.insert_arc(s, a, 0);  // Zero capacity bottleneck
  net.insert_arc(a, t, 10);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 0.0);
}

TEST(DisconnectedTest, InvalidNetworkThrows)
{
  // A truly disconnected network violates single source/sink
  // (Net_Graph determines source/sink by in/out-degree)
  // Algorithms should throw domain_error
  TestNet net;
  auto s = net.insert_node(0);
  auto a = net.insert_node(1);
  auto b = net.insert_node(2);  // Another "source" (no incoming arcs)
  auto t = net.insert_node(3);

  net.insert_arc(s, a, 10);
  net.insert_arc(b, t, 10);

  EXPECT_THROW(dinic_maximum_flow(net), std::exception);
  EXPECT_THROW(capacity_scaling_maximum_flow(net), std::exception);
  EXPECT_THROW(hlpp_maximum_flow(net), std::exception);
}


//==============================================================================
// Bug-hunting tests: Dinic with deep level graph
//==============================================================================

TEST(DinicStressTest, LongChain)
{
  // s -> n1 -> n2 -> ... -> n50 -> t
  // Forces 1 BFS phase with 51 levels
  const int chain_len = 50;
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);

  auto prev = s;
  for (int i = 0; i < chain_len; ++i)
    {
      auto curr = net.insert_node(10 + i);
      net.insert_arc(prev, curr, 100);
      prev = curr;
    }
  net.insert_arc(prev, t, 100);

  auto flow = dinic_maximum_flow(net);
  EXPECT_DOUBLE_EQ(flow, 100.0);
  EXPECT_TRUE(net.check_network());
}

TEST(DinicStressTest, WideParallel)
{
  // s -> {a1, a2, ..., a100} -> t, all with cap 1
  // Many parallel paths
  const int width = 100;
  TestNet net;
  auto s = net.insert_node(0);
  auto t = net.insert_node(1);

  for (int i = 0; i < width; ++i)
    {
      auto mid = net.insert_node(10 + i);
      net.insert_arc(s, mid, 1);
      net.insert_arc(mid, t, 1);
    }

  // Build a second identical network for HLPP
  TestNet net2;
  auto s2 = net2.insert_node(0);
  auto t2 = net2.insert_node(1);
  for (int i = 0; i < width; ++i)
    {
      auto mid = net2.insert_node(10 + i);
      net2.insert_arc(s2, mid, 1);
      net2.insert_arc(mid, t2, 1);
    }

  auto flow_dinic = dinic_maximum_flow(net);
  auto flow_hlpp  = hlpp_maximum_flow(net2);

  EXPECT_DOUBLE_EQ(flow_dinic, static_cast<double>(width));
  EXPECT_DOUBLE_EQ(flow_hlpp, static_cast<double>(width));
  EXPECT_TRUE(net.check_network());
  EXPECT_TRUE(net2.check_network());
}


//==============================================================================
// Randomized stress tests: all algorithms must agree on random graphs
//==============================================================================

namespace
{
  // Integer flow network type
  using IntNet = Net_Graph<Net_Node<int>, Net_Arc<int, int>>;

  // Build a random network with n nodes, m arcs, max_cap capacity
  template <class NetT>
  void build_random_network(NetT & net, int n, int m, int max_cap,
                            unsigned seed)
  {
    ah_invalid_argument_if(n < 3)
      << "build_random_network requires n >= 3";

    using NodeT = typename NetT::Node;
    std::mt19937 rng(seed);
    std::uniform_int_distribution<int> node_dist(0, n - 1);
    std::uniform_int_distribution<int> cap_dist(1, max_cap);

    std::vector<NodeT *> nodes(n);
    for (int i = 0; i < n; ++i)
      nodes[i] = net.insert_node(i);

    // Ensure source (0) has outgoing and sink (n-1) has incoming
    net.insert_arc(nodes[0], nodes[1], cap_dist(rng));
    net.insert_arc(nodes[n - 2], nodes[n - 1], cap_dist(rng));

    for (int i = 0; i < m - 2; ++i)
      {
        int u = node_dist(rng);
        int v = node_dist(rng);
        if (u == v) v = (u + 1 < n) ? (u + 1) : 0;
        // Avoid creating additional sources/sinks
        if (v == 0) v = 1;            // don't point to source
        if (u == n - 1) u = n - 2;    // don't leave from sink
        if (u < 0) u = 0;
        if (u == v) continue;
        net.insert_arc(nodes[u], nodes[v], cap_dist(rng));
      }
  }

  // Build identical random networks (since copy doesn't work)
  template <class NetT>
  void build_n_random_networks(std::vector<NetT> & nets,
                               int count, int n, int m,
                               int max_cap, unsigned seed)
  {
    nets.resize(count);
    for (int i = 0; i < count; ++i)
      build_random_network(nets[i], n, m, max_cap, seed);
  }
}

TEST(RandomStressTest, SmallRandomGraphs)
{
  // Run 30 random small graphs, compare all 5 algorithms
  for (unsigned seed = 100; seed < 130; ++seed)
    {
      std::vector<TestNet> nets;
      build_n_random_networks(nets, 5, 8, 15, 100, seed);

      if (not (nets[0].is_single_source() and nets[0].is_single_sink()))
        continue; // skip invalid graphs

      auto f_ek    = edmonds_karp_maximum_flow(nets[0]);
      auto f_dinic = dinic_maximum_flow(nets[1]);
      auto f_cs    = capacity_scaling_maximum_flow(nets[2]);
      auto f_hlpp  = hlpp_maximum_flow(nets[3]);

      EXPECT_DOUBLE_EQ(f_ek, f_dinic)
        << "EK vs Dinic disagree at seed=" << seed;
      EXPECT_DOUBLE_EQ(f_ek, f_cs)
        << "EK vs CapScale disagree at seed=" << seed;
      EXPECT_DOUBLE_EQ(f_ek, f_hlpp)
        << "EK vs HLPP disagree at seed=" << seed;

      EXPECT_TRUE(nets[0].check_network()) << "EK invalid at seed=" << seed;
      EXPECT_TRUE(nets[1].check_network()) << "Dinic invalid at seed=" << seed;
      EXPECT_TRUE(nets[2].check_network()) << "CapScale invalid at seed=" << seed;
      EXPECT_TRUE(nets[3].check_network()) << "HLPP invalid at seed=" << seed;
    }
}

TEST(RandomStressTest, MediumRandomGraphs)
{
  // 10 medium random graphs, 50 nodes, 150 arcs
  for (unsigned seed = 200; seed < 210; ++seed)
    {
      std::vector<TestNet> nets;
      build_n_random_networks(nets, 4, 50, 150, 1000, seed);

      if (not (nets[0].is_single_source() and nets[0].is_single_sink()))
        continue;

      auto f_ek    = edmonds_karp_maximum_flow(nets[0]);
      auto f_dinic = dinic_maximum_flow(nets[1]);
      auto f_hlpp  = hlpp_maximum_flow(nets[2]);

      EXPECT_DOUBLE_EQ(f_ek, f_dinic)
        << "EK vs Dinic disagree at seed=" << seed;
      EXPECT_DOUBLE_EQ(f_ek, f_hlpp)
        << "EK vs HLPP disagree at seed=" << seed;

      verify_flow_conservation<TestNet>(nets[0]);
      verify_flow_conservation<TestNet>(nets[1]);
      verify_flow_conservation<TestNet>(nets[2]);
    }
}


//==============================================================================
// Integer Flow_Type tests
//==============================================================================

TEST(IntegerFlowTest, AllAlgorithmsAgree)
{
  // Diamond network with integer capacities
  IntNet net1, net2, net3, net4;

  auto build_int_diamond = [](IntNet & net) {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto t = net.insert_node(3);
    net.insert_arc(s, a, 10);
    net.insert_arc(s, b, 10);
    net.insert_arc(a, t, 10);
    net.insert_arc(b, t, 10);
  };

  build_int_diamond(net1);
  build_int_diamond(net2);
  build_int_diamond(net3);
  build_int_diamond(net4);

  auto f_ek    = edmonds_karp_maximum_flow(net1);
  auto f_dinic = dinic_maximum_flow(net2);
  auto f_cs    = capacity_scaling_maximum_flow(net3);
  auto f_hlpp  = hlpp_maximum_flow(net4);

  EXPECT_EQ(f_ek, 20);
  EXPECT_EQ(f_dinic, 20);
  EXPECT_EQ(f_cs, 20);
  EXPECT_EQ(f_hlpp, 20);

  EXPECT_TRUE(net1.check_network());
  EXPECT_TRUE(net2.check_network());
  EXPECT_TRUE(net4.check_network());
}

TEST(IntegerFlowTest, ComplexIntegerNetwork)
{
  IntNet net1, net2, net3;

  auto build = [](IntNet & net) {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto c = net.insert_node(3);
    auto d = net.insert_node(4);
    auto t = net.insert_node(5);

    net.insert_arc(s, a, 16);
    net.insert_arc(s, c, 13);
    net.insert_arc(a, b, 12);
    net.insert_arc(a, c, 10);
    net.insert_arc(b, t, 20);
    net.insert_arc(c, d, 14);
    net.insert_arc(d, b, 7);
    net.insert_arc(d, t, 4);
  };

  build(net1);
  build(net2);
  build(net3);

  auto f_dinic = dinic_maximum_flow(net1);
  auto f_cs    = capacity_scaling_maximum_flow(net2);
  auto f_hlpp  = hlpp_maximum_flow(net3);

  EXPECT_EQ(f_dinic, 23);
  EXPECT_EQ(f_cs, 23);
  EXPECT_EQ(f_hlpp, 23);

  EXPECT_TRUE(net1.check_network());
  EXPECT_TRUE(net3.check_network());
}

TEST(IntegerFlowTest, RandomIntegerGraphs)
{
  // Random integer graphs, all algorithms must agree
  for (unsigned seed = 300; seed < 320; ++seed)
    {
      std::vector<IntNet> nets;
      build_n_random_networks(nets, 4, 12, 25, 50, seed);

      if (not (nets[0].is_single_source() and nets[0].is_single_sink()))
        continue;

      auto f_ek    = edmonds_karp_maximum_flow(nets[0]);
      auto f_dinic = dinic_maximum_flow(nets[1]);
      auto f_cs    = capacity_scaling_maximum_flow(nets[2]);
      auto f_hlpp  = hlpp_maximum_flow(nets[3]);

      EXPECT_EQ(f_ek, f_dinic)
        << "EK vs Dinic disagree at seed=" << seed;
      EXPECT_EQ(f_ek, f_cs)
        << "EK vs CapScale disagree at seed=" << seed;
      EXPECT_EQ(f_ek, f_hlpp)
        << "EK vs HLPP disagree at seed=" << seed;

      EXPECT_TRUE(nets[0].check_network()) << "seed=" << seed;
      EXPECT_TRUE(nets[1].check_network()) << "seed=" << seed;
      EXPECT_TRUE(nets[2].check_network()) << "seed=" << seed;
      EXPECT_TRUE(nets[3].check_network()) << "seed=" << seed;
    }
}

TEST(IntegerFlowTest, MinCutVerification)
{
  // Verify max-flow = min-cut for integer networks
  IntNet net1, net2;

  auto build = [](IntNet & net) {
    auto s = net.insert_node(0);
    auto a = net.insert_node(1);
    auto b = net.insert_node(2);
    auto c = net.insert_node(3);
    auto t = net.insert_node(4);

    net.insert_arc(s, a, 3);
    net.insert_arc(s, b, 2);
    net.insert_arc(a, b, 1);
    net.insert_arc(a, c, 3);
    net.insert_arc(b, c, 1);
    net.insert_arc(c, t, 4);
  };

  build(net1);
  build(net2);

  auto flow = dinic_maximum_flow(net1);
  EXPECT_EQ(flow, 4);

  // Verify min-cut via HLPP reachability
  hlpp_maximum_flow(net2);

  // After HLPP, the min cut separates nodes reachable from source
  // in the residual graph from those not reachable.
  // The min-cut capacity should equal the max-flow.
  int cut_cap = 0;
  for (Arc_Iterator<IntNet> it(net2); it.has_curr(); it.next_ne())
    {
      auto arc = it.get_curr();
      // Arc crosses cut if src is source-side and tgt is sink-side
      // Check via residual: source-side nodes can reach source via
      // residual arcs, sink-side nodes cannot
      if (arc->flow == arc->cap) // saturated arcs are candidates
        cut_cap += arc->cap;
    }
  // Total saturated capacity >= max flow (some saturated arcs may not be cut arcs)
  EXPECT_GE(cut_cap, flow);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
