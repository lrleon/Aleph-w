#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <tpl_graph_indexes.H>

using namespace Aleph;

namespace
{
using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
using DGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;
} // namespace

TEST(NodesIndex, InsertSearchRemoveByInfo)
{
  UGraph g;
  Nodes_Index<UGraph> index(g);

  auto * n10 = index.insert_in_graph(10);
  ASSERT_NE(n10, nullptr);

  auto * n20 = index.insert_in_graph(20);
  ASSERT_NE(n20, nullptr);

  ASSERT_EQ(index.search(10), n10);
  ASSERT_EQ(index.search(20), n20);
  ASSERT_EQ(index.search(30), nullptr);

  index.remove_from_graph(n10);
  ASSERT_EQ(index.search(10), nullptr);
  ASSERT_EQ(index.search(20), n20);
}

TEST(NodesIndex, SearchOrInsertDoesNotDuplicate)
{
  UGraph g;
  Nodes_Index<UGraph> index(g);

  auto * n10_a = index.search_or_insert_in_graph(10);
  auto * n10_b = index.search_or_insert_in_graph(10);
  ASSERT_EQ(n10_a, n10_b);
}

TEST(ArcsIndex, DirectedSearchByEndpointsAndInfo)
{
  DGraph g;
  Arcs_Index<DGraph> arcs(g);

  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);

  auto * a12 = arcs.insert_in_graph(n1, n2, 5);
  ASSERT_NE(a12, nullptr);

  ASSERT_EQ(arcs.search(n1, n2, 5), a12);
  ASSERT_EQ(arcs.search(n2, n1, 5), nullptr);
}

TEST(ArcsIndex, UndirectedSearchFindsReverse)
{
  UGraph g;
  Arcs_Index<UGraph> arcs(g);

  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);

  auto * a12 = arcs.insert_in_graph(n1, n2, 5);
  ASSERT_NE(a12, nullptr);

  ASSERT_EQ(arcs.search(n1, n2, 5), a12);
  ASSERT_EQ(arcs.search(n2, n1, 5), a12);
}

TEST(ArcsIndex, RemoveFromGraphRemovesFromIndex)
{
  UGraph g;
  Arcs_Index<UGraph> arcs(g);

  auto * n1 = g.insert_node(1);
  auto * n2 = g.insert_node(2);

  auto * a12 = arcs.insert_in_graph(n1, n2, 5);
  ASSERT_NE(a12, nullptr);
  ASSERT_EQ(arcs.search(n1, n2, 5), a12);

  arcs.remove_from_graph(a12);
  ASSERT_EQ(arcs.search(n1, n2, 5), nullptr);
}
