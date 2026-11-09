
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

/** @file Bellman_Ford.H
 *  @brief Bellman-Ford algorithm for single-source shortest paths.
 *
 *  This file implements the Bellman-Ford algorithm, which computes the shortest
 *  paths from a single source vertex to all other vertices in a weighted
 *  directed graph. Unlike Dijkstra's algorithm, Bellman-Ford can handle
 *  graphs with negative edge weights and can detect negative-weight cycles.
 *
 *  ## Complexity
 *
 *  | Algorithm | Time (avg) | Time (worst) | Space |
 *  |-----------|------------|--------------|-------|
 *  | Standard | O(V*E) | O(V*E) | O(V) |
 *  | SPFA | O(E) | O(V*E) | O(V) |
 *
 *  @par Example
 *  @code
 *  List_Digraph<Node, Arc> g;
 *  // ... build graph ...
 *  List_Digraph<Node, Arc>::Node * start = g.get_first_node();
 *
 *  Bellman_Ford bf(g);
 *
 *  // Check for negative cycles and compute shortest paths from 'start'
 *  if (bf.has_negative_cycle(start)) {
 *    Path<Graph> cycle = bf.build_negative_cycle();
 *    // handle negative cycle
 *  } else {
 *    // bf.is_painted() is true, Spanning_Tree bits are set
 *  }
 *  @endcode
 *
 *  @see Dijkstra.H For graphs without negative weights (more efficient)
 *  @see Floyd_Warshall.H For all-pairs shortest paths
 *  @see Johnson.H For all-pairs with negative weights (uses Bellman-Ford)
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef BELLMAN_FORD_H
#define BELLMAN_FORD_H

# include <ah-graph-concepts.H>

#include <cmath>
#include <concepts>
#include <type_traits>
#include <limits>
#include <vector>

#include <tpl_array.H>
#include <tpl_dynListQueue.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph_utils.H>
#include <tpl_dfs_stack.H>
#include <Tarjan.H>
#include <ah-errors.H>
#include <ah_init_guard.H>
#include <cookie_guard.H>

namespace Aleph {
/** Bellman-Ford algorithm for shortest paths with negative weights.

    This class implements the Bellman-Ford algorithm for finding shortest
    paths from a single source node. Unlike Dijkstra's algorithm, Bellman-Ford
    can handle graphs with negative edge weights and can detect negative cycles.

    Two versions are provided:
    - **Standard version**: O(V*E) time complexity
    - **Faster version**: Uses a queue-based optimization (SPFA variant)
      which is often faster in practice.

    Template parameters:
    - `GT`: Graph type (typically `List_Digraph`).
    - `Distance`: Arc weight accessor. Must define `Distance_Type` and provide
      `Distance_Type operator()(typename GT::Arc *a)`. It should have no side
      effects: how many times it is called per arc is unspecified (the
      relaxation passes stop as soon as one relaxes nothing). It may give
      `Distance_Type tolerance() const`: a relaxation must then improve a
      distance by more than it, so that the rounding of floating sums does
      not make a cycle of weight zero negative, as an arc and its residual,
      of opposite weights, in a residual network.
    - `Ait`: Arc iterator template for traversing all arcs.
    - `NAit`: Node arc iterator template for traversing arcs from a node.
    - `SA`: Arc filter for the internal iterators.

    Features:
    - Detects negative cycles
    - Can return the negative cycle as a path
    - Computes a shortest-path tree (painted or separate)
    - Used by Johnson's algorithm for node weight computation

    ## State Management

    This class maintains internal state between operations:
    - `painted`: Whether a spanning tree is currently painted on the graph
    - `s`: The start node of the last computation
    - `arcs`: Predecessor array for path reconstruction

    **Important usage notes:**

    1. **Graph modification**: The graph is temporarily modified during each
       operation (node/arc bits and cookies). These modifications are cleaned
       up when the operation completes. Do not modify the graph structure
       during an operation.

    2. **Sequential calls**: Calling paint_spanning_tree() twice without
       calling clear() will overwrite the previous painted state. If you need
       to preserve the previous state, call clear() first or use a new
       Bellman_Ford instance.

    3. **Reusability**: The same Bellman_Ford object can be reused for
       multiple computations on the same graph. Each operation automatically
       resets the internal state.

    4. **Exception safety**: All operations are exception-safe. If an
       exception is thrown, the graph is restored to a consistent state
       (RAII guards ensure cleanup).

    @warning This class is **not thread-safe**. Each instance maintains internal
    state that would be corrupted by concurrent access.

    @warning The constructor takes a non-const graph reference because the
    algorithm temporarily modifies node/arc bits and cookies. The methods that
    detect global negative cycles (no start node) take every node as a source,
    at distance 0, as if a virtual source reached each one by an arc of
    weight 0: the graph gets no node.

    A negative cycle is a cycle of the graph of the predecessors, the arcs of
    the last relaxations, whose cycles are all negative. It is given by its
    arcs: with parallel arcs, the one that was relaxed, and only arcs that the
    filter shows. A floating distance that the last pass finds -inf or NaN,
    from a weight or from an overflow, throws domain_error.

    With an integer distance, a sum beyond max() is no path, and one below
    lowest() throws overflow_error at once. If a node that the start
    reaches is left without a distance, as its distances do not fit, the
    paintings and the searches from a start throw overflow_error, unless
    they find a negative cycle: the one that such a node could reach would
    be missed. Without a start every node begins at 0, and no sum goes
    beyond max().

    @note For graphs without negative weights, Dijkstra's algorithm is more
    efficient (O((V+E) log V) vs O(V*E)).

    @ingroup Graphs
    @see Dijkstra_Min_Paths Johnson Floyd_All_Shortest_Paths
 */
template <AlephGraph GT,
          ArcDistance<GT> Distance = Dft_Dist<GT>,
          template <class, class> class Ait = Arc_Iterator,
          template <class, class> class NAit = Out_Iterator,
          ArcFilter<GT> SA = Dft_Show_Arc<GT>>
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
    int idx;      // index in the predecessor arrays
    size_t mark;  // the walk that reached it, when a cycle is searched
  };

  static Distance_Type &accum(Node *p) noexcept
  {
    return static_cast<Sni *>(NODE_COOKIE(p))->accum;
  }

  static int &idx(Node *p) noexcept
  {
    return static_cast<Ni *>(NODE_COOKIE(p))->idx;
  }

  static size_t &mark(Node *p) noexcept
  {
    return static_cast<Ni *>(NODE_COOKIE(p))->mark;
  }

  // Checked addition to prevent integer overflow
  Distance_Type checked_add(const Distance_Type &a, const Distance_Type &b) const
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

  // Sets out to a + b and returns true; for an integral type whose sum goes
  // beyond max(), no distance that fits goes through it: it returns false,
  // and the painting remembers it in beyond_max_
  [[gnu::always_inline]] bool add_if_fits(const Distance_Type &a,
                                          const Distance_Type &b,
                                          Distance_Type &out)
  {
    if constexpr (std::is_integral_v<Distance_Type>)
      {
        if (b > 0 and a > std::numeric_limits<Distance_Type>::max() - b) [[unlikely]]
          {
            beyond_max_ = true;
            return false;
          }
        ah_overflow_error_if(b < 0 and a < std::numeric_limits<Distance_Type>::min() - b)
          << "Integer underflow in distance addition: " << a << " + " << b;
      }
    out = a + b; // the checks of checked_add(), each once
    return true;
  }

  // After a painting from start in which a sum went beyond max(): a node
  // that start reaches through the arcs that the filter shows, and that has
  // no distance, has one that does not fit
  void check_beyond_max(Node *start)
  {
    DynSetTree<Node *> seen;
    DynListQueue<Node *> queue;
    seen.insert(start);
    queue.put(start);
    while (not queue.is_empty())
      {
        Node *p = queue.get();
        ah_overflow_error_if(accum(p) == Inf)
          << "Bellman_Ford: the distance from the start to a node does not fit "
          << "in its type";
        // each arc from its source, as the relaxations read it
        for (NAit<GT, SA> it(p, sa_); it.has_curr(); it.next_ne())
          if (auto arc = it.get_curr(); g_.get_src_node(arc) == p)
            if (Node *t = g_.get_tgt_node(arc); seen.insert(t) != nullptr)
              queue.put(t);
      }
  }

  // The arc that reached each node, by its index: contiguous, as every
  // relaxation reads or writes it
  Array<typename GT::Arc *> arcs_;
  GT &g_;  // Non-const: algorithm modifies node/arc bits and cookies
  const Distance_Type Inf;
  bool painted_ = false;
  bool parents_ = false;  // the node cookies hold the parents in the tree
  bool beyond_max_ = false;  // a sum of the painting went beyond max()
  Node *s_ = nullptr;
  Node *relaxed_ = nullptr;  // the last node that the last pass relaxed
  SA sa_;
  Distance dist_;

  [[noreturn, gnu::noinline, gnu::cold]] static void throw_not_finite()
  {
    ah_domain_error() << "Bellman_Ford: a distance is -inf or NaN, "
                      << "from a weight or from an overflow";
  }

  /// Whether sum relaxes the distance accum: it is below it, by more than
  /// the tolerance of the distance, if it gives one (see the class)
  [[gnu::always_inline]] bool relaxes(const Distance_Type &sum, const Distance_Type &accum) const
  {
    if constexpr (requires (const Distance &d)
                    { { d.tolerance() } -> std::convertible_to<Distance_Type>; })
      return sum < accum - dist_.tolerance();
    else
      return sum < accum;
  }

  /// Throw domain_error if a floating sum of the last pass is -inf or NaN,
  /// from a weight or from an overflow: -inf plus a weight is not below
  /// -inf, so that a negative cycle that it reaches could be missed. One
  /// comparison for each arc of the last pass, inlined.
  [[gnu::always_inline]] static void check_finite(const Distance_Type &sum)
  {
    if constexpr (std::is_floating_point_v<Distance_Type>)
      if (not (sum > -std::numeric_limits<Distance_Type>::infinity()))
        throw_not_finite();
  }

  /// Initialize node cookies with predecessor tracking for path
  /// reconstruction. Without a start, every node starts at 0, as if a
  /// virtual source reached each one by an arc of weight 0.
  void init_with_indexes(Node *start)
  {
    Init_Guard guard([this]()
    {
      uninit<Ni>();
      arcs_.empty();
    });

    const size_t n = g_.get_num_nodes();
    arcs_.empty();  // Clear any previous data
    arcs_.reserve(n);

    typename GT::Node_Iterator it(g_);
    for (size_t i = 0; it.has_curr(); ++i, it.next())
      {
        arcs_.append(nullptr);  // the entry of index i
        auto p = it.get_curr();
        g_.reset_bit(p, Aleph::Spanning_Tree);  // set bit to zero
        NODE_COOKIE(p) = nullptr;  // clear stale pointer before allocating
        auto ptr = new Ni;
        ptr->accum = start == nullptr ? Distance_Type(0) : Inf;
        ptr->idx = static_cast<int>(i);
        ptr->mark = 0;
        NODE_BITS(p).set_bit(Spanning_Tree, false);
        NODE_BITS(p).set_bit(Depth_First, false);  // indicates if it is in queue
        NODE_COOKIE(p) = ptr;
      }

    painted_ = false;
    parents_ = false;
    beyond_max_ = false;
    relaxed_ = nullptr;
    s_ = start;
    if (s_ != nullptr)
      accum(s_) = 0;

    g_.reset_arcs();

    guard.release();  // Successful initialization, prevent cleanup
  }

  /// Leave the object without a computation, and the graph without the data
  /// of the nodes, after an operation that throws.
  void abandon() noexcept
  {
    uninit<Ni>();
    arcs_.empty();
    painted_ = false;
    parents_ = false;
    s_ = nullptr;
  }

  /// Release the memory associated with the node cookies, and the bits
  /// Depth_First that SPFA leaves on the nodes still in its queue.
  template <class Info_Type>
  void uninit() noexcept
  {
    for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next())
      {
        auto p = it.get_curr();
        delete static_cast<Info_Type *>(NODE_COOKIE(p));
        NODE_COOKIE(p) = nullptr;
        NODE_BITS(p).set_bit(Depth_First, false);
      }
  }

  /** Check that painted arcs form a valid spanning tree structure.

      Verifies the following invariants:
      1. Number of painted arcs == number of painted nodes - 1 (tree property)
      2. Each painted node (except root) has exactly one incoming painted arc
      3. Root node (s) has no incoming painted arcs

      For disconnected graphs, only the component reachable from s is verified.

      @return true if the painted structure is valid, false otherwise.
      @throw anything that the arc filter throws.
   */
  bool check_painted_arcs() noexcept(nothrow_arc_filter_v<GT, SA>)
  {
    if (s_ == nullptr)
      return false;

    size_t num_painted_arcs = 0;
    size_t num_painted_nodes = 0;

    // Count painted arcs
    for (Ait<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
      if (IS_ARC_VISITED(it.get_curr(), Aleph::Spanning_Tree))
        ++num_painted_arcs;

    // Count painted nodes and verify each has exactly one incoming painted arc
    for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
      {
        auto node = it.get_curr();
        if (not IS_NODE_VISITED(node, Aleph::Spanning_Tree))
          continue;

        ++num_painted_nodes;

        // Skip root node - it should have no incoming painted arcs
        if (node == s_)
          continue;

        // Count incoming painted arcs to this node
        size_t incoming_count = 0;
        for (Ait<GT, SA> ait(g_, sa_); ait.has_curr(); ait.next_ne())
          if (auto arc = ait.get_curr();
              IS_ARC_VISITED(arc, Aleph::Spanning_Tree) and g_.get_tgt_node(arc) == node)
            ++incoming_count;

        // Each non-root painted node must have exactly 1 incoming painted arc
        if (incoming_count != 1)
          return false;
      }

    // Tree property: #arcs == #nodes - 1
    return num_painted_nodes != 0 and num_painted_arcs == num_painted_nodes - 1;
  }

public:
  /** Construct a Bellman-Ford executor.

      @param[in] __g The graph to operate on. Note: the graph will be
                     temporarily modified (node/arc bits and cookies) during
                     algorithm execution. The modifications are cleaned up
                     after each operation completes.
      @param[in] d Arc-weight accessor.
      @param[in] __sa Arc filter for internal iterators.

      @warning The graph reference is stored internally. Do not modify
               the graph structure while a Bellman_Ford operation is in
               progress.
   */
  Bellman_Ford(GT &__g, Distance d = Distance(), SA __sa = SA())
    : g_(__g), Inf(std::numeric_limits<Distance_Type>::max()), painted_(false), sa_(__sa), dist_(d)
  {
    // empty
  }

  /** Clear the painted state and reset internal data structures.

      This method should be called if you want to reuse the same Bellman_Ford
      object for a different computation, or if you want to clear the
      Spanning_Tree bits that were painted on the graph.

      After calling clear():
      - has_computation() returns false
      - is_painted() returns false
      - All Spanning_Tree bits on nodes/arcs are cleared
      - The predecessor array is cleared
      - NODE_COOKIE pointers are NOT touched (they were already cleaned
        by previous operations)

      @note This is automatically called at the start of each paint/compute
            operation, so explicit calls are usually not needed.

      @throw anything that the arc filter throws, while it clears the bits
             of a painting.
   */
  void clear() noexcept(nothrow_arc_filter_v<GT, SA>)
  {
    if (painted_)
      {
        // Clear Spanning_Tree bits from all nodes and arcs
        for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
          NODE_BITS(it.get_curr()).set_bit(Aleph::Spanning_Tree, false);

        for (Ait<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
          ARC_BITS(it.get_curr()).set_bit(Aleph::Spanning_Tree, false);
      }

    arcs_.empty();
    painted_ = false;
    parents_ = false;
    s_ = nullptr;
  }

  /// Check if a shortest-path tree has been computed or painted.
  /// @return true if a previous computation exists, false otherwise.
  [[nodiscard]] bool has_computation() const noexcept
  {
    return s_ != nullptr;
  }

  /// Check if a shortest-path tree has been painted.
  /// @return true if a tree has been painted, false otherwise.
  [[nodiscard]] bool is_painted() const noexcept
  {
    return painted_;
  }

  /// Get the start node of the last computation.
  /// @return Pointer to the start node, or nullptr if no computation exists.
  Node *get_start_node() const noexcept
  {
    return s_;
  }

  /// Get reference to the graph.
  /// @return Reference to the graph.
  const GT &get_graph() const noexcept
  {
    return g_;
  }

private:
  /// Relax all arcs up to n-1 times (standard Bellman-Ford). Stops as soon
  /// as a pass relaxes no arc: the distances have reached their fixed point
  /// and no later pass (including the extra negative-cycle check) can relax
  /// anything.
  /// @throw overflow_error if a sum leaves the range of an integer type.
  void relax_arcs()
  {
    const size_t &n = g_.vsize();
    if (n <= 1)
      return;  // Nothing to relax for empty or single-node graphs

    for (size_t i = 0; i < n - 1; ++i)
      {
        bool relaxed = false;
        for (Ait<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
          {
            auto arc = it.get_curr();
            auto src = g_.get_src_node(arc);
            const auto &accum_src = accum(src);
            if (accum_src == Inf)
              continue;

            auto tgt = it.get_tgt_node_ne();
            auto w = dist_(arc);
            Distance_Type sum{};
            if (not add_if_fits(accum_src, w, sum))
              continue; // no distance that fits goes through arc
            auto &accum_tgt = accum(tgt);
            if (relaxes(sum, accum_tgt))  // Relax Arc
              {
                const auto &index = idx(tgt);
                arcs_(index) = arc;
                accum_tgt = sum;
                relaxed = true;
              }
          }
        if (not relaxed)
          return;  // converged early
      }
  }

  /// Insert a node into the queue if not already present (SPFA optimization).
  static void put_in_queue(DynListQueue<typename GT::Node *> &q, typename GT::Node *p)
  {
    if (IS_NODE_VISITED(p, Depth_First))  // is already inside the queue?
      return;
    NODE_BITS(p).set_bit(Depth_First, true);
    q.put(p);
  }

  /// Remove a node from the queue and clear its in-queue flag.
  static typename GT::Node *get_from_queue(DynListQueue<typename GT::Node *> &q)
  {
    auto ret = q.get();
    assert(IS_NODE_VISITED(ret, Depth_First));
    NODE_BITS(ret).set_bit(Depth_First, false);
    return ret;
  }

  /// Relax outgoing arcs from a source node (SPFA variant). Inlined: it is
  /// the step of the loops of SPFA.
  [[gnu::always_inline]] void relax_arcs(typename GT::Node *src,
                                         DynListQueue<typename GT::Node *> &q)
  {
    for (NAit<GT, SA> it(src, sa_); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto arc_src = g_.get_src_node(arc);
        const auto &accum_src = accum(arc_src);
        if (accum_src == Inf)
          continue;

        auto tgt = g_.get_tgt_node(arc);
        auto w = dist_(arc);
        Distance_Type sum{};
        if (not add_if_fits(accum_src, w, sum))
          continue; // no distance that fits goes through arc
        auto &accum_tgt = accum(tgt);
        if (relaxes(sum, accum_tgt))  // Relax Arc
          {
            const auto &index = idx(tgt);
            arcs_(index) = arc;
            accum_tgt = sum;
            put_in_queue(q, tgt);
          }
      }
  }

  /// Paint the spanning tree nodes and arcs with the Spanning_Tree bit.
  void paint_tree() noexcept
  {  // paint the involved nodes and arcs
    const size_t n = g_.vsize();
    for (size_t i = 0; i < n; ++i)
      {
        auto arc = arcs_(i);
        if (arc == nullptr)
          continue;

        ARC_BITS(arc).set_bit(Aleph::Spanning_Tree, true);
        auto src = g_.get_src_node(arc);
        auto tgt = g_.get_tgt_node(arc);
        NODE_BITS(src).set_bit(Aleph::Spanning_Tree, true);
        NODE_BITS(tgt).set_bit(Aleph::Spanning_Tree, true);
      }
    NODE_BITS(s_).set_bit(Aleph::Spanning_Tree, true);

    painted_ = true;
  }

  /// Perform one more relaxation pass and check for negative cycle.
  /// Also updates predecessor array for cycle detection, and keeps the last
  /// node that it relaxes, on a negative cycle or reached from one.
  bool last_relax_and_prepare_check_negative_cycle()
  {
    bool negative_cycle = false;
    for (Ait<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g_.get_src_node(arc);
        auto &accum_src = accum(src);
        if (accum_src == Inf)
          continue;

        auto tgt = g_.get_tgt_node(arc);
        auto d = dist_(arc);
        auto &accum_tgt = accum(tgt);
        Distance_Type sum{};
        if (not add_if_fits(accum_src, d, sum))
          continue; // no distance that fits goes through arc
        check_finite(sum);
        if (relaxes(sum, accum_tgt))
          {
            negative_cycle = true;
            const auto &index = idx(tgt);
            arcs_(index) = arc;
            accum_tgt = sum;
            relaxed_ = tgt;
          }
      }
    return negative_cycle;
  }

  /// Perform one more relaxation pass to detect (but not prepare) negative cycle.
  bool last_relax_and_test_negative_cycle()
  {
    for (Ait<GT, SA> it(g_, sa_); it.has_curr(); it.next_ne())
      {
        auto arc = it.get_curr();
        auto src = g_.get_src_node(arc);
        auto &accum_src = accum(src);
        if (accum_src == Inf)
          continue;

        auto tgt = g_.get_tgt_node(arc);
        auto d = dist_(arc);
        auto &accum_tgt = accum(tgt);
        Distance_Type sum{};
        if (not add_if_fits(accum_src, d, sum))
          continue; // no distance that fits goes through arc
        check_finite(sum);
        if (relaxes(sum, accum_tgt))
          return true;
      }
    return false;
  }

  /// Free node cookies and set up predecessor pointers for path reconstruction.
  void link_cookies_and_free(typename GT::Node *start) noexcept
  {
    uninit<Ni>();

    // Construct the inverted paths to the start origin node
    const size_t n = g_.vsize();
    for (size_t i = 0; i < n; ++i)
      {
        auto arc = arcs_(i);
        if (arc == nullptr)
          continue;

        auto tgt = g_.get_tgt_node(arc);
        NODE_COOKIE(tgt) = g_.get_src_node(arc);
      }

    if (start != nullptr)
      NODE_COOKIE(start) = nullptr;  // just in case there is a negative cycle
    parents_ = true;
  }

  /// Put in q the start, or, without one, every node, as the virtual source
  /// reaches them.
  void put_sources_in_queue(DynListQueue<typename GT::Node *> &q)
  {
    if (s_ != nullptr)
      put_in_queue(q, s_);
    else
      for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
        put_in_queue(q, it.get_curr());
  }

  /// The source of the arc by which the last relaxation of p reached it,
  /// or nullptr.
  Node *pred_node(Node *p) noexcept
  {
    auto arc = arcs_(idx(p));
    return arc == nullptr ? nullptr : g_.get_src_node(arc);
  }

  /** A cycle of the graph of the predecessors, the arcs of the last
      relaxations, as a path of its arcs; empty if there is none. Every cycle
      of that graph is negative: the relaxation of its last arc made its end
      lower than the sum of the weights from it. The search goes back from
      relaxed_ first, and then from each node, each walk marking the nodes
      that it reaches.
  */
  Path<GT> cycle_of_predecessors()
  {
    for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
      mark(it.get_curr()) = 0;

    size_t walk = 0;
    auto cycle_reached_from = [&](Node *v) -> Node *
      {
        if (mark(v) != 0)
          return nullptr;  // an earlier walk went on from it
        ++walk;
        Node *p = v;
        while (p != nullptr and mark(p) == 0)
          {
            mark(p) = walk;
            p = pred_node(p);
          }
        return p != nullptr and mark(p) == walk ? p : nullptr;
      };

    Node *c = relaxed_ == nullptr ? nullptr : cycle_reached_from(relaxed_);
    for (typename GT::Node_Iterator it(g_); c == nullptr and it.has_curr(); it.next_ne())
      c = cycle_reached_from(it.get_curr());

    if (c == nullptr)
      return Path<GT>(g_);

    DynList<Arc *> cycle;  // its arcs from c, as they go
    Node *p = c;
    do
      {
        Arc *arc = arcs_(idx(p));
        cycle.insert(arc);
        p = g_.get_src_node(arc);
      }
    while (p != c);

    Path<GT> ret(g_, c);
    for (auto it = cycle.get_it(); it.has_curr(); it.next_ne())
      ret.append_directed(it.get_curr());
    return ret;
  }

  /// The witness of a negative cycle that the last pass found
  Path<GT> negative_cycle_witness()
  {
    Path<GT> ret = cycle_of_predecessors();
    if (ret.is_empty())
      {
        WARNING(
          "Serious inconsistency. Bellman-Ford algorithm has detected\n"
          "a negative cycle, but the graph of the predecessors has no\n"
          "cycle\n\n"
          "Be very careful, this is provably a bug");
      }
    return ret;
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
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    relax_arcs();
    const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (beyond_max_ and not negative_cycle)
      check_beyond_max(s_);
    guard.release();

    // Only paint the tree if there's no negative cycle
    // A negative cycle makes the shortest path tree meaningless
    if (not negative_cycle)
      paint_tree();

    link_cookies_and_free(s_);

    // After the cookies are freed: the check calls the filter, which can
    // throw
    assert(negative_cycle or check_painted_arcs());

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
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    const auto &n = g_.get_num_nodes();

    DynListQueue<typename GT::Node *> q;

    Node __sentinel;
    Node *sentinel = &__sentinel;

    put_in_queue(q, s_);
    put_in_queue(q, sentinel);

    for (size_t i = 0; not q.is_empty();)
      {
        auto src = get_from_queue(q);
        if (src == sentinel)  // Is the sentinel removed?
          {
            if (i++ > n)
              break;

            put_in_queue(q, sentinel);
          }
        else
          relax_arcs(src, q);
      }

    const bool negative_cycle = last_relax_and_prepare_check_negative_cycle();
    if (beyond_max_ and not negative_cycle)
      check_beyond_max(s_);
    guard.release();

    // Only paint the tree if there's no negative cycle
    // A negative cycle makes the shortest path tree meaningless
    if (not negative_cycle)
      paint_tree();

    link_cookies_and_free(s_);

    // After the cookies are freed: the check calls the filter, which can
    // throw
    assert(negative_cycle or check_painted_arcs());

    return negative_cycle;
  }

public:
  /** Test if a negative cycle exists starting from a specific node.

      @param[in] start Source node from which to check for negative cycles.
      @return true if a negative cycle is reachable from start, false otherwise.
      @throw std::domain_error if start is nullptr.
   */
  bool has_negative_cycle(Node *start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    return detect_negative_cycle(start);
  }

  /** Test if a negative cycle exists anywhere in the graph.

      Every node is a source, at distance 0, as if a virtual source reached
      each one by an arc of weight 0; the graph gets no node.

      @return true if any negative cycle exists in the graph, false otherwise.
   */
  bool has_negative_cycle()
  {
    return detect_negative_cycle(nullptr);
  }

private:
  bool detect_negative_cycle(Node *start)
  {
    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    relax_arcs();
    const bool negative_cycle = last_relax_and_test_negative_cycle();
    if (beyond_max_ and not negative_cycle)
      check_beyond_max(s_);
    guard.release();
    uninit<Ni>();

    return negative_cycle;
  }

  /// Without a start, every node is a source
  Path<GT> find_negative_cycle(Node *start)
  {
    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    relax_arcs();
    Path<GT> ret(g_);
    if (last_relax_and_prepare_check_negative_cycle())
      ret = negative_cycle_witness();
    else if (beyond_max_)
      check_beyond_max(s_);
    guard.release();

    link_cookies_and_free(s_);
    return ret;
  }

  /// SPFA, which, from it_factor * n rounds, and then every step rounds,
  /// looks for a cycle among the predecessors; without a start, every
  /// node is a source
  std::tuple<Path<GT>, size_t> faster_find_negative_cycle(Node *start,
                                                          double it_factor,
                                                          size_t step)
  {
    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    const auto &n = g_.get_num_nodes();
    DynListQueue<typename GT::Node *> q;
    Node __sentinel;
    Node *sentinel = &__sentinel;
    put_sources_in_queue(q);
    put_in_queue(q, sentinel);

    double threshold = it_factor * n;
    Path<GT> ret(g_);

    size_t i = 0;
    while (not q.is_empty())
      {
        auto src = get_from_queue(q);
        if (src == sentinel)
          {
            if (i++ > n)
              break;

            put_in_queue(q, sentinel);
            if (i >= threshold)  // must I search negative cycles?
              {
                ret = cycle_of_predecessors();
                if (not ret.is_empty())  // negative cycle found?
                  {
                    guard.release();
                    link_cookies_and_free(s_);
                    return std::make_tuple(std::move(ret), i);
                  }
                threshold += step;
              }
          }
        else
          relax_arcs(src, q);
      }

    if (last_relax_and_prepare_check_negative_cycle())
      ret = negative_cycle_witness();
    else if (beyond_max_)
      check_beyond_max(s_);
    guard.release();

    link_cookies_and_free(s_);
    return std::make_tuple(std::move(ret), i);
  }

  /// SPFA; without a start, every node is a source
  Path<GT> faster_find_negative_cycle(Node *start)
  {
    init_with_indexes(start);
    Init_Guard guard([this]() { abandon(); });

    const auto &n = g_.get_num_nodes();
    DynListQueue<typename GT::Node *> q;
    Node __sentinel;
    Node *sentinel = &__sentinel;
    put_sources_in_queue(q);
    put_in_queue(q, sentinel);

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

    Path<GT> ret(g_);
    if (last_relax_and_prepare_check_negative_cycle())
      ret = negative_cycle_witness();
    else if (beyond_max_)
      check_beyond_max(s_);
    guard.release();

    link_cookies_and_free(s_);
    return ret;
  }

public:
  /** Search a negative cycle on all possible paths starting from
      `start` node.

      If a negative cycle is found, it is a cycle of the graph of the
      predecessors, given by its arcs. Otherwise (there is no negative
      cycle), the returned path is empty.

      @param[in] start starting node
      @return a valid path corresponding to a negative cycle if this is
      found. Otherwise, an empty path
  */
  Path<GT> test_negative_cycle(Node *start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    return find_negative_cycle(start);
  }

  /** Searches and returns a negative cycle (if it exists).

      Every node is a source, at distance 0, as if a virtual source reached
      each one by an arc of weight 0; the graph gets no node.

      @return a path containing a negative cycle (if it
      exists). Otherwise, it returns an empty path.
   */
  Path<GT> test_negative_cycle()
  {
    return find_negative_cycle(nullptr);
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
  std::tuple<Path<GT>, size_t> search_negative_cycle(Node *start, double it_factor, const size_t step)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    return faster_find_negative_cycle(start, it_factor, step);
  }

  /** Searches a negative cycle using the faster version of Bellman-Ford
      algorithm (SPFA variant).

      @param[in] start source node from which the shortest paths tree is built.
      @return a path containing the negative cycle if found, or an empty path
      if no negative cycle exists reachable from the start node.
   */
  Path<GT> search_negative_cycle(Node *start)
  {
    ah_domain_error_if(start == nullptr) << "start node cannot be null";

    return faster_find_negative_cycle(start);
  }

  /** Searches a negative cycle using the faster version of Bellman-Ford
      algorithm and iteratively searching the cycle before finishing up.

      \overload search_negative_cycle(Node * start, double it_factor, const size_t step)

      This overloaded version searches for negatives cycles in all the
      graph: every node is a source, at distance 0, as if a virtual source
      reached each one by an arc of weight 0. The graph gets no node.
   */
  std::tuple<Path<GT>, size_t> search_negative_cycle(double it_factor, const size_t step)
  {
    return faster_find_negative_cycle(nullptr, it_factor, step);
  }

  /**
     \overload search_negative_cycle(Node * start)
  */
  Path<GT> search_negative_cycle()
  {
    return faster_find_negative_cycle(nullptr);
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
    ah_domain_error_if(not painted_) << "Spanning tree has not been painted";

    DynArray<Arc *> ret;
    ret.reserve(arcs_.size());
    for (size_t i = 0; i < arcs_.size(); ++i)
      ret.touch(i) = arcs_(i);
    return ret;
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
    ah_domain_error_if(not painted_) << "Graph has not been previously painted";
    ah_domain_error_if(not parents_) << "build_tree() mapped the tree; paint it again";

    ah_domain_error_if(node == nullptr) << "node cannot be null";

    // The arcs of the path, from node up to the start
    Dfs_Stack<Arc *> path;
    for (auto curr = node; curr != s_; )
      {
        auto parent = static_cast<Node *>(NODE_COOKIE(curr));
        ah_domain_error_if(parent == nullptr) << "Node is not reachable from start node";

        // Find the arc from parent to curr
        for (NAit<GT, SA> it(parent, sa_); it.has_curr(); it.next_ne())
          if (auto arc = it.get_curr();
              g_.get_tgt_node(arc) == curr && IS_ARC_VISITED(arc, Aleph::Spanning_Tree))
            {
              path.push(arc);
              break;
            }
        curr = parent;
      }

    // From the start, so that each partial sum is the distance of a node of
    // the path, which fits; from node, a sum could leave Distance_Type
    Distance_Type total = 0;
    for (; not path.is_empty(); path.pop())
      total = checked_add(total, dist_(path.top()));
    return total;
  }

  /** Extract from the graph a previously painted shortest paths tree.

      This method requires that the shortest paths tree has been
      computed with `paint_spanning_tree()` or `faster_paint_spanning_tree()`.

      @param[in] tree a graph where the shortest paths tree will be stored.
      @param[in] with_map if true the extracted tree is mapped
      through the cookies with the graph; then the cookies no longer hold
      the parents, and the tree must be painted again for the methods that
      read them.
      @throw domain_error if the tree has not been painted, or if a former
      build_tree() mapped it.
   */
  void build_tree(GT &tree, bool with_map = true)
  {
    ah_domain_error_if(not painted_) << "Spanning tree has not been painted";
    ah_domain_error_if(not parents_) << "build_tree() mapped the tree; paint it again";

    clear_graph(tree);

    DynMapTree<Node *, Node *> table;
    for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
      {
        auto gp = it.get_curr();
        auto tp = tree.insert_node(gp->get_info());
        table.insert(gp, tp);
      }

    for (typename GT::Node_Iterator it(g_); it.has_curr(); it.next_ne())
      {
        auto gtgt = it.get_curr();
        auto gsrc = static_cast<Node *>(NODE_COOKIE(gtgt));
        if (gsrc == nullptr)
          continue;  // This is the source node of the spanning tree

        // Find the spanning tree arc from gsrc to gtgt
        Arc *garc = nullptr;
        for (Out_Iterator<GT, SA> ait(gsrc, sa_); ait.has_curr(); ait.next_ne())
          {
            auto a = ait.get_curr();
            if (g_.get_tgt_node(a) == gtgt and IS_ARC_VISITED(a, Aleph::Spanning_Tree))
              {
                garc = a;
                break;
              }
          }
        ah_logic_error_unless(garc != nullptr) << "Arc not found between nodes in spanning tree";

        auto tsrc_ptr = table.search(gsrc);
        auto ttgt_ptr = table.search(gtgt);
        ah_logic_error_unless(tsrc_ptr) << "Source node not found in mapping table";
        ah_logic_error_unless(ttgt_ptr) << "Target node not found in mapping table";
        auto tarc = tree.insert_arc(tsrc_ptr->second, ttgt_ptr->second, garc->get_info());
        if (with_map)
          {
            ARC_COOKIE(garc) = nullptr; // else it would compose with a former tree
            GT::map_arcs(garc, tarc);
          }
      }

    // the cookies of the nodes held their parents, already used
    if (with_map)
      table.for_each([](const auto &p)
      {
        NODE_COOKIE(p.first) = nullptr;
        GT::map_nodes(p.first, p.second);
      });
    parents_ = parents_ and not with_map;
  }

  /** Test for negative cycle and return it if found.

      @param[in] s The starting node.
      @param[out] cycle Path where the cycle is stored if found.
      @return true if a negative cycle exists, false otherwise.
   */
  bool test_negative_cycle(typename GT::Node *s, Path<GT> &cycle)
  {
    cycle = test_negative_cycle(s);
    return not cycle.is_empty();
  }

  /** Test for negative cycle anywhere in the graph.

      @param[out] cycle Path where the cycle is stored if found.
      @return true if a negative cycle exists, false otherwise.
   */
  bool test_negative_cycle(Path<GT> &cycle)
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
  Distance_Type get_min_path(typename GT::Node *end, Path<GT> &path)
  {
    ah_domain_error_if(not painted_) << "Graph has not been previously painted";
    ah_domain_error_if(not parents_) << "build_tree() mapped the tree; paint it again";

    return Aleph::get_min_path<GT, Distance>(s_, end, path, dist_);
  }

  /** Returns a mapping with the shortest distances to each node obtained
      after executing the Bellman-Ford algorithm from a virtual source
      connected to all nodes with zero-weight edges: every node starts at 0,
      and the graph gets no node. This routine is specifically designed for
      Johnson's algorithm for computing all-pairs shortest paths.

      @return A tree mapping where each node is mapped to its distance from
      the virtual source (used as potential for edge reweighting).
      @throw std::domain_error if negative cycles are detected.
   */
  DynMapTree<Node *, Distance_Type> compute_nodes_weights()
  {
    init_with_indexes(nullptr);
    Init_Guard guard([this]() { abandon(); });

    const auto &n = g_.get_num_nodes();

    DynListQueue<typename GT::Node *> q;

    Node __sentinel;
    Node *sentinel = &__sentinel;

    put_sources_in_queue(q);
    put_in_queue(q, sentinel);

    for (size_t i = 0; not q.is_empty(); /* nothing */)
      {
        auto src = get_from_queue(q);
        if (src == sentinel)  // Is the sentinel removed?
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
      for (auto it = g_.get_node_it(); it.has_curr(); it.next_ne())
        {
          auto p = it.get_curr();
          ret.insert(p, accum(p));
        }

    guard.release();
    abandon();  // the data of the nodes go, and the object has no computation

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
template <AlephGraph GT,
          ArcDistance<GT> Distance = Dft_Dist<GT>,
          template <class, class> class Ait = Arc_Iterator,
          template <class, class> class NAit = Node_Arc_Iterator,
          ArcFilter<GT> SA = Dft_Show_Arc<GT>>
struct Bellman_Ford_Negative_Cycle
{
  /** Invokes the detection and calculation of a negative cycle.

      @param[in,out] g the graph (temporarily modified during computation).
      @param[out] path the path where the cycle is stored in case
      a negative cycle is detected.
      @param[in,out] d distance map (modified during computation).
      @param[in] sa arc filter for traversal.
      @return true if a negative cycle exists; false otherwise.
      @throw bad_alloc if there is not enough memory.
   */
  bool operator()(GT &g, Path<GT> &path, Distance &d, SA &sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).test_negative_cycle(path);
  }

  bool operator()(GT &g, Path<GT> &path, Distance &&d = Distance(), SA &&sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).test_negative_cycle(path);
  }

  /** Invokes the detection and calculation of a negative cycle.

      @param[in,out] g the graph (temporarily modified during computation).
      @param[in] s the starting node.
      @param[out] path the path where the cycle is stored in case
      a negative cycle is detected.
      @param[in,out] d distance map (modified during computation).
      @param[in] sa arc filter for traversal.
      @return true if a negative cycle exists; false otherwise.
      @throw bad_alloc if there is not enough memory.
   */
  bool operator()(GT &g, typename GT::Node *s, Path<GT> &path, Distance &d, SA &sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).test_negative_cycle(s, path);
  }

  bool operator()(GT &g,
                  typename GT::Node *s,
                  Path<GT> &path,
                  Distance &&d = Distance(),
                  SA &&sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).test_negative_cycle(s, path);
  }

  /** Search for a negative cycle starting from a specific node.

      @param[in,out] g The graph (temporarily modified during computation).
      @param[in] s The starting node.
      @param[in] d Distance accessor.
      @param[in] sa Arc filter.
      @return Path containing the negative cycle if found, empty path otherwise.
   */
  Path<GT> operator()(GT &g, typename GT::Node *s, Distance &d, SA &sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).search_negative_cycle(s);
  }

  /// \overload
  Path<GT> operator()(GT &g, typename GT::Node *s, Distance &&d = Distance(), SA &&sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).search_negative_cycle(s);
  }

  /** Search for a negative cycle in the entire graph.

      @param[in,out] g The graph (temporarily modified during computation).
      @param[in] d Distance accessor.
      @param[in] sa Arc filter.
      @return Path containing the negative cycle if found, empty path otherwise.
   */
  Path<GT> operator()(GT &g, Distance &d, SA &sa) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).search_negative_cycle();
  }

  /// \overload
  Path<GT> operator()(GT &g, Distance &&d = Distance(), SA &&sa = SA()) const
  {
    return Bellman_Ford<GT, Distance, Ait, NAit, SA>(g, d, sa).search_negative_cycle();
  }
};
}  // end namespace Aleph

#endif  // BELLMAN_FORD_H
