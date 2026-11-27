
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
 * @file net_sup_dem_test.cc
 * @brief Comprehensive tests for Net_Sup_Dem_Graph class
 */

#include <gtest/gtest.h>
#include <tpl_net_sup_dem.H>
#include <tpl_maxflow.H>
#include <tpl_dynDlist.H>
#include <limits>
#include <random>
#include <string>

using namespace Aleph;

// =============================================================================
// Type Aliases for Testing
// =============================================================================

// Simple network with int flow (use Empty_Class as node info to avoid ambiguity)
using SimpleNode = Net_Sup_Dem_Node<Empty_Class, int>;
using SimpleArc = Net_Arc<Empty_Class, int>;
using SimpleNet = Net_Sup_Dem_Graph<SimpleNode, SimpleArc>;

// Network with string info and double flow
using StringNode = Net_Sup_Dem_Node<std::string, double>;
using StringArc = Net_Arc<std::string, double>;
using StringNet = Net_Sup_Dem_Graph<StringNode, StringArc>;

// =============================================================================
// Net_Sup_Dem_Node Tests
// =============================================================================

class NetSupDemNodeTest : public ::testing::Test
{
protected:
  using Node = Net_Sup_Dem_Node<std::string, int>;
};

TEST_F(NetSupDemNodeTest, DefaultConstructor)
{
  Node n;
  EXPECT_EQ(n.supply_flow, 0);
  EXPECT_EQ(n.get_supply_flow(), 0);
}

TEST_F(NetSupDemNodeTest, InfoConstructor)
{
  Node n("TestNode");
  EXPECT_EQ(n.supply_flow, 0);
}

TEST_F(NetSupDemNodeTest, CopyFromPointer)
{
  Node original("Original");
  original.supply_flow = 42;
  
  Node copy(&original);
  EXPECT_EQ(copy.supply_flow, 42);
}

TEST_F(NetSupDemNodeTest, GetSupplyFlowModifiable)
{
  Node n;
  n.get_supply_flow() = 100;
  EXPECT_EQ(n.supply_flow, 100);
}

// =============================================================================
// Net_Sup_Dem_Graph Basic Tests
// =============================================================================

class NetSupDemGraphTest : public ::testing::Test
{
protected:
  SimpleNet net;
};

TEST_F(NetSupDemGraphTest, DefaultConstructor)
{
  EXPECT_FALSE(net.exist_aux_net());
  EXPECT_EQ(net.get_super_source(), nullptr);
  EXPECT_EQ(net.get_super_sink(), nullptr);
}

TEST_F(NetSupDemGraphTest, InsertNodeWithSupply)
{
  auto* p = net.insert_node(100);  // supply node
  EXPECT_EQ(p->supply_flow, 100);
  EXPECT_EQ(net.get_supply_flow(p), 100);
}

TEST_F(NetSupDemGraphTest, InsertNodeWithDemand)
{
  auto* p = net.insert_node(-50);  // demand node
  EXPECT_EQ(p->supply_flow, -50);
  EXPECT_EQ(net.get_supply_flow(p), -50);
}

TEST_F(NetSupDemGraphTest, InsertTransitNode)
{
  auto* p = net.insert_node(0);  // transit node
  EXPECT_EQ(p->supply_flow, 0);
}

// ==============================================================================
// Supply/Demand Query Tests
// =============================================================================

TEST_F(NetSupDemGraphTest, CountSupplyNodes)
{
  net.insert_node(100);   // supply
  net.insert_node(50);    // supply
  net.insert_node(0);     // transit
  net.insert_node(-30);   // demand
  
  EXPECT_EQ(net.count_supply_nodes(), 2u);
}

TEST_F(NetSupDemGraphTest, CountDemandNodes)
{
  net.insert_node(100);   // supply
  net.insert_node(-50);   // demand
  net.insert_node(-30);   // demand
  net.insert_node(-20);   // demand
  
  EXPECT_EQ(net.count_demand_nodes(), 3u);
}

TEST_F(NetSupDemGraphTest, TotalSupply)
{
  net.insert_node(100);
  net.insert_node(50);
  net.insert_node(-30);  // demand, not counted
  
  EXPECT_EQ(net.total_supply(), 150);
}

TEST_F(NetSupDemGraphTest, TotalDemand)
{
  net.insert_node(100);  // supply, not counted
  net.insert_node(-50);
  net.insert_node(-30);
  
  EXPECT_EQ(net.total_demand(), 80);
}

TEST_F(NetSupDemGraphTest, IsBalanced)
{
  net.insert_node(100);
  net.insert_node(-50);
  net.insert_node(-50);
  
  EXPECT_TRUE(net.is_balanced());
}

TEST_F(NetSupDemGraphTest, IsNotBalanced)
{
  net.insert_node(100);
  net.insert_node(-30);
  
  EXPECT_FALSE(net.is_balanced());
}

// =============================================================================
// Set Supply Flow Tests
// =============================================================================

TEST_F(NetSupDemGraphTest, SetSupplyFlow)
{
  auto* p = net.insert_node(0);
  p->out_cap = 200;  // Set output capacity
  
  net.set_supply_flow(p, 150);
  EXPECT_EQ(p->supply_flow, 150);
}

TEST_F(NetSupDemGraphTest, SetDemandFlow)
{
  auto* p = net.insert_node(0);
  p->in_cap = 100;  // Set input capacity
  
  net.set_supply_flow(p, -80);
  EXPECT_EQ(p->supply_flow, -80);
}

TEST_F(NetSupDemGraphTest, SetSupplyFlowExceedsCapacityThrows)
{
  auto* p = net.insert_node(0);
  p->out_cap = 50;
  
  EXPECT_THROW(net.set_supply_flow(p, 100), std::range_error);
}

TEST_F(NetSupDemGraphTest, SetDemandFlowExceedsCapacityThrows)
{
  auto* p = net.insert_node(0);
  p->in_cap = 30;
  
  EXPECT_THROW(net.set_supply_flow(p, -50), std::range_error);
}

// =============================================================================
// Auxiliary Network Tests
// =============================================================================

class AuxNetTest : public ::testing::Test
{
protected:
  SimpleNet net;
  
  void SetUp() override
  {
    // Create a simple supply-demand network
    // Supply: 100 units
    // Demand: 80 units (feasible)
    auto* s1 = net.insert_node(50);   // supply 50
    auto* s2 = net.insert_node(50);   // supply 50
    auto* t1 = net.insert_node(0);    // transit
    auto* d1 = net.insert_node(-40);  // demand 40
    auto* d2 = net.insert_node(-40);  // demand 40
    
    // Set capacities for supply nodes
    s1->out_cap = 100;
    s2->out_cap = 100;
    
    // Set capacities for demand nodes
    d1->in_cap = 100;
    d2->in_cap = 100;
    
    // Create arcs with sufficient capacity
    net.insert_arc(s1, t1, 100);
    net.insert_arc(s2, t1, 100);
    net.insert_arc(t1, d1, 100);
    net.insert_arc(t1, d2, 100);
  }
};

TEST_F(AuxNetTest, ComputeAuxNet)
{
  EXPECT_FALSE(net.exist_aux_net());
  
  auto* result = net.compute_aux_net();
  
  EXPECT_EQ(result, &net);
  EXPECT_TRUE(net.exist_aux_net());
  EXPECT_NE(net.get_super_source(), nullptr);
  EXPECT_NE(net.get_super_sink(), nullptr);
}

TEST_F(AuxNetTest, ComputeAuxNetTwiceThrows)
{
  net.compute_aux_net();
  
  EXPECT_THROW(net.compute_aux_net(), std::domain_error);
}

TEST_F(AuxNetTest, GetAuxNetBeforeCompute)
{
  EXPECT_EQ(net.get_aux_net(), nullptr);
}

TEST_F(AuxNetTest, GetAuxNetAfterCompute)
{
  net.compute_aux_net();
  EXPECT_EQ(net.get_aux_net(), &net);
}

TEST_F(AuxNetTest, FreeAuxNet)
{
  net.compute_aux_net();
  EXPECT_TRUE(net.exist_aux_net());
  
  net.free_aux_net();
  
  EXPECT_FALSE(net.exist_aux_net());
  EXPECT_EQ(net.get_super_source(), nullptr);
  EXPECT_EQ(net.get_super_sink(), nullptr);
}

TEST_F(AuxNetTest, FreeAuxNetWithoutComputeThrows)
{
  EXPECT_THROW(net.free_aux_net(), std::domain_error);
}

TEST_F(AuxNetTest, IsFeasibleWithoutAuxNetThrows)
{
  EXPECT_THROW((void)net.is_feasible(), std::domain_error);
}

// =============================================================================
// Feasibility Tests
// =============================================================================

class FeasibilityTest : public ::testing::Test
{
protected:
  SimpleNet net;
};

TEST_F(FeasibilityTest, EmptyNetworkIsFeasible)
{
  // Network with no supply or demand (all transit nodes)
  // Note: When there are no supply/demand nodes, compute_aux_net
  // removes both super nodes, so exist_aux_net() returns false.
  // This is correct behavior - there's nothing to check for feasibility.
  auto* n1 = net.insert_node(0);
  auto* n2 = net.insert_node(0);
  net.insert_arc(n1, n2, 100);
  
  // A network with no supply/demand is trivially balanced and feasible
  EXPECT_TRUE(net.is_balanced());
  EXPECT_EQ(net.total_supply(), 0);
  EXPECT_EQ(net.total_demand(), 0);
}

// is_feasible() judges the flow of the auxiliary network, which a max-flow
// algorithm computes; it judged the fields out_flow and in_flow of the
// nodes, which the network does not write and these tests set by hand

// A supply has nowhere to go without a demand
TEST_F(FeasibilityTest, SupplyOnlyNetworkIsNotFeasible)
{
  auto* s = net.insert_node(100);
  s->out_cap = 200;

  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible());
}

TEST_F(FeasibilityTest, SupplyNotMeetingFlowRequirement)
{
  auto* s = net.insert_node(100);
  auto* d = net.insert_node(-100);
  s->out_cap = 200;
  net.insert_arc(s, d, 50); // only 50 can go

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);
  EXPECT_FALSE(net.is_feasible());
}

// A demand has nothing to receive without a supply
TEST_F(FeasibilityTest, DemandOnlyNetworkIsNotFeasible)
{
  auto* d = net.insert_node(-80);
  d->in_cap = 100;

  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible());
}

TEST_F(FeasibilityTest, DemandNotMet)
{
  auto* s = net.insert_node(80);
  auto* d = net.insert_node(-80);
  d->in_cap = 100;
  net.insert_arc(s, d, 50); // only 50 can come

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);
  EXPECT_FALSE(net.is_feasible());
}

TEST_F(FeasibilityTest, MixedSupplyDemandFeasible)
{
  auto* s = net.insert_node(100);
  auto* d = net.insert_node(-100);

  s->out_cap = 150;
  d->in_cap = 150;

  net.insert_arc(s, d, 100);

  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible()); // no flow yet
  (void) edmonds_karp_maximum_flow(net);
  EXPECT_TRUE(net.is_feasible());
}

TEST_F(FeasibilityTest, MixedSupplyDemandNotFeasible)
{
  auto* s = net.insert_node(100);
  auto* d = net.insert_node(-100);

  s->out_cap = 150;
  d->in_cap = 150;

  net.insert_arc(s, d, 80); // only 80 of the 100 can go

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);
  EXPECT_FALSE(net.is_feasible());
}

// =============================================================================
// Non-Feasible Nodes Tests
// =============================================================================

TEST_F(FeasibilityTest, NonFeasibleSupplyNodes)
{
  auto* s1 = net.insert_node(100);
  auto* s2 = net.insert_node(50);
  auto* d = net.insert_node(-110);

  s1->out_cap = 200;
  s2->out_cap = 100;
  net.insert_arc(s1, d, 60); // s1 ships 60 of its 100
  net.insert_arc(s2, d, 50); // s2 ships its 50, and d receives its 110

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);

  DynDlist<SimpleNet::Node*> supply_list, demand_list;
net.non_feasible_nodes(supply_list, demand_list);
  
  EXPECT_EQ(supply_list.size(), 1u);
  EXPECT_EQ(demand_list.size(), 0u);
  EXPECT_EQ(supply_list.get_first(), s1);
}

TEST_F(FeasibilityTest, NonFeasibleDemandNodes)
{
  auto* d1 = net.insert_node(-100);
  auto* d2 = net.insert_node(-50);
  auto* s = net.insert_node(130);

  d1->in_cap = 200;
  d2->in_cap = 100;
  net.insert_arc(s, d1, 80); // d1 receives 80 of its 100
  net.insert_arc(s, d2, 50); // d2 receives its 50, and s ships its 130

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);

  DynDlist<SimpleNet::Node*> supply_list, demand_list;
net.non_feasible_nodes(supply_list, demand_list);
  
  EXPECT_EQ(supply_list.size(), 0u);
  EXPECT_EQ(demand_list.size(), 1u);
  EXPECT_EQ(demand_list.get_first(), d1);
}

TEST_F(FeasibilityTest, MultipleMixedNonFeasibleNodes)
{
  auto* s1 = net.insert_node(100);
  auto* s2 = net.insert_node(70);
  auto* d1 = net.insert_node(-60);
  auto* d2 = net.insert_node(-70);

  s1->out_cap = 200;
  s2->out_cap = 200;
  d1->in_cap = 200;
  d2->in_cap = 200;
  net.insert_arc(s1, d1, 30); // s1 ships 30 of its 100; d1 receives 30 of 60
  net.insert_arc(s2, d2, 80); // s2 ships its 70, and d2 receives its 70

  net.compute_aux_net();
  (void) edmonds_karp_maximum_flow(net);

  DynDlist<SimpleNet::Node*> supply_list, demand_list;
  net.non_feasible_nodes(supply_list, demand_list);
  
  EXPECT_EQ(supply_list.size(), 1u);
  EXPECT_EQ(demand_list.size(), 1u);
}

// =============================================================================
// Capacity Validation Tests
// =============================================================================

TEST_F(NetSupDemGraphTest, SupplyExceedsOutCapacityThrows)
{
  auto* p = net.insert_node(0);
  p->out_cap = 50;
  p->supply_flow = 100;  // Will exceed capacity
  
  EXPECT_THROW(net.compute_aux_net(), std::range_error);
}

TEST_F(NetSupDemGraphTest, DemandExceedsInCapacityThrows)
{
  auto* p = net.insert_node(0);
  p->in_cap = 30;
  p->supply_flow = -50;  // Demand exceeds in_cap
  
  EXPECT_THROW(net.compute_aux_net(), std::range_error);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(NetSupDemGraphTest, OnlySupplyNodes)
{
  auto* s1 = net.insert_node(100);
  auto* s2 = net.insert_node(50);
  
  s1->out_cap = 200;
  s2->out_cap = 100;
  
  net.compute_aux_net();
  
  // Should have super-source but no super-sink
  EXPECT_NE(net.get_super_source(), nullptr);
  EXPECT_EQ(net.get_super_sink(), nullptr);
}

TEST_F(NetSupDemGraphTest, OnlyDemandNodes)
{
  auto* d1 = net.insert_node(-100);
  auto* d2 = net.insert_node(-50);
  
  d1->in_cap = 200;
  d2->in_cap = 100;
  
  net.compute_aux_net();
  
  // Should have super-sink but no super-source
  EXPECT_EQ(net.get_super_source(), nullptr);
  EXPECT_NE(net.get_super_sink(), nullptr);
}

TEST_F(NetSupDemGraphTest, OnlyTransitNodes)
{
  net.insert_node(0);
  net.insert_node(0);
  net.insert_node(0);
  
  net.compute_aux_net();
  
  // Neither super-source nor super-sink needed
  EXPECT_EQ(net.get_super_source(), nullptr);
  EXPECT_EQ(net.get_super_sink(), nullptr);
  // But auxiliary net is still "computed" (flag set)
  EXPECT_FALSE(net.exist_aux_net());  // Both null means not "exist"
}

TEST_F(NetSupDemGraphTest, ZeroSupplyValue)
{
  auto* p = net.insert_node(0);
  EXPECT_EQ(p->supply_flow, 0);
  EXPECT_EQ(net.count_supply_nodes(), 0u);
  EXPECT_EQ(net.count_demand_nodes(), 0u);
}

// =============================================================================
// String Network Tests (Different Types)
// =============================================================================

class StringNetTest : public ::testing::Test
{
protected:
  StringNet net;
};

TEST_F(StringNetTest, InsertNodeWithStringInfo)
{
  auto* p = net.insert_node("Factory", 100.5);
  EXPECT_EQ(p->get_info(), "Factory");
  EXPECT_DOUBLE_EQ(p->supply_flow, 100.5);
}

TEST_F(StringNetTest, DoubleFlowValues)
{
  auto* s = net.insert_node("Source", 100.5);
  auto* d = net.insert_node("Sink", -50.25);
  
  s->out_cap = 200.0;
  d->in_cap = 100.0;
  
  EXPECT_DOUBLE_EQ(net.total_supply(), 100.5);
  EXPECT_DOUBLE_EQ(net.total_demand(), 50.25);
}

// =============================================================================
// Destructor Tests
// =============================================================================

TEST(NetSupDemDestructorTest, DestructorFreesAuxNet)
{
  auto* net = new SimpleNet();
  auto* s = net->insert_node(100);
  auto* d = net->insert_node(-100);
  
  s->out_cap = 200;
  d->in_cap = 200;
  
  net->compute_aux_net();
  EXPECT_TRUE(net->exist_aux_net());
  
  // Should not crash - destructor should handle cleanup
  delete net;
  SUCCEED();
}

// =============================================================================
// Integration Test
// =============================================================================

TEST(IntegrationTest, CompleteWorkflow)
{
  SimpleNet net;
  
  // Create a balanced network
  auto* factory1 = net.insert_node(100);
  auto* factory2 = net.insert_node(50);
  auto* warehouse = net.insert_node(0);
  auto* store1 = net.insert_node(-80);
  auto* store2 = net.insert_node(-70);
  
  // Set capacities
  factory1->out_cap = 150;
  factory2->out_cap = 100;
  store1->in_cap = 100;
  store2->in_cap = 100;
  
  // Connect with arcs
  net.insert_arc(factory1, warehouse, 100);
  net.insert_arc(factory2, warehouse, 50);
  net.insert_arc(warehouse, store1, 80);
  net.insert_arc(warehouse, store2, 70);
  
  // Check balance
  EXPECT_EQ(net.total_supply(), 150);
  EXPECT_EQ(net.total_demand(), 150);
  EXPECT_TRUE(net.is_balanced());
  
  // Compute auxiliary network
  net.compute_aux_net();
  EXPECT_TRUE(net.exist_aux_net());
  
  // The flow, and its feasibility
  EXPECT_EQ(edmonds_karp_maximum_flow(net), 150);
  EXPECT_TRUE(net.is_feasible());

  // Get non-feasible nodes (should be empty)
  DynDlist<SimpleNet::Node*> supply_list, demand_list;
  net.non_feasible_nodes(supply_list, demand_list);
  EXPECT_TRUE(supply_list.is_empty());
  EXPECT_TRUE(demand_list.is_empty());
  
  // Cleanup
  net.free_aux_net();
  EXPECT_FALSE(net.exist_aux_net());
}

// A ships 0.7 to B, which wants 0.1, and through C to D, which wants 0.6:
// it receives 0.7 and sends 0.1 + 0.6, whose exact sum is 2.8e-17 more
// than 0.7 in binary, well within the tolerance of check_node(). The
// return of the residues of rounding at the end of the augmenting paths
// moved it toward the sink, and the arc of D, or of B, to the super-sink
// lost an ulp of its demand: the network was not feasible
TEST(FloatingFeasibilityTest, AFlowOfTenthsIsFeasible)
{
  using Net = Net_Sup_Dem_Graph<Net_Sup_Dem_Node<Empty_Class, double>, Net_Arc<Empty_Class, double>>;
  auto feasible = [] (auto maximum_flow)
    {
      Net net;
      auto * a = net.insert_node(0.7);
      auto * b = net.insert_node(-0.1);
      auto * c = net.insert_node(0.0);
      auto * d = net.insert_node(-0.6);
      net.insert_arc(a, b, 3.2);
      net.insert_arc(a, c, 3.9);
      net.insert_arc(c, d, 0.7);
      net.compute_aux_net();
      maximum_flow(net);
      return net.is_feasible();
    };
  EXPECT_TRUE(feasible([] (Net & net) { edmonds_karp_maximum_flow(net); }));
  EXPECT_TRUE(feasible([] (Net & net) { ford_fulkerson_maximum_flow(net); }));
}

namespace
{
  using Floating_Net =
    Net_Sup_Dem_Graph<Net_Sup_Dem_Node<Empty_Class, double>, Net_Arc<Empty_Class, double>>;

  // The maximum flows of the library that run on a network of supplies
  // and demands, with their names
  struct Named_Maximum_Flow
  {
    const char * name;
    void (*run)(Floating_Net &);
  };

  const Named_Maximum_Flow maximum_flows[] = {
    {"ford_fulkerson", [] (Floating_Net & net) { ford_fulkerson_maximum_flow(net); }},
    {"edmonds_karp", [] (Floating_Net & net) { edmonds_karp_maximum_flow(net); }},
    {"dinic", [] (Floating_Net & net) { dinic_maximum_flow(net); }},
    {"fifo_preflow", [] (Floating_Net & net) { fifo_preflow_maximum_flow(net); }},
    {"heap_preflow", [] (Floating_Net & net) { heap_preflow_maximum_flow(net); }}};
} // anonymous namespace

// A supply of 2.4 meets the demands of 1.1 and 1.3, but 2.4 - 1.1 is
// 1.2999999999999998 in binary: after the path of 1.1, the arc from the
// super-source bounds the second path, whose flow comes out an ulp short
// of the demand of 1.3, or of 1.1 if the paths go the other way. An exact
// comparison took that demand as not met, with the five maximum flows
TEST(FloatingFeasibilityTest, TheDemandsOfASupplyInTenthsAreMet)
{
  for (const auto & [name, maximum_flow] : maximum_flows)
    {
      Floating_Net net;
      auto * a = net.insert_node(2.4);
      auto * b = net.insert_node(-1.1);
      auto * c = net.insert_node(-1.3);
      net.insert_arc(a, b, 5.0);
      net.insert_arc(a, c, 5.0);
      net.compute_aux_net();
      maximum_flow(net);
      EXPECT_TRUE(net.is_feasible()) << name;
      DynDlist<Floating_Net::Node *> supply_list, demand_list;
      net.non_feasible_nodes(supply_list, demand_list);
      EXPECT_TRUE(supply_list.is_empty()) << name;
      EXPECT_TRUE(demand_list.is_empty()) << name;
    }
}

// The tolerance is that of each arc, which keeps a real deficit: a demand
// of 1.3 that an arc of 1.2 bounds; and, beside a supply and a demand of
// 1000, a supply and a demand of 1e-7 that no arc joins, which a tolerance
// of the whole network, 1e-9 of 1000, would take as met
TEST(FloatingFeasibilityTest, ARealDeficitIsNotMet)
{
  for (const auto & [name, maximum_flow] : maximum_flows)
    {
      Floating_Net net;
      auto * a = net.insert_node(2.4);
      auto * b = net.insert_node(-1.1);
      auto * c = net.insert_node(-1.3);
      net.insert_arc(a, b, 5.0);
      net.insert_arc(a, c, 1.2);
      net.compute_aux_net();
      maximum_flow(net);
      EXPECT_FALSE(net.is_feasible()) << name;
      DynDlist<Floating_Net::Node *> supply_list, demand_list;
      net.non_feasible_nodes(supply_list, demand_list);
      ASSERT_EQ(supply_list.size(), 1u) << name;
      EXPECT_EQ(supply_list.get_first(), a) << name;
      ASSERT_EQ(demand_list.size(), 1u) << name;
      EXPECT_EQ(demand_list.get_first(), c) << name;
    }

  for (const auto & [name, maximum_flow] : maximum_flows)
    {
      Floating_Net net;
      auto * a = net.insert_node(1000.0);
      auto * b = net.insert_node(-1000.0);
      auto * c = net.insert_node(1e-7);
      auto * d = net.insert_node(-1e-7);
      net.insert_arc(a, b, 1000.0);
      net.compute_aux_net();
      maximum_flow(net);
      EXPECT_FALSE(net.is_feasible()) << name;
      DynDlist<Floating_Net::Node *> supply_list, demand_list;
      net.non_feasible_nodes(supply_list, demand_list);
      ASSERT_EQ(supply_list.size(), 1u) << name;
      EXPECT_EQ(supply_list.get_first(), c) << name;
      ASSERT_EQ(demand_list.size(), 1u) << name;
      EXPECT_EQ(demand_list.get_first(), d) << name;
    }
}

// On 300 random networks of 5 to 9 nodes in tenths, the supplies and the
// demands are what an exact maximum flow in integer tenths ships from
// and to the nodes that want to supply or demand up to 3.0: the
// instances are feasible by construction, and each maximum flow finds
// them so, where an exact comparison took some 490 of 1500 for not
// feasible
TEST(FloatingFeasibilityTest, FeasibleInstancesInTenthsAreFeasible)
{
  using Integer_Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, long>>;
  struct Spec_Arc
  {
    int u, v;
    long cap;  // in tenths
  };
  std::mt19937 gen(31337);
  size_t instances = 0;
  for (int k = 0; k < 300; ++k)
    {
      // a path through every node from 0 and to n - 1, and some arcs more
      const int n = 5 + int(gen() % 5);
      Array<Spec_Arc> arcs;
      auto cap = [&gen] { return long(1 + gen() % 40); };
      for (int i = 1; i < n - 1; ++i)
        {
          const int u = int(gen() % unsigned(i));
          arcs.append(Spec_Arc{u, i, cap()});
          const int v = i + 1 + int(gen() % unsigned(n - 1 - i));
          arcs.append(Spec_Arc{i, v, cap()});
        }
      for (int extra = int(4 + gen() % 11); extra > 0; --extra)
        {
          const int u = int(gen() % unsigned(n - 1));
          const int v = 1 + int(gen() % unsigned(n - 1));
          if (u != v)
            arcs.append(Spec_Arc{u, v, cap()});
        }

      // what each node wants to supply (positive) or demand (negative)
      Array<long> want(size_t(n), 0);
      for (int i = 0; i < n; ++i)
        {
          const unsigned r = gen() % 4;
          if (r == 0)
            want[size_t(i)] = 1 + long(gen() % 30);
          else if (r == 1)
            want[size_t(i)] = -(1 + long(gen() % 30));
        }
      want[0] = 1 + long(gen() % 30);
      want[size_t(n - 1)] = -(1 + long(gen() % 30));

      // what an exact maximum flow ships from and to them
      Integer_Net inet;
      Array<Integer_Net::Node *> inodes;
      for (int i = 0; i < n; ++i)
        inodes.append(inet.insert_node());
      auto * s = inet.insert_node();
      auto * t = inet.insert_node();
      for (size_t j = 0; j < arcs.size(); ++j)
        inet.insert_arc(inodes[size_t(arcs[j].u)], inodes[size_t(arcs[j].v)], arcs[j].cap);
      Array<Integer_Net::Arc *> ends(size_t(n), nullptr);
      for (int i = 0; i < n; ++i)
        if (want[size_t(i)] > 0)
          ends[size_t(i)] = inet.insert_arc(s, inodes[size_t(i)], want[size_t(i)]);
        else if (want[size_t(i)] < 0)
          ends[size_t(i)] = inet.insert_arc(inodes[size_t(i)], t, -want[size_t(i)]);
      edmonds_karp_maximum_flow(inet);
      Array<long> supply(size_t(n), 0);
      bool supplies = false, demands = false;
      for (int i = 0; i < n; ++i)
        if (ends[size_t(i)] != nullptr)
          {
            supply[size_t(i)] = want[size_t(i)] > 0 ? ends[size_t(i)]->flow : -ends[size_t(i)]->flow;
            supplies = supplies or supply[size_t(i)] > 0;
            demands = demands or supply[size_t(i)] < 0;
          }
      if (not supplies or not demands)
        continue;  // nothing to ship
      ++instances;

      for (const auto & [name, maximum_flow] : maximum_flows)
        {
          Floating_Net net;
          Array<Floating_Net::Node *> nodes;
          for (int i = 0; i < n; ++i)
            nodes.append(net.insert_node(double(supply[size_t(i)]) / 10.0));
          for (size_t j = 0; j < arcs.size(); ++j)
            net.insert_arc(nodes[size_t(arcs[j].u)], nodes[size_t(arcs[j].v)],
                           double(arcs[j].cap) / 10.0);
          net.compute_aux_net();
          maximum_flow(net);
          EXPECT_TRUE(net.is_feasible()) << name << " on the instance " << k;
        }
    }
  EXPECT_GT(instances, 250u);
}

// A supply of infinity and a demand of infinity, which nodes of infinite
// capacities admit, are met only by an infinite flow. The tolerance of
// an arc of infinite capacity, infinite too, took a maximum flow of 2.5
// between them as meeting both, and an infinite flow, inf - inf being
// NaN, as meeting neither
TEST(FloatingFeasibilityTest, AnInfiniteSupplyIsMetOnlyByAnInfiniteFlow)
{
  const double inf = std::numeric_limits<double>::infinity();
  for (const auto & [name, maximum_flow] : maximum_flows)
    for (const double cap : {2.5, inf})
      {
        Floating_Net net;
        auto * a = net.insert_node(inf);
        auto * b = net.insert_node(-inf);
        a->out_cap = inf;
        b->in_cap = inf;
        net.insert_arc(a, b, cap);
        net.compute_aux_net();
        maximum_flow(net);
        const bool met = cap == inf;
        EXPECT_EQ(net.is_feasible(), met) << name << " with a -> b of " << cap;
        DynDlist<Floating_Net::Node *> supply_list, demand_list;
        net.non_feasible_nodes(supply_list, demand_list);
        EXPECT_EQ(supply_list.size(), met ? 0u : 1u) << name << " with a -> b of " << cap;
        EXPECT_EQ(demand_list.size(), met ? 0u : 1u) << name << " with a -> b of " << cap;
        if (not met and supply_list.size() == 1u and demand_list.size() == 1u)
          {
            EXPECT_EQ(supply_list.get_first(), a) << name;
            EXPECT_EQ(demand_list.get_first(), b) << name;
          }
      }
}

// =============================================================================
// The super nodes when the network changes
// =============================================================================
//
// The network keeps the pointers of its super nodes: whatever changes it,
// through the network or through a reference to Net_Graph, must leave it
// remembering the super nodes that it has, and no other. The tests below
// take every way of changing it, by what the way does to the super nodes:
//   - it takes all the nodes away through the base, by the assignments,
//     the swap() and the move of Net_Graph;
//   - it removes nodes: remove_node() of a super node or of another,
//     clear_graph() and a copy_graph() into the network;
//   - it takes the super nodes along: the swap(), the moves and the copies
//     of the network itself;
//   - it leaves them: an insertion, a change of the arcs, another order of
//     the nodes, and what is done to the network with itself;
// each with every auxiliary network that a network may have: the two
// super nodes, the super source alone and the super sink alone. Through a
// reference to Array_Graph none of this holds: see Net_Graph.

namespace
{
  using SimpleBase = SimpleNet::Net_Class;

  // The auxiliary networks that a network may have: with the two super
  // nodes, with the super source alone, when nothing demands, and with the
  // super sink alone, when nothing supplies
  enum class Aux { both, source, sink };
  constexpr Aux every_aux[] = {Aux::both, Aux::source, Aux::sink};

  const char * name_of(const Aux aux)
  {
    return aux == Aux::both ? "with the two super nodes"
      : aux == Aux::source ? "with the super source alone" : "with the super sink alone";
  }

  // whether p is a node of net
  bool is_node_of(const SimpleNet & net, const SimpleNode * p)
  {
    for (Node_Iterator<SimpleNet> it(net); it.has_curr(); it.next_ne())
      if (it.get_curr() == p)
        return true;
    return false;
  }

  // Two nodes joined by an arc, with the auxiliary network asked for; it
  // keeps the super nodes as compute_aux_net() made them
  struct Shipment
  {
    SimpleNet net;
    SimpleNode * s = nullptr;
    SimpleNode * t = nullptr;
    SimpleArc * st = nullptr;
    SimpleNode * source = nullptr;
    SimpleNode * sink = nullptr;

    explicit Shipment(const Aux aux)
    {
      s = net.insert_node(aux == Aux::sink ? 0 : 10);
      t = net.insert_node(aux == Aux::source ? 0 : -10);
      st = net.insert_arc(s, t, 20);
      net.compute_aux_net();
      source = net.get_super_source();
      sink = net.get_super_sink();
    }

    // how many super nodes it was made with
    size_t supers() const { return size_t(source != nullptr) + size_t(sink != nullptr); }
  };

  // puts in net three nodes and two arcs
  void fill_plain(SimpleBase & net)
  {
    auto x = net.insert_node();
    auto y = net.insert_node();
    auto z = net.insert_node();
    net.insert_arc(x, y, 1);
    net.insert_arc(y, z, 1);
  }

  // net says that it has no auxiliary network, and has the nodes given
  void expect_no_aux_net(SimpleNet & net, const size_t nodes)
  {
    ASSERT_FALSE(net.exist_aux_net());
    EXPECT_EQ(net.get_super_source(), nullptr);
    EXPECT_EQ(net.get_super_sink(), nullptr);
    EXPECT_EQ(net.get_aux_net(), nullptr);
    EXPECT_THROW(net.free_aux_net(), std::domain_error);
    EXPECT_THROW((void) net.is_feasible(), std::domain_error);
    EXPECT_EQ(net.get_num_nodes(), nodes);
  }

  // net remembers the super nodes given, which are nodes of it, besides
  // `own` nodes; and free_aux_net() takes them away, and only them
  void expect_aux_net(SimpleNet & net, const SimpleNode * source, const SimpleNode * sink,
                      const size_t own)
  {
    ASSERT_EQ(net.get_super_source(), source);
    ASSERT_EQ(net.get_super_sink(), sink);
    const size_t supers = size_t(source != nullptr) + size_t(sink != nullptr);
    if (supers == 0)
      {
        expect_no_aux_net(net, own);
        return;
      }
    ASSERT_TRUE(source == nullptr or is_node_of(net, source));
    ASSERT_TRUE(sink == nullptr or is_node_of(net, sink));
    EXPECT_TRUE(net.exist_aux_net());
    EXPECT_EQ(net.get_aux_net(), &net);
    EXPECT_EQ(net.get_num_nodes(), own + supers);
    if (supers == 1)
      {
        EXPECT_FALSE(net.is_feasible()); // without one of them, nothing is shipped
      }
    net.free_aux_net();
    EXPECT_FALSE(net.exist_aux_net());
    EXPECT_EQ(net.get_num_nodes(), own);
  }
}

TEST(NetSupDemChangedNetwork, AnExchangeThroughTheBaseTakesTheSuperNodesAway)
{
  for (const Aux aux : every_aux)
    {
      SCOPED_TRACE(name_of(aux));
      {
        // the copy assignment of Net_Graph frees the nodes of the network,
        // its super nodes among them
        Shipment sh(aux);
        SimpleBase other;
        fill_plain(other);
        SimpleBase & base = sh.net;
        base = other;
        expect_no_aux_net(sh.net, 3);

        // and the network that came takes an auxiliary network of its own
        sh.net.set_supply_flow(sh.net.get_first_node(), 5);
        sh.net.compute_aux_net();
        ASSERT_NE(sh.net.get_super_source(), nullptr);
        expect_aux_net(sh.net, sh.net.get_super_source(), nullptr, 3);
      }
      {
        // the move assignment swaps: the other network has them then, as
        // nodes like the others
        Shipment sh(aux);
        SimpleBase other;
        fill_plain(other);
        SimpleBase & base = sh.net;
        base = std::move(other);
        expect_no_aux_net(sh.net, 3);
        EXPECT_EQ(other.get_num_nodes(), 2 + sh.supers());
      }
      {
        // so does swap()
        Shipment sh(aux);
        SimpleBase other;
        fill_plain(other);
        SimpleBase & base = sh.net;
        base.swap(other);
        expect_no_aux_net(sh.net, 3);
        EXPECT_EQ(other.get_num_nodes(), 2 + sh.supers());
      }
      {
        // and the move construction of a Net_Graph, which empties it
        Shipment sh(aux);
        SimpleBase & base = sh.net;
        SimpleBase taken(std::move(base));
        EXPECT_EQ(taken.get_num_nodes(), 2 + sh.supers());
        expect_no_aux_net(sh.net, 0);
      }
      for (const Aux aux2 : every_aux)
        {
          // between two networks with their auxiliary networks, each one
          // has then the super nodes of the other, and remembers none
          SCOPED_TRACE(name_of(aux2));
          Shipment sh(aux), sh2(aux2);
          sh2.net.insert_node(0);
          SimpleBase & base = sh.net;
          SimpleBase & base2 = sh2.net;
          base.swap(base2);
          expect_no_aux_net(sh.net, 3 + sh2.supers());
          expect_no_aux_net(sh2.net, 2 + sh.supers());
        }
    }
}

TEST(NetSupDemChangedNetwork, ARemovedSuperNodeIsForgotten)
{
  for (const Aux aux : every_aux)
    for (const bool the_source : {true, false})
      for (const bool through_the_base : {false, true})
        {
          Shipment sh(aux);
          SimpleNode * const gone = the_source ? sh.source : sh.sink;
          if (gone == nullptr)
            continue; // it has no such super node
          SCOPED_TRACE(std::string(name_of(aux)) + (the_source ? ", its super source" : ", its super sink")
                       + (through_the_base ? ", through the base" : ""));
          // remove_node() is virtual: through a reference to the base too
          if (through_the_base)
            static_cast<SimpleBase &>(sh.net).remove_node(gone);
          else
            sh.net.remove_node(gone);
          expect_aux_net(sh.net, the_source ? nullptr : sh.source, the_source ? sh.sink : nullptr, 2);
        }
}

TEST(NetSupDemChangedNetwork, AnotherNodeRemovedIsNoneOfThem)
{
  for (const Aux aux : every_aux)
    {
      SCOPED_TRACE(name_of(aux));
      {
        // a node without arcs
        Shipment sh(aux);
        sh.net.remove_node(sh.net.insert_node(0));
        expect_aux_net(sh.net, sh.source, sh.sink, 2);
      }
      {
        // a node with an arc of a super node, which goes with it
        Shipment sh(aux);
        sh.net.remove_node(aux == Aux::sink ? sh.t : sh.s);
        expect_aux_net(sh.net, sh.source, sh.sink, 1);
      }
    }
}

TEST(NetSupDemChangedNetwork, ClearGraphAndCopyGraphTakeTheSuperNodesAway)
{
  for (const Aux aux : every_aux)
    {
      SCOPED_TRACE(name_of(aux));
      {
        Shipment sh(aux);
        clear_graph(sh.net);
        expect_no_aux_net(sh.net, 0);
      }
      for (const Aux aux2 : every_aux)
        {
          // copy_graph() empties the network first; the super nodes of the
          // network that it copies come as nodes like the others
          SCOPED_TRACE(name_of(aux2));
          Shipment sh(aux), from(aux2);
          copy_graph(sh.net, from.net);
          expect_no_aux_net(sh.net, 2 + from.supers());
          expect_aux_net(from.net, from.source, from.sink, 2);
        }
    }
}

TEST(NetSupDemChangedNetwork, AChangeThatLeavesTheSuperNodesKeepsThem)
{
  // each way returns how many nodes the network has then, besides its
  // super nodes
  struct Way
  {
    const char * name;
    size_t (*change)(Shipment &);
  };
  const Way ways[] = {
    {"nothing", [] (Shipment &) -> size_t { return 2; }},
    {"a node inserted", [] (Shipment & sh) -> size_t
      {
        sh.net.insert_node(0);
        return 3;
      }},
    {"an arc inserted", [] (Shipment & sh) -> size_t
      {
        sh.net.insert_arc(sh.t, sh.s, 1);
        return 2;
      }},
    {"an arc removed", [] (Shipment & sh) -> size_t
      {
        sh.net.remove_arc(sh.st);
        return 2;
      }},
    {"the arcs of the super nodes removed", [] (Shipment & sh) -> size_t
      {
        DynDlist<SimpleArc *> arcs;
        for (SimpleNode * p : {sh.source, sh.sink})
          if (p != nullptr)
            for (SimpleBase::Node_Arc_Iterator it(p); it.has_curr(); it.next_ne())
              arcs.append(it.get_curr());
        for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
          sh.net.remove_arc(it.get_curr());
        return 2;
      }},
    {"an arc disconnected and connected again", [] (Shipment & sh) -> size_t
      {
        sh.net.disconnect_arc(sh.st);
        sh.net.connect_arc(sh.st);
        return 2;
      }},
    {"another order of the nodes", [] (Shipment & sh) -> size_t
      {
        sh.net.sort_nodes([] (SimpleNode * p, SimpleNode * q) { return p->supply_flow < q->supply_flow; });
        return 2;
      }},
    {"swapped with itself through the base", [] (Shipment & sh) -> size_t
      {
        SimpleBase & base = sh.net;
        base.swap(base);
        return 2;
      }},
    {"assigned to itself through the base", [] (Shipment & sh) -> size_t
      {
        SimpleBase & base = sh.net;
        const SimpleBase & same = sh.net;
        base = same;
        return 2;
      }},
    {"moved to itself through the base", [] (Shipment & sh) -> size_t
      {
        SimpleBase & base = sh.net;
        SimpleBase & same = sh.net;
        base = std::move(same);
        return 2;
      }},
    {"swapped with itself", [] (Shipment & sh) -> size_t
      {
        sh.net.swap(sh.net);
        return 2;
      }},
    {"assigned to itself", [] (Shipment & sh) -> size_t
      {
        const SimpleNet & same = sh.net;
        sh.net = same;
        return 2;
      }},
    {"moved to itself", [] (Shipment & sh) -> size_t
      {
        SimpleNet & same = sh.net;
        sh.net = std::move(same);
        return 2;
      }}
  };
  for (const Aux aux : every_aux)
    for (const Way & way : ways)
      {
        SCOPED_TRACE(std::string(name_of(aux)) + ", " + way.name);
        Shipment sh(aux);
        const size_t own = way.change(sh);
        expect_aux_net(sh.net, sh.source, sh.sink, own);
      }
}

TEST(NetSupDemChangedNetwork, ItsOwnSwapMovesAndCopiesTakeTheSuperNodesAlong)
{
  for (const Aux aux : every_aux)
    {
      SCOPED_TRACE(name_of(aux));
      {
        Shipment sh(aux);
        SimpleNet moved(std::move(sh.net));
        expect_no_aux_net(sh.net, 0);
        expect_aux_net(moved, sh.source, sh.sink, 2);

        // the network that was moved from takes an auxiliary network again
        sh.net.insert_node(7);
        sh.net.compute_aux_net();
        ASSERT_NE(sh.net.get_super_source(), nullptr);
        expect_aux_net(sh.net, sh.net.get_super_source(), nullptr, 1);
      }
      {
        // a copy has super nodes of its own, at the places of those of the
        // network, which keeps its own
        Shipment sh(aux);
        SimpleNet copy(sh.net);
        EXPECT_EQ(copy.get_super_source() != nullptr, sh.source != nullptr);
        EXPECT_EQ(copy.get_super_sink() != nullptr, sh.sink != nullptr);
        if (sh.source != nullptr)
          {
            EXPECT_NE(copy.get_super_source(), sh.source);
          }
        if (sh.sink != nullptr)
          {
            EXPECT_NE(copy.get_super_sink(), sh.sink);
          }
        expect_aux_net(copy, copy.get_super_source(), copy.get_super_sink(), 2);
        expect_aux_net(sh.net, sh.source, sh.sink, 2);
      }
      for (const Aux aux2 : every_aux)
        {
          SCOPED_TRACE(name_of(aux2));
          {
            // the move assignment exchanges them, as swap()
            Shipment sh(aux), target(aux2);
            target.net.insert_node(0);
            target.net = std::move(sh.net);
            expect_aux_net(target.net, sh.source, sh.sink, 2);
            expect_aux_net(sh.net, target.source, target.sink, 3);
          }
          {
            Shipment sh(aux), sh2(aux2);
            sh2.net.insert_node(0);
            sh.net.swap(sh2.net);
            expect_aux_net(sh.net, sh2.source, sh2.sink, 3);
            expect_aux_net(sh2.net, sh.source, sh.sink, 2);
          }
          {
            // the copy assignment frees those that the target had
            Shipment sh(aux), target(aux2);
            target.net.insert_node(0);
            target.net = sh.net;
            EXPECT_EQ(target.net.get_super_source() != nullptr, sh.source != nullptr);
            EXPECT_EQ(target.net.get_super_sink() != nullptr, sh.sink != nullptr);
            expect_aux_net(target.net, target.net.get_super_source(), target.net.get_super_sink(), 2);
            expect_aux_net(sh.net, sh.source, sh.sink, 2);
          }
        }
    }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
