#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <warshall.H>

using namespace Aleph;

using TestGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;

class WarshallTest : public ::testing::Test
{
protected:
  TestGraph g;
  Bit_Mat_Graph<TestGraph> mat;
  Warshall_Compute_Transitive_Clausure<TestGraph> warshall;
};

TEST_F(WarshallTest, EmptyGraph)
{
  EXPECT_NO_THROW(warshall_compute_transitive_clausure(g, mat));
  EXPECT_EQ(mat.get_num_nodes(), 0u);
}

TEST_F(WarshallTest, SingleNode)
{
  g.insert_node(0);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), 1u);
  EXPECT_EQ(mat(0L, 0L), 0);
}

TEST_F(WarshallTest, SingleNodeWithSelfLoop)
{
  auto n0 = g.insert_node(0);
  g.insert_arc(n0, n0, 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), 1u);
  EXPECT_EQ(mat(0L, 0L), 1);
}

TEST_F(WarshallTest, TwoNodesConnected)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  g.insert_arc(n0, n1, 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), 2u);
  EXPECT_EQ(mat(0L, 1L), 1);
  EXPECT_EQ(mat(1L, 0L), 1);
}

TEST_F(WarshallTest, TwoNodesDisconnected)
{
  g.insert_node(0);
  g.insert_node(1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), 2u);
  EXPECT_EQ(mat(0L, 1L), 0);
  EXPECT_EQ(mat(1L, 0L), 0);
  EXPECT_EQ(mat(0L, 0L), 0);
  EXPECT_EQ(mat(1L, 1L), 0);
}

TEST_F(WarshallTest, SimpleChain)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n3, 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat(0L, 1L), 1);
  EXPECT_EQ(mat(1L, 2L), 1);
  EXPECT_EQ(mat(2L, 3L), 1);

  EXPECT_EQ(mat(0L, 2L), 1);
  EXPECT_EQ(mat(0L, 3L), 1);
  EXPECT_EQ(mat(1L, 3L), 1);
}

TEST_F(WarshallTest, Triangle)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n2, n0, 1);

  warshall_compute_transitive_clausure(g, mat);

  for (long i = 0; i < 3; ++i)
    for (long j = 0; j < 3; ++j)
      if (i != j)
        EXPECT_EQ(mat(i, j), 1);
}

TEST_F(WarshallTest, CompleteGraph)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n0, n3, 1);
  g.insert_arc(n1, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n2, n3, 1);

  warshall_compute_transitive_clausure(g, mat);

  for (long i = 0; i < 4; ++i)
    for (long j = 0; j < 4; ++j)
      if (i != j)
        EXPECT_EQ(mat(i, j), 1);
}

TEST_F(WarshallTest, DisconnectedComponents)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n2, n3, 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), 4u);

  EXPECT_EQ(mat(n0, n1), 1);
  EXPECT_EQ(mat(n1, n0), 1);
  EXPECT_EQ(mat(n2, n3), 1);
  EXPECT_EQ(mat(n3, n2), 1);

  EXPECT_EQ(mat(n0, n2), 0);
  EXPECT_EQ(mat(n0, n3), 0);
  EXPECT_EQ(mat(n1, n2), 0);
  EXPECT_EQ(mat(n1, n3), 0);
  EXPECT_EQ(mat(n2, n0), 0);
  EXPECT_EQ(mat(n2, n1), 0);
  EXPECT_EQ(mat(n3, n0), 0);
  EXPECT_EQ(mat(n3, n1), 0);
}

TEST_F(WarshallTest, DiamondGraph)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n0, n2, 1);
  g.insert_arc(n1, n3, 1);
  g.insert_arc(n2, n3, 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat(0L, 1L), 1);
  EXPECT_EQ(mat(0L, 2L), 1);
  EXPECT_EQ(mat(0L, 3L), 1);
  EXPECT_EQ(mat(1L, 3L), 1);
  EXPECT_EQ(mat(2L, 3L), 1);
}

TEST_F(WarshallTest, LargeGraph)
{
  constexpr int N = 50;
  std::vector<TestGraph::Node *> nodes;

  for (int i = 0; i < N; ++i)
    nodes.push_back(g.insert_node(i));

  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], 1);

  warshall_compute_transitive_clausure(g, mat);

  EXPECT_EQ(mat.get_num_nodes(), static_cast<size_t>(N));
  EXPECT_EQ(mat(0L, static_cast<long>(N - 1)), 1);
  EXPECT_EQ(mat(static_cast<long>(N - 1), 0L), 1);
}

TEST_F(WarshallTest, ClassWrapper)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);

  warshall(g, mat);

  EXPECT_EQ(mat(0L, 1L), 1);
  EXPECT_EQ(mat(1L, 2L), 1);
  EXPECT_EQ(mat(0L, 2L), 1);
}

TEST_F(WarshallTest, Idempotence)
{
  auto n0 = g.insert_node(0);
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  g.insert_arc(n0, n1, 1);
  g.insert_arc(n1, n2, 1);

  Bit_Mat_Graph<TestGraph> mat1;
  Bit_Mat_Graph<TestGraph> mat2;

  warshall_compute_transitive_clausure(g, mat1);
  warshall_compute_transitive_clausure(g, mat2);

  for (size_t i = 0; i < mat1.get_num_nodes(); ++i)
    for (size_t j = 0; j < mat1.get_num_nodes(); ++j)
      EXPECT_EQ(mat1(static_cast<long>(i), static_cast<long>(j)),
                mat2(static_cast<long>(i), static_cast<long>(j)));
}

TEST_F(WarshallTest, StarGraph)
{
  auto center = g.insert_node(0);
  std::vector<TestGraph::Node *> leaves;

  for (int i = 1; i <= 5; ++i)
    {
      auto leaf = g.insert_node(i);
      leaves.push_back(leaf);
      g.insert_arc(center, leaf, 1);
    }

  warshall_compute_transitive_clausure(g, mat);

  for (long i = 1; i <= 5; ++i)
    EXPECT_EQ(mat(0L, i), 1);

  for (long i = 1; i <= 5; ++i)
    {
      EXPECT_EQ(mat(i, 0L), 1);
      for (long j = 1; j <= 5; ++j)
        if (i != j)
          EXPECT_EQ(mat(i, j), 1);
    }
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
