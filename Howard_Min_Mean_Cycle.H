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

/** @file Howard_Min_Mean_Cycle.H
 *  @brief Minimum mean cycle by Howard's policy iteration.
 *
 *  Solves the same problem as `karp_minimum_mean_cycle()` (`Min_Mean_Cycle.H`):
 *  the cycle @f$C@f$ of a directed weighted graph that minimizes
 *  @f$w(C) / |C|@f$. It is a faster and leaner backend, not a different
 *  criterion: it ranks cycles by *mean*, not by total cost, so for arbitrage
 *  the cycle to execute still comes from `Negative_Cycles.H`.
 *
 *  ## The algorithm
 *
 *  A *policy* picks one outgoing arc for every node, so it defines a
 *  functional graph: every component has exactly one cycle, with trees hanging
 *  from it. Evaluating a policy gives each node the mean `eta` of the cycle it
 *  reaches and a *bias* `x` (the cost of the path to the cycle, relative to
 *  `eta` per arc). Improving a policy first moves every node to a successor
 *  with a smaller `eta`; when none exists it moves nodes to a successor with
 *  the same `eta` and a smaller bias. When nothing changes the policy is
 *  optimal and the smallest `eta` is the minimum mean (Cochet-Terrasson,
 *  Cohen, Gaubert, McGettrick and Quadrat, 1998; Dasdan, 2004).
 *
 *  Only the *cyclic core* of the graph takes part (the arcs inside strongly
 *  connected components, see `Negative_Cycles.H`), so every node has an
 *  outgoing arc inside its own component.
 *
 *  ## Complexity
 *
 *  Each iteration takes @f$O(V + E)@f$ time and the working space is
 *  @f$O(V + E)@f$, against @f$O(V E)@f$ time and @f$O(V^2)@f$ space for Karp.
 *  No polynomial bound on the number of iterations is known (as for the
 *  simplex method) and constructed instances need many, but on graphs that
 *  occur in practice it is small. The function therefore stops after
 *  `V + 64` iterations and returns Karp's answer instead, so it never costs
 *  much more than Karp and always returns a correct result.
 *
 *  ## Arithmetic
 *
 *  - **Integer costs** (any integral type whose values fit a `long long`) are
 *    handled exactly: `eta` is a reduced fraction and the bias is scaled by its
 *    denominator, so every comparison is one between integers and the mean is
 *    the exact one. The scaled bias can exceed the range of `long long` before
 *    Karp's sums do; the function then returns Karp's answer.
 *  - **Floating-point costs** use `long double` with a relative tolerance in
 *    the bias comparison; the result is the minimum mean up to the rounding
 *    of that arithmetic, and the cost of the reported cycle is recomputed
 *    from its arcs with compensated summation.
 *
 *  ## Differences from `karp_minimum_mean_cycle()`
 *
 *  - The witness is always a *simple* cycle; Karp's is a closed walk that can
 *    go around a cycle many times. When several cycles share the minimum mean
 *    the two functions may return different ones.
 *  - The distance accessor is called once per arc (unless the fallback runs).
 *  - No value of the cost type is reserved as "infinity". (Karp reserves
 *    `numeric_limits<Cost_Type>::max()`, so an arc of that cost is ignored by
 *    it; when Howard hands a problem over to Karp it inherits that.)
 *  - It can answer where Karp throws: Karp adds up to `V` costs in its table,
 *    even on acyclic graphs, and overflows a narrow cost type that Howard,
 *    which only needs the cycle it reports, does not.
 *
 *  @note Both the snapshot and the cyclic core come from the internal
 *        `negative_cycles_detail` namespace of `Negative_Cycles.H`.
 *
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

#ifndef HOWARD_MIN_MEAN_CYCLE_H
#define HOWARD_MIN_MEAN_CYCLE_H

#include <ah-graph-concepts.H>

#include <cmath>
#include <cstddef>
#include <limits>
#include <numeric>
#include <type_traits>
#include <utility>

#include <ah-errors.H>
#include <Min_Mean_Cycle.H>
#include <Negative_Cycles.H>
#include <shortest_path_common.H>
#include <tpl_array.H>
#include <tpl_graph.H>

namespace Aleph {
/** @brief Result of `howard_minimum_mean_cycle()`.
 *
 *  It is a `Min_Mean_Cycle_Result` (the same fields, so it can replace the
 *  result of `karp_minimum_mean_cycle()`) plus how the answer was obtained.
 *  `cycle_nodes` holds a *simple* cycle, the first node repeated at the end.
 *
 *  @tparam GT Graph type.
 *  @tparam Cost_Type Arc weight type returned by the distance accessor.
 *  @ingroup Graphs
 */
template <AlephGraph GT, typename Cost_Type>
struct Howard_Mean_Cycle_Result : public Min_Mean_Cycle_Result<GT, Cost_Type>
{
  size_t iterations = 0;  ///< Policy improvements applied before the policy was optimal.
  bool used_karp =
    false;  ///< True if Karp's algorithm produced the answer (see the file documentation).
};

namespace howard_detail {
/// Integral costs that fit a `long long` use exact arithmetic.
template <typename Cost_Type>
inline constexpr bool exact_arithmetic_v = std::is_integral_v<Cost_Type> and
  std::numeric_limits<Cost_Type>::max() <= std::numeric_limits<long long>::max();

/** @brief Arithmetic of the policy evaluation, specialised by exactness.
 *
 *  Provides the mean of a cycle (`Eta`), the bias of a node (`Bias`) and
 *  the comparisons the algorithm needs. None of its functions throws:
 *  `step()` reports a result that cannot be represented by returning false.
 */
template <typename Cost_Type, bool Exact>
struct Arithmetic;

/// Exact arithmetic for integral costs: `Eta` is a reduced fraction and the
/// bias of a node is scaled by the denominator of its `Eta`.
template <typename Cost_Type>
struct Arithmetic<Cost_Type, true>
{
  using Wide = long long;

  /// Reduced fraction `num / den` with `den > 0`: the representation is
  /// canonical, so two equal means have equal fields.
  struct Eta
  {
    Wide num = 0;
    Wide den = 1;
  };

  /// `den * x`, with `x` the real-valued bias: an integer.
  using Bias = Wide;

  /// Accumulator of the total cost of a cycle: wide, so that the sum of the
  /// arcs of a cycle does not overflow a narrow cost type.
  using Sum = Wide;

  /// The empty sum.
  static Sum zero_sum() noexcept
  {
    return 0;
  }

  /// Add `w` to `total`; false if the sum does not fit.
  static bool add(Sum &total, const Cost_Type &w) noexcept
  {
    return not add_overflows(total, static_cast<Wide>(w), total);
  }

  /** @brief Mean of a cycle of `length` arcs and total cost `total`.
   *  @param total Sum of the arc weights of the cycle.
   *  @param length Number of arcs, at least 1.
   *  @return The reduced fraction `total / length`.
   */
  static Eta make_eta(const Sum &total, const size_t length) noexcept
  {
    const Wide num = total;
    const Wide den = static_cast<Wide>(length);
    const unsigned long long magnitude =
      num < 0 ? 0ULL - static_cast<unsigned long long>(num) : static_cast<unsigned long long>(num);
    const auto g = static_cast<Wide>(std::gcd(magnitude, static_cast<unsigned long long>(den)));
    return Eta{num / g, den / g};
  }

  /// The bias of the root of a cycle.
  static Bias zero_bias() noexcept
  {
    return 0;
  }

  /// True if the two means are equal.
  static bool equal(const Eta &x, const Eta &y) noexcept
  {
    return x.num == y.num and x.den == y.den;
  }

  /** @brief Compare `a1 / b1` with `a2 / b2` (denominators positive)
   *         without overflow, by the Euclidean algorithm.
   *  @return -1, 0 or 1.
   */
  static int compare_fractions(Wide a1, Wide b1, Wide a2, Wide b2) noexcept
  {
    for (;;)
      {
        Wide q1 = a1 / b1;
        Wide r1 = a1 % b1;
        if (r1 < 0)
          {
            --q1;
            r1 += b1;
          }
        Wide q2 = a2 / b2;
        Wide r2 = a2 % b2;
        if (r2 < 0)
          {
            --q2;
            r2 += b2;
          }

        if (q1 != q2)
          return q1 < q2 ? -1 : 1;
        if (r1 == 0 or r2 == 0)
          return r1 == 0 and r2 == 0 ? 0 : r1 == 0 ? -1 : 1;

        // r1 / b1 < r2 / b2  <=>  b2 / r2 < b1 / r1
        const Wide na1 = b2;
        const Wide nb1 = r2;
        const Wide na2 = b1;
        const Wide nb2 = r1;
        a1 = na1;
        b1 = nb1;
        a2 = na2;
        b2 = nb2;
      }
  }

  /// True if `x` is strictly smaller than `y`.
  static bool less(const Eta &x, const Eta &y) noexcept
  {
    return not equal(x, y) and compare_fractions(x.num, x.den, y.num, y.den) < 0;
  }

  /** @brief Bias of a node whose chosen arc has weight `w` and leads to a
   *         node of bias `next`, when the mean is `e`:
   *         `e.den * w - e.num + next`.
   *  @param[in] w Weight of the chosen arc.
   *  @param[in] e Mean of the cycle the node reaches.
   *  @param[in] next Bias of the node the arc leads to.
   *  @param[out] out The bias.
   *  @return False if an intermediate result does not fit a `long long`.
   */
  static bool step(const Cost_Type &w, const Eta &e, const Bias next, Bias &out) noexcept
  {
    Wide scaled = 0;
    if (multiply_overflows(e.den, static_cast<Wide>(w), scaled))
      return false;
    Wide shifted = 0;
    if (subtract_overflows(scaled, e.num, shifted))
      return false;
    return not add_overflows(shifted, next, out);
  }

  /// True if the candidate bias is strictly smaller than the current one.
  static bool improves(const Bias candidate, const Bias current, const Eta &) noexcept
  {
    return candidate < current;
  }

private:
  static bool add_overflows(const Wide a, const Wide b, Wide &out) noexcept
  {
    if ((b > 0 and a > std::numeric_limits<Wide>::max() - b) or
        (b < 0 and a < std::numeric_limits<Wide>::min() - b))
      return true;
    out = a + b;
    return false;
  }

  static bool subtract_overflows(const Wide a, const Wide b, Wide &out) noexcept
  {
    if ((b < 0 and a > std::numeric_limits<Wide>::max() + b) or
        (b > 0 and a < std::numeric_limits<Wide>::min() + b))
      return true;
    out = a - b;
    return false;
  }

  static bool multiply_overflows(const Wide a, const Wide b, Wide &out) noexcept
  {
    if (a == 0 or b == 0)
      {
        out = 0;
        return false;
      }
    constexpr Wide hi = std::numeric_limits<Wide>::max();
    constexpr Wide lo = std::numeric_limits<Wide>::min();
    if (a > 0 ? (b > 0 ? a > hi / b : b < lo / a) : b > 0 ? a < lo / b : b < hi / a)
      return true;
    out = a * b;
    return false;
  }
};

/// Floating-point arithmetic: `long double` with a relative tolerance in
/// the comparison of biases. Means and biases are not exact numbers here.
template <typename Cost_Type>
struct Arithmetic<Cost_Type, false>
{
  using Eta = long double;
  using Bias = long double;

  /// Accumulator of the total cost of a cycle.
  using Sum = long double;

  /// The empty sum.
  static Sum zero_sum() noexcept
  {
    return 0.0L;
  }

  /// Add `w` to `total`; false if the sum is not finite.
  static bool add(Sum &total, const Cost_Type &w) noexcept
  {
    total += static_cast<long double>(w);
    return std::isfinite(total);
  }

  /// `total / length`.
  static Eta make_eta(const Sum &total, const size_t length) noexcept
  {
    return total / static_cast<long double>(length);
  }

  /// The bias of the root of a cycle.
  static Bias zero_bias() noexcept
  {
    return 0.0L;
  }

  /// True if the two means are equal (the means of one component are copies).
  static bool equal(const Eta &x, const Eta &y) noexcept
  {
    return x == y;
  }

  /// True if `x` is strictly smaller than `y`.
  static bool less(const Eta &x, const Eta &y) noexcept
  {
    return x < y;
  }

  /** @brief Bias of a node: `w - e + next`.
   *  @param[in] w Weight of the chosen arc.
   *  @param[in] e Mean of the cycle the node reaches.
   *  @param[in] next Bias of the node the arc leads to.
   *  @param[out] out The bias.
   *  @return False if it is not finite.
   */
  static bool step(const Cost_Type &w, const Eta &e, const Bias next, Bias &out) noexcept
  {
    out = static_cast<long double>(w) - e + next;
    return std::isfinite(out);
  }

  /// True if the candidate is smaller than the current bias by more than
  /// the rounding noise of the arithmetic that produced them.
  static bool improves(const Bias candidate, const Bias current, const Eta &e) noexcept
  {
    const long double scale = 1.0L + std::fabs(candidate) + std::fabs(current) + std::fabs(e);
    return candidate < current - 64.0L * std::numeric_limits<long double>::epsilon() * scale;
  }
};

/** @brief Policy iteration over the out-adjacency of a graph in which every
 *         node has at least one outgoing arc.
 *
 *  @tparam Cost_Type Arc weight type.
 */
template <typename Cost_Type>
class Solver
{
  static constexpr bool exact = exact_arithmetic_v<Cost_Type>;
  using Ar = Arithmetic<Cost_Type, exact>;
  using Eta = typename Ar::Eta;
  using Bias = typename Ar::Bias;

  const negative_cycles_detail::Out_Adjacency<Cost_Type> &adj_;
  const size_t n_;
  Array<size_t> policy_;        // position in `adj_` of the chosen arc of each node
  Array<Eta> eta_;              // mean of the cycle each node reaches
  Array<Bias> bias_;            // bias of each node
  Array<unsigned char> state_;  // 0 unvisited, 1 on the current path, 2 evaluated
  Array<size_t> path_;          // scratch of `evaluate()`
  bool overflow_ = false;

  size_t successor(const size_t u) const
  {
    return adj_.tgt(policy_(u));
  }

  /// Compute `eta_` and `bias_` of every node for the current policy.
  /// Returns false on overflow.
  bool evaluate()
  {
    for (size_t i = 0; i < n_; ++i)
      state_(i) = 0;

    for (size_t s = 0; s < n_; ++s)
      {
        if (state_(s) != 0)
          continue;

        size_t top = 0;
        size_t u = s;
        while (state_(u) == 0)
          {
            state_(u) = 1;
            path_(top++) = u;
            u = successor(u);
          }

        if (state_(u) == 1)  // the walk closed a new cycle through u
          {
            size_t j = top;
            do
              --j;
            while (path_(j) != u);

            // The cycle is path_(j), ..., path_(top - 1); each node leads to the
            // next and the last one to the first.
            const size_t length = top - j;
            typename Ar::Sum total = Ar::zero_sum();
            size_t root = 0;  // offset in the cycle of its smallest node
            for (size_t i = 0; i < length; ++i)
              {
                if (not Ar::add(total, adj_.weight(policy_(path_(j + i)))))
                  return false;
                if (path_(j + i) < path_(j + root))
                  root = i;
              }

            // The root is the smallest node of the cycle, not the first one the
            // walk met: the reference of the biases has to stay the same while the
            // cycle does, or comparing biases across components can oscillate.
            const Eta e = Ar::make_eta(total, length);
            const size_t root_node = path_(j + root);
            eta_(root_node) = e;
            bias_(root_node) = Ar::zero_bias();
            state_(root_node) = 2;
            Bias next = bias_(root_node);
            for (size_t k = 1; k < length; ++k)
              {
                const size_t v = path_(j + (root + length - k) % length);
                if (not Ar::step(adj_.weight(policy_(v)), e, next, bias_(v)))
                  return false;
                eta_(v) = e;
                state_(v) = 2;
                next = bias_(v);
              }
            top = j;
          }

        // What is left of the path hangs from an evaluated node.
        while (top > 0)
          {
            const size_t v = path_(--top);
            const size_t t = successor(v);
            eta_(v) = eta_(t);
            if (not Ar::step(adj_.weight(policy_(v)), eta_(v), bias_(t), bias_(v)))
              return false;
            state_(v) = 2;
          }
      }
    return true;
  }

  /// Improve the policy. Returns true if it changed; on overflow sets
  /// `overflow_` and returns false.
  bool improve()
  {
    bool changed = false;

    // First the mean: move to a successor with a smaller one.
    for (size_t u = 0; u < n_; ++u)
      {
        size_t best = policy_(u);
        size_t best_node = u;
        for (size_t p = adj_.start(u); p < adj_.start(u + 1); ++p)
          {
            const size_t v = adj_.tgt(p);
            if (Ar::less(eta_(v), eta_(best_node)))
              {
                best = p;
                best_node = v;
              }
          }
        if (best != policy_(u))
          {
            policy_(u) = best;
            changed = true;
          }
      }
    if (changed)
      return true;

    // No mean can be improved: improve the bias among equal means.
    for (size_t u = 0; u < n_; ++u)
      {
        size_t best = policy_(u);
        Bias best_bias = bias_(u);
        for (size_t p = adj_.start(u); p < adj_.start(u + 1); ++p)
          {
            const size_t v = adj_.tgt(p);
            if (not Ar::equal(eta_(v), eta_(u)))
              continue;

            Bias candidate = Ar::zero_bias();
            if (not Ar::step(adj_.weight(p), eta_(u), bias_(v), candidate))
              {
                overflow_ = true;
                return false;
              }
            if (Ar::improves(candidate, best_bias, eta_(u)))
              {
                best = p;
                best_bias = candidate;
              }
          }
        if (best != policy_(u))
          {
            policy_(u) = best;
            changed = true;
          }
      }
    return changed;
  }

public:
  /// How `solve()` ended.
  enum class Status
  {
    Optimal,          ///< No improvement exists: the policy is optimal.
    Iteration_Limit,  ///< The limit was reached first.
    Overflow          ///< A scaled value did not fit the arithmetic.
  };

  /** @param[in] adj Out-adjacency; every node must have an outgoing arc.
   *  @param[in] n Number of nodes.
   */
  Solver(const negative_cycles_detail::Out_Adjacency<Cost_Type> &adj, const size_t n)
    : adj_(adj), n_(n), policy_(n, 0), eta_(n, Eta{}), bias_(n, Bias{}), state_(n, 0), path_(n, 0)
  {
    // The initial policy takes the cheapest outgoing arc of each node.
    for (size_t u = 0; u < n; ++u)
      {
        size_t best = adj.start(u);
        for (size_t p = best + 1; p < adj.start(u + 1); ++p)
          if (adj.weight(p) < adj.weight(best))
            best = p;
        policy_(u) = best;
      }
  }

  /** @brief Iterate until the policy is optimal.
   *  @param[in] limit Maximum number of improvements.
   *  @param[out] iterations Improvements applied.
   *  @return How it ended.
   */
  Status solve(const size_t limit, size_t &iterations)
  {
    iterations = 0;
    for (;;)
      {
        if (not evaluate())
          return Status::Overflow;
        if (not improve())
          return overflow_ ? Status::Overflow : Status::Optimal;
        if (++iterations > limit)
          return Status::Iteration_Limit;
      }
  }

  /// A node whose mean is the smallest: the cycle it reaches is optimal.
  size_t best_node() const
  {
    size_t best = 0;
    for (size_t u = 1; u < n_; ++u)
      if (Ar::less(eta_(u), eta_(best)))
        best = u;
    return best;
  }

  /// Position in the adjacency of the arc the policy chooses for `u`.
  size_t chosen_arc(const size_t u) const
  {
    return policy_(u);
  }

  /// Node the policy leads to from `u`.
  size_t next_node(const size_t u) const
  {
    return successor(u);
  }
};

/// Iterations allowed beyond one per node before Karp takes over.
inline constexpr size_t iteration_slack = 64;

/// Value of `max_iterations` that selects the default limit, `V + 64`.
inline constexpr size_t default_iterations = std::numeric_limits<size_t>::max();

/** @brief Body of `howard_minimum_mean_cycle()` with an explicit iteration
 *         limit, so that the fallback to Karp can be exercised.
 *  @param[in] g Graph (must be directed).
 *  @param[in] distance Arc cost accessor.
 *  @param[in] sa Arc filter.
 *  @param[in] max_iterations Improvements allowed, or `default_iterations`.
 *  @return The result of `howard_minimum_mean_cycle()`.
 */
template <AlephGraph GT, ArcDistance<GT> Distance, ArcFilter<GT> SA>
Howard_Mean_Cycle_Result<GT, typename Distance::Distance_Type> minimum_mean_cycle(
  const GT &g, Distance distance, SA sa, const size_t max_iterations)
{
  using Cost_Type = typename Distance::Distance_Type;
  using Result = Howard_Mean_Cycle_Result<GT, Cost_Type>;
  using Solver = howard_detail::Solver<Cost_Type>;
  using Arithmetic =
    howard_detail::Arithmetic<Cost_Type, howard_detail::exact_arithmetic_v<Cost_Type>>;

  Result result;
  const auto snapshot =
    negative_cycles_detail::index_graph(g, distance, sa, "howard_minimum_mean_cycle");
  if (snapshot.arcs.is_empty())
    return result;

  // Only the arcs inside strongly connected components lie on cycles; in
  // the core every node has an outgoing arc, which a policy needs.
  negative_cycles_detail::Indexed_Graph<GT, Cost_Type> core;
  auto adj = negative_cycles_detail::build_out_adjacency(snapshot);
  const bool whole = negative_cycles_detail::cyclic_core(snapshot, adj, core);
  if (not whole)
    {
      if (core.arcs.is_empty())
        return result;  // no cycle at all
      adj = negative_cycles_detail::build_out_adjacency(core);
    }
  const auto &ig = whole ? snapshot : core;
  const size_t n = ig.nodes.size();

  Solver solver(adj, n);
  size_t iterations = 0;
  const auto status = solver.solve(
    max_iterations == default_iterations ? n + iteration_slack : max_iterations, iterations);

  const auto use_karp = [&](const size_t done)
    {
      Result karp;
      static_cast<Min_Mean_Cycle_Result<GT, Cost_Type> &>(karp) =
        karp_minimum_mean_cycle<GT, Distance, SA>(g, distance, sa);
      karp.iterations = done;
      karp.used_karp = true;
      return karp;
    };

  if (status != Solver::Status::Optimal)
    return use_karp(iterations);

  // The cycle the optimal policy reaches from a node of minimum mean.
  Array<size_t> visit(n, 0);
  for (size_t i = 0; i < n; ++i)
    visit(i) = 0;
  size_t v = solver.best_node();
  for (size_t step = 1; visit(v) == 0; ++step)
    {
      visit(v) = step;
      v = solver.next_node(v);
    }
  const size_t first = v;  // a node of the cycle

  Array<size_t> arcs;  // indices into ig.arcs, in cycle order
  result.cycle_nodes.append(ig.nodes(first));
  size_t u = first;
  do
    {
      const size_t a = adj.arc(solver.chosen_arc(u));
      arcs.append(a);
      result.cycle_arcs.append(ig.arcs(a).arc);
      u = solver.next_node(u);
      result.cycle_nodes.append(ig.nodes(u));
  } while (u != first);

  result.has_cycle = true;
  result.witness_node = ig.nodes(first);
  result.cycle_length = arcs.size();
  if constexpr (howard_detail::exact_arithmetic_v<Cost_Type>)
    {
      // The wide total is exact; it has to be representable in the cost type.
      // (A sum with a check at every step would reject totals that fit but
      // whose partial sums do not.)
      auto total = Arithmetic::zero_sum();
      bool fits = true;
      for (size_t i = 0; fits and i < arcs.size(); ++i)
        fits = Arithmetic::add(total, ig.arcs(arcs(i)).weight);
      if (fits)
        fits = total >= static_cast<long long>(std::numeric_limits<Cost_Type>::min()) and
          total <= static_cast<long long>(std::numeric_limits<Cost_Type>::max());
      if (not fits)
        return use_karp(iterations);
      result.cycle_total_cost = static_cast<Cost_Type>(total);
    }
  else
    result.cycle_total_cost = negative_cycles_detail::cycle_cost(
      ig, arcs, "howard_minimum_mean_cycle(): cycle cost became non-finite");
  result.minimum_mean = static_cast<long double>(result.cycle_total_cost) /
    static_cast<long double>(result.cycle_length);
  result.iterations = iterations;
  return result;
}
}  // namespace howard_detail

/** @brief Minimum mean cycle by Howard's policy iteration.
 *
 *  See the file documentation for the algorithm, its complexity and its
 *  arithmetic.
 *
 *  - If the digraph has no directed cycle, `has_cycle = false`.
 *  - Otherwise `minimum_mean` is the minimum cycle mean and the witness is a
 *    simple cycle attaining it, with its exact total cost and length.
 *
 *  The witness can differ from the one of `karp_minimum_mean_cycle()` when
 *  several cycles attain the minimum; the value is the same.
 *
 *  @tparam GT Directed Aleph graph type.
 *  @tparam Distance Arc cost accessor (`Distance::Distance_Type` required).
 *  @tparam SA Arc filter; arcs it rejects do not exist.
 *
 *  @param[in] g Graph (must be directed). It is not modified.
 *  @param[in] distance Arc cost accessor.
 *  @param[in] sa Arc filter.
 *
 *  @return A `Howard_Mean_Cycle_Result`. `iterations` counts the policy
 *          improvements; `used_karp` is true if the iteration limit
 *          (`V + 64`) or the range of the scaled integer bias was exceeded and
 *          Karp's algorithm computed the answer (it then calls the distance
 *          accessor again).
 *
 *  @throws std::domain_error If `g` is undirected, or a floating weight (or
 *          an accumulated cost) is not finite.
 *  @throws std::overflow_error If integer additions overflow.
 *
 *  @note Complexity: `O(iterations * (V + E))` time and `O(V + E)` space, plus
 *        `O((V + E) log V)` for the snapshot of the graph; the
 *        distance accessor is called once per arc.
 *
 *  @see karp_minimum_mean_cycle() The dynamic-programming alternative, with a
 *       worst-case bound and `O(V^2)` space.
 *
 *  @ingroup Graphs
 */
template <AlephGraph GT, ArcDistance<GT> Distance = Dft_Dist<GT>, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
Howard_Mean_Cycle_Result<GT, typename Distance::Distance_Type> howard_minimum_mean_cycle(
  const GT &g, Distance distance = Distance(), SA sa = SA())
{
  return howard_detail::minimum_mean_cycle<GT, Distance, SA>(g, std::move(distance), std::move(sa),
                                                             howard_detail::default_iterations);
}

/** @brief Minimum mean cycle value by Howard's policy iteration, without the
 *         witness.
 *
 *  Same algorithm and requirements as `howard_minimum_mean_cycle()`.
 *
 *  @return The result of `karp_minimum_mean_cycle_value()`: whether a cycle
 *          exists and the minimum mean.
 *
 *  @ingroup Graphs
 */
template <AlephGraph GT, ArcDistance<GT> Distance = Dft_Dist<GT>, ArcFilter<GT> SA = Dft_Show_Arc<GT>>
Min_Mean_Cycle_Value_Result howard_minimum_mean_cycle_value(const GT &g,
                                                            Distance distance = Distance(),
                                                            SA sa = SA())
{
  const auto r = howard_minimum_mean_cycle<GT, Distance, SA>(g, std::move(distance), std::move(sa));
  Min_Mean_Cycle_Value_Result value;
  value.has_cycle = r.has_cycle;
  value.minimum_mean = r.minimum_mean;
  return value;
}
}  // namespace Aleph

#endif  // HOWARD_MIN_MEAN_CYCLE_H
