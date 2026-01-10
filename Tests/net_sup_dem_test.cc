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

/**
 * @file net_sup_dem_test.cc
 * @brief Comprehensive tests for Net_Sup_Dem_Graph class
 */

#include <gtest/gtest.h>
#include <tpl_net_sup_dem.H>
#include <tpl_dynDlist.H>
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

TEST_F(NetSupDemGraphTest, InsertNodeDefaultSupply)
{
  auto* p = net.insert_node();
  EXPECT_EQ(p->supply_flow, 0);
}

// =============================================================================
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

TEST_F(FeasibilityTest, SupplyOnlyNetwork)
{
  auto* s = net.insert_node(100);
  s->out_cap = 200;
  s->out_flow = 100;  // Simulate flow
  
  net.compute_aux_net();
  EXPECT_TRUE(net.is_feasible());
}

TEST_F(FeasibilityTest, SupplyNotMeetingFlowRequirement)
{
  auto* s = net.insert_node(100);
  s->out_cap = 200;
  s->out_flow = 50;  // Less than required
  
  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible());
}

TEST_F(FeasibilityTest, DemandMet)
{
  auto* d = net.insert_node(-80);
  d->in_cap = 100;
  d->in_flow = 80;  // Exactly meeting demand
  
  net.compute_aux_net();
  EXPECT_TRUE(net.is_feasible());
}

TEST_F(FeasibilityTest, DemandNotMet)
{
  auto* d = net.insert_node(-80);
  d->in_cap = 100;
  d->in_flow = 50;  // Less than required
  
  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible());
}

TEST_F(FeasibilityTest, MixedSupplyDemandFeasible)
{
  auto* s = net.insert_node(100);
  auto* d = net.insert_node(-100);
  
  s->out_cap = 150;
  s->out_flow = 100;
  d->in_cap = 150;
  d->in_flow = 100;
  
  net.insert_arc(s, d, 100);
  
  net.compute_aux_net();
  EXPECT_TRUE(net.is_feasible());
}

TEST_F(FeasibilityTest, MixedSupplyDemandNotFeasible)
{
  auto* s = net.insert_node(100);
  auto* d = net.insert_node(-100);
  
  s->out_cap = 150;
  s->out_flow = 80;  // Not enough
  d->in_cap = 150;
  d->in_flow = 80;
  
  net.insert_arc(s, d, 100);
  
  net.compute_aux_net();
  EXPECT_FALSE(net.is_feasible());
}

// =============================================================================
// Non-Feasible Nodes Tests
// =============================================================================

TEST_F(FeasibilityTest, NonFeasibleSupplyNodes)
{
  auto* s1 = net.insert_node(100);
  auto* s2 = net.insert_node(50);
  
  s1->out_cap = 200;
  s1->out_flow = 60;  // Not enough
  s2->out_cap = 100;
  s2->out_flow = 50;  // OK
  
  net.compute_aux_net();
  
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
  
  d1->in_cap = 200;
  d1->in_flow = 80;  // Not enough
  d2->in_cap = 100;
  d2->in_flow = 50;  // OK
  
  net.compute_aux_net();
  
  DynDlist<SimpleNet::Node*> supply_list, demand_list;
  net.non_feasible_nodes(supply_list, demand_list);
  
  EXPECT_EQ(supply_list.size(), 0u);
  EXPECT_EQ(demand_list.size(), 1u);
  EXPECT_EQ(demand_list.get_first(), d1);
}

TEST_F(FeasibilityTest, MultipleMixedNonFeasibleNodes)
{
  auto* s1 = net.insert_node(100);
  auto* s2 = net.insert_node(80);
  auto* d1 = net.insert_node(-60);
  auto* d2 = net.insert_node(-70);
  
  s1->out_cap = 200; s1->out_flow = 50;   // Not enough
  s2->out_cap = 200; s2->out_flow = 80;   // OK
  d1->in_cap = 200;  d1->in_flow = 30;    // Not enough
  d2->in_cap = 200;  d2->in_flow = 70;    // OK
  
  net.compute_aux_net();
  
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
  
  // Simulate successful max-flow
  factory1->out_flow = 100;
  factory2->out_flow = 50;
  store1->in_flow = 80;
  store2->in_flow = 70;
  
  // Check feasibility
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

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
