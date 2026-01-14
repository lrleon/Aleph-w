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

/** @file Bellman_Ford.H
 *  @brief Bellman-Ford algorithm for single-source shortest paths.
 *
 *  This file implements the Bellman-Ford algorithm, which computes shortest
 *  paths from a single source vertex to all other vertices in a weighted
 *  directed graph. Unlike Dijkstra's algorithm, Bellman-Ford can handle
 *  graphs with negative edge weights and can detect negative-weight cycles.
 *
 *  ## Algorithms Provided
 *
 *  - **Standard Bellman-Ford**: O(V*E) time complexity
 *  - **SPFA (Shortest Path Faster Algorithm)**: Queue-based optimization
 *    that is often faster in practice on sparse graphs
 *
 *  ## Key Features
 *
 *  - Handles negative edge weights
 *  - Detects negative-weight cycles
 *  - Can return the negative cycle as a path
 *  - Supports both painted (in-place) and separate tree construction
 *
 *  ## Complexity
 *
 *  | Operation | Time | Space |
 *  |-----------|------|-------|
 *  | Standard | O(V*E) | O(V) |
 *  | SPFA (avg) | O(E) | O(V) |
 *  | SPFA (worst) | O(V*E) | O(V) |
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  List_Digraph<Node, Arc> g;
 *  // ... build graph ...
 *
 *  Bellman_Ford<List_Digraph<Node, Arc>> bf(g);
 *  
 *  // Check for negative cycles and compute shortest paths
 *  if (bf.has_negative_cycle(start_node)) {
 *    Path<Graph> cycle = bf.build_negative_cycle();
 *    // handle negative cycle
 *  } else {
 *    // Use the shortest path tree
 *  }
 *  ```
 *
 *  @see Dijkstra.H For graphs without negative weights (more efficient)
 *  @see Floyd_Warshall.H For all-pairs shortest paths
 *  @see Johnson.H For all-pairs with negative weights (uses Bellman-Ford)
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef BELLMAN_FORD_H
# define BELLMAN_FORD_H

# include <tpl_dynListQueue.H>
# include <tpl_dynSetTree.H>
# include <tpl_graph_utils.H>
# include <Tarjan.H>
# include <ah-errors.H>
# include <ah_init_guard.H>
# include <cookie_guard.H>

# include <vector>

namespace Aleph
{
  /** Bellman-Ford algorithm for shortest paths with negative weights.

      This class implements the Bellman-Ford algorithm for finding shortest
      paths from a single source node. Unlike Dijkstra's algorithm, Bellman-Ford
      can handle graphs with negative edge weights and can detect negative cycles.

      Two versions are provided:
      - **Standard version**: O(V*E) time complexity
      - **Faster version**: Uses a queue-based optimization (SPFA variant)
        which is often faster in practice

      Template parameters:
      - `GT`: Graph type (typically `List_Digraph`).
      - `Distance`: Arc weight accessor. Must define `Distance_Type` and provide
        `Distance_Type operator()(typename GT::Arc *a)`.
      - `Ait`: Arc iterator template for traversing all arcs.
      - `NAit`: Node arc iterator template for traversing arcs from a node.
      - `SA`: Arc filter for the internal iterators.

      Features:
      - Detects negative cycles
      - Can return the negative cycle as a path
      - Computes shortest-path tree (painted or separate)
      - Used by Johnson's algorithm for node weight computation

      @warning This class is **not thread-safe**. Each instance maintains internal
      state that would be corrupted by concurrent access.

      @note For graphs without negative weights, Dijkstra's algorithm is more
      efficient (O((V+E) log V) vs O(V*E)).

      @ingroup Graphs
      @see Dijkstra_Min_Paths Johnson Floyd_All_Shortest_Paths
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            template <class, class> class Ait = Arc_Iterator,
            template <class, class> class NAit = Out_Iterator,
            class SA = Dft_Show_Arc<GT>>
  class Bellman_Ford
  {
    typedef typename Distance::Distance_Type Distance_Type;

    using Node = typename GT::Node;
    using Arc = typename GT::Arc;

    struct Sni
    {
      Distance_Type accum;
    };

    struct Ni : public Sni
    {
      int idx; // index in the predecessor arrays
    };

    static Distance_Type &accum(Node *p) noexcept
    {
      return static_cast<Sni *>(NODE_COOKIE(p))->accum;
    }

    static int &idx(Node *p) noexcept
    {
      return static_cast<Ni *>(NODE_COOKIE(p))->idx;
    }

    // Checked addition to prevent integer overflow
    Distance_Type checked_add(const Distance_Type & a, const Distance_Type & b) const
    {
      if constexpr (std::is_integral_v<Distance_Type>)
        {
          // Check for positive overflow
          ah_overflow_error_if(b > 0 && a > std::numeric_limits<Distance_Type>::max() - b)
            << "Integer overflow in distance addition: " << a << " + " << b;

          // Check for negative overflow (underflow)
          ah_overflow_error_if(b < 0 && a < std::numeric_limits<Distance_Type>::min() - b)
            << "Integer underflow in distance addition: " << a << " + " << b;
        }

      return a + b;
    }

    DynArray<typename GT::Arc *> arcs;
    const GT & g;
    const Distance_Type Inf;
    bool painted = false;
    Node *s = nullptr;
    SA sa;
    Distance dist;

    /// Initialize node cookies for simple mode (without predecessor tracking).
    void init_simple(Node *start)
    {
      Init_Guard guard([this]() { uninit<Sni>(); });

      typename GT::Node_Iterator it(g);
      for (int i = 0; it.has_curr(); ++i, it.next_ne())
        {
          auto p = it.get_curr();
          g.reset_bit(p, Aleph::Spanning_Tree); // set bit to zero
          auto ptr = new Sni;
          ptr->accum = Inf;
          NODE_BITS(p).set_bit(Spanning_Tree, false);
          NODE_COOKIE(p) = ptr;
        }
      s = start;
      accum(s) = 0;
      g.reset_arcs();

      guard.release(); // Successful initialization, prevent cleanup
    }

    /// Initialize node cookies with predecessor tracking for path reconstruction.
    void init_with_indexes(Node *start)
    {
      Init_Guard guard([this]()
                         {
                           uninit<Ni>();
                           arcs.cut();
                         });

      arcs.reserve(g.get_num_nodes());
      typename GT::Node_Iterator it(g);
      for (int i = 0; it.has_curr(); ++i, it.next())
        {
          arcs(i) = nullptr;
          auto p = it.get_curr();
          g.reset_bit(p, Aleph::Spanning_Tree); // set bit to zero
          auto ptr = new Ni;
          ptr->accum = Inf;
          ptr->idx = i;
          NODE_BITS(p).set_bit(Spanning_Tree, false);
          NODE_BITS(p).set_bit(Depth_First, false); // indicates if it is in queue
          NODE_COOKIE(p) = ptr;
        }

      s = start;
      accum(s) = 0;

      g.reset_arcs();

      guard.release(); // Successful initialization, prevent cleanup
    }

    /// Release the memory associated with the node cookies.
    template <class Info_Type>
    void uninit()
    {
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
        {
          auto p = it.get_curr();
          delete static_cast<Info_Type *>(NODE_COOKIE(p));
          NODE_COOKIE(p) = nullptr;
        }
    }

    /// Check that painted arcs form a valid spanning tree.
    /// For disconnected graphs, the number of painted arcs may be less than n-1.
    bool check_painted_arcs() noexcept
    {
      size_t num_arcs = 0;
      size_t num_painted_nodes = 0;
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
          ++num_arcs;

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        if (IS_NODE_VISITED(it.get_curr(), Aleph::Spanning_Tree))
          ++num_painted_nodes;

      // In a tree with k nodes, there are k-1 arcs (or k arcs in digraphs
      // where cycles may exist). For disconnected graphs, some nodes may
      // not be painted.
      return num_arcs <= num_painted_nodes;
    }

  public:
    /** Construct a Bellman-Ford executor.

        @param[in] __g The graph to operate on.
        @param[in] d Arc-weight accessor.
        @param[in] __sa Arc filter for internal iterators.
     */
    Bellman_Ford(const GT & __g, Distance d = Distance(), SA __sa = SA())
      : g(const_cast<GT &>(__g)), Inf(std::numeric_limits<Distance_Type>::max()),
        painted(false), sa(__sa), dist(d)
    {
      // empty
    }

    /// Check if a shortest-path tree has been computed or painted.
    /// @return true if a previous computation exists, false otherwise.
    [[nodiscard]] bool has_computation() const noexcept { return s != nullptr; }

    /// Check if a shortest-path tree has been painted.
    /// @return true if a tree has been painted, false otherwise.
    [[nodiscard]] bool is_painted() const noexcept { return painted; }

    /// Get the start node of the last computation.
    /// @return Pointer to the start node, or nullptr if no computation exists.
    Node * get_start_node() const noexcept { return s; }

    /// Get reference to the graph.
    /// @return Reference to the graph.
    const GT &get_graph() const noexcept { return g; }

  private:
    /// Relax all arcs n-1 times (standard Bellman-Ford).
    void relax_arcs() noexcept
    {
      const size_t & n = g.vsize();
      if (n <= 1)
        return; // Nothing to relax for empty or single-node graphs

      for (size_t i = 0; i < n - 1; ++i)
        for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_curr();
            auto src = g.get_src_node(arc);
            const auto & accum_src = accum(src);
            if (accum_src == Inf)
              continue;

            auto tgt = it.get_tgt_node_ne();
            auto w = dist(arc);
            auto sum = checked_add(accum_src, w);
            auto & accum_tgt = accum(tgt);
            if (sum < accum_tgt) // Relax Arc
              {
                const auto & index = idx(tgt);
                arcs(index) = arc;
                accum_tgt = sum;
              }
          }
    }

    /// Insert a node into the queue if not already present (SPFA optimization).
    static void
    put_in_queue(DynListQueue<typename GT::Node *> & q, typename GT::Node *p)
    {
      if (IS_NODE_VISITED(p, Depth_First)) // is already inside the queue?
        return;
      NODE_BITS(p).set_bit(Depth_First, true);
      q.put(p);
    }

    /// Remove a node from the queue and clear its in-queue flag.
    static typename GT::Node * get_from_queue(DynListQueue<typename GT::Node *> & q)
    {
      auto ret = q.get();
      assert(IS_NODE_VISITED(ret, Depth_First));
      NODE_BITS(ret).set_bit(Depth_First, false);
      return ret;
    }

    /// Relax outgoing arcs from a source node (SPFA variant).
    void relax_arcs(typename GT::Node *src, DynListQueue<typename GT::Node *> & q)
    {
      for (NAit<GT, SA> it(src, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto arc_src = g.get_src_node(arc);
          const auto & accum_src = accum(arc_src);
          if (accum_src == Inf)
            continue;

          auto tgt = g.get_tgt_node(arc);
          auto w = dist(arc);
          auto sum = checked_add(accum_src, w);
          auto & accum_tgt = accum(tgt);
          if (sum < accum_tgt) // Relax Arc
            {
              const auto & index = idx(tgt);
              arcs(index) = arc;
              accum_tgt = sum;
              put_in_queue(q, tgt);
            }
        }
    }

    /// Paint the spanning tree nodes and arcs with the Spanning_Tree bit.
    void paint_tree() noexcept
    { // paint the involved nodes and arcs
      const size_t n = g.vsize();
      for (size_t i = 0; i < n; ++i)
        {
          auto arc = arcs(i);
          if (arc == nullptr)
            continue;

          ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
          auto src = g.get_src_node(arc);
          auto tgt = g.get_tgt_node(arc);
          NODE_BITS(src).set_bit(Aleph::Spanning_Tree, true);
          NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
        }
      NODE_BITS(s).set_bit(Aleph::Spanning_Tree, true);

      assert(check_painted_arcs());

      painted = true;
    }

    /// Perform one more relaxation pass and check for negative cycle.
    /// Also updates predecessor array for cycle detection.
    bool last_relax_and_prepare_check_negative_cycle() noexcept
    {
      bool negative_cycle = false;
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto src = g.get_src_node(arc);
          auto & accum_src = accum(src);
          if (accum_src == Inf)
            continue;

          auto tgt = g.get_tgt_node(arc);
          auto d = dist(arc);
          auto & accum_tgt = accum(tgt);
          auto sum = checked_add(accum_src, d);
          if (sum < accum_tgt)
            {
              negative_cycle = true;
              const auto & index = idx(tgt);
              arcs(index) = arc;
              accum_tgt = sum;
            }
        }
      return negative_cycle;
    }

    /// Perform one more relaxation pass to detect (but not prepare) negative cycle.
    bool last_relax_and_test_negative_cycle() noexcept
    {
      for (Ait<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          auto arc = it.get_curr();
          auto src = g.get_src_node(arc);
          auto & accum_src = accum(src);
          if (accum_src == Inf)
            continue;

          auto tgt = g.get_tgt_node(arc);
          auto d = dist(arc);
          auto & accum_tgt = accum(tgt);
          auto sum = checked_add(accum_src, d);
          if (sum < accum_tgt)
            return true;
        }
      return false;
    }

    /// Free node cookies and set up predecessor pointers for path reconstruction.
    void link_cookies_and_free(typename GT::Node *start) noexcept
    {
      uninit<Ni>();

      // Construct the inverted paths to the start origin node
      const size_t n = g.vsize();
      for (size_t i = 0; i < n; ++i)
        {
          auto arc = arcs(i);
          if (arc == nullptr)
            continue;

          auto tgt = g.get_tgt_node(arc);
          NODE_COOKIE(tgt) = g.get_src_node(arc);
        }

      NODE_COOKIE(start) = nullptr; // just in case there is a negative cycle
    }

  public:
    /** Paint the shortest paths tree from a `start` node.

        @param[in] start source node from which the shortest paths will be
        computed.
        @return true if negative cycles are detected, in which case the
        shortest paths tree has no sense. Otherwise, `false` is
        returned and the shortest paths tree is painted with the bit
        `Spanning_Tree`.
    */
    bool paint_spanning_tree(Node *start)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);

      relax_arcs();
      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
      paint_tree();
      link_cookies_and_free(s);

      return negative_cycle;
    }

    /** Faster shortest paths tree painting from a `start` node.

        This method executes a faster version of Bellman-Ford algorithm
        (SPFA variant) which is often more efficient in practice.

        @param[in] start source node from which the shortest paths will be
        computed.
        @return true if negative cycles are detected, in which case the
        shortest paths tree has no sense. Otherwise, `false` is
        returned and the shortest paths tree is painted with the bit
        `Spanning_Tree`.
    */
    bool faster_paint_spanning_tree(Node *start)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);

      const auto & n = g.get_num_nodes();

      DynListQueue<typename GT::Node *> q;

      Node __sentinel;
      Node *sentinel = &__sentinel;

      put_in_queue(q, s);
      put_in_queue(q, sentinel);

      for (size_t i = 0; not q.is_empty();)
        {
          auto src = get_from_queue(q);
          if (src == sentinel) // Is the sentinel removed?
            {
              if (i++ > n)
                break;

              put_in_queue(q, sentinel);
            }
          else
            relax_arcs(src, q);
        }

      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
      paint_tree();
      link_cookies_and_free(s);

      return negative_cycle;
    }

  private:
    /// Create a dummy node connected to all nodes with zero-weight edges.
    /// Used for detecting negative cycles in the entire graph.
    Node * create_dummy_node()
    {
      // RAII guard to ensure cleanup on exception
      struct Dummy_Guard
      {
        GT & graph;
        Node *dummy;
        std::vector<Arc *> inserted_arcs;
        bool released = false;

        explicit Dummy_Guard(GT & g, Node *d) : graph(g), dummy(d)
        {
          inserted_arcs.reserve(g.get_num_nodes());
        }

        ~Dummy_Guard()
        {
          if (not released)
            {
              // Remove all inserted arcs
              for (auto arc: inserted_arcs)
                graph.remove_arc(arc);

              // Remove dummy node
              if (dummy != nullptr)
                graph.remove_node(dummy);
            }
        }

        void add_arc(Arc *a) { inserted_arcs.push_back(a); }
        void release() { released = true; }

        Dummy_Guard(const Dummy_Guard &) = delete;

        Dummy_Guard & operator=(const Dummy_Guard &) = delete;
      };

      s = const_cast<GT &>(g).insert_node(typename GT::Node_Type());
      Dummy_Guard guard(const_cast<GT &>(g), s);

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          if (p == s)
            continue;
          auto a = const_cast<GT &>(g).insert_arc(s, p);
          guard.add_arc(a);
          Distance::set_zero(a);
        }

      guard.release(); // Successful creation, prevent cleanup
      return s;
    }

    /// Remove a dummy node and clean up its cookie.
    template <class Info_Type>
    void remove_dummy_node(Node *p)
    {
      delete static_cast<Info_Type *>(NODE_COOKIE(p));
      const_cast<GT &>(g).remove_node(p);
    }

  public:
    /** Test if a negative cycle exists starting from a specific node.

        @param[in] start Source node from which to check for negative cycles.
        @return true if a negative cycle is reachable from start, false otherwise.
        @throw std::domain_error if start is nullptr.
     */
    bool has_negative_cycle(Node *start)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);
      // Note: s and accum(s) already set by init_with_indexes

      relax_arcs();
      const bool negative_cycle = last_relax_and_test_negative_cycle();
      uninit<Ni>();

      return negative_cycle;
    }

    /** Test if a negative cycle exists anywhere in the graph.

        Creates a temporary dummy node connected to all other nodes with
        zero-weight edges, then runs negative cycle detection from it.

        @return true if any negative cycle exists in the graph, false otherwise.
     */
    bool has_negative_cycle()
    {
      create_dummy_node();
      auto ret = has_negative_cycle(s);
      remove_dummy_node<Ni>(s);

      return ret;
    }

  private:
    /// Build spanning tree from arcs and search for cycle using Tarjan's algorithm.
    Path<GT> search_negative_cycle_on_partial_graph()
    {
      GT aux = build_spanning_tree<GT>(arcs);

      // we map because Tarjan algorithm modifies cookies
      DynMapTree<Node *, Node *> table;
      for (typename GT::Node_Iterator it(aux); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          table.insert(p, static_cast<Node *>(NODE_COOKIE(p)));
        }

      // Save and restore cookies around Tarjan call (Tarjan modifies cookies)
      Cookie_Saver<GT> cookie_saver(aux, true, false); // only save node cookies

      // Clear cookies for Tarjan's use
      for (typename GT::Node_Iterator it(aux); it.has_curr(); it.next_ne())
        NODE_COOKIE(it.get_curr()) = nullptr;

      if (Path<GT> path(aux); Tarjan_Connected_Components<GT, NAit, SA>(sa).compute_cycle(aux, path))
        {
          Path<GT> ret(g);
          for (typename Path<GT>::Iterator it(path); it.has_current_node();
               it.next_ne())
            ret.append_directed(static_cast<Node *>(table.find(it.get_current_node_ne())));
          return ret;
        }

      return Path<GT>(g);
    }

  public:
    /** Search a negative cycle on all possible paths starting from
        `start` node.

        If a negative cycle is found, then the Tarjan algorithm is executed
        for retrieving it. In this case the cycle is returned. Otherwise
        (there is no negative cycle), the returned path is empty.

        @param[in] start starting node
        @return a valid path corresponding to a negative cycle if this is
        found. Otherwise, an empty path
    */
    Path<GT> test_negative_cycle(Node *start)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);

      relax_arcs();
      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
      if (not negative_cycle)
        {
          link_cookies_and_free(s);
          return Path<GT>(g);
        }

      Path<GT> ret = search_negative_cycle_on_partial_graph();
      if (ret.is_empty())
        WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
              "a negative cycle, but Tarjan algorithm executed on partial\n"
              "graph has not found such cycle\n\n"
              "Be very careful, this is provably a bug");

      link_cookies_and_free(s);
      return ret;
    }

    /** Searches and returns a negative cycle (if it exists).

        @return a path containing a negative cycle (if it
        exists). Otherwise, it returns an empty path.
     */
    Path<GT> test_negative_cycle()
    {
      auto start = create_dummy_node();
      auto ret_val = test_negative_cycle(start);
      remove_dummy_node<Ni>(start);
      return ret_val;
    }

    /** Searches a negative cycle using the faster version of Bellman-Ford
        algorithm and iteratively searching the cycle before finishing up.

        Normally, the Bellman-Ford algorith certainly detects a negative
        cycle if during an additional arcs scanning an arc is
        relaxed. However, very frequently the cycle appears in graph used
        for representing the partial spanning tree.

        This version could be seen as thus:

            threshold = it_factor*|V|;
            for (int i = 0; i < |V|; ++i)
              {
                for (e in Arcs) // for each arc e
                  relax e;
                if (i >= threshold)
                  {
                    // search a cycle in the graph representing the spanning tree
                    threshold += step;
                    if negative cycle is found
                      return it;
                  }
              }

        So, from the threshold-th iteration, the algorithm tries to find a
        negative cycle on the hope that if this exists, then the algorithm
        will finish much before the normal completion.

        @warning The cycle searching is done with respect to a source
        node. So, if no negative cycle is found, then this is still not
        conclusive for determining that the graph has no negative cycles.

        @param[in] start node from which the spanning tree is built.
        @param[in] it_factor iterative factor since then the negative
        cycle is searched.
        @param[in] step next step from which the next negative cycle will
        be done.
        @return a tuple whose `get<0>` value corresponds to the found
        negative cycle (if this one exists) and `get<1>` value is the
        external iteration when the cycle was found. The idea of this
        second field is to give feedback in order to eventually refine the
        `it_factor` value.
     */
    std::tuple<Path<GT>, size_t>
    search_negative_cycle(Node *start, double it_factor, const size_t step)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);

      const auto & n = g.get_num_nodes();
      DynListQueue<typename GT::Node *> q;
      Node __sentinel;
      Node *sentinel = &__sentinel;
      put_in_queue(q, s);
      put_in_queue(q, sentinel);

      double threshold = it_factor * n;
      Path<GT> ret(g);

      size_t i = 0;
      while (not q.is_empty())
        {
          auto src = get_from_queue(q);
          if (src == sentinel)
            {
              if (i++ > n)
                break;

              put_in_queue(q, sentinel);
              if (i >= threshold) // must I search negative cycles?
                {
                  ret = search_negative_cycle_on_partial_graph();
                  if (not ret.is_empty()) // negative cycle found?
                    {
                      link_cookies_and_free(s);
                      return std::make_tuple(std::forward<Path<GT>>(ret), i);
                    }
                  threshold += step;
                }
            }
          else
            relax_arcs(src, q);
        }

      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
      if (negative_cycle)
        {
          ret = search_negative_cycle_on_partial_graph();
          if (ret.is_empty())
            WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
                  "a negative cycle, but Tarjan algorithm executed on partial\n"
                  "graph has not found such cycle\n\n"
                  "Be very careful, this provably is a bug");
        }

      link_cookies_and_free(s);
      return std::make_tuple(std::forward<Path<GT>>(ret), i);
    }

    /** Searches a negative cycle using the faster version of Bellman-Ford
        algorithm (SPFA variant).

        @param[in] start source node from which the shortest paths tree is built.
        @return a path containing the negative cycle if found, or an empty path
        if no negative cycle exists reachable from the start node.
     */
    Path<GT> search_negative_cycle(Node *start)
    {
      ah_domain_error_if(start == nullptr)
        << "start node cannot be null";

      init_with_indexes(start);

      const auto & n = g.get_num_nodes();
      DynListQueue<typename GT::Node *> q;
      Node __sentinel;
      Node *sentinel = &__sentinel;
      put_in_queue(q, s);
      put_in_queue(q, sentinel);

      Path<GT> ret(g);
      for (size_t i = 0; not q.is_empty(); /* nothing */)
        {
          auto src = get_from_queue(q);
          if (src == sentinel)
            {
              if (i++ > n)
                break;

              put_in_queue(q, sentinel);
            }
          else
            relax_arcs(src, q);
        }

      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
      if (negative_cycle)
        {
          ret = search_negative_cycle_on_partial_graph();
          if (ret.is_empty())
            WARNING("Serious inconsistency. Bellman-Ford algorithm has detected\n"
                  "a negative cycle, but Tarjan algorithm executed on partial\n"
                  "graph has not found such cycle\n\n"
                  "Be very careful, this provably is a bug");
        }

      link_cookies_and_free(s);

      return ret;
    }

    /** Searches a negative cycle using the faster version of Bellman-Ford
        algorithm and iteratively searching the cycle before finishing up.

        \overload search_negative_cycle(Node * start, double it_factor, const size_t step)

        This overloaded version searches for negatives cycles in all the
        graph. For that, a dummy and temporal node is inserted into the
        graph and the search is performed using the dummy node as starting
        node.
     */
    std::tuple<Path<GT>, size_t> search_negative_cycle(double it_factor,
                                                       const size_t step)
    {
      auto start = create_dummy_node();
      auto ret_val = search_negative_cycle(start, it_factor, step);
      remove_dummy_node<Ni>(start);
      return ret_val;
    }

    /**
       \overload search_negative_cycle(Node * start)
    */
    Path<GT> search_negative_cycle()
    {
      auto start = create_dummy_node();
      auto ret_val = search_negative_cycle(start);
      remove_dummy_node<Ni>(start);
      return ret_val;
    }


    /** Extract the shortest paths tree in a compressed form.

        On a previously painted shortest paths tree starting from a source
        node, this method extracts the tree and loads it in an array of arcs.

        @return an array of arcs belonging to the shortest paths tree.
        @throw domain_error if the shortest paths tree has not been
        previously built.
    */
    DynArray<Arc *> extract_min_spanning_tree()
    {
      ah_domain_error_if(not painted)
        << "Spanning tree has not been painted";

      return arcs;
    }

    /** Get the accumulated distance to a node from a previously painted tree.

        This method is useful when you only need the distance without
        reconstructing the full path.

        @param[in] node The destination node.
        @return The accumulated distance from the start node to node.
        @throw std::domain_error if the graph has not been previously painted
               or if the node is unreachable.
     */
    Distance_Type get_distance(typename GT::Node *node)
    {
      ah_domain_error_if(not painted)
        << "Graph has not been previously painted";

      ah_domain_error_if(node == nullptr)
        << "node cannot be null";

      // Follow predecessor chain to verify reachability
      auto curr = node;
      while (curr != s)
        {
          auto parent = static_cast<Node *>(NODE_COOKIE(curr));
          ah_domain_error_if(parent == nullptr)
            << "Node is not reachable from start node";
          curr = parent;
        }

      // Compute distance by following the path
      Distance_Type total = 0;
      curr = node;
      while (curr != s)
        {
          auto parent = static_cast<Node *>(NODE_COOKIE(curr));

          // Find the arc from parent to curr
          for (NAit<GT, SA> it(parent, sa); it.has_curr(); it.next_ne())
            if (auto arc = it.get_curr(); g.get_tgt_node(arc) == curr &&
                                          IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
              {
                total = checked_add(total, dist(arc));
                break;
              }
          curr = parent;
        }
      return total;
    }

    /** Extract from the graph a previously painted shortest paths tree.

        This method requires that the shortest paths tree has been
        computed with `paint_spanning_tree()` or `faster_paint_spanning_tree()`.

        @param[in] tree a graph where the shortest paths tree will be stored.
        @param[in] with_map if true the extracted tree is mapped
        through the cookies with the graph.
     */
    void build_tree(GT & tree, bool with_map = true)
    {
      ah_domain_error_if(not painted and with_map)
        << "Spanning tree has not been painted";

      clear_graph(tree);

      DynMapTree<Node *, Node *> table;
      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto gp = it.get_curr();
          auto tp = tree.insert_node(gp->get_info());
          table.insert(gp, tp);
        }

      for (typename GT::Node_Iterator it(g); it.has_curr(); it.next_ne())
        {
          auto gtgt = it.get_curr();
          auto gsrc = static_cast<Node *>(NODE_COOKIE(gtgt));
          if (gsrc == nullptr)
            continue; // This is the source node of the spanning tree

          // Find the spanning tree arc from gsrc to gtgt
          Arc *garc = nullptr;
          for (Out_Iterator<GT, SA> ait(gsrc, sa); ait.has_curr(); ait.next_ne())
            {
              auto a = ait.get_curr();
              if (g.get_tgt_node(a) == gtgt and IS_ARC_VISITED(a, Aleph::Spanning_Tree))
                {
                  garc = a;
                  break;
                }
            }
          ah_logic_error_unless(garc != nullptr)
            << "Arc not found between nodes in spanning tree";

          auto tsrc_ptr = table.search(gsrc);
          auto ttgt_ptr = table.search(gtgt);
          ah_logic_error_unless(tsrc_ptr) << "Source node not found in mapping table";
          ah_logic_error_unless(ttgt_ptr) << "Target node not found in mapping table";
          auto tarc = tree.insert_arc(tsrc_ptr->second, ttgt_ptr->second,
                                      garc->get_info());
          if (with_map)
            GT::map_arcs(garc, tarc);
        }

      if (with_map)
        table.for_each([](const auto & p) { GT::map_nodes(p.first, p.second); });
    }

    /** Test for negative cycle and return it if found.

        @param[in] s The starting node.
        @param[out] cycle Path where the cycle is stored if found.
        @return true if a negative cycle exists, false otherwise.
     */
    bool test_negative_cycle(typename GT::Node *s, Path<GT> & cycle)
    {
      cycle = test_negative_cycle(s);
      return not cycle.is_empty();
    }

    /** Test for negative cycle anywhere in the graph.

        @param[out] cycle Path where the cycle is stored if found.
        @return true if a negative cycle exists, false otherwise.
     */
    bool test_negative_cycle(Path<GT> & cycle)
    {
      cycle = test_negative_cycle();
      return not cycle.is_empty();
    }

    /** Extract the shortest path to a node from a previously painted tree.

        @param[in] end The destination node.
        @param[out] path Path where the shortest path will be stored.
        @return The total cost of the path.
        @throw std::domain_error if the graph has not been previously painted.
     */
    Distance_Type get_min_path(typename GT::Node *end, Path<GT> & path)
    {
      ah_domain_error_if(not painted) << "Graph has not been previously painted";

      return Aleph::get_min_path<GT, Distance>(s, end, path);
    }

    /** Returns a mapping with the shortest distances to each node obtained
        after executing the Bellman-Ford algorithm from a dummy node connected
        to all nodes with zero-weight edges. This routine is specifically
        designed for Johnson's algorithm for computing all-pairs shortest paths.

        @return A tree mapping where each node is mapped to its distance from
        the dummy source (used as potential for edge reweighting).
        @throw std::domain_error if negative cycles are detected.
     */
    DynMapTree<Node *, Distance_Type> compute_nodes_weights()
    {
      Node *dummy = create_dummy_node();

      Init_Guard guard([this, dummy]()
                         {
                           uninit<Ni>();
                           arcs.cut();
                           if (dummy != nullptr)
                             remove_dummy_node<Ni>(dummy);
                         });

      init_with_indexes(s);

      const auto & n = g.get_num_nodes();

      DynListQueue<typename GT::Node *> q;

      Node __sentinel;
      Node *sentinel = &__sentinel;

      put_in_queue(q, s);
      put_in_queue(q, sentinel);

      for (size_t i = 0; not q.is_empty(); /* nothing */)
        {
          auto src = get_from_queue(q);
          if (src == sentinel) // Is the sentinel removed?
            {
              if (i++ > n)
                break;
              put_in_queue(q, sentinel);
            }
          else
            relax_arcs(src, q);
        }

      const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();

      DynMapTree<Node *, Distance_Type> ret;

      // build mapping if there are no negative cycles
      if (not negative_cycle)
        for (auto it = g.get_node_it(); it.has_curr(); it.next_ne())
          {
            auto p = it.get_curr();
            if (p == dummy) // Skip the dummy node
              continue;
            ret.insert(p, accum(p));
          }

      // Manual cleanup before throwing (guard will handle cleanup on exception)
      uninit<Ni>();
      arcs.cut();
      remove_dummy_node<Ni>(dummy);
      guard.release(); // Prevent double cleanup

      ah_domain_error_if(negative_cycle) << "negative cycles detected";

      return ret;
    }
  };

  /** Detects if a negative cycle exists and eventually computes it.

      This functor takes a digraph and optionally a source node, executes the
      Bellman-Ford algorithm and if a negative cycle is detected
      then it stores it in a path.

      The procedure is parameterized with the following specifications:
      - `GT`: the graph type based on List_Graph or List_Digraph.
      - `Distance`: The arc weight reading class that must export the
        following attributes:
        - `Distance::Distance_Type`: the data type that represents a
          weight on an arc.
        - `Distance::Distance_Type operator()(typename GT::Arc *a)`:
          returns the weight value on arc `a`.
      - `SA`: arc filter.

      @ingroup Graphs
   */
  template <class GT,
            class Distance = Dft_Dist<GT>,
            template <class, class> class Ait = Arc_Iterator,
            template <class, class> class NAit = Node_Arc_Iterator,
            class SA = Dft_Show_Arc<GT>>
  struct Bellman_Ford_Negative_Cycle
  {
    /** Invokes the detection and calculation of a negative cycle.

        @param[in] g the graph.
        @param[out] path the path where the cycle is stored in case
        a negative cycle is detected.
        @param[in,out] d distance map (modified during computation).
        @param[in] sa arc filter for traversal.
        @return true if a negative cycle exists; false otherwise.
        @throw bad_alloc if there is not enough memory.
     */
    bool operator ()(const GT & g, Path<GT> & path,
                     Distance & d, SA & sa) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          test_negative_cycle(path);
    }

    bool operator ()(const GT & g, Path<GT> & path,
                     Distance && d = Distance(), SA && sa = SA()) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          test_negative_cycle(path);
    }

    /** Invokes the detection and calculation of a negative cycle.

        @param[in] g the graph.
        @param[in] s the starting node.
        @param[out] path the path where the cycle is stored in case
        a negative cycle is detected.
        @param[in,out] d distance map (modified during computation).
        @param[in] sa arc filter for traversal.
        @return true if a negative cycle exists; false otherwise.
        @throw bad_alloc if there is not enough memory.
     */
    bool operator ()(const GT & g, typename GT::Node *s,
                     Path<GT> & path, Distance & d, SA & sa) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          test_negative_cycle(s, path);
    }

    bool operator ()(const GT & g, typename GT::Node *s,
                     Path<GT> & path, Distance && d = Distance(),
                     SA && sa = SA()) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          test_negative_cycle(s, path);
    }

    /** Search for a negative cycle starting from a specific node.

        @param[in] g The graph.
        @param[in] s The starting node.
        @param[in] d Distance accessor.
        @param[in] sa Arc filter.
        @return Path containing the negative cycle if found, empty path otherwise.
     */
    Path<GT> operator ()(const GT & g, typename GT::Node *s,
                         Distance & d, SA & sa) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          search_negative_cycle(s);
    }

    /// \overload
    Path<GT> operator ()(const GT & g, typename GT::Node *s,
                         Distance && d = Distance(), SA && sa = SA()) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          search_negative_cycle(s);
    }

    /** Search for a negative cycle in the entire graph.

        @param[in] g The graph.
        @param[in] d Distance accessor.
        @param[in] sa Arc filter.
        @return Path containing the negative cycle if found, empty path otherwise.
     */
    Path<GT> operator ()(const GT & g, Distance & d, SA & sa) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          search_negative_cycle();
    }

    /// \overload
    Path<GT>
    operator ()(const GT & g, Distance && d = Distance(), SA && sa = SA()) const
    {
      return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).
          search_negative_cycle();
    }
  };
} // end namespace Aleph

# endif // BELLMAN_FORD_H
