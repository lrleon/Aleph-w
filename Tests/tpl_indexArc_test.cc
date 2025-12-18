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
 * @file tpl_indexArc_test.cc
 * @brief Tests for tpl_indexArc.H (IndexArc)
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include <tpl_graph.H>
#include <tpl_indexArc.H>

using namespace Aleph;

namespace
{
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using DGraph = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  struct EvenArcInfo
  {
    bool operator()(UGraph::Arc* a) const noexcept
    {
      return (a->get_info() % 2) == 0;
    }

    bool operator()(const UGraph&, UGraph::Arc* a) const noexcept
    {
      return (*this)(a);
    }
  };
}

TEST(IndexArcUndirected, SearchIsSymmetric)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 10);

  IndexArc<UGraph> idx(g, true);

  EXPECT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.search(n1, n2), a);
  EXPECT_EQ(idx.search(n2, n1), a);
  EXPECT_EQ(idx.search_directed(n1, n2), a);
  EXPECT_EQ(idx.search_directed(n2, n1), a);
}

TEST(IndexArcDirected, SearchRespectsDirection)
{
  DGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a12 = g.insert_arc(n1, n2, 10);

  IndexArc<DGraph> idx(g, true);

  EXPECT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.search(n1, n2), a12);
  EXPECT_EQ(idx.search(n2, n1), nullptr);

  auto a21 = g.insert_arc(n2, n1, 20);
  idx.insert(a21);
  EXPECT_EQ(idx.size(), 2u);
  EXPECT_EQ(idx.search(n2, n1), a21);
}

TEST(IndexArc, InsertDoesNotDuplicateSamePointer)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto a = g.insert_arc(n1, n2, 1);

  IndexArc<UGraph> idx(g, false);
  EXPECT_EQ(idx.size(), 0u);

  EXPECT_EQ(idx.insert(a), a);
  EXPECT_EQ(idx.size(), 1u);

  EXPECT_EQ(idx.insert(a), a);
  EXPECT_EQ(idx.size(), 1u);
}

TEST(IndexArc, InsertDuplicateEndpointsReturnsExisting)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  IndexArc<UGraph> idx(g, false);

  // Insert a dummy arc that is not part of the graph, but has the same endpoints.
  auto* dummy = new UGraph::Arc(0);
  dummy->src_node = n1;
  dummy->tgt_node = n2;
  EXPECT_EQ(idx.insert(dummy), dummy);
  EXPECT_EQ(idx.size(), 1u);

  auto a = g.insert_arc(n1, n2, 123);
  EXPECT_EQ(idx.insert(a), dummy);
  EXPECT_EQ(idx.size(), 1u);

  idx.remove(dummy);
  delete dummy;
}

TEST(IndexArc, InsertInGraphAndRemoveFromGraph)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);

  IndexArc<UGraph> idx(g, true);
  EXPECT_EQ(idx.size(), 0u);

  auto a = idx.insert_in_graph(n1, n2, 7);
  EXPECT_EQ(g.get_num_arcs(), 1u);
  EXPECT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.search(n1, n2), a);

  EXPECT_THROW(idx.insert_in_graph(n1, n2, 8), std::domain_error);

  idx.remove_from_graph(a);
  EXPECT_EQ(g.get_num_arcs(), 0u);
  EXPECT_EQ(idx.size(), 0u);
  EXPECT_EQ(idx.search(n1, n2), nullptr);
}

TEST(IndexArc, ClearIndexAndRebuild)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  auto a12 = g.insert_arc(n1, n2, 1);
  auto a23 = g.insert_arc(n2, n3, 2);
  (void)a12;
  (void)a23;

  IndexArc<UGraph> idx(g, true);
  EXPECT_EQ(idx.size(), g.get_num_arcs());

  idx.clear_index();
  EXPECT_EQ(idx.size(), 0u);

  idx.build_index();
  EXPECT_EQ(idx.size(), g.get_num_arcs());
}

TEST(IndexArc, ConstructorWithFilterOnlyIndexesMatchingArcs)
{
  UGraph g;
  auto n1 = g.insert_node(1);
  auto n2 = g.insert_node(2);
  auto n3 = g.insert_node(3);

  auto a12 = g.insert_arc(n1, n2, 1); // odd
  auto a23 = g.insert_arc(n2, n3, 2); // even

  IndexArc<UGraph, Rand_Tree, EvenArcInfo> idx(g, true, EvenArcInfo{});

  EXPECT_EQ(idx.size(), 1u);
  EXPECT_EQ(idx.search(n2, n3), a23);
  EXPECT_EQ(idx.search(n1, n2), nullptr);

  // But the arc is in the graph.
  EXPECT_EQ(Aleph::search_arc(g, n1, n2), a12);
}
