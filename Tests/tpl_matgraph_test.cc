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
 * @file tpl_matgraph_test.cc
 * @brief Comprehensive tests for adjacency matrix graph classes
 */

#include <gtest/gtest.h>
#include <tpl_matgraph.H>
#include <tpl_graph.H>
#include <limits>

using namespace Aleph;

// =============================================================================
// Type Aliases
// =============================================================================

// Graph with int node attributes and int arc attributes
using IntGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using IntDigraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
using StringGraph = List_Graph<Graph_Node<std::string>, Graph_Arc<double>>;

// =============================================================================
// Helper Functions
// =============================================================================

class MatGraphTestBase : public ::testing::Test
{
protected:
  IntGraph g;
  IntGraph::Node* n0 = nullptr;
  IntGraph::Node* n1 = nullptr;
  IntGraph::Node* n2 = nullptr;
  IntGraph::Node* n3 = nullptr;

  void SetUp() override
  {
    // Create a simple graph:
    // 0 -- 1 -- 2
    //      |
    //      3
    n0 = g.insert_node(0);
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);
    n3 = g.insert_node(3);

    g.insert_arc(n0, n1, 10);
    g.insert_arc(n1, n2, 20);
    g.insert_arc(n1, n3, 30);
  }
};

class DigraphTestBase : public ::testing::Test
{
protected:
  IntDigraph g;
  IntDigraph::Node* n0 = nullptr;
  IntDigraph::Node* n1 = nullptr;
  IntDigraph::Node* n2 = nullptr;

  void SetUp() override
  {
    // Create a simple digraph:
    // 0 -> 1 -> 2
    //      ^    |
    //      |____|
    n0 = g.insert_node(0);
    n1 = g.insert_node(1);
    n2 = g.insert_node(2);

    g.insert_arc(n0, n1, 10);
    g.insert_arc(n1, n2, 20);
    g.insert_arc(n2, n1, 30);
  }
};

// =============================================================================
// matgraph_detail Helper Tests
// =============================================================================

TEST(MatgraphDetailTest, IndexArray)
{
  // Row-major indexing: i + j*n
  EXPECT_EQ(matgraph_detail::index_array(0L, 0L, 4L), 0L);
  EXPECT_EQ(matgraph_detail::index_array(1L, 0L, 4L), 1L);
  EXPECT_EQ(matgraph_detail::index_array(0L, 1L, 4L), 4L);
  EXPECT_EQ(matgraph_detail::index_array(3L, 3L, 4L), 15L);
}

TEST(MatgraphDetailTest, CheckedIndexArrayValid)
{
  EXPECT_NO_THROW((void)matgraph_detail::checked_index_array(0L, 0L, 4L));
  EXPECT_NO_THROW((void)matgraph_detail::checked_index_array(3L, 3L, 4L));
}

TEST(MatgraphDetailTest, CheckedIndexArrayOutOfRange)
{
  EXPECT_THROW((void)matgraph_detail::checked_index_array(4L, 0L, 4L), std::out_of_range);
  EXPECT_THROW((void)matgraph_detail::checked_index_array(0L, 4L, 4L), std::out_of_range);
  EXPECT_THROW((void)matgraph_detail::checked_index_array(-1L, 0L, 4L), std::out_of_range);
}

// =============================================================================
// Map_Matrix_Graph Tests
// =============================================================================

class MapMatrixGraphTest : public MatGraphTestBase
{
};

TEST_F(MapMatrixGraphTest, Constructor)
{
  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 4u);
}

TEST_F(MapMatrixGraphTest, GetNodeByIndex)
{
  Map_Matrix_Graph<IntGraph> mat(g);

  // Nodes are sorted by pointer, so order may vary
  EXPECT_NE(mat(0), nullptr);
  EXPECT_NE(mat(1), nullptr);
  EXPECT_NE(mat(2), nullptr);
  EXPECT_NE(mat(3), nullptr);
}

TEST_F(MapMatrixGraphTest, GetIndexByNode)
{
  Map_Matrix_Graph<IntGraph> mat(g);

  // Get indices and verify they're valid
  long idx0 = mat(n0);
  long idx1 = mat(n1);
  long idx2 = mat(n2);
  long idx3 = mat(n3);

  EXPECT_GE(idx0, 0);
  EXPECT_LT(idx0, 4);
  EXPECT_GE(idx1, 0);
  EXPECT_LT(idx1, 4);
  EXPECT_GE(idx2, 0);
  EXPECT_LT(idx2, 4);
  EXPECT_GE(idx3, 0);
  EXPECT_LT(idx3, 4);

  // All indices should be unique
  std::set<long> indices{idx0, idx1, idx2, idx3};
  EXPECT_EQ(indices.size(), 4u);
}

TEST_F(MapMatrixGraphTest, GetArcByIndices)
{
  Map_Matrix_Graph<IntGraph> mat(g);

  long i0 = mat(n0);
  long i1 = mat(n1);
  long i2 = mat(n2);
  long i3 = mat(n3);

  // Arc between n0 and n1 should exist (undirected)
  EXPECT_NE(mat(i0, i1), nullptr);
  EXPECT_NE(mat(i1, i0), nullptr);  // Undirected

  // Arc between n1 and n2
  EXPECT_NE(mat(i1, i2), nullptr);

  // No direct arc between n0 and n2
  EXPECT_EQ(mat(i0, i2), nullptr);
  EXPECT_EQ(mat(i2, i0), nullptr);

  // No arc between n0 and n3
  EXPECT_EQ(mat(i0, i3), nullptr);
}

TEST_F(MapMatrixGraphTest, GetArcByNodes)
{
  Map_Matrix_Graph<IntGraph> mat(g);

  EXPECT_NE(mat(n0, n1), nullptr);
  EXPECT_NE(mat(n1, n0), nullptr);
  EXPECT_NE(mat(n1, n2), nullptr);
  EXPECT_NE(mat(n1, n3), nullptr);
  EXPECT_EQ(mat(n0, n2), nullptr);
  EXPECT_EQ(mat(n0, n3), nullptr);
}

TEST_F(MapMatrixGraphTest, GetListGraph)
{
  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_EQ(&mat.get_list_graph(), &g);
}

TEST_F(MapMatrixGraphTest, CopyConstructor)
{
  Map_Matrix_Graph<IntGraph> mat1(g);
  Map_Matrix_Graph<IntGraph> mat2(mat1);

  EXPECT_EQ(mat2.get_num_nodes(), mat1.get_num_nodes());
  EXPECT_EQ(&mat2.get_list_graph(), &mat1.get_list_graph());
}

TEST_F(MapMatrixGraphTest, AssignmentOperator)
{
  Map_Matrix_Graph<IntGraph> mat1(g);
  IntGraph g2;
  g2.insert_node(100);
  g2.insert_node(200);
  Map_Matrix_Graph<IntGraph> mat2(g2);

  mat2 = mat1;
  EXPECT_EQ(mat2.get_num_nodes(), 4u);
}

TEST_F(MapMatrixGraphTest, OutOfRangeIndex)
{
  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_THROW((void)mat(10), std::out_of_range);
}

// =============================================================================
// Matrix_Graph Tests
// =============================================================================

class MatrixGraphTest : public MatGraphTestBase
{
};

TEST_F(MatrixGraphTest, Constructor)
{
  Matrix_Graph<IntGraph> mat(g, -1);
  EXPECT_EQ(mat.get_num_nodes(), 4u);
  EXPECT_EQ(mat.null_value(), -1);
}

TEST_F(MatrixGraphTest, GetNodeAttribute)
{
  Matrix_Graph<IntGraph> mat(g, -1);

  // Node attributes are stored by index
  std::set<int> values;
  for (size_t i = 0; i < 4; ++i)
    values.insert(mat(static_cast<long>(i)));

  // Should have node values 0, 1, 2, 3
  EXPECT_TRUE(values.count(0));
  EXPECT_TRUE(values.count(1));
  EXPECT_TRUE(values.count(2));
  EXPECT_TRUE(values.count(3));
}

TEST_F(MatrixGraphTest, GetArcAttribute)
{
  Matrix_Graph<IntGraph> mat(g, -1);

  // Find indices of nodes
  long idx0 = -1, idx1 = -1;
  for (size_t i = 0; i < 4; ++i)
    {
      if (mat(static_cast<long>(i)) == 0) idx0 = i;
      if (mat(static_cast<long>(i)) == 1) idx1 = i;
    }

  ASSERT_NE(idx0, -1);
  ASSERT_NE(idx1, -1);

  // Arc between nodes 0 and 1 has weight 10
  EXPECT_EQ(mat(idx0, idx1), 10);
  EXPECT_EQ(mat(idx1, idx0), 10);  // Undirected
}

TEST_F(MatrixGraphTest, NoArcReturnsNull)
{
  Matrix_Graph<IntGraph> mat(g, -1);

  long idx0 = -1, idx2 = -1;
  for (size_t i = 0; i < 4; ++i)
    {
      if (mat(static_cast<long>(i)) == 0) idx0 = i;
      if (mat(static_cast<long>(i)) == 2) idx2 = i;
    }

  EXPECT_EQ(mat(idx0, idx2), -1);
}

TEST_F(MatrixGraphTest, ModifyEntry)
{
  Matrix_Graph<IntGraph> mat(g, -1);

  mat(0, 3) = 999;
  EXPECT_EQ(mat(0, 3), 999);
}

TEST_F(MatrixGraphTest, CopyConstructor)
{
  Matrix_Graph<IntGraph> mat1(g, -1);
  Matrix_Graph<IntGraph> mat2(mat1);

  EXPECT_EQ(mat2.get_num_nodes(), mat1.get_num_nodes());
  EXPECT_EQ(mat2.null_value(), mat1.null_value());
}

// =============================================================================
// Ady_Mat Tests
// =============================================================================

class AdyMatTest : public DigraphTestBase
{
};

TEST_F(AdyMatTest, ConstructorWithoutNull)
{
  Ady_Mat<IntDigraph, double> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 3u);
}

TEST_F(AdyMatTest, ConstructorWithNull)
{
  Ady_Mat<IntDigraph, double> mat(g, -1.0);
  EXPECT_EQ(mat.get_num_nodes(), 3u);
  EXPECT_DOUBLE_EQ(mat.null_value(), -1.0);
}

TEST_F(AdyMatTest, GetNodeByIndex)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);

  EXPECT_NE(mat(0), nullptr);
  EXPECT_NE(mat(1), nullptr);
  EXPECT_NE(mat(2), nullptr);
}

TEST_F(AdyMatTest, GetIndexByNode)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);

  long i0 = mat(n0);
  long i1 = mat(n1);
  long i2 = mat(n2);

  EXPECT_GE(i0, 0);
  EXPECT_LT(i0, 3);
  EXPECT_GE(i1, 0);
  EXPECT_LT(i1, 3);
  EXPECT_GE(i2, 0);
  EXPECT_LT(i2, 3);
}

TEST_F(AdyMatTest, AccessByIndices)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);

  mat(0L, 1L) = 100;
  mat(1L, 2L) = 200;

  EXPECT_EQ(mat(0L, 1L), 100);
  EXPECT_EQ(mat(1L, 2L), 200);
}

TEST_F(AdyMatTest, AccessByNodes)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);

  mat(n0, n1) = 100;
  EXPECT_EQ(mat(n0, n1), 100);
}

TEST_F(AdyMatTest, ConstAccess)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);
  mat(0L, 0L) = 42;

  const Ady_Mat<IntDigraph, int>& cmat = mat;
  EXPECT_EQ(cmat(0L, 0L), 42);
}

TEST_F(AdyMatTest, GetListGraph)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);
  EXPECT_EQ(&mat.get_list_graph(), &g);
}

TEST_F(AdyMatTest, SetNullValue)
{
  Ady_Mat<IntDigraph, int> mat(g);
  mat.set_null_value(-999);
  EXPECT_EQ(mat.null_value(), -999);
}

// Operation functor for operate_all_arcs_list_graph
struct InitFromArc
{
  void operator()(Ady_Mat<IntDigraph, int>& mat,
                  IntDigraph::Arc* arc,
                  long i, long j,
                  int& entry)
  {
    (void)mat;
    (void)i;
    (void)j;
    entry = arc->get_info();
  }
};

TEST_F(AdyMatTest, OperateAllArcsListGraph)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);
  mat.operate_all_arcs_list_graph<InitFromArc>();

  // Check that arc values were copied
  long i0 = mat(n0), i1 = mat(n1), i2 = mat(n2);

  EXPECT_EQ(mat(i0, i1), 10);  // Arc 0->1
  EXPECT_EQ(mat(i1, i2), 20);  // Arc 1->2
  EXPECT_EQ(mat(i2, i1), 30);  // Arc 2->1
}

// Operation functor for operate_all_arcs_matrix
struct InitAll
{
  void operator()(Ady_Mat<IntDigraph, int>& mat,
                  IntDigraph::Node* src, IntDigraph::Node* tgt,
                  long i, long j,
                  int& entry)
  {
    (void)mat;
    (void)src;
    (void)tgt;
    entry = i * 10 + j;
  }
};

TEST_F(AdyMatTest, OperateAllArcsMatrix)
{
  Ady_Mat<IntDigraph, int> mat(g, -1);
  mat.operate_all_arcs_matrix<InitAll>();

  EXPECT_EQ(mat(0L, 0L), 0);
  EXPECT_EQ(mat(0L, 1L), 1);
  EXPECT_EQ(mat(1L, 0L), 10);
  EXPECT_EQ(mat(2L, 2L), 22);
}

// =============================================================================
// Bit_Mat_Graph Tests
// =============================================================================

class BitMatGraphTest : public MatGraphTestBase
{
};

TEST_F(BitMatGraphTest, DefaultConstructor)
{
  Bit_Mat_Graph<IntGraph> mat;
  EXPECT_EQ(mat.get_num_nodes(), 0u);
  EXPECT_EQ(mat.get_list_graph(), nullptr);
}

TEST_F(BitMatGraphTest, ConstructFromGraph)
{
  Bit_Mat_Graph<IntGraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 4u);
  EXPECT_EQ(mat.get_list_graph(), &g);
}

TEST_F(BitMatGraphTest, ConstructWithDimension)
{
  Bit_Mat_Graph<IntGraph> mat(5);
  EXPECT_EQ(mat.get_num_nodes(), 5u);
  EXPECT_EQ(mat.get_list_graph(), nullptr);
}

TEST_F(BitMatGraphTest, GetNodeByIndex)
{
  Bit_Mat_Graph<IntGraph> mat(g);

  EXPECT_NE(mat(0), nullptr);
  EXPECT_NE(mat(1), nullptr);
}

TEST_F(BitMatGraphTest, GetIndexByNode)
{
  Bit_Mat_Graph<IntGraph> mat(g);

  long i0 = mat(n0);
  long i1 = mat(n1);

  EXPECT_GE(i0, 0);
  EXPECT_LT(i0, 4);
  EXPECT_GE(i1, 0);
  EXPECT_LT(i1, 4);
  EXPECT_NE(i0, i1);
}

TEST_F(BitMatGraphTest, CheckConnectivity)
{
  Bit_Mat_Graph<IntGraph> mat(g);

  long i0 = mat(n0), i1 = mat(n1), i2 = mat(n2), i3 = mat(n3);

  // Arc between n0 and n1
  EXPECT_EQ(static_cast<int>(mat(i0, i1)), 1);
  EXPECT_EQ(static_cast<int>(mat(i1, i0)), 1);

  // Arc between n1 and n2
  EXPECT_EQ(static_cast<int>(mat(i1, i2)), 1);

  // Arc between n1 and n3
  EXPECT_EQ(static_cast<int>(mat(i1, i3)), 1);

  // No arc between n0 and n2
  EXPECT_EQ(static_cast<int>(mat(i0, i2)), 0);
}

TEST_F(BitMatGraphTest, ModifyEntries)
{
  Bit_Mat_Graph<IntGraph> mat(g);

  long i0 = mat(n0), i2 = mat(n2);

  EXPECT_EQ(static_cast<int>(mat(i0, i2)), 0);

  mat(i0, i2) = 1;
  EXPECT_EQ(static_cast<int>(mat(i0, i2)), 1);

  mat(i0, i2) = 0;
  EXPECT_EQ(static_cast<int>(mat(i0, i2)), 0);
}

TEST_F(BitMatGraphTest, AccessByNodes)
{
  Bit_Mat_Graph<IntGraph> mat(g);

  EXPECT_EQ(static_cast<int>(mat(n0, n1)), 1);
  EXPECT_EQ(static_cast<int>(mat(n0, n2)), 0);
}

TEST_F(BitMatGraphTest, SetListGraph)
{
  Bit_Mat_Graph<IntGraph> mat;
  EXPECT_EQ(mat.get_num_nodes(), 0u);

  mat.set_list_graph(g);
  EXPECT_EQ(mat.get_num_nodes(), 4u);
}

TEST_F(BitMatGraphTest, CopyConstructor)
{
  Bit_Mat_Graph<IntGraph> mat1(g);
  Bit_Mat_Graph<IntGraph> mat2(mat1);

  EXPECT_EQ(mat2.get_num_nodes(), mat1.get_num_nodes());

  long i0 = mat1(n0), i1 = mat1(n1);
  EXPECT_EQ(static_cast<int>(mat2(i0, i1)), static_cast<int>(mat1(i0, i1)));
}

TEST_F(BitMatGraphTest, AssignmentOperator)
{
  Bit_Mat_Graph<IntGraph> mat1(g);
  Bit_Mat_Graph<IntGraph> mat2;

  mat2 = mat1;
  EXPECT_EQ(mat2.get_num_nodes(), 4u);
}

TEST_F(BitMatGraphTest, NoGraphThrows)
{
  Bit_Mat_Graph<IntGraph> mat;

  EXPECT_THROW((void)mat(0), std::domain_error);
  EXPECT_THROW((void)mat(n0), std::domain_error);
}

// =============================================================================
// Digraph Tests
// =============================================================================

class DigraphMatrixTest : public DigraphTestBase
{
};

TEST_F(DigraphMatrixTest, MapMatrixDirected)
{
  Map_Matrix_Graph<IntDigraph> mat(g);

  long i0 = mat(n0), i1 = mat(n1), i2 = mat(n2);

  // 0 -> 1 exists
  EXPECT_NE(mat(i0, i1), nullptr);
  // 1 -> 0 does not exist (directed)
  EXPECT_EQ(mat(i1, i0), nullptr);

  // 1 -> 2 exists
  EXPECT_NE(mat(i1, i2), nullptr);
  // 2 -> 1 exists
  EXPECT_NE(mat(i2, i1), nullptr);
}

TEST_F(DigraphMatrixTest, BitMatDirected)
{
  Bit_Mat_Graph<IntDigraph> mat(g);

  long i0 = mat(n0), i1 = mat(n1), i2 = mat(n2);

  EXPECT_EQ(static_cast<int>(mat(i0, i1)), 1);
  EXPECT_EQ(static_cast<int>(mat(i1, i0)), 0);
  EXPECT_EQ(static_cast<int>(mat(i1, i2)), 1);
  EXPECT_EQ(static_cast<int>(mat(i2, i1)), 1);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(MatGraphEdgeCases, SingleNodeGraph)
{
  IntGraph g;
  g.insert_node(42);

  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 1u);
  EXPECT_EQ(mat(0L, 0L), nullptr);  // No self-loop

  Ady_Mat<IntGraph, int> amat(g, -1);
  EXPECT_EQ(amat.get_num_nodes(), 1u);
  amat(0L, 0L) = 100;
  EXPECT_EQ(amat(0L, 0L), 100);
}

TEST(MatGraphEdgeCases, DisconnectedGraph)
{
  IntGraph g;
  auto* n0 = g.insert_node(0);
  auto* n1 = g.insert_node(1);
  auto* n2 = g.insert_node(2);
  (void)n0; (void)n1; (void)n2;
  // No arcs

  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 3u);

  for (long i = 0; i < 3; ++i)
    for (long j = 0; j < 3; ++j)
      EXPECT_EQ(mat(i, j), nullptr);
}

TEST(MatGraphEdgeCases, CompleteGraph)
{
  IntGraph g;
  IntGraph::Node* nodes[4];
  for (int i = 0; i < 4; ++i)
    nodes[i] = g.insert_node(i);

  // Add all edges
  for (int i = 0; i < 4; ++i)
    for (int j = i + 1; j < 4; ++j)
      g.insert_arc(nodes[i], nodes[j], i * 10 + j);

  Map_Matrix_Graph<IntGraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), 4u);

  // All pairs should have arcs (undirected)
  for (long i = 0; i < 4; ++i)
    for (long j = 0; j < 4; ++j)
      if (i != j)
        EXPECT_NE(mat(i, j), nullptr);
}

TEST(MatGraphEdgeCases, SelfLoop)
{
  IntDigraph g;
  auto* n = g.insert_node(0);
  g.insert_arc(n, n, 99);  // Self-loop

  Map_Matrix_Graph<IntDigraph> mat(g);
  long idx = mat(n);
  EXPECT_NE(mat(idx, idx), nullptr);
  EXPECT_EQ(mat(idx, idx)->get_info(), 99);
}

// =============================================================================
// Performance / Stress Tests
// =============================================================================

TEST(MatGraphStress, LargeGraph)
{
  constexpr int N = 100;

  IntDigraph g;
  std::vector<IntDigraph::Node*> nodes(N);

  for (int i = 0; i < N; ++i)
    nodes[i] = g.insert_node(i);

  // Create chain: 0 -> 1 -> 2 -> ... -> N-1
  for (int i = 0; i < N - 1; ++i)
    g.insert_arc(nodes[i], nodes[i + 1], i);

  Map_Matrix_Graph<IntDigraph> mat(g);
  EXPECT_EQ(mat.get_num_nodes(), static_cast<size_t>(N));

  Ady_Mat<IntDigraph, int> amat(g, -1);
  amat.operate_all_arcs_list_graph<InitFromArc>();

  // Verify chain structure
  for (int i = 0; i < N - 1; ++i)
    {
      long si = amat(nodes[i]);
      long ti = amat(nodes[i + 1]);
      EXPECT_EQ(amat(si, ti), i);
    }
}

int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
