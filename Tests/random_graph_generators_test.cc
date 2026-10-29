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
 * @file random_graph_generators_test.cc
 * @brief The generators of random_graph.H: one graph after another, and
 * the Eulerian graphs.
 *
 * Each graph that a generator gives is its own graph moved out, and the
 * rest of its state stayed: a graph of an undirected type lost the flag
 * that said that it was directed, the parities of the nodes of the former
 * graphs went into the next Eulerian ones, the tracking of the parities
 * that eulerian() started stayed on, and a graph that threw halfway stayed
 * in the generator, which then removed it as an undirected graph.
 *
 * And the Eulerian graphs: a digraph took the node between the ends of an
 * arc only among its balanced nodes, which could be none, and an
 * undirected graph whose odd nodes were all adjacent had no end.
 */

#include <gtest/gtest.h>

#include <stdexcept>

#include <random_graph.H>
#include <tpl_dynMapTree.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph.H>

using namespace Aleph;

namespace
{
  using Graph = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
  using Digraph_Type = List_Digraph<Graph_Node<int>, Graph_Arc<int>>;

  // The ends of each arc are nodes of g: a node of another graph that a
  // generator kept was taken as an end
  template <class GT>
  bool arcs_inside(const GT & g)
  {
    DynSetTree<typename GT::Node *> nodes;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      nodes.insert(it.get_curr_ne());
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr_ne();
        if (not nodes.contains(g.get_src_node(a)) or
            not nodes.contains(g.get_tgt_node(a)))
          return false;
      }
    return true;
  }

  // A directed arc is only in its source
  template <class GT>
  bool linked_as_digraph(const GT & g)
  {
    size_t in_nodes = 0;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      in_nodes += g.get_num_arcs(it.get_curr_ne());
    return g.is_digraph() and in_nodes == g.get_num_arcs();
  }

  template <class GT>
  bool all_degrees_even(const GT & g)
  {
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      if (g.get_num_arcs(it.get_curr_ne()) % 2 != 0)
        return false;
    return true;
  }

  template <class GT>
  DynMapTree<typename GT::Node *, size_t> in_degrees(const GT & g)
  {
    DynMapTree<typename GT::Node *, size_t> in;
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      in.insert(it.get_curr_ne(), 0);
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      ++in.find(g.get_tgt_node(it.get_curr_ne()));
    return in;
  }

  // The in-degree of each node is its out-degree
  template <class GT>
  bool balanced(const GT & g)
  {
    auto in = in_degrees(g);
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
      if (in.find(it.get_curr_ne()) != g.get_num_arcs(it.get_curr_ne()))
        return false;
    return true;
  }

  // What sufficient_hamiltonian() ensures of a digraph: out(u) + in(v) >= n
  // for each pair without the arc u --> v
  template <class GT>
  bool hamiltonian_degrees(const GT & g)
  {
    const size_t n = g.get_num_nodes();
    auto in = in_degrees(g);
    DynSetTree<std::pair<typename GT::Node *, typename GT::Node *>> arcs;
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next_ne())
      arcs.insert({g.get_src_node(it.get_curr_ne()),
                   g.get_tgt_node(it.get_curr_ne())});
    for (typename GT::Node_Iterator i(g); i.has_curr(); i.next_ne())
      for (typename GT::Node_Iterator j(g); j.has_curr(); j.next_ne())
        {
          auto u = i.get_curr_ne(), v = j.get_curr_ne();
          if (u != v and not arcs.contains({u, v}) and
              g.get_num_arcs(u) + in.find(v) < n)
            return false;
        }
    return true;
  }

  template <class GT>
  struct Throw_At_Arc
  {
    int left = 0; // the arc that throws; 0 for none

    void operator ()(GT &, typename GT::Arc *)
    {
      if (left > 0 and --left == 0)
        throw std::runtime_error("init_arc");
    }
  };
}

// The second digraph, and the next ones, were undirected
TEST(RandomGraphReuse, AnUndirectedTypeGetsADigraphEachTime)
{
  Random_Digraph<Graph> gen(7);
  for (int i = 0; i < 3; ++i)
    {
      SCOPED_TRACE(i);
      Graph sparse = gen(size_t(20), size_t(60), false);
      EXPECT_TRUE(linked_as_digraph(sparse));
      Graph dense = gen(size_t(12), 0.3, false);
      EXPECT_TRUE(linked_as_digraph(dense));
      EXPECT_EQ(dense.get_num_nodes(), 12u);
    }
}

// The parities of the nodes of the former graphs, and the graphs built
// after an Eulerian one, went into the next Eulerian graphs, whose arcs
// then reached nodes of other graphs; all the graphs stay alive
TEST(RandomGraphReuse, EachEulerianGraphIsEulerian)
{
  Random_Graph<Graph> gen(7);
  DynList<Graph> graphs;
  for (int i = 0; i < 20; ++i)
    {
      SCOPED_TRACE(i);
      auto & e1 = graphs.append(gen.eulerian(size_t(30), size_t(40)));
      auto & g = graphs.append(gen(size_t(30), size_t(40)));
      auto & e2 = graphs.append(gen.eulerian(size_t(16), 0.2));
      ASSERT_TRUE(arcs_inside(e1) and arcs_inside(g) and arcs_inside(e2));
      EXPECT_TRUE(all_degrees_even(e1));
      EXPECT_TRUE(all_degrees_even(e2));
      EXPECT_EQ(g.get_num_nodes(), 30u);
    }
}

TEST(RandomGraphReuse, EachEulerianDigraphIsBalanced)
{
  Random_Digraph<Digraph_Type> gen(7);
  DynList<Digraph_Type> graphs;
  for (int i = 0; i < 20; ++i)
    {
      SCOPED_TRACE(i);
      auto & e1 = graphs.append(gen.eulerian(size_t(30), size_t(40)));
      auto & g = graphs.append(gen(size_t(30), size_t(40)));
      auto & e2 = graphs.append(gen.eulerian(size_t(16), 0.2));
      ASSERT_TRUE(arcs_inside(e1) and arcs_inside(g) and arcs_inside(e2));
      EXPECT_TRUE(balanced(e1));
      EXPECT_TRUE(balanced(e2));
      EXPECT_EQ(g.get_num_nodes(), 30u);
    }
}

// eulerian() left the tracking of the parities on, and then the
// Hamiltonian digraph counted each in-degree twice
TEST(RandomGraphReuse, AHamiltonianDigraphAfterAnEulerianOne)
{
  Random_Digraph<Digraph_Type> gen(7);
  for (int i = 0; i < 3; ++i)
    {
      SCOPED_TRACE(i);
      Digraph_Type e = gen.eulerian(size_t(20), size_t(40));
      EXPECT_TRUE(balanced(e));
      Digraph_Type h = gen.sufficient_hamiltonian(size_t(14), 0.2);
      EXPECT_TRUE(arcs_inside(h));
      EXPECT_TRUE(hamiltonian_degrees(h));
    }
}

namespace
{
  template <class GT>
  using Throwing_Digraphs =
    Random_Digraph<GT, Dft_Init_Rand_Node<GT>, Throw_At_Arc<GT>>;

  template <class GT>
  using Throwing_Graphs =
    Random_Graph<GT, Dft_Init_Rand_Node<GT>, Throw_At_Arc<GT>>;

  // A graph that throws halfway stays in the generator: it removed the
  // arcs of a digraph as those of an undirected graph when it was
  // destroyed, and gave the nodes to the next graph
  template <class GT, class Generator>
  void a_graph_after_one_that_threw(const bool digraph)
  {
    Dft_Init_Rand_Node<GT> init_node;
    Throw_At_Arc<GT> init_arc;
    {
      Generator gen(7, init_node, init_arc);
      init_arc.left = 10;
      EXPECT_THROW(gen(size_t(20), size_t(60), false), std::runtime_error);
    }

    Generator gen(7, init_node, init_arc);
    init_arc.left = 10;
    EXPECT_THROW(gen(size_t(20), size_t(60), false), std::runtime_error);
    init_arc.left = 0;
    GT g = gen(size_t(20), size_t(60), false);
    EXPECT_EQ(g.get_num_nodes(), 20u);
    EXPECT_TRUE(arcs_inside(g));
    EXPECT_EQ(g.is_digraph(), digraph);
  }
}

TEST(RandomGraphReuse, AGraphAfterOneThatThrew)
{
  using D = Digraph_Type;
  a_graph_after_one_that_threw<D, Throwing_Digraphs<D>>(true);
  a_graph_after_one_that_threw<Graph, Throwing_Digraphs<Graph>>(true);
  a_graph_after_one_that_threw<Graph, Throwing_Graphs<Graph>>(false);
}

// The odd nodes of K4 are all adjacent, and no arc can go in: the pairs
// had no end
TEST(EulerianGraphs, ACompleteGraphOfEvenOrderIsRejected)
{
  Random_Graph<Graph> gen(7);
  EXPECT_THROW(gen.eulerian(size_t(4), 1.0), std::domain_error);
  Graph k5 = gen.eulerian(size_t(5), 1.0);
  EXPECT_EQ(k5.get_num_arcs(), 10u);
  EXPECT_TRUE(all_degrees_even(k5));
}

// Graphs from a new generator each: a sparse one is always made Eulerian;
// a small or dense one may not be by adding arcs, as when a node of odd
// degree is adjacent to all the others, and then it throws, but it does
// not abort nor search with no end
TEST(EulerianGraphs, FromSparseToDense)
{
  for (unsigned long seed = 1; seed <= 150; ++seed)
    {
      SCOPED_TRACE(seed);
      const size_t n = 20 + seed % 20;
      {
        Random_Digraph<Digraph_Type> gen(seed);
        Digraph_Type e = gen.eulerian(n, n + seed % n);
        EXPECT_TRUE(balanced(e) and arcs_inside(e));
      }
      {
        Random_Graph<Graph> gen(seed);
        Graph e = gen.eulerian(n, n + seed % n);
        EXPECT_TRUE(all_degrees_even(e) and arcs_inside(e));
      }

      const size_t small = 4 + seed % 10;
      for (double p : {0.3, 0.7, 0.95})
        {
          try
            {
              Random_Digraph<Digraph_Type> gen(seed);
              Digraph_Type e = gen.eulerian(small, p);
              EXPECT_TRUE(balanced(e) and arcs_inside(e));
            }
          catch (const std::domain_error &) {}
          try
            {
              Random_Graph<Graph> gen(seed);
              Graph e = gen.eulerian(small, p);
              EXPECT_TRUE(all_degrees_even(e) and arcs_inside(e));
            }
          catch (const std::domain_error &) {}
        }
    }
}
