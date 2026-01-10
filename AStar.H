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
 *  @ingroup Grafos
 *  @author Leandro Rabindranath León
 */

# ifndef ASTAR_H
# define ASTAR_H

# include <ahFunction.H>
# include <ah-errors.H>
# include <archeap.H>
# include <tpl_find_path.H>
# include <tpl_agraph.H>

namespace Aleph
{
  // Cookie to Node_Info conversion
# define ANassert(p) ((Node_Info*) NODE_COOKIE((p)))

  // Access to tree node in the graph
# define ATREENODE(p) (((Tree_Node_Info*)ANassert(p))->tree_node)

# define AACC(p) (ANassert(p)->g_cost)      // g(n): accumulated cost from start
# define AFCOST(p) (ANassert(p)->f_cost)    // f(n) = g(n) + h(n)
# define AHEAPNODE(p) (ANassert(p)->heap_node)
# define APARENT(p) (ANassert(p)->ret_node)

# define AAassert(p) ((Arc_Info*) ARC_COOKIE(p))
# define AARC_DIST(p) (Distance () (p))
# define ATREEARC(p) (((Tree_Arc_Info*)AAassert(p))->tree_arc)
# define APOT(p) (AAassert(p)->pot)        // f-cost for the arc's target
# define AGRAPHNODE(p) (static_cast<typename GT::Node*>(NODE_COOKIE(p)))

  /** @brief Default heuristic for A* (zero heuristic, degrades to Dijkstra).
   *
   *  This is a trivial heuristic that always returns 0. When used,
   *  A* behaves identically to Dijkstra's algorithm.
   *
   *  @tparam GT Graph type.
   *  @tparam Distance Distance accessor functor.
   *
   *  @ingroup Grafos
   */
  template <class GT, class Distance = Dft_Dist<GT>>
  struct Zero_Heuristic
  {
    using Distance_Type = typename Distance::Distance_Type;

    Distance_Type operator()(typename GT::Node *, typename GT::Node *) const
    {
      return Distance_Type(0);
    }
  };

  /** @brief A* algorithm for finding shortest path between two nodes.
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
   *  The class receives the following template parameters:
   *  -# `GT`: graph type.
   *  -# `Distance<GT>`: class reading the weight. It should export:
   *     - `typedef Distance<GT>::Distance_Type`: Data type for weights.
   *     - `Distance<GT>::Distance_Type operator()(typename GT::Arc *a)`:
   *       returns the weight of arc `a`.
   *  -# `Heuristic<GT, Distance>`: heuristic functor. It should export:
   *     - `Distance_Type operator()(Node *from, Node *to)`: returns the
   *       estimated cost from `from` to `to`.
   *  -# `Itor<GT, SA>`: arc iterator type (defaults to Node_Arc_Iterator).
   *  -# `SA`: arc filter for internal iterators.
   *  -# `HeapT<GT, Distance, Access>`: priority-queue adapter.
   *
   *  @note To use a Fibonacci heap, pass ArcFibonacciHeap as HeapT.
   *
   *  @warning This class is **not thread-safe**. Concurrent calls require
   *           external synchronization.
   *
   *  @see Dijkstra_Min_Paths Floyd_All_Shortest_Paths
   *
   *  @ingroup Grafos
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            class Heuristic = Zero_Heuristic<GT, Distance>,
            template <typename, class> class Itor = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>,
            template <class, class, class> class HeapT = ArcHeap>
  class AStar_Min_Path
  {
    // Information to place on the arc for the painting version
    struct Arc_Info
    {
      typename Distance::Distance_Type pot; // f-cost through this arc
    };

    // Information for the tree-building version
    struct Tree_Arc_Info : public Arc_Info
    {
      typename GT::Arc *tree_arc = nullptr;
    };

    // Wrapper to get arc potential (f-cost)
    struct Get_Potential_Arc : public Distance
    {
      Get_Potential_Arc() = default;

      Get_Potential_Arc(Distance & d) noexcept : Distance(d)
      { /* empty */ }

      typename Distance::Distance_Type
      operator()(typename GT::Arc *a) const noexcept
      {
        auto arc_info = static_cast<Arc_Info *>(ARC_COOKIE(a));
        return arc_info->pot;
      }
    };

    // Information for node in painting version
    struct Node_Info
    {
      typename Distance::Distance_Type g_cost = 0; // cost from start
      typename Distance::Distance_Type f_cost = 0; // g + h (total estimate)
      void *heap_node = nullptr;
      void *ret_node = nullptr; // parent in path
    };

    // Information for node in tree-building version
    struct Tree_Node_Info : public Node_Info
    {
      typename GT::Node *tree_node = nullptr;
    };

    // Access to the arc heap
    struct AStar_Heap_Info
    {
      void *&operator()(typename GT::Node *p) const noexcept
      {
        return AHEAPNODE(p);
      }
    };

    // Node initialization for painting version
    struct Initialize_Node
    {
      void operator()(const GT & g, typename GT::Node *p) const
      {
        g.reset_bit(p, Aleph::Spanning_Tree);
        NODE_COOKIE(p) = new Node_Info;
      }
    };

    // Node cleanup for painting version
    struct Destroy_Node
    {
      void operator()(const GT &, typename GT::Node *p) const noexcept
      {
        void *tmp = APARENT(p);
        delete ANassert(p);
        NODE_COOKIE(p) = tmp;
      }
    };

    // Arc initialization for painting version
    struct Initialize_Arc
    {
      void operator()(const GT & g, typename GT::Arc *a) const
      {
        g.reset_bit(a, Aleph::Spanning_Tree);
        ARC_COOKIE(a) = new Arc_Info;
        APOT(a) = 0;
      }
    };

    // Arc cleanup for painting version
    struct Destroy_Arc
    {
      void operator()(const GT &, typename GT::Arc *ga) const noexcept
      {
        delete AAassert(ga);
      }
    };

    // Node initialization for tree-building version
    struct Initialize_Tree_Node
    {
      void operator()(const GT & g, typename GT::Node *p) const
      {
        g.reset_bit(p, Aleph::Spanning_Tree);
        NODE_COOKIE(p) = new Tree_Node_Info;
      }
    };

    // Node cleanup and mapping for tree-building version
    struct Destroy_Tree_Node
    {
      void operator()(const GT &, typename GT::Node *p) const noexcept
      {
        auto aux = static_cast<Tree_Node_Info *>(ANassert(p));
        auto tp = ATREENODE(p);
        if (tp != nullptr)
          {
            NODE_COOKIE(p) = NODE_COOKIE(tp) = nullptr;
            GT::map_nodes(p, tp);
          }
        else
          NODE_COOKIE(p) = nullptr;

        delete aux;
      }
    };

    // Arc initialization for tree-building version
    struct Initialize_Tree_Arc
    {
      void operator()(const GT & g, typename GT::Arc *a) const
      {
        g.reset_bit(a, Aleph::Spanning_Tree);
        ARC_COOKIE(a) = new Tree_Arc_Info;
        APOT(a) = 0;
        ATREEARC(a) = nullptr;
      }
    };

    // Arc cleanup and mapping for tree-building version
    struct Destroy_Tree_Arc
    {
      void operator()(const GT &, typename GT::Arc *ga) const noexcept
      {
        auto aux = static_cast<Tree_Arc_Info *>(AAassert(ga));
        typename GT::Arc *ta = ATREEARC(ga);
        if (ta != nullptr)
          {
            assert(IS_ARC_VISITED(ga, Aleph::Spanning_Tree));
            GT::map_arcs(ga, ta);
          }

        delete aux;
      }
    };

    typedef AStar_Heap_Info Heap_Info;
    typedef HeapT<GT, Get_Potential_Arc, Heap_Info> Heap;

    SA sa;
    Distance dist;
    Heuristic heuristic;
    Get_Potential_Arc get_pot;
    Heap heap;
    bool painted = false;
    GT *ptr_g = nullptr;
    typename GT::Node *s = nullptr;

  public:
    /** Constructor.
     *
     *  @param[in] __dist Distance functor for arc weights.
     *  @param[in] __heuristic Heuristic functor for estimating remaining cost.
     *  @param[in] __sa Arc filter for iterators.
     */
    AStar_Min_Path(Distance __dist = Distance(),
                   Heuristic __heuristic = Heuristic(),
                   SA __sa = SA())
      : sa(__sa), dist(__dist), heuristic(__heuristic),
        get_pot(dist), heap(get_pot, Heap_Info()),
        painted(false), ptr_g(nullptr), s(nullptr)
    {
      // empty
    }

    /// @name State getters
    /// @{

    /** Check if a computation has been performed.
     *  @return true if a path has been computed, false otherwise.
     */
    [[nodiscard]] bool has_computation() const noexcept { return ptr_g != nullptr; }

    /** Check if the graph has been painted.
     *  @return true if paint_path was called, false otherwise.
     */
    [[nodiscard]] bool is_painted() const noexcept { return painted; }

    /** Get the start node of the last computation.
     *  @return Pointer to the start node, or nullptr if none.
     */
    typename GT::Node * get_start_node() const noexcept { return s; }

    /** Get the graph of the last computation.
     *  @return Pointer to the graph, or nullptr if none.
     */
    GT * get_graph() const noexcept { return ptr_g; }

    /// @}

  private:
    template <class IN, class IA>
    void init(const GT & g, typename GT::Node *start)
    {
      heap.empty();

      ptr_g = &const_cast<GT &>(g);
      s = start;

      Operate_On_Nodes<GT, IN>()(g);
      (Operate_On_Arcs<GT, IA>(sa))(g);
    }

    template <class DN, class DA>
    void uninit()
    {
      Operate_On_Nodes<GT, DN>()(*ptr_g);
      (Operate_On_Arcs<GT, DA, SA>(sa))(*ptr_g);
    }

    /// Checked addition to prevent integer overflow
    typename Distance::Distance_Type
    checked_add(const typename Distance::Distance_Type & a,
                const typename Distance::Distance_Type & b) const
    {
      using DT = typename Distance::Distance_Type;
      if constexpr (std::is_integral_v<DT>)
        {
          ah_overflow_error_if(b > 0 and a > std::numeric_limits<DT>::max() - b)
            << "Integer overflow in distance addition: " << a << " + " << b;

          ah_overflow_error_if(b < 0 and a < std::numeric_limits<DT>::min() - b)
            << "Integer underflow in distance addition: " << a << " + " << b;
        }

      return a + b;
    }

  public:
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
    typename GT::Node *
    compute_path(const GT & g,
                 typename GT::Node *start,
                 typename GT::Node *end,
                 GT & tree)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(end == nullptr) << "end node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

      init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);
      clear_graph(tree);

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      AACC(start) = 0;
      AFCOST(start) = heuristic(start, end);
      ATREENODE(start) = tree.insert_node(start->get_info());
      NODE_COOKIE(ATREENODE(start)) = start;

      // Insert initial arcs from start
      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          auto tgt = it.get_tgt_node();
          auto g_cost = AARC_DIST(arc);
          auto h_cost = heuristic(tgt, end);
          APOT(arc) = checked_add(g_cost, h_cost); // f = g + h
          heap.put_arc(arc, tgt);
        }

      typename GT::Node *result = nullptr;

      while (not heap.is_empty())
        {
          auto garc = heap.get_min_arc();
          if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
            continue;

          auto gsrc = g.get_src_node(garc);
          auto gtgt = g.get_tgt_node(garc);

          // Skip if both nodes already visited (would create cycle)
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

          AACC(gtgt) = checked_add(AACC(gsrc), AARC_DIST(garc));

          // Check if we reached the destination
          if (gtgt == end)
            {
              result = ttgt;
              break;
            }

          const auto & g_cost = AACC(gtgt);

          // Expand neighbors
          for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              auto tgt = it.get_tgt_node();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue;

              auto new_g = checked_add(g_cost, AARC_DIST(arc));
              auto h = heuristic(tgt, end);
              APOT(arc) = checked_add(new_g, h); // f = g + h
              heap.put_arc(arc, tgt);
            }
        }

      uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();

      return result;
    }

    /** Paints the shortest path from start to end on the graph.
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
    bool paint_path(const GT & g,
                    typename GT::Node *start,
                    typename GT::Node *end)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(end == nullptr) << "end node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";

      // Special case: start equals end
      if (start == end)
        {
          ptr_g = &const_cast<GT &>(g);
          s = start;
          painted = true;
          return true;
        }

      init<Initialize_Node, Initialize_Arc>(g, start);

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      AACC(start) = 0;
      AFCOST(start) = heuristic(start, end);

      // Insert initial arcs from start
      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          auto tgt = it.get_tgt_node();
          auto g_cost = AARC_DIST(arc);
          auto h_cost = heuristic(tgt, end);
          APOT(arc) = checked_add(g_cost, h_cost);
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

          // Check if destination reached
          if (tgt == end)
            {
              found = true;
              break;
            }

          AACC(tgt) = checked_add(AACC(src), AARC_DIST(garc));
          const auto & g_cost = AACC(tgt);

          // Expand neighbors
          for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              auto t = it.get_tgt_node();
              if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
                continue;

              auto new_g = checked_add(g_cost, AARC_DIST(arc));
              auto h = heuristic(t, end);
              APOT(arc) = checked_add(new_g, h);
              heap.put_arc(arc, t);
            }
        }

      uninit<Destroy_Node, Destroy_Arc>();
      painted = true;

      return found;
    }

    /** Extracts the shortest path from a previously painted graph.
     *
     *  @param[in] end Destination node.
     *  @param[out] path Path where the result will be stored.
     *  @return The total cost of the path.
     *  @throw domain_error If the graph has not been painted.
     */
    typename Distance::Distance_Type
    get_min_path(typename GT::Node *end, Path<GT> & path)
    {
      ah_domain_error_if(ptr_g == nullptr) << "Path has not been computed";
      ah_domain_error_if(not painted) << "Graph has not been painted";

      return Aleph::get_min_path<GT, Distance>(s, end, path);
    }

    /** Finds the shortest path from start to end using A*.
     *
     *  This is the recommended entry point. It paints the graph and
     *  extracts the path in one call.
     *
     *  @param[in] g The graph.
     *  @param[in] start The starting node.
     *  @param[in] end The destination node.
     *  @param[out] path The shortest path (empty if no path exists).
     *  @return The total cost, or max value if no path exists.
     *  @throw bad_alloc If there is not enough memory.
     */
    typename Distance::Distance_Type
    find_path(const GT & g,
              typename GT::Node *start,
              typename GT::Node *end,
              Path<GT> & path)
    {
      path.empty();
      if (paint_path(g, start, end))
        return get_min_path(end, path);

      return std::numeric_limits<typename Distance::Distance_Type>::max();
    }

    /** Operator shortcut for find_path.
     *
     *  @param[in] g The graph.
     *  @param[in] start The starting node.
     *  @param[in] end The destination node.
     *  @param[out] path The shortest path.
     *  @return The total cost.
     */
    typename Distance::Distance_Type
    operator()(const GT & g,
               typename GT::Node *start,
               typename GT::Node *end,
               Path<GT> & path)
    {
      return find_path(g, start, end, path);
    }

    /** Gets the g-cost (accumulated distance) to a node after painting.
     *
     *  @param[in] node The node to query.
     *  @return The accumulated distance from start to node.
     *  @throw domain_error If not painted or node unreachable.
     */
    typename Distance::Distance_Type
    get_g_cost(typename GT::Node *node)
    {
      ah_domain_error_if(not painted) << "Graph has not been painted";
      ah_domain_error_if(node == nullptr) << "node cannot be null";

      if (node == s)
        return 0;

      ah_domain_error_if(not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
        << "node is not reachable from start";

      typename Distance::Distance_Type total = 0;

      for (auto curr = node; curr != s;)
        {
          auto parent = static_cast<typename GT::Node *>(NODE_COOKIE(curr));
          if (parent == nullptr)
            break;

          for (Itor<GT, SA> it(parent, sa); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_current_arc_ne();
              auto tgt = it.get_tgt_node();
              if (tgt == curr and IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                {
                  total += Distance()(arc);
                  break;
                }
            }
          curr = parent;
        }

      return total;
    }
  };

  /** @brief Euclidean distance heuristic for A* in 2D grids.
   *
   *  Example heuristic for graphs where nodes have (x, y) coordinates.
   *  Assumes `node->get_info()` returns a struct with `x` and `y` fields.
   *
   *  @tparam GT Graph type.
   *  @tparam Distance Distance accessor functor.
   *
   *  @ingroup Grafos
   */
  template <class GT, class Distance = Dft_Dist<GT>>
  struct Euclidean_Heuristic
  {
    using Distance_Type = typename Distance::Distance_Type;

    Distance_Type operator()(typename GT::Node *from, typename GT::Node *to) const
    {
      auto & f = from->get_info();
      auto & t = to->get_info();
      auto dx = f.x - t.x;
      auto dy = f.y - t.y;
      return static_cast<Distance_Type>(std::sqrt(dx*dx + dy*dy));
    }
  };

  /** @brief Manhattan distance heuristic for A* in grid graphs.
   *
   *  Example heuristic for graphs where nodes have (x, y) coordinates
   *  and movement is restricted to horizontal/vertical directions.
   *  Assumes `node->get_info()` returns a struct with `x` and `y` fields.
   *
   *  @tparam GT Graph type.
   *  @tparam Distance Distance accessor functor.
   *
   *  @ingroup Grafos
   */
  template <class GT, class Distance = Dft_Dist<GT>>
  struct Manhattan_Heuristic
  {
    using Distance_Type = typename Distance::Distance_Type;

    Distance_Type operator()(typename GT::Node *from, typename GT::Node *to) const
    {
      auto & f = from->get_info();
      auto & t = to->get_info();
      return static_cast<Distance_Type>(std::abs(f.x - t.x) + std::abs(f.y - t.y));
    }
  };

# undef ANassert
# undef APARENT
# undef ATREENODE
# undef AACC
# undef AFCOST
# undef AHEAPNODE
# undef AAassert
# undef AARC_DIST
# undef ATREEARC
# undef APOT
# undef AGRAPHNODE

} // end namespace Aleph

# endif // ASTAR_H