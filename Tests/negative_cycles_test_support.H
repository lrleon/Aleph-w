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
 * @file negative_cycles_test_support.H
 * @brief Graph types and probe helpers shared by `negative_cycles_test.cc`
 *        and `bounded_negative_cycle_test.cc`.
 *
 * Both suites build the same small digraphs, count how often the distance
 * accessor and the arc filter are called, hide or reject arcs, and check that
 * a search leaves the cookies and control bits of the graph untouched. The
 * helpers live here so that the two files do not keep verbatim copies in sync.
 * What each suite checks about a result (the validators, the oracles and the
 * test graphs of one function) stays in its own file.
 */

# ifndef NEGATIVE_CYCLES_TEST_SUPPORT_H
# define NEGATIVE_CYCLES_TEST_SUPPORT_H

# include <cmath>
# include <cstddef>
# include <tuple>
# include <vector>

# include <tpl_agraph.H>
# include <tpl_graph.H>

namespace Negative_Cycles_Test_Support
{
  using namespace Aleph;

  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<long long>>;
  using Float_Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
  using Arr_Digraph = Array_Digraph<Graph_Anode<int>, Graph_Aarc<long long>>;
  using UGraph = List_Graph<Graph_Node<int>, Graph_Arc<long long>>;
  using Node = Graph::Node;
  using Arc = Graph::Arc;
  using Edge_Def = std::tuple<size_t, size_t, long long>;

  template <class GT>
  struct Built_Graph_T
  {
    GT g;
    std::vector<typename GT::Node *> nodes;
    std::vector<typename GT::Arc *> arcs;
  };

  template <class GT, typename Weight_Type>
  Built_Graph_T<GT>
  build_graph_generic(const size_t n,
                      const std::vector<std::tuple<size_t, size_t, Weight_Type>> & edges)
  {
    Built_Graph_T<GT> built;
    built.nodes.reserve(n);
    built.arcs.reserve(edges.size());

    for (size_t i = 0; i < n; ++i)
      built.nodes.push_back(built.g.insert_node(static_cast<int>(i)));

    for (const auto & [u, v, w] : edges)
      built.arcs.push_back(built.g.insert_arc(built.nodes[u], built.nodes[v], w));

    return built;
  }

  inline Built_Graph_T<Graph> build_graph(const size_t n, const std::vector<Edge_Def> & edges)
  {
    return build_graph_generic<Graph, long long>(n, edges);
  }

  struct Hide_Arc
  {
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const noexcept
    {
      return arc != blocked;
    }
  };

  // Counts how many times the distance accessor is called.
  struct Counting_Dist
  {
    using Distance_Type = long long;
    int * calls = nullptr;

    Distance_Type operator()(Arc * arc) const
    {
      ++*calls;
      return arc->get_info();
    }
  };

  // Counts how many times the filter is asked, and hides one arc.
  struct Counting_Filter
  {
    int * calls = nullptr;
    Arc * blocked = nullptr;

    bool operator()(Arc * arc) const
    {
      ++*calls;
      return arc != blocked;
    }
  };

  // Rejects the arcs whose weight is not finite.
  struct Finite_Arcs
  {
    bool operator()(Float_Graph::Arc * arc) const noexcept
    {
      return std::isfinite(arc->get_info());
    }
  };

  // Non-null cookies and control bits planted on a graph before a search,
  // checked afterwards: the searches must not read, use or clear them.
  template <class GT>
  struct Planted_State
  {
    std::vector<int> cells;

    void plant(Built_Graph_T<GT> & built)
    {
      cells.assign(built.nodes.size() + built.arcs.size(), 0);
      size_t k = 0;
      for (size_t i = 0; i < built.nodes.size(); ++i, ++k)
        {
          NODE_COOKIE(built.nodes[i]) = &cells[k];
          NODE_BITS(built.nodes[i]).set_bit(Aleph::Spanning_Tree, i % 2 == 0);
          NODE_BITS(built.nodes[i]).set_bit(Aleph::Find_Path, true);
        }
      for (size_t i = 0; i < built.arcs.size(); ++i, ++k)
        {
          ARC_COOKIE(built.arcs[i]) = &cells[k];
          ARC_BITS(built.arcs[i]).set_bit(Aleph::Spanning_Tree, true);
          ARC_BITS(built.arcs[i]).set_bit(Aleph::Find_Path, i % 2 == 1);
        }
    }

    bool intact(const Built_Graph_T<GT> & built) const
    {
      size_t k = 0;
      for (size_t i = 0; i < built.nodes.size(); ++i, ++k)
        if (NODE_COOKIE(built.nodes[i]) != &cells[k]
            or IS_NODE_VISITED(built.nodes[i], Aleph::Spanning_Tree) != (i % 2 == 0)
            or not IS_NODE_VISITED(built.nodes[i], Aleph::Find_Path))
          return false;
      for (size_t i = 0; i < built.arcs.size(); ++i, ++k)
        if (ARC_COOKIE(built.arcs[i]) != &cells[k]
            or not IS_ARC_VISITED(built.arcs[i], Aleph::Spanning_Tree)
            or IS_ARC_VISITED(built.arcs[i], Aleph::Find_Path) != (i % 2 == 1))
          return false;
      return true;
    }
  };
} // namespace Negative_Cycles_Test_Support

# endif // NEGATIVE_CYCLES_TEST_SUPPORT_H
