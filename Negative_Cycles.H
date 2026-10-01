
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


/** @file Negative_Cycles.H
 *  @brief Enumeration of negative cycles and bounded-length most negative
 *         cycle in directed graphs.
 *
 *  Two algorithms aimed at applications, such as currency or crypto
 *  arbitrage detection, where a negative cycle in a graph weighted with
 *  `-log(rate)` is an opportunity, and where one negative cycle is not
 *  enough: a trading engine wants several disjoint opportunities at once,
 *  ranked by the total profit of executing each one *once*, and only up to
 *  a small number of hops.
 *
 *  - `find_disjoint_negative_cycles()`: repeatedly runs Bellman-Ford from a
 *    virtual source (every node at distance zero), scanning the predecessor
 *    graph for a cycle after every pass (any such cycle is negative), so a
 *    certificate is usually found after a few passes; it then extracts the
 *    cycle, excludes some of its arcs and runs again, until `max_cycles`
 *    cycles are found or no negative cycle is left.
 *  - `most_negative_cycle_bounded()`: for every node as origin, runs a
 *    relaxation bounded to `max_length` steps to find the closed walks of
 *    minimum total cost, decomposes them into simple cycles and returns the
 *    most negative simple cycle of at most `max_length` arcs.
 *
 *  Unlike `Bellman_Ford` (`Bellman_Ford.H`), neither function modifies the
 *  graph (no dummy node, no cookies, no bits) and both take a `const GT &`;
 *  they keep their state in private arrays indexed by node/arc position.
 *  The graph is snapshotted once (arcs, endpoints and weights), so the
 *  distance accessor is called exactly once per arc.
 *
 *  ## Complexity
 *
 *  | Function | Time | Space |
 *  |----------|------|-------|
 *  | `find_disjoint_negative_cycles` | O(max_cycles · V · E) | O(V + E) |
 *  | `most_negative_cycle_bounded` | O(V · L · E) | O(L · V + E) |
 *
 *  @see Bellman_Ford.H One negative cycle, with the SPFA variant.
 *  @see Min_Mean_Cycle.H Cycle of minimum *mean* weight (Karp).
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef NEGATIVE_CYCLES_H
# define NEGATIVE_CYCLES_H

# include <ah-graph-concepts.H>

# include <algorithm>
# include <cmath>
# include <cstddef>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <htlist.H>
# include <shortest_path_common.H>
# include <tpl_array.H>
# include <tpl_dynMapTree.H>
# include <tpl_graph.H>

namespace Aleph
{
  /** @brief Which arcs of a found negative cycle are excluded before
   *         searching for the next one in `find_disjoint_negative_cycles()`.
   *
   *  @ingroup Graphs
   */
  enum class Negative_Cycle_Exclusion
  {
    /** Exclude only the arc of minimum (most negative) weight. Cycles that
        share nodes, or even arcs other than that one, with an already
        reported cycle can still be found: more coverage, weaker
        independence between the reported cycles. */
    Min_Weight_Arc,

    /** Exclude every arc of the cycle. The reported cycles are pairwise
        arc-disjoint (they may still share nodes), which is what a caller
        that wants to execute them simultaneously needs. */
    All_Arcs
  };

  /** @brief One negative cycle reported by `find_disjoint_negative_cycles()`.
   *
   *  @tparam GT Graph type.
   *  @tparam Cost_Type Arc weight type returned by the distance accessor.
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename Cost_Type>
  struct Negative_Cycle_Item
  {
    Path<GT> cycle;                        ///< Closed path: `is_cycle()` holds, every arc is a graph arc.
    Cost_Type total_cost = Cost_Type{0};   ///< Sum of the arc weights; always negative.
    size_t length = 0;                     ///< Number of arcs.
  };

  /** @brief Result of `most_negative_cycle_bounded()`.
   *
   *  `cycle_nodes` is a closed walk (the first node is repeated at the end)
   *  and `cycle_arcs[i]` goes from `cycle_nodes[i]` to `cycle_nodes[i + 1]`,
   *  the same layout as `Min_Mean_Cycle_Result`.
   *
   *  @tparam GT Graph type.
   *  @tparam Cost_Type Arc weight type returned by the distance accessor.
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename Cost_Type>
  struct Bounded_Cycle_Result
  {
    bool has_cycle = false;  ///< True if the graph has a cycle of at most `max_length` arcs.

    /** True when `total_cost` equals the minimum cost over all closed walks
        of at most `max_length` arcs, which proves that no simple cycle
        within the bound is more negative than the reported one. It is
        false only when every minimum-cost closed walk repeats a node other
        than its origin (a walk that leaves its origin, goes around a cycle
        hanging from some node twice or more, and comes back); the reported
        cycle is then the most negative simple cycle among the components of
        all the closed walks examined, see `most_negative_cycle_bounded()`. */
    bool is_exact = false;

    Cost_Type total_cost = Cost_Type{0};       ///< Sum of the arc weights of the reported cycle.
    size_t length = 0;                         ///< Number of arcs of the reported cycle.
    DynList<typename GT::Node *> cycle_nodes;  ///< Closed walk, first node repeated at the end.
    DynList<typename GT::Arc *> cycle_arcs;    ///< Arcs aligned with `cycle_nodes`.

    /// True if a cycle was found and its total cost is negative.
    [[nodiscard]] bool is_negative() const noexcept
    {
      return has_cycle and total_cost < Cost_Type{0};
    }
  };

  namespace negative_cycles_detail
  {
    /** @brief Snapshot of one graph arc: endpoint positions, the arc and its
     *         weight (read once from the distance accessor).
     *
     *  @tparam GT Graph type.
     *  @tparam Cost_Type Arc weight type.
     */
    template <AlephGraph GT, typename Cost_Type>
    struct Arc_Rec
    {
      size_t src = 0;
      size_t tgt = 0;
      typename GT::Arc * arc = nullptr;
      Cost_Type weight = Cost_Type{0};
    };

    /** @brief Positional snapshot of a graph: nodes by position and the arcs
     *         accepted by the arc filter, in iteration order.
     *
     *  @tparam GT Graph type.
     *  @tparam Cost_Type Arc weight type.
     */
    template <AlephGraph GT, typename Cost_Type>
    struct Indexed_Graph
    {
      Array<typename GT::Node *> nodes;          // position -> node
      Array<Arc_Rec<GT, Cost_Type>> arcs;        // accepted arcs, in Arc_Iterator order
    };

    template <typename Cost_Type>
    inline void check_finite([[maybe_unused]] const Cost_Type & value,
                             [[maybe_unused]] const char * context)
    {
      if constexpr (std::is_floating_point_v<Cost_Type>)
        ah_domain_error_if(not std::isfinite(value)) << context;
    }

    /// Snapshot the graph: index its nodes and the arcs accepted by `sa`,
    /// reading each weight once. Rejects undirected graphs and non-finite
    /// floating weights.
    template <AlephGraph GT, ArcDistance<GT> Distance, ArcFilter<GT> SA>
    Indexed_Graph<GT, typename Distance::Distance_Type>
    index_graph(const GT & g, Distance & distance, SA & sa, const char * fn)
    {
      using Node = typename GT::Node;
      using Arc = typename GT::Arc;
      using Cost_Type = typename Distance::Distance_Type;

      static_assert(std::is_arithmetic_v<Cost_Type>,
                    "negative cycle search requires arithmetic arc costs");

      ah_domain_error_if(not g.is_digraph()) << fn << "(): graph must be directed";

      Indexed_Graph<GT, Cost_Type> ig;
      const size_t n = g.get_num_nodes();
      ig.nodes.reserve(n);

      DynMapTree<Node *, size_t> node_to_idx;
      size_t idx = 0;
      for (Node_Iterator<GT> it(g); it.has_curr(); it.next_ne(), ++idx)
        {
          Node * node = it.get_curr_ne();
          ig.nodes.append(node);
          node_to_idx.insert(node, idx);
        }

      ig.arcs.reserve(g.get_num_arcs());
      for (Arc_Iterator<GT, SA> it(g, sa); it.has_curr(); it.next_ne())
        {
          Arc * arc = it.get_curr_ne();
          const Cost_Type w = distance(arc);
          check_finite(w, "negative cycle search requires finite arc weights");
          ig.arcs.append(Arc_Rec<GT, Cost_Type>{node_to_idx.find(g.get_src_node(arc)),
                                                node_to_idx.find(g.get_tgt_node(arc)),
                                                arc, w});
        }

      return ig;
    }

    /** @brief Scan the predecessor graph for a cycle.
     *
     *  `pred(v)` holds the index into `ig.arcs` of the arc that last relaxed
     *  `v`, or -1. Any cycle of that graph certifies a negative cycle of the
     *  underlying graph: each `pred` assignment records an arc that strictly
     *  improved its target, so the weights around a predecessor cycle
     *  telescope to a strictly negative sum (the "walk to the root" check of
     *  Cherkassky and Goldberg, 1999).
     *
     *  @param[in] ig Indexed snapshot of the graph.
     *  @param[in] pred Predecessor arc index per node, -1 for none.
     *  @param[in,out] mark Scratch array of `pred.size()` entries; overwritten.
     *
     *  @return The index of a node lying on a predecessor cycle, or -1 if
     *          the predecessor graph is acyclic. O(V): every node is walked
     *          at most once.
     */
    template <AlephGraph GT, typename Cost_Type>
    long find_predecessor_cycle(const Indexed_Graph<GT, Cost_Type> & ig,
                                const Array<long> & pred,
                                Array<long> & mark)
    {
      const size_t n = mark.size();
      for (size_t i = 0; i < n; ++i)
        mark(i) = 0;

      long chain = 0;
      for (size_t s = 0; s < n; ++s)
        {
          if (mark(s) != 0)
            continue;

          ++chain;
          for (size_t v = s; ; )
            {
              mark(v) = chain;
              const long a = pred(v);
              if (a == -1)
                break;
              const size_t u = ig.arcs(static_cast<size_t>(a)).src;
              if (mark(u) == chain)
                return static_cast<long>(u);   // u closes a cycle of this chain
              if (mark(u) != 0)
                break;   // joins an older chain, already known to be cycle-free
              v = u;
            }
        }
      return -1;
    }
  } // namespace negative_cycles_detail


  /** @brief Find up to `max_cycles` negative cycles, excluding arcs of each
   *         cycle found before searching for the next one.
   *
   *  Each round is a Bellman-Ford relaxation over the arcs not yet excluded,
   *  started from a virtual source (every node at distance zero, so cycles
   *  reachable from anywhere are found). After every pass the predecessor
   *  graph (at most one arc per node) is scanned in O(V) for a cycle; any
   *  such cycle is a negative cycle, so the certificate is found as soon as
   *  it appears — typically after a few passes — instead of waiting for the
   *  classical criterion of a relaxation in the n-th pass. The cycle is
   *  extracted, reported with its exact arcs and total cost, and its arcs
   *  are excluded according to `exclusion`. The search stops when
   *  `max_cycles` cycles have been reported or a pass relaxes nothing (no
   *  negative cycle is left among the remaining arcs).
   *
   *  This is a greedy enumeration, not an exhaustive one (listing all
   *  negative cycles is exponential in general). What it guarantees:
   *  - every reported cycle is a simple negative cycle of the graph, built
   *    from the very arcs that were relaxed (parallel arcs are told apart);
   *  - no two reported cycles are equal, and with
   *    `Negative_Cycle_Exclusion::All_Arcs` they are pairwise arc-disjoint;
   *  - if the graph has a negative cycle among the arcs accepted by `sa`,
   *    the first round finds one;
   *  - the search terminates: every round excludes at least one arc that was
   *    still allowed.
   *
   *  What it does not guarantee: that the reported cycles are the most
   *  negative ones, or that after excluding an arc no negative cycle through
   *  the other arcs of the reported cycle is missed
   *  (`Negative_Cycle_Exclusion::Min_Weight_Arc` keeps them findable,
   *  `All_Arcs` deliberately does not).
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter; arcs it rejects do not exist for the search.
   *
   *  @param[in] g Graph (must be directed). It is not modified.
   *  @param[in] max_cycles Maximum number of cycles to report; `0` returns
   *             an empty list without touching the graph.
   *  @param[in] exclusion Which arcs of each found cycle are excluded.
   *  @param[in] distance Arc cost accessor.
   *  @param[in] sa Arc filter.
   *
   *  @return The cycles found, in the order they were found. Empty if the
   *          graph has no negative cycle.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow.
   *
   *  @note With floating-point weights a cycle of the predecessor graph
   *        whose exact total is negative may sum to zero or above because
   *        of rounding; such a cycle is not reported but its arcs are still
   *        excluded, so the search keeps making progress.
   *
   *  @note Complexity: O(max_cycles · V · E) worst-case time (the per-pass
   *        cycle check adds O(V) to the O(E) pass), O(V + E) extra space.
   *        In practice each round stops at the first pass whose predecessor
   *        graph contains a cycle, usually far earlier than pass V.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  DynList<Negative_Cycle_Item<GT, typename Distance::Distance_Type>>
  find_disjoint_negative_cycles(const GT & g,
                                const size_t max_cycles,
                                const Negative_Cycle_Exclusion exclusion =
                                    Negative_Cycle_Exclusion::Min_Weight_Arc,
                                Distance distance = Distance(),
                                SA sa = SA())
  {
    using Cost_Type = typename Distance::Distance_Type;
    using Item = Negative_Cycle_Item<GT, Cost_Type>;

    DynList<Item> result;
    if (max_cycles == 0)
      return result;   // documented: no snapshot, no validation, no distance call

    auto ig = negative_cycles_detail::index_graph(g, distance, sa,
                                                  "find_disjoint_negative_cycles");
    const size_t n = ig.nodes.size();
    const size_t m = ig.arcs.size();
    if (n == 0 or m == 0)
      return result;

    constexpr long none = -1;
    Array<bool> forbidden(m, false);
    Array<Cost_Type> dist(n, Cost_Type{0});
    Array<long> pred(n, none);   // index into ig.arcs of the arc that last relaxed the node
    Array<long> mark(n, 0);      // scratch for the predecessor cycle check

    while (result.size() < max_cycles)
      {
        for (size_t i = 0; i < n; ++i)
          {
            dist(i) = Cost_Type{0};
            pred(i) = none;
          }

        // Up to n passes; after each one the predecessor graph is scanned
        // for a cycle, which certifies a negative cycle as soon as one has
        // taken shape instead of waiting for a relaxation in the n-th pass.
        long cycle_entry = none;
        for (size_t pass = 0; pass < n and cycle_entry == none; ++pass)
          {
            bool relaxed = false;
            for (size_t a = 0; a < m; ++a)
              {
                if (forbidden(a))
                  continue;

                const auto & rec = ig.arcs(a);
                const Cost_Type cand =
                    shortest_path_detail::checked_add(dist(rec.src), rec.weight);
                negative_cycles_detail::check_finite(
                    cand, "find_disjoint_negative_cycles(): accumulation became non-finite");

                if (cand < dist(rec.tgt))
                  {
                    dist(rec.tgt) = cand;
                    pred(rec.tgt) = static_cast<long>(a);
                    relaxed = true;
                  }
              }

            if (not relaxed)
              break;   // converged: no negative cycle among the allowed arcs

            cycle_entry = negative_cycles_detail::find_predecessor_cycle(ig, pred, mark);
          }

        if (cycle_entry == none)
          break;   // no negative cycle among the allowed arcs

        // cycle_entry lies on a cycle of the predecessor graph.
        const size_t y = static_cast<size_t>(cycle_entry);
        bool ok = true;

        // Extract the cycle through y; predecessors give it in reverse order.
        Array<size_t> reversed;
        for (size_t cur = y, guard = 0; ; ++guard)
          {
            const long a = pred(cur);
            if (a == none or guard > n)
              {
                ok = false;
                break;
              }
            reversed.append(static_cast<size_t>(a));
            cur = ig.arcs(static_cast<size_t>(a)).src;
            if (cur == y)
              break;
          }
        if (not ok)
          break;

        const size_t len = reversed.size();
        Array<size_t> forward;
        forward.reserve(len);
        for (size_t i = len; i > 0; --i)
          forward.append(reversed(i - 1));

        Cost_Type total = Cost_Type{0};
        for (size_t i = 0; i < len; ++i)
          {
            total = shortest_path_detail::checked_add(total, ig.arcs(forward(i)).weight);
            negative_cycles_detail::check_finite(
                total, "find_disjoint_negative_cycles(): cycle cost became non-finite");
          }

        // Exclude before deciding whether to report, so that the search
        // always makes progress.
        if (exclusion == Negative_Cycle_Exclusion::All_Arcs)
          for (size_t i = 0; i < len; ++i)
            forbidden(forward(i)) = true;
        else
          {
            size_t min_arc = forward(0);
            for (size_t i = 1; i < len; ++i)
              if (ig.arcs(forward(i)).weight < ig.arcs(min_arc).weight)
                min_arc = forward(i);
            forbidden(min_arc) = true;
          }

        if (not (total < Cost_Type{0}))
          continue;   // rounding made a negative cycle sum to >= 0: not reportable

        Item item;
        item.cycle = Path<GT>(g);
        item.cycle.init(ig.nodes(y));
        for (size_t i = 0; i < len; ++i)
          item.cycle.append_directed(ig.arcs(forward(i)).arc);
        item.total_cost = total;
        item.length = len;
        result.append(std::move(item));
      }

    return result;
  }


  namespace negative_cycles_detail
  {
    /// Best simple cycle found so far by `most_negative_cycle_bounded()`.
    template <typename Cost_Type>
    struct Best_Cycle
    {
      bool found = false;
      Cost_Type cost = Cost_Type{0};
      Array<size_t> arcs;   // indices into Indexed_Graph::arcs, in cycle order

      void offer(const Cost_Type & c, const Array<size_t> & candidate)
      {
        if (found and not (c < cost or (c == cost and candidate.size() < arcs.size())))
          return;
        found = true;
        cost = c;
        arcs = candidate;
      }
    };

    /** Decompose the closed walk `walk` (arc indices, in order, starting
        and ending at the same node) into simple cycles and offer each one
        to `best`. Every node repetition closes one simple cycle; after
        popping it the scan continues, so a walk that repeats a triangle
        yields the triangle, and a figure-eight yields its two loops. */
    template <AlephGraph GT, typename Cost_Type>
    void offer_simple_components(const Indexed_Graph<GT, Cost_Type> & ig,
                                 const Array<size_t> & walk,
                                 Array<long> & pos,              // n entries, all -1 on entry and exit
                                 Array<size_t> & st_nodes,       // scratch
                                 Array<size_t> & st_in_arc,      // scratch, aligned with st_nodes
                                 Best_Cycle<Cost_Type> & best)
    {
      if (walk.is_empty())
        return;

      st_nodes.empty();
      st_in_arc.empty();

      const size_t origin = ig.arcs(walk(0)).src;
      st_nodes.append(origin);
      st_in_arc.append(std::numeric_limits<size_t>::max());   // no arc enters the origin
      pos(origin) = 0;

      Array<size_t> component;
      for (size_t i = 0; i < walk.size(); ++i)
        {
          const size_t a = walk(i);
          const size_t t = ig.arcs(a).tgt;
          if (pos(t) == -1)
            {
              st_nodes.append(t);
              st_in_arc.append(a);
              pos(t) = static_cast<long>(st_nodes.size() - 1);
              continue;
            }

          // t is already on the stack at position p: the arcs entering the
          // nodes above p, plus the current one, form a simple cycle.
          const size_t p = static_cast<size_t>(pos(t));
          component.empty();
          for (size_t j = p + 1; j < st_nodes.size(); ++j)
            component.append(st_in_arc(j));
          component.append(a);

          Cost_Type c = Cost_Type{0};
          for (size_t j = 0; j < component.size(); ++j)
            {
              c = shortest_path_detail::checked_add(c, ig.arcs(component(j)).weight);
              check_finite(c, "most_negative_cycle_bounded(): cycle cost became non-finite");
            }
          best.offer(c, component);

          while (st_nodes.size() > p + 1)
            {
              pos(st_nodes.get_last()) = -1;
              (void) st_nodes.remove_last();
              (void) st_in_arc.remove_last();
            }
        }

      // The walk is closed, so everything above the origin was popped.
      while (not st_nodes.is_empty())
        {
          pos(st_nodes.get_last()) = -1;
          (void) st_nodes.remove_last();
        }
      st_in_arc.empty();
    }
  } // namespace negative_cycles_detail


  /** @brief Most negative simple cycle of at most `max_length` arcs, ranked
   *         by total cost (not by mean cost as Karp's algorithm does).
   *
   *  For every node `v` taken as origin, the arcs are relaxed `max_length`
   *  times in Bellman-Ford fashion from `dist[v] = 0` (every other node at
   *  infinity), with the constraint that `v` is left only in the first step:
   *  after `k` steps `dist[v]` is the minimum cost of a closed walk of
   *  exactly `k` arcs that visits `v` only at its ends. Each such closed walk
   *  is reconstructed from per-step predecessors, decomposed into simple
   *  cycles, and the most negative simple cycle over all origins and lengths
   *  is returned (ties prefer fewer arcs).
   *
   *  ## Why closed walks are decomposed
   *
   *  A minimum-cost closed walk may repeat a node other than its origin:
   *  `v -> u -> a -> u -> b -> u -> v` (6 arcs) goes around the two cycles
   *  hanging from `u` and, if each of them costs -2, beats a simple
   *  triangle of cost -3 through `v`. Ranking by "one execution of the
   *  cycle" needs simple cycles, but finding the most negative simple cycle
   *  of bounded length exactly is exponential in the bound (it amounts to
   *  enumerating simple paths, which explodes on graphs with hub nodes).
   *  Decomposing the optimal walks keeps the O(V · L · E) cost and gives:
   *
   *  - the reported cycle is always a simple cycle of at most `max_length`
   *    arcs with its exact total cost, never better than the true optimum;
   *  - when some minimum-cost closed walk is itself simple (`is_exact` is
   *    true), it is provably the most negative simple cycle within the
   *    bound;
   *  - otherwise its cost is at least `1/m` of the optimum, where `m` is the
   *    number of simple cycles the optimal walk decomposes into; every
   *    simple cycle of the graph is still a candidate through each of its
   *    own nodes as origin, so in the example above the triangle (-3) is
   *    still reported, only with `is_exact = false` (the walk of cost -4
   *    could not certify it).
   *
   *  A walk that repeats a node goes around some cycle hanging from that
   *  node, which in a bid/ask market graph is at least a round trip paying
   *  the spread; the minimum-cost walk is then normally simple and
   *  `is_exact` holds. Choosing `max_length <= 3` rules the case out
   *  entirely when the graph has no self-loops.
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter; arcs it rejects do not exist for the search.
   *
   *  @param[in] g Graph (must be directed). It is not modified.
   *  @param[in] max_length Maximum number of arcs of the cycle; `0` yields
   *             `has_cycle = false`. Values above the node count are capped
   *             to it, since a closed walk that revisits its origin only at
   *             the end has at most that many arcs.
   *  @param[in] distance Arc cost accessor.
   *  @param[in] sa Arc filter.
   *
   *  @return `Bounded_Cycle_Result`; `has_cycle` is false when no cycle of
   *          at most `max_length` arcs exists. The reported cycle can have
   *          non-negative cost when no negative cycle exists within the
   *          bound: check `is_negative()`.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow, or the
   *          predecessor table size overflows `size_t`.
   *
   *  @note Complexity: O(V · L · E) time, O(L · V + E) extra space, with
   *        `L = min(max_length, V)`.
   *
   *  @see karp_minimum_mean_cycle() Minimum *mean* cycle, no length bound.
   *
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Bounded_Cycle_Result<GT, typename Distance::Distance_Type>
  most_negative_cycle_bounded(const GT & g,
                              const size_t max_length,
                              Distance distance = Distance(),
                              SA sa = SA())
  {
    using Cost_Type = typename Distance::Distance_Type;
    using Result = Bounded_Cycle_Result<GT, Cost_Type>;

    Result result;

    auto ig = negative_cycles_detail::index_graph(g, distance, sa,
                                                  "most_negative_cycle_bounded");
    const size_t n = ig.nodes.size();
    const size_t m = ig.arcs.size();
    if (max_length == 0 or n == 0 or m == 0)
      return result;

    const size_t L = std::min(max_length, n);

    ah_overflow_error_if(L + 1 > std::numeric_limits<size_t>::max() / n)
      << "most_negative_cycle_bounded(): predecessor table size overflow";

    constexpr long none = -1;

    // Reachability is tracked apart from the cost: every value of Cost_Type,
    // including numeric_limits::max(), is a legitimate cost.
    Array<Cost_Type> prev(n, Cost_Type{0});
    Array<Cost_Type> cur(n, Cost_Type{0});
    Array<bool> prev_reached(n, false);
    Array<bool> cur_reached(n, false);
    Array<long> pred((L + 1) * n, none);   // pred(k * n + u): arc that reaches u in step k

    bool has_walk = false;
    Cost_Type best_walk = Cost_Type{0};   // minimum cost over all closed walks considered
    negative_cycles_detail::Best_Cycle<Cost_Type> best;

    Array<long> pos(n, none);
    Array<size_t> st_nodes;
    Array<size_t> st_in_arc;
    Array<size_t> walk;

    for (size_t v = 0; v < n; ++v)
      {
        for (size_t i = 0; i < n; ++i)
          prev_reached(i) = false;
        prev(v) = Cost_Type{0};
        prev_reached(v) = true;

        for (size_t k = 1; k <= L; ++k)
          {
            for (size_t i = 0; i < n; ++i)
              cur_reached(i) = false;

            bool any = false;
            for (size_t a = 0; a < m; ++a)
              {
                const auto & rec = ig.arcs(a);
                if (k > 1 and rec.src == v)
                  continue;   // the origin is visited only at the ends

                if (not prev_reached(rec.src))
                  continue;

                const Cost_Type cand = shortest_path_detail::checked_add(prev(rec.src), rec.weight);
                negative_cycles_detail::check_finite(
                    cand, "most_negative_cycle_bounded(): accumulation became non-finite");

                if (not cur_reached(rec.tgt) or cand < cur(rec.tgt))
                  {
                    cur(rec.tgt) = cand;
                    cur_reached(rec.tgt) = true;
                    pred(k * n + rec.tgt) = static_cast<long>(a);
                    any = true;
                  }
              }

            if (not any)
              break;   // nothing is reachable in k steps, so nothing in more

            if (cur_reached(v))
              {
                result.has_cycle = true;
                if (not has_walk or cur(v) < best_walk)
                  {
                    has_walk = true;
                    best_walk = cur(v);
                  }

                // Reconstruct the closed walk of k arcs (predecessors give it
                // backwards) and offer its simple components.
                walk.empty();
                size_t node = v;
                for (size_t kk = k; kk > 0; --kk)
                  {
                    const long a = pred(kk * n + node);
                    ah_logic_error_if(a == none)
                      << "most_negative_cycle_bounded(): broken predecessor chain";
                    walk.append(static_cast<size_t>(a));
                    node = ig.arcs(static_cast<size_t>(a)).src;
                  }
                ah_logic_error_if(node != v)
                  << "most_negative_cycle_bounded(): walk does not close at its origin";

                for (size_t i = 0, j = walk.size() - 1; i < j; ++i, --j)
                  std::swap(walk(i), walk(j));

                negative_cycles_detail::offer_simple_components(ig, walk, pos,
                                                                st_nodes, st_in_arc, best);
              }

            prev.swap(cur);
            prev_reached.swap(cur_reached);
          }
      }

    if (not best.found)
      return result;

    result.total_cost = best.cost;
    result.length = best.arcs.size();
    result.is_exact = not (best_walk < best.cost);

    result.cycle_nodes.append(ig.nodes(ig.arcs(best.arcs(0)).src));
    for (size_t i = 0; i < best.arcs.size(); ++i)
      {
        const auto & rec = ig.arcs(best.arcs(i));
        result.cycle_nodes.append(ig.nodes(rec.tgt));
        result.cycle_arcs.append(rec.arc);
      }

    return result;
  }
} // namespace Aleph

# endif // NEGATIVE_CYCLES_H
