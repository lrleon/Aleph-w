
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
 * @file graph_allocation_failure_test.cc
 * @brief Insertions and removals in graphs when memory runs out.
 *
 * This program replaces the global operator new, so that a test can make
 * one allocation fail. If insert_arc(), remove_node() or, in a network,
 * remove_arc() or disconnect_arc() throws std::bad_alloc, the graph must
 * be as it was: no node may keep an arc that the graph freed.
 */

#include <gtest/gtest.h>

#include "allocation_failure.H"

#include <algorithm>
#include <new>
#include <vector>

#include <tpl_agraph.H>
#include <tpl_cut_nodes.H>
#include <tpl_graph.H>
#include <tpl_graph_utils.H>
#include <tpl_net.H>
#include <tpl_sgraph.H>

using namespace Aleph;

namespace
{
  // The tests need the operator new of allocation_failure.H
  template <class G>
  struct GraphWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  using Graph_Kinds =
    ::testing::Types<List_Graph<Graph_Node<int>, Graph_Arc<int>>,
                     List_Digraph<Graph_Node<int>, Graph_Arc<int>>,
                     List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
                     List_SDigraph<Graph_Snode<int>, Graph_Sarc<int>>,
                     Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>,
                     Array_Digraph<Graph_Anode<int>, Graph_Aarc<int>>>;

  // The arcs of g, and of each node, are the expected ones; the walk over
  // the adjacency of the nodes reads each arc, so a freed one is seen
  template <class G>
  void expect_arcs(const G & g, const std::vector<typename G::Arc *> & arcs)
  {
    ASSERT_EQ(g.get_num_arcs(), arcs.size());
    size_t in_graph = 0;
    for (typename G::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr_ne();
        EXPECT_NE(std::find(arcs.begin(), arcs.end(), a), arcs.end());
        ++in_graph;
      }
    EXPECT_EQ(in_graph, arcs.size());

    for (typename G::Node_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr_ne();
        size_t adjacent = 0, sum = 0;
        for (typename G::Node_Arc_Iterator j(p); j.has_curr(); j.next_ne())
          {
            auto a = j.get_curr_ne();
            EXPECT_NE(std::find(arcs.begin(), arcs.end(), a), arcs.end());
            sum += size_t(a->get_info());
            ++adjacent;
          }
        EXPECT_EQ(adjacent, g.get_num_arcs(p));
        EXPECT_GE(sum, adjacent);
      }
  }
}

TYPED_TEST_SUITE(GraphWithoutMemory, Graph_Kinds);

// List_SGraph::insert_arc() put the arc in the list of its source, then in
// that of its target and in the set of arcs; when one of them could not
// take it, the others kept it, and the arc, freed when insert_arc() threw,
// was left in the graph
TYPED_TEST(GraphWithoutMemory, InsertArcLeavesTheGraphAsItWas)
{
  using G = TypeParam;
  for (bool loop : {false, true})
    for (size_t k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "allocation " << k
                     << (loop ? ", a loop" : ""));
        G g;
        auto a = g.insert_node(1);
        auto b = g.insert_node(2);
        std::vector<typename G::Arc *> arcs = {g.insert_arc(a, b, 1),
                                               g.insert_arc(b, a, 2)};

        typename G::Arc * arc = nullptr;
        bool thrown = false;
        {
          Fail_At failing(k);
          try
            {
              arc = g.insert_arc(b, loop ? b : a, 3);
            }
          catch (const std::bad_alloc &)
            {
              thrown = true;
            }
        }

        if (not thrown)
          {
            arcs.push_back(arc);
            expect_arcs(g, arcs);
            break;
          }
        expect_arcs(g, arcs);

        // and it takes the arc with memory
        arcs.push_back(g.insert_arc(b, loop ? b : a, 3));
        expect_arcs(g, arcs);
      }
}

// remove_node() was noexcept, though it makes a list of the arcs of the
// node, so the program ended without memory for it; that of List_SGraph
// made the list while it disconnected the arcs, so it would have left some
// out of the graph and still in its set of arcs
TYPED_TEST(GraphWithoutMemory, RemoveNodeLeavesTheGraphAsItWasOrRemovesIt)
{
  using G = TypeParam;
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      G g;
      auto a = g.insert_node(1);
      auto b = g.insert_node(2);
      auto c = g.insert_node(3);
      std::vector<typename G::Arc *> arcs = {g.insert_arc(a, b, 1),
                                             g.insert_arc(b, c, 2),
                                             g.insert_arc(c, b, 3),
                                             g.insert_arc(c, a, 4),
                                             g.insert_arc(b, b, 5)};
      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            g.remove_node(b);
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }

      if (thrown)
        {
          EXPECT_EQ(g.get_num_nodes(), 3u);
          expect_arcs(g, arcs);
          continue;
        }

      EXPECT_EQ(g.get_num_nodes(), 2u);
      expect_arcs(g, {arcs[3]});
      break;
    }
}

namespace
{
  using Net = Net_Graph<Net_Node<int>, Net_Arc<int, int>>;

  // The sets of sources and of sinks of net are those of its arcs
  void expect_sources_and_sinks(const Net & net)
  {
    for (Net::Node_Iterator it(net); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr_ne();
        EXPECT_EQ(net.is_source(p), net.get_in_degree(p) == 0);
        EXPECT_EQ(net.is_sink(p), net.get_out_degree(p) == 0);
      }
  }

  struct NetWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };
}

// remove_arc() and disconnect_arc() put in the set of sinks the source of
// the arc after the arc was gone, so without memory for it the network had
// a sink that was not in its set; disconnect_arc() was noexcept, too
TEST_F(NetWithoutMemory, ArcsGoOnlyIfTheSetsOfSourcesAndSinksTakeTheirEnds)
{
  for (int how = 0; how < 3; ++how) // remove_arc(), disconnect_arc(), remove_node()
    for (size_t k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "way " << how << ", allocation " << k);
        Net net;
        auto s = net.insert_node(0);
        auto a = net.insert_node(1);
        auto t = net.insert_node(2);
        auto sa = net.insert_arc(s, a, 3, 0);
        net.insert_arc(a, t, 2, 0);
        net.insert_arc(s, t, 1, 0);
        bool thrown = false;
        {
          Fail_At failing(k);
          try
            {
              if (how == 0)
                net.remove_arc(sa);
              else if (how == 1)
                net.disconnect_arc(sa);
              else
                net.remove_node(a);
            }
          catch (const std::bad_alloc &)
            {
              thrown = true;
            }
        }
        expect_sources_and_sinks(net);
        if (how == 1 and not thrown)
          net.connect_arc(sa);
        if (not thrown)
          break;
        if (how < 2)
          EXPECT_EQ(net.get_num_arcs(), 3u);
        else // the arcs of a that went are gone, and a is still there
          EXPECT_EQ(net.get_num_nodes(), 3u);
      }
}

// clear_graph() was noexcept, though a network puts in its sets of sources
// and sinks the ends of the arcs that it removes, so that without memory
// for them the program ended
TEST_F(NetWithoutMemory, ClearGraphThrowsAndLeavesTheNetworkRight)
{
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      Net net;
      auto s = net.insert_node(0);
      auto a = net.insert_node(1);
      auto t = net.insert_node(2);
      net.insert_arc(s, a, 3, 0);
      net.insert_arc(a, t, 2, 0);
      net.insert_arc(s, t, 1, 0);
      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            clear_graph(net);
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }
      expect_sources_and_sinks(net);
      if (not thrown)
        {
          EXPECT_EQ(net.get_num_arcs(), 0u);
          EXPECT_EQ(net.get_num_nodes(), 0u);
          break;
        }
      EXPECT_EQ(net.get_num_nodes(), 3u); // the nodes go after the arcs
      clear_graph(net);
      EXPECT_EQ(net.get_num_nodes(), 0u);
    }
}

// The copy assignment emptied the graph before it copied the other, so
// without memory for the copy the graph lost what it had
TYPED_TEST(GraphWithoutMemory, CopyAssignmentLeavesTheGraphAsItWas)
{
  using G = TypeParam;
  G other;
  auto x = other.insert_node(10);
  auto y = other.insert_node(20);
  other.insert_arc(x, y, 7);
  other.insert_arc(y, y, 8);

  int mark = 0;
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      G g;
      auto a = g.insert_node(1);
      auto b = g.insert_node(2);
      std::vector<typename G::Arc *> arcs = {g.insert_arc(a, b, 1)};
      g.get_cookie() = &mark;

      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            g = other;
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }

      EXPECT_EQ(g.get_cookie(), &mark);
      EXPECT_EQ(g.is_digraph(), other.is_digraph());
      if (thrown)
        {
          EXPECT_EQ(g.get_num_nodes(), 2u);
          expect_arcs(g, arcs);
          continue;
        }

      EXPECT_EQ(g.get_num_nodes(), 2u);
      EXPECT_EQ(g.get_num_arcs(), 2u);
      int sum = 0;
      for (typename G::Arc_Iterator it(g); it.has_curr(); it.next_ne())
        sum += it.get_curr_ne()->get_info();
      EXPECT_EQ(sum, 15);
      break;
    }
}

// List_SGraph::insert_node() counted the node before it put it in its set
// of nodes, so without memory for the set the graph counted a node that it
// did not have, and the node was lost
TYPED_TEST(GraphWithoutMemory, InsertNodeLeavesTheGraphAsItWas)
{
  using G = TypeParam;
  for (size_t k = 1; ; ++k)
    {
      SCOPED_TRACE(testing::Message() << "allocation " << k);
      G g;
      g.insert_node(1);
      bool thrown = false;
      {
        Fail_At failing(k);
        try
          {
            g.insert_node(2);
          }
        catch (const std::bad_alloc &)
          {
            thrown = true;
          }
      }

      size_t nodes = 0;
      for (typename G::Node_Iterator it(g); it.has_curr(); it.next_ne())
        ++nodes;
      EXPECT_EQ(nodes, g.get_num_nodes());
      if (not thrown)
        {
          EXPECT_EQ(nodes, 2u);
          break;
        }
      EXPECT_EQ(nodes, 1u);
    }
}

namespace
{
  // The tests need the operator new of allocation_failure.H
  template <class G>
  struct CopiesWithoutMemory : public ::testing::Test
  {
    void SetUp() override
    {
      if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
        GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";
    }
  };

  using Undirected_Kinds =
    ::testing::Types<List_Graph<Graph_Node<int>, Graph_Arc<int>>,
                     List_SGraph<Graph_Snode<int>, Graph_Sarc<int>>,
                     Array_Graph<Graph_Anode<int>, Graph_Aarc<int>>>;

  // Two triangles joined at the node 2, the second one with a tail: the
  // cut nodes are 2 and 4
  template <class G>
  std::vector<typename G::Node *> bow(G & g)
  {
    std::vector<typename G::Node *> v;
    for (int i = 0; i < 6; ++i)
      v.push_back(g.insert_node(i));

    const int arcs[][2] = {{0, 1}, {1, 2}, {2, 0}, {2, 3}, {3, 4}, {4, 2}, {4, 5}};
    int k = 1;
    for (const auto & a : arcs)
      g.insert_arc(v[a[0]], v[a[1]], k++);
    return v;
  }

  // Runs copy failing at its allocations, one after the other, until it
  // does not throw
  template <class Copy>
  void fail_each_allocation(Copy copy)
  {
    for (size_t k = 1; ; ++k)
      {
        SCOPED_TRACE(testing::Message() << "allocation " << k);
        bool thrown = false;
        {
          Fail_At failing(k);
          try
            {
              copy();
            }
          catch (const std::bad_alloc &)
            {
              thrown = true;
            }
        }
        if (not thrown)
          return;
      }
  }
}

TYPED_TEST_SUITE(CopiesWithoutMemory, Undirected_Kinds);

// The copies kept each new node in a unique_ptr while they inserted it,
// but List_SGraph::insert_node() deletes the node that it cannot take, so
// that the node was deleted twice; Array_Graph::insert_node() did not
// delete it, so that the nodes that insert_node(info) made were lost.
// AddressSanitizer stops at a node deleted twice, and LeakSanitizer
// reports a lost one
TYPED_TEST(CopiesWithoutMemory, EachNodeIsDeletedOnce)
{
  using G = TypeParam;
  G g;
  auto v = bow(g);
  auto cuts = compute_cut_nodes(g, v[0]);
  ASSERT_EQ(cuts.size(), 2u);
  paint_subgraphs(g, cuts);
  const long color = NODE_COUNTER(v[0]);

  fail_each_allocation([&] { G tree = find_breadth_first_spanning_tree(g, v[0]); });
  fail_each_allocation([&] { G block = map_subgraph(g, color); });
  fail_each_allocation([&] { auto copies = map_cut_graph(g, cuts); });

  Compute_Cut_Nodes<G> cc(g);
  DynDlist<typename G::Node *> cut_list;
  cc(v[0], cut_list);
  cc.paint_subgraphs();
  fail_each_allocation([&]
    {
      G block;
      cc.map_subgraph(block, NODE_COUNTER(v[0]));
    });
  fail_each_allocation([&]
    {
      G cut_graph;
      DynDlist<typename G::Arc *> cross;
      cc.map_cut_graph(cut_graph, cross);
    });

  fail_each_allocation([&] { (void) g.insert_node(7); });
}
