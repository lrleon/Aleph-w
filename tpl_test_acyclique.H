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
