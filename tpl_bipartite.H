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

/** @file tpl_bipartite.H
 *  @brief Bipartite graph detection and 2-coloring.
 *
 *  Tests if a graph is bipartite (can be 2-colored) and computes
 *  the bipartition if it exists.
 *
 *  ## Features
 *  - Test bipartiteness via BFS
 *  - Compute vertex 2-coloring
 *  - Detect odd cycles (non-bipartite proof)
 *
 *  ## Complexity: O(V + E)
 *
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BIPARTITE_H
# define TPL_BIPARTITE_H

# include <tpl_dynDlist.H>
# include <tpl_net.H>
# include <ah-errors.H>
# include <cookie_guard.H>

namespace Aleph
{
  enum Bipartite_Color { Bp_White, Bp_Red, Bp_Blue };

  template <class GT>
  static long &color(typename GT::Node *p)
  {
    return NODE_COUNTER(p);
  }

  template <class GT>
  static long &color(typename GT::Arc *a)
  {
    return ARC_COUNTER(a);
  }

  /** Computes the partition sets of a bipartite graph.

A graph is bipartite if it can be divided into two subsets l and
r such that every node in l only has edges to nodes in r and
vice versa.

compute_bipartite(g,l,r) takes a bipartite graph g and computes
the bipartition sets named by parameters l and r, respectively.

@param[in] g the bipartite graph.
@param[out] l one partition set.
@param[out] r the other partition set.
@exception domain_error if during computation it is determined that
the graph is not bipartite.
@exception bad_alloc if there is not enough memory.
@ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  void compute_bipartite(const GT & g,
                         DynDlist<typename GT::Node *> & l,
                         DynDlist<typename GT::Node *> & r)
  {
    g.reset_nodes(); // initialize counters to White
    g.reset_arcs();

    DynDlist<typename GT::Node *> red, blue;
    typename GT::Node *p = g.get_first_node();
    color<GT>(p) = Bp_Red;
    red.put(p);
    l.put(p);

    while (true)
      if (not red.is_empty()) // any red node with unvisited arcs?
        {
          typename GT::Node *p = red.get();
          for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
            {
              typename GT::Arc *a = it.get_curr();
              ah_domain_error_if(color<GT>(a) == Bp_Red) << "Graph is not bipartite";
              if (color<GT>(a) == Bp_Blue)
                continue;

              color<GT>(a) = Bp_Red;
              typename GT::Node *q = it.get_tgt_node();
              ah_domain_error_if(color<GT>(q) == Bp_Red) << "Graph is not bipartite";
              if (color<GT>(q) == Bp_Blue)
                continue;

              color<GT>(q) = Bp_Blue;
              blue.put(q);
              r.put(q);
            }
        }
      else if (not blue.is_empty()) // any blue node with unvisited arcs?
        {
          typename GT::Node *p = blue.get();
          for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
            {
              typename GT::Arc *a = it.get_curr();
              ah_domain_error_if(color<GT>(a) == Bp_Blue) << "Graph is not bipartite";
              if (color<GT>(a) == Bp_Red)
                continue;

              color<GT>(a) = Bp_Blue;

              typename GT::Node *q = it.get_tgt_node();
              ah_domain_error_if(color<GT>(q) == Bp_Blue) << "Graph is not bipartite";
              if (color<GT>(q) == Bp_Red)
                continue;

              color<GT>(q) = Bp_Red;
              red.put(q);
              l.put(q);
            }
        }
      else
        break;
  }

  /** Class that takes a bipartite graph and computes the partition sets.

 A graph is bipartite if it can be divided into two subsets l and
 r such that every node in l only has edges to nodes in r and
 vice versa.

 @ingroup Graphs
    */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Compute_Bipartite
  {
  public:
    /** Computes the partition sets of a bipartite graph.

 @param[in] g the bipartite graph.
 @param[out] l one partition set.
 @param[out] r the other partition set.
 @exception domain_error if during computation it is determined that
 the graph is not bipartite.
 @exception bad_alloc if there is not enough memory.
    */
    void operator ()(const GT & g,
                     DynDlist<typename GT::Node *> & l,
                     DynDlist<typename GT::Node *> & r)
    {
      compute_bipartite<GT, SA>(g, l, r);
    }
  };

  /** Computes the maximum cardinality matching of a bipartite graph.

compute_maximum_cardinality_bipartite_matching(g,matching)
takes a bipartite graph g and computes the maximum bipartite
matching in the list matching.

The procedure computes the bipartition sets, then constructs
an equivalent unit-capacity flow network and invokes a
maximum flow algorithm on it.

The routine handles two type parameters:
-# GT the bipartite graph type
-# Max_Flow the maximum flow algorithm to use for the computation
.

@param[in] g the bipartite graph.
@param[out] matching list of arcs that form the matching.
@exception bad_alloc if there is not enough memory.

@ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  void compute_maximum_cardinality_bipartite_matching
  (const GT & g, DynDlist<typename GT::Arc *> & matching)
  {
    DynDlist<typename GT::Node *> l, r;

    compute_bipartite(g, l, r);

    typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class>> AN;
    AN net;

    // Cookie_Guard ensures cookies are cleared when function exits
    // (prevents dangling pointers to destroyed 'net' nodes)
    Cookie_Guard<GT> cookie_guard(g, true, true);

    // traverse nodes of g and insert their image in net
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        typename GT::Node *p = it.get_curr();
        NODE_COOKIE(p) = net.insert_node();
        NODE_COOKIE((typename GT::Node *) NODE_COOKIE(p)) = p;
      }

    typename AN::Node *source = net.insert_node();

    // traverse nodes of l, connect them to source and insert their arcs
    for (typename DynDlist<typename GT::Node *>::Iterator i(l);
         i.has_curr(); i.next_ne())
      {
        typename GT::Node *p = i.get_curr();
        typename AN::Node *src = mapped_node<GT, AN>(p);
        net.insert_arc(source, src, 1);

        for (Node_Arc_Iterator<GT, SA> j(p); j.has_curr(); j.next_ne())
          {
            typename GT::Arc *arc = j.get_current_arc_ne();
            typename AN::Node *tgt = mapped_node<GT, AN>(g.get_tgt_node(arc));
            typename AN::Arc *a = net.insert_arc(src, tgt, 1);
            ARC_COOKIE(arc) = a;
            ARC_COOKIE(a) = arc;
          }
      }

    typename AN::Node *sink = net.insert_node();

    // traverse nodes of r and connect them to the sink
    for (typename DynDlist<typename GT::Node *>::Iterator it(r);
         it.has_curr(); it.next_ne())
      {
        typename GT::Node *p = it.get_curr();
        net.insert_arc(mapped_node<GT, AN>(p), sink, 1);
      }

    Max_Flow<AN>()(net);

    for (Arc_Iterator<AN> it(net); it.has_curr(); it.next_ne())
      {
        typename AN::Arc *a = it.get_curr();
        if (a->flow == 0)
          continue;

        typename GT::Arc *arc = mapped_arc<AN, GT>(a);
        if (arc == nullptr)
          continue;

        matching.append(arc);
      }
  }

  /** Class for computing the maximum cardinality matching of a bipartite graph.

The class handles two type parameters:
-# GT the bipartite graph type
-# Max_Flow the maximum flow algorithm to use for the computation.
By default, the Ford-Fulkerson algorithm is used.
.

@ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Ford_Fulkerson_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Maximum_Cardinality_Bipartite_Matching
  {
  public:
    /** Computes the maximum bipartite matching of a graph.

        compute_maximum_cardinality_bipartite_matching(g,matching)
        takes a bipartite graph g and computes the maximum bipartite
        matching in the list matching.

        The procedure computes the bipartition sets, then constructs
        an equivalent unit-capacity flow network and invokes a
        maximum flow algorithm on it.

        @param[in] g the bipartite graph.
        @param[out] matching list of arcs that form the matching.
        @exception bad_alloc if there is not enough memory.
     */
    void operator ()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      compute_maximum_cardinality_bipartite_matching<GT, Max_Flow, SA>
          (g, matching);
    }
  };
} // end namespace Aleph

# endif //  TPL_BIPARTITE_H
