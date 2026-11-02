
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
 * @file tpl_multicommodity_test.cc
 * @brief Comprehensive tests for Multi-commodity Flow algorithms
 */

#include <gtest/gtest.h>
#include <tpl_multicommodity.H>
#include <cmath>
#include <vector>

using namespace Aleph;

// Type aliases at file scope
using TestNode = Graph_Anode<Empty_Class>;
using TestArc = MCF_Arc<Empty_Class, double>;
using TestNet = MCF_Graph<TestNode, TestArc>;

//==============================================================================
// Test Fixture
//==============================================================================

class MultiCommodityTest : public ::testing::Test
{
protected:
  static constexpr double EPSILON = 1e-6;

  bool nearly_equal(double a, double b, double eps = EPSILON)
  {
    return std::fabs(a - b) < eps;
  }

  // Build a simple 2-node network
  TestNet build_single_arc_network(double capacity, double cost)
  {
    TestNet net;
    auto s = net.insert_node();
    auto t = net.insert_node();
    net.insert_arc(s, t, capacity, cost);
    return net;
  }

  // Build a diamond network
  TestNet build_diamond_network()
  {
    TestNet net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 10.0, 1.0);
    net.insert_arc(s, b, 10.0, 2.0);
    net.insert_arc(a, t, 10.0, 1.0);
    net.insert_arc(b, t, 10.0, 2.0);

    return net;
  }

  // Build a parallel paths network
  TestNet build_parallel_paths()
  {
    TestNet net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 5.0, 1.0);
    net.insert_arc(a, t, 5.0, 1.0);
    net.insert_arc(s, b, 5.0, 2.0);
    net.insert_arc(b, t, 5.0, 2.0);

    return net;
  }
};

//==============================================================================
// Basic Structure Tests
//==============================================================================

TEST_F(MultiCommodityTest, EmptyNetwork)
{
  TestNet net;
  EXPECT_EQ(net.vsize(), 0);
  EXPECT_EQ(net.esize(), 0);
  EXPECT_EQ(net.num_commodities(), 0);
}

TEST_F(MultiCommodityTest, InsertNodesAndArcs)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 5.0);

  EXPECT_EQ(net.vsize(), 2);
  EXPECT_EQ(net.esize(), 1);
  EXPECT_EQ(arc->capacity, 10.0);
  EXPECT_EQ(arc->base_cost, 5.0);
}

TEST_F(MultiCommodityTest, AddCommodity)
{
  auto net = build_single_arc_network(10.0, 1.0);

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  auto t = it.get_curr();

  size_t k = net.add_commodity(s, t, 5.0, "Commodity 1");

  EXPECT_EQ(k, 0);
  EXPECT_EQ(net.num_commodities(), 1);

  const auto& comm = net.get_commodity(0);
  EXPECT_EQ(comm.source, s);
  EXPECT_EQ(comm.sink, t);
  EXPECT_EQ(comm.demand, 5.0);
  EXPECT_EQ(comm.name, "Commodity 1");
}

TEST_F(MultiCommodityTest, ArcFlowPerCommodity)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 20.0, 1.0);

  net.add_commodity(s, t, 5.0);
  net.add_commodity(s, t, 8.0);

  arc->set_flow(0, 5.0);
  arc->set_flow(1, 8.0);

  EXPECT_EQ(arc->flow(0), 5.0);
  EXPECT_EQ(arc->flow(1), 8.0);
  EXPECT_EQ(arc->total_flow(), 13.0);
  EXPECT_EQ(arc->residual(), 7.0);
}

TEST_F(MultiCommodityTest, MultipleCommodities)
{
  auto net = build_diamond_network();

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  it.next(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 5.0, "K1");
  net.add_commodity(s, t, 3.0, "K2");

  EXPECT_EQ(net.num_commodities(), 2);
  EXPECT_EQ(net.get_commodity(0).demand, 5.0);
  EXPECT_EQ(net.get_commodity(1).demand, 3.0);
}

//==============================================================================
// Solver Tests - Single Commodity
//==============================================================================

TEST_F(MultiCommodityTest, SingleCommoditySingleArc)
{
  auto net = build_single_arc_network(10.0, 2.0);

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 5.0);

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 10.0, 0.1);  // 5 * 2 = 10
}

TEST_F(MultiCommodityTest, SingleCommodityDiamond)
{
  auto net = build_diamond_network();

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  it.next(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 5.0);

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 10.0, 1.0);
}

TEST_F(MultiCommodityTest, SingleCommodityParallelPaths)
{
  auto net = build_parallel_paths();

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  it.next(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 8.0);

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_GT(result.total_cost, 0.0);
}

//==============================================================================
// Solver Tests - Multiple Commodities
//==============================================================================

TEST_F(MultiCommodityTest, TwoCommoditiesCompeting)
{
  TestNet net;
  auto s1 = net.insert_node();
  auto s2 = net.insert_node();
  auto m = net.insert_node();
  auto t1 = net.insert_node();
  auto t2 = net.insert_node();

  net.insert_arc(s1, m, 10.0, 1.0);
  net.insert_arc(s2, m, 10.0, 1.0);
  net.insert_arc(m, t1, 5.0, 1.0);
  net.insert_arc(m, t2, 5.0, 1.0);

  net.add_commodity(s1, t1, 3.0, "K1");
  net.add_commodity(s2, t2, 3.0, "K2");

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_GT(result.total_cost, 0.0);
}

TEST_F(MultiCommodityTest, TwoCommoditiesSharedCapacity)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();

  auto arc = net.insert_arc(s, t, 10.0, 1.0);

  net.add_commodity(s, t, 4.0, "K1");
  net.add_commodity(s, t, 4.0, "K2");

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 8.0, 0.1);

  EXPECT_NEAR(arc->flow(0), 4.0, 0.1);
  EXPECT_NEAR(arc->flow(1), 4.0, 0.1);
  EXPECT_NEAR(arc->total_flow(), 8.0, 0.1);
}

TEST_F(MultiCommodityTest, ThreeCommodities)
{
  auto net = build_diamond_network();

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  it.next(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 2.0, "K1");
  net.add_commodity(s, t, 3.0, "K2");
  net.add_commodity(s, t, 4.0, "K3");

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_GT(result.total_cost, 0.0);
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F(MultiCommodityTest, ZeroDemand)
{
  auto net = build_single_arc_network(10.0, 1.0);

  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  auto t = it.get_curr();

  net.add_commodity(s, t, 0.0);

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 0.0, 0.1);
}

TEST_F(MultiCommodityTest, NoCommodities)
{
  auto net = build_diamond_network();

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 0.0, 0.1);
}

//==============================================================================
// Validation Tests
//==============================================================================

TEST_F(MultiCommodityTest, CapacityRespected)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  net.insert_arc(s, t, 10.0, 1.0);

  net.add_commodity(s, t, 4.0);
  net.add_commodity(s, t, 5.0);

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_TRUE(net.capacities_respected());
}

TEST_F(MultiCommodityTest, DifferentCostsPerCommodity)
{
  TestNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 20.0, 1.0);  // base cost = 1

  // Add commodities FIRST, then set per-commodity costs
  net.add_commodity(s, t, 3.0, "Cheap");       // k=0
  net.add_commodity(s, t, 2.0, "Expensive");   // k=1

  // Now set different costs per commodity
  arc->set_cost(0, 1.0);  // Commodity 0: cost 1
  arc->set_cost(1, 5.0);  // Commodity 1: cost 5

  auto result = solve_multicommodity_flow(net);

  // Expected: 3 * 1 + 2 * 5 = 13
  EXPECT_TRUE(result.is_optimal());
  EXPECT_NEAR(result.total_cost, 13.0, 0.1);
}

//==============================================================================
// Performance Test
//==============================================================================

TEST_F(MultiCommodityTest, MediumNetwork)
{
  const int grid_size = 4;
  TestNet mcf_net;

  std::vector<TestNode*> grid_nodes;
  grid_nodes.reserve(grid_size * grid_size);

  for (int i = 0; i < grid_size * grid_size; ++i)
    grid_nodes.push_back(mcf_net.insert_node());

  auto node_at = [&](int i, int j) { return grid_nodes[i * grid_size + j]; };

  for (int i = 0; i < grid_size; ++i)
    {
      for (int j = 0; j < grid_size; ++j)
        {
          if (j + 1 < grid_size)
            mcf_net.insert_arc(node_at(i, j), node_at(i, j + 1), 5.0, 1.0);
          if (i + 1 < grid_size)
            mcf_net.insert_arc(node_at(i, j), node_at(i + 1, j), 5.0, 1.0);
        }
    }

  mcf_net.add_commodity(node_at(0, 0), node_at(grid_size-1, grid_size-1),
                        2.0, "TopLeft-BottomRight");
  mcf_net.add_commodity(node_at(0, grid_size-1), node_at(grid_size-1, 0),
                        2.0, "TopRight-BottomLeft");

  auto result = solve_multicommodity_flow(mcf_net);

  std::cout << "Grid " << grid_size << "x" << grid_size << ": "
            << "cost=" << result.total_cost
            << ", time=" << result.solve_time_ms << " ms"
            << ", iterations=" << result.iterations << "\n";

  EXPECT_TRUE(result.is_optimal());
  EXPECT_GT(result.total_cost, 0.0);
}

//==============================================================================
// Result Structure Tests
//==============================================================================

TEST_F(MultiCommodityTest, ResultStructure)
{
  TestNet net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto b = net.insert_node();
  auto t = net.insert_node();

  net.insert_arc(s, a, 10.0, 1.0);
  net.insert_arc(s, b, 10.0, 2.0);
  net.insert_arc(a, t, 10.0, 1.0);
  net.insert_arc(b, t, 10.0, 2.0);

  net.add_commodity(s, t, 5.0, "K1");
  net.add_commodity(s, t, 3.0, "K2");

  auto result = solve_multicommodity_flow(net);

  EXPECT_TRUE(result.is_optimal());
  EXPECT_EQ(result.commodity_costs.size(), 2u);
  EXPECT_GE(result.solve_time_ms, 0.0);
  EXPECT_GT(result.iterations, 0u);

  double sum = 0;
  for (auto c : result.commodity_costs)
    sum += c;
  EXPECT_NEAR(sum, result.total_cost, 0.1);
}

//==============================================================================
// demands_satisfied()
//==============================================================================

// MCF_Graph derives from Array_Digraph: a digraph, whose list of a node
// holds only the arcs that leave it
static_assert(DigraphType<TestNet>);

// The flow that comes back into a source counts against its demand
TEST_F(MultiCommodityTest, DemandsSatisfiedCountsTheFlowIntoTheSource)
{
  TestNet net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto t = net.insert_node();
  auto sa = net.insert_arc(s, a, 10.0, 1.0);
  auto at = net.insert_arc(a, t, 10.0, 1.0);
  auto as = net.insert_arc(a, s, 10.0, 1.0); // back into the source
  net.add_commodity(s, t, 2.0);

  // 3 leave s and 1 comes back: 2 go out, the demand
  sa->set_flow(0, 3.0);
  as->set_flow(0, 1.0);
  at->set_flow(0, 2.0);
  EXPECT_TRUE(net.demands_satisfied());

  // Without the flow back, 3 go out
  as->set_flow(0, 0.0);
  EXPECT_FALSE(net.demands_satisfied());

  // 2 leave s and 2 come back: nothing goes out
  sa->set_flow(0, 2.0);
  as->set_flow(0, 2.0);
  EXPECT_FALSE(net.demands_satisfied());

  // A loop at the source leaves it and comes back
  auto ss = net.insert_arc(s, s, 5.0, 1.0);
  sa->set_flow(0, 3.0);
  as->set_flow(0, 1.0);
  ss->set_flow(0, 4.0);
  EXPECT_TRUE(net.demands_satisfied());
}

// Each commodity counts its own flow, at its own source
TEST_F(MultiCommodityTest, DemandsSatisfiedOnEachCommodity)
{
  TestNet net;
  auto s1 = net.insert_node();
  auto s2 = net.insert_node();
  auto t = net.insert_node();
  auto s1s2 = net.insert_arc(s1, s2, 10.0, 1.0);
  auto s2s1 = net.insert_arc(s2, s1, 10.0, 1.0);
  auto s2t = net.insert_arc(s2, t, 10.0, 1.0);
  net.add_commodity(s1, t, 1.0, "from s1");
  net.add_commodity(s2, t, 2.0, "from s2");

  // Commodity 0: 3 from s1 to s2, 2 back, 1 on to t. Commodity 1: 1 from
  // s2 to s1 and back, and 2 from s2 to t
  s1s2->set_flow(0, 3.0);
  s2s1->set_flow(0, 2.0);
  s2t->set_flow(0, 1.0);
  s2s1->set_flow(1, 1.0);
  s1s2->set_flow(1, 1.0);
  s2t->set_flow(1, 2.0);
  EXPECT_TRUE(net.demands_satisfied());

  // The flow of commodity 1 into s2 does not count for commodity 0
  s2s1->set_flow(0, 0.0);
  EXPECT_FALSE(net.demands_satisfied());
}

TEST_F(MultiCommodityTest, DemandsSatisfiedByTheSolver)
{
  auto net = build_diamond_network();
  auto nodes = net.nodes();
  auto it = nodes.get_it();
  auto s = it.get_curr(); it.next();
  it.next(); it.next();
  auto t = it.get_curr();
  net.add_commodity(s, t, 5.0, "K1");
  net.add_commodity(s, t, 3.0, "K2");

  EXPECT_FALSE(net.demands_satisfied()); // no flow yet
  auto result = solve_multicommodity_flow(net);
  ASSERT_TRUE(result.is_optimal());
  EXPECT_TRUE(net.demands_satisfied());
  EXPECT_TRUE(net.capacities_respected());
}

TEST_F(MultiCommodityTest, DemandsSatisfiedWithoutCommodities)
{
  auto net = build_single_arc_network(10.0, 1.0);
  EXPECT_TRUE(net.demands_satisfied());
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
