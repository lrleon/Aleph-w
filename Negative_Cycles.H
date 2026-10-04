
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
 *  Algorithms aimed at applications, such as currency or crypto
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
 *  - `most_negative_cycle_up_to_3()`: independently enumerates self-loops,
 *    opposite arc pairs and directed triangles, returning the exact minimum
 *    total cost within a bound in [0, 3], with the shortest cycle on ties.
 *
 *  Unlike `Bellman_Ford` (`Bellman_Ford.H`), none modifies the
 *  graph (no dummy node, no cookies, no bits) and all take a `const GT &`;
 *  they keep their state in private arrays indexed by node/arc position.
 *  The graph is snapshotted once (arcs, endpoints and weights), so the
 *  distance accessor is called exactly once per accepted arc.
 *
 *  ## Complexity
 *
 *  The functions first snapshot the graph in expected O(V + E) time (nodes
 *  are mapped to positions with an open-addressing hash table at most half
 *  full, mixing the pointers so that allocation patterns cannot cluster;
 *  only a degenerate hash could make a lookup cost more than O(1)), then
 *  work on arrays. `E` counts every arc the arc iterator visits, which
 *  includes the arcs rejected by the filter. Working space excludes the
 *  cycles that are returned (up to O(K · V) for `K` of them). The number of
 *  memory allocations does not depend on `E`, on the bound `L` or on the
 *  length of the walks examined: only the cycles that are returned allocate
 *  per cycle.
 *
 *  | Function | Time | Working space |
 *  |----------|------|---------------|
 *  | `find_disjoint_negative_cycles` | O(V + E + R · V · (V + E)) | O(V + E) |
 *  | `most_negative_cycle_bounded` | O(V + E + V_c · L · (V_c + E_c) + V_c · L²) | O(L · V_c + V + E) |
 *  | `most_negative_cycle_up_to_3` | O(V + E + sum_v d_in(v) · d_out(v)) | O(V + E) |
 *
 *  For the short-cycle search, degrees count distinct neighbours in the
 *  cyclic core; the degree-product term is absent for bounds 1 and 2.
 *
 *  `R` is the number of searches (rounds) the enumeration runs: at most
 *  `max_cycles + 1` unless floating-point candidates are discarded without
 *  being reported, and never more than `E + 1`. `L = min(max_length, V)`.
 *  `V_c` and `E_c` are the nodes and arcs of the *cyclic core*: those inside
 *  a strongly connected component that has an arc (`V_c <= V`, `E_c <= E`).
 *  A graph without cycles costs just the snapshot and an O(V + E) scan. The
 *  `V_c · L²` term is the reconstruction of the closed walks: it disappears
 *  into the previous one when `L <= V_c + E_c`, but `L` is bounded by the
 *  nodes of the whole graph, not of the core, so a small core searched with a
 *  large bound pays it (see `most_negative_cycle_bounded()`). The usual
 *  `O(V · L · E)` form holds when `E_c >= V_c` and `L <= E_c`.
 *
 *  ## Numeric model
 *
 *  The searches accumulate costs with the `+` and `<` of
 *  `Distance::Distance_Type`, in the order the algorithm happens to add them.
 *  For integer costs that is exact arithmetic (an overflow throws
 *  `std::overflow_error`), and every statement below about "negative",
 *  "minimum" and `is_exact` is a mathematical one. For floating-point costs
 *  the searches round, and the results say how far they can be trusted, for
 *  the weights exactly as given (as binary fractions):
 *
 *  - The cost reported for a cycle is the exact sum of its weights,
 *    rounded within one ulp (see `negative_cycles_detail::Exact_Sum`, an expansion in
 *    the sense of Shewchuk, 1997). Its sign is exact, so a cycle is reported
 *    as negative only if it is, and a cycle whose exact total is zero (such
 *    as `2^100, 1, 2^-100, -1, -2^100, -2^-100`) costs zero.
 *  - The exact sums hold any finite weights while the values stay in the
 *    range of the type: a cycle whose total, or a partial sum in cycle
 *    order, leaves it throws `std::domain_error`
 *    (`most_negative_cycle_up_to_3()` orders its three terms so that only
 *    the total matters, and skips candidates above the range). A value
 *    within one unit in the last place of the largest finite number may
 *    count as out of range too; none is ever computed wrongly.
 *  - `find_disjoint_negative_cycles()` reports only cycles whose exact total
 *    is negative. Its search, though, can miss one: a huge partial distance
 *    can absorb a small negative weight (`-1e16 + (-1)` rounds to `-1e16`)
 *    and hide the cycle, so with floating-point costs an empty result means
 *    that no negative cycle was detected, not that none exists.
 *  - `most_negative_cycle_bounded()` ranks the candidate cycles by their
 *    exact costs, and publishes `optimality_gap`: a rigorous bound of how
 *    much cheaper than the reported cycle the best one can be, which covers
 *    the rounding of every sum the search made. `is_exact` holds exactly when
 *    that bound is zero, for instance when the range of the weights makes
 *    every sum of the search exact (`numeric_quality` is `Exact`).
 *  - `most_negative_cycle_up_to_3()` always selects the exact optimum among
 *    simple cycles within its bound. It enumerates all candidates after
 *    collapsing parallel arcs to their cheapest representative, and uses
 *    exact expansions whenever a rigorous rounding filter cannot order them.
 *  - `howard_minimum_mean_cycle()` (`Howard_Min_Mean_Cycle.H`) checks its
 *    rounded result with exact arithmetic when it can; its `numeric_quality`
 *    says whether it could.
 *
 *  Applications that need a safety margin should fold it into the distance
 *  accessor (for instance by adding a minimum profit to every weight): it is
 *  a business decision, separate from these numeric guarantees. The exact
 *  arithmetic needs IEEE round-to-nearest evaluated as written: no
 *  `-ffast-math` or other reassociation of floating-point sums.
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
# include <bit>
# include <cfloat>
# include <cmath>
# include <cstddef>
# include <cstdint>
# include <limits>
# include <type_traits>
# include <utility>

# include <ah-errors.H>
# include <htlist.H>
# include <shortest_path_common.H>
# include <tpl_array.H>
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
        arc-disjoint (they may still share nodes). That diversifies the
        candidates; it does not make them executable together: two
        arc-disjoint cycles can need the same balance, and two cycles that
        share an arc might both fit in its liquidity. Checking capital,
        quantities and liquidity is up to the caller. */
    All_Arcs
  };

  /** @brief How far the numbers behind the result of a cycle search can be
   *         trusted.
   *
   *  Reported by `most_negative_cycle_bounded()` and by
   *  `howard_minimum_mean_cycle()` (`Howard_Min_Mean_Cycle.H`). Later versions
   *  may add values; code that switches on it should treat an unknown value
   *  as `Rounded`.
   *
   *  @ingroup Graphs
   */
  enum class Cycle_Numeric_Quality
  {
    /** Every sum and comparison that decided the result was exact: integral
        costs (an overflow throws instead of wrapping around), floating-point
        weights whose range makes every sum of the search exact, or a
        floating-point result verified with exact arithmetic (Howard). The
        result is what the algorithm computes in exact arithmetic (Howard's
        is then the optimum), and a certificate such as
        `Bounded_Cycle_Result::is_exact` is a mathematical proof. */
    Exact,

    /** The cycle was chosen with rounded values (floating-point costs, or
        means compared in `long double` by Karp's fallback). When weights of
        very different magnitudes cancel, a worse cycle can be chosen;
        `Bounded_Cycle_Result` bounds how much worse with `optimality_gap`.
        The reported cost is still the exact sum of the weights of the cycle,
        rounded once, so its sign is exact. */
    Rounded
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
    Cost_Type total_cost = Cost_Type{0};   ///< Sum of the arc weights, negative; for floating-point costs the exact sum, rounded once.
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

    /** Proof that `total_cost` is the minimum cost of a simple cycle of at
        most `max_length` arcs: true exactly when `optimality_gap` is zero.
        With integral costs that is when the cost equals the minimum over
        the relaxed set of closed walks the dynamic program ranges over (see
        `most_negative_cycle_bounded()`). With floating-point costs the gap
        also covers the rounding of the search, so a proof is possible when
        no sum the search computes can be rounded (`numeric_quality` is
        `Cycle_Numeric_Quality::Exact`) and, rarely, otherwise; with
        market-like weights it is usually false even though the gap is tiny.
        `matches_relaxed_bound` keeps what the search concluded in its own
        arithmetic.

        It is a *sufficient* condition for optimality of the **cost**, not a
        necessary one: it can be false although the reported cycle is
        optimal, and being false does not mean that every minimum-cost
        closed walk is non-simple. It says nothing about the number of
        arcs: among cycles of equal cost the one with fewest arcs is
        preferred only among the candidates the search evaluated, so a
        shorter cycle of the same cost may exist even when the flag is true.

        @note During development, before 2026-10-02, this flag was also set
              for floating-point costs whenever `matches_relaxed_bound` was. */
    bool is_exact = false;

    /** True when, in the arithmetic of the search, the cost the search
        computed for the reported cycle equals the minimum cost over the
        relaxed set of closed walks the dynamic program ranges over (see
        `most_negative_cycle_bounded()`), and that cost and the recomputed
        `total_cost` agree on the sign. That set contains every simple cycle
        of the bound, so in exact arithmetic no simple cycle is cheaper than
        the reported one. With integral costs it equals `is_exact`. With
        floating-point costs it is a strong hint, not a proof: rounding can
        make a worse cycle win. For instance, in the ring `2^54, 1, -2^54, -2`
        (exact total -1) the plain sum loses the 1 and gives -2, so the ring
        beats a self-loop of -1.5 and this flag is true for it. */
    bool matches_relaxed_bound = false;

    /** Rigorous upper bound of how much cheaper than `total_cost` the best
        simple cycle of at most `max_length` arcs can be: it is never
        negative, and zero means `is_exact`. With integral costs it is the
        exact difference with the relaxed minimum (saturated to the largest
        value of the type if it overflows). With floating-point costs it also
        covers the rounding of every sum the search made, computed with
        outward rounding; for market-like weights it is of the order of
        `L² · u · max|weight|` (`u` the unit roundoff), while for weights that
        cancel across many orders of magnitude it can be large, and then it
        says so. Meaningful only when `has_cycle`. */
    Cost_Type optimality_gap = Cost_Type{0};

    /// Whether the sums and comparisons behind this result were exact.
    Cycle_Numeric_Quality numeric_quality = Cycle_Numeric_Quality::Exact;

    Cost_Type total_cost = Cost_Type{0};       ///< Sum of the arc weights of the reported cycle (for floating-point costs the exact sum, rounded once, which can differ from the plain rounded sums the dynamic program compared).
    size_t length = 0;                         ///< Number of arcs of the reported cycle.
    DynList<typename GT::Node *> cycle_nodes;  ///< Closed walk, first node repeated at the end.
    DynList<typename GT::Arc *> cycle_arcs;    ///< Arcs aligned with `cycle_nodes`.

    /// True if a cycle was found and its total cost is negative.
    [[nodiscard]] bool is_negative() const noexcept
    {
      return has_cycle and total_cost < Cost_Type{0};
    }
  };

  /** @brief Exact optimum returned by most_negative_cycle_up_to_3().
   *
   *  When `has_cycle`, the witness is a minimum-total-cost simple cycle
   *  within the requested bound, with the fewest arcs among all minimizers.
   *  Floating-point weights are compared as exact binary numbers; only the
   *  published total is rounded (within one ulp, with the exact sign).
   *  Unlike Bounded_Cycle_Result, this result needs no relaxed-walk certificate.
   *  Pointers belong to the input graph and share its lifetime requirements.
   *
   *  @tparam GT Directed graph type.
   *  @tparam Cost_Type Arc weight type.
   *  @ingroup Graphs
   */
  template <AlephGraph GT, typename Cost_Type>
  struct Short_Cycle_Result
  {
    bool has_cycle = false;  ///< Whether a cycle within the bound exists.
    Cost_Type total_cost = Cost_Type{0};  ///< Exact weight sum, rounded for floating-point costs.
    size_t length = 0;  ///< Number of arcs, at most three.
    DynList<typename GT::Node *> cycle_nodes;  ///< Closed node list, first node repeated last.
    DynList<typename GT::Arc *> cycle_arcs;  ///< Original arcs aligned with consecutive nodes.

    /** @brief Whether the optimum has strictly negative total cost.
     *  @return True exactly when a negative cycle within the bound exists.
     */
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

    /** @brief Floating-point format in which the error-free transformations
     *         on values of `T` are carried out.
     *
     *  `T` itself when the compiler evaluates `T` in its own format
     *  (`FLT_EVAL_METHOD == 0`, the case of x86-64, AArch64 and MSVC);
     *  otherwise `long double`, which holds every value of `T` exactly, so
     *  that excess precision cannot break the transformations.
     */
# if defined(FLT_EVAL_METHOD) and FLT_EVAL_METHOD != 0
    template <typename T>
    using exact_float_t = long double;
# else
    template <typename T>
    using exact_float_t = T;
# endif

    /** @brief Error-free sum (Knuth's TwoSum): `s = fl(a + b)` and
     *         `s + e == a + b` exactly.
     *
     *  Needs IEEE round-to-nearest arithmetic evaluated as written (no
     *  `-ffast-math` or other reassociation) and a finite `a + b`. TwoSum's
     *  `s - a` can overflow although `s` does not, when an operand is the
     *  largest finite value (Boldo, Graillat and Muller, "On the robustness
     *  of the 2Sum and Fast2Sum algorithms", 2017); the error is then
     *  recomputed with Fast2Sum, the larger magnitude first, which is exact
     *  and cannot overflow while `s` is finite.
     *
     *  @param[in] a First term.
     *  @param[in] b Second term.
     *  @param[out] s The rounded sum.
     *  @param[out] e Its rounding error.
     */
    template <typename F>
    inline void two_sum(const F a, const F b, F & s, F & e) noexcept
    {
      s = a + b;
      const F b_virtual = s - a;
      e = (a - (s - b_virtual)) + (b - b_virtual);
      if (not std::isfinite(e)) [[unlikely]]
        e = std::fabs(a) >= std::fabs(b) ? b - (s - a) : a - (s - b);
    }

    /** @brief Exact sum of floating-point numbers.
     *
     *  The value is kept as a nonoverlapping expansion (Shewchuk, "Adaptive
     *  Precision Floating-Point Arithmetic and Fast Robust Geometric
     *  Predicates", 1997): components of increasing magnitude whose exact sum
     *  is the value, no two of them sharing a bit position. Adding a number
     *  is error free, so the value is the exact sum of everything added; only
     *  a value beyond the range of the format cannot be represented, and
     *  `add()` reports it (as it may report a value within one unit in the
     *  last place of the largest finite number). The sign of the value is
     *  the sign of its largest component, so it is exact. Storage is reused
     *  across `clear()`.
     *
     *  @tparam T Floating-point type of the numbers added.
     */
    template <typename T>
    class Exact_Sum
    {
    public:
      /// Type of the components (see `exact_float_t`).
      using Component = exact_float_t<T>;

    private:
      Array<Component> h_;   // h_(0) .. h_(n_ - 1): nonzero components, increasing magnitude
      Array<Component> g_;   // scratch of compress() and upper()
      size_t n_ = 0;

      static void put(Array<Component> & a, const size_t i, const Component x)
      {
        if (i < a.size())
          a(i) = x;
        else
          a.append(x);
      }

      // Add b to a(0 .. n - 1). Inside Grow-Expansion a partial sum can
      // overflow although the value fits: b near the largest finite value,
      // lower components of its sign, a higher one of the other sign. The
      // lower components of an expansion add up to less than the lowest bit
      // of the next one, at most half the range, so with |b| at most a
      // quarter of it every partial sum stays below three quarters: a larger
      // b is added as four exact quarters, and only the value itself (within
      // one unit in the last place of the largest finite number, or beyond)
      // can overflow.
      static bool grow(Array<Component> & a, size_t & n, const Component b)
      {
        constexpr Component quarter_of_max = std::numeric_limits<Component>::max() / 4;
        if (not (std::fabs(b) > quarter_of_max))   // NaN included: it fails below
          return grow_expansion(a, n, b);
        const Component quarter = b / 4;   // exact: |b| is far above the subnormals
        for (int i = 0; i < 4; ++i)
          if (not grow_expansion(a, n, quarter))
            return false;
        return true;
      }

      // Shewchuk's Grow-Expansion with zero elimination on a(0 .. n - 1).
      static bool grow_expansion(Array<Component> & a, size_t & n, const Component b)
      {
        Component q = b;
        size_t k = 0;
        for (size_t i = 0; i < n; ++i)
          {
            Component s, e;
            two_sum(q, a(i), s, e);
            q = s;
            if (e != Component(0))
              a(k++) = e;
          }
        n = k;
        if (q != Component(0))
          put(a, n++, q);
        return std::isfinite(q);
      }

    public:
      /// Set the value to zero; the storage is kept.
      void clear() noexcept
      {
        n_ = 0;
      }

      /// True if the value is exactly zero.
      [[nodiscard]] bool is_zero() const noexcept
      {
        return n_ == 0;
      }

      /// Exact sign of the value: -1, 0 or 1.
      [[nodiscard]] int sign() const noexcept
      {
        return n_ == 0 ? 0 : (h_(n_ - 1) < Component(0) ? -1 : 1);
      }

      /** @brief Add `b` exactly.
       *  @param[in] b The number to add.
       *  @return False if the value overflowed the components (it may also
       *          be returned for a value within one unit in the last place of
       *          their largest finite number); the value is then meaningless.
       */
      [[nodiscard]] bool add(const Component b)
      {
        return grow(h_, n_, b);
      }

      /** @brief Add the value of `x`, multiplied by `2^exponent` and negated
       *         if asked. Exact unless it overflows.
       *  @param[in] x Another sum (not this one).
       *  @param[in] exponent Power of two applied to every component.
       *  @param[in] negate Whether to subtract instead of adding.
       *  @return False on overflow.
       */
      [[nodiscard]] bool add(const Exact_Sum & x, const int exponent = 0, const bool negate = false)
      {
        ah_invalid_argument_if(&x == this) << "Exact_Sum::add(): a sum cannot be added to itself";
        for (size_t i = 0; i < x.n_; ++i)
          {
            const Component c = std::ldexp(x.h_(i), exponent);
            if (not add(negate ? -c : c))
              return false;
          }
        return true;
      }

      /** @brief Add `factor` times the value of `x`, negated if asked. The
       *         factor is applied as a sum of powers of two, so it is exact.
       *  @param[in] x Another sum (not this one).
       *  @param[in] factor Non-negative integer factor.
       *  @param[in] negate Whether to subtract instead of adding.
       *  @return False on overflow.
       */
      [[nodiscard]] bool add_multiple(const Exact_Sum & x, size_t factor, const bool negate = false)
      {
        for (int bit = 0; factor != 0; ++bit, factor >>= 1)
          if ((factor & 1) != 0 and not add(x, bit, negate))
            return false;
        return true;
      }

      /// Rewrite the expansion so that its largest component approximates the
      /// value within one unit in its last place (Shewchuk's Compress).
      void compress()
      {
        if (n_ < 2)
          return;
        while (g_.size() < n_)
          g_.append(Component(0));

        Component q = h_(n_ - 1);
        size_t bottom = n_ - 1;
        for (size_t i = n_ - 1; i-- > 0; )
          {
            Component s, e;
            two_sum(q, h_(i), s, e);
            if (e != Component(0))
              {
                g_(bottom--) = s;
                q = e;
              }
            else
              q = s;
          }
        g_(bottom) = q;

        size_t top = 0;
        for (size_t i = bottom + 1; i < n_; ++i)
          {
            Component s, e;
            two_sum(g_(i), q, s, e);
            if (e != Component(0))
              h_(top++) = e;
            q = s;
          }
        h_(top++) = q;
        n_ = top;
      }

      /// The value rounded to `T`: within one unit in the last place, zero
      /// only if the value is zero, and always with the exact sign. Plus or
      /// minus infinity if the value does not fit `T` (or comes within one
      /// unit in the last place of its largest finite number), which callers
      /// must check when the components are wider than `T` (`exact_float_t`):
      /// `add()` does not report it then.
      [[nodiscard]] T rounded()
      {
        compress();
        return n_ == 0 ? T(0) : static_cast<T>(h_(n_ - 1));
      }

      /// The smallest number of `T` not below the value, or a slightly larger
      /// one; checked exactly, so it is a rigorous upper bound. A value above
      /// the largest finite `T` gives infinity, and so does a meaningless one
      /// (after an `add()` that returned false).
      [[nodiscard]] T upper()
      {
        for (size_t i = 0; i < n_; ++i)
          if (not std::isfinite(h_(i)))
            return std::numeric_limits<T>::infinity();
        T b = rounded();
        for (;;)
          {
            if (b == std::numeric_limits<T>::infinity())
              return b;   // no finite bound; -b would not be finite either
            size_t n = n_;   // value - b, computed exactly in g_
            for (size_t i = 0; i < n_; ++i)
              put(g_, i, h_(i));
            const bool finite = grow(g_, n, -static_cast<Component>(b));
            if (finite and (n == 0 or g_(n - 1) < Component(0)))
              return b;   // value <= b
            b = std::nextafter(b, std::numeric_limits<T>::infinity());
          }
      }
    };

    /** @brief Exponents of the lowest and of the highest set bit of a nonzero
     *         finite `x`: `|x|` is an odd integer times `2^low`, and
     *         `2^high <= |x| < 2^(high + 1)`.
     *
     *  @param[in] x A nonzero finite number.
     *  @param[out] low Exponent of the lowest set bit.
     *  @param[out] high Exponent of the highest set bit.
     */
    template <typename T>
    void bit_span(const T x, int & low, int & high) noexcept
    {
      if constexpr (std::numeric_limits<T>::is_iec559
                    and (std::is_same_v<T, double> or std::is_same_v<T, float>))
        {
          // Read the fields: |x| = m * 2^e with m the integer significand.
          using Bits = std::conditional_t<std::is_same_v<T, double>, std::uint64_t, std::uint32_t>;
          constexpr int fraction_bits = std::numeric_limits<T>::digits - 1;
          constexpr int exponent_mask = std::is_same_v<T, double> ? 0x7FF : 0xFF;
          constexpr int bias = std::numeric_limits<T>::max_exponent - 1;
          const Bits bits = std::bit_cast<Bits>(x);
          const Bits fraction = bits & ((Bits(1) << fraction_bits) - 1);
          const int biased = static_cast<int>((bits >> fraction_bits) & Bits(exponent_mask));
          const Bits m = biased == 0 ? fraction : (fraction | (Bits(1) << fraction_bits));
          const int e = (biased == 0 ? 1 : biased) - bias - fraction_bits;
          low = e + std::countr_zero(m);
          high = e + static_cast<int>(sizeof(Bits) * 8) - 1 - std::countl_zero(m);
        }
      else
        {
          int e = 0;
          const T f = std::frexp(std::fabs(x), &e);   // |x| = f * 2^e, f in [0.5, 1)
          constexpr int digits = std::numeric_limits<T>::digits;
          T m = std::ldexp(f, digits);                // an integer in [2^(digits - 1), 2^digits)
          int zeros = 0;
          if constexpr (digits <= 64)
            zeros = std::countr_zero(static_cast<unsigned long long>(m));
          else
            while (std::fmod(m, T(2)) == T(0))
              {
                m /= T(2);
                ++zeros;
              }
          low = e - digits + zeros;
          high = e - 1;
        }
    }

    /** @brief The odd integer `|x| / 2^low`, with `low` the exponent of the
     *         lowest set bit of the nonzero finite `x` (see `bit_span()`).
     *
     *  @param[in] x A nonzero finite number whose significand fits 64 bits.
     *  @return The odd part of its significand.
     */
    template <typename T>
    std::uint64_t odd_significand(const T x) noexcept
    {
      static_assert(std::numeric_limits<T>::digits <= 64,
                    "odd_significand(): the significand must fit 64 bits");
      if constexpr (std::numeric_limits<T>::is_iec559
                    and (std::is_same_v<T, double> or std::is_same_v<T, float>))
        {
          using Bits = std::conditional_t<std::is_same_v<T, double>, std::uint64_t, std::uint32_t>;
          constexpr int fraction_bits = std::numeric_limits<T>::digits - 1;
          constexpr int exponent_mask = std::is_same_v<T, double> ? 0x7FF : 0xFF;
          const Bits bits = std::bit_cast<Bits>(x);
          const Bits fraction = bits & ((Bits(1) << fraction_bits) - 1);
          const bool subnormal = ((bits >> fraction_bits) & Bits(exponent_mask)) == 0;
          const Bits m = subnormal ? fraction : (fraction | (Bits(1) << fraction_bits));
          return static_cast<std::uint64_t>(m >> std::countr_zero(m));
        }
      else
        {
          int low = 0;
          int high = 0;
          bit_span(x, low, high);
          return static_cast<std::uint64_t>(std::ldexp(std::fabs(x), -low));
        }
    }

    /** @brief Total cost of the arcs `arcs` (indices into `ig.arcs`).
     *
     *  Integer costs are summed exactly, with overflow checks. Floating-point
     *  costs are summed exactly as well (`Exact_Sum`) and rounded once at the
     *  end, so the sign of the result is always the sign of the exact sum of
     *  the weights, and the value is within one unit in the last place. (A
     *  cycle such as `2^100, 1, 2^-100, -1, -2^100, -2^-100`, whose exact total
     *  is zero, costs exactly zero.)
     *
     *  @param[in] ig Indexed snapshot of the graph.
     *  @param[in] arcs Indices of the arcs to add.
     *  @param[in] context Prefix for the error message if a partial sum is
     *             not finite.
     *  @return The total cost.
     *  @throw std::overflow_error Integer overflow.
     *  @throw std::domain_error Floating-point sum outside the range of the
     *         type.
     */
    template <AlephGraph GT, typename Cost_Type>
    Cost_Type cycle_cost(const Indexed_Graph<GT, Cost_Type> & ig,
                         const Array<size_t> & arcs,
                         const char * context)
    {
      if constexpr (std::is_floating_point_v<Cost_Type>)
        {
          Exact_Sum<Cost_Type> sum;
          for (size_t i = 0; i < arcs.size(); ++i)
            ah_domain_error_if(not sum.add(ig.arcs(arcs(i)).weight)) << context;
          const Cost_Type total = sum.rounded();
          ah_domain_error_if(not std::isfinite(total)) << context;
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

    /** @brief Rounding behaviour of the sums of at most `L` weights of `ig`,
     *         added left to right in a floating-point `Cost_Type`.
     *
     *  If every weight is a multiple of `2^low` (its lowest set bit) and `L`
     *  times the largest one stays below `2^(digits + low)`, every such sum is
     *  a multiple of `2^low` with at most `digits` significant bits: it is
     *  computed exactly. Otherwise the rounding error of a sum of `k <= L`
     *  terms is at most `gamma_(k-1)` times the sum of their magnitudes
     *  (Higham, "Accuracy and Stability of Numerical Algorithms", §4.2), so at
     *  most `2 (L - 1) L u max|w|`, with `u` the unit roundoff, which is what
     *  `error_bound` receives (rounded up).
     *
     *  @param[in] ig Indexed snapshot (the cyclic core searched).
     *  @param[in] L Largest number of terms of a sum.
     *  @param[out] error_bound Bound of the rounding error of any such sum:
     *              zero when they are all exact.
     *  @return True if every such sum is exact.
     */
    template <AlephGraph GT, typename Cost_Type>
    bool short_sums_are_exact(const Indexed_Graph<GT, Cost_Type> & ig, const size_t L,
                              Cost_Type & error_bound)
    {
      static_assert(std::is_floating_point_v<Cost_Type>);
      constexpr int digits = std::numeric_limits<Cost_Type>::digits;
      constexpr Cost_Type inf = std::numeric_limits<Cost_Type>::infinity();

      bool any = false;
      int low_min = 0;
      int high_max = 0;
      Cost_Type max_abs = Cost_Type{0};
      for (size_t a = 0; a < ig.arcs.size(); ++a)
        {
          const Cost_Type w = ig.arcs(a).weight;
          if (w == Cost_Type{0})
            continue;
          int low = 0;
          int high = 0;
          bit_span(w, low, high);
          low_min = any ? std::min(low_min, low) : low;
          high_max = any ? std::max(high_max, high) : high;
          max_abs = std::max(max_abs, std::fabs(w));
          any = true;
        }

      error_bound = Cost_Type{0};
      if (not any or L < 2)
        return true;   // all zero, or single weights: nothing is ever rounded
      if (high_max + 1 + static_cast<int>(std::bit_width(L - 1)) <= digits + low_min)
        return true;

      // 2 (L - 1) L u max|w|, every step rounded up; u = 2^-digits.
      const Cost_Type factor = std::nextafter(static_cast<Cost_Type>(2.0L * static_cast<long double>(L - 1)
                                                                     * static_cast<long double>(L)), inf);
      const Cost_Type scaled = std::ldexp(max_abs, -digits) + std::numeric_limits<Cost_Type>::denorm_min();
      error_bound = std::nextafter(factor * scaled, inf);
      return false;
    }

    /** @brief Map from node pointers to their positions, built once and never
     *         modified afterwards.
     *
     *  Open addressing with linear probing over two `Array`s whose capacity is
     *  the first power of two not below `2 * n`, so the load factor never
     *  exceeds one half and a lookup takes about 1.5 probes. Pointers are
     *  mixed with the SplitMix64 finalizer, a bijection of the 64-bit
     *  pointer, before the low bits select the slot: allocation patterns
     *  (nodes at a fixed stride, for instance) cannot cluster. Compared with
     *  an AVL map (`DynMapTree`) it makes four allocations instead of one per
     *  node, and a lookup does not chase pointers.
     *
     *  @tparam Node Node type; pointers to it are the keys and are never null.
     */
    template <typename Node>
    class Node_Index
    {
      Array<Node *> keys_;
      Array<size_t> positions_;
      size_t mask_ = 0;

      static size_t slot_of(const Node * node) noexcept
      {
        auto x = reinterpret_cast<std::uintptr_t>(node);
        x += 0x9e3779b97f4a7c15ULL;
        x = (x ^ (x >> 30)) * 0xbf58476d1ce4e5b9ULL;
        x = (x ^ (x >> 27)) * 0x94d049bb133111ebULL;
        return x ^ (x >> 31);
      }

    public:
      /** @param[in] n Number of nodes that will be inserted.
       *  @throws std::overflow_error If the capacity does not fit `size_t`.
       */
      explicit Node_Index(const size_t n)
      {
        ah_overflow_error_if(n > (std::numeric_limits<size_t>::max() >> 2))
          << "Node_Index: capacity overflow";
        size_t capacity = 4;
        while (capacity < 2 * n)
          capacity <<= 1;
        keys_ = Array<Node *>(capacity, nullptr);
        positions_ = Array<size_t>(capacity, size_t{0});
        mask_ = capacity - 1;
      }

      /** Record that `node` (not yet inserted) is at `position`.
       *  @throws std::domain_error If `node` is null (null marks a free slot).
       */
      void insert(Node * node, const size_t position)
      {
        ah_domain_error_if(node == nullptr) << "Node_Index: null node";
        size_t i = slot_of(node) & mask_;
        while (keys_(i) != nullptr)
          i = (i + 1) & mask_;
        keys_(i) = node;
        positions_(i) = position;
      }

      /** Position of `node`.
       *  @throws std::domain_error If `node` is null or was not inserted. A
       *          null key has to be rejected before probing: it would match
       *          the first free slot and answer with a position of nobody.
       */
      [[nodiscard]] size_t find(const Node * node) const
      {
        ah_domain_error_if(node == nullptr) << "Node_Index: null node";
        size_t i = slot_of(node) & mask_;
        while (keys_(i) != node)
          {
            ah_domain_error_if(keys_(i) == nullptr)
              << "Node_Index: the arc endpoint is not a node of the graph";
            i = (i + 1) & mask_;
          }
        return positions_(i);
      }
    };

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

      Node_Index<Node> node_to_idx(n);
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
   *    total cost is negative: exactly so, also for floating-point costs
   *    (the exact sum of its weights);
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
   *          graph has no negative cycle; with floating-point costs, empty
   *          means only that none was detected in the evaluated arithmetic,
   *          not that none exists. The paths refer to the nodes and arcs of
   *          `g`.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow.
   *
   *  @note With floating-point weights the cost of every extracted cycle is
   *        the exact sum of its weights (rounded once), and the cycle is
   *        reported only if it is negative. A cycle of the predecessor graph
   *        that was detected only through rounding is therefore never
   *        reported, and one whose plain sum rounds to zero or above but whose
   *        exact total is negative is; in both cases its arcs are excluded, so
   *        the search keeps making progress. What the exact sum cannot undo is
   *        absorption during the search, which can hide a cycle (see the
   *        numeric model in the file documentation).
   *
   *  @note Complexity: O(V + E + R · V · (V + E)) expected worst-case time,
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

    const auto ig = negative_cycles_detail::index_graph(g, distance, sa,
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
    Array<size_t> reversed;      // the cycle of a round, last arc first
    Array<size_t> forward;       // the same cycle in order
    size_t reported = 0;         // cycles in `result`: DynList::size() is O(length)

    while (reported < max_cycles)
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
        reversed.empty();
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
        forward.empty();
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
        ++reported;
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
      Cost_Type cost = Cost_Type{0};   // plain sum: what the search computed
      Array<size_t> arcs;              // indices into Indexed_Graph::arcs, in cycle order

      /** @brief Prepare the rounding filter for cycles of at most `L` arcs.
       *  @param[in] ig Indexed graph searched.
       *  @param[in] L Largest length of an offered cycle.
       */
      template <AlephGraph GT>
      Best_Cycle(const Indexed_Graph<GT, Cost_Type> & ig, const size_t L)
      {
        if constexpr (std::is_floating_point_v<Cost_Type>)
          exact_sums_ = short_sums_are_exact(ig, L, error_);
      }

      /// Keep the cycle of `length` arcs `candidate[0..length)`, of plain-sum
      /// cost `c`, if it is cheaper than the best so far, or equally cheap
      /// with fewer arcs. With floating-point costs "cheaper" compares the
      /// exact sums of the weights, so rounding cannot rank the candidates in
      /// the wrong order. When every sum of at most `L` weights is exact, the
      /// plain sums already are; otherwise a candidate whose plain sum is
      /// above the best one's by more than twice the error bound of both is
      /// exactly worse and is dropped without an exact sum. The tie-break
      /// only sees the candidates that are offered, not every simple cycle of
      /// the graph. Storing the winner reuses the capacity of `arcs`.
      template <AlephGraph GT>
      void offer(const Indexed_Graph<GT, Cost_Type> & ig, const Cost_Type & c,
                 const size_t * candidate, const size_t length)
      {
        if constexpr (std::is_floating_point_v<Cost_Type>)
          {
            if (exact_sums_)
              {
                if (found and not (c < cost or (c == cost and length < arcs.size())))
                  return;
                store(c, candidate, length);
                return;
              }
            if (found and c > reject_above_)
              return;
            exact_candidate_.clear();
            for (size_t i = 0; i < length; ++i)
              ah_domain_error_if(not exact_candidate_.add(ig.arcs(candidate[i]).weight))
                << "most_negative_cycle_bounded(): cycle cost out of range";
            if (found)
              {
                difference_.clear();
                ah_domain_error_if(not difference_.add(exact_candidate_)
                                   or not difference_.add(exact_best_, 0, true))
                  << "most_negative_cycle_bounded(): cycle cost out of range";
                const int sign = difference_.sign();
                if (not (sign < 0 or (sign == 0 and length < arcs.size())))
                  return;
              }
            std::swap(exact_best_, exact_candidate_);
            constexpr Cost_Type inf = std::numeric_limits<Cost_Type>::infinity();
            reject_above_ = std::nextafter(std::nextafter(c + error_, inf) + error_, inf);
          }
        else if (found and not (c < cost or (c == cost and length < arcs.size())))
          return;

        store(c, candidate, length);
      }

    private:
      void store(const Cost_Type & c, const size_t * candidate, const size_t length)
      {
        found = true;
        cost = c;
        arcs.empty();
        for (size_t i = 0; i < length; ++i)
          arcs.append(candidate[i]);
      }

      // Floating-point costs only: whether every plain sum is exact, else the
      // bound of its error, and the plain sum above which a candidate is
      // exactly worse than the best (the best's plain sum plus twice that bound).
      bool exact_sums_ = true;
      Cost_Type error_ = Cost_Type{0};
      Cost_Type reject_above_ = Cost_Type{0};
      // Floating-point costs only: exact cost of the best cycle, and scratch.
      Exact_Sum<Cost_Type> exact_best_;
      Exact_Sum<Cost_Type> exact_candidate_;
      Exact_Sum<Cost_Type> difference_;
    };

    /** @brief Scratch space of `offer_simple_components()`, allocated once.
     *
     *  The stacks are used through a logical depth rather than with
     *  `Array::remove_last()`, which can contract (and so reallocate) the
     *  array when it empties out.
     */
    struct Decomposition_Scratch
    {
      Array<size_t> st_nodes;    ///< Node stack.
      Array<size_t> st_in_arc;   ///< Arc entering each stacked node, aligned with `st_nodes`.
      Array<size_t> component;   ///< Arcs of the simple cycle being offered.

      /** @param[in] max_nodes Upper bound of the distinct nodes on the stack
       *             (the node count of the graph searched).
       *  @param[in] max_walk Upper bound of the arcs of a walk.
       */
      Decomposition_Scratch(const size_t max_nodes, const size_t max_walk)
        : st_nodes(max_nodes + 1, 0), st_in_arc(max_nodes + 1, 0), component(max_walk + 1, 0)
      {
        // empty
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
                                 Decomposition_Scratch & scratch,
                                 Best_Cycle<Cost_Type> & best)
    {
      if (walk.is_empty())
        return;

      Array<size_t> & st_nodes = scratch.st_nodes;
      Array<size_t> & st_in_arc = scratch.st_in_arc;
      Array<size_t> & component = scratch.component;

      const size_t origin = ig.arcs(walk(0)).src;
      size_t top = 0;   // logical depth of both stacks
      st_nodes(top) = origin;
      st_in_arc(top) = std::numeric_limits<size_t>::max();   // no arc enters the origin
      pos(origin) = 0;
      ++top;

      for (size_t i = 0; i < walk.size(); ++i)
        {
          const size_t a = walk(i);
          const size_t t = ig.arcs(a).tgt;
          if (pos(t) == -1)
            {
              st_nodes(top) = t;
              st_in_arc(top) = a;
              pos(t) = static_cast<long>(top);
              ++top;
              continue;
            }

          // t is already on the stack at position p: the arcs entering the
          // nodes above p, plus the current one, form a simple cycle.
          const size_t p = static_cast<size_t>(pos(t));
          size_t length = 0;
          for (size_t j = p + 1; j < top; ++j)
            component(length++) = st_in_arc(j);
          component(length++) = a;

          Cost_Type c = Cost_Type{0};
          for (size_t j = 0; j < length; ++j)
            {
              c = shortest_path_detail::checked_add(c, ig.arcs(component(j)).weight);
              check_finite(c, "most_negative_cycle_bounded(): cycle cost became non-finite");
            }
          best.offer(ig, c, &component(0), length);

          while (top > p + 1)
            {
              --top;
              pos(st_nodes(top)) = -1;
            }
        }

      // The walk is closed, so everything above the origin was popped.
      while (top > 0)
        {
          --top;
          pos(st_nodes(top)) = -1;
        }
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
   *  fewest arcs (with floating-point costs the candidates are compared by
   *  the exact sums of their weights). This keeps the cost polynomial and
   *  gives:
   *
   *  - the reported cycle is always a simple cycle of at most `max_length`
   *    arcs with its exact total cost (as evaluated), never better than the
   *    true optimum: `c_opt <= total_cost`;
   *  - when the cost of that cycle equals `best_walk`
   *    (`matches_relaxed_bound`), it is the minimum cost of a simple cycle
   *    within the bound: `c_opt <= total_cost = best_walk <= c_opt`. That
   *    argument needs exact sums. With floating-point costs the dynamic
   *    program rounds, but every value it computes is at most the rounded sum
   *    of any walk it represents (rounding is monotonic), so the true minimum
   *    over `W` is at least `best_walk` minus a bound of the rounding of a sum
   *    of `L` weights. `optimality_gap` is the reported cost minus that lower
   *    bound: a rigorous bound of `total_cost - c_opt`, zero exactly when
   *    `is_exact` proves the optimum;
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
   *  when the graph has no self-loops, so the result is then optimal (in the
   *  arithmetic of the search, for floating-point costs). In
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
   *          bound: check `is_negative()`, whose answer is exact.
   *          `optimality_gap` bounds how far from the optimum the cycle can
   *          be, and `numeric_quality` says whether the search was exact. The
   *          node and arc pointers refer to `g`.
   *
   *  @throws std::domain_error If `g` is undirected or a floating weight
   *          (or an accumulated sum) is not finite.
   *  @throws std::overflow_error If integer additions overflow (on the arcs
   *          of the cyclic core), or the predecessor table size overflows
   *          `size_t`.
   *  @throws std::bad_alloc If the O(L · V_c) tables do not fit in memory
   *          (a large core searched with `max_length` near `V`).
   *
   *  @note Complexity: O(V + E + V_c · L · (V_c + E_c) + V_c · L²)
   *        expected time, O(L · V_c + V + E) working space, with
   *        `L = min(max_length, V)` and `V_c`, `E_c` the nodes and arcs of
   *        the cyclic core (see the file documentation). The last term is the
   *        reconstruction and decomposition of the closed walks, O(L) for
   *        each origin and length that closes one. It is not absorbed by the
   *        relaxation: `L` is bounded by the nodes of the whole graph, so a
   *        small core searched with a large bound is quadratic in `L` (two
   *        cyclic nodes among `V`, searched with `max_length = V`, cost
   *        Θ(V²)). The usual `O(V_c · L · E_c)` form holds when `E_c >= V_c`
   *        and `L <= E_c`. With floating-point costs, ranking the candidates
   *        by their exact sums multiplies the reconstruction term by the
   *        length of those sums as expansions (one to three components for
   *        weights of similar magnitudes), and the certificate adds O(E_c).
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
    if constexpr (std::is_floating_point_v<Cost_Type>)
      result.numeric_quality = Cycle_Numeric_Quality::Rounded;
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
    negative_cycles_detail::Best_Cycle<Cost_Type> best(ig, L);

    Array<long> pos(n, none);
    negative_cycles_detail::Decomposition_Scratch scratch(n, L);
    Array<size_t> walk;
    walk.reserve(L);

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

                negative_cycles_detail::offer_simple_components(ig, walk, pos, scratch, best);
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

    // The reported cost is recomputed from the arcs of the cycle (an exact sum,
    // rounded once, for floating point). The relaxed-bound condition is what
    // the search concluded in its own arithmetic; it is withdrawn if its
    // plain-sum cost and the exact one disagree on the sign.
    result.total_cost = negative_cycles_detail::cycle_cost(
        ig, best.arcs, "most_negative_cycle_bounded(): cycle cost became non-finite");
    result.length = best.arcs.size();
    const bool sign_agrees = (best.cost < Cost_Type{0}) == (result.total_cost < Cost_Type{0});
    result.matches_relaxed_bound = sign_agrees and not (best_walk < best.cost);

    // The certificate. No simple cycle of the bound costs less than the true
    // minimum over the relaxed set, and the dynamic program computes that
    // minimum up to the rounding of its sums: by monotonicity of rounding,
    // its value at every node is at most the plain sum of any walk that
    // reaches it, so the true minimum is at least `best_walk - error_bound`.
    // `optimality_gap` is therefore a rigorous bound of how much cheaper than
    // the reported cycle the best one can be, and a zero gap is a proof.
    if constexpr (std::is_floating_point_v<Cost_Type>)
      {
        Cost_Type error_bound = Cost_Type{0};
        if (negative_cycles_detail::short_sums_are_exact(ig, L, error_bound))
          result.numeric_quality = Cycle_Numeric_Quality::Exact;
        negative_cycles_detail::Exact_Sum<Cost_Type> gap;
        bool finite = gap.add(-best_walk) and gap.add(error_bound);
        for (size_t i = 0; finite and i < best.arcs.size(); ++i)
          finite = gap.add(ig.arcs(best.arcs(i)).weight);
        result.optimality_gap = finite ? gap.upper() : std::numeric_limits<Cost_Type>::infinity();
      }
    else
      {
        // Exact sums: the gap is the difference itself (saturated if it
        // does not fit the type).
        const Cost_Type exact_cost = result.total_cost;
        if (best_walk < Cost_Type{0}
            and exact_cost > std::numeric_limits<Cost_Type>::max() + best_walk)
          result.optimality_gap = std::numeric_limits<Cost_Type>::max();
        else
          result.optimality_gap = exact_cost - best_walk;
      }
    result.is_exact = result.optimality_gap == Cost_Type{0};

    result.cycle_nodes.append(ig.nodes(ig.arcs(best.arcs(0)).src));
    for (size_t i = 0; i < best.arcs.size(); ++i)
      {
        const auto & rec = ig.arcs(best.arcs(i));
        result.cycle_nodes.append(ig.nodes(rec.tgt));
        result.cycle_arcs.append(rec.arc);
      }

    return result;
  }

  namespace negative_cycles_detail
  {
    /** @brief Keep the cheapest parallel arc of each ordered pair.
     *  @param[in] ig Indexed graph.
     *  @param[in] adj Its out-adjacency, in snapshot order within each node.
     *  @return Out-adjacency retaining original arc indices; equal weights
     *          keep the first arc. O(V + E) time and working space.
     */
    template <AlephGraph GT, typename Cost_Type>
    Out_Adjacency<Cost_Type> cheapest_parallel_arcs(
        const Indexed_Graph<GT, Cost_Type> & ig, const Out_Adjacency<Cost_Type> & adj)
    {
      constexpr size_t none = std::numeric_limits<size_t>::max();
      const size_t n = ig.nodes.size();
      Array<size_t> best(n, none);
      Out_Adjacency<Cost_Type> out;
      out.start = Array<size_t>(n + 1, 0);
      out.arc.reserve(ig.arcs.size());
      out.tgt.reserve(ig.arcs.size());
      out.weight.reserve(ig.arcs.size());
      for (size_t u = 0; u < n; ++u)
        {
          for (size_t p = adj.start(u); p < adj.start(u + 1); ++p)
            {
              const size_t v = adj.tgt(p), a = adj.arc(p);
              if (best(v) == none or ig.arcs(a).weight < ig.arcs(best(v)).weight)
                best(v) = a;
            }
          for (size_t p = adj.start(u); p < adj.start(u + 1); ++p)
            {
              const size_t v = adj.tgt(p), a = adj.arc(p);
              if (best(v) != a)
                continue;
              out.arc.append(a);
              out.tgt.append(v);
              out.weight.append(ig.arcs(a).weight);
              best(v) = none;
            }
          out.start(u + 1) = out.arc.size();
        }
      return out;
    }

    /** @brief Rank cycles of at most three arcs exactly, reusing scratch sums.
     *
     *  A rounded sum can reject a candidate only when it is above an outward
     *  bound of `best_plain + 2 * error`: both plain sums are within `error`
     *  of their exact values. Ambiguous comparisons use exact expansions.
     *  @tparam Cost_Type Arc weight type.
     */
    template <typename Cost_Type>
    class Short_Cycle_Best
    {
      Cost_Type plain_ = Cost_Type{0};
      Cost_Type error_ = Cost_Type{0};
      Cost_Type reject_above_ = Cost_Type{0};
      bool exact_sums_ = true;
      Exact_Sum<Cost_Type> best_, candidate_, difference_;

      // Plain sum of the weights of `idx`, in that order: 0, or the sign of
      // the bound a partial sum crossed (1 above the type, -1 below it).
      template <AlephGraph GT>
      static int plain_sum(const Indexed_Graph<GT, Cost_Type> & ig, const size_t * idx,
                           const size_t len, Cost_Type & sum)
      {
        sum = Cost_Type{0};
        for (size_t i = 0; i < len; ++i)
          {
            const Cost_Type w = ig.arcs(idx[i]).weight;
            if constexpr (std::is_floating_point_v<Cost_Type>)
              {
                sum = sum + w;
                if (not std::isfinite(sum))
                  return sum > Cost_Type{0} ? 1 : -1;
              }
            else
              {
                if (w > Cost_Type{0} and sum > std::numeric_limits<Cost_Type>::max() - w)
                  return 1;
                if (w < Cost_Type{0} and sum < std::numeric_limits<Cost_Type>::lowest() - w)
                  return -1;
                sum = static_cast<Cost_Type>(sum + w);
              }
          }
        return 0;
      }

    public:
      size_t length = 0;  ///< Zero until the first cycle is offered.
      size_t arcs[3] = {};  ///< Original snapshot indices in cycle order.
      /// Whether a cycle was skipped because its total is above the largest
      /// value of the cost type. If no cycle was kept, the best one does not fit.
      bool above_range = false;

      /** @brief Prepare a rigorous rounding filter for this graph and bound.
       *  @param[in] ig Indexed graph searched.
       *  @param[in] bound Largest cycle length, in [1, 3].
       */
      template <AlephGraph GT>
      Short_Cycle_Best(const Indexed_Graph<GT, Cost_Type> & ig, const size_t bound)
      {
        if constexpr (std::is_floating_point_v<Cost_Type>)
          exact_sums_ = short_sums_are_exact(ig, bound, error_);
      }

      /** @brief Offer a cycle; exact cost and then length decide the winner.
       *
       *  The weights are added in cycle order. If a partial sum leaves the
       *  cost type, two weights of opposite signs are added first, which
       *  cannot leave it; if the total still does, it is out of range. A
       *  total above the type is worse than any that fits and is skipped
       *  (`above_range`); one below it would be the best cycle, which has no
       *  representable cost, and throws.
       *
       *  @param[in] ig Indexed graph searched.
       *  @param[in] path Snapshot arc indices of the candidate.
       *  @param[in] len Candidate length, in [1, 3].
       *  @throws std::overflow_error An integral total below the cost type.
       *  @throws std::domain_error A floating-point total below the format,
       *          or an exact sum that is not finite.
       */
      template <AlephGraph GT>
      void offer(const Indexed_Graph<GT, Cost_Type> & ig, const size_t * path, const size_t len)
      {
        size_t order[3] = {path[0], len > 1 ? path[1] : 0, len > 2 ? path[2] : 0};
        Cost_Type sum = Cost_Type{0};
        int out = plain_sum(ig, order, len, sum);
        if (out != 0 and len == 3)
          for (size_t i = 0; i < 2; ++i)
            for (size_t j = i + 1; j < 3; ++j)
              if (out != 0 and (ig.arcs(path[i]).weight < Cost_Type{0})
                                 != (ig.arcs(path[j]).weight < Cost_Type{0}))
                {
                  order[0] = path[i];
                  order[1] = path[j];
                  order[2] = path[3 - i - j];
                  out = plain_sum(ig, order, len, sum);
                }
        if (out > 0)
          {
            above_range = true;
            return;
          }
        if constexpr (std::is_floating_point_v<Cost_Type>)
          {
            ah_domain_error_if(out < 0) << "most_negative_cycle_up_to_3(): cycle cost out of range";
          }
        else
          {
            ah_overflow_error_if(out < 0) << "most_negative_cycle_up_to_3(): cycle cost out of range";
          }

        if constexpr (std::is_floating_point_v<Cost_Type>)
          {
            if (not exact_sums_)
              {
                if (length != 0 and sum > reject_above_)
                  return;
                candidate_.clear();
                bool fits = true;
                for (size_t i = 0; fits and i < len; ++i)
                  fits = candidate_.add(ig.arcs(order[i]).weight);
                if (not fits)
                  {
                    // The plain sums fitted, so the exact total is within
                    // rounding of the limit on the side of their sign.
                    ah_domain_error_if(sum < Cost_Type{0})
                      << "most_negative_cycle_up_to_3(): cycle cost out of range";
                    above_range = true;
                    return;
                  }
                if (length != 0)
                  {
                    // Different signs order the totals without subtracting
                    // values near opposite ends of the floating-point range.
                    int order = candidate_.sign() - best_.sign();
                    if (order == 0)
                      {
                        difference_.clear();
                        ah_domain_error_if(not difference_.add(candidate_)
                                           or not difference_.add(best_, 0, true))
                          << "most_negative_cycle_up_to_3(): comparison out of range";
                        order = difference_.sign();
                      }
                    if (not (order < 0 or (order == 0 and len < length)))
                      return;
                  }
                std::swap(best_, candidate_);
                constexpr Cost_Type inf = std::numeric_limits<Cost_Type>::infinity();
                reject_above_ = std::nextafter(std::nextafter(sum + error_, inf) + error_, inf);
              }
            else if (length != 0 and not (sum < plain_ or (sum == plain_ and len < length)))
              return;
          }
        else if (length != 0 and not (sum < plain_ or (sum == plain_ and len < length)))
          return;

        plain_ = sum;
        length = len;
        for (size_t i = 0; i < len; ++i)
          arcs[i] = path[i];
      }

      /** @brief Published total of the winning cycle.
       *  @return Exact integral total, or floating total within one ulp and
       *          with the exact sign. Zero when no cycle was offered.
       *  @throws std::domain_error If the floating total does not fit the
       *          cost type.
       */
      Cost_Type total()
      {
        if constexpr (std::is_floating_point_v<Cost_Type>)
          if (not exact_sums_)
            {
              // Infinite only if the best total is out of range, which
              // `add()` does not report when the components are wider.
              const Cost_Type t = best_.rounded();
              ah_domain_error_if(not std::isfinite(t))
                << "most_negative_cycle_up_to_3(): cycle cost out of range";
              return t;
            }
        return plain_;
      }
    };
  } // namespace negative_cycles_detail

  /** @brief Minimum-total-cost simple cycle with at most one, two or three arcs.
   *
   *  This independent, exact search enumerates self-loops, opposite arc pairs
   *  and directed triangles. Among equal exact costs it returns the shortest
   *  cycle; remaining ties are deterministic for a fixed snapshot order.
   *  Each ordered pair retains its cheapest accepted arc (first in snapshot
   *  order on a tie), so parallel arcs do not multiply the triangle search.
   *  Replacing a parallel arc by a cheaper one cannot worsen a simple cycle.
   *
   *  Floating-point candidates are ordered by exact sums of the binary weights,
   *  using error-free expansions when a rigorous rounding filter cannot decide.
   *  The published cost is within one ulp of that sum and has its exact sign.
   *  Requires round-to-nearest arithmetic without reassociation/fast-math, as
   *  described in the file documentation. Only the total of the reported
   *  cycle has to fit the cost type: a cycle whose total is above it is
   *  worse than any that fits and is skipped, whatever its partial sums;
   *  one below it would be the best and has no representable total.
   *  No closed-walk relaxation or certificate from that relaxation is used.
   *
   *  The graph is not modified. Each accepted arc's weight is read once.
   *  Returned pointers belong to `g`; concurrent calls require that nobody
   *  changes the graph and that the accessor and filter permit concurrent use.
   *
   *  @tparam GT Directed Aleph graph type.
   *  @tparam Distance Arc weight accessor.
   *  @tparam SA Arc filter; rejected arcs do not exist for this search.
   *  @param[in] g Graph to search.
   *  @param[in] max_length Bound in [0, 3], default 3. Zero returns an empty
   *             result without validation or calls to the accessor/filter.
   *  @param[in] distance Arc weight accessor.
   *  @param[in] sa Arc filter.
   *  @return Short_Cycle_Result; `has_cycle` includes non-negative cycles.
   *          `is_negative()` decides existence of a negative cycle in the bound.
   *  @throws std::invalid_argument If `max_length > 3` (before graph access).
   *  @throws std::domain_error If the graph is undirected, an accepted weight
   *          is not finite, or the total of the best floating-point cycle
   *          (or an exact comparison) exceeds the format.
   *  @throws std::overflow_error If the total of the best integral cycle does
   *          not fit the cost type.
   *  @note Expected time O(V + E) for bounds 1 and 2, and
   *        O(V + E + sum_v d_in(v) * d_out(v)) for bound 3. Degrees count
   *        distinct neighbours inside the cyclic core after parallel collapse.
   *        Working space O(V + E); no V-by-V table. For bound 1 only the snapshot
   *        is needed; larger bounds also compute the cyclic core.
   *  @see most_negative_cycle_bounded() General bound, relaxed-walk search.
   *  @ingroup Graphs
   */
  template <AlephGraph GT,
            ArcDistance<GT> Distance = Dft_Dist<GT>,
            ArcFilter<GT> SA = Dft_Show_Arc<GT>>
  Short_Cycle_Result<GT, typename Distance::Distance_Type>
  most_negative_cycle_up_to_3(const GT & g, const size_t max_length = 3,
                              Distance distance = Distance(), SA sa = SA())
  {
    using Cost_Type = typename Distance::Distance_Type;
    constexpr size_t none = std::numeric_limits<size_t>::max();
    Short_Cycle_Result<GT, Cost_Type> result;
    ah_invalid_argument_if(max_length > 3)
      << "most_negative_cycle_up_to_3(): max_length must be in [0, 3]";
    if (max_length == 0)
      return result;
    const auto snapshot = negative_cycles_detail::index_graph(g, distance, sa,
                                                             "most_negative_cycle_up_to_3");
    if (snapshot.arcs.is_empty())
      return result;

    negative_cycles_detail::Indexed_Graph<GT, Cost_Type> core;
    negative_cycles_detail::Out_Adjacency<Cost_Type> adj;
    bool whole = true;
    if (max_length > 1)
      {
        adj = negative_cycles_detail::build_out_adjacency(snapshot);
        whole = negative_cycles_detail::cyclic_core(snapshot, adj, core);
        if (not whole)
          {
            if (core.arcs.is_empty())
              return result;
            adj = negative_cycles_detail::build_out_adjacency(core);
          }
        adj = negative_cycles_detail::cheapest_parallel_arcs(whole ? snapshot : core, adj);
      }
    const auto & ig = whole ? snapshot : core;
    const size_t n = ig.nodes.size();
    negative_cycles_detail::Short_Cycle_Best<Cost_Type> best(ig, max_length);

    if (max_length == 1)
      {
        for (size_t a = 0; a < ig.arcs.size(); ++a)
          if (ig.arcs(a).src == ig.arcs(a).tgt)
            best.offer(ig, &a, 1);
      }
    else
      {
        // Incoming adjacency lets each origin mark all its closing arcs in
        // O(in-degree), using one shared O(V) array instead of a dense matrix.
        Array<size_t> in_start(n + 1, 0);
        for (size_t p = 0; p < adj.arc.size(); ++p)
          ++in_start(adj.tgt(p) + 1);
        for (size_t u = 0; u < n; ++u)
          in_start(u + 1) += in_start(u);
        Array<size_t> incoming(adj.arc.size(), 0), fill(n, 0), closing(n, none);
        for (size_t p = 0; p < adj.arc.size(); ++p)
          incoming(in_start(adj.tgt(p)) + fill(adj.tgt(p))++) = adj.arc(p);

        // All shorter cycles are offered before triangles, which gives the
        // rounding filter a useful incumbent even in a clean, dense market.
        for (size_t phase = 2; phase <= max_length; ++phase)
          for (size_t u = 0; u < n; ++u)
            {
              for (size_t p = in_start(u); p < in_start(u + 1); ++p)
                closing(ig.arcs(incoming(p)).src) = incoming(p);
              if (phase == 2 and closing(u) != none)
                best.offer(ig, &closing(u), 1);
              for (size_t p = adj.start(u); p < adj.start(u + 1); ++p)
                {
                  const size_t v = adj.tgt(p);
                  if (v <= u)
                    continue;   // the origin is the smallest node of the cycle
                  if (phase == 2)
                    {
                      if (closing(v) != none)
                        {
                          const size_t path[] = {adj.arc(p), closing(v)};
                          best.offer(ig, path, 2);
                        }
                    }
                  else
                    for (size_t q = adj.start(v); q < adj.start(v + 1); ++q)
                      {
                        const size_t w = adj.tgt(q);
                        if (w <= u or w == v or closing(w) == none)
                          continue;
                        const size_t path[] = {adj.arc(p), adj.arc(q), closing(w)};
                        best.offer(ig, path, 3);
                      }
                }
              for (size_t p = in_start(u); p < in_start(u + 1); ++p)
                closing(ig.arcs(incoming(p)).src) = none;
            }
      }

    if (best.length == 0)
      {
        // Every cycle had a total above the cost type: the best one does not fit.
        if constexpr (std::is_floating_point_v<Cost_Type>)
          {
            ah_domain_error_if(best.above_range)
              << "most_negative_cycle_up_to_3(): cycle cost out of range";
          }
        else
          {
            ah_overflow_error_if(best.above_range)
              << "most_negative_cycle_up_to_3(): cycle cost out of range";
          }
        return result;
      }
    result.has_cycle = true;
    result.length = best.length;
    result.total_cost = best.total();
    result.cycle_nodes.append(ig.nodes(ig.arcs(best.arcs[0]).src));
    for (size_t i = 0; i < best.length; ++i)
      {
        const auto & arc = ig.arcs(best.arcs[i]);
        result.cycle_arcs.append(arc.arc);
        result.cycle_nodes.append(ig.nodes(arc.tgt));
      }
    return result;
  }
} // namespace Aleph

# endif // NEGATIVE_CYCLES_H
