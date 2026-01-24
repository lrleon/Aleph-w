
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file tpl_kgraph.H
 *  @brief K-connected graph structures and connectivity algorithms.
 *
 *  @par Example: Computing edge connectivity
 *  @code
 *  using GT = List_Graph<Graph_Node<int>, Graph_Arc<int>>;
 *  GT network;
 *  
 *  // Create nodes
 *  auto a = network.insert_node(1);
 *  auto b = network.insert_node(2);
 *  auto c = network.insert_node(3);
 *  auto d = network.insert_node(4);
 *  
 *  // 2-connected: requires removing 2 edges to disconnect
 *  network.insert_arc(a, b);
 *  network.insert_arc(b, c);
 *  network.insert_arc(c, d);
 *  network.insert_arc(d, a);
 *  network.insert_arc(a, c); // diagonal
 *  
 *  long k = edge_connectivity(network);
 *  cout << "Edge connectivity: " << k << endl; // k = 2
 *  @endcode
 *
 *  @par Example: Network reliability
 *  @code
 *  // Higher k = more fault-tolerant network
 *  // k=1: single point of failure (bridge exists)
 *  // k=2: can survive one edge failure
 *  // k=3: can survive two edge failures
 *  @endcode
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_KGRAPH_H
# define TPL_KGRAPH_H

# include <limits>
# include <tpl_dynSetTree.H>
# include <tpl_net.H>
# include <cookie_guard.H>

namespace Aleph
{
  /** @brief Compute edge connectivity (arc connectivity) of an undirected graph.
   *
   * Builds a unit-capacity auxiliary network and computes maximum flow
   * from a fixed source to every other node. The minimum of those flow
   * values equals the edge connectivity.
   *
   * The arc filter `SA` is applied when computing degrees and when
   * building the auxiliary network.
   *
   * @tparam GT Graph type (undirected).
   * @tparam Max_Flow Maximum flow functor to use.
   * @tparam SA Arc filter functor.
   *
   * Side effects:
   * - Uses the `Depth_First` control bit on nodes and arcs to test connectivity.
   * - Overwrites node cookies to store the GT -> Net mapping when the
   *   auxiliary network is built.
   *
   * @param[in,out] g Graph to analyze.
   * @return Edge connectivity of `g` (0 for empty or disconnected graphs).
   * @note For connected graphs with minimum filtered degree <= 1, returns
   *       that degree without running max flow.
   * @throw std::domain_error If `g` is a digraph.
   * @throw std::bad_alloc If there is not enough memory.
   *
   * @ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Random_Preflow_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  long edge_connectivity(GT & g)
  {
    const SA sa;
    ah_domain_error_if(g.is_digraph())
      << "edge_connectivity() does not work on digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return 0;

    typename GT::Node *source_node = nullptr;
    long min_degree = std::numeric_limits<long>::max();
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        long degree = 0;
        for (Node_Arc_Iterator<GT, SA> it_arc(p, sa); it_arc.has_curr();
             it_arc.next_ne())
          ++degree;
        if (degree < min_degree)
          {
            min_degree = degree;
            source_node = p;
          }
      }

    Depth_First_Traversal<GT, Default_Visit_Op<GT>, SA> dfs(sa);
    if (dfs(g, source_node) != num_nodes)
      return 0;

    if (min_degree <= 1)
      return min_degree;

    typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class>> Net;
    Net net;

    // Cookie_Guard clears cookies when function exits (prevents dangling pointers)
    Cookie_Guard<GT> cookie_guard(g, true, false);

    typename Net::Node *source = nullptr;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        auto q = net.insert_node();
        NODE_COOKIE(p) = q;
        if (p == source_node)
          source = q;
      }

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        auto src = mapped_node<GT, Net>(g.get_src_node(a));
        auto tgt = mapped_node<GT, Net>(g.get_tgt_node(a));
        net.insert_arc(tgt, src, typename Net::Flow_Type(1));
        net.insert_arc(src, tgt, typename Net::Flow_Type(1));
      }

    long min_k = min_degree;
    const typename Net::Flow_Type super_cap =
        static_cast<typename Net::Flow_Type>(min_degree) + 1;

    DynDlist<typename Net::Node *> sinks;
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
      {
        auto node = it.get_curr();
        if (node != source)
          sinks.append(node);
      }

    for (typename DynDlist<typename Net::Node *>::Iterator it(sinks);
         it.has_curr(); it.next_ne())
      {
        const auto sink = it.get_curr();
        const auto super_source = net.insert_node();
        const auto super_sink = net.insert_node();
        net.insert_arc(super_source, source, super_cap);
        net.insert_arc(sink, super_sink, super_cap);

        if (const typename Net::Flow_Type flow = Max_Flow<Net>()(net); flow < min_k)
          min_k = flow;

        net.remove_node(super_source);
        net.remove_node(super_sink);
        net.reset(); // reset flow to zero
      }

    return min_k;
  }

  /** @brief Functor wrapper for edge_connectivity().
   *
   * @tparam GT Graph type (undirected).
   * @tparam Max_Flow Maximum flow functor.
   *
   * @ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Heap_Preflow_Maximum_Flow>
  class Edge_Connectivity
  {
  public:
    /** @brief Compute edge connectivity of `g`.
     *
     * @param[in,out] g Graph to analyze.
     * @return Edge connectivity of `g`.
     * @throw std::domain_error If `g` is a digraph.
     * @throw std::bad_alloc If there is not enough memory.
     */
    long operator ()(GT & g) { return edge_connectivity<GT, Max_Flow>(g); }
  };


  /** @brief Compute a minimum edge cut of an undirected graph.
   *
   * Builds a unit-capacity auxiliary network and uses successive max-flow
   * computations to obtain a minimum cut. The partition and cut arcs are
   * returned through output parameters.
   *
   * The arc filter `SA` is applied when computing degrees, connectivity,
   * and when building the auxiliary network.
   *
   * Output parameters are cleared on entry.
   *
   * @note If the graph is disconnected, returns 0, `cut` is empty, and the
   *       partition `l`/`r` corresponds to the component reachable from a
   *       minimum-degree node.
   * @note If the graph is connected and the minimum filtered degree is <= 1,
   *       the cut isolates a minimum-degree node.
   *
   * Side effects:
   * - Uses the `Depth_First` control bit on nodes and arcs to test connectivity.
   * - Overwrites node cookies to map GT <-> Net nodes when the auxiliary
   *   network is built.
   * - Overwrites arc cookies on the auxiliary network to map back to GT arcs.
   *
   * @tparam GT Graph type (undirected).
   * @tparam Max_Flow Maximum flow functor.
   * @tparam SA Arc filter functor.
   *
   * @param[in,out] g Graph to analyze.
   * @param[out] l Set of nodes on the source side of the cut.
   * @param[out] r Set of nodes on the sink side of the cut.
   * @param[out] cut List of cut arcs (arcs crossing between `l` and `r`).
   * @return Edge connectivity of `g` (minimum cut size).
   * @throw std::domain_error If `g` is a digraph.
   * @throw std::bad_alloc If there is not enough memory.
   *
   * @ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Heap_Preflow_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  long compute_min_cut(GT & g,
                       DynSetTree<typename GT::Node *> & l,
                       DynSetTree<typename GT::Node *> & r,
                       DynDlist<typename GT::Arc *> & cut)
  {
    const SA sa;
    l.empty();
    r.empty();
    cut.empty();

    ah_domain_error_if(g.is_digraph())
      << "compute_min_cut() does not work on digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes == 0)
      return 0;

    typename GT::Node *source_node = nullptr;
    long min_degree = std::numeric_limits<long>::max();
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        long degree = 0;
        for (Node_Arc_Iterator<GT, SA> it_arc(p, sa); it_arc.has_curr();
             it_arc.next_ne())
          ++degree;
        if (degree < min_degree)
          {
            min_degree = degree;
            source_node = p;
          }
      }

    Depth_First_Traversal<GT, Default_Visit_Op<GT>, SA> dfs(sa);
    if (dfs(g, source_node) != num_nodes)
      {
        for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
          {
            auto p = it.get_curr();
            if (IS_NODE_VISITED(p, Depth_First))
              l.insert(p);
            else
              r.insert(p);
          }
        return 0;
      }

    if (min_degree <= 1)
      {
        if (source_node != nullptr)
          l.insert(source_node);

        for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
          {
            auto p = it.get_curr();
            if (p != source_node)
              r.insert(p);
          }

        for (Node_Arc_Iterator<GT, SA> it(source_node, sa);
             it.has_curr(); it.next_ne())
          {
            auto arc = it.get_curr();
            auto other = g.get_connected_node(arc, source_node);
            if (other != source_node)
              cut.append(arc);
          }

        return min_degree;
      }

    typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class>> Net;
    Net net;
    DynMapTreap<typename Net::Node *, typename GT::Node *> net_node_map;
    DynMapTreap<typename Net::Arc *, typename GT::Arc *> net_arc_map;
    typename Net::Node *source = nullptr;
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        auto q = net.insert_node();
        NODE_COOKIE(p) = nullptr;
        GT::map_nodes(p, q);
        net_node_map.insert(q, p);
        if (p == source_node)
          source = q;
      }

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        auto src = mapped_node<GT, Net>(g.get_src_node(a));
        auto tgt = mapped_node<GT, Net>(g.get_tgt_node(a));
        auto arc = net.insert_arc(tgt, src, static_cast<typename Net::Flow_Type>(1));
        ARC_COOKIE(arc) = a;
        net_arc_map.insert(arc, a);

        arc = net.insert_arc(src, tgt, static_cast<typename Net::Flow_Type>(1));
        ARC_COOKIE(arc) = a;
        net_arc_map.insert(arc, a);
      }

    DynSetTree<typename Net::Node *> tmp_vs, tmp_vt;
    DynList<typename Net::Arc *> tmp_cuts, tmp_cutt;
    long min_k = std::numeric_limits<long>::max();
    const typename Net::Flow_Type super_cap =
        static_cast<typename Net::Flow_Type>(min_degree) + 1;

    DynDlist<typename Net::Node *> sinks;
    for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
        if (auto node = it.get_curr(); node != source)
          sinks.append(node);

    for (typename DynDlist<typename Net::Node *>::Iterator it(sinks);
         it.has_curr(); it.next_ne())
      {
        auto sink = it.get_curr();
        auto super_source = net.insert_node();
        auto super_sink = net.insert_node();
        net.insert_arc(super_source, source, super_cap);
        net.insert_arc(sink, super_sink, super_cap);

        DynSetTree<typename Net::Node *> vs, vt;
        DynList<typename Net::Arc *> cuts, cutt;
        const auto flow = Min_Cut<Net, Max_Flow>()(net, vs, vt, cuts, cutt);

        if (flow < min_k)
          {
            DynSetTree<typename Net::Node *> vs_filtered, vt_filtered;
            DynList<typename Net::Arc *> cuts_filtered, cutt_filtered;

            for (typename DynSetTree<typename Net::Node *>::Iterator it(vs);
                 it.has_curr(); it.next_ne())
                if (auto node = it.get_curr(); net_node_map.contains(node))
                  vs_filtered.insert(node);

            for (typename DynSetTree<typename Net::Node *>::Iterator it(vt);
                 it.has_curr(); it.next_ne())
                if (auto node = it.get_curr(); net_node_map.contains(node))
                  vt_filtered.insert(node);

            for (typename DynList<typename Net::Arc *>::Iterator it(cuts);
                 it.has_curr(); it.next_ne())
                if (auto arc = it.get_curr(); net_arc_map.contains(arc))
                  cuts_filtered.append(arc);

            for (typename DynList<typename Net::Arc *>::Iterator it(cutt);
                 it.has_curr(); it.next_ne())
                if (auto arc = it.get_curr(); net_arc_map.contains(arc))
                  cutt_filtered.append(arc);

            min_k = flow;
            tmp_vs.swap(vs_filtered);
            tmp_vt.swap(vt_filtered);
            tmp_cuts.swap(cuts_filtered);
            tmp_cutt.swap(cutt_filtered);
          }

        net.remove_node(super_source);
        net.remove_node(super_sink);
        net.reset(); // reset flow to zero
      }

    for (typename DynSetTree<typename Net::Node *>::Iterator it(tmp_vs);
         it.has_curr(); it.next_ne())
      l.insert(net_node_map.find(it.get_curr()));

    for (typename DynSetTree<typename Net::Node *>::Iterator it(tmp_vt);
         it.has_curr(); it.next_ne())
      r.insert(net_node_map.find(it.get_curr()));

    for (typename DynList<typename Net::Arc *>::Iterator it(tmp_cuts);
         it.has_curr(); it.next_ne())
      {
        typename Net::Arc *arc = it.get_curr();
        cut.append(net_arc_map.find(arc));
      }

    return min_k;
  }

  /** @brief Functor wrapper for compute_min_cut().
   *
   * @tparam GT Graph type (undirected).
   * @tparam Max_Flow Maximum flow functor.
   * @tparam SA Arc filter functor.
   *
   * @ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Heap_Preflow_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  class Compute_Min_Cut
  {
  public:
    /** @brief Compute a minimum edge cut for `g`. */
    long operator ()(GT & g,
                     DynSetTree<typename GT::Node *> & l,
                     DynSetTree<typename GT::Node *> & r,
                     DynDlist<typename GT::Arc *> & cut)
    {
      return compute_min_cut<GT, Max_Flow, SA>(g, l, r, cut);
    }
  };


  /** @brief Compute vertex connectivity of an undirected graph.
   *
   * Uses a max-flow node-splitting reduction to compute the minimum number
   * of vertices whose removal disconnects the graph.
   *
   * The arc filter `SA` is applied when computing degrees, connectivity,
   * and when building the auxiliary network.
   *
   * Side effects:
   * - Uses the `Depth_First` control bit on nodes and arcs to test connectivity.
   * - Overwrites node cookies to store the GT -> Net mapping (and auxiliary
   *   mappings during the reduction) when the auxiliary network is built.
   *
   * @tparam GT Graph type (undirected).
   * @tparam Max_Flow Maximum flow functor.
   * @tparam SA Arc filter functor.
   *
   * @param[in,out] g Graph to analyze.
   * @return Vertex connectivity of `g` (0 for empty or disconnected graphs).
   * @note For connected graphs with minimum filtered degree <= 1, returns
   *       that degree without running max flow.
   * @throw std::domain_error If `g` is a digraph.
   * @throw std::bad_alloc If there is not enough memory.
   *
   * @ingroup Graphs
   */
  template <class GT,
            template <class> class Max_Flow = Random_Preflow_Maximum_Flow,
            class SA = Dft_Show_Arc<GT>>
  long vertex_connectivity(GT & g)
  {
    const SA sa;
    ah_domain_error_if(g.is_digraph())
      << "vertex_connectivity() does not work on digraphs";

    const auto num_nodes = g.get_num_nodes();
    if (num_nodes <= 1)
      return 0;

    typename GT::Node *source_node = nullptr;
    long min_degree = std::numeric_limits<long>::max();
    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        long degree = 0;
        for (Node_Arc_Iterator<GT, SA> it_arc(p, sa); it_arc.has_curr();
             it_arc.next_ne())
          ++degree;
        if (degree < min_degree)
          {
            min_degree = degree;
            source_node = p;
          }
      }

    Depth_First_Traversal<GT, Default_Visit_Op<GT>, SA> dfs(sa);
    if (dfs(g, source_node) != num_nodes)
      return 0;

    if (min_degree <= 1)
      return min_degree;

    typedef Net_Graph<Net_Node<Empty_Class>, Net_Arc<Empty_Class>> Net;
    Net net;

    // Cookie_Guard clears cookies when function exits (prevents dangling pointers)
    Cookie_Guard<GT> cookie_guard(g, true, false);

    for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
      {
        auto p = it.get_curr();
        NODE_COOKIE(p) = net.insert_node();
      }

    for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
      {
        auto a = it.get_curr();
        auto src = mapped_node<GT, Net>(g.get_src_node(a));
        auto tgt = mapped_node<GT, Net>(g.get_tgt_node(a));
        net.insert_arc(tgt, src, static_cast<typename Net::Flow_Type>(1));
        net.insert_arc(src, tgt, static_cast<typename Net::Flow_Type>(1));
      }

    long min_k = min_degree;

    for (Node_Iterator<Net> k(net); k.has_curr() and min_k > 1; k.next_ne())
      {
        auto source = k.get_curr();
        DynDlist<typename Net::Arc *> to_source_list;
        for (_In_Iterator<Net> it(source); it.has_curr(); it.next_ne())
          to_source_list.append(it.get_curr());

        for (typename DynDlist<typename Net::Arc *>::Iterator it(to_source_list);
             it.has_curr(); it.next_ne())
          net.disconnect_arc(it.get_curr());

        {
          Node_Iterator<Net> j(k);
          for (j.next(); j.has_curr(); j.next_ne())
            {
              auto sink = j.get_curr();
              if (search_arc<Net>(net, source, sink) != nullptr)
                continue; // adjacent nodes: skip to next sink

              DynDlist<typename Net::Arc *> from_sink_arcs;

              for (_Out_Iterator<Net> it(sink); it.has_curr(); it.next_ne())
                from_sink_arcs.append(it.get_curr());

              for (typename DynDlist<typename Net::Arc *>::Iterator
                   it(from_sink_arcs); it.has_curr(); it.next_ne())
                net.disconnect_arc(it.get_curr());

              {
                // Build node-splitting network to model vertex capacities.
                Net aux_net;
                {
                  DynMapTreap<typename Net::Node *, typename Net::Arc *> map;
                  for (Node_Iterator<Net> it(net); it.has_curr(); it.next_ne())
                    {
                      auto p = it.get_curr();
                      if (p == source or p == sink)
                        {
                          NODE_COOKIE(p) = aux_net.insert_node();
                          continue;
                        }

                      auto ps = aux_net.insert_node(p->get_info());
                      auto pt = aux_net.insert_node(p->get_info());
                      map.insert(p, aux_net.insert_arc(ps, pt, typename Net::Flow_Type(1)));
                    }

                  for (Arc_Iterator<Net> it(net); it.has_curr(); it.next_ne())
                    {
                      auto a = it.get_curr();
                      auto nsrc = net.get_src_node(a);
                      auto ntgt = net.get_tgt_node(a);
                      typename Net::Node *asrc = nullptr;
                      typename Net::Node *atgt = nullptr;

                      if (nsrc == source)
                        asrc = static_cast<typename Net::Node *>(NODE_COOKIE(nsrc));
                      else
                        {
                          auto arc = map.find(nsrc);
                          asrc = aux_net.get_tgt_node(arc);
                        }

                      if (ntgt == sink)
                        atgt = static_cast<typename Net::Node *>(NODE_COOKIE(ntgt));
                      else
                        {
                          auto arc = map.find(ntgt);
                          atgt = aux_net.get_src_node(arc);
                        }
                      aux_net.insert_arc(asrc, atgt, typename Net::Flow_Type(1));
                    }
                } // end mapping block

                const auto flow = Max_Flow<Net>()(aux_net);
                if (flow < min_k)
                  min_k = flow;
              }

              while (not from_sink_arcs.is_empty())
                net.connect_arc(from_sink_arcs.get());

              net.reset(); // reset flow to zero
            }

          while (not to_source_list.is_empty())
            net.connect_arc(to_source_list.get());
        }
      }

    return min_k;
  }
} // end namespace Aleph

# endif // TPL_KGRAPH_H
