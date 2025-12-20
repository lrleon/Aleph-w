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
# ifndef DIJKSTRA_H
# define DIJKSTRA_H

# include <cassert>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ahFunction.H>
# include <ah-errors.H>
# include <ah_init_guard.H>
# include <archeap.H>
# include <tpl_find_path.H>
# include <tpl_agraph.H>

namespace Aleph
{
  // Conversion from cookie to Node_Info
# define DNassert(p) ((Node_Info*) NODE_COOKIE((p)))

  // Access to the tree node in the graph
# define TREENODE(p) (((Tree_Node_Info*)DNassert(p))->tree_node)

# define ACC(p) (DNassert(p)->dist) // Access to accumulated distance
# define HEAPNODE(p) (DNassert(p)->heap_node)
# define PARENT(p) (DNassert(p)->ret_node)

# define DAassert(p) ((Arc_Info*) ARC_COOKIE(p))
# define ARC_DIST(p) (Distance () (p))
# define TREEARC(p) (((Tree_Arc_Info*)DAassert(p))->tree_arc)
# define POT(p) (DAassert(p)->pot)
# define GRAPHNODE(p) (static_cast<typename GT::Node*>(NODE_COOKIE(p)))


  /** @brief Shortest paths and shortest-path trees (Dijkstra).
   *
   * Computes shortest paths from a single source node using Dijkstra's
   * algorithm.
   *
   * Two usage modes are provided:
   * - **Painting mode**: marks the shortest-path tree directly on the input
   *   graph (via the `Spanning_Tree` control bit) and stores predecessor
   *   pointers in node cookies so that get_min_path() can later reconstruct a
   *   shortest path.
   * - **Tree-building mode**: builds a separate graph that contains the
   *   shortest-path tree and maps nodes/arcs between the input graph and the
   *   tree via cookies.
   *
   * Template parameters:
   * - `GT`: graph type.
   * - `Distance`: arc weight accessor. Must define `Distance_Type` and provide
   *   `Distance_Type operator()(typename GT::Arc *a)`.
   * - `Itor`: iterator template used to traverse arcs incident to a node.
   * - `SA`: arc filter for the internal iterators.
   *
   * @warning Dijkstra's algorithm requires non-negative weights. For signed
   * arithmetic `Distance_Type`, negative weights throw `std::domain_error`.
   *
   * @ingroup Grafos
   * @see Find_Path_Depth_First Floyd_All_Shortest_Paths Bellman_Ford
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            template <typename, class> class Itor = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Dijkstra_Min_Paths
  {
    // Although the algorithm is fundamentally the same, there are two approaches
    // for computing the shortest path. The first is to paint the spanning
    // tree of all shortest paths from a start node. "Painting" means that
    // the solution is found within the same graph and is distinguished by marks.
    //
    // The other approach consists of building a separate spanning tree.
    //
    // Depending on the approach, the graph is initialized with different
    // information. Thus, classes prefixed with "Tree" are classes that
    // pertain to the solution that builds a separate spanning tree.

    // Information to place on the arc for the painting version
    struct Arc_Info
    {
      typename Distance::Distance_Type pot{}; // arc potential (tentative distance)
    };

    // Information to place on the arc for the tree-building version
    struct Tree_Arc_Info : public Arc_Info
    {
      typename GT::Arc *tree_arc = nullptr; // image in tree
      // Note: pot is inherited from Arc_Info
    };

    // Distance access wrapper (via the Distance template parameter class)
    struct Get_Potential_Arc : public Distance
    {
      Get_Potential_Arc() noexcept
      { /* empty */
      }

      Get_Potential_Arc(Distance & d) noexcept : Distance(d)
      { /* empty */
      }

      typename Distance::Distance_Type
      operator ()(typename GT::Arc *a) const noexcept
      {
        auto arc_info = static_cast<Arc_Info *>(ARC_COOKIE(a));
        return arc_info->pot;
      }
    };

    // Information to place on the node for the painting version
    struct Node_Info
    {
      typename Distance::Distance_Type dist{}; // accumulated distance
      void *heap_node = nullptr;
      void *ret_node = nullptr; // parent in tree
    };

    // Information to place on the node for the tree-building version
    struct Tree_Node_Info : public Node_Info
    {
      typename GT::Node *tree_node = nullptr; // spanning tree node
      // Note: dist and heap_node are inherited from Node_Info
    };

    // Access to the arc heap
    struct Dijkstra_Heap_Info
    {
      typedef typename
      ArcHeap<GT, Get_Potential_Arc, Dijkstra_Heap_Info>::Node Node;

      Node *&operator ()(typename GT::Node *p) const noexcept
      {
        return reinterpret_cast<Node *&>(HEAPNODE(p));
      }
    };

    // Node initialization for the painting version
    struct Initialize_Node
    {
      void operator ()(const GT & g, typename GT::Node *p) const noexcept
      {
        g.reset_bit(p, Aleph::Spanning_Tree);
        NODE_COOKIE(p) = new Node_Info;
      }
    };

    // Node memory release for the painting version
    struct Destroy_Node
    {
      void operator ()(const GT &, typename GT::Node *p) const noexcept
      {
        void *tmp = PARENT(p);
        delete DNassert(p); // block to release
        NODE_COOKIE(p) = tmp;
      }
    };

    // Arc initialization for the painting version
    struct Initialize_Arc
    {
      void operator ()(const GT & g, typename GT::Arc *a) const noexcept
      {
        g.reset_bit(a, Aleph::Spanning_Tree);
        ARC_COOKIE(a) = new Arc_Info;
        POT(a) = typename Distance::Distance_Type{};
      }
    };

    // Arc memory release for the painting version
    struct Destroy_Arc
    {
      void operator ()(const GT &, typename GT::Arc *ga) const noexcept
      {
        delete DAassert(ga);
        ARC_COOKIE(ga) = nullptr;
      }
    };

    // Node memory initialization for the tree-building version
    struct Initialize_Tree_Node
    {
      void operator ()(const GT & g, typename GT::Node *p) const noexcept
      {
        g.reset_bit(p, Aleph::Spanning_Tree);
        NODE_COOKIE(p) = new Tree_Node_Info;
      }
    };

    // Node memory release and mapping for the tree-building version
    struct Destroy_Tree_Node
    {
      void operator ()(const GT &, typename GT::Node *p) const noexcept
      {
        auto aux = static_cast<Tree_Node_Info *>(DNassert(p)); // block to release
        auto tp = TREENODE(p); // image in spanning tree
        if (tp != nullptr) // is this node included in the spanning tree?
          {
            NODE_COOKIE(p) = NODE_COOKIE(tp) = nullptr;
            GT::map_nodes(p, tp);
          }
        else
          NODE_COOKIE(p) = nullptr;

        delete aux;
      }
    };

    // Arc initialization for the tree-building version
    struct Initialize_Tree_Arc
    {
      void operator ()(const GT & g, typename GT::Arc *a) const noexcept
      {
        g.reset_bit(a, Aleph::Spanning_Tree);
        ARC_COOKIE(a) = new Tree_Arc_Info;
        POT(a) = typename Distance::Distance_Type{};
        TREEARC(a) = nullptr;
      }
    };

    // Arc memory release and mapping for the tree-building version
    struct Destroy_Tree_Arc
    {
      void operator ()(const GT &, typename GT::Arc *ga) const noexcept
      {
        auto aux = static_cast<Tree_Arc_Info *>(ARC_COOKIE(ga));
        typename GT::Arc *ta = TREEARC(ga);
        if (ta != nullptr) // does this arc belong to the spanning tree?
          {
            assert(IS_ARC_VISITED(ga, Aleph::Spanning_Tree));
            ARC_COOKIE(ga) = ARC_COOKIE(ta) = nullptr;
            GT::map_arcs(ga, ta); // yes ==> map it
          }
        else
          ARC_COOKIE(ga) = nullptr;

        delete aux;
      }
    };

    typedef Dijkstra_Heap_Info Heap_Info;

    typedef ArcHeap<GT, Get_Potential_Arc, Heap_Info> Heap;

    SA sa;
    Distance dist;
    Get_Potential_Arc get_pot;
    Heap heap;
    bool painted = false;
    GT *ptr_g = nullptr;
    typename GT::Node *s = nullptr;

  public:
    // Constructors

    /** Construct a Dijkstra executor.
     *
     * @param[in] distance Arc-weight accessor.
     * @param[in] __sa Arc filter for internal iterators.
     */
    Dijkstra_Min_Paths(Distance distance = Distance(), SA __sa = SA())
      : sa(__sa),
        dist(distance),
        get_pot(distance),
        heap(get_pot, Heap_Info()),
        painted(false),
        ptr_g(nullptr),
        s(nullptr)
    {
      // empty
    }

  private:
    template <class IN, class IA>
    void init(const GT & g, typename GT::Node *start)
    {
      heap.empty();

      ptr_g = &const_cast<GT &>(g);
      s = start;
      painted = false;

      Operate_On_Nodes<GT, IN>()(g);

      (Operate_On_Arcs<GT, IA>(sa))(g);
    }

    template <class DN, class DA>
    void uninit()
    {
      Operate_On_Nodes<GT, DN>()(*ptr_g);

      (Operate_On_Arcs<GT, DA, SA>(sa))(*ptr_g);
    }

    /// Validate the arc weight precondition for Dijkstra's algorithm.
    void validate_weight(const typename Distance::Distance_Type & w) const
    {
      using DT = typename Distance::Distance_Type;
      if constexpr (std::is_arithmetic_v<DT> && std::is_signed_v<DT>)
        ah_domain_error_if(w < DT{})
          << "Dijkstra_Min_Paths: negative arc weight: " << w;
    }

    /// Read an arc weight (as a value) and validate it.
    typename Distance::Distance_Type weight_of(typename GT::Arc *a) const
    {
      const auto w = dist(a);
      validate_weight(w);
      return w;
    }

    /// Checked addition to prevent integer overflow for integral Distance_Type.
    typename Distance::Distance_Type
    checked_add(const typename Distance::Distance_Type & a,
                const typename Distance::Distance_Type & b) const
    {
      using DT = typename Distance::Distance_Type;
      if constexpr (std::is_integral_v<DT>)
        {
          // Check for positive overflow
          ah_overflow_error_if(b > 0 && a > std::numeric_limits<DT>::max() - b)
          << "Integer overflow in distance addition: " << a << " + " << b;

          // Check for negative overflow (underflow)
          ah_overflow_error_if(b < 0 && a < std::numeric_limits<DT>::min() - b)
            << "Integer underflow in distance addition: " << a << " + " << b;
        }

      return a + b;
    }

  public:
    /** Computes the spanning tree of all shortest paths from the start node.

        @param[in] g The graph.
        @param[in] start The starting node for all shortest paths.
        @param[out] tree The spanning tree of all shortest paths starting
        from start, mapped via cookies to the graph g.
        @return Pointer to the tree node corresponding to the start node.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If start is nullptr or g is empty.
     */
    typename GT::Node *
    compute_min_paths_tree(const GT & g, typename GT::Node *start, GT & tree)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";
      init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

      Init_Guard guard([this]() { uninit<Destroy_Tree_Node, Destroy_Tree_Arc>(); });

      clear_graph(tree); // clear destination spanning tree

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      ACC(start) = typename Distance::Distance_Type{};
      auto ret = TREENODE(start) = tree.insert_node(start->get_info());
      NODE_COOKIE(TREENODE(start)) = start;

      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          POT(arc) = weight_of(arc);
          heap.put_arc(arc, it.get_tgt_node());
        }

      const auto & n = g.get_num_nodes();

      while (tree.get_num_nodes() < n and not heap.is_empty()) // while reachable nodes remain
        {
          auto garc = heap.get_min_arc();
          if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
            continue;

          auto gsrc = g.get_src_node(garc);
          auto gtgt = g.get_tgt_node(garc);

          // Are both nodes visited?
          if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
            continue; // inserting arc would cause a cycle

          ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

          if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
            std::swap(gsrc, gtgt);

          NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

          auto ttgt = tree.insert_node(gtgt->get_info());
          TREENODE(gtgt) = ttgt;
          auto tsrc = TREENODE(gsrc);

          auto tarc = tree.insert_arc(tsrc, ttgt, garc->get_info());
          TREEARC(garc) = tarc;

          ACC(gtgt) = checked_add(ACC(gsrc), weight_of(garc)); // total distance from start
          const auto & acc = ACC(gtgt);

          // for each arc compute potential and insert into heap
          for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              auto tgt = it.get_tgt_node();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // would cause cycle ==> not inserted in heap

              POT(arc) = checked_add(acc, weight_of(arc)); // compute potential
              heap.put_arc(arc, tgt);
            }
        }

      guard.release();
      uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();

      return ret;
    }

    /** Computes the partial spanning tree of all shortest paths from start
        that contains the path from start to end.

        compute_partial_min_paths_tree() builds the spanning tree of shortest
        paths from start but the computation stops when the end node is found.

        @param[in] g The graph.
        @param[in] start The starting node for all shortest paths.
        @param[in] end The destination node.
        @param[out] tree The spanning tree of all shortest paths starting
        from start, mapped via cookies to the graph g.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If start or end is nullptr, or if g is empty.
    */
    void compute_partial_min_paths_tree(const GT & g,
                                        typename GT::Node *start,
                                        typename GT::Node *end,
                                        GT & tree)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(end == nullptr) << "end node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";
      init<Initialize_Tree_Node, Initialize_Tree_Arc>(g, start);

      Init_Guard guard([this]() { uninit<Destroy_Tree_Node, Destroy_Tree_Arc>(); });

      clear_graph(tree);

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      ACC(start) = typename Distance::Distance_Type{};
      TREENODE(start) = tree.insert_node(start->get_info());
      NODE_COOKIE(TREENODE(start)) = start;

      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          POT(arc) = weight_of(arc);
          heap.put_arc(arc, it.get_tgt_node());
        }

      const auto & n = g.get_num_nodes();

      while (tree.get_num_nodes() < n and not heap.is_empty()) // while tree doesn't span
        {
          auto garc = heap.get_min_arc();
          if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
            continue;

          auto gsrc = g.get_src_node(garc);
          auto gtgt = g.get_tgt_node(garc);

          // Are both nodes visited?
          if (IS_NODE_VISITED(gsrc, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
            continue; // inserting arc would cause a cycle

          ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

          if (IS_NODE_VISITED(gtgt, Aleph::Spanning_Tree))
            std::swap(gsrc, gtgt);

          auto ttgt = tree.insert_node(gtgt->get_info());
          TREENODE(gtgt) = ttgt;
          NODE_BITS(gtgt).set_bit(Aleph::Spanning_Tree, true);

          auto tarc = // insert new arc in tree
              tree.insert_arc(TREENODE(gsrc), TREENODE(gtgt), garc->get_info());
          TREEARC(garc) = tarc;

          if (gtgt == end) // is end_node in the spanning tree?
            break; // yes ==> the shortest path is already in the spanning tree

          ACC(gtgt) = checked_add(ACC(gsrc), weight_of(garc)); // total distance from start
          const auto & acc = ACC(gtgt);

          // for each arc compute potential and insert into heap
          for (Itor<GT, SA> it(gtgt, sa); it.has_curr(); it.next_ne())
            {
              auto arc = it.get_current_arc_ne();
              if (IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
                continue;

              auto tgt = it.get_tgt_node();
              if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
                continue; // would cause cycle ==> not inserted in heap

              POT(arc) = checked_add(acc, weight_of(arc)); // compute potential
              heap.put_arc(arc, tgt);
            }
        }

      guard.release();
      uninit<Destroy_Tree_Node, Destroy_Tree_Arc>();
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
    bool paint_partial_min_paths_tree(const GT & g,
                                      typename GT::Node *start,
                                      typename GT::Node *end)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(end == nullptr) << "end node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";
      bool ret_val = false;
      init<Initialize_Node, Initialize_Arc>(g, start);

      Init_Guard guard([this]() { uninit<Destroy_Node, Destroy_Arc>(); });

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      ACC(start) = typename Distance::Distance_Type{};

      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          POT(arc) = weight_of(arc);
          heap.put_arc(arc, it.get_tgt_node());
        }

      const auto & n = g.get_num_nodes();
      size_t tn = 1; // number of painted nodes

      while (tn < n and not heap.is_empty()) // while tree doesn't span g
        {
          auto garc = heap.get_min_arc();
          if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
            continue;

          auto src = g.get_src_node(garc);
          auto tgt = g.get_tgt_node(garc);

          // Are both nodes visited?
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // this arc would cause a cycle

          ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

          if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            std::swap(src, tgt);

          NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
          PARENT(tgt) = src;

          ++tn; // simulates adding p to the spanning tree

          if (tgt == end) // is end_node in the spanning tree?
            {
              ret_val = true;
              break; // yes ==> the shortest path is already painted
            }

          ACC(tgt) = checked_add(ACC(src), weight_of(garc)); // total distance from start

          const auto & acc = ACC(tgt);

          // for each arc compute potential and insert into heap
          for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
            {
              auto a = it.get_current_arc_ne();
              if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
                continue;

              auto t = it.get_tgt_node();
              if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
                continue; // would cause cycle ==> not inserted in heap

              POT(a) = checked_add(acc, weight_of(a)); // compute potential
              heap.put_arc(a, t);
            }
        }

      guard.release();
      uninit<Destroy_Node, Destroy_Arc>();
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
    void paint_min_paths_tree(const GT & g, typename GT::Node *start)
    {
      ah_domain_error_if(start == nullptr) << "start node cannot be null";
      ah_domain_error_if(g.get_num_nodes() == 0) << "graph is empty";
      init<Initialize_Node, Initialize_Arc>(g, start);

      Init_Guard guard([this]() { uninit<Destroy_Node, Destroy_Arc>(); });

      NODE_BITS(start).set_bit(Aleph::Spanning_Tree, true);
      ACC(start) = typename Distance::Distance_Type{};

      for (Itor<GT, SA> it(start, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_current_arc_ne();
          POT(arc) = weight_of(arc);
          heap.put_arc(arc, it.get_tgt_node());
        }

      const auto & n = g.get_num_nodes();
      size_t tn = 1; // number of painted nodes

      while (tn < n and not heap.is_empty()) // while tree doesn't span g
        {
          auto garc = heap.get_min_arc();
          if (IS_ARC_VISITED(garc, Aleph::Spanning_Tree))
            continue;

          auto src = g.get_src_node(garc);
          auto tgt = g.get_tgt_node(garc);

          // Are both nodes visited?
          if (IS_NODE_VISITED(src, Aleph::Spanning_Tree) and
              IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            continue; // this arc would cause a cycle

          ARC_BITS(garc).set_bit(Aleph::Spanning_Tree, true);

          if (IS_NODE_VISITED(tgt, Aleph::Spanning_Tree))
            std::swap(src, tgt);

          NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
          PARENT(tgt) = src;

          ++tn; // simulates adding p to the spanning tree

          ACC(tgt) = checked_add(ACC(src), weight_of(garc)); // total distance from start
          const auto & acc = ACC(tgt);

          // for each arc compute potential and insert into heap
          for (Itor<GT, SA> it(tgt, sa); it.has_curr(); it.next_ne())
            {
              auto a = it.get_current_arc_ne();
              if (IS_ARC_VISITED(a, Aleph::Spanning_Tree))
                continue;

              auto t = it.get_tgt_node();
              if (IS_NODE_VISITED(t, Aleph::Spanning_Tree))
                continue; // would cause cycle ==> not inserted in heap

              POT(a) = checked_add(acc, weight_of(a)); // compute potential
              heap.put_arc(a, t);
            }
        }

      guard.release();
      uninit<Destroy_Node, Destroy_Arc>();
      painted = true;
    }

    /** Extracts a shortest path to end from a previously painted graph.

        get_min_path() takes a graph g on which shortest paths (total or
        partial) have been painted by previous calls to paint_min_paths_tree()
        or paint_partial_min_paths_tree() and extracts the shortest path
        to end, storing it in path.

        The result is undefined if end is not contained within the graph;
        i.e., paint_partial_min_paths_tree() was previously called with a
        destination node different from end and the result does not contain end.

        A previous call to paint_min_paths_tree() or
        paint_partial_min_paths_tree() stores the graph and source node.

        @param[in] end Destination node of the path.
        @param[out] path Path where the shortest path will be stored.
        @return The total cost of the path.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If the graph has not been previously painted
        via paint_min_paths_tree() or paint_partial_min_paths_tree().
     */
    typename Distance::Distance_Type
    get_min_path(typename GT::Node *end, Path<GT> & path)
    {
      ah_domain_error_if(ptr_g == nullptr) << "Min path has not been computed";
      ah_domain_error_if(not painted) << "Graph has not previously painted";

      return Aleph::get_min_path<GT, Distance>(s, end, path);
    }

    /** Computes the shortest path between start and end by painting the graph.

        This is the version with the lowest memory consumption and probably
        also the fastest. Note that each call computes the partial shortest
        paths tree from start.

        Use get_min_path() if you have already computed the spanning tree and
        simply want to obtain the shortest path.

        @param[in] g The graph.
        @param[in] start The starting node of the path.
        @param[in] end The destination node of the path.
        @param[out] min_path The shortest path.
        @return The total cost of the path from start to end.
        @throw bad_alloc If there is not enough memory.
        @see get_min_path
     */
    typename Distance::Distance_Type
    find_min_path(const GT & g,
                  typename GT::Node *start, typename GT::Node *end,
                  Path<GT> & min_path)
    {
      min_path.empty();
      if (paint_partial_min_paths_tree(g, start, end))
        return get_min_path(end, min_path);

      return numeric_limits<typename Distance::Distance_Type>::max();
    }

    /** Computes the spanning tree of all shortest paths from a given
        node according to Dijkstra's algorithm.

        The resulting spanning tree of all shortest paths from start_node
        is completely mapped with g. Therefore, a depth-first path search,
        using Find_Path_Depth_First, on tree will find and construct
        any shortest path from start_node to any other node.

        @param[in] g The graph for which to compute the shortest paths
        spanning tree.
        @param[in] s The starting node for the shortest paths.
        @param[out] tree The graph where the resulting spanning tree of
        all shortest paths from start_node will be stored. This graph
        is cleared before the algorithm begins.
        @throw bad_alloc If there is not enough memory to build tree;
        either because tree fails or because of the internal queue. In
        this case the value of tree is undefined and not clean.
     */
    void operator ()(const GT & g, typename GT::Node *s, GT & tree)
    {
      compute_min_paths_tree(g, s, tree);
    }

    /** Extracts the shortest paths tree (partial or total) and puts it in tree.

        copy_painted_min_paths_tree() takes a graph g on which shortest paths
        (total or partial) have been painted by previous calls to
        paint_min_paths_tree() or paint_partial_min_paths_tree() and extracts
        a copy to the tree parameter.

        @param[in] g The previously painted graph where the shortest paths are.
        @param[out] tree The graph where the spanning tree will be copied.
        @return The total distance of the spanning tree.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If the graph has not been previously painted.
    */
    typename Distance::Distance_Type
    copy_painted_min_paths_tree(const GT & g, GT & tree)
    {
      ah_domain_error_if(not painted) << "Graph has not previously painted";

      using Paint_Filt = Painted_Min_Spanning_Tree<GT, Distance>;
      Paint_Filt paint_filter;
      (Copy_Graph<GT, Dft_Show_Node<GT>, Paint_Filt>(paint_filter))(tree, g);

      return paint_filter.dist;
    }

    /// \oveload find_min_path()
    typename Distance::Distance_Type operator ()(const GT & g,
                                                 typename GT::Node *s,
                                                 typename GT::Node *e,
                                                 Path<GT> & path)
    {
      return find_min_path(g, s, e, path);
    }

    /// Distance totalizer class.
    struct Total
    {
      typename Distance::Distance_Type dist;

      Total() noexcept : dist(0)
      { /* empty */
      }

      bool operator ()(typename GT::Arc *a) noexcept
      {
        dist += ARC_DIST(a);
        return true;
      }
    };

    /** Extracts a shortest path to end from a previously computed shortest
        paths tree.

        get_min_path() takes a tree, previously built via
        compute_min_paths_tree() or compute_partial_min_paths_tree(),
        and extracts the shortest path to end, storing it in path.

        The result is undefined if end is not contained within the tree;
        i.e., compute_partial_min_paths_tree() was previously called with a
        destination node different from end and the result does not contain end.

        @param[in] tree Shortest paths tree which must have been previously
        computed via compute_min_paths_tree() or compute_partial_min_paths_tree().
        @param[in] end Destination node of the path.
        @param[out] path Path where the shortest path will be stored.
        @return The total cost of the path.
        @throw bad_alloc If there is not enough memory.
        @throw domain_error If the tree has not been previously computed.
     */
    typename Distance::Distance_Type
    get_min_path(const GT & tree, typename GT::Node *end, Path<GT> & path)
    {
      ah_domain_error_if(ptr_g == nullptr)
        << "Min path has not been computed";

      auto ts = mapped_node<GT>(s);
      auto te = mapped_node<GT>(end);

      Path<GT> tree_path(tree);
      Total total;
      (Find_Path_Depth_First<GT, Itor, Total>(total))(tree, ts, te, tree_path);

      path.empty();
      path.init(s);
      typename Path<GT>::Iterator it(tree_path);
      for (it.next(); it.has_curr(); it.next_ne())
        path.append(mapped_node<GT>(it.get_current_node_ne()));

      return total.dist;
    }
  };


# undef DNassert
# undef PARENT
# undef TREENODE
# undef ACC
# undef HEAPNODE
# undef DAassert
# undef ARC_DIST
# undef TREEARC
# undef POT
# undef GRAPHNODE
} // end namespace Aleph
# endif // DIJKSTRA_H
