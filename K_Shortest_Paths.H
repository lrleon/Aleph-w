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


/** @file K_Shortest_Paths.H
 *  @brief K-shortest path algorithms (Yen and Eppstein-style API).
 *
 *  This header provides two k-shortest path modes over Aleph graphs.
 *
 *  - `yen_k_shortest_paths(...)`: Yen's algorithm for loopless paths.
 *  - `eppstein_k_shortest_paths(...)`: General (non-loopless) k-shortest API
 *    based on deviation expansion over shortest suffixes.
 *
 *  ## Problem solved
 *
 *  Given graph @f$G=(V,E)@f$, source @f$s@f$, target @f$t@f$, and integer
 *  @f$k@f$, compute:
 *  - `Yen`: the first @f$k@f$ shortest *simple* (loopless) paths.
 *  - `Eppstein-style API`: the first @f$k@f$ shortest general paths
 *    (cycles may appear).
 *
 *  ## Assumptions
 *
 *  - Non-negative arc weights (validated at runtime).
 *  - Works with Aleph graph backends (`List_Graph`, `List_Digraph`,
 *    `List_SGraph`, `Array_Graph`, etc.) through generic graph interfaces.
 *
 *  @see Dijkstra.H For single-source shortest paths
 *  @see Johnson.H For all-pairs shortest paths on sparse graphs
 *  @ingroup Graphs
 */

# ifndef K_SHORTEST_PATHS_H
# define K_SHORTEST_PATHS_H

# include <cmath>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <Dijkstra.H>
# include <ah-errors.H>
# include <cookie_guard.H>
# include <shortest_path_common.H>
# include <htlist.H>
# include <tpl_array.H>
# include <tpl_dynMapTree.H>
# include <tpl_dynSetTree.H>

namespace Aleph
{
  /** @brief A shortest-path item returned by k-shortest algorithms.
   *
   *  @tparam GT Graph type.
   *  @tparam Cost_Type Numeric type used for path cost.
   *  @ingroup Graphs
   */
  template <class GT, typename Cost_Type>
  struct K_Shortest_Path_Item
  {
    Cost_Type total_cost = Cost_Type{0}; ///< Total path cost.
    Path<GT> path; ///< Path from source to target.
  };


  namespace k_shortest_paths_detail
  {
    template <class GT, typename Cost_Type>
    struct Path_State
    {
      DynList<typename GT::Node *> nodes;
      DynList<typename GT::Arc *> arcs;
      Cost_Type total_cost = Cost_Type{0};
    };


    template <class GT, typename Cost_Type>
    struct Suffix_Index
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

      Cost_Type inf = std::numeric_limits<Cost_Type>::max();
      DynMapTree<Node *, Cost_Type> dist_to_target;
      DynMapTree<Node *, Node *> tree_next;
      DynMapTree<Node *, Arc *> tree_arc;
    };


    template <class GT, class SA>
    struct Filtered_Show_Arc
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

      const GT *g = nullptr;
      SA base_sa;
      const DynSetTree<Node *> *forbidden_nodes = nullptr;
      const DynSetTree<Arc *> *forbidden_arcs = nullptr;

      bool operator()(Arc *arc) const
      {
        if (not base_sa(arc))
          return false;

        if (forbidden_arcs != nullptr and forbidden_arcs->contains(arc))
          return false;

        if (forbidden_nodes != nullptr)
          {
            Node *src = g->get_src_node(arc);
            Node *tgt = g->get_tgt_node(arc);
            if (forbidden_nodes->contains(src) or forbidden_nodes->contains(tgt))
              return false;
          }

        return true;
      }
    };


    template <class GT, class SA>
    class In_Iterator_Adapter
    {
      In_Iterator<GT, SA> it_;

    public:
      In_Iterator_Adapter() = default;

      In_Iterator_Adapter(typename GT::Node * p, SA sa = SA())
        : it_(p, sa)
      {
        // empty
      }

      bool has_curr() const noexcept
      {
        return it_.has_curr();
      }

      void next_ne() noexcept
      {
        it_.next_ne();
      }

      typename GT::Arc * get_current_arc_ne() const noexcept
      {
        return it_.get_curr_ne();
      }

      typename GT::Node * get_tgt_node() const
      {
        return it_.get_tgt_node();
      }
    };


    template <class GT, class Distance>
    struct Reverse_Arc_Distance
    {
      using Arc = typename GT::Arc;
      using Distance_Type = typename Distance::Distance_Type;

      // Context for reverse-graph Dijkstra in build_suffix_index_digraph().
      // This is intentionally thread-local, but not re-entrant on the same
      // thread: nested calls can overwrite map_ptr/base_ptr. The
      // Reset_Reverse_Distance guard below restores this state.
      inline static thread_local const DynMapTree<Arc *, Arc *> * map_ptr = nullptr;
      inline static thread_local const Distance * base_ptr = nullptr;

      Distance_Type operator()(Arc * rev_arc) const
      {
        ah_runtime_error_if(map_ptr == nullptr)
          << "Reverse_Arc_Distance: reverse arc map is null";
        ah_runtime_error_if(base_ptr == nullptr)
          << "Reverse_Arc_Distance: base distance functor is null";

        auto * pair = map_ptr->search(rev_arc);
        auto * orig_arc = pair == nullptr ? nullptr : pair->second;
        ah_runtime_error_if(orig_arc == nullptr)
          << "Reverse_Arc_Distance: missing reverse-to-original arc map";

        return (*base_ptr)(orig_arc);
      }
    };


    template <class GT, class Distance, class SA>
    void validate_non_negative_weights(const GT & g,
                                       Distance distance,
                                       SA sa)
    {
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;

      static_assert(std::is_arithmetic_v<Cost_Type>,
                    "K shortest paths require arithmetic arc costs");

      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          Arc *arc = it.get_curr_ne();
          const Cost_Type w = distance(arc);

          if constexpr (std::is_floating_point_v<Cost_Type>)
            ah_domain_error_if(not std::isfinite(w))
              << "K shortest paths require finite arc weights";

          ah_domain_error_if(w < Cost_Type{0})
            << "K shortest paths require non-negative arc weights";
        }
    }


    template <class GT, typename Cost_Type>
    struct Path_Snapshot
    {
      Array<typename GT::Node *> nodes;
      Array<typename GT::Arc *> arcs;
    };


    template <class GT, typename Cost_Type>
    Path_Snapshot<GT, Cost_Type>
    make_path_snapshot(const Path_State<GT, Cost_Type> & path)
    {
      Path_Snapshot<GT, Cost_Type> snapshot;
      snapshot.nodes.reserve(path.nodes.size());
      snapshot.arcs.reserve(path.arcs.size());

      for (auto it = path.nodes.get_it(); it.has_curr(); it.next_ne())
        snapshot.nodes.append(it.get_curr_ne());

      for (auto it = path.arcs.get_it(); it.has_curr(); it.next_ne())
        snapshot.arcs.append(it.get_curr_ne());

      return snapshot;
    }


    template <class GT, typename Cost_Type>
    Path_State<GT, Cost_Type>
    path_to_state(const Path<GT> & path, const Cost_Type total_cost)
    {
      Path_State<GT, Cost_Type> state;
      state.total_cost = total_cost;

      for (typename Path<GT>::Iterator it(path); it.has_current_node(); it.next_ne())
        {
          state.nodes.append(it.get_current_node_ne());
          if (it.has_current_arc())
            state.arcs.append(it.get_current_arc_ne());
        }

      return state;
    }


    template <class GT, typename Cost_Type>
    Path<GT> state_to_path(const GT & g, const Path_State<GT, Cost_Type> & state)
    {
      Path<GT> path(g);
      if (state.nodes.is_empty())
        return path;

      path.set_graph(g, state.nodes.get_first());

      if (g.is_digraph())
        {
          for (typename DynList<typename GT::Arc *>::Iterator it(state.arcs);
               it.has_curr(); it.next_ne())
            path.append_directed(it.get_curr_ne());
        }
      else
        {
          for (typename DynList<typename GT::Arc *>::Iterator it(state.arcs);
               it.has_curr(); it.next_ne())
            path.append(it.get_curr_ne());
        }

      return path;
    }


    template <class GT, typename Cost_Type>
    bool same_path_sequence(const Path_State<GT, Cost_Type> & a,
                            const Path_State<GT, Cost_Type> & b)
    {
      if (a.nodes.size() != b.nodes.size())
        return false;
      if (a.arcs.size() != b.arcs.size())
        return false;

      auto an = a.nodes.get_it();
      auto bn = b.nodes.get_it();
      while (an.has_curr())
        {
          if (an.get_curr_ne() != bn.get_curr_ne())
            return false;
          an.next_ne();
          bn.next_ne();
        }

      auto aa = a.arcs.get_it();
      auto ba = b.arcs.get_it();
      while (aa.has_curr())
        {
          if (aa.get_curr_ne() != ba.get_curr_ne())
            return false;
          aa.next_ne();
          ba.next_ne();
        }

      return true;
    }


    template <class GT, typename Cost_Type>
    bool same_prefix_nodes(const Path_Snapshot<GT, Cost_Type> & a,
                           const Path_Snapshot<GT, Cost_Type> & b,
                           const size_t spur_index)
    {
      if (a.nodes.size() <= spur_index or b.nodes.size() <= spur_index)
        return false;
      if (a.arcs.size() < spur_index or b.arcs.size() < spur_index)
        return false;

      for (size_t i = 0; i <= spur_index; ++i)
        if (a.nodes[i] != b.nodes[i])
          return false;

      for (size_t i = 0; i < spur_index; ++i)
        if (a.arcs[i] != b.arcs[i])
          return false;

      return true;
    }


    template <class GT, typename Cost_Type>
    bool contains_path(const Array<Path_State<GT, Cost_Type>> & container,
                       const Path_State<GT, Cost_Type> & candidate)
    {
      for (const auto & p: container)
        if (same_path_sequence(p, candidate))
          return true;
      return false;
    }


    template <class GT, typename Cost_Type>
    Cost_Type
    get_dist_to_target(const Suffix_Index<GT, Cost_Type> & index,
                       typename GT::Node * node)
    {
      auto * p = index.dist_to_target.search(node);
      if (p == nullptr)
        return index.inf;
      return p->second;
    }


    template <class GT, typename Cost_Type>
    typename GT::Node *
    get_tree_next(const Suffix_Index<GT, Cost_Type> & index,
                  typename GT::Node * node)
    {
      auto * p = index.tree_next.search(node);
      return p == nullptr ? nullptr : p->second;
    }


    template <class GT, typename Cost_Type>
    typename GT::Arc *
    get_tree_arc(const Suffix_Index<GT, Cost_Type> & index,
                 typename GT::Node * node)
    {
      auto * p = index.tree_arc.search(node);
      return p == nullptr ? nullptr : p->second;
    }


    template <template <class, class> class Itor,
              class GT, class Distance, class SA>
    Suffix_Index<GT, typename Distance::Distance_Type>
    build_suffix_index_with_itor(const GT & g,
                                 typename GT::Node * target,
                                 Distance distance,
                                 SA sa)
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;

      Suffix_Index<GT, Cost_Type> index;

      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
        {
          Node * node = it.get_curr_ne();
          index.dist_to_target.insert(node, index.inf);
          index.tree_next.insert(node, nullptr);
          index.tree_arc.insert(node, nullptr);
        }

      auto & mg = const_cast<GT &>(g);
      Cookie_Saver<GT> cookie_saver(mg, true, true);
      mg.reset_nodes();
      mg.reset_arcs();

      Dijkstra_Min_Paths<GT, Distance, Itor, SA> reverse_dijkstra(distance, sa);
      reverse_dijkstra.paint_min_paths_tree(g, target);

      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
        {
          Node * node = it.get_curr_ne();
          if (not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
            continue;

          index.dist_to_target.find(node) = reverse_dijkstra.get_distance(node);

          Node * parent = static_cast<Node *>(NODE_COOKIE(node));
          index.tree_next.find(node) = parent;

          if (parent == nullptr)
            continue;

          Arc * selected = nullptr;
          for (Node_Arc_Iterator<GT, SA> ait(node, sa); ait.has_curr(); ait.next_ne())
            {
              Arc * arc = ait.get_current_arc_ne();
              if (ait.get_tgt_node() == parent and IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                {
                  selected = arc;
                  break;
                }
            }

          if (selected == nullptr)
            {
              const Cost_Type dist_node = index.dist_to_target.find(node);
              const Cost_Type dist_parent = index.dist_to_target.find(parent);
              for (Node_Arc_Iterator<GT, SA> ait(node, sa); ait.has_curr(); ait.next_ne())
                {
                  Arc * arc = ait.get_current_arc_ne();
                  if (ait.get_tgt_node() != parent)
                    continue;

                  const Cost_Type cand =
                      shortest_path_detail::checked_add(distance(arc), dist_parent);
                  if (cand == dist_node)
                    {
                      selected = arc;
                      break;
                    }
                }
            }

          index.tree_arc.find(node) = selected;
        }

      return index;
    }


    template <class GT, class Distance, class SA>
    Suffix_Index<GT, typename Distance::Distance_Type>
    build_suffix_index_digraph(const GT & g,
                               typename GT::Node * target,
                               Distance distance,
                               SA sa)
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;

      Suffix_Index<GT, Cost_Type> index;

      GT reverse_graph;
      DynMapTree<Node *, Node *> orig_to_rev;
      DynMapTree<Node *, Node *> rev_to_orig;
      DynMapTree<Arc *, Arc *> rev_arc_to_orig;

      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne())
        {
          Node * node = it.get_curr_ne();
          index.dist_to_target.insert(node, index.inf);
          index.tree_next.insert(node, nullptr);
          index.tree_arc.insert(node, nullptr);

          Node * rev_node = reverse_graph.insert_node(node->get_info());
          orig_to_rev.insert(node, rev_node);
          rev_to_orig.insert(rev_node, node);
        }

      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_curr_ne();
          Node * src = g.get_src_node(arc);
          Node * tgt = g.get_tgt_node(arc);

          Arc * rev_arc = reverse_graph.insert_arc(
              orig_to_rev.find(tgt),
              orig_to_rev.find(src),
              arc->get_info());
          rev_arc_to_orig.insert(rev_arc, arc);
        }

      using Reverse_Distance = Reverse_Arc_Distance<GT, Distance>;
      using Rev_Show_Arc = Dft_Show_Arc<GT>;
      Reverse_Distance::map_ptr = &rev_arc_to_orig;
      Reverse_Distance::base_ptr = &distance;
      // Restores thread-local reverse-distance context on scope exit.
      // Nested/re-entrant use on the same thread is not supported.
      struct Reset_Reverse_Distance
      {
        ~Reset_Reverse_Distance()
        {
          Reverse_Distance::map_ptr = nullptr;
          Reverse_Distance::base_ptr = nullptr;
        }
      } reset_reverse_distance;

      Cookie_Saver<GT> reverse_cookie_saver(reverse_graph, true, true);
      reverse_graph.reset_nodes();
      reverse_graph.reset_arcs();

      Dijkstra_Min_Paths<GT, Reverse_Distance, Node_Arc_Iterator, Rev_Show_Arc>
          reverse_dijkstra{Reverse_Distance(), Rev_Show_Arc()};

      Node * rev_target = orig_to_rev.find(target);
      reverse_dijkstra.paint_min_paths_tree(reverse_graph, rev_target);

      for (Node_Iterator<GT> rit(reverse_graph); rit.has_curr(); rit.next_ne())
        {
          Node * rev_node = rit.get_curr_ne();
          Node * orig_node = rev_to_orig.find(rev_node);

          if (not IS_NODE_VISITED(rev_node, Aleph::Spanning_Tree))
            continue;

          index.dist_to_target.find(orig_node) = reverse_dijkstra.get_distance(rev_node);

          Node * rev_parent = static_cast<Node *>(NODE_COOKIE(rev_node));
          if (rev_parent == nullptr)
            continue;

          Node * orig_parent = rev_to_orig.find(rev_parent);
          index.tree_next.find(orig_node) = orig_parent;

          Arc * selected = nullptr;
          const Cost_Type dist_u = index.dist_to_target.find(orig_node);
          const Cost_Type dist_v = index.dist_to_target.find(orig_parent);
          for (Node_Arc_Iterator<GT, SA> ait(orig_node, sa); ait.has_curr(); ait.next_ne())
            {
              Arc * arc = ait.get_current_arc_ne();
              if (ait.get_tgt_node() != orig_parent)
                continue;

              if (selected == nullptr)
                selected = arc;

              if (dist_v != index.inf)
                {
                  const Cost_Type cand =
                      shortest_path_detail::checked_add(distance(arc), dist_v);
                  if (cand == dist_u)
                    {
                      selected = arc;
                      break;
                    }
                }
            }

          index.tree_arc.find(orig_node) = selected;
        }

      return index;
    }


    template <class GT, class Distance, class SA>
    Suffix_Index<GT, typename Distance::Distance_Type>
    build_suffix_index(const GT & g,
                       typename GT::Node * target,
                       Distance distance,
                       SA sa)
    {
      if (g.is_digraph())
        return build_suffix_index_digraph<GT, Distance, SA>(
            g, target, distance, sa);

      return build_suffix_index_with_itor<Node_Arc_Iterator, GT, Distance, SA>(
          g, target, distance, sa);
    }


    template <class GT, typename Cost_Type>
    bool build_suffix_state(const GT & g,
                            const Suffix_Index<GT, Cost_Type> & index,
                            typename GT::Node * start,
                            typename GT::Node * target,
                            Path_State<GT, Cost_Type> & out_suffix)
    {
      out_suffix = Path_State<GT, Cost_Type>();
      out_suffix.nodes.append(start);
      out_suffix.total_cost = get_dist_to_target(index, start);
      if (out_suffix.total_cost == index.inf)
        return false;

      typename GT::Node * curr = start;
      size_t guard = 0;
      const size_t max_steps = g.get_num_nodes() + 1;

      while (curr != target)
        {
          if (++guard > max_steps)
            return false;

          auto * next = get_tree_next(index, curr);
          auto * arc = get_tree_arc(index, curr);
          if (next == nullptr or arc == nullptr)
            return false;

          out_suffix.arcs.append(arc);
          out_suffix.nodes.append(next);
          curr = next;
        }

      return true;
    }


    template <class GT, class Distance, class SA>
    bool shortest_path_filtered(const GT & g,
                                typename GT::Node *source,
                                typename GT::Node *target,
                                Distance distance,
                                SA sa,
                                const DynSetTree<typename GT::Node *> *forbidden_nodes,
                                const DynSetTree<typename GT::Arc *> *forbidden_arcs,
                                Path_State<GT, typename Distance::Distance_Type> & out)
    {
      using Cost_Type = typename Distance::Distance_Type;
      using Filter = Filtered_Show_Arc<GT, SA>;

      if (forbidden_nodes != nullptr and
          (forbidden_nodes->contains(source) or forbidden_nodes->contains(target)))
        return false;

      if (source == target)
        {
          out = Path_State<GT, Cost_Type>();
          out.total_cost = Cost_Type{0};
          out.nodes.append(source);
          return true;
        }

      Cookie_Saver<GT> cookie_saver(const_cast<GT &>(g), true, true);
      const_cast<GT &>(g).reset_nodes();
      const_cast<GT &>(g).reset_arcs();

      Filter filter{&g, sa, forbidden_nodes, forbidden_arcs};
      Dijkstra_Min_Paths<GT, Distance, Node_Arc_Iterator, Filter> dijkstra(distance, filter);

      Path<GT> shortest(g);
      const Cost_Type dist = dijkstra(g, source, target, shortest);
      if (dist == std::numeric_limits<Cost_Type>::max() or shortest.is_empty())
        return false;

      out = path_to_state<GT, Cost_Type>(shortest, dist);
      return true;
    }


    template <class GT, class Distance>
    typename Distance::Distance_Type
    compute_cost_from_arcs(const DynList<typename GT::Arc *> & arcs,
                           Distance distance)
    {
      using Cost_Type = typename Distance::Distance_Type;

      Cost_Type total = Cost_Type{0};
      for (typename DynList<typename GT::Arc *>::Iterator it(arcs);
           it.has_curr(); it.next_ne())
        total = shortest_path_detail::checked_add(total, distance(it.get_curr_ne()));
      return total;
    }


    template <class GT, typename Cost_Type>
    Path_State<GT, Cost_Type>
    compose_candidate(const Path_Snapshot<GT, Cost_Type> & base_path,
                      const size_t spur_index,
                      const Path_State<GT, Cost_Type> & spur_state)
    {
      Path_State<GT, Cost_Type> candidate;

      for (size_t i = 0; i <= spur_index; ++i)
        candidate.nodes.append(base_path.nodes[i]);

      for (size_t i = 0; i < spur_index; ++i)
        candidate.arcs.append(base_path.arcs[i]);

      bool first_spur_node = true;
      for (typename DynList<typename GT::Node *>::Iterator it(spur_state.nodes);
           it.has_curr(); it.next_ne())
        {
          if (first_spur_node)
            {
              first_spur_node = false;
              continue;
            }

          candidate.nodes.append(it.get_curr_ne());
        }

      for (typename DynList<typename GT::Arc *>::Iterator it(spur_state.arcs);
           it.has_curr(); it.next_ne())
        candidate.arcs.append(it.get_curr_ne());

      return candidate;
    }


    template <class GT, typename Cost_Type>
    Path_State<GT, Cost_Type>
    compose_general_candidate(const Path_Snapshot<GT, Cost_Type> & base_path,
                              const size_t spur_index,
                              typename GT::Arc * deviation_arc,
                              typename GT::Node * deviation_next_node,
                              const Path_State<GT, Cost_Type> & suffix_state)
    {
      Path_State<GT, Cost_Type> candidate;

      for (size_t i = 0; i <= spur_index; ++i)
        candidate.nodes.append(base_path.nodes[i]);

      for (size_t i = 0; i < spur_index; ++i)
        candidate.arcs.append(base_path.arcs[i]);

      candidate.arcs.append(deviation_arc);
      candidate.nodes.append(deviation_next_node);

      bool first_suffix_node = true;
      for (typename DynList<typename GT::Node *>::Iterator it(suffix_state.nodes);
           it.has_curr(); it.next_ne())
        {
          if (first_suffix_node)
            {
              first_suffix_node = false;
              continue;
            }

          candidate.nodes.append(it.get_curr_ne());
        }

      for (typename DynList<typename GT::Arc *>::Iterator it(suffix_state.arcs);
           it.has_curr(); it.next_ne())
        candidate.arcs.append(it.get_curr_ne());

      return candidate;
    }


    template <class GT, class Distance, class SA, typename Cost_Type>
    void generate_general_deviation_candidates(
        const GT & g,
        const Path_State<GT, Cost_Type> & base_path,
        typename GT::Node * target,
        const Suffix_Index<GT, Cost_Type> & suffix_index,
        Distance distance,
        SA sa,
        const Array<Path_State<GT, Cost_Type>> & accepted,
        Array<Path_State<GT, Cost_Type>> & candidates)
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;

      if (base_path.arcs.is_empty())
        return;

      const Path_Snapshot<GT, Cost_Type> base_snapshot =
          make_path_snapshot<GT, Cost_Type>(base_path);

      Cost_Type prefix_cost = Cost_Type{0};
      const size_t arc_count = base_snapshot.arcs.size();

      for (size_t spur_index = 0; spur_index < arc_count; ++spur_index)
        {
          Node * spur_node = base_snapshot.nodes[spur_index];
          Arc * base_arc = base_snapshot.arcs[spur_index];

          for (Node_Arc_Iterator<GT, SA> it(spur_node, sa); it.has_curr(); it.next_ne())
            {
              Arc * deviation_arc = it.get_current_arc_ne();
              if (deviation_arc == base_arc)
                continue;

              Node * deviation_next_node = it.get_tgt_node();
              const Cost_Type suffix_dist =
                  get_dist_to_target(suffix_index, deviation_next_node);
              if (suffix_dist == suffix_index.inf)
                continue;

              Path_State<GT, Cost_Type> suffix_state;
              if (not build_suffix_state(g,
                                         suffix_index,
                                         deviation_next_node,
                                         target,
                                         suffix_state))
                continue;

              Path_State<GT, Cost_Type> candidate =
                  compose_general_candidate<GT, Cost_Type>(
                      base_snapshot,
                      spur_index,
                      deviation_arc,
                      deviation_next_node,
                      suffix_state);

              candidate.total_cost = shortest_path_detail::checked_add(
                  shortest_path_detail::checked_add(
                      prefix_cost,
                      distance(deviation_arc)),
                  suffix_dist);

              if (not contains_path(accepted, candidate)
                  and not contains_path(candidates, candidate))
                candidates.append(candidate);
            }

          prefix_cost = shortest_path_detail::checked_add(
              prefix_cost,
              distance(base_arc));
        }
    }
  } // namespace k_shortest_paths_detail


  /** @brief Compute k shortest loopless paths using Yen's algorithm.
   *
   *  Returns up to `k` simple paths ordered by non-decreasing total cost.
   *  If fewer than `k` paths exist, all available paths are returned.
   *
   *  @tparam GT Graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter.
   *
   *  @param[in] g Graph.
   *  @param[in] source Source node.
   *  @param[in] target Target node.
   *  @param[in] k Maximum number of paths to return.
   *  @param[in] distance Arc cost accessor.
   *  @param[in] sa Arc filter.
   *
   *  @return `DynList<K_Shortest_Path_Item<GT, Cost_Type>>`.
   *
   *  @exception domain_error If `source` or `target` is null, or if negative
   *                          weights are detected.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  DynList<K_Shortest_Path_Item<GT, typename Distance::Distance_Type>>
  yen_k_shortest_paths(const GT & g,
                       typename GT::Node *source,
                       typename GT::Node *target,
                       const size_t k,
                       Distance distance = Distance(),
                       SA sa = SA())
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    using Cost_Type = typename Distance::Distance_Type;
    using Item = K_Shortest_Path_Item<GT, Cost_Type>;
    using State = k_shortest_paths_detail::Path_State<GT, Cost_Type>;

    DynList<Item> result;
    if (k == 0)
      return result;

    ah_domain_error_if(source == nullptr) << "yen_k_shortest_paths(): source is null";
    ah_domain_error_if(target == nullptr) << "yen_k_shortest_paths(): target is null";

    k_shortest_paths_detail::validate_non_negative_weights(g, distance, sa);

    if (source == target)
      {
        Item item;
        item.total_cost = Cost_Type{0};
        item.path = Path<GT>(g, source);
        result.append(item);
        return result;
      }

    Array<State> accepted;
    Array<State> candidates;
    accepted.reserve(k);
    candidates.reserve(k * 2 + 1);

    State first;
    if (not k_shortest_paths_detail::shortest_path_filtered<GT, Distance, SA>(
                                                                              g, source, target, distance, sa, nullptr,
                                                                              nullptr, first))
      return result;

    accepted.append(first);

    for (size_t kth = 1; kth < k; ++kth)
      {
        const State & previous = accepted.get_last();
        const auto previous_snapshot =
            k_shortest_paths_detail::make_path_snapshot<GT, Cost_Type>(previous);
        if (previous_snapshot.nodes.size() < 2)
          break;

        Array<k_shortest_paths_detail::Path_Snapshot<GT, Cost_Type>>
            accepted_snapshots;
        accepted_snapshots.reserve(accepted.size());
        for (const State & p: accepted)
          accepted_snapshots.append(
              k_shortest_paths_detail::make_path_snapshot<GT, Cost_Type>(p));

        for (size_t spur_index = 0; spur_index + 1 < previous_snapshot.nodes.size();
             ++spur_index)
          {
            Node *spur_node = previous_snapshot.nodes[spur_index];

            DynSetTree<Node *> forbidden_nodes;
            DynSetTree<Arc *> forbidden_arcs;

            for (size_t i = 0; i < spur_index; ++i)
              forbidden_nodes.insert(previous_snapshot.nodes[i]);

            for (size_t p_idx = 0; p_idx < accepted_snapshots.size(); ++p_idx)
              {
                const auto & p_snapshot = accepted_snapshots[p_idx];
                if (k_shortest_paths_detail::same_prefix_nodes(
                        p_snapshot, previous_snapshot, spur_index))
                  forbidden_arcs.insert(p_snapshot.arcs[spur_index]);
              }

            State spur_state;
            if (not k_shortest_paths_detail::shortest_path_filtered<GT, Distance, SA>(
               g,
               spur_node,
               target,
               distance,
               sa,
               &forbidden_nodes,
               &forbidden_arcs,
               spur_state))
              continue;

            State candidate =
                k_shortest_paths_detail::compose_candidate(
                    previous_snapshot, spur_index, spur_state);
            candidate.total_cost =
                k_shortest_paths_detail::compute_cost_from_arcs<GT, Distance>(
                                                                              candidate.arcs, distance);

            if (not k_shortest_paths_detail::contains_path(accepted, candidate)
                and not k_shortest_paths_detail::contains_path(candidates, candidate))
              candidates.append(candidate);
          }

        if (candidates.is_empty())
          break;

        size_t best_idx = 0;
        for (size_t i = 1; i < candidates.size(); ++i)
          if (candidates[i].total_cost < candidates[best_idx].total_cost)
            best_idx = i;

        State best = candidates[best_idx];
        if (best_idx + 1 < candidates.size())
          candidates[best_idx] = candidates.get_last();
        const auto removed_candidate = candidates.remove_last();
        (void) removed_candidate;

        accepted.append(best);
      }

    for (const State & state: accepted)
      {
        Item item;
        item.total_cost = state.total_cost;
        item.path = k_shortest_paths_detail::state_to_path(g, state);
        result.append(item);
      }

    return result;
  }


  /** @brief General (non-loopless) k-shortest-path API.
   *
   *  This routine is separated from Yen's loopless engine:
   *  - `yen_k_shortest_paths()` returns simple (cycle-free) paths.
   *  - `eppstein_k_shortest_paths()` may return paths with repeated
   *    vertices/arcs when they are among the next best alternatives.
   *
   *  The method expands complete path candidates by deviations over each
   *  accepted path and reconnects each deviation through a shortest suffix
   *  to `target`.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  DynList<K_Shortest_Path_Item<GT, typename Distance::Distance_Type>>
  eppstein_k_shortest_paths(const GT & g,
                            typename GT::Node *source,
                            typename GT::Node *target,
                            const size_t k,
                            Distance distance = Distance(),
                            SA sa = SA())
  {
    using Cost_Type = typename Distance::Distance_Type;
    using Item = K_Shortest_Path_Item<GT, Cost_Type>;
    using State = k_shortest_paths_detail::Path_State<GT, Cost_Type>;

    DynList<Item> result;
    if (k == 0)
      return result;

    ah_domain_error_if(source == nullptr) << "eppstein_k_shortest_paths(): source is null";
    ah_domain_error_if(target == nullptr) << "eppstein_k_shortest_paths(): target is null";

    k_shortest_paths_detail::validate_non_negative_weights(g, distance, sa);

    if (source == target)
      {
        Item item;
        item.total_cost = Cost_Type{0};
        item.path = Path<GT>(g, source);
        result.append(item);
        return result;
      }

    Array<State> accepted;
    Array<State> candidates;
    accepted.reserve(k);
    candidates.reserve(k * 3 + 1);

    const auto suffix_index =
        k_shortest_paths_detail::build_suffix_index<GT, Distance, SA>(
            g, target, distance, sa);

    State first;
    if (not k_shortest_paths_detail::build_suffix_state<GT, Cost_Type>(
            g, suffix_index, source, target, first))
      return result;

    accepted.append(first);
    k_shortest_paths_detail::generate_general_deviation_candidates<GT, Distance, SA, Cost_Type>(
        g, first, target, suffix_index, distance, sa, accepted, candidates);

    while (accepted.size() < k and not candidates.is_empty())
      {
        size_t best_idx = 0;
        for (size_t i = 1; i < candidates.size(); ++i)
          if (candidates[i].total_cost < candidates[best_idx].total_cost)
            best_idx = i;

        State best = candidates[best_idx];
        if (best_idx + 1 < candidates.size())
          candidates[best_idx] = candidates.get_last();
        const auto removed_candidate = candidates.remove_last();
        (void) removed_candidate;

        if (k_shortest_paths_detail::contains_path(accepted, best))
          continue;

        accepted.append(best);
        k_shortest_paths_detail::generate_general_deviation_candidates<GT, Distance, SA, Cost_Type>(
            g, best, target, suffix_index, distance, sa, accepted, candidates);
      }

    for (const State & state: accepted)
      {
        Item item;
        item.total_cost = state.total_cost;
        item.path = k_shortest_paths_detail::state_to_path(g, state);
        result.append(item);
      }

    return result;
  }


  /** @brief Functor wrapper for Yen k-shortest paths.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Yen_K_Shortest_Paths
  {
    Distance distance_;
    SA sa_;

  public:
    Yen_K_Shortest_Paths(Distance distance = Distance(),
                         SA sa = SA())
      : distance_(std::move(distance)),
        sa_(std::move(sa))
    {
      // empty
    }

    DynList<K_Shortest_Path_Item<GT, typename Distance::Distance_Type>>
    operator()(const GT & g,
               typename GT::Node *source,
               typename GT::Node *target,
               const size_t k) const
    {
      return yen_k_shortest_paths<GT, Distance, SA>(
                                                    g, source, target, k, distance_, sa_);
    }
  };


  /** @brief Functor wrapper for Eppstein-style k-shortest paths API.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class SA = Dft_Show_Arc<GT>>
  class Eppstein_K_Shortest_Paths
  {
    Distance distance_;
    SA sa_;

  public:
    Eppstein_K_Shortest_Paths(Distance distance = Distance(),
                              SA sa = SA())
      : distance_(std::move(distance)),
        sa_(std::move(sa))
    {
      // empty
    }

    DynList<K_Shortest_Path_Item<GT, typename Distance::Distance_Type>>
    operator()(const GT & g,
               typename GT::Node *source,
               typename GT::Node *target,
               const size_t k) const
    {
      return eppstein_k_shortest_paths<GT, Distance, SA>(
                                                         g, source, target, k, distance_, sa_);
    }
  };
} // namespace Aleph

# endif // K_SHORTEST_PATHS_H
