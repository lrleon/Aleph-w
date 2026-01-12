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

/** @file Dijkstra.H
 *  @brief Dijkstra's shortest path algorithm.
 *
 *  Implements Dijkstra's algorithm for finding shortest paths from a
 *  source vertex to all other vertices in a weighted graph with
 *  non-negative edge weights. Uses binary heap for O((V+E)log V).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef DIJKSTRA_H
#define DIJKSTRA_H

#include <ahFunction.H>
#include <ah-errors.H>
#include <archeap.H>
#include <tpl_find_path.H>
#include <tpl_agraph.H>
#include <shortest_path_common.H>

namespace Aleph
{

/** @brief Spanning tree calculation of all shortest paths from
    a given node according to Dijkstra's algorithm.

    @details This class handles Dijkstra's algorithm to compute a tree
    containing of all shortest paths from a node `s` of a graph `g`.

    The algorithm uses an internal queue whose length is proportional
    to the number of nodes in the graph.

    Dijkstra's algorithm does not work for graphs with negative weights.

    The class receives the following template parameters:
    -# `GT`: graph type.
    -# `Distance<GT>`: class reading the weight. It should export the
        following attributes:
    -# `typedef Distance<GT>::Distance_Type`: Data type. It represents a
       weight on an arc.
    -# `Distance<GT>::Distance_Type operator()(typename GT::Arc *a)`: returns
       the weight of arc `a`.
    -# `Itor<GT, SA>`: arc iterator type (defaults to Node_Arc_Iterator).
    -# `SA`: arc filter for internal iterators.
    -# `HeapT<GT, Distance, Access>`: priority-queue adapter used by the
       algorithm (defaults to ArcHeap). It must provide `put_arc`,
       `get_min_arc`, `is_empty`, and `empty`, and accept an access
       functor that maps nodes to heap handles.

    @note To use a Fibonacci heap, pass ArcFibonacciHeap as HeapT.

    @warning This class is **not thread-safe**. Concurrent calls from
             multiple threads on the same instance require external
             synchronization.

    @see Find_Path_Depth_First Floyd_All_Shortest_Paths Bellman_Ford_Min_Spanning_Tree
         AStar_Min_Path

@ingroup Graphs
*/
template <class GT,
          class Distance = Dft_Dist<GT>,
          template <typename, class> class Itor = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>,
          template <class, class, class> class HeapT = ArcHeap>
class Dijkstra_Min_Paths
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
#define DNassert(p) (static_cast<Node_Info*>(NODE_COOKIE(p)))
#define TREENODE(p) (static_cast<Tree_Node_Info*>(NODE_COOKIE(p))->tree_node)
#define ACC(p) (DNassert(p)->dist)
#define HEAPNODE(p) (DNassert(p)->heap_node)
#define PARENT(p) (DNassert(p)->ret_node)
#define DAassert(p) (static_cast<Arc_Info*>(ARC_COOKIE(p)))
#define ARC_DIST(p) (Distance()(p))
#define TREEARC(p) (static_cast<Tree_Arc_Info*>(ARC_COOKIE(p))->tree_arc)
#define POT(p) (DAassert(p)->pot)

  // Import member variables from base
  using Base::sa;
  using Base::heap;
  using Base::painted;
  using Base::ptr_g;
  using Base::s;

public:
  /** Constructor.

      @param[in] dist Distance functor for arc weights.
      @param[in] __sa Arc filter for iterators.
   */
  Dijkstra_Min_Paths(Distance dist = Distance(), SA __sa = SA())
    : Base(dist, __sa)
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

  /** Computes the spanning tree of all shortest paths from the start node.

      @param[in] g The graph.
      @param[in] start The starting node for all shortest paths.
      @param[out] tree The spanning tree of all shortest paths starting
      from start, mapped via cookies to the graph g.
      @return Pointer to the tree node corresponding to the start node.
      @note For disconnected graphs, the resulting tree spans only the
            component reachable from start.
      @throw bad_alloc If there is not enough memory.
      @throw domain_error If start is nullptr or g is empty.
   */
  typename GT::Node*
  compute_min_paths_tree(const GT& g, typename GT::Node* start, GT& tree)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

    clear_graph(tree);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    ACC(start) = 0;
    auto ret = TREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        POT(arc) = ARC_DIST(arc);
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
        TREENODE(gtgt) = ttgt;
        auto tsrc = TREENODE(gsrc);

        auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
        TREEARC(garc) = tarc;

        ACC(gtgt) = this->checked_add(ACC(gsrc), ARC_DIST(garc));
        const auto& acc = ACC(gtgt);

        for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto tgt = it.get_tgt_node();
            if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
              continue;

            POT(arc) = this->checked_add(acc, ARC_DIST(arc));
            heap.put_arc(arc, tgt);
          }
      }

    this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();

    return ret;
  }

  /** Computes the partial spanning tree of all shortest paths from start
      that contains the path from start to end.

      @param[in] g The graph.
      @param[in] start The starting node for all shortest paths.
      @param[in] end The destination node.
      @param[out] tree The spanning tree of all shortest paths starting
      from start, mapped via cookies to the graph g.
      @throw bad_alloc If there is not enough memory.
      @throw domain_error If start or end is nullptr, or if g is empty.
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

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    ACC(start) = 0;
    TREENODE(start) = tree.insert_node(start->get_info());
    NODE_COOKIE(TREENODE(start)) = start;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        POT(arc) = ARC_DIST(arc);
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

        auto ttgt = tree.insert_node(gtgt->get_info());
        TREENODE(gtgt) = ttgt;
        NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

        auto tarc = tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
        TREEARC(garc) = tarc;

        if (gtgt == end)
          break;

        ACC(gtgt) = this->checked_add(ACC(gsrc), ARC_DIST(garc));
        const auto& acc = ACC(gtgt);

        for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_current_arc_ne();
            if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              continue;

            auto tgt = it.get_tgt_node();
            if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
              continue;

            POT(arc) = this->checked_add(acc, ARC_DIST(arc));
            heap.put_arc(arc, tgt);
          }
      }

    this->template uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();
  }

  /** Paints on graph g the partial shortest paths tree starting from
      start and stopping when the end node is found.

      @param[in] g The graph.
      @param[in] start The starting node for shortest paths.
      @param[in] end The destination node.
      @return true if end was found in the spanning tree, false otherwise.
      @throw bad_alloc If there is not enough memory.
      @throw domain_error If start or end is nullptr, or if g is empty.
   */
  bool paint_partial_min_paths_tree(const GT& g,
                                    typename GT::Node* start,
                                    typename GT::Node* end)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    bool ret_val = false;
    this->template init<Initialize_Node, Initialize_Arc>(g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    ACC(start) = 0;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        POT(arc) = ARC_DIST(arc);
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
        PARENT(tgt) = src;

        ++tn;

        if (tgt == end)
          {
            ret_val = true;
            break;
          }

        ACC(tgt) = this->checked_add(ACC(src), ARC_DIST(garc));
        const auto& acc = ACC(tgt);

        for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
          {
            auto a = it.get_current_arc_ne();
            if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
              continue;

            auto t = it.get_tgt_node();
            if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
              continue;

            POT(a) = this->checked_add(acc, ARC_DIST(a));
            heap.put_arc(a, t);
          }
      }

    this->template uninit<Destroy_Node, Destroy_Arc>();
    painted = true;

    return ret_val;
  }

  /** Paints on graph g the spanning tree of all shortest paths
      starting from start.

      @param[in] g The graph.
      @param[in] start The starting node for shortest paths.
      @throw bad_alloc If there is not enough memory.
      @throw domain_error If start is nullptr or g is empty.
  */
  void paint_min_paths_tree(const GT& g, typename GT::Node* start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    this->template init<Initialize_Node, Initialize_Arc>(g, start);

    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
    ACC(start) = 0;

    for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_current_arc_ne();
        POT(arc) = ARC_DIST(arc);
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
        PARENT(tgt) = src;

        ++tn;

        ACC(tgt) = this->checked_add(ACC(src), ARC_DIST(garc));
        const auto& acc = ACC(tgt);

        for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
          {
            auto a = it.get_current_arc_ne();
            if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
              continue;

            auto t = it.get_tgt_node();
            if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
              continue;

            POT(a) = this->checked_add(acc, ARC_DIST(a));
            heap.put_arc(a, t);
          }
      }

    this->template uninit<Destroy_Node, Destroy_Arc>();
    painted = true;
  }

  /** Computes the shortest path between start and end by painting the graph.

      This is the version with the lowest memory consumption and probably
      also the fastest.

      @param[in] g The graph.
      @param[in] start The starting node of the path.
      @param[in] end The destination node of the path.
      @param[out] min_path The shortest path.
      @return The total cost of the path from start to end.
      @throw bad_alloc If there is not enough memory.
   */
  typename Distance::Distance_Type
  find_min_path(const GT& g,
                typename GT::Node* start, typename GT::Node* end,
                Path<GT>& min_path)
  {
    min_path.empty();
    if (paint_partial_min_paths_tree(g, start, end))
      return this->get_min_path(end, min_path);

    return std::numeric_limits<typename Distance::Distance_Type>::max();
  }

  /** Computes the spanning tree of all shortest paths from a given
      node according to Dijkstra's algorithm.

      @param[in] g The graph.
      @param[in] s The starting node.
      @param[out] tree The spanning tree.
      @throw bad_alloc If there is not enough memory.
   */
  void operator()(const GT& g, typename GT::Node* s, GT& tree)
  {
    compute_min_paths_tree(g, s, tree);
  }

  /// @overload find_min_path()
  typename Distance::Distance_Type operator()(const GT& g,
                                              typename GT::Node* s,
                                              typename GT::Node* e,
                                              Path<GT>& path)
  {
    return find_min_path(g, s, e, path);
  }

#undef DNassert
#undef PARENT
#undef TREENODE
#undef ACC
#undef HEAPNODE
#undef DAassert
#undef ARC_DIST
#undef TREEARC
#undef POT
};

} // end namespace Aleph

#endif // DIJKSTRA_H
