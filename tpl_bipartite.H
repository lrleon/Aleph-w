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
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_BIPARTITE_H
# define TPL_BIPARTITE_H

# include <tpl_dynDlist.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynListQueue.H>
# include <tpl_net.H>
# include <ah-errors.H>
# include <cookie_guard.H>
# include <limits>

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

    // Cookie_Guard ensures cookies are cleared when a function exits
    // (prevents dangling pointers to destroyed 'net' nodes)
    Cookie_Guard<GT> cookie_guard(g, true, true);

    // Use a map to store arc mapping since Max_Flow algorithms reset cookies
    DynMapTree<typename AN::Arc *, typename GT::Arc *> arc_map;

    // traverse nodes of g and insert their image in the net
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        typename GT::Node *p = it.get_curr();
        NODE_COOKIE(p) = net.insert_node();
        NODE_COOKIE((typename GT::Node *) NODE_COOKIE(p)) = p;
      }

    typename AN::Node *source = net.insert_node();

    // traverse nodes of l, connect them to the source and insert their arcs
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
            // Store mapping in map (not cookies, which are cleared by Max_Flow)
            arc_map.insert(a, arc);
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

        // Look up the original arc in our map
        auto *pair_ptr = arc_map.search(a);
        if (pair_ptr == nullptr)
          continue;

        matching.append(pair_ptr->second);
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
        an equivalent unit-capacity flow network, and invokes a
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

  /** Computes the partition sets of a bipartite graph handling all
      connected components.

  Unlike compute_bipartite(), this function processes disconnected
  graphs correctly by running BFS from each unvisited node.

  @param[in] g the graph to test.
  @param[out] l one partition set (red nodes).
  @param[out] r the other partition set (blue nodes).
  @return true if the graph is bipartite, false otherwise.
  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  bool compute_bipartite_all_components(const GT & g,
                                        DynDlist<typename GT::Node *> & l,
                                        DynDlist<typename GT::Node *> & r)
  {
    using Node = typename GT::Node;

    g.reset_nodes();
    g.reset_arcs();

    for (Node_Iterator<GT> nit(g); nit.has_curr(); nit.next_ne())
      {
        Node *start = nit.get_curr();
        if (color<GT>(start) != Bp_White)
          continue;

        color<GT>(start) = Bp_Red;
        l.append(start);

        DynListQueue<Node *> queue;
        queue.put(start);

        while (not queue.is_empty())
          {
            Node *p = queue.get();
            const long p_color = color<GT>(p);
            const long neighbor_color = (p_color == Bp_Red) ? Bp_Blue : Bp_Red;

            for (Node_Arc_Iterator<GT, SA> it(p); it.has_curr(); it.next_ne())
              if (Node *q = it.get_tgt_node(); color<GT>(q) == Bp_White)
                {
                  color<GT>(q) = neighbor_color;
                  if (neighbor_color == Bp_Red)
                    l.append(q);
                  else
                    r.append(q);
                  queue.put(q);
                }
              else if (color<GT>(q) == p_color)
                return false;
          }
      }

    return true;
  }

  // Helper: DFS for augmenting path in Hopcroft-Karp.
  // Returns true if an augmenting path from u to a free right-node
  // was found and the matching was augmented along it.
  template <class GT, class SA>
  static bool hopcroft_karp_dfs(const GT & g, typename GT::Node *u)
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    constexpr long INF = std::numeric_limits<long>::max();
    long & u_dist = NODE_COUNTER(u);

    for (Node_Arc_Iterator<GT, SA> it(u); it.has_curr(); it.next_ne())
      {
        Arc *arc = it.get_current_arc_ne();
        Node *v = g.get_connected_node(arc, u); // right-side node

        // v's match partner (maybe nullptr if v is free)
        Arc *match_arc = static_cast<Arc *>(NODE_COOKIE(v));
        Node *pair_v = (match_arc != nullptr) ? g.get_connected_node(match_arc, v) : nullptr;

        if (pair_v == nullptr) // v is free → augmenting path found
          {
            NODE_COOKIE(u) = arc;
            NODE_COOKIE(v) = arc;
            u_dist = INF;
            return true;
          }

        // pair_v is a left-side node matched to v
        if (NODE_COUNTER(pair_v) == u_dist + 1)
          {
            if (hopcroft_karp_dfs<GT, SA>(g, pair_v))
              {
                NODE_COOKIE(u) = arc;
                NODE_COOKIE(v) = arc;
                u_dist = INF;
                return true;
              }
          }
      }

    u_dist = INF; // mark u as "dead" for this phase
    return false;
  }

  /** Computes a maximum cardinality matching on a bipartite graph
      using the Hopcroft-Karp algorithm.

  The Hopcroft-Karp algorithm runs in O(E * sqrt(V)) time, which is
  more efficient than the max-flow based approach for sparse bipartite
  graphs.

  This implementation handles disconnected graphs correctly.

  @param[in] g the bipartite graph.
  @param[out] matching list of arcs forming the maximum matching.
  @exception domain_error if the graph is not bipartite.
  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  void hopcroft_karp_matching(const GT & g,
                              DynDlist<typename GT::Arc *> & matching)
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    constexpr long INF = std::numeric_limits<long>::max();

    if (g.get_num_nodes() == 0)
      return;

    DynDlist<Node *> left_nodes, right_nodes;

    // 2-coloring (uses NODE_COUNTER for color)
    const bool is_bipartite =
        compute_bipartite_all_components<GT, SA>(g, left_nodes, right_nodes);
    ah_domain_error_if(not is_bipartite) << "Graph is not bipartite";

    // Now repurpose NODE_COOKIE for matching arcs and
    // NODE_COUNTER for BFS distances (left-side only).
    // Cookie_Guard clears cookies on exit.
    Cookie_Guard<GT> cookie_guard(g, true, false);

    // Initialize: all nodes are free (NODE_COOKIE = nullptr already from guard)
    // Set all left-side distances to INF
    for (auto it = left_nodes.get_it(); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_curr()) = INF;
    for (auto it = right_nodes.get_it(); it.has_curr(); it.next_ne())
      NODE_COUNTER(it.get_curr()) = 0; // right-side counters unused

    // BFS phase: returns true if augmenting paths exist
    // Sets distances on left-side nodes
    auto bfs = [&]() -> bool
      {
        DynListQueue<Node *> queue;
        long dist_to_free = INF;

        // Enqueue all free left-side nodes at distance 0
        for (auto it = left_nodes.get_it(); it.has_curr(); it.next_ne())
          if (Node *u = it.get_curr(); NODE_COOKIE(u) == nullptr) // u is free
            {
              NODE_COUNTER(u) = 0;
              queue.put(u);
            }
          else
            NODE_COUNTER(u) = INF;

        while (not queue.is_empty())
          {
            Node *u = queue.get();
            const long u_dist = NODE_COUNTER(u);

            if (u_dist >= dist_to_free)
              continue;

            for (Node_Arc_Iterator<GT, SA> it(u); it.has_curr(); it.next_ne())
              {
                Arc *arc = it.get_current_arc_ne();
                Node *v = g.get_connected_node(arc, u); // right-side

                Arc *match_arc = static_cast<Arc *>(NODE_COOKIE(v));
                Node *pair_v = (match_arc != nullptr) ? g.get_connected_node(match_arc, v) : nullptr;

                if (pair_v == nullptr) // v is free
                  dist_to_free = u_dist + 1;
                else if (NODE_COUNTER(pair_v) == INF)
                  {
                    NODE_COUNTER(pair_v) = u_dist + 1;
                    queue.put(pair_v);
                  }
              }
          }

        return dist_to_free != INF;
      };

    // Main loop: BFS + DFS phases
    while (bfs())
      {
        for (auto it = left_nodes.get_it(); it.has_curr(); it.next_ne())
          if (Node *u = it.get_curr(); NODE_COOKIE(u) == nullptr) // u is free
            hopcroft_karp_dfs<GT, SA>(g, u);
      }

    // Collect matching arcs from left-side nodes
    DynSetAvlTree<Arc *> seen;
    for (auto it = left_nodes.get_it(); it.has_curr(); it.next_ne())
      {
        Arc *a = static_cast<Arc *>(NODE_COOKIE(it.get_curr()));
        if (a != nullptr and not seen.contains(a))
          {
            matching.append(a);
            seen.insert(a);
          }
      }
  }

  /** Functor wrapper for hopcroft_karp_matching.

  @tparam GT the graph type.
  @tparam SA arc filter (default: Dft_Show_Arc<GT>).
  @ingroup Graphs
   */
  template <class GT, class SA = Dft_Show_Arc<GT>>
  class Hopcroft_Karp_Matching
  {
  public:
    /** Computes the maximum matching using Hopcroft-Karp.

    @param[in] g the bipartite graph.
    @param[out] matching list of arcs forming the maximum matching.
    @exception domain_error if the graph is not bipartite.
     */
    void operator()(const GT & g, DynDlist<typename GT::Arc *> & matching)
    {
      hopcroft_karp_matching<GT, SA>(g, matching);
    }
  };
} // end namespace Aleph

# endif //  TPL_BIPARTITE_H
