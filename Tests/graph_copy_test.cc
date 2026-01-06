/**
 * @file graph_copy_test.cc
 * @brief Tests for GraphCopyWithMapping class.
 *
 * Tests cover:
 * - Basic copy and mapping functionality
 * - Node lookup (get_copy, search_copy, has_copy)
 * - Unmapped node insertion (for auxiliary nodes)
 * - Edge cases: empty graph, single node, disconnected components
 * - Different graph types: directed and undirected
 *
 * @author Generated for Aleph-w library
 */

#include <gtest/gtest.h>
#include <tpl_graph.H>
#include <tpl_graph_copy.H>
#include <vector>

using namespace Aleph;

// ==================== Type definitions ====================

struct IntNode : public Graph_Node<int>
{
  using Graph_Node<int>::Graph_Node;
};

struct DoubleArc : public Graph_Arc<double>
{
  using Graph_Arc<double>::Graph_Arc;
};

using TestGraph = List_Graph<IntNode, DoubleArc>;
using TestDigraph = List_Digraph<IntNode, DoubleArc>;

// ==================== Test Fixtures ====================

class GraphCopyTest : public ::testing::Test
{
protected:
  TestGraph g;
  TestDigraph dg;
  std::vector<TestGraph::Node*> nodes;
  std::vector<TestDigraph::Node*> dnodes;
  
  void SetUp() override
  {
    nodes.clear();
    dnodes.clear();
  }
};

// ==================== Basic Tests ====================

TEST_F(GraphCopyTest, EmptyGraph)
{
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 0);
  EXPECT_EQ(copy.num_arcs(), 0);
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 0);
  EXPECT_EQ(copy.get_graph().get_num_arcs(), 0);
}

TEST_F(GraphCopyTest, SingleNode)
{
  auto* n = g.insert_node(42);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 1);
  EXPECT_EQ(copy.num_arcs(), 0);
  
  auto* copy_n = copy.get_copy(n);
  ASSERT_NE(copy_n, nullptr);
  EXPECT_EQ(copy_n->get_info(), 42);
  EXPECT_NE(copy_n, n);  // Should be a different pointer
}

TEST_F(GraphCopyTest, TwoNodesOneArc)
{
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 3.14);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 2);
  EXPECT_EQ(copy.num_arcs(), 1);
  
  auto* c1 = copy.get_copy(n1);
  auto* c2 = copy.get_copy(n2);
  
  EXPECT_EQ(c1->get_info(), 1);
  EXPECT_EQ(c2->get_info(), 2);
  
  // Verify arc exists in copy
  bool found_arc = false;
  for (auto it = copy.get_graph().get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = copy.get_graph().get_src_node(arc);
    auto* tgt = copy.get_graph().get_tgt_node(arc);
    if ((src == c1 && tgt == c2) || (src == c2 && tgt == c1))
    {
      found_arc = true;
      EXPECT_DOUBLE_EQ(arc->get_info(), 3.14);
    }
  }
  EXPECT_TRUE(found_arc);
}

TEST_F(GraphCopyTest, DirectedGraph)
{
  auto* n1 = dg.insert_node(1);
  auto* n2 = dg.insert_node(2);
  dg.insert_arc(n1, n2, 1.0);
  dg.insert_arc(n2, n1, 2.0);
  
  GraphCopyWithMapping<TestDigraph> copy(dg);
  
  EXPECT_EQ(copy.num_nodes(), 2);
  EXPECT_EQ(copy.num_arcs(), 2);
  
  auto* c1 = copy.get_copy(n1);
  auto* c2 = copy.get_copy(n2);
  
  // Verify both directed arcs
  int arc_count = 0;
  for (auto it = copy.get_graph().get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = copy.get_graph().get_src_node(arc);
    auto* tgt = copy.get_graph().get_tgt_node(arc);
    
    if (src == c1 && tgt == c2)
    {
      EXPECT_DOUBLE_EQ(arc->get_info(), 1.0);
      arc_count++;
    }
    else if (src == c2 && tgt == c1)
    {
      EXPECT_DOUBLE_EQ(arc->get_info(), 2.0);
      arc_count++;
    }
  }
  EXPECT_EQ(arc_count, 2);
}

// ==================== Lookup Tests ====================

TEST_F(GraphCopyTest, GetCopyThrowsOnInvalidNode)
{
  auto* n1 = g.insert_node(1);
  GraphCopyWithMapping<TestGraph> copy(g);
  
  // Create a node not in the original graph
  TestGraph other;
  auto* other_node = other.insert_node(999);
  
  EXPECT_THROW(copy.get_copy(other_node), std::domain_error);
  
  // Original node should work
  EXPECT_NO_THROW(copy.get_copy(n1));
}

TEST_F(GraphCopyTest, SearchCopyReturnsNullOnInvalidNode)
{
  auto* n1 = g.insert_node(1);
  GraphCopyWithMapping<TestGraph> copy(g);
  
  TestGraph other;
  auto* other_node = other.insert_node(999);
  
  EXPECT_EQ(copy.search_copy(other_node), nullptr);
  EXPECT_NE(copy.search_copy(n1), nullptr);
}

TEST_F(GraphCopyTest, HasCopy)
{
  auto* n1 = g.insert_node(1);
  GraphCopyWithMapping<TestGraph> copy(g);
  
  TestGraph other;
  auto* other_node = other.insert_node(999);
  
  EXPECT_TRUE(copy.has_copy(n1));
  EXPECT_FALSE(copy.has_copy(other_node));
}

// ==================== Unmapped Node Tests ====================

TEST_F(GraphCopyTest, InsertUnmappedNode)
{
  auto* n1 = g.insert_node(1);
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 1);
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 1);
  
  // Insert unmapped node (like dummy node in Johnson)
  auto* dummy = copy.insert_unmapped_node(999);
  
  // Mapping count doesn't change
  EXPECT_EQ(copy.num_nodes(), 1);
  // But graph has more nodes
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 2);
  
  EXPECT_EQ(dummy->get_info(), 999);
}

TEST_F(GraphCopyTest, InsertArcToUnmappedNode)
{
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  auto* c1 = copy.get_copy(n1);
  auto* dummy = copy.insert_unmapped_node(0);
  
  // Connect dummy to all nodes (like Johnson's algorithm)
  copy.insert_arc(dummy, c1, 0.0);
  copy.insert_arc(dummy, copy.get_copy(n2), 0.0);
  
  EXPECT_EQ(copy.get_graph().get_num_arcs(), 2);
}

TEST_F(GraphCopyTest, RemoveUnmappedNode)
{
  auto* n1 = g.insert_node(1);
  GraphCopyWithMapping<TestGraph> copy(g);
  
  auto* dummy = copy.insert_unmapped_node(999);
  auto* c1 = copy.get_copy(n1);
  copy.insert_arc(dummy, c1, 0.0);
  
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 2);
  EXPECT_EQ(copy.get_graph().get_num_arcs(), 1);
  
  copy.remove_node(dummy);
  
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 1);
  EXPECT_EQ(copy.get_graph().get_num_arcs(), 0);
  
  // Original mapping still works
  EXPECT_EQ(copy.get_copy(n1), c1);
}

// ==================== Complex Graph Tests ====================

TEST_F(GraphCopyTest, TriangleGraph)
{
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);
  g.insert_arc(n1, n2, 1.0);
  g.insert_arc(n2, n3, 2.0);
  g.insert_arc(n3, n1, 3.0);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 3);
  EXPECT_EQ(copy.num_arcs(), 3);
  
  auto* c1 = copy.get_copy(n1);
  auto* c2 = copy.get_copy(n2);
  auto* c3 = copy.get_copy(n3);
  
  // All should be different
  EXPECT_NE(c1, c2);
  EXPECT_NE(c2, c3);
  EXPECT_NE(c1, c3);
  
  // Values should match
  EXPECT_EQ(c1->get_info(), 1);
  EXPECT_EQ(c2->get_info(), 2);
  EXPECT_EQ(c3->get_info(), 3);
}

TEST_F(GraphCopyTest, DisconnectedComponents)
{
  // Component 1
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 1.0);
  
  // Component 2 (isolated)
  auto* n3 = g.insert_node(3);
  auto* n4 = g.insert_node(4);
  g.insert_arc(n3, n4, 2.0);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 4);
  EXPECT_EQ(copy.num_arcs(), 2);
  
  // All nodes should be mapped
  EXPECT_TRUE(copy.has_copy(n1));
  EXPECT_TRUE(copy.has_copy(n2));
  EXPECT_TRUE(copy.has_copy(n3));
  EXPECT_TRUE(copy.has_copy(n4));
}

TEST_F(GraphCopyTest, SelfLoop)
{
  auto* n1 = dg.insert_node(1);
  dg.insert_arc(n1, n1, 5.0);
  
  GraphCopyWithMapping<TestDigraph> copy(dg);
  
  EXPECT_EQ(copy.num_nodes(), 1);
  EXPECT_EQ(copy.num_arcs(), 1);
  
  auto* c1 = copy.get_copy(n1);
  
  // Verify self-loop
  bool found = false;
  for (auto it = copy.get_graph().get_arc_it(); it.has_curr(); it.next())
  {
    auto* arc = it.get_curr();
    auto* src = copy.get_graph().get_src_node(arc);
    auto* tgt = copy.get_graph().get_tgt_node(arc);
    if (src == c1 && tgt == c1)
    {
      found = true;
      EXPECT_DOUBLE_EQ(arc->get_info(), 5.0);
    }
  }
  EXPECT_TRUE(found);
}

TEST_F(GraphCopyTest, ParallelArcs)
{
  auto* n1 = dg.insert_node(1);
  auto* n2 = dg.insert_node(2);
  dg.insert_arc(n1, n2, 1.0);
  dg.insert_arc(n1, n2, 2.0);
  dg.insert_arc(n1, n2, 3.0);
  
  GraphCopyWithMapping<TestDigraph> copy(dg);
  
  EXPECT_EQ(copy.num_nodes(), 2);
  EXPECT_EQ(copy.num_arcs(), 3);
}

// ==================== ForEach Tests ====================

TEST_F(GraphCopyTest, ForEachMapping)
{
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  auto* n3 = g.insert_node(3);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  int count = 0;
  int sum_orig = 0;
  int sum_copy = 0;
  
  copy.for_each_mapping([&](TestGraph::Node* orig, TestGraph::Node* cp) {
    count++;
    sum_orig += orig->get_info();
    sum_copy += cp->get_info();
  });
  
  EXPECT_EQ(count, 3);
  EXPECT_EQ(sum_orig, 6);  // 1+2+3
  EXPECT_EQ(sum_copy, 6);  // Same values in copy
}

// ==================== Integration with Algorithms ====================

TEST_F(GraphCopyTest, CopyPreservesStructureForAlgorithms)
{
  // Create a graph suitable for shortest path algorithms
  auto* s = dg.insert_node(0);  // source
  auto* a = dg.insert_node(1);
  auto* b = dg.insert_node(2);
  auto* t = dg.insert_node(3);  // target
  
  dg.insert_arc(s, a, 1.0);
  dg.insert_arc(s, b, 4.0);
  dg.insert_arc(a, b, 2.0);
  dg.insert_arc(a, t, 6.0);
  dg.insert_arc(b, t, 3.0);
  
  GraphCopyWithMapping<TestDigraph> copy(dg);
  
  // Verify structure is preserved
  EXPECT_EQ(copy.get_graph().get_num_nodes(), dg.get_num_nodes());
  EXPECT_EQ(copy.get_graph().get_num_arcs(), dg.get_num_arcs());
  
  // Get copied nodes for algorithm use
  auto* cs = copy.get_copy(s);
  auto* ct = copy.get_copy(t);
  
  EXPECT_EQ(cs->get_info(), 0);
  EXPECT_EQ(ct->get_info(), 3);
  
  // Verify out-degree is preserved
  int orig_out_degree = 0;
  for (auto it = dg.get_out_it(s); it.has_curr(); it.next())
    orig_out_degree++;
  
  int copy_out_degree = 0;
  for (auto it = copy.get_graph().get_out_it(cs); it.has_curr(); it.next())
    copy_out_degree++;
  
  EXPECT_EQ(orig_out_degree, copy_out_degree);
}

TEST_F(GraphCopyTest, LargerGraph)
{
  // Create a larger graph to stress test
  const int N = 100;
  std::vector<TestDigraph::Node*> ns;
  
  for (int i = 0; i < N; ++i)
    ns.push_back(dg.insert_node(i));
  
  // Create some edges
  for (int i = 0; i < N - 1; ++i)
    dg.insert_arc(ns[i], ns[i + 1], static_cast<double>(i));
  
  // Add some cross edges
  for (int i = 0; i < N - 10; i += 10)
    dg.insert_arc(ns[i], ns[i + 10], static_cast<double>(i * 10));
  
  GraphCopyWithMapping<TestDigraph> copy(dg);
  
  EXPECT_EQ(copy.num_nodes(), N);
  
  // Verify all nodes are mapped correctly
  for (int i = 0; i < N; ++i)
  {
    auto* cp = copy.get_copy(ns[i]);
    ASSERT_NE(cp, nullptr);
    EXPECT_EQ(cp->get_info(), i);
  }
}

// ==================== Clear Test ====================

TEST_F(GraphCopyTest, Clear)
{
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  g.insert_arc(n1, n2, 1.0);
  
  GraphCopyWithMapping<TestGraph> copy(g);
  
  EXPECT_EQ(copy.num_nodes(), 2);
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 2);
  
  copy.clear();
  
  EXPECT_EQ(copy.num_nodes(), 0);
  EXPECT_EQ(copy.get_graph().get_num_nodes(), 0);
  EXPECT_EQ(copy.get_graph().get_num_arcs(), 0);
}

// ==================== Main ====================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

