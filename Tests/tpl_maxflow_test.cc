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


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
