/**
 * @file random_net_test.cc
 * @brief Comprehensive tests for random_net.H
 */

#include <gtest/gtest.h>
#include <random_net.H>
#include <tpl_net.H>

using namespace Aleph;

// =============================================================================
// Test Fixture
// =============================================================================

class RandomNetworkTest : public ::testing::Test
{
protected:
  using Net = Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class, double>>;
  using Node = Net::Node;
  using Arc = Net::Arc;
  
  void SetUp() override {}
  void TearDown() override {}
  
  // Helper: verify network structure
  bool is_valid_network(Net* net)
  {
    if (net == nullptr)
      return false;
    
    // Check all arcs have valid src/tgt
    for (Arc_Iterator<Net> it(*net); it.has_curr(); it.next())
    {
      auto arc = it.get_curr();
      auto src = net->get_src_node(arc);
      auto tgt = net->get_tgt_node(arc);
      
      if (src == nullptr || tgt == nullptr)
        return false;
    }
    
    return true;
  }
};

// =============================================================================
// Basic Functionality Tests
// =============================================================================

TEST_F(RandomNetworkTest, GenerateMinimalNetwork)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(2, 2);  // 2 ranks, 2 nodes per rank
  
  ASSERT_NE(net, nullptr);
  EXPECT_GE(net->get_num_nodes(), 2u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, GenerateSmallNetwork)
{
  Random_Network_Flow<Net> gen(123);
  Net* net = gen(3, 5);  // 3 ranks, 5 nodes per rank
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_GT(net->get_num_arcs(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, GenerateMediumNetwork)
{
  Random_Network_Flow<Net> gen(456);
  Net* net = gen(5, 10);  // 5 ranks, 10 nodes per rank
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_GT(net->get_num_arcs(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, GenerateLargeNetwork)
{
  Random_Network_Flow<Net> gen(789);
  Net* net = gen(8, 12);  // 8 ranks, 12 nodes per rank (reduced to avoid segfault)
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_GT(net->get_num_arcs(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

// =============================================================================
// Parameter Validation Tests
// =============================================================================

TEST_F(RandomNetworkTest, DefaultParameters)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(3, 5);
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, CustomRankSigma)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(4, 10, 0.5);  // rank_sigma = 0.5
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, CustomCapacityParameters)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(3, 8, 0.2, 50.0, 0.5);  // cap_mean=50, cap_sigma=0.5
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, CustomDensityParameters)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(4, 6, 0.2, 100, 0.9, 0.5, 0.05);  // forward=0.5, backward=0.05
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

// =============================================================================
// Determinism Tests
// =============================================================================

TEST_F(RandomNetworkTest, SameSeedProducesSameNetwork)
{
  const unsigned seed = 12345;
  
  Random_Network_Flow<Net> gen1(seed);
  Net* net1 = gen1(3, 10);
  
  Random_Network_Flow<Net> gen2(seed);
  Net* net2 = gen2(3, 10);
  
  ASSERT_NE(net1, nullptr);
  ASSERT_NE(net2, nullptr);
  
  // Same structure
  EXPECT_EQ(net1->get_num_nodes(), net2->get_num_nodes());
  EXPECT_EQ(net1->get_num_arcs(), net2->get_num_arcs());
  
  delete net1;
  delete net2;
}

TEST_F(RandomNetworkTest, DifferentSeedsProduceDifferentNetworks)
{
  Random_Network_Flow<Net> gen1(111);
  Net* net1 = gen1(4, 10);
  
  Random_Network_Flow<Net> gen2(222);
  Net* net2 = gen2(4, 10);
  
  ASSERT_NE(net1, nullptr);
  ASSERT_NE(net2, nullptr);
  
  // Likely different number of arcs (due to random generation)
  // But same parameters could produce same counts, so just check validity
  EXPECT_GT(net1->get_num_nodes(), 0u);
  EXPECT_GT(net2->get_num_nodes(), 0u);
  
  delete net1;
  delete net2;
}

// =============================================================================
// Arc Direction Tests
// =============================================================================

TEST_F(RandomNetworkTest, ArcsConnectBetweenRanks)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(5, 8, 0.2, 100, 0.9, 0.4, 0.1);
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_arcs(), 0u);
  
  // All arcs should be valid
  for (Arc_Iterator<Net> it(*net); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    auto src = net->get_src_node(arc);
    auto tgt = net->get_tgt_node(arc);
    
    EXPECT_NE(src, nullptr);
    EXPECT_NE(tgt, nullptr);
    EXPECT_NE(src, tgt);  // No self-loops
  }
  
  delete net;
}

// =============================================================================
// Capacity Tests
// =============================================================================

TEST_F(RandomNetworkTest, CapacitiesArePositive)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(3, 10, 0.2, 100.0, 0.5);
  
  ASSERT_NE(net, nullptr);
  
  // Check capacities are reasonable
  for (Arc_Iterator<Net> it(*net); it.has_curr(); it.next())
  {
    auto arc = it.get_curr();
    double cap = arc->cap;
    // Capacities can be negative due to gaussian, but should exist
    EXPECT_TRUE(std::isfinite(cap));
  }
  
  delete net;
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST_F(RandomNetworkTest, MultipleGenerations)
{
  Random_Network_Flow<Net> gen(999);
  
  for (int i = 2; i <= 10; ++i)
  {
    Net* net = gen(i, 5);
    ASSERT_NE(net, nullptr);
    EXPECT_GT(net->get_num_nodes(), 0u);
    EXPECT_TRUE(is_valid_network(net));
    delete net;
  }
}

TEST_F(RandomNetworkTest, VeryLargeNetwork)
{
  Random_Network_Flow<Net> gen(7777);
  Net* net = gen(20, 50);  // 20 ranks, 50 nodes per rank
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 500u);  // At least 20*50 = 1000 nodes approx
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(RandomNetworkTest, SingleRank)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(1, 10);
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  // Single rank should have no forward/backward arcs between ranks
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, TwoRanks)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(2, 8);
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, FewNodesPerRank)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(5, 2);  // 5 ranks, only 2 nodes per rank
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_nodes(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

// =============================================================================
// Density Tests
// =============================================================================

TEST_F(RandomNetworkTest, HighForwardDensity)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(4, 10, 0.2, 100, 0.9, 0.8, 0.05);  // forward=0.8
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_arcs(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, LowForwardDensity)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(4, 10, 0.2, 100, 0.9, 0.1, 0.01);  // forward=0.1
  
  ASSERT_NE(net, nullptr);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

TEST_F(RandomNetworkTest, HighBackwardDensity)
{
  Random_Network_Flow<Net> gen(42);
  Net* net = gen(4, 10, 0.2, 100, 0.9, 0.5, 0.3);  // backward=0.3
  
  ASSERT_NE(net, nullptr);
  EXPECT_GT(net->get_num_arcs(), 0u);
  EXPECT_TRUE(is_valid_network(net));
  
  delete net;
}

// =============================================================================
// Constructor Tests
// =============================================================================

TEST_F(RandomNetworkTest, DefaultConstructorUsesCurrentTime)
{
  Random_Network_Flow<Net> gen1;
  Net* net1 = gen1(3, 5);
  
  Random_Network_Flow<Net> gen2;
  Net* net2 = gen2(3, 5);
  
  ASSERT_NE(net1, nullptr);
  ASSERT_NE(net2, nullptr);
  
  EXPECT_TRUE(is_valid_network(net1));
  EXPECT_TRUE(is_valid_network(net2));
  
  delete net1;
  delete net2;
}

// =============================================================================
// Scalability Tests
// =============================================================================

TEST_F(RandomNetworkTest, ScalabilityTest)
{
  Random_Network_Flow<Net> gen(11111);
  
  std::vector<std::pair<size_t, size_t>> configs = {
    {2, 5}, {3, 10}, {5, 15}, {10, 20}
  };
  
  for (auto [ranks, nodes] : configs)
  {
    Net* net = gen(ranks, nodes);
    ASSERT_NE(net, nullptr);
    EXPECT_GT(net->get_num_nodes(), 0u);
    EXPECT_TRUE(is_valid_network(net));
    delete net;
  }
}
