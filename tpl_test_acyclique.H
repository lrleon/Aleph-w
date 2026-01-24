
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


/** @file tpl_test_acyclique.H
 *  @brief DAG (acyclic) graph testing.
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_TEST_ACYCLIQUE_H
# define TPL_TEST_ACYCLIQUE_H

# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  /** Determines whether a graph is acyclic (contains no cycles).

      Is_Graph_Acyclique performs a depth-first search on graph g,
      starting from a node, and checks whether the graph is acyclic;
      i.e., that it contains no cycle.

      The class takes two template parameters:
      -# GT: the graph type, which must be derived from List_Graph.
      -# SA: a class responsible for selecting/showing arcs. Internally,
      the function uses the filtered iterator Node_Arc_Iterator (based
      on Filter_Iterator) to traverse each node's arcs. SA is the class
      that decides whether an arc is included in the traversal.

      The class uses the Test_Cycle bit and resets it at the beginning
      of the algorithm to mark already visited nodes and arcs.

      @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Is_Graph_Acyclique
  {
    SA & sa;

    bool is_acyclique(typename GT::Node *curr)
    {
      if (IS_NODE_VISITED(curr, Test_Cycle))
        return false;

      NODE_BITS(curr).set_bit(Test_Cycle, true); // mark node

      for (Node_Arc_Iterator<GT, SA> i(curr, sa); i.has_curr(); i.next_ne())
        {
          typename GT::Arc *arc = i.get_current_arc_ne();
          if (IS_ARC_VISITED(arc, Test_Cycle))
            continue;

          ARC_BITS(arc).set_bit(Test_Cycle, true);

          if (not is_acyclique(i.get_tgt_node()))
            return false;
        }

      // all arcs traversed without finding a cycle ==>
      // the graph is acyclic through curr_node
      return true;
    }

    bool is_acyclique(GT & g, size_t num_arcs)
    {
      ah_domain_error_if(g.is_digraph())
        << "is_graph_acyclique() does not work for digraphs";

      if (num_arcs >= g.get_num_nodes())
        return false;

      g.reset_bit_arcs(Test_Cycle);
      g.reset_bit_nodes(Test_Cycle);

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          typename GT::Node *curr = it.get_current_node_ne();
          if (IS_NODE_VISITED(curr, Test_Cycle))
            continue;

          if (not is_acyclique(curr))
            return false;
        }

      return true;
    }

  public:
    Is_Graph_Acyclique(SA && __sa = SA()) : sa(__sa)
    { /* empty */
    }

    Is_Graph_Acyclique(SA & __sa) : sa(__sa)
    { /* empty */
    }

    /** Invokes the acyclicity test.

      If g is an undirected graph (not a digraph), then the algorithm
      compares the number of arcs with the number of nodes. If the graph
      has as many arcs as nodes (or more), then it is concluded that the
      graph is not acyclic. Consequently, this technique does not work
      for multigraphs.

      @param[in] g the graph to test.
@param num_arcs number of arcs in g. Use this value if you are
using filtered iterators over marked arcs, or arcs interpreted
according to a given criterion. This way, the routine knows how
many arcs satisfy the criterion.
      @return true if the graph contains no cycle; false otherwise.
      @note Because of the initial arc-count check, this routine can be
      preferable to test_for_cycle(), even if you know the graph is
      connected, since the latter routine always performs a depth-first
      search, while is_graph_acyclique() may not.
   */
    bool operator ()(GT & g, size_t num_arcs)
    {
      return is_acyclique(g, num_arcs);
    }

    /** Invokes the acyclicity test.

      If g is an undirected graph (not a digraph), then the algorithm
      compares the number of arcs with the number of nodes. If the graph
      has as many arcs as nodes (or more), then it is concluded that the
      graph is not acyclic. Consequently, this technique does not work
      for multigraphs.

      @param[in] g the graph to test.
      @return true if the graph contains no cycle; false otherwise.
      @note Because of the initial arc-count check, this routine can be
      preferable to test_for_cycle(), even if you know the graph is
      connected, since the latter routine always performs a depth-first
      search, while is_graph_acyclique() may not.
   */
    bool operator ()(GT & g)
    {
      return is_acyclique(g, g.get_num_arcs());
    }
  };

  /** Determines whether a graph contains cycles.

       Has_Cycle performs a depth-first search on graph g, starting from
       a node, and checks whether the graph contains a cycle.

       The class takes two template parameters:
       -# GT: the graph type, which must be derived from List_Graph.
       -# SA: a class responsible for selecting/showing arcs. Internally,
       the function uses the filtered iterator Node_Arc_Iterator (based
       on Filter_Iterator) to traverse each node's arcs. SA is the class
       that decides whether an arc is included in the traversal.

       The class uses the Test_Cycle bit and resets it at the beginning
       of the algorithm to mark already visited nodes and arcs.

       @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Has_Cycle
  {
    SA & sa;

  public:
    Has_Cycle(SA && __sa = SA()) : sa(__sa)
    { /* empty */
    }

    Has_Cycle(SA & __sa) : sa(__sa)
    { /* empty */
    }

    /** Invokes the cycle-existence test.

        If g is an undirected graph (not a digraph), then the algorithm
        compares the number of arcs with the number of nodes. If the graph
        has as many arcs as nodes (or more), then it is concluded that the
        graph contains cycles. Consequently, this technique does not work
        for multigraphs.

        @param[in] g the graph to test.
        @return true if the graph contains at least one cycle; false otherwise.
        @note Because of the initial arc-count check, this routine can be
        preferable to test_for_cycle(), even if you know the graph is
        connected, since the latter routine always performs a depth-first
        search, while is_graph_acyclique() may not.
    */
    bool operator ()(GT & g) const
    {
      return not Is_Graph_Acyclique<GT, SA>(sa)(g);
    }

    /** Invokes the cycle-existence test.

        If g is an undirected graph (not a digraph), then the algorithm
        compares the number of arcs with the number of nodes. If the graph
        has as many arcs as nodes (or more), then it is concluded that the
        graph contains cycles. Consequently, this technique does not work
        for multigraphs.

        @param[in] g the graph to test.
        @param[in] num_arcs number of arcs in the graph (for optimization).
        @return true if the graph contains at least one cycle; false otherwise.
        @note Because of the initial arc-count check, this routine can be
        preferable to test_for_cycle(), even if you know the graph is
        connected, since the latter routine always performs a depth-first
        search, while is_graph_acyclique() may not.
    */
    bool operator ()(GT & g, size_t num_arcs) const
    {
      return not Is_Graph_Acyclique<GT, SA>(sa)(g, num_arcs);
    }
  };
} // end namespace Aleph

# endif //  TPL_TEST_ACYCLIQUE_H
