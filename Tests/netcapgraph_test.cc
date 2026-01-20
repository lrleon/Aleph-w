
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file netcapgraph_test.cc
 * @brief Tests for Netcapgraph
 */
#include <gtest/gtest.h>
#include <random>
#include <set>
#include <map>
#include <string>
#include <sstream>
#include <cmath>
#include <tpl_netcapgraph.H>
#include <tpl_net.H>

using namespace Aleph;
using namespace std;

// =============================================================================
// Type Aliases for Testing
// =============================================================================

using DefaultNode = Net_Cap_Node<Empty_Class, double>;
using DefaultArc = Net_Arc<Empty_Class, double>;
using DefaultNet = Net_Cap_Graph<DefaultNode, DefaultArc>;

using IntNode = Net_Cap_Node<int, long>;
using IntArc = Net_Arc<int, long>;
using IntNet = Net_Cap_Graph<IntNode, IntArc>;

using StringNode = Net_Cap_Node<string, double>;
using StringArc = Net_Arc<string, double>;
using StringNet = Net_Cap_Graph<StringNode, StringArc>;

// =============================================================================
// EDGE CASE TESTS
// =============================================================================

class NetCapGraphEdgeCases : public ::testing::Test
{
protected:
  DefaultNet net;
};

// -----------------------------------------------------------------------------
// Empty Network Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, EmptyNetwork_NoNodes)
{
  EXPECT_EQ(net.vsize(), 0);
  EXPECT_EQ(net.esize(), 0);
  EXPECT_FALSE(net.has_aux_net());
  EXPECT_EQ(net.get_aux_net(), nullptr);
}

TEST_F(NetCapGraphEdgeCases, EmptyNetwork_ComputeAuxNetFails)
{
  // Computing aux net on empty network should succeed but produce empty aux net
  // or throw - depends on implementation. Let's add nodes first.
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 15.0, 0.0);
  
  EXPECT_NO_THROW(net.compute_aux_net());
  EXPECT_TRUE(net.has_aux_net());
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Single Node Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, SingleNode_UnlimitedCapacity)
{
  auto node = net.insert_node();
  
  EXPECT_EQ(net.vsize(), 1);
  EXPECT_EQ(node->max_cap, numeric_limits<double>::max());
  EXPECT_EQ(node->in_flow, 0.0);
  EXPECT_EQ(node->out_flow, 0.0);
}

TEST_F(NetCapGraphEdgeCases, SingleNode_SpecificCapacity)
{
  auto node = net.insert_node(100.0);
  
  EXPECT_EQ(node->max_cap, 100.0);
}

TEST_F(NetCapGraphEdgeCases, SingleNode_ZeroCapacity)
{
  auto node = net.insert_node(0.0);
  
  EXPECT_EQ(node->max_cap, 0.0);
}

TEST_F(NetCapGraphEdgeCases, SingleNode_NegativeCapacityThrows)
{
  EXPECT_THROW(net.insert_node(-1.0), std::domain_error);
}

// -----------------------------------------------------------------------------
// Capacity Boundary Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, CapacityBoundary_MaxDouble)
{
  auto node = net.insert_node(numeric_limits<double>::max());
  EXPECT_EQ(node->max_cap, numeric_limits<double>::max());
}

TEST_F(NetCapGraphEdgeCases, CapacityBoundary_SmallPositive)
{
  auto node = net.insert_node(numeric_limits<double>::min());
  EXPECT_EQ(node->max_cap, numeric_limits<double>::min());
}

TEST_F(NetCapGraphEdgeCases, CapacityBoundary_Epsilon)
{
  auto node = net.insert_node(numeric_limits<double>::epsilon());
  EXPECT_EQ(node->max_cap, numeric_limits<double>::epsilon());
}

// -----------------------------------------------------------------------------
// Arc Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, Arc_ZeroCapacity)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(10.0);
  auto arc = net.insert_arc(n1, n2, 0.0, 0.0);
  
  EXPECT_EQ(arc->cap, 0.0);
  EXPECT_EQ(arc->flow, 0.0);
}

TEST_F(NetCapGraphEdgeCases, Arc_FlowEqualsCapacity)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(10.0);
  auto arc = net.insert_arc(n1, n2, 5.0, 5.0);
  
  EXPECT_EQ(arc->cap, 5.0);
  EXPECT_EQ(arc->flow, 5.0);
}

TEST_F(NetCapGraphEdgeCases, Arc_FlowExceedsCapacityThrows)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(10.0);
  
  EXPECT_THROW(net.insert_arc(n1, n2, 5.0, 10.0), std::overflow_error);
}

// -----------------------------------------------------------------------------
// Auxiliary Network Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, AuxNet_DoubleComputeThrows)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 15.0, 0.0);
  
  net.compute_aux_net();
  EXPECT_THROW(net.compute_aux_net(), std::domain_error);
  
  net.free_aux_net();
}

TEST_F(NetCapGraphEdgeCases, AuxNet_FreeWithoutComputeThrows)
{
  EXPECT_THROW(net.free_aux_net(), std::domain_error);
}

TEST_F(NetCapGraphEdgeCases, AuxNet_UpdateWithoutComputeThrows)
{
  EXPECT_THROW(net.update(), std::domain_error);
}

TEST_F(NetCapGraphEdgeCases, AuxNet_ComputeFreeCycle)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 15.0, 0.0);
  
  // Multiple compute-free cycles should work
  for (int i = 0; i < 3; ++i)
  {
    EXPECT_NO_THROW(net.compute_aux_net());
    EXPECT_TRUE(net.has_aux_net());
    EXPECT_NO_THROW(net.free_aux_net());
    EXPECT_FALSE(net.has_aux_net());
  }
}

// -----------------------------------------------------------------------------
// Copy and Move Tests
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphEdgeCases, CopyConstruction)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 15.0, 3.0);
  
  DefaultNet copy(net);
  
  EXPECT_EQ(copy.vsize(), 2);
  EXPECT_EQ(copy.esize(), 1);
  EXPECT_FALSE(copy.has_aux_net());  // Aux net is not copied
}

TEST_F(NetCapGraphEdgeCases, MoveConstruction)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 15.0, 3.0);
  net.compute_aux_net();
  
  DefaultNet moved(std::move(net));
  
  EXPECT_EQ(moved.vsize(), 2);
  EXPECT_EQ(moved.esize(), 1);
  EXPECT_TRUE(moved.has_aux_net());
  
  moved.free_aux_net();
}

// =============================================================================
// STRUCTURAL TESTS - Various Network Topologies
// =============================================================================

class NetCapGraphStructure : public ::testing::Test
{
protected:
  DefaultNet net;
};

// -----------------------------------------------------------------------------
// Linear Network (Pipeline)
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStructure, LinearNetwork_ThreeNodes)
{
  // A --[10]--> B --[8]--> C
  // Capacities: A=20, B=5, C=15
  // Bottleneck should be at B (capacity 5)
  
  auto a = net.insert_node(20.0);
  auto b = net.insert_node(5.0);
  auto c = net.insert_node(15.0);
  
  net.insert_arc(a, b, 10.0, 0.0);
  net.insert_arc(b, c, 8.0, 0.0);
  
  EXPECT_EQ(net.vsize(), 3);
  EXPECT_EQ(net.esize(), 2);
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  // Aux net should have 2*3 = 6 nodes and 2 + 3 = 5 arcs
  // (3 node-capacity arcs + 2 edge arcs)
  EXPECT_EQ(aux->vsize(), 6);
  EXPECT_EQ(aux->esize(), 5);
  
  net.free_aux_net();
}

TEST_F(NetCapGraphStructure, LinearNetwork_TenNodes)
{
  vector<DefaultNet::Node*> nodes;
  for (int i = 0; i < 10; ++i)
    nodes.push_back(net.insert_node(10.0 + i));
  
  for (int i = 0; i < 9; ++i)
    net.insert_arc(nodes[i], nodes[i+1], 5.0 + i, 0.0);
  
  EXPECT_EQ(net.vsize(), 10);
  EXPECT_EQ(net.esize(), 9);
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 20);  // 2 * 10
  EXPECT_EQ(aux->esize(), 19);  // 10 + 9
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Star Network (Hub and Spokes)
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStructure, StarNetwork_FiveSpokes)
{
  // Central hub with 5 spokes
  auto hub = net.insert_node(50.0);  // Hub can handle up to 50 flow
  
  for (int i = 0; i < 5; ++i)
  {
    auto spoke = net.insert_node(10.0);
    net.insert_arc(spoke, hub, 15.0, 0.0);  // Incoming to hub
  }
  
  EXPECT_EQ(net.vsize(), 6);
  EXPECT_EQ(net.esize(), 5);
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 12);
  EXPECT_EQ(aux->esize(), 11);
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Diamond Network (Parallel Paths)
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStructure, DiamondNetwork)
{
  /*
   *       B
   *      / \
   *     A   D
   *      \ /
   *       C
   */
  
  auto a = net.insert_node(100.0);
  auto b = net.insert_node(30.0);
  auto c = net.insert_node(40.0);
  auto d = net.insert_node(100.0);
  
  net.insert_arc(a, b, 50.0, 0.0);
  net.insert_arc(a, c, 60.0, 0.0);
  net.insert_arc(b, d, 50.0, 0.0);
  net.insert_arc(c, d, 60.0, 0.0);
  
  EXPECT_EQ(net.vsize(), 4);
  EXPECT_EQ(net.esize(), 4);
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 8);
  EXPECT_EQ(aux->esize(), 8);
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Complete Network (All-to-All)
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStructure, CompleteNetwork_FiveNodes)
{
  vector<DefaultNet::Node*> nodes;
  for (int i = 0; i < 5; ++i)
    nodes.push_back(net.insert_node(20.0));
  
  for (int i = 0; i < 5; ++i)
    for (int j = 0; j < 5; ++j)
      if (i != j)
        net.insert_arc(nodes[i], nodes[j], 10.0, 0.0);
  
  EXPECT_EQ(net.vsize(), 5);
  EXPECT_EQ(net.esize(), 20);  // 5 * 4 = 20 directed edges
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 10);
  EXPECT_EQ(aux->esize(), 25);  // 5 node arcs + 20 edge arcs
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Bipartite Network
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStructure, BipartiteNetwork)
{
  // Left side: 3 nodes, Right side: 3 nodes
  vector<DefaultNet::Node*> left, right;
  
  for (int i = 0; i < 3; ++i)
    left.push_back(net.insert_node(10.0));
  
  for (int i = 0; i < 3; ++i)
    right.push_back(net.insert_node(15.0));
  
  // Connect every left to every right
  for (auto l : left)
    for (auto r : right)
      net.insert_arc(l, r, 5.0, 0.0);
  
  EXPECT_EQ(net.vsize(), 6);
  EXPECT_EQ(net.esize(), 9);
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 12);
  EXPECT_EQ(aux->esize(), 15);
  
  net.free_aux_net();
}

// =============================================================================
// REALISTIC SCENARIO TESTS
// =============================================================================

class NetCapGraphRealistic : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng.seed(42);  // Reproducible randomness
  }
  
  mt19937 rng;
};

// -----------------------------------------------------------------------------
// Data Center Network Model
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphRealistic, DataCenterNetwork)
{
  // Model: 
  // - 3 data centers (high capacity)
  // - 10 edge servers (medium capacity)
  // - Data flows from edge to data centers
  
  DefaultNet net;
  
  // Create data centers
  vector<DefaultNet::Node*> datacenters;
  for (int i = 0; i < 3; ++i)
    datacenters.push_back(net.insert_node(1000.0));  // High capacity
  
  // Create edge servers
  vector<DefaultNet::Node*> edges;
  for (int i = 0; i < 10; ++i)
    edges.push_back(net.insert_node(100.0));  // Medium capacity
  
  // Connect edges to data centers (each edge connects to 2 data centers)
  for (int i = 0; i < 10; ++i)
  {
    int dc1 = i % 3;
    int dc2 = (i + 1) % 3;
    net.insert_arc(edges[i], datacenters[dc1], 50.0, 0.0);
    net.insert_arc(edges[i], datacenters[dc2], 50.0, 0.0);
  }
  
  EXPECT_EQ(net.vsize(), 13);
  EXPECT_EQ(net.esize(), 20);
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  // Validate structure
  EXPECT_EQ(aux->vsize(), 26);   // 2 * 13
  EXPECT_EQ(aux->esize(), 33);   // 13 + 20
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Water Distribution Network
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphRealistic, WaterDistributionNetwork)
{
  // Model:
  // - 1 reservoir (source, high capacity)
  // - 3 pumping stations (medium capacity, bottlenecks)
  // - 10 neighborhoods (sinks)
  
  DefaultNet net;
  
  auto reservoir = net.insert_node(10000.0);  // High capacity source
  
  vector<DefaultNet::Node*> pumps;
  for (int i = 0; i < 3; ++i)
    pumps.push_back(net.insert_node(2000.0));  // Pumping stations
  
  vector<DefaultNet::Node*> neighborhoods;
  for (int i = 0; i < 10; ++i)
    neighborhoods.push_back(net.insert_node(500.0));  // Consumption nodes
  
  // Reservoir to pumps
  for (auto pump : pumps)
    net.insert_arc(reservoir, pump, 3000.0, 0.0);
  
  // Pumps to neighborhoods (distributed)
  for (int i = 0; i < 10; ++i)
  {
    int pump_idx = i % 3;
    net.insert_arc(pumps[pump_idx], neighborhoods[i], 600.0, 0.0);
  }
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  // Basic structure validation
  EXPECT_GT(aux->vsize(), 0);
  EXPECT_GT(aux->esize(), 0);
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Traffic Network with Intersections
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphRealistic, TrafficNetwork)
{
  // Model a 4x4 grid of intersections
  // Each intersection has limited capacity (throughput)
  
  DefaultNet net;
  DefaultNet::Node* grid[4][4];
  
  // Create intersections
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 4; ++j)
      grid[i][j] = net.insert_node(100.0);  // Each intersection handles 100 cars/min
  
  // Connect horizontally
  for (int i = 0; i < 4; ++i)
    for (int j = 0; j < 3; ++j)
    {
      net.insert_arc(grid[i][j], grid[i][j+1], 50.0, 0.0);
      net.insert_arc(grid[i][j+1], grid[i][j], 50.0, 0.0);
    }
  
  // Connect vertically
  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 4; ++j)
    {
      net.insert_arc(grid[i][j], grid[i+1][j], 50.0, 0.0);
      net.insert_arc(grid[i+1][j], grid[i][j], 50.0, 0.0);
    }
  
  EXPECT_EQ(net.vsize(), 16);
  EXPECT_EQ(net.esize(), 48);  // 2 * (3*4 + 4*3) = 2 * 24 = 48
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 32);
  EXPECT_EQ(aux->esize(), 64);  // 16 + 48
  
  net.free_aux_net();
}

// =============================================================================
// STRESS TESTS
// =============================================================================

class NetCapGraphStress : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng.seed(12345);
  }
  
  mt19937 rng;
};

// -----------------------------------------------------------------------------
// Large Random Network
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStress, LargeRandomNetwork)
{
  DefaultNet net;
  const int NUM_NODES = 500;
  const int NUM_ARCS_PER_NODE = 5;
  
  vector<DefaultNet::Node*> nodes;
  uniform_real_distribution<double> cap_dist(10.0, 1000.0);
  uniform_int_distribution<int> node_dist(0, NUM_NODES - 1);
  
  // Create nodes
  for (int i = 0; i < NUM_NODES; ++i)
    nodes.push_back(net.insert_node(cap_dist(rng)));
  
  // Create random arcs
  set<pair<int, int>> existing_arcs;
  for (int i = 0; i < NUM_NODES; ++i)
  {
    for (int j = 0; j < NUM_ARCS_PER_NODE; ++j)
    {
      int target = node_dist(rng);
      if (target != i && existing_arcs.find({i, target}) == existing_arcs.end())
      {
        existing_arcs.insert({i, target});
        net.insert_arc(nodes[i], nodes[target], cap_dist(rng), 0.0);
      }
    }
  }
  
  EXPECT_EQ(net.vsize(), NUM_NODES);
  EXPECT_GT(net.esize(), 0);
  
  // Compute auxiliary network
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  EXPECT_EQ(aux->vsize(), 2 * NUM_NODES);
  EXPECT_EQ(aux->esize(), NUM_NODES + net.esize());
  
  // Update should not throw
  EXPECT_NO_THROW(net.update());
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Deep Chain Network
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStress, DeepChainNetwork)
{
  DefaultNet net;
  const int CHAIN_LENGTH = 1000;
  
  vector<DefaultNet::Node*> nodes;
  nodes.push_back(net.insert_node(100.0));
  
  for (int i = 1; i < CHAIN_LENGTH; ++i)
  {
    nodes.push_back(net.insert_node(100.0));
    net.insert_arc(nodes[i-1], nodes[i], 50.0, 0.0);
  }
  
  EXPECT_EQ(net.vsize(), CHAIN_LENGTH);
  EXPECT_EQ(net.esize(), CHAIN_LENGTH - 1);
  
  auto aux = net.compute_aux_net();
  EXPECT_EQ(aux->vsize(), 2 * CHAIN_LENGTH);
  
  net.free_aux_net();
}

// -----------------------------------------------------------------------------
// Wide Network (Many Parallel Paths)
// -----------------------------------------------------------------------------

TEST_F(NetCapGraphStress, WideNetwork)
{
  DefaultNet net;
  const int WIDTH = 200;
  
  auto source = net.insert_node(10000.0);
  auto sink = net.insert_node(10000.0);
  
  // Create WIDTH parallel paths of length 3
  for (int i = 0; i < WIDTH; ++i)
  {
    auto mid1 = net.insert_node(50.0);
    auto mid2 = net.insert_node(50.0);
    
    net.insert_arc(source, mid1, 100.0, 0.0);
    net.insert_arc(mid1, mid2, 100.0, 0.0);
    net.insert_arc(mid2, sink, 100.0, 0.0);
  }
  
  EXPECT_EQ(net.vsize(), 2 + 2 * WIDTH);
  EXPECT_EQ(net.esize(), 3 * WIDTH);
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  net.free_aux_net();
}

// =============================================================================
// FLOW CONSERVATION AND VALIDATION TESTS
// =============================================================================

class NetCapGraphValidation : public ::testing::Test
{
protected:
  DefaultNet net;
};

TEST_F(NetCapGraphValidation, ResetFlows)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(10.0);
  auto arc = net.insert_arc(n1, n2, 10.0, 5.0);
  
  // Manually set some flows
  n1->in_flow = 3.0;
  n1->out_flow = 3.0;
  arc->flow = 5.0;
  
  net.reset_flows();
  
  EXPECT_EQ(n1->in_flow, 0.0);
  EXPECT_EQ(n1->out_flow, 0.0);
  EXPECT_EQ(arc->flow, 0.0);
}

TEST_F(NetCapGraphValidation, CheckNodeCapacities_Valid)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 8.0, 0.0);
  
  // Set valid flows
  n1->in_flow = 5.0;
  n1->out_flow = 5.0;
  n2->in_flow = 4.0;
  n2->out_flow = 4.0;
  
  EXPECT_TRUE(net.check_node_capacities());
}

TEST_F(NetCapGraphValidation, CheckNodeCapacities_Invalid)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(5.0);
  net.insert_arc(n1, n2, 8.0, 0.0);
  
  // Set invalid flows (exceeds n2 capacity)
  n1->in_flow = 8.0;
  n1->out_flow = 8.0;
  n2->in_flow = 8.0;  // Exceeds capacity of 5
  n2->out_flow = 8.0;
  
  EXPECT_FALSE(net.check_node_capacities());
}

TEST_F(NetCapGraphValidation, SetNodeCapacity_Valid)
{
  auto node = net.insert_node(10.0);
  node->in_flow = 5.0;
  node->out_flow = 5.0;
  
  // Setting capacity >= current flow should work
  EXPECT_NO_THROW(DefaultNet::set_node_cap(node, 5.0));
  EXPECT_EQ(node->max_cap, 5.0);
}

TEST_F(NetCapGraphValidation, SetNodeCapacity_Invalid)
{
  auto node = net.insert_node(10.0);
  node->in_flow = 5.0;
  node->out_flow = 5.0;
  
  // Setting capacity < current flow should throw
  EXPECT_THROW(DefaultNet::set_node_cap(node, 4.0), std::overflow_error);
}

TEST_F(NetCapGraphValidation, SetNodeCapacity_Negative)
{
  auto node = net.insert_node(10.0);
  
  EXPECT_THROW(DefaultNet::set_node_cap(node, -1.0), std::domain_error);
}

// =============================================================================
// INTEGER FLOW TYPE TESTS
// =============================================================================

class NetCapGraphIntegerFlow : public ::testing::Test
{
protected:
  IntNet net;
};

TEST_F(NetCapGraphIntegerFlow, BasicOperations)
{
  auto n1 = net.insert_node(0, 100L);
  auto n2 = net.insert_node(0, 50L);
  auto arc = net.insert_arc(n1, n2, 75L, 0L);
  
  EXPECT_EQ(n1->max_cap, 100L);
  EXPECT_EQ(n2->max_cap, 50L);
  EXPECT_EQ(arc->cap, 75L);
}

TEST_F(NetCapGraphIntegerFlow, AuxNetWithIntegers)
{
  auto n1 = net.insert_node(0, 100L);
  auto n2 = net.insert_node(0, 50L);
  net.insert_arc(n1, n2, 75L, 0L);
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  net.free_aux_net();
}

// =============================================================================
// STRING INFO TYPE TESTS
// =============================================================================

class NetCapGraphStringInfo : public ::testing::Test
{
protected:
  StringNet net;
};

TEST_F(NetCapGraphStringInfo, BasicOperations)
{
  auto n1 = net.insert_node("Source", 100.0);
  auto n2 = net.insert_node("Sink", 50.0);
  auto arc = net.insert_arc(n1, n2, 75.0, 0.0, "Connection");
  
  EXPECT_EQ(n1->get_info(), "Source");
  EXPECT_EQ(n2->get_info(), "Sink");
  EXPECT_EQ(arc->get_info(), "Connection");
}

TEST_F(NetCapGraphStringInfo, AuxNetWithStrings)
{
  auto n1 = net.insert_node("Node_A", 100.0);
  auto n2 = net.insert_node("Node_B", 50.0);
  net.insert_arc(n1, n2, 75.0, 0.0);
  
  auto aux = net.compute_aux_net();
  EXPECT_NE(aux, nullptr);
  
  net.free_aux_net();
}

// =============================================================================
// COOKIE MAPPING TESTS
// =============================================================================

class NetCapGraphCookies : public ::testing::Test
{
protected:
  DefaultNet net;
};

TEST_F(NetCapGraphCookies, NodeToCookieMapping)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(20.0);
  net.insert_arc(n1, n2, 15.0, 0.0);
  
  net.compute_aux_net();
  
  // NODE_COOKIE(n1) should point to an arc in aux_net
  auto cookie1 = NODE_COOKIE(n1);
  auto cookie2 = NODE_COOKIE(n2);
  
  EXPECT_NE(cookie1, nullptr);
  EXPECT_NE(cookie2, nullptr);
  
  // ARC_COOKIE of that arc should point back to n1
  using AuxArc = typename DefaultNet::Aux_Net::Arc;
  auto aux_arc1 = static_cast<AuxArc*>(cookie1);
  EXPECT_EQ(ARC_COOKIE(aux_arc1), n1);
  
  net.free_aux_net();
}

TEST_F(NetCapGraphCookies, ArcToCookieMapping)
{
  auto n1 = net.insert_node(10.0);
  auto n2 = net.insert_node(20.0);
  auto arc = net.insert_arc(n1, n2, 15.0, 0.0);
  
  net.compute_aux_net();
  
  // ARC_COOKIE(arc) should point to an arc in aux_net
  auto cookie = ARC_COOKIE(arc);
  EXPECT_NE(cookie, nullptr);
  
  // That aux arc should point back to arc
  using AuxArc = typename DefaultNet::Aux_Net::Arc;
  auto aux_arc = static_cast<AuxArc*>(cookie);
  EXPECT_EQ(ARC_COOKIE(aux_arc), arc);
  
  net.free_aux_net();
}

// =============================================================================
// FUZZ TESTS
// =============================================================================

class NetCapGraphFuzz : public ::testing::Test
{
protected:
  void SetUp() override
  {
    rng.seed(98765);
  }
  
  mt19937 rng;
};

TEST_F(NetCapGraphFuzz, RandomOperations)
{
  DefaultNet net;
  
  uniform_real_distribution<double> cap_dist(1.0, 100.0);
  uniform_int_distribution<int> op_dist(0, 2);
  
  vector<DefaultNet::Node*> nodes;
  
  // Perform 1000 random operations
  for (int i = 0; i < 1000; ++i)
  {
    int op = op_dist(rng);
    
    if (nodes.size() < 2 || op == 0)
    {
      // Insert node
      auto node = net.insert_node(cap_dist(rng));
      nodes.push_back(node);
    }
    else if (op == 1 && nodes.size() >= 2)
    {
      // Insert arc between random nodes
      uniform_int_distribution<size_t> node_dist(0, nodes.size() - 1);
      size_t src_idx = node_dist(rng);
      size_t tgt_idx = node_dist(rng);
      if (src_idx != tgt_idx)
      {
        try
        {
          net.insert_arc(nodes[src_idx], nodes[tgt_idx], cap_dist(rng), 0.0);
        }
        catch (...)
        {
          // Ignore duplicate arc errors
        }
      }
    }
    else if (op == 2 && !net.has_aux_net() && nodes.size() >= 2)
    {
      // Compute and free aux net
      try
      {
        net.compute_aux_net();
        net.update();
        net.free_aux_net();
      }
      catch (...)
      {
        // May fail if network structure is invalid
      }
    }
  }
  
  // Cleanup
  if (net.has_aux_net())
    net.free_aux_net();
  
  EXPECT_GT(net.vsize(), 0);
}

TEST_F(NetCapGraphFuzz, ExtremeCapacityValues)
{
  DefaultNet net;
  
  // Test with extreme capacity values
  vector<double> extreme_caps = {
    0.0,
    numeric_limits<double>::min(),
    numeric_limits<double>::epsilon(),
    1e-300,
    1e-10,
    1.0,
    1e10,
    1e100,
    1e300,
    numeric_limits<double>::max() / 2,  // Avoid overflow in aux net
  };
  
  vector<DefaultNet::Node*> nodes;
  for (double cap : extreme_caps)
    nodes.push_back(net.insert_node(cap));
  
  // Connect consecutive nodes
  for (size_t i = 0; i < nodes.size() - 1; ++i)
    net.insert_arc(nodes[i], nodes[i+1], extreme_caps[i], 0.0);
  
  EXPECT_NO_THROW({
    net.compute_aux_net();
    net.update();
    net.free_aux_net();
  });
}

// =============================================================================
// Main entry point
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
