
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

# include <type_traits>

# include <ah-graph-concepts.H>

# include <tpl_dfs_stack.H>
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
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Is_Graph_Acyclique
  {
    Kept_Filter<SA> sa;

    // DFS from curr that tells whether no node is reached twice. A path
    // can have as many nodes as the graph, too many for recursion, so that
    // the search keeps its own stack; the order of the visits is that of a
    // recursion.
    bool is_acyclique(typename GT::Node *curr)
    {
      if (IS_NODE_VISITED(curr, Test_Cycle))
        return false;

      NODE_BITS(curr).set_bit(Test_Cycle, true); // mark node

      // The node being visited keeps its iterator in here until it visits
      // a node; then it waits in frames, where it goes on when it comes
      // back
      using Frame = Dfs_Arc_Frame<Node_Arc_Iterator<GT, SA>>;
      Dfs_Stack<Frame> frames;
      Frame here(curr, sa);
      while (true)
        {
          if (auto tgt = here.template next_target<Test_Cycle, true>())
            { // the iterator stays on the arc to tgt, as in a recursion
              if (IS_NODE_VISITED(tgt, Test_Cycle)) // reached again
                return false;

              NODE_BITS(tgt).set_bit(Test_Cycle, true);
              frames.push(std::move(here));
              here = Frame(tgt, sa);
              continue;
            }

          while (true) // back to the nodes that wait
            {
              // all arcs traversed without finding a cycle ==>
              // the graph is acyclic through curr_node
              if (frames.is_empty())
                return true;

              Frame & f = frames.top();
              f.next_arc(); // past the arc to the node that was visited
              if (auto tgt = f.template next_target<Test_Cycle, true>())
                {
                  if (IS_NODE_VISITED(tgt, Test_Cycle)) // reached again
                    return false;

                  NODE_BITS(tgt).set_bit(Test_Cycle, true);
                  here = Frame(tgt, sa);
                  break;
                }
              frames.pop();
            }
        }
    }

    // Whether g has no cycle; num_arcs, if known, is the number of arcs
    // that sa shows
    bool is_acyclique(GT & g, size_t num_arcs, bool num_arcs_known = true)
    {
      ah_domain_error_if(g.is_digraph())
        << "is_graph_acyclique() does not work for digraphs";

      // a forest with nodes has fewer arcs than nodes
      if (num_arcs_known and g.get_num_nodes() > 0
          and num_arcs >= g.get_num_nodes())
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
    Is_Graph_Acyclique(SA && __sa = SA()) : sa(std::move(__sa))
    { /* empty */
    }

    Is_Graph_Acyclique(SA & __sa) : sa(__sa)
    { /* empty */
    }

    /** Invokes the acyclicity test, with the number of arcs that the
      filter shows.

      A forest has fewer arcs than nodes, so that a graph with nodes and
      with `num_arcs` as large as their number, or larger, has a cycle,
      which is answered without a search. A loop is a cycle, and so are
      two arcs between the same nodes.

      @param[in] g the graph to test.
      @param[in] num_arcs number of arcs of g that the filter shows. This
      form serves a filter that hides arcs, whose number the routine
      cannot know otherwise.
      @return true if the graph contains no cycle; false otherwise.
      @throw domain_error if g is a digraph.
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

      With the default filter, which shows all the arcs, a graph with
      nodes and with as many arcs as nodes, or more, has a cycle, which is
      answered without a search; with another filter, which may hide arcs,
      the graph is searched. A loop is a cycle, and so are two arcs between
      the same nodes.

      @param[in] g the graph to test.
      @return true if the graph contains no cycle; false otherwise.
      @throw domain_error if g is a digraph.
      @note Because of the initial arc-count check, this routine can be
      preferable to test_for_cycle(), even if you know the graph is
      connected, since the latter routine always performs a depth-first
      search, while is_graph_acyclique() may not.
   */
    bool operator ()(GT & g)
    {
      // the number of arcs of g is that of those that sa shows only if sa
      // shows them all
      return is_acyclique(g, g.get_num_arcs(),
                          std::is_same_v<SA, Dft_Show_Arc<GT>>);
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
  template <AlephGraph GT, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  class Has_Cycle
  {
    Kept_Filter<SA> sa;

  public:
    Has_Cycle(SA && __sa = SA()) : sa(std::move(__sa))
    { /* empty */
    }

    Has_Cycle(SA & __sa) : sa(__sa)
    { /* empty */
    }

    /** Invokes the cycle-existence test.

        With the default filter, which shows all the arcs, a graph with
        nodes and with as many arcs as nodes, or more, has a cycle, which
        is answered without a search; with another filter, which may hide
        arcs, the graph is searched. A loop is a cycle, and so are two arcs
        between the same nodes.

        @param[in] g the graph to test.
        @return true if the graph contains at least one cycle; false otherwise.
        @throw domain_error if g is a digraph.
        @note Because of the initial arc-count check, this routine can be
        preferable to test_for_cycle(), even if you know the graph is
        connected, since the latter routine always performs a depth-first
        search, while is_graph_acyclique() may not.
    */
    bool operator ()(GT & g) const
    {
      return not Is_Graph_Acyclique<GT, SA>(sa)(g);
    }

    /** Invokes the cycle-existence test, with the number of arcs that the
        filter shows.

        A forest has fewer arcs than nodes, so that a graph with nodes and
        with `num_arcs` as large as their number, or larger, has a cycle,
        which is answered without a search. A loop is a cycle, and so are
        two arcs between the same nodes.

        @param[in] g the graph to test.
        @param[in] num_arcs number of arcs of g that the filter shows.
        @return true if the graph contains at least one cycle; false otherwise.
        @throw domain_error if g is a digraph.
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
