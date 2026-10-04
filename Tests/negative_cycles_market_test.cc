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

/**
 * @file negative_cycles_market_test.cc
 * @brief Real Binance quotes through the cycle searches, against an exact oracle.
 *
 * The fixture (`negative_cycles_market_snapshots.H`) holds eighteen states of a
 * six-asset market taken from a 24-hour capture. Each one becomes a digraph
 * whose arcs weigh `-ln(rate) - ln(1 - fee)`, for four fees, with and without
 * hiding the quotes that are stale or whose coverage is not confirmed (the arc
 * filter an application would use). The oracle enumerates every simple cycle of
 * the visible arcs (409 in the complete market) and adds the double weights as
 * exact rationals, so it shares nothing with the library. It is compared with
 * `most_negative_cycle_up_to_3()`, `most_negative_cycle_bounded()`,
 * `find_disjoint_negative_cycles()`, `howard_minimum_mean_cycle()` and
 * `karp_minimum_mean_cycle()`. The sign of the best cycle is also checked
 * against the money it moves, with integer arithmetic on the quoted prices.
 */

# include <gtest/gtest.h>
# include <gmp.h>

# include <algorithm>
# include <cmath>
# include <cstddef>
# include <functional>
# include <limits>
# include <set>
# include <string>
# include <vector>

# include <Howard_Min_Mean_Cycle.H>
# include <Min_Mean_Cycle.H>
# include <Negative_Cycles.H>
# include <tpl_graph.H>

# include "negative_cycles_market_snapshots.H"

using namespace Aleph;
using namespace Negative_Cycles_Market_Data;

namespace
{
  using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
  using Distance = Dft_Dist<Graph>;
  constexpr int A = asset_count;

  /// A quote older than this, or one whose coverage is not confirmed, is hidden
  /// in the "fresh" mode.
  constexpr long long max_age_ms = 1000;

  /// Below this magnitude a total is too close to zero for a sign to be required.
  constexpr double clear_margin = 1e-9;

  struct Fee
  {
    const char * name;
    long num;
    long den;   // the fee is num / den per leg
  };

  const Fee fees[] = {{"0", 0, 1}, {"1bp", 1, 10000}, {"0.075%", 75, 100000}, {"0.1%", 1, 1000}};
  constexpr size_t fee_075 = 2;
  constexpr size_t fee_10 = 3;

  // ------------------------------------------------------------------------
  // Exact rationals over the GMP C interface.
  // ------------------------------------------------------------------------
  class Rational
  {
    mpq_t q_;

  public:
    Rational() { mpq_init(q_); }
    explicit Rational(const double x) { mpq_init(q_); mpq_set_d(q_, x); }   // exact
    Rational(const Rational & other) { mpq_init(q_); mpq_set(q_, other.q_); }
    Rational & operator=(const Rational & other) { mpq_set(q_, other.q_); return *this; }
    ~Rational() { mpq_clear(q_); }

    Rational & operator+=(const Rational & other) { mpq_add(q_, q_, other.q_); return *this; }

    friend Rational operator-(const Rational & a, const Rational & b)
    {
      Rational r;
      mpq_sub(r.q_, a.q_, b.q_);
      return r;
    }

    Rational times(const size_t factor) const
    {
      Rational f;
      mpq_set_ui(f.q_, static_cast<unsigned long>(factor), 1);
      Rational r;
      mpq_mul(r.q_, q_, f.q_);
      return r;
    }

    Rational divided_by(const size_t divisor) const
    {
      Rational d;
      mpq_set_ui(d.q_, static_cast<unsigned long>(divisor), 1);
      Rational r;
      mpq_div(r.q_, q_, d.q_);
      return r;
    }

    friend int compare(const Rational & a, const Rational & b) { return mpq_cmp(a.q_, b.q_); }
    int sign() const { return mpq_sgn(q_); }
    double to_double() const { return mpq_get_d(q_); }
  };

  // a / la < b / lb, with positive lengths.
  bool mean_less(const Rational & a, const size_t la, const Rational & b, const size_t lb)
  {
    return compare(a.times(lb), b.times(la)) < 0;
  }

  bool mean_equal(const Rational & a, const size_t la, const Rational & b, const size_t lb)
  {
    return compare(a.times(lb), b.times(la)) == 0;
  }

  // True if `computed` is within one unit in the last place of `exact`, with
  // the same sign (zero only for zero).
  bool within_one_ulp(const double computed, const Rational & exact)
  {
    if (computed == 0.0)
      return exact.sign() == 0;
    if ((computed < 0.0 ? -1 : 1) != exact.sign())
      return false;
    const double up = std::nextafter(computed, std::numeric_limits<double>::infinity());
    const double down = std::nextafter(computed, -std::numeric_limits<double>::infinity());
    return compare(Rational(down), exact) <= 0 and compare(exact, Rational(up)) <= 0;
  }

  // ------------------------------------------------------------------------
  // The market of one snapshot, as a digraph.
  // ------------------------------------------------------------------------
  struct Hide_Set
  {
    const std::set<Graph::Arc *> * hidden = nullptr;

    bool operator()(Graph::Arc * arc) const
    {
      return hidden == nullptr or hidden->count(arc) == 0;
    }
  };

  struct Market
  {
    Graph g;
    Graph::Node * node[A] = {};
    Graph::Arc * arc[A][A] = {};
    double weight[A][A] = {};
    Rational exact[A][A];                    // the weights, as exact rationals
    bool visible[A][A] = {};
    const Market_Quote * quote[A][A] = {};   // the quote that prices each arc
    std::set<Graph::Arc *> hidden;

    /// Build the market of `snapshot` with `fee` charged on every leg. With
    /// `fresh_only`, the pairs whose quote is stale or unobserved are hidden.
    Market(const Market_Snapshot & snapshot, const Fee & fee, const bool fresh_only)
    {
      for (int i = 0; i < A; ++i)
        node[i] = g.insert_node(i);

      const double fee_term = -std::log1p(-static_cast<double>(fee.num) / static_cast<double>(fee.den));
      const double ln10 = std::log(10.0);
      for (const auto & q : snapshot.quotes)
        {
          // Selling the base takes the bid, buying it takes the ask.
          weight[q.base][q.quote] = -(std::log(static_cast<double>(q.bid)) - q.exponent * ln10) + fee_term;
          weight[q.quote][q.base] = -(q.exponent * ln10 - std::log(static_cast<double>(q.ask))) + fee_term;
          const bool shown = not fresh_only or (q.age_ms <= max_age_ms and q.observed != 0);
          visible[q.base][q.quote] = visible[q.quote][q.base] = shown;
          quote[q.base][q.quote] = quote[q.quote][q.base] = &q;
        }

      for (int u = 0; u < A; ++u)
        for (int v = 0; v < A; ++v)
          if (u != v)
            {
              arc[u][v] = g.insert_arc(node[u], node[v], weight[u][v]);
              exact[u][v] = Rational(weight[u][v]);
              if (not visible[u][v])
                hidden.insert(arc[u][v]);
            }
    }
  };

  // ------------------------------------------------------------------------
  // Exhaustive oracle over the simple cycles of the visible arcs.
  // ------------------------------------------------------------------------
  struct Oracle
  {
    size_t cycles = 0;                 // simple cycles of the visible arcs
    size_t by_length[A + 1] = {};      // ... of each length
    bool any_within[A + 1] = {};       // a cycle of at most L arcs exists
    Rational best_total[A + 1];        // minimum total among cycles of at most L arcs
    size_t best_length[A + 1] = {};    // fewest arcs among the minimizers
    std::vector<int> best_nodes;       // a minimizer over every length
    bool any = false;
    Rational mean_total;               // the minimum mean is mean_total / mean_length
    size_t mean_length = 1;
  };

  Oracle enumerate(const Market & m)
  {
    Oracle o;
    std::vector<int> path;
    bool on_path[A] = {};

    const auto record = [&](const Rational & total)
    {
      const size_t len = path.size();   // a cycle has as many arcs as nodes
      ++o.cycles;
      ++o.by_length[len];
      for (size_t bound = len; bound <= A; ++bound)
        if (not o.any_within[bound] or compare(total, o.best_total[bound]) < 0
            or (compare(total, o.best_total[bound]) == 0 and len < o.best_length[bound]))
          {
            o.any_within[bound] = true;
            o.best_total[bound] = total;
            o.best_length[bound] = len;
            if (bound == A)
              o.best_nodes = path;
          }
      if (not o.any or mean_less(total, len, o.mean_total, o.mean_length))
        {
          o.any = true;
          o.mean_total = total;
          o.mean_length = len;
        }
    };

    std::function<void(int, int, const Rational &)> dfs =
      [&](const int start, const int u, const Rational & total)
    {
      for (int v = 0; v < A; ++v)
        {
          if (v == u or not m.visible[u][v])
            continue;
          Rational t = total;
          t += m.exact[u][v];
          if (v == start)
            {
              record(t);
              continue;
            }
          if (v < start or on_path[v])
            continue;
          on_path[v] = true;
          path.push_back(v);
          dfs(start, v, t);
          path.pop_back();
          on_path[v] = false;
        }
    };

    for (int s = 0; s < A; ++s)
      {
        on_path[s] = true;
        path.assign(1, s);
        dfs(s, s, Rational());
        on_path[s] = false;
      }
    return o;
  }

  // The exact total of a witness given as a list of arcs, which must be
  // consecutive, visible, simple and closed.
  bool exact_cycle(const Market & m, const DynList<Graph::Arc *> & arcs, Rational & total,
                   size_t & length)
  {
    std::set<Graph::Node *> seen;
    Graph::Node * first = nullptr;
    Graph::Node * current = nullptr;
    length = 0;
    total = Rational();
    for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        if (m.hidden.count(arc) != 0)
          return false;
        auto * src = m.g.get_src_node(arc);
        if (first == nullptr)
          first = current = src;
        if (src != current or not seen.insert(src).second)
          return false;
        current = m.g.get_tgt_node(arc);
        total += Rational(arc->get_info());
        ++length;
      }
    return length > 0 and current == first;
  }

  // Sign of the profit of going around `nodes` at the quoted prices with `fee`
  // charged on every leg, in integers: 1 if the money grows, -1 if it shrinks.
  // Selling the base takes the bid; buying it takes the ask.
  int exact_profit_sign(const Market & m, const std::vector<int> & nodes, const Fee & fee)
  {
    mpz_t num, den, scale;
    mpz_inits(num, den, scale, nullptr);
    mpz_set_ui(num, 1);
    mpz_set_ui(den, 1);
    for (size_t i = 0; i < nodes.size(); ++i)
      {
        const int u = nodes[i];
        const int v = nodes[(i + 1) % nodes.size()];
        const Market_Quote & q = *m.quote[u][v];
        mpz_ui_pow_ui(scale, 10, static_cast<unsigned long>(q.exponent));
        if (u == q.base)
          {
            mpz_mul_si(num, num, q.bid);
            mpz_mul(den, den, scale);
          }
        else
          {
            mpz_mul(num, num, scale);
            mpz_mul_si(den, den, q.ask);
          }
        mpz_mul_si(num, num, fee.den - fee.num);
        mpz_mul_si(den, den, fee.den);
      }
    const int order = mpz_cmp(num, den);
    mpz_clears(num, den, scale, nullptr);
    return order > 0 ? 1 : order < 0 ? -1 : 0;
  }

  std::string describe(const Market_Snapshot & s, const Fee & fee, const bool fresh_only)
  {
    return std::string("snapshot=") + s.label + " fee=" + fee.name + (fresh_only ? " fresh quotes only" : " every quote");
  }

  const Market_Snapshot & find_snapshot(const char * label)
  {
    for (const auto & s : snapshots)
      if (std::string(s.label) == label)
        return s;
    ADD_FAILURE() << "no snapshot labelled " << label;
    return snapshots[0];
  }

  /** @brief `most_negative_cycle_up_to_3()` must return the exact optimum of
   *         its bound, with the fewest arcs among the minimizers. */
  void check_short(const Market & m, const Oracle & o)
  {
    for (size_t bound = 1; bound <= 3; ++bound)
      {
        SCOPED_TRACE(::testing::Message() << "up_to_3 bound=" << bound);
        const auto r = most_negative_cycle_up_to_3<Graph, Distance, Hide_Set>(
            m.g, bound, Distance(), Hide_Set{&m.hidden});
        ASSERT_EQ(r.has_cycle, o.any_within[bound]);
        if (not r.has_cycle)
          continue;
        Rational total;
        size_t length = 0;
        ASSERT_TRUE(exact_cycle(m, r.cycle_arcs, total, length));
        ASSERT_EQ(length, r.length);
        ASSERT_EQ(length, o.best_length[bound]);
        ASSERT_EQ(compare(total, o.best_total[bound]), 0);
        ASSERT_TRUE(within_one_ulp(r.total_cost, total));
        ASSERT_EQ(r.is_negative(), total.sign() < 0);
      }
  }

  /** @brief `most_negative_cycle_bounded()` returns a simple cycle of at most
   *         `L` arcs whose gap covers its distance to the exact optimum. It
   *         need not be the optimum (the bound is a relaxation), but it must
   *         be negative whenever a clearly negative cycle exists. */
  void check_bounded(const Market & m, const Oracle & o)
  {
    for (const size_t bound : {size_t(3), size_t(4), size_t(6)})
      {
        SCOPED_TRACE(::testing::Message() << "bounded L=" << bound);
        const auto r = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
            m.g, bound, Distance(), Hide_Set{&m.hidden});
        ASSERT_EQ(r.has_cycle, o.any_within[bound]);
        if (not r.has_cycle)
          continue;
        Rational total;
        size_t length = 0;
        ASSERT_TRUE(exact_cycle(m, r.cycle_arcs, total, length));
        ASSERT_EQ(length, r.length);
        ASSERT_LE(length, bound);
        ASSERT_TRUE(within_one_ulp(r.total_cost, total));
        ASSERT_EQ(r.is_negative(), total.sign() < 0);
        ASSERT_GE(compare(total, o.best_total[bound]), 0);   // no cycle beats the optimum

        // The gap is rigorous: the optimum is no cheaper than total - gap.
        ASSERT_GE(r.optimality_gap, 0.0);
        ASSERT_LE(compare(total - o.best_total[bound], Rational(r.optimality_gap)), 0);
        if (r.is_exact)
          {
            ASSERT_EQ(compare(total, o.best_total[bound]), 0);
          }
        if (o.best_total[bound].to_double() < -clear_margin)
          {
            ASSERT_TRUE(r.is_negative());
          }
      }
  }

  /** @brief `find_disjoint_negative_cycles()` reports only exactly negative,
   *         simple, arc-disjoint cycles, and some when a clearly negative one exists. */
  void check_disjoint(const Market & m, const Oracle & o)
  {
    for (const size_t wanted : {size_t(1), size_t(3)})
      {
        SCOPED_TRACE(::testing::Message() << "find_disjoint wanted=" << wanted);
        const auto cycles = find_disjoint_negative_cycles<Graph, Distance, Hide_Set>(
            m.g, wanted, Negative_Cycle_Exclusion::All_Arcs, Distance(), Hide_Set{&m.hidden});
        ASSERT_LE(cycles.size(), wanted);
        std::set<Graph::Arc *> used;
        for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
          {
            const auto & item = it.get_curr();
            const auto arcs = item.cycle.arcs();   // a list by value: keep it alive while iterating
            Rational total;
            size_t length = 0;
            ASSERT_TRUE(exact_cycle(m, arcs, total, length));
            ASSERT_EQ(length, item.length);
            ASSERT_LT(total.sign(), 0);   // exactly negative
            ASSERT_TRUE(within_one_ulp(item.total_cost, total));
            for (auto ai = arcs.get_it(); ai.has_curr(); ai.next_ne())
              ASSERT_TRUE(used.insert(ai.get_curr()).second);
          }
        if (not o.any_within[A])
          {
            ASSERT_TRUE(cycles.is_empty());
            continue;
          }
        const double best = o.best_total[A].to_double();
        if (best < -clear_margin)
          {
            ASSERT_FALSE(cycles.is_empty());
          }
        else if (best > clear_margin)
          {
            ASSERT_TRUE(cycles.is_empty());
          }
      }
  }

  /** @brief The minimum mean cycle of Howard and Karp against the exact one. */
  void check_means(const Market & m, const Oracle & o)
  {
    const auto h = howard_minimum_mean_cycle<Graph, Distance, Hide_Set>(
        m.g, Distance(), Hide_Set{&m.hidden});
    const auto k = karp_minimum_mean_cycle<Graph, Distance, Hide_Set>(
        m.g, Distance(), Hide_Set{&m.hidden});
    ASSERT_EQ(h.has_cycle, o.any);
    ASSERT_EQ(k.has_cycle, o.any);
    if (not o.any)
      return;

    const double mean = o.mean_total.divided_by(o.mean_length).to_double();
    Rational total;
    size_t length = 0;
    ASSERT_TRUE(exact_cycle(m, h.cycle_arcs, total, length)) << "Howard";
    ASSERT_EQ(length, h.cycle_length);
    ASSERT_TRUE(within_one_ulp(h.cycle_total_cost, total));
    if (not h.used_karp and h.numeric_quality == Cycle_Numeric_Quality::Exact)
      {
        ASSERT_TRUE(mean_equal(total, length, o.mean_total, o.mean_length)) << "Howard is not exact";
      }
    if (std::fabs(mean) > clear_margin)
      {
        ASSERT_EQ(h.minimum_mean < 0, mean < 0) << "Howard sign";
      }

    ASSERT_TRUE(exact_cycle(m, k.cycle_arcs, total, length)) << "Karp";
    ASSERT_EQ(length, k.cycle_length);
    ASSERT_LE(std::fabs(static_cast<double>(k.minimum_mean) - mean), 1e-12) << "Karp mean";
    if (std::fabs(mean) > clear_margin)
      {
        ASSERT_EQ(k.minimum_mean < 0, mean < 0) << "Karp sign";
      }
  }

  /** @brief The sign of the best cycle, which comes from `-ln` weights, is the
   *         sign of the money the cycle moves, computed in integers. */
  void check_money(const Market & m, const Oracle & o, const Fee & fee)
  {
    if (not o.any_within[A])
      return;
    const double best = o.best_total[A].to_double();
    const int profit = exact_profit_sign(m, o.best_nodes, fee);
    if (best < -clear_margin)
      {
        ASSERT_EQ(profit, 1) << "a negative total must be a profit";
      }
    else if (best > clear_margin)
      {
        ASSERT_EQ(profit, -1) << "a positive total must be a loss";
      }
  }

  /** @brief Every check on one market. */
  void check_market(const Market_Snapshot & s, const Fee & fee, const bool fresh_only)
  {
    SCOPED_TRACE(describe(s, fee, fresh_only));
    const Market m(s, fee, fresh_only);
    const Oracle o = enumerate(m);
    check_short(m, o);
    if (::testing::Test::HasFatalFailure())
      return;
    check_bounded(m, o);
    if (::testing::Test::HasFatalFailure())
      return;
    check_disjoint(m, o);
    if (::testing::Test::HasFatalFailure())
      return;
    check_means(m, o);
    if (::testing::Test::HasFatalFailure())
      return;
    check_money(m, o, fee);
  }
} // namespace


TEST(NegativeCyclesMarketTest, TheOracleCountsEveryCycleOfTheCompleteMarket)
{
  // 15 pairs of nodes, 20 triples with 2 orientations, 15 quadruples with 6
  // cyclic orders, 6 quintuples with 24 and 1 sextuple with 120: 409 cycles.
  const Market m(snapshots[0], fees[0], false);
  const Oracle o = enumerate(m);
  EXPECT_EQ(o.cycles, 409u);
  EXPECT_EQ(o.by_length[1], 0u);
  EXPECT_EQ(o.by_length[2], 15u);
  EXPECT_EQ(o.by_length[3], 40u);
  EXPECT_EQ(o.by_length[4], 90u);
  EXPECT_EQ(o.by_length[5], 144u);
  EXPECT_EQ(o.by_length[6], 120u);
}


TEST(NegativeCyclesMarketTest, RealQuotesAgreeWithTheExhaustiveOracle)
{
  // Every snapshot, four fees, with every quote and with the stale or
  // unobserved ones hidden through the arc filter: 144 markets, some of them
  // without a single visible arc.
  for (const auto & s : snapshots)
    for (const auto & fee : fees)
      for (const bool fresh_only : {false, true})
        {
          check_market(s, fee, fresh_only);
          if (::testing::Test::HasFatalFailure())
            return;
        }
}


TEST(NegativeCyclesMarketTest, ARaisedFeeNeverImprovesTheBestCycle)
{
  // Every arc weighs more with a higher fee, and rounding is monotone, so the
  // exact optimum of every bound can only rise.
  for (const auto & s : snapshots)
    for (const bool fresh_only : {false, true})
      {
        SCOPED_TRACE(std::string("snapshot=") + s.label + (fresh_only ? " fresh" : " all"));
        Rational previous[4];
        bool had[4] = {};
        for (const auto & fee : fees)
          {
            const Market m(s, fee, fresh_only);
            for (size_t bound = 2; bound <= 3; ++bound)
              {
                const auto r = most_negative_cycle_up_to_3<Graph, Distance, Hide_Set>(
                    m.g, bound, Distance(), Hide_Set{&m.hidden});
                if (not r.has_cycle)
                  continue;
                Rational total;
                size_t length = 0;
                ASSERT_TRUE(exact_cycle(m, r.cycle_arcs, total, length));
                if (had[bound])
                  {
                    ASSERT_GE(compare(total, previous[bound]), 0) << "bound " << bound << " fee " << fee.name;
                  }
                previous[bound] = total;
                had[bound] = true;
              }
          }
      }
}


TEST(NegativeCyclesMarketTest, QuietStatesOnlyOfferTheRoundTripThatCostsTwoFees)
{
  // Away from the burst nothing is profitable after fees: the cheapest cycle
  // is going back and forth on the tightest pair, which costs the fee twice.
  for (const auto & s : snapshots)
    {
      const std::string label = s.label;
      const bool quiet = label.rfind("typical-", 0) == 0 or label == "first" or label == "len3"
                         or label == "len5" or label == "len6";
      if (not quiet)
        continue;
      for (const size_t f : {fee_075, fee_10})
        {
          SCOPED_TRACE(describe(s, fees[f], false));
          const Market m(s, fees[f], false);
          const Oracle o = enumerate(m);
          const double two_fees = 2 * -std::log1p(-static_cast<double>(fees[f].num) / fees[f].den);
          ASSERT_TRUE(o.any_within[A]);
          EXPECT_EQ(o.best_length[A], 2u);
          const double best = o.best_total[A].to_double();
          EXPECT_GE(best, two_fees - 1e-12);
          EXPECT_LE(best, two_fees + 1e-4);   // the spread of the tightest pair

          const auto r = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
              m.g, A, Distance(), Hide_Set{&m.hidden});
          EXPECT_FALSE(r.is_negative());
          const auto reported = find_disjoint_negative_cycles<Graph, Distance, Hide_Set>(
              m.g, 1, Negative_Cycle_Exclusion::All_Arcs, Distance(), Hide_Set{&m.hidden});
          EXPECT_TRUE(reported.is_empty());
        }
    }
}


TEST(NegativeCyclesMarketTest, TheFreshBurstReproducesTheEpisodesOfTheDay)
{
  // The three episodes of the capture in which every leg was at most one
  // second old and its coverage observed. With a fee of 0.075 % they gain 22.67,
  // 23.77 and 45.15 basis points; with 0.1 %, 15.17, 16.27 and 35.14.
  struct Expected
  {
    const char * label;
    size_t fee;
    size_t legs;
    double total;
  };
  const Expected expected[] = {
    {"burst-1", fee_075, 3, -22.6732e-4}, {"burst-1", fee_10, 3, -15.1667e-4},
    {"burst-2", fee_075, 3, -23.7742e-4}, {"burst-2", fee_10, 3, -16.2677e-4},
    {"burst-3", fee_075, 4, -45.1460e-4}, {"burst-3", fee_10, 4, -35.1372e-4},
  };
  for (const auto & e : expected)
    {
      SCOPED_TRACE(describe(find_snapshot(e.label), fees[e.fee], true));
      const Market m(find_snapshot(e.label), fees[e.fee], true);
      const Oracle o = enumerate(m);
      ASSERT_TRUE(o.any_within[A]);
      EXPECT_EQ(o.best_length[A], e.legs);
      EXPECT_NEAR(o.best_total[A].to_double(), e.total, 1e-7);

      const auto found = find_disjoint_negative_cycles<Graph, Distance, Hide_Set>(
          m.g, 1, Negative_Cycle_Exclusion::All_Arcs, Distance(), Hide_Set{&m.hidden});
      EXPECT_FALSE(found.is_empty());
    }
}


TEST(NegativeCyclesMarketTest, TrianglesUnderestimateTheFourLegOpportunityOfTheBurst)
{
  // The best fresh cycle of the last episode has four legs and gains 45.15 bp;
  // the best triangle gains 23.79. The bounded search with L = 4 finds it and
  // certifies it with a gap of rounding size, while L = 6 cannot certify
  // anything on this market (its gap is 2.28 bp).
  const Market m(find_snapshot("burst-3"), fees[fee_075], true);
  const Oracle o = enumerate(m);
  ASSERT_TRUE(o.any_within[A]);
  ASSERT_EQ(o.best_length[A], 4u);

  const auto triangle = most_negative_cycle_up_to_3<Graph, Distance, Hide_Set>(
      m.g, 3, Distance(), Hide_Set{&m.hidden});
  ASSERT_TRUE(triangle.has_cycle);
  EXPECT_EQ(triangle.length, 3u);
  EXPECT_TRUE(triangle.is_negative());
  EXPECT_NEAR(triangle.total_cost, -23.7934e-4, 1e-7);
  EXPECT_GT(triangle.total_cost, o.best_total[A].to_double() + 1e-3);   // 21 bp worse

  const auto four = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
      m.g, 4, Distance(), Hide_Set{&m.hidden});
  ASSERT_TRUE(four.has_cycle);
  EXPECT_EQ(four.length, 4u);
  Rational total;
  size_t length = 0;
  ASSERT_TRUE(exact_cycle(m, four.cycle_arcs, total, length));
  EXPECT_EQ(compare(total, o.best_total[A]), 0);
  EXPECT_LT(four.optimality_gap, 1e-12);

  const auto six = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
      m.g, 6, Distance(), Hide_Set{&m.hidden});
  EXPECT_FALSE(six.is_exact);
  EXPECT_GT(six.optimality_gap, 1e-4);
}


TEST(NegativeCyclesMarketTest, StaleQuotesOverstateTheBurstAndFakeTheBiggestProfitOfTheDay)
{
  // Without the freshness filter the last episode looks like a five-leg cycle
  // of 56.65 bp, 11 bp better than the real one, which uses quotes up to 21
  // seconds old. The largest profit of the day, 135 bp, is made only of stale
  // or unobserved quotes: it vanishes once they are hidden.
  {
    const Market all(find_snapshot("burst-3"), fees[fee_075], false);
    const Market fresh(find_snapshot("burst-3"), fees[fee_075], true);
    const Oracle o_all = enumerate(all);
    const Oracle o_fresh = enumerate(fresh);
    ASSERT_TRUE(o_all.any_within[A]);
    ASSERT_TRUE(o_fresh.any_within[A]);
    EXPECT_EQ(o_all.best_length[A], 5u);
    EXPECT_NEAR(o_all.best_total[A].to_double(), -56.6517e-4, 1e-7);
    EXPECT_LT(compare(o_all.best_total[A], o_fresh.best_total[A]), 0);

    bool uses_stale_quote = false;
    for (size_t i = 0; i < o_all.best_nodes.size(); ++i)
      {
        const int u = o_all.best_nodes[i];
        const int v = o_all.best_nodes[(i + 1) % o_all.best_nodes.size()];
        uses_stale_quote = uses_stale_quote or not fresh.visible[u][v];
      }
    EXPECT_TRUE(uses_stale_quote);
  }

  const Market all(find_snapshot("raw-peak"), fees[fee_075], false);
  const Market fresh(find_snapshot("raw-peak"), fees[fee_075], true);
  const Oracle o_all = enumerate(all);
  ASSERT_TRUE(o_all.any_within[A]);
  EXPECT_EQ(o_all.best_length[A], 5u);
  EXPECT_NEAR(o_all.best_total[A].to_double(), -135.175e-4, 1e-6);
  EXPECT_EQ(exact_profit_sign(all, o_all.best_nodes, fees[fee_075]), 1);   // real at those prices

  EXPECT_FALSE(enumerate(fresh).any);
  const auto found = find_disjoint_negative_cycles<Graph, Distance, Hide_Set>(
      fresh.g, 1, Negative_Cycle_Exclusion::All_Arcs, Distance(), Hide_Set{&fresh.hidden});
  EXPECT_TRUE(found.is_empty());
}


TEST(NegativeCyclesMarketTest, TheRelaxedBoundLeavesAGapOnARealState)
{
  // A state where the best cycle of at most six legs has six (-6.25 bp) and the
  // bounded search with L = 6, a relaxed problem, returns a triangle of -5.82
  // bp instead. Its gap, 5.17 bp, covers the 0.43 bp it leaves on the table, and
  // `most_negative_cycle_up_to_3()` agrees with it on the triangle.
  const Market m(find_snapshot("bounded-miss"), fees[fee_075], false);
  const Oracle o = enumerate(m);
  ASSERT_TRUE(o.any_within[A]);
  EXPECT_EQ(o.best_length[A], 6u);
  EXPECT_NEAR(o.best_total[A].to_double(), -6.2466e-4, 1e-7);

  const auto r = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
      m.g, 6, Distance(), Hide_Set{&m.hidden});
  ASSERT_TRUE(r.has_cycle);
  EXPECT_TRUE(r.is_negative());
  EXPECT_FALSE(r.is_exact);
  Rational total;
  size_t length = 0;
  ASSERT_TRUE(exact_cycle(m, r.cycle_arcs, total, length));
  EXPECT_GT(compare(total, o.best_total[A]), 0);   // not the optimum
  EXPECT_GE(r.optimality_gap, (total - o.best_total[A]).to_double());

  const auto triangle = most_negative_cycle_up_to_3<Graph, Distance, Hide_Set>(
      m.g, 3, Distance(), Hide_Set{&m.hidden});
  ASSERT_TRUE(triangle.has_cycle);
  EXPECT_NEAR(triangle.total_cost, -5.81819e-4, 1e-7);
}


TEST(NegativeCyclesMarketTest, TheNearBreakEvenStateIsAnUnprofitableTriangle)
{
  // A state whose best cycle with a fee of 0.1 % is a triangle that loses
  // 0.0034 bp, so no search reports a profit and the integer arithmetic on the
  // prices agrees. With a fee of 0.075 % the same state gains 7.5 bp.
  const Market m(find_snapshot("near-zero"), fees[fee_10], false);
  const Oracle o = enumerate(m);
  ASSERT_TRUE(o.any_within[A]);
  EXPECT_EQ(o.best_length[A], 3u);
  EXPECT_GT(o.best_total[A].to_double(), 0.0);
  EXPECT_LT(o.best_total[A].to_double(), 1e-6);
  EXPECT_EQ(exact_profit_sign(m, o.best_nodes, fees[fee_10]), -1);

  const auto r = most_negative_cycle_bounded<Graph, Distance, Hide_Set>(
      m.g, 4, Distance(), Hide_Set{&m.hidden});
  EXPECT_FALSE(r.is_negative());
  const auto found = find_disjoint_negative_cycles<Graph, Distance, Hide_Set>(
      m.g, 1, Negative_Cycle_Exclusion::All_Arcs, Distance(), Hide_Set{&m.hidden});
  EXPECT_TRUE(found.is_empty());

  const Market cheaper(find_snapshot("near-zero"), fees[fee_075], false);
  const auto gain = most_negative_cycle_up_to_3<Graph, Distance, Hide_Set>(
      cheaper.g, 3, Distance(), Hide_Set{&cheaper.hidden});
  EXPECT_TRUE(gain.is_negative());
  EXPECT_NEAR(gain.total_cost, -7.50313e-4, 1e-7);
}
