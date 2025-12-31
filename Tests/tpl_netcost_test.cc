/* Aleph-w

   / \  | | ___ _ __ | |__      __      __
   / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

#include <gtest/gtest.h>
#include <sstream>
#include <cmath>

#include <tpl_netcost.H>

using namespace Aleph;

namespace
{

// Type aliases for convenience
using Net = Net_Cost_Graph<>;
using Node = Net::Node;
using Arc = Net::Arc;

// =============================================================================
// NET_COST_ARC TESTS
// =============================================================================

class NetCostArcTest : public ::testing::Test
{
protected:
  using TestArc = Net_Cost_Arc<int, double>;
};

TEST_F(NetCostArcTest, DefaultConstructor)
{
  TestArc arc;
  EXPECT_EQ(arc.cost, 0.0);
  EXPECT_EQ(arc.flow, 0.0);
  EXPECT_EQ(arc.cap, 0.0);
}

TEST_F(NetCostArcTest, CopyConstructor)
{
  TestArc arc1;
  arc1.cost = 5.5;
  arc1.flow = 2.0;
  arc1.cap = 10.0;

  TestArc arc2(arc1);
  EXPECT_EQ(arc2.cost, 5.5);
  EXPECT_EQ(arc2.flow, 2.0);
  EXPECT_EQ(arc2.cap, 10.0);
}

TEST_F(NetCostArcTest, CopyAssignment)
{
  TestArc arc1;
  arc1.cost = 3.3;
  arc1.flow = 1.0;
  arc1.cap = 5.0;

  TestArc arc2;
  arc2 = arc1;
  EXPECT_EQ(arc2.cost, 3.3);
  EXPECT_EQ(arc2.flow, 1.0);
  EXPECT_EQ(arc2.cap, 5.0);
}

TEST_F(NetCostArcTest, SelfAssignment)
{
  TestArc arc;
  arc.cost = 7.7;
  arc.flow = 3.0;
  arc.cap = 8.0;

  arc = arc;
  EXPECT_EQ(arc.cost, 7.7);
  EXPECT_EQ(arc.flow, 3.0);
  EXPECT_EQ(arc.cap, 8.0);
}

TEST_F(NetCostArcTest, FlowCost)
{
  TestArc arc;
  arc.cost = 2.5;
  arc.flow = 4.0;
  EXPECT_DOUBLE_EQ(arc.flow_cost(), 10.0);  // 2.5 * 4.0 = 10.0
}

TEST_F(NetCostArcTest, FlowCostZeroFlow)
{
  TestArc arc;
  arc.cost = 100.0;
  arc.flow = 0.0;
  EXPECT_DOUBLE_EQ(arc.flow_cost(), 0.0);
}

TEST_F(NetCostArcTest, FlowCostZeroCost)
{
  TestArc arc;
  arc.cost = 0.0;
  arc.flow = 50.0;
  EXPECT_DOUBLE_EQ(arc.flow_cost(), 0.0);
}

TEST_F(NetCostArcTest, FlowCostNegativeCost)
{
  TestArc arc;
  arc.cost = -3.0;
  arc.flow = 5.0;
  EXPECT_DOUBLE_EQ(arc.flow_cost(), -15.0);
}


// =============================================================================
// NET_COST_GRAPH BASIC TESTS
// =============================================================================

class NetCostGraphTest : public ::testing::Test
{
protected:
  Net net;
};

TEST_F(NetCostGraphTest, DefaultConstructor)
{
  EXPECT_EQ(net.vsize(), 0);
  EXPECT_EQ(net.esize(), 0);
}

TEST_F(NetCostGraphTest, InsertNode)
{
  auto n = net.insert_node();
  EXPECT_NE(n, nullptr);
  EXPECT_EQ(net.vsize(), 1);
}

TEST_F(NetCostGraphTest, InsertArcWithCost)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 2.5);

  EXPECT_NE(arc, nullptr);
  EXPECT_EQ(arc->cap, 10.0);
  EXPECT_EQ(arc->cost, 2.5);
  EXPECT_EQ(arc->flow, 0.0);
}

TEST_F(NetCostGraphTest, GetCostModifiable)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 5.0);

  net.get_cost(arc) = 7.5;
  EXPECT_EQ(arc->cost, 7.5);
}

TEST_F(NetCostGraphTest, GetCostConst)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 3.0);

  const Net& const_net = net;
  EXPECT_EQ(const_net.get_cost(arc), 3.0);
}

TEST_F(NetCostGraphTest, ArcFlowCost)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 2.0);
  arc->flow = 5.0;

  EXPECT_DOUBLE_EQ(net.arc_flow_cost(arc), 10.0);  // 5.0 * 2.0
}

TEST_F(NetCostGraphTest, TotalFlowCost)
{
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto t = net.insert_node();

  auto arc1 = net.insert_arc(s, a, 10.0, 2.0);
  auto arc2 = net.insert_arc(a, t, 10.0, 3.0);

  arc1->flow = 5.0;
  arc2->flow = 4.0;

  // Total cost = 5*2 + 4*3 = 10 + 12 = 22
  EXPECT_DOUBLE_EQ(net.flow_cost(), 22.0);
}

TEST_F(NetCostGraphTest, TotalFlowCostEmpty)
{
  EXPECT_DOUBLE_EQ(net.flow_cost(), 0.0);
}


// =============================================================================
// NET_COST_GRAPH COPY TESTS
// =============================================================================

class NetCostGraphCopyTest : public ::testing::Test
{
protected:
  Net net;
  Node *s, *a, *t;
  Arc *arc1, *arc2;

  void SetUp() override
  {
    s = net.insert_node();
    a = net.insert_node();
    t = net.insert_node();

    arc1 = net.insert_arc(s, a, 10.0, 2.0);
    arc2 = net.insert_arc(a, t, 8.0, 3.0);

    arc1->flow = 5.0;
    arc2->flow = 4.0;
  }
};

TEST_F(NetCostGraphCopyTest, CopyConstructor)
{
  Net copy(net);

  EXPECT_EQ(copy.vsize(), net.vsize());
  EXPECT_EQ(copy.esize(), net.esize());
  EXPECT_DOUBLE_EQ(copy.flow_cost(), net.flow_cost());
}

TEST_F(NetCostGraphCopyTest, CopyConstructorPreservesCosts)
{
  Net copy(net);

  // Verify that total costs are preserved
  EXPECT_DOUBLE_EQ(copy.flow_cost(), net.flow_cost());
}

TEST_F(NetCostGraphCopyTest, CopyAssignment)
{
  Net copy;
  copy = net;

  EXPECT_EQ(copy.vsize(), net.vsize());
  EXPECT_EQ(copy.esize(), net.esize());
  EXPECT_DOUBLE_EQ(copy.flow_cost(), net.flow_cost());
}

TEST_F(NetCostGraphCopyTest, SelfCopyAssignment)
{
  auto original_cost = net.flow_cost();
  net = net;
  EXPECT_DOUBLE_EQ(net.flow_cost(), original_cost);
}


// =============================================================================
// RESIDUAL NETWORK TESTS
// =============================================================================

class ResidualNetTest : public ::testing::Test
{
protected:
  using Rnet = Residual_Net<double>;
};

TEST_F(ResidualNetTest, CreateResidualArc)
{
  Rnet rnet;
  auto s = rnet.insert_node();
  auto t = rnet.insert_node();

  auto arc = create_residual_arc(rnet, s, t, 10.0, 3.0, 2.0);

  EXPECT_NE(arc, nullptr);
  EXPECT_EQ(arc->cap, 10.0);
  EXPECT_EQ(arc->flow, 3.0);
  EXPECT_EQ(arc->cost, 2.0);
  EXPECT_FALSE(arc->is_residual);

  // Check mirror arc
  EXPECT_NE(arc->img, nullptr);
  EXPECT_EQ(arc->img->cap, 10.0);
  EXPECT_EQ(arc->img->flow, 7.0);  // cap - flow = 10 - 3
  EXPECT_EQ(arc->img->cost, -2.0);  // Negative cost
  EXPECT_TRUE(arc->img->is_residual);

  // Check bidirectional link
  EXPECT_EQ(arc->img->img, arc);
}

TEST_F(ResidualNetTest, CheckResidualNetConsistency)
{
  Rnet rnet;
  auto s = rnet.insert_node();
  auto t = rnet.insert_node();

  create_residual_arc(rnet, s, t, 10.0, 3.0, 2.0);
  create_residual_arc(rnet, s, t, 5.0, 2.0, 1.0);

  EXPECT_TRUE(check_residual_net(rnet));
}


// =============================================================================
// FILTER TESTS
// =============================================================================

class FilterTest : public ::testing::Test
{
protected:
  Net net;
};

TEST_F(FilterTest, ResFiltPositiveResidual)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 2.0);
  arc->flow = 5.0;  // residual = 5

  Res_Filt<Net> filter;
  EXPECT_TRUE(filter(arc));
}

TEST_F(FilterTest, ResFiltZeroResidual)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 2.0);
  arc->flow = 10.0;  // residual = 0

  Res_Filt<Net> filter;
  EXPECT_FALSE(filter(arc));
}

TEST_F(FilterTest, RcostReturnsArcCost)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 7.5);

  Rcost<Net> rcost;
  EXPECT_DOUBLE_EQ(rcost(arc), 7.5);
}

TEST_F(FilterTest, RcostSetZero)
{
  auto s = net.insert_node();
  auto t = net.insert_node();
  auto arc = net.insert_arc(s, t, 10.0, 5.0);
  arc->flow = 3.0;

  Rcost<Net>::set_zero(arc);

  EXPECT_EQ(arc->cap, std::numeric_limits<double>::max());
  EXPECT_EQ(arc->flow, 0.0);
  EXPECT_EQ(arc->cost, 0.0);
}


// =============================================================================
// OUT_PARS AND IN_PARS TESTS
// =============================================================================

class FlowParsTest : public ::testing::Test
{
protected:
  Net net;
  Node *s, *a, *b, *t;

  void SetUp() override
  {
    s = net.insert_node();
    a = net.insert_node();
    b = net.insert_node();
    t = net.insert_node();
  }
};

TEST_F(FlowParsTest, OutParsMultipleArcs)
{
  auto arc1 = net.insert_arc(s, a, 10.0, 2.0);
  auto arc2 = net.insert_arc(s, b, 5.0, 3.0);
  arc1->flow = 4.0;
  arc2->flow = 2.0;

  auto [cap, flow, cost] = net.out_pars(s);

  EXPECT_DOUBLE_EQ(cap, 15.0);   // 10 + 5
  EXPECT_DOUBLE_EQ(flow, 6.0);   // 4 + 2
  EXPECT_DOUBLE_EQ(cost, 5.0);   // 2 + 3
}

TEST_F(FlowParsTest, InParsMultipleArcs)
{
  auto arc1 = net.insert_arc(a, t, 10.0, 2.0);
  auto arc2 = net.insert_arc(b, t, 5.0, 3.0);
  arc1->flow = 4.0;
  arc2->flow = 2.0;

  auto [cap, flow, cost] = net.in_pars(t);

  EXPECT_DOUBLE_EQ(cap, 15.0);
  EXPECT_DOUBLE_EQ(flow, 6.0);
  EXPECT_DOUBLE_EQ(cost, 5.0);
}

TEST_F(FlowParsTest, OutParsNoArcs)
{
  auto [cap, flow, cost] = net.out_pars(s);

  EXPECT_DOUBLE_EQ(cap, 0.0);
  EXPECT_DOUBLE_EQ(flow, 0.0);
  EXPECT_DOUBLE_EQ(cost, 0.0);
}


// =============================================================================
// EDGE CASE TESTS
// =============================================================================

class EdgeCaseTest : public ::testing::Test
{
};

TEST_F(EdgeCaseTest, EmptyNetwork)
{
  Net net;
  EXPECT_EQ(net.flow_cost(), 0.0);
}

TEST_F(EdgeCaseTest, SingleNodeNetwork)
{
  Net net;
  net.insert_node();

  EXPECT_EQ(net.vsize(), 1);
  EXPECT_EQ(net.esize(), 0);
  EXPECT_EQ(net.flow_cost(), 0.0);
}

TEST_F(EdgeCaseTest, ZeroCostArcs)
{
  Net net;
  auto s = net.insert_node();
  auto t = net.insert_node();

  auto arc = net.insert_arc(s, t, 100.0, 0.0);
  arc->flow = 50.0;

  EXPECT_EQ(net.flow_cost(), 0.0);
}

TEST_F(EdgeCaseTest, VeryLargeCosts)
{
  Net net;
  auto s = net.insert_node();
  auto t = net.insert_node();

  double large_cost = 1e10;
  auto arc = net.insert_arc(s, t, 100.0, large_cost);
  arc->flow = 5.0;

  EXPECT_DOUBLE_EQ(net.flow_cost(), 5.0 * large_cost);
}

TEST_F(EdgeCaseTest, IntegerFlowType)
{
  using IntNet = Net_Cost_Graph<Net_Cost_Node<>, Net_Cost_Arc<Empty_Class, int>>;

  IntNet net;
  auto s = net.insert_node();
  auto t = net.insert_node();

  auto arc = net.insert_arc(s, t, 10, 3);
  arc->flow = 4;

  EXPECT_EQ(net.flow_cost(), 12);  // 4 * 3
}


// =============================================================================
// FEASIBLE SPANNING TREE TESTS (EXPERIMENTAL)
// =============================================================================

class FeasibleTreeTest : public ::testing::Test
{
protected:
  Net net;
};

TEST_F(FeasibleTreeTest, ClassifiesArcsCorrectly)
{
  auto s = net.insert_node();
  auto t = net.insert_node();

  auto empty_arc = net.insert_arc(s, t, 10.0, 1.0);
  empty_arc->flow = 0.0;

  auto full_arc = net.insert_arc(s, t, 5.0, 2.0);
  full_arc->flow = 5.0;

  auto partial_arc = net.insert_arc(s, t, 8.0, 3.0);
  partial_arc->flow = 3.0;

  auto [empty, full, partial] = build_feasible_spanning_tree(net);

  EXPECT_EQ(empty.size(), 1);
  EXPECT_EQ(full.size(), 1);
  EXPECT_EQ(partial.size(), 1);
}

TEST_F(FeasibleTreeTest, GetPartialArcs)
{
  auto s = net.insert_node();
  auto t = net.insert_node();

  auto arc1 = net.insert_arc(s, t, 10.0, 1.0);
  arc1->flow = 0.0;  // empty

  auto arc2 = net.insert_arc(s, t, 10.0, 1.0);
  arc2->flow = 10.0;  // full

  auto arc3 = net.insert_arc(s, t, 10.0, 1.0);
  arc3->flow = 5.0;  // partial

  auto partials = get_partial_arcs(net);

  EXPECT_EQ(partials.size(), 1);
}


// =============================================================================
// MAX FLOW MIN COST INTEGRATION TESTS
// =============================================================================

class MaxFlowMinCostIntegrationTest : public ::testing::Test
{
protected:
  // Classic example network from textbooks:
  //
  //           (cap=4, cost=2)
  //        +--------> a --------+
  //        |                    | (cap=3, cost=1)
  //        |    (cap=2, cost=3) v
  //   s ---+--------> b ------> t
  //        |          ^         ^
  //        |          | (cap=1, cost=1)
  //        |          |         |
  //        +--------> c --------+
  //        (cap=2, cost=1)  (cap=4, cost=2)
  //
  // Optimal max flow = 5, optimal min cost = 14
  //
  // Flow assignment for min cost:
  //   s->a: 3, s->b: 0, s->c: 2
  //   a->t: 3
  //   c->b: 1, c->t: 1
  //   b->t: 1
  //
  // Cost = 3*2 + 2*1 + 3*1 + 1*1 + 1*2 + 1*3 = 6 + 2 + 3 + 1 + 2 + 3 = 17
  // Actually, let's use a simpler example with verified solution.

  // Simpler well-known example:
  //
  //        (cap=3, cost=1)
  //   s --------> a --------> t
  //   |           ^           ^
  //   |  (cap=2,  | (cap=2,   | (cap=2, cost=1)
  //   |  cost=2)  | cost=0)   |
  //   +---------> b ----------+
  //
  // This network has:
  // - s->a: cap=3, cost=1
  // - s->b: cap=2, cost=2
  // - b->a: cap=2, cost=0
  // - a->t: cap=3, cost=3
  // - b->t: cap=2, cost=1
  //
  // Max flow = min(3+2, 3+2) = 4 (limited by sink capacity)
  // For max flow of 4, optimal routing:
  //   s->a: 2, a->t: 2 (cost = 2*1 + 2*3 = 8)
  //   s->b: 2, b->t: 2 (cost = 2*2 + 2*1 = 6)
  //   Total cost = 14
  //
  // Alternative: s->a:3, s->b:1, but a->t limited to 3
  //   s->a: 3, a->t: 3 (cost = 3*1 + 3*3 = 12)
  //   s->b: 1, b->t: 1 (cost = 1*2 + 1*1 = 3)
  //   Total = 15 (worse)

  Net build_textbook_network()
  {
    Net net;

    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    // s->a: cap=3, cost=1
    net.insert_arc(s, a, 3.0, 1.0);
    // s->b: cap=2, cost=2
    net.insert_arc(s, b, 2.0, 2.0);
    // b->a: cap=2, cost=0 (transfer arc)
    net.insert_arc(b, a, 2.0, 0.0);
    // a->t: cap=3, cost=3
    net.insert_arc(a, t, 3.0, 3.0);
    // b->t: cap=2, cost=1
    net.insert_arc(b, t, 2.0, 1.0);

    return net;
  }

  // Very simple network: single path
  //   s ---(cap=5, cost=3)---> t
  //
  // Max flow = 5, cost = 15
  Net build_single_path_network()
  {
    Net net;
    auto s = net.insert_node();
    auto t = net.insert_node();
    net.insert_arc(s, t, 5.0, 3.0);
    return net;
  }

  // Two parallel arcs network (multigraph with direct parallel arcs):
  //
  //   s ===(cap=4, cost=1)===> t  (cheap arc)
  //   s ===(cap=3, cost=2)===> t  (expensive arc)
  //
  // This tests the algorithm's ability to handle multigraphs (parallel arcs
  // between the same pair of nodes). This was a regression that caused infinite
  // loops before the fix in tpl_net.H.
  //
  // Max flow = 7
  // Min cost: both arcs are saturated since both go directly to sink
  // Cost = 4*1 + 3*2 = 10
  Net build_parallel_paths_network()
  {
    Net net;
    auto s = net.insert_node();
    auto t = net.insert_node();
    net.insert_arc(s, t, 4.0, 1.0);  // cheap arc
    net.insert_arc(s, t, 3.0, 2.0);  // expensive arc
    return net;
  }

  // Diamond network with known optimal solution:
  //
  //           (cap=3, cost=1)
  //        +--------> a --------+
  //        |                    | (cap=3, cost=2)
  //   s ---+                    +---> t
  //        |                    |
  //        +--------> b --------+
  //           (cap=3, cost=2)      (cap=3, cost=1)
  //
  // Max flow = 3 (limited by paths)
  // Cheapest: 3 units via s->a->t costs 3*(1+2) = 9
  // vs 3 units via s->b->t costs 3*(2+1) = 9
  // Both paths have same cost!
  Net build_diamond_network()
  {
    Net net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 3.0, 1.0);  // cheap entry
    net.insert_arc(s, b, 3.0, 2.0);  // expensive entry
    net.insert_arc(a, t, 3.0, 2.0);  // expensive exit
    net.insert_arc(b, t, 3.0, 1.0);  // cheap exit

    return net;
  }

  // Helper to compute total outgoing flow from source
  double get_max_flow(const Net& net)
  {
    double flow = 0;
    auto source = net.get_source();
    for (Out_Iterator<Net> it(source); it.has_curr(); it.next_ne())
      flow += it.get_curr()->flow;
    return flow;
  }

  // Helper to verify flow conservation at intermediate nodes
  bool verify_flow_conservation(const Net& net)
  {
    auto source = net.get_source();
    auto sink = net.get_sink();

    for (auto p : net.nodes())
      {
        if (p == source || p == sink)
          continue;

        double in_flow = 0, out_flow = 0;

        for (In_Iterator<Net> it(p); it.has_curr(); it.next_ne())
          in_flow += it.get_curr()->flow;

        for (Out_Iterator<Net> it(p); it.has_curr(); it.next_ne())
          out_flow += it.get_curr()->flow;

        if (std::abs(in_flow - out_flow) > 1e-9)
          return false;
      }
    return true;
  }

  // Helper to verify capacity constraints
  bool verify_capacity_constraints(const Net& net)
  {
    for (auto a : net.arcs())
      {
        if (a->flow < 0 || a->flow > a->cap)
          return false;
      }
    return true;
  }
};

TEST_F(MaxFlowMinCostIntegrationTest, SinglePathNetwork)
{
  auto net = build_single_path_network();

  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 5.0);
  EXPECT_DOUBLE_EQ(total_cost, 15.0);  // 5 * 3
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(MaxFlowMinCostIntegrationTest, ParallelPathsNetwork)
{
  auto net = build_parallel_paths_network();

  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 7.0);
  // Min cost: use cheap path (cost=1) fully, then expensive (cost=2)
  // Optimal: 4 units @ cost 1 + 3 units @ cost 2 = 4 + 6 = 10
  EXPECT_DOUBLE_EQ(total_cost, 10.0);
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(MaxFlowMinCostIntegrationTest, DiamondNetwork)
{
  auto net = build_diamond_network();

  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  // Max flow through diamond is limited by min-cut
  // Two paths: s->a->t and s->b->t, each with capacity 3
  // But can only push 6 total (3 each way)
  EXPECT_EQ(max_flow, 6.0);

  // Both paths cost the same: 1+2 = 2+1 = 3 per unit
  // Total cost = 6 * 3 = 18
  EXPECT_DOUBLE_EQ(total_cost, 18.0);
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(MaxFlowMinCostIntegrationTest, TextbookNetwork)
{
  auto net = build_textbook_network();

  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  // Verify flow conservation and capacity
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));

  // Max flow analysis:
  // Source capacity: s->a (3) + s->b (2) = 5
  // Sink capacity: a->t (3) + b->t (2) = 5
  // So max flow could be up to 5, but need to check paths
  EXPECT_GT(max_flow, 0);
  EXPECT_LE(max_flow, 5.0);

  // Total cost should be non-negative
  EXPECT_GE(total_cost, 0);

  // Print actual results for debugging
  std::cout << "Textbook network: max_flow=" << max_flow
            << ", total_cost=" << total_cost
            << ", cycles_cancelled=" << cycles << std::endl;
}

TEST_F(MaxFlowMinCostIntegrationTest, LargerNetwork)
{
  // Build a larger network with 6 nodes
  //
  //        a ----> c
  //       /|\      |\
  //      / | \     | \
  //     /  |  \    |  \
  //    s   |   \   |   t
  //     \  |    \  |  /
  //      \ |     \ | /
  //       \v      vv
  //        b ----> d
  //
  Net net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto b = net.insert_node();
  auto c = net.insert_node();
  auto d = net.insert_node();
  auto t = net.insert_node();

  // From source
  net.insert_arc(s, a, 4.0, 1.0);
  net.insert_arc(s, b, 4.0, 2.0);

  // Middle layer
  net.insert_arc(a, c, 3.0, 2.0);
  net.insert_arc(a, d, 2.0, 3.0);
  net.insert_arc(b, c, 2.0, 1.0);
  net.insert_arc(b, d, 3.0, 2.0);

  // To sink
  net.insert_arc(c, t, 4.0, 1.0);
  net.insert_arc(d, t, 4.0, 1.0);

  auto [cycles, factor] = max_flow_min_cost_by_cycle_canceling(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
  EXPECT_GT(max_flow, 0);
  EXPECT_GE(total_cost, 0);

  std::cout << "Larger network: max_flow=" << max_flow
            << ", total_cost=" << total_cost
            << ", cycles_cancelled=" << cycles << std::endl;
}

TEST_F(MaxFlowMinCostIntegrationTest, FunctorInterface)
{
  auto net = build_parallel_paths_network();

  Max_Flow_Min_Cost_By_Cycle_Canceling<Net> algo;
  auto [cycles, factor] = algo(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 7.0);
  EXPECT_DOUBLE_EQ(total_cost, 10.0);
}

// =============================================================================
// NETWORK SIMPLEX TESTS
// =============================================================================

class NetworkSimplexTest : public ::testing::Test
{
protected:
  // Very simple network: single path
  //   s ---(cap=5, cost=3)---> t
  Net build_single_path_network()
  {
    Net net;
    auto s = net.insert_node();
    auto t = net.insert_node();
    net.insert_arc(s, t, 5.0, 3.0);
    return net;
  }

  // Two parallel arcs network (multigraph):
  //   s ===(cap=4, cost=1)===> t  (cheap arc)
  //   s ===(cap=3, cost=2)===> t  (expensive arc)
  Net build_parallel_paths_network()
  {
    Net net;
    auto s = net.insert_node();
    auto t = net.insert_node();
    net.insert_arc(s, t, 4.0, 1.0);  // cheap arc
    net.insert_arc(s, t, 3.0, 2.0);  // expensive arc
    return net;
  }

  // Diamond network:
  //           (cap=3, cost=1)
  //        +--------> a --------+
  //        |                    | (cap=3, cost=2)
  //   s ---+                    +---> t
  //        |                    |
  //        +--------> b --------+
  //           (cap=3, cost=2)      (cap=3, cost=1)
  Net build_diamond_network()
  {
    Net net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 3.0, 1.0);
    net.insert_arc(s, b, 3.0, 2.0);
    net.insert_arc(a, t, 3.0, 2.0);
    net.insert_arc(b, t, 3.0, 1.0);

    return net;
  }

  // Textbook network with transfer arc
  Net build_textbook_network()
  {
    Net net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 3.0, 1.0);
    net.insert_arc(s, b, 2.0, 2.0);
    net.insert_arc(b, a, 2.0, 0.0);
    net.insert_arc(a, t, 3.0, 3.0);
    net.insert_arc(b, t, 2.0, 1.0);

    return net;
  }

  // Larger network with 6 nodes
  Net build_larger_network()
  {
    Net net;
    auto s = net.insert_node();
    auto a = net.insert_node();
    auto b = net.insert_node();
    auto c = net.insert_node();
    auto d = net.insert_node();
    auto t = net.insert_node();

    net.insert_arc(s, a, 4.0, 1.0);
    net.insert_arc(s, b, 4.0, 2.0);
    net.insert_arc(a, c, 3.0, 2.0);
    net.insert_arc(a, d, 2.0, 3.0);
    net.insert_arc(b, c, 2.0, 1.0);
    net.insert_arc(b, d, 3.0, 2.0);
    net.insert_arc(c, t, 4.0, 1.0);
    net.insert_arc(d, t, 4.0, 1.0);

    return net;
  }

  double get_max_flow(const Net& net)
  {
    double flow = 0;
    auto source = net.get_source();
    for (Out_Iterator<Net> it(source); it.has_curr(); it.next_ne())
      flow += it.get_curr()->flow;
    return flow;
  }

  bool verify_flow_conservation(const Net& net)
  {
    auto source = net.get_source();
    auto sink = net.get_sink();

    for (auto p : net.nodes())
      {
        if (p == source || p == sink)
          continue;

        double in_flow = 0, out_flow = 0;

        for (In_Iterator<Net> it(p); it.has_curr(); it.next_ne())
          in_flow += it.get_curr()->flow;

        for (Out_Iterator<Net> it(p); it.has_curr(); it.next_ne())
          out_flow += it.get_curr()->flow;

        if (std::abs(in_flow - out_flow) > 1e-9)
          return false;
      }
    return true;
  }

  bool verify_capacity_constraints(const Net& net)
  {
    for (auto a : net.arcs())
      {
        if (a->flow < 0 || a->flow > a->cap + 1e-9)
          return false;
      }
    return true;
  }
};

TEST_F(NetworkSimplexTest, SinglePathNetwork)
{
  auto net = build_single_path_network();

  max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 5.0);
  EXPECT_DOUBLE_EQ(total_cost, 15.0);  // 5 * 3
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(NetworkSimplexTest, ParallelPathsNetwork)
{
  auto net = build_parallel_paths_network();

  max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 7.0);
  EXPECT_DOUBLE_EQ(total_cost, 10.0);  // 4*1 + 3*2
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(NetworkSimplexTest, DiamondNetwork)
{
  auto net = build_diamond_network();

  max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_EQ(max_flow, 6.0);
  EXPECT_DOUBLE_EQ(total_cost, 18.0);  // 6 units * 3 cost per unit
  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
}

TEST_F(NetworkSimplexTest, TextbookNetwork)
{
  auto net = build_textbook_network();

  size_t pivots = max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
  EXPECT_GT(max_flow, 0);
  EXPECT_LE(max_flow, 5.0);
  EXPECT_GE(total_cost, 0);

  std::cout << "Network Simplex textbook: max_flow=" << max_flow
            << ", total_cost=" << total_cost
            << ", pivots=" << pivots << std::endl;
}

TEST_F(NetworkSimplexTest, LargerNetwork)
{
  auto net = build_larger_network();

  size_t pivots = max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_TRUE(verify_flow_conservation(net));
  EXPECT_TRUE(verify_capacity_constraints(net));
  EXPECT_GT(max_flow, 0);
  EXPECT_GE(total_cost, 0);

  std::cout << "Network Simplex larger: max_flow=" << max_flow
            << ", total_cost=" << total_cost
            << ", pivots=" << pivots << std::endl;
}

TEST_F(NetworkSimplexTest, FunctorInterface)
{
  auto net = build_parallel_paths_network();

  Max_Flow_Min_Cost_By_Network_Simplex<Net> algo;
  algo(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 7.0);
  EXPECT_DOUBLE_EQ(total_cost, 10.0);
}

TEST_F(NetworkSimplexTest, CompareWithCycleCanceling)
{
  // Build same network twice and compare results
  auto net_simplex = build_larger_network();
  auto net_cycle = build_larger_network();

  max_flow_min_cost_by_network_simplex(net_simplex);
  max_flow_min_cost_by_cycle_canceling(net_cycle);

  double flow_simplex = get_max_flow(net_simplex);
  double flow_cycle = get_max_flow(net_cycle);
  double cost_simplex = net_simplex.flow_cost();
  double cost_cycle = net_cycle.flow_cost();

  // Both should find the same max flow
  EXPECT_DOUBLE_EQ(flow_simplex, flow_cycle);

  // Costs should be equal (or very close) for optimal solution
  EXPECT_NEAR(cost_simplex, cost_cycle, 1e-6);
}

TEST_F(NetworkSimplexTest, EmptyNetwork)
{
  // A network with nodes but no arcs is not a valid single-source
  // single-sink network, so it should throw an exception
  Net net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  (void)s; (void)t;  // silence unused warnings

  EXPECT_THROW(max_flow_min_cost_by_network_simplex(net), std::domain_error);
}

TEST_F(NetworkSimplexTest, ZeroCostNetwork)
{
  Net net;
  auto s = net.insert_node();
  auto t = net.insert_node();
  net.insert_arc(s, t, 10.0, 0.0);  // zero cost

  max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  EXPECT_DOUBLE_EQ(max_flow, 10.0);
  EXPECT_DOUBLE_EQ(total_cost, 0.0);
}

TEST_F(NetworkSimplexTest, HighCostArcAvoidance)
{
  // Network where one path is much cheaper
  //        (cap=5, cost=1)
  //   s ---------> a ---------> t
  //                    (cap=5, cost=1)
  //        (cap=5, cost=100)
  //   s ---------> b ---------> t
  //                    (cap=5, cost=100)
  Net net;
  auto s = net.insert_node();
  auto a = net.insert_node();
  auto b = net.insert_node();
  auto t = net.insert_node();

  net.insert_arc(s, a, 5.0, 1.0);
  net.insert_arc(a, t, 5.0, 1.0);
  net.insert_arc(s, b, 5.0, 100.0);
  net.insert_arc(b, t, 5.0, 100.0);

  max_flow_min_cost_by_network_simplex(net);

  double max_flow = get_max_flow(net);
  double total_cost = net.flow_cost();

  // Should route through cheaper path (a) first
  EXPECT_DOUBLE_EQ(max_flow, 10.0);
  // Min cost: 5 units through a (cost 5*2=10) + 5 units through b (cost 5*200=1000)
  EXPECT_DOUBLE_EQ(total_cost, 1010.0);
  EXPECT_TRUE(verify_flow_conservation(net));
}

// =============================================================================
// SIMPLEX DATA STRUCTURE TESTS
// =============================================================================

class SimplexDataStructuresTest : public ::testing::Test
{
};

TEST_F(SimplexDataStructuresTest, SimplexNodeInfoDefaults)
{
  Simplex_Node_Info<double> info;
  EXPECT_EQ(info.potential, 0.0);
  EXPECT_EQ(info.parent, nullptr);
  EXPECT_EQ(info.parent_arc, nullptr);
  EXPECT_EQ(info.depth, 0);
  EXPECT_EQ(info.mark, 0);
  EXPECT_TRUE(info.arc_from_parent);  // Default is true
}

TEST_F(SimplexDataStructuresTest, SimplexArcStateEnum)
{
  EXPECT_NE(static_cast<int>(Simplex_Arc_State::Lower),
            static_cast<int>(Simplex_Arc_State::Upper));
  EXPECT_NE(static_cast<int>(Simplex_Arc_State::Lower),
            static_cast<int>(Simplex_Arc_State::Tree));
  EXPECT_NE(static_cast<int>(Simplex_Arc_State::Upper),
            static_cast<int>(Simplex_Arc_State::Tree));
}

} // anonymous namespace

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}