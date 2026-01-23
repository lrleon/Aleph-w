
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


/** @file AStar.H
 *  @brief A* shortest path algorithm.
 *
 *  Implements the A* algorithm for finding the shortest path between two
 *  vertices in a weighted graph using a heuristic function. The algorithm
 *  uses f(n) = g(n) + h(n) where g(n) is the cost from start and h(n) is
 *  the heuristic estimate to the goal. Uses binary heap for O((V+E)log V).
 *
 *  The heuristic must be admissible (never overestimate) for optimal paths.
 *
 *  When no goal is specified (e.g., compute_min_paths_tree), the algorithm
 *  behaves like Dijkstra's algorithm (zero heuristic).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef ASTAR_H
#define ASTAR_H

#include <iostream>
#include <ahFunction.H>
#include <ah-errors.H>
#include <archeap.H>
#include <tpl_find_path.H>
#include <tpl_agraph.H>
#include <shortest_path_common.H>

namespace Aleph
{

/** @brief Default heuristic for A* (zero heuristic, degrades to Dijkstra).
 *
 *  This is a trivial heuristic that always returns 0. When used,
 *  A* behaves identically to Dijkstra's algorithm.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance accessor functor.
 *
 *  @ingroup Graphs
 */
template <class GT, class Distance = Dft_Dist<GT>>
struct Zero_Heuristic
{
  using Distance_Type = typename Distance::Distance_Type;

  Distance_Type operator()(typename GT::Node*, typename GT::Node*) const
  {
    return Distance_Type(0);
  }
};

/** @brief A* algorithm for finding the shortest path between two nodes.
 *
 *  @details This class implements the A* algorithm to find the shortest
 *  path from a source node `start` to a destination node `end` in a
 *  weighted graph.
 *
 *  A* uses a heuristic function h(n) that estimates the cost from node n
 *  to the goal. The algorithm expands nodes in order of f(n) = g(n) + h(n)
 *  where g(n) is the actual cost from start to n.
 *
 *  For the algorithm to find optimal paths, the heuristic must be
 *  *admissible* (never overestimate the true cost) and *consistent*
 *  (satisfy h(n) <= d(n,m) + h(m) for any edge n->m).
 *
 *  This class also provides Dijkstra-compatible methods (compute_min_paths_tree,
 *  paint_min_paths_tree) that use zero heuristic for computing all shortest
 *  paths from a source.
 *
 *  ## Method Categories
 *
 *  **A* Methods (with heuristic, for single-target search):**
 *  - `find_path()` - Find shortest path start->end (recommended)
 *  - `compute_partial_path()` - Same but builds tree structure
 *  - `paint_partial_path()` - Same but marks on original graph
 *
 *  **Dijkstra Methods (no heuristic, for multi-target search):**
 *  - `find_min_path()` - Find path without heuristic
 *  - `compute_min_paths_tree()` - Build complete shortest paths tree
 *  - `paint_min_paths_tree()` - Mark complete tree on graph
 *  - `compute_partial_min_paths_tree()` - Build partial tree until target found
 *  - `paint_partial_min_paths_tree()` - Mark partial tree on graph
 *
 *  The "partial" methods stop as soon as the target node is reached,
 *  while "complete" methods explore all reachable nodes.
 *
 *  The class receives the following template parameters:
 *  -# `GT`: graph type.
 *  -# `Distance<GT>`: class reading the weight.
 *  -# `Heuristic<GT, Distance>`: heuristic functor.
 *  -# `Itor<GT, SA>`: arc iterator type (defaults to Node_Arc_Iterator).
 *  -# `SA`: arc filter for internal iterators.
 *  -# `HeapT<GT, Distance, Access>`: priority-queue adapter.
 *
 *  @note To use a Fibonacci heap, pass ArcFibonacciHeap as HeapT.
 *
 *  @note **Reusability**: This class can be reused for multiple searches.
 *        Each call to find_path(), compute_*(), or paint_*() clears state.
 *
 *  @warning This class is **not thread-safe**. Concurrent calls require
 *           external synchronization.
 *
 *  @warning **Negative weights**: This algorithm does NOT support graphs
 *           with negative edge weights. Use Bellman-Ford for such graphs.
 *           Behavior with negative weights is undefined.
 *
 *  @warning **Inadmissible heuristics**: If the heuristic overestimates
 *           (violates admissibility), A* may return suboptimal paths.
 *           In debug mode, validation checks are performed when possible.
 *
 *  @see Dijkstra_Min_Paths Floyd_All_Shortest_Paths Bellman_Ford_Min_Paths
 *
 *  @ingroup Graphs
 */
template <class GT,
          class Distance = Dft_Dist<GT>,
          class Heuristic = Zero_Heuristic<GT, Distance>,
          template <typename, class> class Itor = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>,
          template <class, class, class> class HeapT = ArcHeap>
class AStar_Min_Path
  : public Shortest_Path_Base<GT, Distance, Itor, SA, HeapT>
{
  using Base = Shortest_Path_Base<GT, Distance, Itor, SA, HeapT>;

  // Import types from base class
  using typename Base::Node_Info;
  using typename Base::Tree_Node_Info;
  using typename Base::Arc_Info;
  using typename Base::Tree_Arc_Info;
  using typename Base::Initialize_Node;
  using typename Base::Initialize_Arc;
  using typename Base::Initialize_Tree_Node;
  using typename Base::Initialize_Tree_Arc;
  using typename Base::Destroy_Node;
  using typename Base::Destroy_Arc;
  using typename Base::Destroy_Tree_Node;
  using typename Base::Destroy_Tree_Arc;

  // Local cookie access macros
#define ANassert(p) (static_cast<Node_Info*>(NODE_COOKIE(p)))
#define ATREENODE(p) (static_cast<Tree_Node_Info*>(NODE_COOKIE(p))->tree_node)
#define AACC(p) (ANassert(p)->dist)
#define AHEAPNODE(p) (ANassert(p)->heap_node)
#define APARENT(p) (ANassert(p)->ret_node)
#define AAassert(p) (static_cast<Arc_Info*>(ARC_COOKIE(p)))
#define AARC_DIST(p) (Distance()(p))
#define ATREEARC(p) (static_cast<Tree_Arc_Info*>(ARC_COOKIE(p))->tree_arc)
#define APOT(p) (AAassert(p)->pot)

  // Import member variables from base
  using Base::sa;
  using Base::heap;
  using Base::painted;
  using Base::ptr_g;
  using Base::s;

  // A* specific: heuristic functor
  Heuristic heuristic;

#ifndef NDEBUG
  // Debug mode: track heuristic validation
  bool validate_heuristic = true;

  void check_heuristic_admissibility(typename GT::Node* node,
                                     typename GT::Node* goal,
                                     const typename Distance::Distance_Type& actual_cost)
  {
    if (!validate_heuristic)
      return;

    auto h_estimate = heuristic(node, goal);
    if (h_estimate > actual_cost)
      {
        std::cerr << "WARNING: Inadmissible heuristic detected!\n";
        std::cerr << "  Heuristic estimate h(" << node << ") = " << h_estimate << "\n";
        std::cerr << "  Actual remaining cost = " << actual_cost << "\n";
        std::cerr << "  Heuristic overestimates by " << (h_estimate - actual_cost) << "\n";
        std::cerr << "  A* may return suboptimal paths.\n";
      }
  }
#endif

public:
  /** Constructor.
   *
   *  @param[in] dist Distance functor for arc weights.
   *  @param[in] __heuristic Heuristic functor for estimating remaining cost.
   *  @param[in] __sa Arc filter for iterators.
   */
  AStar_Min_Path(Distance dist = Distance(),
                 Heuristic __heuristic = Heuristic(),
                 SA __sa = SA())
    : Base(dist, __sa), heuristic(__heuristic)
  {
    // empty
  }

  // Import public methods from base
  using Base::has_computation;
  using Base::is_painted;
  using Base::get_start_node;
  using Base::get_graph;
  using Base::get_distance;
  using Base::get_min_path;
  using Base::copy_painted_min_paths_tree;

  // =========================================================================
  // Methods WITH Heuristic (A* specific)
  // =========================================================================

  /** Computes the shortest path from start to end using A*.
   *
   *  Builds a partial spanning tree containing the shortest path from
   *  start to end, stopping as soon as end is reached.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] tree The partial tree containing the path, mapped to g.
   *  @return Pointer to tree node corresponding to end, or nullptr if
   *          no path exists.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   */
  typename GT::Node*
  compute_partial_path(const GT& g,
                       typename GT::Node* start,
                       typename GT::Node* end,
                       GT& tree)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);
    clear_graph(tree);

    // Handle trivial case: start == end
    if (start == end)
      {
        NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
        AACC(start) = 0;
        auto tree_node = tree.insert_node(start->get_info());
        ATREENODE(start) = tree_node;
        NODE_COOKIE(tree_node) = start;
        this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();
        return tree_node;
      }

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;
    ATREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(ATREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        auto tgt = it.get_tgt_node();
        auto g_cost = AARC_DIST(arc);
        auto h_cost = heuristic(tgt, end);
        APOT(arc) = this->checked_add(g_cost, h_cost);
        heap.put_arc(arc, tgt);
      }

    typename GT::Node* result = nullptr;

    while (not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto gsrc = g.get_src_node(garc);
        auto gtgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          std::swap(gsrc, gtgt);

        NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

        auto ttgt = tree.insert_node(gtgt->get_info());
        ATREENODE(gtgt) = ttgt;
        auto tsrc = ATREENODE(gsrc);

        auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
        ATREEARC(garc) = tarc;

        AACC(gtgt) = this->checked_add(AACC(gsrc), AARC_DIST(garc));

        if (gtgt == end)
          {
            result = ttgt;
            break;
          }

        const auto& g_cost = AACC(gtgt);

        for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto tgt = it.get_tgt_node();
            if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
              continue;

            auto new_g = this->checked_add(g_cost, AARC_DIST(arc));
            auto h = heuristic(tgt, end);
            APOT(arc) = this->checked_add(new_g, h);
            heap.put_arc(arc, tgt);
          }
      }

    this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();

    return result;
  }

  /** Paints the shortest path from start to end on the graph using A*.
   *
   *  This is the memory-efficient version that marks the path
   *  directly on the graph using spanning tree bits.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @return true if a path was found, false otherwise.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   */
  bool paint_partial_path(const GT& g,
                          typename GT::Node* start,
                          typename GT::Node* end)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Node, Initialize_Arc>(g, start);

    // Handle trivial case: start == end
    if (start == end)
      {
        NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
        AACC(start) = 0;
        this->template uninit<Destroy_Node, Destroy_Arc>();
        painted = true;
        return true;
      }

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        auto tgt = it.get_tgt_node();
        auto g_cost = AARC_DIST(arc);
        auto h_cost = heuristic(tgt, end);
        APOT(arc) = this->checked_add(g_cost, h_cost);
        heap.put_arc(arc, tgt);
      }

    bool found = false;

    while (not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto src = g.get_src_node(garc);
        auto tgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          std::swap(src, tgt);

        NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
        APARENT(tgt) = src;

        if (tgt == end)
          {
            found = true;
            break;
          }

        AACC(tgt) = this->checked_add(AACC(src), AARC_DIST(garc));
        const auto& g_cost = AACC(tgt);

        for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto t = it.get_tgt_node();
            if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
              continue;

            auto new_g = this->checked_add(g_cost, AARC_DIST(arc));
            auto h = heuristic(t, end);
            APOT(arc) = this->checked_add(new_g, h);
            heap.put_arc(arc, t);
          }
      }

    this->template uninit<Destroy_Node, Destroy_Arc>();
    painted = true;

    return found;
  }

  /** Finds the shortest path from start to end using A*.
   *
   *  This is the recommended entry point for A* with heuristic.
   *  It paints the graph and extracts the path in one call.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path (empty if no path exists).
   *  @return The total cost, or max value if no path exists.
   *  @throw bad_alloc If there is not enough memory.
   */
  typename Distance::Distance_Type
  find_path(const GT& g,
            typename GT::Node* start,
            typename GT::Node* end,
            Path<GT>& path)
  {
    path.empty();
    if (paint_partial_path(g, start, end))
      return this->get_min_path(end, path);

    return std::numeric_limits<typename Distance::Distance_Type>::max();
  }

  // =========================================================================
  // Backward Compatibility Aliases
  // =========================================================================

  /// @brief Alias for compute_partial_path (backward compatibility).
  typename GT::Node*
  compute_path(const GT& g, typename GT::Node* start,
               typename GT::Node* end, GT& tree)
  {
    return compute_partial_path(g, start, end, tree);
  }

  /// @brief Alias for paint_partial_path (backward compatibility).
  bool paint_path(const GT& g, typename GT::Node* start, typename GT::Node* end)
  {
    return paint_partial_path(g, start, end);
  }

  // =========================================================================
  // Methods WITHOUT Heuristic (Dijkstra-compatible)
  // =========================================================================

  /** Computes the spanning tree of ALL shortest paths from the start node.
   *
   *  This method uses zero heuristic, making it equivalent to Dijkstra's
   *  algorithm. Use this when you need the complete shortest paths tree
   *  from a source to all reachable nodes.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node for all shortest paths.
   *  @param[out] tree The spanning tree of all shortest paths starting
   *  from start, mapped via cookies to the graph g.
   *  @return Pointer to the tree node corresponding to the start node.
   *  @note For disconnected graphs, the resulting tree spans only the
   *        component reachable from start.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If start is nullptr or g is empty.
   */
  typename GT::Node*
  compute_min_paths_tree(const GT& g, typename GT::Node* start, GT& tree)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

    clear_graph(tree);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;
    auto ret = ATREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(ATREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        APOT(arc) = AARC_DIST(arc);  // No heuristic: pot = g only
        heap.put_arc(arc, it.get_tgt_node());
      }

    const auto& n = g.get_num_nodes();

    while (tree.get_num_nodes() < n and not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto gsrc = g.get_src_node(garc);
        auto gtgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          std::swap(gsrc, gtgt);

        NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

        auto ttgt = tree.insert_node(gtgt->get_info());
        ATREENODE(gtgt) = ttgt;
        auto tsrc = ATREENODE(gsrc);

        auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
        ATREEARC(garc) = tarc;

        AACC(gtgt) = this->checked_add(AACC(gsrc), AARC_DIST(garc));
        const auto& acc = AACC(gtgt);

        for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto tgt = it.get_tgt_node();
            if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
              continue;

            APOT(arc) = this->checked_add(acc, AARC_DIST(arc));
            heap.put_arc(arc, tgt);
          }
      }

    this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();

    return ret;
  }

  /** Computes partial spanning tree from start to end (without heuristic).
   *
   *  Uses Dijkstra's algorithm (zero heuristic) but stops when end is reached.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] tree The partial spanning tree.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   */
  void compute_partial_min_paths_tree(const GT& g,
                                      typename GT::Node* start,
                                      typename GT::Node* end,
                                      GT& tree)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);
    clear_graph(tree);

    // Handle trivial case: start == end
    if (start == end)
      {
        NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
        AACC(start) = 0;
        auto tree_node = tree.insert_node(start->get_info());
        ATREENODE(start) = tree_node;
        NODE_COOKIE(tree_node) = start;
        this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();
        return;
      }

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;
    ATREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(ATREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        APOT(arc) = AARC_DIST(arc);
        heap.put_arc(arc, it.get_tgt_node());
      }

    const auto& n = g.get_num_nodes();

    while (tree.get_num_nodes() < n and not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto gsrc = g.get_src_node(garc);
        auto gtgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
          std::swap(gsrc, gtgt);

        NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

        auto ttgt = tree.insert_node(gtgt->get_info());
        ATREENODE(gtgt) = ttgt;

        auto tarc = tree.insert_arc(ATREENODE(gsrc), ATREENODE(gtgt), garc->get_info());
        ATREEARC(garc) = tarc;

        AACC(gtgt) = this->checked_add(AACC(gsrc), AARC_DIST(garc));

        if (gtgt == end)
          break;
        const auto& acc = AACC(gtgt);

        for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto tgt = it.get_tgt_node();
            if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
              continue;

            APOT(arc) = this->checked_add(acc, AARC_DIST(arc));
            heap.put_arc(arc, tgt);
          }
      }

    this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();
  }

  /** Paints on graph g the partial shortest paths tree (without heuristic).
   *
   *  Uses Dijkstra's algorithm but stops when end is reached.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @return true if end was found, false otherwise.
   *  @throw bad_alloc If there is not enough memory.
   *  @throw domain_error If start or end is nullptr, or g is empty.
   */
  bool paint_partial_min_paths_tree(const GT& g,
                                    typename GT::Node* start,
                                    typename GT::Node* end)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Node, Initialize_Arc>(g, start);

    // Handle trivial case: start == end
    if (start == end)
      {
        NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
        AACC(start) = 0;
        this->template uninit<Destroy_Node, Destroy_Arc>();
        painted = true;
        return true;
      }

    bool ret_val = false;

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        APOT(arc) = AARC_DIST(arc);
        heap.put_arc(arc, it.get_tgt_node());
      }

    const auto& n = g.get_num_nodes();
    size_t tn = 1;

    while (tn < n and not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto src = g.get_src_node(garc);
        auto tgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          std::swap(src, tgt);

        NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
        APARENT(tgt) = src;

        ++tn;

        if (tgt == end)
          {
            ret_val = true;
            break;
          }

        AACC(tgt) = this->checked_add(AACC(src), AARC_DIST(garc));
        const auto& acc = AACC(tgt);

        for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
          {
            auto a = it.get_current_arc_ne();
            if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
              continue;

            auto t = it.get_tgt_node();
            if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
              continue;

            APOT(a) = this->checked_add(acc, AARC_DIST(a));
            heap.put_arc(a, t);
          }
      }

    this->template uninit<Destroy_Node, Destroy_Arc>();
    painted = true;

    return ret_val;
  }

  /** Paints on graph g the spanning tree of ALL shortest paths
      starting from start (without heuristic).

      Uses Dijkstra's algorithm (zero heuristic).

      @param[in] g The graph.
      @param[in] start The starting node.
      @throw bad_alloc If there is not enough memory.
      @throw domain_error If start is nullptr or g is empty.
  */
  void paint_min_paths_tree(const GT& g, typename GT::Node* start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Node, Initialize_Arc>(g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    AACC(start) = 0;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        APOT(arc) = AARC_DIST(arc);
        heap.put_arc(arc, it.get_tgt_node());
      }

    const auto& n = g.get_num_nodes();
    size_t tn = 1;

    while (tn < n and not heap.is_empty())
      {
        auto garc = heap.get_min_arc();
        if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
          continue;

        auto src = g.get_src_node(garc);
        auto tgt = g.get_tgt_node(garc);

        if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
            IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          continue;

        ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

        if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
          std::swap(src, tgt);

        NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
        APARENT(tgt) = src;

        ++tn;

        AACC(tgt) = this->checked_add(AACC(src), AARC_DIST(garc));
        const auto& acc = AACC(tgt);

        for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
          {
            auto a = it.get_current_arc_ne();
            if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
              continue;

            auto t = it.get_tgt_node();
            if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
              continue;

            APOT(a) = this->checked_add(acc, AARC_DIST(a));
            heap.put_arc(a, t);
          }
      }

    this->template uninit<Destroy_Node, Destroy_Arc>();
    painted = true;
  }

  /** Computes shortest path by painting the graph (without heuristic).
   *
   *  Dijkstra-compatible interface.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] min_path The shortest path.
   *  @return The total cost, or max value if no path.
   *  @throw bad_alloc If there is not enough memory.
   */
  typename Distance::Distance_Type
  find_min_path(const GT& g,
                typename GT::Node* start,
                typename GT::Node* end,
                Path<GT>& min_path)
  {
    min_path.empty();
    if (paint_partial_min_paths_tree(g, start, end))
      return this->get_min_path(end, min_path);

    return std::numeric_limits<typename Distance::Distance_Type>::max();
  }

  // =========================================================================
  // Operator Interfaces
  // =========================================================================

  /** Computes the spanning tree of all shortest paths.
   *
   *  Dijkstra-compatible operator interface.
   *
   *  @param[in] g The graph.
   *  @param[in] s The starting node.
   *  @param[out] tree The spanning tree.
   */
  void operator()(const GT& g, typename GT::Node* s, GT& tree)
  {
    compute_min_paths_tree(g, s, tree);
  }

  /** Finds shortest path using A* heuristic.
   *
   *  @param[in] g The graph.
   *  @param[in] s The starting node.
   *  @param[in] e The destination node.
   *  @param[out] path The shortest path.
   *  @return The total cost.
   */
  typename Distance::Distance_Type operator()(const GT& g,
                                              typename GT::Node* s,
                                              typename GT::Node* e,
                                              Path<GT>& path)
  {
    return find_path(g, s, e, path);
  }

#undef ANassert
#undef APARENT
#undef ATREENODE
#undef AACC
#undef AHEAPNODE
#undef AAassert
#undef AARC_DIST
#undef ATREEARC
#undef APOT
};

/** @brief Euclidean distance heuristic for A* in 2D grids.
 *
 *  Example heuristic for graphs where nodes have (x, y) coordinates.
 *  Assumes `node->get_info()` returns a struct with `x` and `y` fields.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance accessor functor.
 *
 *  @ingroup Graphs
 */
template <class GT, class Distance = Dft_Dist<GT>>
struct Euclidean_Heuristic
{
  using Distance_Type = typename Distance::Distance_Type;

  Distance_Type operator()(typename GT::Node* from, typename GT::Node* to) const
  {
    auto& f = from->get_info();
    auto& t = to->get_info();
    auto dx = f.x - t.x;
    auto dy = f.y - t.y;
    return static_cast<Distance_Type>(std::sqrt(dx * dx + dy * dy));
  }
};

/** @brief Manhattan distance heuristic for A* in grid graphs.
 *
 *  Example heuristic for graphs where nodes have (x, y) coordinates
 *  and movement is restricted to horizontal/vertical directions.
 *
 *  @tparam GT Graph type.
 *  @tparam Distance Distance accessor functor.
 *
 *  @ingroup Graphs
 */
template <class GT, class Distance = Dft_Dist<GT>>
struct Manhattan_Heuristic
{
  using Distance_Type = typename Distance::Distance_Type;

  Distance_Type operator()(typename GT::Node* from, typename GT::Node* to) const
  {
    auto& f = from->get_info();
    auto& t = to->get_info();
    return static_cast<Distance_Type>(std::abs(f.x - t.x) + std::abs(f.y - t.y));
  }
};

} // end namespace Aleph

#endif // ASTAR_H
