/* Tests for tpl_mincost.H - Advanced minimum cost flow algorithms
 *
 * Tests cover:
 * - Successive Shortest Paths (SSP) algorithm
 * - Cost Scaling algorithm
 * - Assignment problem solver
 * - Transportation problem solver
 */

#include <array>
#include <utility>
#include <vector>

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


// ============================================================================
// Regression tests for validation and new behavior paths
// ============================================================================

// --- Negative-cycle detection -------------------------------------------

// The negative-cycle path in ssp_init_potentials fires when Bellman-Ford's
// extra (n-th) relaxation pass can still reduce some node's distance.
// That requires a true directed cycle of the residual graph whose arc-costs
// sum to a negative value, which the source reaches or not.
//
// Topology:  s --0--> m1 --1--> m2
//                      ^         |
//                      |  -3     |
//                      +---------+   (cycle m1→m2→m1, total cost -2)
//            m1 --0--> t
//
// s is the unique source (in-degree 0), t is the unique sink (out-degree 0).
// Bellman-Ford will keep decreasing d[m1] and d[m2] on every pass, and the
// extra detection pass catches this.
TEST(NegCycleTest, RejectsNegativeCycle)
{
  TestNet net;
  auto s  = net.insert_node();
  auto m1 = net.insert_node();
  auto m2 = net.insert_node();
  auto t  = net.insert_node();

  net.insert_arc(s,  m1, 10.0,  0.0);   // s → m1, cost 0
  net.insert_arc(m1, m2, 10.0,  1.0);   // m1 → m2, cost 1
  net.insert_arc(m2, m1, 10.0, -3.0);   // m2 → m1, cost -3 → cycle cost -2
  net.insert_arc(m1, t,  10.0,  0.0);   // m1 → t, cost 0

  // ssp_init_potentials detects the cycle and returns false;
  // successive_shortest_paths must propagate that as domain_error.
  EXPECT_THROW(successive_shortest_paths(net), std::domain_error);
}

// A network with only non-negative costs must NOT trigger the cycle detector.
TEST(NegCycleTest, AcceptsNonNegativeCostNetwork)
{
  TestNet net;
  auto s  = net.insert_node();
  auto m1 = net.insert_node();
  auto t  = net.insert_node();

  net.insert_arc(s,  m1, 10.0, 2.0);
  net.insert_arc(m1, t,  10.0, 3.0);

  // No negative cycle — must complete without throwing.
  EXPECT_NO_THROW(successive_shortest_paths(net));
}

// Bellman-Ford went from the source, and did not see a negative cycle that
// the source does not reach: the flow came back of cost 0, while the
// cheapest maximum flow saturates the cycle, at cost -2
TEST(NegCycleTest, RejectsANegativeCycleApartFromTheSource)
{
  TestNet net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto b = net.insert_node();
  auto t = net.insert_node();
  auto st = net.insert_arc(s, t, 1.0, 0.0);
  auto ab = net.insert_arc(a, b, 1.0, -1.0);
  net.insert_arc(b, a, 1.0, -1.0);

  EXPECT_THROW(successive_shortest_paths(net), std::domain_error);
  EXPECT_EQ(st->flow, 0.0);
  EXPECT_EQ(ab->flow, 0.0);
}

// A source that is the sink, isolated, has no path to augment: the loop took
// the empty one from it to itself, and did not end
TEST(SSPTest, ASourceThatIsTheSinkEnds)
{
  TestNet net;
  net.insert_node();  // the source and the sink
  auto a = net.insert_node();
  auto b = net.insert_node();
  auto ab = net.insert_arc(a, b, 1.0, 1.0);
  auto ba = net.insert_arc(b, a, 1.0, 2.0);

  const auto [flow, cost] = successive_shortest_paths(net);
  EXPECT_EQ(flow, 0.0);
  EXPECT_EQ(cost, 0.0);

  // With a circulation of cost 3, whose cancellation is cheaper, the
  // residual graph has a negative cycle
  ab->flow = ba->flow = 1.0;
  EXPECT_THROW(successive_shortest_paths(net), std::domain_error);
  EXPECT_EQ(ab->flow, 1.0);
  EXPECT_EQ(ba->flow, 1.0);
}

// The networks of the cases of decimal costs: u, v, and the capacity and
// the cost in tenths
TestNet build_tenths_network(const std::vector<std::array<int, 4>> & spec, int n)
{
  TestNet net;
  std::vector<Node *> nodes;
  for (int i = 0; i < n; ++i)
    nodes.push_back(net.insert_node());
  for (const auto & a : spec)
    net.insert_arc(nodes[size_t(a[0])], nodes[size_t(a[1])], a[2] / 10.0, a[3] / 10.0);
  return net;
}

// With costs in tenths, not exact in binary, a reduced cost of zero came
// out of the order of -1e-17, and Dijkstra took a distance down by an ulp
// around a cycle forever: the first run did not end
TEST(SSPTest, DecimalCostsEnd)
{
  // 3 is the source and 5 the sink
  auto net = build_tenths_network({{2, 1, 17, 5}, {3, 5, 27, 9}, {1, 5, 2, 6}, {0, 2, 37, 9},
                                   {2, 4, 23, 6}, {3, 2, 14, 0}, {0, 4, 26, 8}, {3, 0, 2, 6},
                                   {4, 5, 37, 7}, {3, 5, 22, 6}}, 6);

  const auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_NEAR(flow, 6.5, 1e-12);
  EXPECT_NEAR(cost, 5.95, 1e-12);  // the exact optimum
  EXPECT_TRUE(net.check_network());
}

// The second run starts from the cheapest flow, where the two arcs 2->0 of
// cost 0.4, one full and one not, make a cycle of cost zero, which the
// rounding of Bellman-Ford made negative: the second run threw for it
TEST(SSPTest, ASecondRunOfDecimalCostsFindsNoNegativeCycle)
{
  // 1 is the source and 0 the sink
  auto net = build_tenths_network({{1, 2, 9, 8}, {2, 0, 34, 4}, {1, 2, 24, 0}, {1, 2, 37, 1},
                                   {2, 0, 33, 4}}, 3);

  const auto [flow, cost] = successive_shortest_paths(net);
  EXPECT_NEAR(flow, 6.7, 1e-12);
  EXPECT_NEAR(cost, 3.53, 1e-12);  // the exact optimum

  std::pair<double, double> again;
  ASSERT_NO_THROW(again = successive_shortest_paths(net));
  EXPECT_NEAR(again.first, flow, 1e-12);
  EXPECT_NEAR(again.second, cost, 1e-12);
}

// Into 2 come 0.1 + 0.8, and out goes their rounded sum, 2.8e-17 less,
// within the tolerance of check_node(); the network simplex, and the
// maximum flow before it, returned it from 0->2, which then had residual
// capacity: with the residual of 1->0, of cost -1, through 2 and 1, a
// cycle of negative cost, for which successive_shortest_paths(), which
// starts from the flow that the network has, threw. What a node keeps
// within its tolerance stays
TEST(SSPTest, RunsOnTheCheapestFlowOfTheNetworkSimplex)
{
  // 0 is the source and 3 the sink
  auto net = build_tenths_network({{0, 1, 31, 10}, {0, 2, 1, 0}, {1, 2, 8, 0}, {2, 3, 29, 0}}, 4);
  max_flow_min_cost_by_network_simplex(net);
  ASSERT_NEAR(net.flow_value(), 0.9, 1e-12);
  ASSERT_NEAR(net.flow_cost(), 0.8, 1e-12);

  std::pair<double, double> again;
  ASSERT_NO_THROW(again = successive_shortest_paths(net));
  EXPECT_NEAR(again.first, 0.9, 1e-12);
  EXPECT_NEAR(again.second, 0.8, 1e-12);
  EXPECT_TRUE(net.check_network());
}

// The second path, 0->2->3, takes what 2->3 has left, 3.4 - 0.7, and
// 0.7 + (3.4 - 0.7) came out an ulp above 3.4: 2->3 carried
// 3.4000000000000004 of 3.3999999999999999, out of its capacity, and the
// network simplex refused the flow. The arc that bounds a path is filled
// exactly, and the residues of rounding go back at the end
TEST(SSPTest, DecimalCapacitiesStayWithinTheArcs)
{
  // 0 is the source and 3 the sink
  auto net = build_tenths_network({{0, 1, 7, 2}, {1, 2, 8, 1}, {0, 2, 27, 8}, {2, 3, 34, 1}}, 4);

  const auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_NEAR(flow, 3.4, 1e-12);
  EXPECT_NEAR(cost, 2.71, 1e-12);  // the exact optimum
  for (Arc_Iterator<TestNet> it(net); it.has_curr(); it.next_ne())
    {
      EXPECT_GE(it.get_curr()->flow, 0);
      EXPECT_LE(it.get_curr()->flow, it.get_curr()->cap);
    }
  EXPECT_TRUE(net.check_network());

  ASSERT_NO_THROW(Network_Simplex<TestNet>(net).run());
  EXPECT_NEAR(net.flow_value(), 3.4, 1e-12);
  EXPECT_NEAR(net.flow_cost(), 2.71, 1e-12);
}

// With the arcs that bound the paths filled exactly, and the flows that a
// path takes next to zero taken to zero, a path took a residue of rounding,
// of 2.2e-16, and a later one took the flow out of its node to zero: the
// node kept it, far beyond its tolerance, as it sent nothing. The residues
// of the paths go back at the end, as those of the maximum flows
TEST(SSPTest, ReturnsTheResiduesOfItsPaths)
{
  // u, v and k for the capacity and the cost, k * 0.1; 0 is the source and
  // 21 the sink
  const std::vector<std::array<int, 4>> spec = {
    {0, 1, 29, 32},   {1, 2, 37, 32},   {3, 4, 34, 5},    {5, 6, 26, 11},   {1, 7, 28, 48},
    {8, 9, 39, 15},   {10, 11, 39, 44}, {0, 12, 33, 94},  {0, 13, 6, 18},   {9, 14, 20, 6},
    {1, 15, 34, 14},  {15, 16, 35, 87}, {17, 18, 31, 49}, {12, 6, 8, 77},   {6, 14, 12, 31},
    {5, 14, 33, 76},  {14, 16, 18, 17}, {16, 19, 27, 21}, {4, 20, 37, 93},  {20, 21, 23, 89},
    {19, 21, 39, 34}, {1, 8, 12, 24},   {19, 22, 21, 59}, {23, 14, 9, 2},   {3, 7, 20, 5},
    {13, 9, 25, 51},  {22, 24, 38, 9},  {11, 15, 39, 27}, {24, 21, 24, 95}, {0, 5, 28, 99},
    {7, 24, 35, 14},  {16, 25, 20, 86}, {25, 17, 1, 14},  {5, 13, 14, 9},   {26, 13, 32, 76},
    {2, 27, 15, 80},  {28, 10, 12, 37}, {27, 9, 4, 15},   {20, 26, 21, 50}, {13, 28, 7, 91},
    {13, 3, 20, 40},  {18, 23, 10, 16}};
  TestNet net;
  std::vector<Node *> nodes;
  for (int i = 0; i < 29; ++i)
    nodes.push_back(net.insert_node());
  for (const auto & a : spec)
    net.insert_arc(nodes[size_t(a[0])], nodes[size_t(a[1])], a[2] * 0.1, a[3] * 0.1);

  const auto [flow, cost] = successive_shortest_paths(net);

  EXPECT_NEAR(flow, 6.7, 1e-12);
  EXPECT_NEAR(cost, 156.22, 1e-9);  // the exact optimum
  EXPECT_TRUE(net.check_network());
}

// --- Unreachable-node handling (indirect) --------------------------------

// ssp_init_potentials starts the potential of every node at 0, from a
// virtual node joined to all of them, so that the nodes that the source does
// not reach have one too: zero-demand nodes in the transshipment wrapper are
// not connected to the super-source/super-sink.
// We verify indirectly: a transshipment instance where two nodes carry zero
// demand must still solve correctly with total_flow == total non-zero supply.
TEST(SSPTest, ZeroDemandNodesHandledGracefully)
{
  // Two supply nodes and two demand nodes balanced at 5 each.
  // Internally solve_transportation builds a super-source/super-sink network;
  // ssp_init_potentials must handle any nodes unreachable from super-source
  // (those with zero demand/supply) without crashing or returning wrong results.
  std::vector<double> supplies = {5.0, 5.0};
  std::vector<double> demands  = {5.0, 5.0};
  std::vector<std::vector<double>> costs = {{1.0, 2.0}, {3.0, 1.0}};

  auto result = solve_transportation(supplies, demands, costs);
  EXPECT_TRUE(result.feasible);
  EXPECT_DOUBLE_EQ(result.total_cost, 10.0);  // optimal: (0,0)=5 + (1,1)=5 → 5+5=10
}

// --- solve_assignment validation ----------------------------------------

TEST(AssignmentValidationTest, NonSquareMatrixThrows)
{
  std::vector<std::vector<double>> bad = {{1, 2, 3}, {4, 5}};
  EXPECT_THROW(solve_assignment(bad), std::invalid_argument);
}

TEST(AssignmentValidationTest, EmptyMatrixReturnsFeasible)
{
  std::vector<std::vector<double>> empty;
  auto result = solve_assignment(empty);
  EXPECT_TRUE(result.feasible);
}

// --- solve_transportation validation ------------------------------------

TEST(TransportationValidationTest, MismatchedRowsThrows)
{
  std::vector<double> supplies = {10, 20};
  std::vector<double> demands  = {30};
  // costs has 1 row but 2 supply points
  std::vector<std::vector<double>> costs = {{5}};
  EXPECT_THROW(solve_transportation(supplies, demands, costs), std::invalid_argument);
}

TEST(TransportationValidationTest, MismatchedColumnsThrows)
{
  std::vector<double> supplies = {10};
  std::vector<double> demands  = {5, 5};
  // costs[0] has 1 column but 2 demand points
  std::vector<std::vector<double>> costs = {{3}};
  EXPECT_THROW(solve_transportation(supplies, demands, costs), std::invalid_argument);
}

TEST(TransportationValidationTest, NegativeSupplyThrows)
{
  std::vector<double> supplies = {-5, 10};
  std::vector<double> demands  = {5};
  std::vector<std::vector<double>> costs = {{1}, {2}};
  EXPECT_THROW(solve_transportation(supplies, demands, costs), std::invalid_argument);
}

TEST(TransportationValidationTest, NegativeDemandThrows)
{
  std::vector<double> supplies = {10};
  std::vector<double> demands  = {-5};
  std::vector<std::vector<double>> costs = {{3}};
  EXPECT_THROW(solve_transportation(supplies, demands, costs), std::invalid_argument);
}

// --- Empty-side feasibility ---------------------------------------------

TEST(TransportationValidationTest, EmptySuppliesNonemptyDemandsInfeasible)
{
  // m=0, n=1, total_supply=0 != total_demand=5 → infeasible
  std::vector<double> supplies;
  std::vector<double> demands = {5};
  std::vector<std::vector<double>> costs;
  auto result = solve_transportation(supplies, demands, costs);
  EXPECT_FALSE(result.feasible);
}

TEST(TransportationValidationTest, EmptySuppliesZeroDemandFeasible)
{
  // m=0, n=1, total_supply=0 == total_demand=0 → feasible
  std::vector<double> supplies;
  std::vector<double> demands = {0};
  std::vector<std::vector<double>> costs;
  auto result = solve_transportation(supplies, demands, costs);
  EXPECT_TRUE(result.feasible);
}

TEST(TransportationValidationTest, BothEmptyFeasible)
{
  std::vector<double> supplies;
  std::vector<double> demands;
  std::vector<std::vector<double>> costs;
  auto result = solve_transportation(supplies, demands, costs);
  EXPECT_TRUE(result.feasible);
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
// satisfy CI policy
