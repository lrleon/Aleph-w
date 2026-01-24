
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
 * @file graph_indexes_test.cc
 * @brief Tests for Graph Indexes
 */

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
