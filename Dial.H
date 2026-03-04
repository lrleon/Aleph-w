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


/** @file Dial.H
 *  @brief Dial's algorithm for shortest paths with small integer weights.
 *
 *  Implements Dial's algorithm, a bucket-based variant of Dijkstra's
 *  algorithm optimized for graphs with non-negative integer edge weights.
 *  It achieves O(V + E + C) time complexity, where C is the maximum
 *  edge weight times the number of vertices.
 *
 *  ## How it works
 *
 *  Instead of a priority queue, Dial's algorithm uses an array of
 *  buckets indexed by distance. Each bucket holds the nodes at that
 *  distance from the source. The algorithm scans buckets in order,
 *  processing all nodes in the current bucket before moving to the next.
 *
 *  This is especially efficient when the maximum edge weight C is small,
 *  as the number of buckets is proportional to V * C.
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Single-target | O(V + E + V*C) | O(V + V*C) |
 *  | All-targets | O(V + E + V*C) | O(V + V*C) |
 *
 *  where C is the maximum edge weight.
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Graph with small integer weights
 *  List_Graph<Graph_Node<int>, Graph_Arc<int>> g;
 *  auto* n0 = g.insert_node(0);
 *  auto* n1 = g.insert_node(1);
 *  auto* n2 = g.insert_node(2);
 *  g.insert_arc(n0, n1, 3);
 *  g.insert_arc(n1, n2, 2);
 *
 *  Dial_Min_Paths<decltype(g)> dial;
 *  Path<decltype(g)> path(g);
 *  int dist = dial(g, n0, n2, path);  // dist == 5
 *  ```
 *
 *  @see Dijkstra.H For general non-negative weights (heap-based)
 *  @see Zero_One_BFS.H For 0/1 weights only
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath Leon
 */

# ifndef DIAL_H
# define DIAL_H

# include <limits>
# include <type_traits>
# include <tpl_array.H>
# include <tpl_graph_utils.H>
# include <ah-errors.H>

namespace Aleph
{
  /** @brief Dial's algorithm for shortest paths with integer weights.
   *
   *  @details This class computes shortest paths in graphs where every
   *  edge weight is a non-negative integer. It uses bucket-based
   *  relaxation that runs in O(V + E + C) where C = max_weight * V.
   *
   *  This is faster than Dijkstra when the maximum weight C is small
   *  relative to log V (i.e., C << log V), because bucket operations
   *  are O(1) vs O(log V) for heap operations.
   *
   *  The class receives the following template parameters:
   *  -# `GT`: graph type (List_Graph, List_SGraph, Array_Graph, etc.).
   *  -# `Distance`: class reading the weight from an arc. Must export
   *     `Distance_Type` (which must be integral) and `operator()(Arc*)`.
   *  -# `Itor`: arc iterator template (defaults to Node_Arc_Iterator).
   *  -# `SA`: arc filter for internal iterators.
   *
   *  @warning All arc weights must be non-negative integers. The
   *           `Distance_Type` must be an integral type.
   *
   *  @note This algorithm works for both directed and undirected graphs.
   *
   *  @see Dijkstra_Min_Paths For general non-negative weights.
   *  @see Zero_One_BFS For 0/1 weights.
   *
   *  @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            template <typename, class> class Itor = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Dial_Min_Paths
  {
  public:
    using Distance_Type = typename Distance::Distance_Type;
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    static_assert(std::is_integral_v<Distance_Type>,
                  "Dial's algorithm requires integral distance type");

  private:
    SA sa;
    Distance distance;
    bool painted = false;
    GT *ptr_g = nullptr;
    Node *s = nullptr;
    static constexpr size_t Max_Sane_Buckets = 16u * 1024u * 1024u;

    static constexpr Distance_Type Inf =
        std::numeric_limits<Distance_Type>::max();

    struct Node_Info
    {
      Distance_Type dist;
      Node *parent;
      Arc *parent_arc;

      Node_Info() noexcept : dist(Inf), parent(nullptr), parent_arc(nullptr) {}
    };

#define DIAL_INFO(p)   (static_cast<Node_Info *>(NODE_COOKIE(p)))
#define DIAL_DIST(p)   (DIAL_INFO(p)->dist)
#define DIAL_PARENT(p) (DIAL_INFO(p)->parent)
#define DIAL_PARC(p)   (DIAL_INFO(p)->parent_arc)

    void reset_state_after_failure() noexcept
    {
      painted = false;
      ptr_g = nullptr;
      s = nullptr;
    }

    class Dial_Init_Guard
    {
      Dial_Min_Paths * owner = nullptr;
      const GT * g = nullptr;
      bool committed = false;

    public:
      Dial_Init_Guard(Dial_Min_Paths & __owner, const GT & __g) noexcept
        : owner(&__owner), g(&__g)
      {
        // empty
      }

      Dial_Init_Guard(const Dial_Init_Guard &) = delete;
      Dial_Init_Guard & operator = (const Dial_Init_Guard &) = delete;
      Dial_Init_Guard(Dial_Init_Guard &&) = delete;
      Dial_Init_Guard & operator = (Dial_Init_Guard &&) = delete;

      void commit() noexcept { committed = true; }

      ~Dial_Init_Guard() noexcept
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
            NODE_COOKIE(it.get_curr()) = new Node_Info;
        }
      catch (...)
        {
          uninit_discard();
          reset_state_after_failure();
          throw;
        }
    }

    void uninit_paint()
    {
      for (typename GT::Node_Iterator it(*ptr_g); it.has_curr(); it.next())
        {
          auto p = it.get_curr();
          auto info = DIAL_INFO(p);
          auto parent = info->parent;
          delete info;
          NODE_COOKIE(p) = parent;
        }
    }

    void uninit_discard()
    {
      if (ptr_g == nullptr)
        return;

      for (typename GT::Node_Iterator it(*ptr_g); it.has_curr(); it.next())
        {
          auto p = it.get_curr();
          auto info = DIAL_INFO(p);
          if (info != nullptr)
            delete info;
          NODE_COOKIE(p) = nullptr;
          ptr_g->reset_bit(p, Aleph::Spanning_Tree);
        }

      for (typename GT::Arc_Iterator it(*ptr_g); it.has_curr(); it.next())
        ptr_g->reset_bit(it.get_curr(), Aleph::Spanning_Tree);
    }

    Distance_Type find_max_weight(const GT & g)
    {
      Distance_Type max_w = 0;
      for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next())
        {
          auto w = distance(it.get_curr());
          ah_domain_error_if(w < 0) << "Dial: negative weight " << w << " not allowed";
          if (w > max_w)
            max_w = w;
        }
      return max_w;
    }

    void run_dial(const GT & g, Node *start, Node *end = nullptr)
    {
      auto max_w = find_max_weight(g);

      if (max_w == 0)
        {
          // All edges have weight 0: simple BFS
          DIAL_DIST(start) = 0;
          NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);

          DynList<Node *> queue;
          queue.append(start);

          while (not queue.is_empty())
            {
              auto curr = queue.remove_first();

              if (end != nullptr and curr == end)
                break;

              for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
                {
                  auto arc = it.get_current_arc();
                  auto tgt = g.get_connected_node(arc, curr);

                  if (DIAL_DIST(tgt) > 0)
                    {
                      DIAL_DIST(tgt) = 0;
                      DIAL_PARENT(tgt) = curr;
                      DIAL_PARC(tgt) = arc;
                      NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
                      ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
                      queue.append(tgt);
                    }
                }
            }
          return;
        }

      auto num_nodes = g.get_num_nodes();
      const size_t path_edge_bound = num_nodes > 0 ? num_nodes - 1 : 0;
      const size_t max_w_size = static_cast<size_t>(max_w);

      // Maximum possible shortest-path distance in this graph:
      // (|V|-1) * max_w. Compute it with checked size_t arithmetic.
      ah_overflow_error_if(max_w_size != 0 and
                           path_edge_bound > std::numeric_limits<size_t>::max() / max_w_size)
        << "Dial: overflow computing max_dist = (|V|-1) * max_w with |V|="
        << num_nodes << " and max_w=" << max_w;

      const size_t max_dist_size = path_edge_bound * max_w_size;
      ah_overflow_error_if(max_dist_size > static_cast<size_t>(std::numeric_limits<Distance_Type>::max()))
        << "Dial: max_dist " << max_dist_size
        << " exceeds Distance_Type range; use a wider type or Dijkstra";
      ah_overflow_error_if(max_dist_size == std::numeric_limits<size_t>::max())
        << "Dial: overflow computing number of buckets";

      const Distance_Type max_dist = static_cast<Distance_Type>(max_dist_size);
      const size_t num_buckets = max_dist_size + 1;
      ah_runtime_error_if(num_buckets > Max_Sane_Buckets)
        << "Dial: bucket count " << num_buckets
        << " exceeds safety limit " << Max_Sane_Buckets
        << "; use Dijkstra_Min_Paths for large weight ranges";

      Array<DynList<Node *>> buckets(num_buckets, DynList<Node *>());

      DIAL_DIST(start) = 0;
      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      buckets[0].append(start);

      size_t processed = 0;

      for (size_t idx = 0; idx < num_buckets and processed < num_nodes; ++idx)
        {
          while (not buckets[idx].is_empty())
            {
              auto curr = buckets[idx].remove_first();
              auto curr_dist = DIAL_DIST(curr);

              // Skip if this entry is stale (node was already relaxed to
              // a shorter distance)
              if (static_cast<size_t>(curr_dist) != idx)
                continue;

              ++processed;

              if (end != nullptr and curr == end)
                return;

              for (Itor<GT, SA> it(curr, sa); it.has_curr(); it.next())
                {
                  auto arc = it.get_current_arc();
                  auto tgt = g.get_connected_node(arc, curr);
                  auto w = distance(arc);
                  ah_domain_error_if(w < 0) << "Dial: negative weight " << w
                                            << " not allowed";
                  ah_overflow_error_if(curr_dist > std::numeric_limits<Distance_Type>::max() - w)
                    << "Dial: overflow computing new_dist = curr_dist + w";
                  auto new_dist = curr_dist + w;

                  // Distances above the computed bound are not representable in buckets.
                  if (new_dist > max_dist)
                    continue;

                  if (new_dist < DIAL_DIST(tgt))
                    {
                      // Clear previous Spanning_Tree mark for tgt's parent arc
                      // to correctly handle multi-edges and avoid stale marks.
                      if (DIAL_PARC(tgt) != nullptr)
                        ARC_BITS(DIAL_PARC(tgt)).set_bit(Aleph::Spanning_Tree, false);

                      DIAL_DIST(tgt) = new_dist;
                      DIAL_PARENT(tgt) = curr;
                      DIAL_PARC(tgt) = arc;
                      NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
                      ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
                      buckets[static_cast<size_t>(new_dist)].append(tgt);
                    }
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
    Dial_Min_Paths(Distance dist = Distance(), SA __sa = SA())
      : sa(__sa), distance(dist)
    {
      // empty
    }

    /** @brief Check if the graph has been painted.
     *  @return true if a painting method was called, false otherwise.
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
     *         weight is negative.
     *  @throw bad_alloc If there is not enough memory.
     *
     *  @note Time complexity: O(V + E + V*C) where C is max edge weight.
     */
    void paint_min_paths_tree(const GT & g, Node *start)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

      painted = false;
      s = nullptr;

      init(g);
      Dial_Init_Guard guard(*this, g);
      s = start;
      run_dial(g, start);
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
    [[nodiscard]] Distance_Type get_distance(Node *node)
    {
      ah_domain_error_if(not painted) << "Graph has not been painted";
      ah_domain_error_if(node == nullptr) << "node cannot be null";

      if (node == s)
        return Distance_Type(0);

      ah_domain_error_if(not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
      << "node is not reachable from start";

      Distance_Type total = 0;
      for (auto curr = node; curr != s;)
        {
          auto parent = static_cast<Node *>(NODE_COOKIE(curr));
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
    Distance_Type get_min_path(Node *end, Path<GT> & path)
    {
      ah_domain_error_if(not painted) << "Graph has not been painted";
      ah_domain_error_if(ptr_g == nullptr) << "No computation has been done";

      return Aleph::get_min_path<GT, Distance>(s, end, path);
    }

    /** @brief Computes the shortest path from start to end.
     *
     *  Paints the graph (stopping early when end is found) and extracts
     *  the path.
     *
     *  @param[in] g The graph.
     *  @param[in] start The starting node.
     *  @param[in] end The destination node.
     *  @param[out] path The shortest path (empty if no path exists).
     *  @return The total cost, or max value if no path exists.
     *  @throw domain_error If start or end is nullptr, g is empty, or
     *         any arc weight is negative.
     *  @throw bad_alloc If there is not enough memory.
     *
     *  @note Time complexity: O(V + E + V*C) where C is max edge weight.
     */
    Distance_Type find_min_path(const GT & g, Node *start, Node *end,
                                Path<GT> & path)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(end == nullptr) << "end node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

      path.empty();

      painted = false;
      s = nullptr;

      init(g);
      Dial_Init_Guard guard(*this, g);
      s = start;
      run_dial(g, start, end);
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
    Distance_Type operator()(const GT & g, Node *start, Node *end,
                             Path<GT> & path)
    {
      return find_min_path(g, start, end, path);
    }

#undef DIAL_INFO
#undef DIAL_DIST
#undef DIAL_PARENT
#undef DIAL_PARC
  };
} // end namespace Aleph

# endif // DIAL_H
