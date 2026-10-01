
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
 *    most negative simple cycle of at most `max_length` arcs. It runs only
 *    on the nodes and arcs that lie inside a strongly connected component.
 *
 *  Unlike `Bellman_Ford` (`Bellman_Ford.H`), neither function modifies the
 *  graph (no dummy node, no cookies, no bits) and both take a `const GT &`;
 *  they keep their state in private arrays indexed by node/arc position.
 *  The graph is snapshotted once (arcs, endpoints and weights), so the
 *  distance accessor is called exactly once per arc.
 *
 *  ## Complexity
 *
 *  Both functions first snapshot the graph in O((V + E) log V) (nodes are
 *  mapped to positions with an AVL map), then work on arrays. `E` counts
 *  every arc the arc iterator visits, which includes the arcs rejected by
 *  the filter. Working space excludes the cycles that are returned (up to
 *  O(K · V) for `K` of them).
 *
 *  | Function | Time | Working space |
 *  |----------|------|---------------|
 *  | `find_disjoint_negative_cycles` | O((V + E) log V + R · V · (V + E)) | O(V + E) |
 *  | `most_negative_cycle_bounded` | O((V + E) log V + V_c · L · (V_c + E_c)) | O(L · V_c + V + E) |
 *
 *  `R` is the number of searches (rounds) the enumeration runs: at most
 *  `max_cycles + 1` unless floating-point candidates are discarded without
 *  being reported, and never more than `E + 1`. `L = min(max_length, V)`.
 *  `V_c` and `E_c` are the nodes and arcs of the *cyclic core*: those inside
 *  a strongly connected component that has an arc (`V_c <= V`, `E_c <= E`).
 *  A graph without cycles costs just the snapshot and an O(V + E) scan; the
 *  usual `O(V · L · E)` form of the second bound holds when `E_c >= V_c`.
 *
 *  ## Numeric model
 *
 *  Every cost is accumulated with the `+` and `<` of `Distance::Distance_Type`,
 *  in the order the algorithm happens to add them. For integer costs that is
 *  exact arithmetic (an overflow throws `std::overflow_error`), and every
 *  statement below about "negative", "minimum" and `is_exact` is a
 *  mathematical one. For floating-point costs the statements hold *for the
 *  arithmetic as evaluated*, not for the real numbers the weights denote:
 *  rounding, cancellation and absorption can make a cycle whose exact total
 *  is zero look negative, and can hide a negative cycle (a huge partial sum
 *  absorbs a small weight). Results for floating-point costs are therefore
 *  never a mathematical certificate; applications that need a safety margin
 *  should fold it into the distance accessor (for instance by adding a
 *  minimum profit to every weight).
 *
 *  Two measures limit the damage. The cost reported for a cycle is always
 *  recomputed from its arcs with compensated (Neumaier) summation, so a
 *  cycle whose exact total is zero is not presented as negative because of
 *  cancellation, and `is_exact` is withdrawn if that cost and the plain sum
 *  used to rank the cycles disagree on the sign. Absorption during the
 *  search itself is not corrected: a negative cycle hidden by a huge partial
 *  distance can still go undetected by `find_disjoint_negative_cycles()`.
 *
 *  ## Lifetime and concurrency
 *
 *  The returned paths and node/arc lists hold pointers into the graph `g`:
 *  they are valid only while those nodes and arcs are. Several threads may
 *  run these functions on the same graph if nobody modifies it meanwhile and
 *  the distance accessor and the arc filter are safe to call concurrently;
 *  taking `const GT &` does not make a concurrent modification safe.
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
    Cost_Type total_cost = Cost_Type{0};   ///< Sum of the arc weights, negative; compensated sum for floating-point costs.
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

    /** Certificate that `total_cost` is the minimum cost of a simple cycle of
        at most `max_length` arcs. It is true when `total_cost` equals the
        minimum cost over the relaxed set of closed walks the dynamic
        program ranges over (see `most_negative_cycle_bounded()`), a set
        that contains every simple cycle of the bound, so no simple cycle
        can be cheaper than the reported one.

        It is a *sufficient* condition for optimality of the **cost**, not a
        necessary one: it can be false although the reported cycle is
        optimal, and being false does not mean that every minimum-cost
        closed walk is non-simple. It says nothing about the number of
        arcs: among cycles of equal cost the one with fewest arcs is
        preferred only among the candidates the search evaluated, so a
        shorter cycle of the same cost may exist even when the flag is true.
        The certificate is mathematical for integer costs; for
        floating-point costs it holds for the arithmetic as evaluated, and
        it is withdrawn when the plain-sum cost the search ranked and the
        compensated `total_cost` disagree on the sign (see the numeric model
        in the file documentation). */
    bool is_exact = false;

    Cost_Type total_cost = Cost_Type{0};       ///< Sum of the arc weights of the reported cycle (compensated sum for floating-point costs).
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

    /** @brief Total cost of the arcs `arcs` (indices into `ig.arcs`).
     *
     *  Integer costs are summed exactly, with overflow checks. Floating-point
     *  costs are summed with Neumaier's compensated summation, which keeps
     *  the rounding error of the sum nearly independent of the number of
     *  terms and of the order of the arcs, and recovers the exact total in
     *  cancellation cases such as `1e16 + 1 - 1e16 - 1`. It is much more
     *  accurate than plain summation but not exact in general.
     *
     *  @param[in] ig Indexed snapshot of the graph.
     *  @param[in] arcs Indices of the arcs to add.
     *  @param[in] context Prefix for the error message if a partial sum is
     *             not finite.
     *  @return The total cost.
     *  @throw std::overflow_error Integer overflow.
     *  @throw std::domain_error Non-finite floating-point sum.
     */
    template <AlephGraph GT, typename Cost_Type>
    Cost_Type cycle_cost(const Indexed_Graph<GT, Cost_Type> & ig,
                         const Array<size_t> & arcs,
                         const char * context)
    {
      if constexpr (std::is_floating_point_v<Cost_Type>)
        {
          Cost_Type sum = Cost_Type{0};
          Cost_Type comp = Cost_Type{0};   // running compensation
          for (size_t i = 0; i < arcs.size(); ++i)
            {
              const Cost_Type w = ig.arcs(arcs(i)).weight;
              const Cost_Type t = sum + w;
              check_finite(t, context);
              if (std::fabs(sum) >= std::fabs(w))
                comp += (sum - t) + w;
              else
                comp += (w - t) + sum;
              sum = t;
            }
          const Cost_Type total = sum + comp;
          check_finite(total, context);
          return total;
        }
      else
        {
          Cost_Type sum = Cost_Type{0};
          for (size_t i = 0; i < arcs.size(); ++i)
            sum = shortest_path_detail::checked_add(sum, ig.arcs(arcs(i)).weight);
          return sum;
        }
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
   *  - every reported cycle is a simple cycle of the graph, built from the
   *    very arcs that were relaxed (parallel arcs are told apart), whose
   *    total cost is negative as evaluated;
   *  - no two reported cycles are equal, and with
   *    `Negative_Cycle_Exclusion::All_Arcs` they are pairwise arc-disjoint;
   *  - if the graph has a negative cycle among the arcs accepted by `sa`,
   *    the first round finds one, provided the arithmetic is exact (integer
   *    costs). With floating-point costs a huge partial distance can absorb
   *    a small negative weight and hide the cycle (see the numeric model
   *    in the file documentation);
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
   *          graph has no negative cycle (or, with floating-point costs, if
   *          none is detectable in the evaluated arithmetic). The paths refer
   *          to the nodes and arcs of `g`.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow.
   *
   *  @note With floating-point weights the cost of every extracted cycle is
   *        recomputed with compensated summation, and the cycle is reported
   *        only if that total is negative. A cycle of the predecessor graph
   *        that was detected through rounding (its exact total is zero or
   *        positive) is therefore not reported, and one whose plain sum
   *        rounds to zero or above but is negative is reported; in both
   *        cases its arcs are excluded, so the search keeps making progress.
   *        The reported `total_cost` is that accurate total.
   *
   *  @note Complexity: O((V + E) log V + R · V · (V + E)) worst-case time,
   *        where `R <= E + 1` is the number of rounds (at most
   *        `max_cycles + 1` unless floating-point candidates are discarded
   *        without being reported); the per-pass cycle check adds O(V) to
   *        the O(E) pass. O(V + E) working space, not counting the returned
   *        paths. In practice each round stops at the first pass whose
   *        predecessor graph contains a cycle, usually far earlier than
   *        pass V.
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

        const Cost_Type total = negative_cycles_detail::cycle_cost(
            ig, forward, "find_disjoint_negative_cycles(): cycle cost became non-finite");

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
          continue;   // the accurate total is not negative: not reportable

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
    /** @brief Out-adjacency of a snapshot in compressed form.
     *
     *  The arcs leaving node `u` occupy the positions `start(u)` to
     *  `start(u + 1) - 1`, in ascending order of their index in the
     *  snapshot; `arc`, `tgt` and `weight` are aligned with those positions.
     *
     *  @tparam Cost_Type Arc weight type.
     */
    template <typename Cost_Type>
    struct Out_Adjacency
    {
      Array<size_t> start;      ///< `n + 1` offsets.
      Array<size_t> arc;        ///< Index of the arc in `Indexed_Graph::arcs`.
      Array<size_t> tgt;        ///< Position of the target node.
      Array<Cost_Type> weight;  ///< Weight of the arc.
    };

    /** @brief Build the out-adjacency of `ig` by a counting sort, O(V + E).
     *  @param[in] ig Indexed snapshot of the graph.
     *  @return The compressed out-adjacency.
     */
    template <AlephGraph GT, typename Cost_Type>
    Out_Adjacency<Cost_Type> build_out_adjacency(const Indexed_Graph<GT, Cost_Type> & ig)
    {
      const size_t n = ig.nodes.size();
      const size_t m = ig.arcs.size();

      Out_Adjacency<Cost_Type> adj;
      adj.start = Array<size_t>(n + 1, 0);
      for (size_t a = 0; a < m; ++a)
        ++adj.start(ig.arcs(a).src + 1);
      for (size_t i = 0; i < n; ++i)
        adj.start(i + 1) += adj.start(i);

      adj.arc = Array<size_t>(m, 0);
      adj.tgt = Array<size_t>(m, 0);
      adj.weight = Array<Cost_Type>(m, Cost_Type{0});
      Array<size_t> fill(n, 0);
      for (size_t a = 0; a < m; ++a)
        {
          const auto & rec = ig.arcs(a);
          const size_t p = adj.start(rec.src) + fill(rec.src)++;
          adj.arc(p) = a;
          adj.tgt(p) = rec.tgt;
          adj.weight(p) = rec.weight;
        }
      return adj;
    }

    /** @brief Cyclic core of a snapshot: the nodes and arcs that lie inside a
     *         strongly connected component with at least one arc.
     *
     *  Every cycle, closed walk included, stays inside one strongly connected
     *  component, so an arc whose ends belong to different components, and a
     *  node that has no arc inside its component, can take part in none. The
     *  core keeps exactly the arcs with both ends in the same component
     *  (parallel arcs and self-loops included, so a single node with a loop is
     *  a cyclic component) and the nodes they touch. Both keep their relative
     *  order, which makes a search over the core visit its candidates in the
     *  order the full snapshot would.
     *
     *  The components come from an iterative Tarjan scan over arrays built
     *  from the snapshot: neither the graph nor its cookies and bits are
     *  touched, and the recursion depth does not depend on the graph.
     *
     *  @param[in] ig Indexed snapshot of the graph.
     *  @param[in] adj Out-adjacency of `ig`.
     *  @param[out] core Receives the core nodes and arcs (ascending
     *              position), both renumbered; left empty when the core is
     *              `ig` itself.
     *  @return True if every node and arc of `ig` is in the core (`ig` is then
     *          its own core and nothing is copied); false otherwise, in which
     *          case `core` holds the core, empty if the graph has no cycle.
     *          O(V + E) time and space.
     */
    template <AlephGraph GT, typename Cost_Type>
    bool cyclic_core(const Indexed_Graph<GT, Cost_Type> & ig,
                     const Out_Adjacency<Cost_Type> & adj,
                     Indexed_Graph<GT, Cost_Type> & core)
    {
      constexpr size_t unset = std::numeric_limits<size_t>::max();
      const size_t n = ig.nodes.size();
      const size_t m = ig.arcs.size();
      const Array<size_t> & start = adj.start;

      // Iterative Tarjan: `next(v)` is the position of the next arc of v to examine.
      Array<size_t> order(n, unset);
      Array<size_t> low(n, 0);
      Array<size_t> comp(n, unset);
      Array<size_t> next(n, 0);
      Array<bool> on_stack(n, false);
      Array<size_t> scc_stack;
      Array<size_t> frames;
      size_t counter = 0;
      size_t components = 0;

      for (size_t root = 0; root < n; ++root)
        {
          if (order(root) != unset)
            continue;

          order(root) = low(root) = counter++;
          next(root) = start(root);
          scc_stack.append(root);
          on_stack(root) = true;
          frames.append(root);

          while (not frames.is_empty())
            {
              const size_t v = frames.get_last();
              if (next(v) < start(v + 1))
                {
                  const size_t w = adj.tgt(next(v)++);
                  if (order(w) == unset)
                    {
                      order(w) = low(w) = counter++;
                      next(w) = start(w);
                      scc_stack.append(w);
                      on_stack(w) = true;
                      frames.append(w);
                    }
                  else if (on_stack(w) and order(w) < low(v))
                    low(v) = order(w);
                  continue;
                }

              if (low(v) == order(v))
                {
                  for (size_t x = unset; x != v; )
                    {
                      x = scc_stack.get_last();
                      (void) scc_stack.remove_last();
                      on_stack(x) = false;
                      comp(x) = components;
                    }
                  ++components;
                }

              (void) frames.remove_last();
              if (not frames.is_empty() and low(v) < low(frames.get_last()))
                low(frames.get_last()) = low(v);
            }
        }

      Array<size_t> new_id(n, unset);
      size_t kept = 0;
      for (size_t a = 0; a < m; ++a)
        {
          const auto & rec = ig.arcs(a);
          if (comp(rec.src) != comp(rec.tgt))
            continue;
          ++kept;
          new_id(rec.src) = 0;   // marks the node as part of the core
          new_id(rec.tgt) = 0;
        }

      size_t active = 0;
      for (size_t i = 0; i < n; ++i)
        if (new_id(i) != unset)
          ++active;
      if (kept == m and active == n)
        return true;

      core.nodes.reserve(active);
      for (size_t i = 0; i < n; ++i)
        if (new_id(i) != unset)
          {
            new_id(i) = core.nodes.size();
            core.nodes.append(ig.nodes(i));
          }
      core.arcs.reserve(kept);
      for (size_t a = 0; a < m; ++a)
        {
          const auto & rec = ig.arcs(a);
          if (comp(rec.src) == comp(rec.tgt))
            core.arcs.append(Arc_Rec<GT, Cost_Type>{new_id(rec.src), new_id(rec.tgt),
                                                    rec.arc, rec.weight});
        }
      return false;
    }

    /// Best simple cycle found so far by `most_negative_cycle_bounded()`.
    template <typename Cost_Type>
    struct Best_Cycle
    {
      bool found = false;
      Cost_Type cost = Cost_Type{0};
      Array<size_t> arcs;   // indices into Indexed_Graph::arcs, in cycle order

      /// Keep `candidate` if it is cheaper than the best so far, or equally
      /// cheap with fewer arcs. The tie-break only sees the candidates that
      /// are offered, not every simple cycle of the graph.
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
   *  ## The relaxed problem
   *
   *  Finding the most negative *simple* cycle of bounded length exactly is
   *  exponential in the bound (it amounts to enumerating simple paths, which
   *  explodes on graphs with hub nodes). The function solves in polynomial
   *  time a relaxed problem and then repairs its answer.
   *
   *  Let `L = min(max_length, V)`. For every node `v` taken as origin, the
   *  arcs are relaxed layer by layer from `D_0[v] = 0` (every other node
   *  unreached): `D_k[u]` is the minimum cost of a walk of exactly `k` arcs
   *  from `v` to `u`, with the constraint that `v` is left only in the first
   *  step. Hence `D_k[v]` is the minimum cost of a closed walk of exactly
   *  `k` arcs that leaves `v` once and does not pass through `v` again before
   *  its last arc. The relaxed set `W` is the set of those walks for all `v`
   *  and all `k <= L`; `best_walk` is the minimum of their costs. `W` is
   *  not "all closed walks": a walk that goes through its origin in the
   *  middle is not in it. It contains every simple cycle of at most `L` arcs
   *  (seen from any of its own nodes), so in exact arithmetic
   *  `best_walk <= c_opt`, the cost of the best simple cycle.
   *
   *  Capping `max_length` at `V` loses no simple cycle, because a simple
   *  cycle has at most `V` arcs. It does bound the walks of `W`: a walk of
   *  `W` can repeat a cycle hanging from an interior node, so it may have
   *  more than `V` arcs, and those are not examined.
   *
   *  ## Why closed walks are decomposed
   *
   *  A minimum-cost walk of `W` may repeat a node other than its origin:
   *  `v -> u -> a -> u -> b -> u -> v` (6 arcs) goes around the two cycles
   *  hanging from `u` and, if each of them costs -2, beats a simple
   *  triangle of cost -3 through `v`. Ranking by "one execution of the
   *  cycle" needs simple cycles, so for every origin `v` and every length
   *  `k` with `D_k[v]` reached, the walk the dynamic program selects
   *  (reconstructed from per-step predecessors) is decomposed into simple
   *  cycles, and each component is offered as a candidate. The most negative
   *  candidate is returned; among candidates of equal cost, the one with
   *  fewest arcs. This keeps the O((V + E) log V + V_c · L · (V_c + E_c)) cost
   *  and gives:
   *
   *  - the reported cycle is always a simple cycle of at most `max_length`
   *    arcs with its exact total cost (as evaluated), never better than the
   *    true optimum: `c_opt <= total_cost`;
   *  - when `total_cost` equals `best_walk` (`is_exact`), it is the minimum
   *    cost of a simple cycle within the bound: `c_opt <= total_cost =
   *    best_walk <= c_opt`. The flag is a sufficient certificate; see
   *    `Bounded_Cycle_Result::is_exact`;
   *  - otherwise, if the selected walk of cost `best_walk` decomposes into
   *    `m` simple cycles, `c_opt <= total_cost <= best_walk / m <= c_opt / m`,
   *    which for a negative optimum means the reported cycle earns at least
   *    `1/m` of the best possible gain;
   *  - ties are broken among the candidates that were evaluated. Per
   *    `(k, u)` the dynamic program keeps a single predecessor, so when
   *    several walks of equal cost exist only one of them is decomposed;
   *    a simple cycle of the same cost, or with fewer arcs, may therefore
   *    never become a candidate. The function does not promise the globally
   *    cheapest, nor the globally shortest among the cheapest, simple cycle
   *    unless `is_exact` certifies the cost.
   *
   *  ## Cyclic core and traversal order
   *
   *  Every cycle lies inside one strongly connected component, and a walk that
   *  leaves its component can never return to its origin. Before the dynamic
   *  program the snapshot is therefore reduced to its *cyclic core*: the arcs
   *  whose two ends are in the same component (self-loops and parallel arcs
   *  included, so a node with a self-loop is a cyclic component) and the nodes
   *  they touch. Origins, steps and relaxations only see the core; this
   *  changes none of the closed walks, the relaxed set `W`, `best_walk`, the
   *  candidates offered or their order, so the result, `is_exact` included,
   *  is the same as without the reduction. The bound `L` is still taken from
   *  the whole graph: bounding each component by its own size would drop
   *  non-simple walks the certificate ranges over.
   *
   *  A step relaxes the arcs that leave the nodes reached in the previous
   *  step. For each node reached in step `k` the predecessor is the cheapest
   *  arc into it and, among equally cheap arcs, the one with the lowest index
   *  in the snapshot (the order in which the arc iterator of `g` visits them),
   *  however the arcs are visited. A step whose reached nodes own few of the
   *  arcs visits just those through an out-adjacency; otherwise it scans every
   *  arc in index order, which is faster than many short lists. Both ways give
   *  the same walks.
   *
   *  Arcs outside the core are never accumulated, so an integer overflow
   *  that involves only them is not reported; floating-point weights are
   *  still checked for finiteness for every arc the filter accepts, when the
   *  graph is snapshotted.
   *
   *  Choosing `max_length <= 3` rules out walks that repeat an interior node
   *  when the graph has no self-loops, so the result is then exact. In
   *  market graphs the minimum-cost walk is often simple, but that is an
   *  empirical observation and not a guarantee: the repeated part can be a
   *  negative cycle by itself (a profitable triangle traversed twice).
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
   *  @tparam SA Arc filter; arcs it rejects do not exist for the search.
   *
   *  @param[in] g Graph (must be directed). It is not modified.
   *  @param[in] max_length Maximum number of arcs of the cycle; `0` yields
   *             `has_cycle = false` without touching the graph (no
   *             validation, no call to the accessor). Values above the node
   *             count are capped to it, which loses no simple cycle (a simple
   *             cycle has at most `V` arcs) but limits the closed walks that
   *             the certificate `is_exact` ranges over.
   *  @param[in] distance Arc cost accessor.
   *  @param[in] sa Arc filter.
   *
   *  @return `Bounded_Cycle_Result`; `has_cycle` is false when no cycle of
   *          at most `max_length` arcs exists. The reported cycle can have
   *          non-negative cost when no negative cycle exists within the
   *          bound: check `is_negative()`. The node and arc pointers refer
   *          to `g`.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow (on the arcs
   *          of the cyclic core), or the predecessor table size overflows
   *          `size_t`.
   *
   *  @note Complexity: O((V + E) log V + V_c · L · (V_c + E_c)) time,
   *        O(L · V_c + V + E) working space, with `L = min(max_length, V)`
   *        and `V_c`, `E_c` the nodes and arcs of the cyclic core (see the
   *        file documentation). The reconstruction and decomposition cost
   *        O(V_c · L²) in total and are absorbed because `L <= V`. The usual
   *        `O(V_c · L · E_c)` form holds when `E_c >= V_c`.
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
    if (max_length == 0)
      return result;   // documented: no snapshot, no validation, no distance call

    const auto snapshot = negative_cycles_detail::index_graph(g, distance, sa,
                                                              "most_negative_cycle_bounded");
    if (snapshot.nodes.is_empty() or snapshot.arcs.is_empty())
      return result;

    // The bound is a property of the whole graph (see the documentation), not
    // of the cyclic core the search runs on.
    const size_t L = std::min(max_length, snapshot.nodes.size());

    // Only arcs inside strongly connected components can be on a cycle: the
    // search runs on that cyclic core, which is the snapshot itself when the
    // whole graph is cyclic.
    negative_cycles_detail::Indexed_Graph<GT, Cost_Type> core;
    auto adj = negative_cycles_detail::build_out_adjacency(snapshot);
    const bool whole = negative_cycles_detail::cyclic_core(snapshot, adj, core);
    if (not whole)
      {
        if (core.arcs.is_empty())
          return result;   // no cycle at all
        adj = negative_cycles_detail::build_out_adjacency(core);
      }
    const auto & ig = whole ? snapshot : core;
    const size_t n = ig.nodes.size();
    const size_t m = ig.arcs.size();

    ah_overflow_error_if(L + 1 > std::numeric_limits<size_t>::max() / n)
      << "most_negative_cycle_bounded(): predecessor table size overflow";

    constexpr long none = -1;

    // A layer visits the arcs of the reached nodes only if they are less than
    // one in `frontier_arc_ratio` of all the arcs.
    constexpr size_t frontier_arc_ratio = 3;

    // Reachability is tracked apart from the cost: every value of Cost_Type,
    // including numeric_limits::max(), is a legitimate cost. The layers are
    // kept apart (`prev` is layer k - 1, `cur` is layer k) because a walk of
    // exactly k arcs may only extend a walk of exactly k - 1 arcs. Each layer
    // also lists the nodes it reached, so that nothing is ever reset or
    // scanned for the nodes it did not reach.
    Array<Cost_Type> prev(n, Cost_Type{0});
    Array<Cost_Type> cur(n, Cost_Type{0});
    Array<bool> prev_reached(n, false);
    Array<bool> cur_reached(n, false);
    Array<size_t> prev_list(n, 0);
    Array<size_t> cur_list(n, 0);
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
        prev(v) = Cost_Type{0};
        prev_reached(v) = true;
        prev_list(0) = v;
        size_t prev_count = 1;
        size_t prev_arcs = adj.start(v + 1) - adj.start(v);   // arcs leaving the reached nodes

        for (size_t k = 1; k <= L; ++k)
          {
            // Relax the arcs that leave the nodes reached in step k - 1. A
            // node is relaxed to the cheapest candidate and, among equally
            // cheap ones, to the arc with the lowest index in the snapshot, so
            // the outcome does not depend on the order the arcs are visited
            // in. When the reached nodes own few arcs it is cheaper to visit
            // just those (through the out-adjacency); otherwise one sequential
            // scan of all the arcs, in index order, is faster than many short
            // adjacency lists.
            size_t cur_count = 0;
            size_t cur_arcs = 0;
            auto reach = [&](const size_t t, const Cost_Type & cand, const size_t a)
            {
              cur(t) = cand;
              cur_reached(t) = true;
              pred(k * n + t) = static_cast<long>(a);
              cur_list(cur_count++) = t;
              cur_arcs += adj.start(t + 1) - adj.start(t);
            };

            if (prev_arcs * frontier_arc_ratio < m)
              {
                for (size_t i = 0; i < prev_count; ++i)
                  {
                    const size_t u = prev_list(i);
                    if (k > 1 and u == v)
                      continue;   // the origin is visited only at the ends

                    const Cost_Type base = prev(u);
                    for (size_t p = adj.start(u); p < adj.start(u + 1); ++p)
                      {
                        const size_t t = adj.tgt(p);
                        const Cost_Type cand = shortest_path_detail::checked_add(base, adj.weight(p));
                        negative_cycles_detail::check_finite(
                            cand, "most_negative_cycle_bounded(): accumulation became non-finite");

                        if (not cur_reached(t))
                          reach(t, cand, adj.arc(p));
                        else if (cand < cur(t)
                                 or (cand == cur(t)
                                     and adj.arc(p) < static_cast<size_t>(pred(k * n + t))))
                          {
                            cur(t) = cand;
                            pred(k * n + t) = static_cast<long>(adj.arc(p));
                          }
                      }
                  }
              }
            else
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

                  if (not cur_reached(rec.tgt))
                    reach(rec.tgt, cand, a);
                  else if (cand < cur(rec.tgt))   // arcs come in index order: ties keep the lowest
                    {
                      cur(rec.tgt) = cand;
                      pred(k * n + rec.tgt) = static_cast<long>(a);
                    }
                }

            for (size_t i = 0; i < prev_count; ++i)
              prev_reached(prev_list(i)) = false;

            if (cur_count == 0)
              {
                prev_count = 0;
                break;   // nothing is reachable in k steps, so nothing in more
              }

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
            prev_list.swap(cur_list);
            prev_count = cur_count;
            prev_arcs = cur_arcs;
          }

        for (size_t i = 0; i < prev_count; ++i)
          prev_reached(prev_list(i)) = false;
      }

    if (not best.found)
      return result;

    // The reported cost is recomputed from the arcs of the cycle (compensated
    // summation for floating point, exact for integers). The certificate is
    // withdrawn if the dynamic program's plain-sum cost and the accurate cost
    // disagree on the sign: the search then ranked cycles with a total that
    // rounding had pushed to the wrong side of zero.
    result.total_cost = negative_cycles_detail::cycle_cost(
        ig, best.arcs, "most_negative_cycle_bounded(): cycle cost became non-finite");
    result.length = best.arcs.size();
    const bool sign_agrees = (best.cost < Cost_Type{0}) == (result.total_cost < Cost_Type{0});
    result.is_exact = sign_agrees and not (best_walk < best.cost);

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
