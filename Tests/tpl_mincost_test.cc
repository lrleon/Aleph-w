/* Tests for tpl_mincost.H - Advanced minimum cost flow algorithms
 *
 * Tests cover:
 * - Successive Shortest Paths (SSP) algorithm
 * - Cost Scaling algorithm
 * - Assignment problem solver
 * - Transportation problem solver
 */

#include <gtest/gtest.h>
#include <tpl_mincost.H>
#include <tpl_netcost.H>

using namespace Aleph;

namespace
{
  // Network type for tests
  using TestNet = Net_Cost_Graph<>;
  using Node = TestNet::Node;
  using Arc = TestNet::Arc;

  // Helper to compute total outgoing flow from source
  double get_max_flow(const TestNet& net)
  {
    double flow = 0;
    auto source = net.get_source();
    for (Out_Iterator<TestNet> it(source); it.has_curr(); it.next_ne())
      flow += it.get_curr()->flow;
    return flow;
  }

  // Build a simple path network: s -> a -> t
  TestNet build_simple_path_network()
  {
    TestNet net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 10.0, 2.0);  // cap=10, cost=2
    net.insert_arc(a, t, 10.0, 3.0);  // cap=10, cost=3

    return net;
  }

  // Build diamond network with two paths
  TestNet build_diamond_network()
  {
    TestNet net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    // Two paths: s->a->t (expensive) and s->b->t (cheap)
    net.insert_arc(s, a, 5.0, 10.0);  // cap=5, cost=10
    net.insert_arc(s, b, 5.0, 1.0);   // cap=5, cost=1
    net.insert_arc(a, t, 5.0, 10.0);  // cap=5, cost=10
    net.insert_arc(b, t, 5.0, 1.0);   // cap=5, cost=1

    return net;
  }

  // Build network with cross-connection
  TestNet build_cross_network()
  {
    TestNet net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 10.0, 5.0);   // s->a: cap=10, cost=5
    net.insert_arc(s, b, 10.0, 2.0);   // s->b: cap=10, cost=2
    net.insert_arc(a, b, 5.0, 0.0);    // a->b: cap=5, cost=0 (transfer)
    net.insert_arc(a, t, 10.0, 3.0);   // a->t: cap=10, cost=3
    net.insert_arc(b, t, 10.0, 1.0);   // b->t: cap=10, cost=1

    return net;
  }

  // Build network with multiple parallel paths
  TestNet build_parallel_network()
  {
    TestNet net;
    auto s = net.insert_node();
    auto t = net.insert_node();

    // Three parallel arcs with different costs
    net.insert_arc(s, t, 5.0, 1.0);   // cheap, limited
    net.insert_arc(s, t, 3.0, 5.0);   // medium
    net.insert_arc(s, t, 2.0, 10.0);  // expensive

    return net;
  }
}


//==============================================================================
// Successive Shortest Paths (SSP) Tests
//==============================================================================

TEST(SSPTest, SimplePath)
{
  auto net = build_simple_path_network();

  auto [flow, cost] = successive_shortest_paths(net);

  // Max flow = 10, cost = 10*(2+3) = 50
  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 50.0);
  EXPECT_TRUE(net.check_network());
}

TEST(SSPTest, DiamondNetwork)
{
  auto net = build_diamond_network();

  auto [flow, cost] = successive_shortest_paths(net);

  // Max flow = 10 (5 via each path)
  // SSP should prefer cheap path: s->b->t costs 1+1=2 per unit
  // Then use expensive path: s->a->t costs 10+10=20 per unit
  // Cost = 5*2 + 5*20 = 10 + 100 = 110
  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 110.0);
  EXPECT_TRUE(net.check_network());
}

TEST(SSPTest, CrossNetwork)
{
  auto net = build_cross_network();

  auto [flow, cost] = successive_shortest_paths(net);

  // Should find path that uses cheap arcs
  EXPECT_GT(flow, 0.0);
  EXPECT_GT(cost, 0.0);
  EXPECT_TRUE(net.check_network());
}

TEST(SSPTest, ParallelPaths)
{
  auto net = build_parallel_network();

  auto [flow, cost] = successive_shortest_paths(net);

  // Max flow = 5 + 3 + 2 = 10
  // SSP uses cheap arc first (5 units @ cost 1 = 5)
  // Then medium arc (3 units @ cost 5 = 15)
  // Then expensive arc (2 units @ cost 10 = 20)
  // Total cost = 5 + 15 + 20 = 40
  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 40.0);
  EXPECT_TRUE(net.check_network());
}

TEST(SSPTest, LargeCosts)
{
  TestNet net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto t = net.insert_node();

  net.insert_arc(s, a, 5.0, 1000.0);
  net.insert_arc(a, t, 5.0, 2000.0);

  auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_DOUBLE_EQ(flow, 5.0);
  EXPECT_DOUBLE_EQ(cost, 15000.0);  // 5 * (1000 + 2000)
}

TEST(SSPTest, SingleArc)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  net.insert_arc(s, t, 15.0, 3.0);

  auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_DOUBLE_EQ(flow, 15.0);
  EXPECT_DOUBLE_EQ(cost, 45.0);  // 15 * 3
}

TEST(SSPTest, ZeroCapacity)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  net.insert_arc(s, t, 0.0, 5.0);  // Zero capacity

  auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_DOUBLE_EQ(flow, 0.0);
  EXPECT_DOUBLE_EQ(cost, 0.0);
}

TEST(SSPTest, FunctorInterface)
{
  auto net = build_simple_path_network();

  Successive_Shortest_Paths<TestNet> ssp;
  auto [flow, cost] = ssp(net);

  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 50.0);
}

TEST(SSPTest, CompareWithCycleCanceling)
{
  // Build same network twice and compare results
  auto net_ssp = build_diamond_network();
  auto net_cc = build_diamond_network();

  auto [flow_ssp, cost_ssp] = successive_shortest_paths(net_ssp);
  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net_cc);
  (void)cycles;
  (void)factor;
  
  double flow_cc = get_max_flow(net_cc);
  double cost_cc = net_cc.flow_cost();

  // Both should find the same max flow
  EXPECT_DOUBLE_EQ(flow_ssp, flow_cc);

  // Costs should be equal (both optimal)
  EXPECT_NEAR(cost_ssp, cost_cc, 1e-6);
}


//==============================================================================
// Cost Scaling Tests
// 
// NOTE: Cost Scaling algorithm is mentioned in the documentation but not yet
// implemented in tpl_mincost.H. These tests are placeholders for when it gets
// implemented. For now, we test SSP extensively and compare with Cycle Canceling.
//==============================================================================

// TODO: Uncomment these tests when cost_scaling_min_cost_flow is implemented
/*
TEST(CostScalingTest, SimplePath)
{
  auto net = build_simple_path_network();

  auto [flow, cost] = cost_scaling_min_cost_flow(net);

  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 50.0);
  EXPECT_TRUE(net.check_network());
}

TEST(CostScalingTest, DiamondNetwork)
{
  auto net = build_diamond_network();

  auto [flow, cost] = cost_scaling_min_cost_flow(net);

  EXPECT_DOUBLE_EQ(flow, 10.0);
  EXPECT_DOUBLE_EQ(cost, 110.0);
  EXPECT_TRUE(net.check_network());
}
*/


//==============================================================================
// Algorithm Comparison Tests
//==============================================================================

TEST(AlgorithmComparisonTest, SSP_vs_CycleCanceling_Diamond)
{
  auto net1 = build_diamond_network();
  auto net2 = build_diamond_network();

  auto [flow_ssp, cost_ssp] = successive_shortest_paths(net1);
  max_flow_min_cost_by_cycle_canceling(net2);
  
  double flow_cc = get_max_flow(net2);
  double cost_cc = net2.flow_cost();

  // Both should find same max flow
  EXPECT_DOUBLE_EQ(flow_ssp, flow_cc);

  // Both should find same min cost
  EXPECT_NEAR(cost_ssp, cost_cc, 1e-6);
}

TEST(AlgorithmComparisonTest, SSP_vs_CycleCanceling_CrossNetwork)
{
  auto net1 = build_cross_network();
  auto net2 = build_cross_network();

  auto [flow_ssp, cost_ssp] = successive_shortest_paths(net1);
  max_flow_min_cost_by_cycle_canceling(net2);
  
  double flow_cc = get_max_flow(net2);
  double cost_cc = net2.flow_cost();

  EXPECT_DOUBLE_EQ(flow_ssp, flow_cc);
  EXPECT_NEAR(cost_ssp, cost_cc, 1e-6);
}


//==============================================================================
// Performance Comparison Tests
//==============================================================================

TEST(PerformanceTest, SSP_vs_CycleCanceling_OnLargerNetwork)
{
  // Build a grid-like network
  const int SIZE = 5;
  
  auto build_grid = []() {
    TestNet net;
    std::vector<std::vector<Node*>> nodes(SIZE, std::vector<Node*>(SIZE));
    
    for (int i = 0; i < SIZE; ++i)
      for (int j = 0; j < SIZE; ++j)
        nodes[i][j] = net.insert_node();
    
    // Add horizontal arcs
    for (int i = 0; i < SIZE; ++i)
      for (int j = 0; j < SIZE - 1; ++j)
        net.insert_arc(nodes[i][j], nodes[i][j+1], 
                      10.0 + (i+j) % 5,           // capacity
                      1.0 + (i*j) % 3);           // cost
    
    // Add vertical arcs
    for (int i = 0; i < SIZE - 1; ++i)
      for (int j = 0; j < SIZE; ++j)
        net.insert_arc(nodes[i][j], nodes[i+1][j],
                      10.0 + (i+j+1) % 5,
                      1.0 + ((i+1)*j) % 3);
    
    return net;
  };

  auto net_ssp = build_grid();
  auto net_cc = build_grid();

  auto [flow_ssp, cost_ssp] = successive_shortest_paths(net_ssp);
  max_flow_min_cost_by_cycle_canceling(net_cc);
  
  double flow_cc = get_max_flow(net_cc);
  double cost_cc = net_cc.flow_cost();

  // Both algorithms should agree
  EXPECT_DOUBLE_EQ(flow_ssp, flow_cc);
  EXPECT_NEAR(cost_ssp, cost_cc, 1e-6);

  std::cout << "\nGrid " << SIZE << "x" << SIZE << " results:\n";
  std::cout << "  SSP: Flow=" << flow_ssp << ", Cost=" << cost_ssp << "\n";
  std::cout << "  CC:  Flow=" << flow_cc << ", Cost=" << cost_cc << "\n";
}


//==============================================================================
// Assignment Problem Tests
//==============================================================================

TEST(AssignmentTest, Basic3x3)
{
  std::vector<std::vector<double>> costs = {
    {10, 5, 13},
    {3, 9, 18},
    {10, 6, 12}
  };

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 3);
  EXPECT_DOUBLE_EQ(result.total_cost, 20.0);
}

TEST(AssignmentTest, Single1x1)
{
  std::vector<std::vector<double>> costs = {{42}};

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 1);
  EXPECT_DOUBLE_EQ(result.total_cost, 42.0);
}

TEST(AssignmentTest, Empty)
{
  std::vector<std::vector<double>> costs;

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 0);
}

TEST(AssignmentTest, Larger5x5)
{
  std::vector<std::vector<double>> costs = {
    {9, 2, 7, 8, 3},
    {6, 4, 3, 7, 5},
    {5, 8, 1, 8, 6},
    {7, 6, 9, 4, 5},
    {3, 7, 2, 8, 2}
  };

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 5);
  EXPECT_GT(result.total_cost, 0.0);
}


//==============================================================================
// Transportation Problem Tests
//==============================================================================

TEST(TransportationTest, Balanced)
{
  std::vector<double> supplies = {100, 100};
  std::vector<double> demands = {100, 100};
  std::vector<std::vector<double>> costs = {
    {1, 2},
    {3, 4}
  };

  auto result = solve_transportation(supplies, demands, costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.shipments.size(), 2);
}

TEST(TransportationTest, Imbalanced)
{
  std::vector<double> supplies = {100, 100};
  std::vector<double> demands = {50, 50};

  std::vector<std::vector<double>> costs = {
    {1, 2},
    {3, 4}
  };

  auto result = solve_transportation(supplies, demands, costs);

  EXPECT_FALSE(result.feasible);
}

TEST(TransportationTest, Larger3x4)
{
  // 3 suppliers, 4 customers
  std::vector<double> supplies = {50, 60, 40};
  std::vector<double> demands = {30, 40, 50, 30};
  std::vector<std::vector<double>> costs = {
    {2, 3, 1, 4},
    {3, 2, 4, 1},
    {1, 4, 2, 3}
  };

  auto result = solve_transportation(supplies, demands, costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.shipments.size(), 3);
  
  // Verify shipments sum correctly
  double total_shipped = 0;
  for (size_t i = 0; i < result.shipments.size(); ++i)
    for (size_t j = 0; j < result.shipments[i].size(); ++j)
      total_shipped += result.shipments[i][j];
  
  EXPECT_DOUBLE_EQ(total_shipped, 150.0);  // Total supply/demand
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
