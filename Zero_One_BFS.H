
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


/** @file Zero_One_BFS.H
 *  @brief 0-1 BFS shortest path algorithm.
 *
 *  Implements the 0-1 BFS algorithm for finding shortest paths in graphs
 *  where all edge weights are either 0 or 1. Uses a double-ended queue
 *  (deque) to achieve O(V + E) time complexity, which is optimal for
 *  this class of graphs.
 *
 *  ## How it works
 *
 *  Instead of a priority queue (as in Dijkstra), 0-1 BFS uses a deque:
 *  - When traversing a 0-weight edge, the target node is pushed to the
 *    **front** of the deque (it has the same distance as the current node).
 *  - When traversing a 1-weight edge, the target node is pushed to the
 *    **back** of the deque (it has distance + 1).
 *
 *  This maintains the invariant that the deque is sorted by distance,
 *  so the front always contains the node with minimum distance.
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Single-target | O(V + E) | O(V) |
 *  | All-targets | O(V + E) | O(V) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Graph with 0/1 edge weights
 *  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 *  auto* n0 = g.insert_node(0);
 *  auto* n1 = g.insert_node(1);
 *  auto* n2 = g.insert_node(2);
 *  g.insert_arc(n0, n1, 0);  // free edge
 *  g.insert_arc(n1, n2, 1);  // cost-1 edge
 *
 *  Zero_One_BFS<decltype(g)> bfs01;
 *  Path<decltype(g)> path(g);
 *  int dist = bfs01(g, n0, n2, path);  // dist == 1
 *  ```
 *
 *  @see Dijkstra.H For general non-negative weights
 *  @see Dial.H For small integer weights
 *  @see tpl_find_path.H For unweighted BFS
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef ZERO_ONE_BFS_H
# define ZERO_ONE_BFS_H

# include <limits>
# include <type_traits>
# include <tpl_dynDlist.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{

/** @brief 0-1 BFS algorithm for shortest paths in graphs with 0/1 weights.
 *
 *  @details This class computes shortest paths in graphs where every
 *  edge weight is either 0 or 1. It uses a deque-based BFS that runs
 *  in O(V + E) time, which is asymptotically faster than Dijkstra's
 *  O((V + E) log V) for this special case.
 *
 *  The algorithm works by maintaining a deque of nodes. When relaxing
 *  an edge with weight 0, the target is pushed to the front of the
 *  deque; when relaxing an edge with weight 1, the target is pushed
 *  to the back. This ensures nodes are always processed in
 *  non-decreasing order of distance.
 *
 *  The class receives the following template parameters:
 *  -# `GT`: graph type (List_Graph, List_SGraph, Array_Graph, etc.).
 *  -# `Distance`: class reading the weight from an arc. Must export
 *     `Distance_Type` and `operator()(Arc*)`.
 *  -# `Itor`: arc iterator template (defaults to Node_Arc_Iterator).
 *  -# `SA`: arc filter for internal iterators.
 *
 *  @warning All arc weights must be exactly 0 or 1. A `domain_error`
 *           is thrown if any other weight is encountered.
 *
 *  @note This algorithm works for both directed and undirected graphs.
 *
 *  @see Dijkstra_Min_Paths For general non-negative weights.
 *  @see Dial_Min_Paths For small integer weights.
 *
 *  @ingroup Graphs
 */
template <class GT,
          class Distance = Dft_Dist<GT>,
          template <typename, class> class Itor = Node_Arc_Iterator,
          class SA = Dft_Show_Arc<GT>>
class Zero_One_BFS
{
public:
  using Distance_Type = typename Distance::Distance_Type;
  using Node = typename GT::Node;
  using Arc = typename GT::Arc;

private:
  SA sa;
  Distance distance;
  bool painted = false;
  GT * ptr_g = nullptr;
  Node * s = nullptr;

  static constexpr Distance_Type Inf =
    std::numeric_limits<Distance_Type>::max();

  struct Node_Info
  {
    Distance_Type dist;
    Node * parent;
    Arc * parent_arc;

    Node_Info() noexcept : dist(Inf), parent(nullptr), parent_arc(nullptr) {}
  };

  /** @brief Stores parent and distance after painting for O(1) access. */
  struct Painted_Info
  {
    Node * parent;
    Distance_Type dist;
  };

  DynList<std::pair<Node *, Node_Info *>> owned_node_infos;
  DynList<std::pair<Node *, Painted_Info *>> owned_painted_infos;

#define ZOB_INFO(p)   (static_cast<Node_Info *>(NODE_COOKIE(p)))
#define ZOB_PNTD(p)   (static_cast<Painted_Info *>(NODE_COOKIE(p)))
#define ZOB_DIST(p)   (ZOB_INFO(p)->dist)
#define ZOB_PARENT(p) (ZOB_INFO(p)->parent)
#define ZOB_PARENT_ARC(p) (ZOB_INFO(p)->parent_arc)

  void release_owned_node_infos() noexcept
  {
    for (auto it = owned_node_infos.get_it(); it.has_curr(); it.next())
      {
        auto item = it.get_curr();
        auto p = item.first;
        auto info = item.second;
        if (info != nullptr)
          delete info;
        if (NODE_COOKIE(p) == info)
          NODE_COOKIE(p) = nullptr;
      }
    owned_node_infos.empty();
  }

  void release_owned_painted_infos() noexcept
  {
    for (auto it = owned_painted_infos.get_it(); it.has_curr(); it.next())
      {
        auto item = it.get_curr();
        auto p = item.first;
        auto info = item.second;
        if (info != nullptr)
          delete info;
        if (NODE_COOKIE(p) == info)
          NODE_COOKIE(p) = nullptr;
      }
    owned_painted_infos.empty();
  }

  void reset_state_after_failure() noexcept
  {
    painted = false;
    ptr_g = nullptr;
    s = nullptr;
  }

  /** @brief RAII guard for Zero_One_BFS initialization and cleanup. */
  class ZOB_Init_Guard
  {
    Zero_One_BFS * owner = nullptr;
    const GT * g = nullptr;
    bool committed = false;

  public:
    /** @brief Constructor.
     *  @param[in] __owner The Zero_One_BFS instance.
     *  @param[in] __g The graph being processed.
     */
    ZOB_Init_Guard(Zero_One_BFS & __owner, const GT & __g) noexcept
      : owner(&__owner), g(&__g)
    {
      // empty
    }

    /** @brief Deleted copy constructor. */
    ZOB_Init_Guard(const ZOB_Init_Guard &) = delete;
    /** @brief Deleted copy assignment. */
    ZOB_Init_Guard & operator = (const ZOB_Init_Guard &) = delete;
    /** @brief Deleted move constructor. */
    ZOB_Init_Guard(ZOB_Init_Guard &&) = delete;
    /** @brief Deleted move assignment. */
    ZOB_Init_Guard & operator = (ZOB_Init_Guard &&) = delete;

    /** @brief Commits the operation, preventing cleanup on destruction. */
    void commit() noexcept { committed = true; }

    /** @brief Destructor. Performs cleanup if not committed. */
    ~ZOB_Init_Guard() noexcept
    {
      if (not committed)
        {
          owner->uninit_discard();
          owner->reset_state_after_failure();
        }
    }
  };

  void init(const GT & g)
  {
    ptr_g = &const_cast<GT &>(g);
    release_owned_node_infos();
    release_owned_painted_infos();
    for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
      {
        auto p = it.get_curr();
        g.reset_bit(p, Aleph::Spanning_Tree);
        NODE_COOKIE(p) = nullptr;
      }
    for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next())
      g.reset_bit(it.get_curr(), Aleph::Spanning_Tree);

    try
      {
        for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
          {
            auto p = it.get_curr();
            auto info = new Node_Info;
            NODE_COOKIE(p) = info;
            try
              {
                owned_node_infos.append({p, info});
              }
            catch (...)
              {
                delete info;
                NODE_COOKIE(p) = nullptr;
                throw;
              }
          }
      }
    catch (...)
      {
        release_owned_node_infos();
        release_owned_painted_infos();
        throw;
      }
  }

  void uninit_paint()
  {
    // Two-phase conversion to be exception-safe:
    // 1. Allocate all Painted_Info objects
    // 2. Atomically swap and delete old Node_Info
    DynList<std::pair<Node *, Painted_Info *>> pending;
    try
      {
        for (typename GT::Node_Iterator it(*ptr_g); it.has_curr(); it.next())
          {
            auto p = it.get_curr();
            auto info = ZOB_INFO(p);
            pending.append({p, new Painted_Info{info->parent, info->dist}});
          }
      }
    catch (...)
      {
        for (auto it = pending.get_it(); it.has_curr(); it.next())
          delete it.get_curr().second;
        throw;
      }

    // Phase 2: Apply changes
    release_owned_node_infos();
    for (auto it = pending.get_it(); it.has_curr(); it.next())
      {
        auto item = it.get_curr();
        auto p = item.first;
        auto pinfo = item.second;
        NODE_COOKIE(p) = pinfo;
      }
    owned_painted_infos.swap(pending);
  }

  void uninit_discard()
  {
    if (ptr_g == nullptr)
      return;

    release_owned_node_infos();
    release_owned_painted_infos();

    for (typename GT::Node_Iterator it(*ptr_g); it.has_curr(); it.next())
      {
        auto p = it.get_curr();
        ptr_g->reset_bit(p, Aleph::Spanning_Tree);
      }

    for (typename GT::Arc_Iterator it(*ptr_g); it.has_curr(); it.next())
      ptr_g->reset_bit(it.get_curr(), Aleph::Spanning_Tree);
  }

  void run_bfs(const GT & g, Node * start, Node * end = nullptr)
  {
    ZOB_DIST(start) = 0;
    NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);

    DynDlist<Node *> deque;
    deque.append(start);

    while (not deque.is_empty())
      {
        auto curr = deque.remove_first();
        auto curr_dist = ZOB_DIST(curr);

        if (end != nullptr and curr == end)
          break;

        for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
          {
            auto arc = it.get_current_arc();
            auto tgt = g.get_connected_node(arc, curr);

            auto w = distance(arc);
            ah_domain_error_if(w != Distance_Type(0) and w != Distance_Type(1))
              << "Zero_One_BFS: arc weight must be 0 or 1, got " << w;

            auto new_dist = curr_dist + w;

            if (new_dist < ZOB_DIST(tgt))
              {
                auto prev_parent_arc = ZOB_PARENT_ARC(tgt);
                if (prev_parent_arc != nullptr)
                  ARC_BITS(prev_parent_arc).set_bit(Aleph::Spanning_Tree, false);

                ZOB_DIST(tgt) = new_dist;
                ZOB_PARENT(tgt) = curr;
                ZOB_PARENT_ARC(tgt) = arc;
                NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);

                // Mark the selected parent arc in the spanning tree
                ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);

                if (w == Distance_Type(0))
                  deque.insert(tgt);   // push front
                else
                  deque.append(tgt);   // push back
              }
          }
      }
  }

public:
  /** @brief Constructor.
   *
   *  @param[in] dist Distance functor for arc weights.
   *  @param[in] __sa Arc filter for iterators.
   */
  Zero_One_BFS(Distance dist = Distance(), SA __sa = SA())
    : sa(__sa), distance(dist)
  {
    // empty
  }

  /** @brief Destructor.
   *
   *  Releases any per-node data left in NODE_COOKIE by the last
   *  computation so sanitizers do not report leaks.
   */
  ~Zero_One_BFS() noexcept
  {
    uninit_discard();
    reset_state_after_failure();
  }

  /** @brief Check if a computation has been performed.
   *  @return true if a graph has been painted, false otherwise.
   */
  [[nodiscard]] bool is_painted() const noexcept { return painted; }

  /** @brief Get the start node of the last computation.
   *  @return Pointer to the start node, or nullptr if no computation done.
   */
  [[nodiscard]] Node * get_start_node() const noexcept { return s; }

  /** @brief Get the graph of the last computation.
   *  @return Pointer to the graph, or nullptr if no computation done.
   */
  [[nodiscard]] GT * get_graph() const noexcept { return ptr_g; }

  /** @brief Paints the shortest paths tree on the graph from start.
   *
   *  After calling this method, NODE_COOKIE(p) stores the parent node
   *  in the shortest path tree, and the `Spanning_Tree` bit is set on
   *  nodes and arcs that belong to the tree.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @throw domain_error If start is nullptr, g is empty, or any arc
   *         weight is not 0 or 1.
   *  @throw bad_alloc If there is not enough memory.
   *
   *  @note Time complexity: O(V + E)
   */
  void paint_min_paths_tree(const GT & g, Node * start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    if (ptr_g != nullptr)
      uninit_discard();

    painted = false;
    s = nullptr;

    init(g);
    ZOB_Init_Guard guard(*this, g);
    s = start;
    run_bfs(g, start);
    uninit_paint();
    guard.commit();
    painted = true;
  }

  /** @brief Gets the accumulated distance to a node after painting.
   *
   *  @param[in] node The destination node.
   *  @return The shortest distance from start to node.
   *  @throw domain_error If the graph has not been painted, node is null,
   *         or node is not reachable from the start.
   *
   *  @note Time complexity: O(path_length)
   */
  [[nodiscard]] Distance_Type get_distance(Node * node)
  {
    ah_domain_error_if(not painted) << "Graph has not been painted";
    ah_domain_error_if(node == nullptr) << "node cannot be null";

    if (node == s)
      return Distance_Type(0);

    ah_domain_error_if(not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
      << "node is not reachable from start";

    // O(1) access thanks to Painted_Info
    auto dist_val = ZOB_PNTD(node)->dist;
    if (dist_val != Inf)
      return dist_val;

    // Fallback: Compute distance by walking the path
    Distance_Type total = 0;
    for (auto curr = node; curr != s;)
      {
        auto pinfo = ZOB_PNTD(curr);
        auto parent = pinfo->parent;
        if (parent == nullptr)
          break;

        for (Itor<GT, SA> it(parent, sa); it.has_curr(); it.next())
          {
            auto arc = it.get_current_arc();
            auto tgt = ptr_g->get_connected_node(arc, parent);
            if (tgt == curr and IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              {
                total += distance(arc);
                break;
              }
          }
        curr = parent;
      }

    return total;
  }

  /** @brief Extracts a shortest path from a previously painted graph.
   *
   *  @param[in] end Destination node of the path.
   *  @param[out] path Path where the shortest path will be stored.
   *  @return The total cost of the path.
   *  @throw domain_error If the graph has not been painted.
   *
   *  @note Time complexity: O(path_length)
   */
  Distance_Type get_min_path(Node * end, Path<GT> & path)
  {
    ah_domain_error_if(not painted) << "Graph has not been painted";
    ah_domain_error_if(ptr_g == nullptr) << "No computation has been done";
    ah_domain_error_if(end == nullptr) << "end pointer is null";

    path.empty();
    if (end == s)
      {
        path.set_graph(*ptr_g, s);
        return 0;
      }

    if (not IS_NODE_VISITED(end, Aleph::Spanning_Tree))
      return Inf;

    Distance_Type total = 0;
    path.set_graph(*ptr_g, s);
    DynList<Arc *> arcs;
    for (auto curr = end; curr != s;)
      {
        auto parent = ZOB_PNTD(curr)->parent;
        if (parent == nullptr)
          return Inf;

        bool found = false;
        for (Itor<GT, SA> it(parent, sa); it.has_curr(); it.next())
          {
            auto arc = it.get_current_arc();
            if (ptr_g->get_connected_node(arc, parent) == curr and
                IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              {
                arcs.insert(arc);
                total += distance(arc);
                curr = parent;
                found = true;
                break;
              }
          }
        if (not found)
          return Inf;
      }

    for (auto it = arcs.get_it(); it.has_curr(); it.next())
      path.append(it.get_curr());

    return total;
  }

  /** @brief Computes the shortest path from start to end.
   *
   *  Paints the graph (stopping early when end is found) and extracts
   *  the path. This is the recommended entry point for single-target
   *  queries.
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path (empty if no path exists).
   *  @return The total cost, or max value if no path exists.
   *  @throw domain_error If start or end is nullptr, g is empty, or
   *         any arc weight is not 0 or 1.
   *  @throw bad_alloc If there is not enough memory.
   *
   *  @note Time complexity: O(V + E)
   */
  Distance_Type find_min_path(const GT & g, Node * start, Node * end,
                              Path<GT> & path)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";
    ah_domain_error_if(end == nullptr) << "end node cannot be null";
    ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

    if (ptr_g != nullptr)
      uninit_discard();

    path.empty();

    painted = false;
    s = nullptr;

    init(g);
    ZOB_Init_Guard guard(*this, g);
    s = start;
    run_bfs(g, start, end);
    uninit_paint();
    guard.commit();
    painted = true;

    if (not IS_NODE_VISITED(end, Aleph::Spanning_Tree))
      return Inf;

    return get_min_path(end, path);
  }

  /** @brief Computes shortest path (operator interface).
   *
   *  @param[in] g The graph.
   *  @param[in] start The starting node.
   *  @param[in] end The destination node.
   *  @param[out] path The shortest path.
   *  @return The total cost, or max value if no path exists.
   */
  Distance_Type operator()(const GT & g, Node * start, Node * end,
                           Path<GT> & path)
  {
    return find_min_path(g, start, end, path);
  }

#undef ZOB_INFO
#undef ZOB_PNTD
#undef ZOB_DIST
#undef ZOB_PARENT
#undef ZOB_PARENT_ARC
};

} // end namespace Aleph

# endif // ZERO_ONE_BFS_H
