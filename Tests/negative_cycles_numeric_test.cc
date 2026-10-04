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
 * @file negative_cycles_numeric_test.cc
 * @brief Numeric contract of Negative_Cycles.H and Howard_Min_Mean_Cycle.H
 *        with floating-point weights, checked against an exact oracle.
 *
 * The oracle enumerates the simple cycles of small graphs and adds their
 * weights as exact rationals (the GMP C interface, so that it does not depend
 * on the C++ ABI of gmpxx). It covers float, double and long double weights,
 * subnormal values, cancellations across three binary scales, arc filters and
 * disconnected components, and checks the invariance of the results under
 * scaling by powers of two, the order of the arcs and the order of the nodes.
 */

# include <gtest/gtest.h>
# include <gmp.h>

# include <algorithm>
# include <cmath>
# include <functional>
# include <limits>
# include <random>
# include <set>
# include <tuple>
# include <vector>

# include <Howard_Min_Mean_Cycle.H>
# include <Negative_Cycles.H>
# include <tpl_graph.H>

using namespace Aleph;

namespace
{
  // ------------------------------------------------------------------------
  // Exact rationals over the GMP C interface.
  // ------------------------------------------------------------------------
  class Rational
  {
    mpq_t q_;

  public:
    Rational() { mpq_init(q_); }
    Rational(const Rational & other) { mpq_init(q_); mpq_set(q_, other.q_); }
    Rational & operator=(const Rational & other) { mpq_set(q_, other.q_); return *this; }
    ~Rational() { mpq_clear(q_); }

    // The exact value of a finite floating-point number: its integer
    // mantissa, built 32 bits at a time, times a power of two.
    template <typename T>
    static Rational of(const T x)
    {
      Rational r;
      if (x == T(0))
        return r;
      int e = 0;
      const T f = std::frexp(std::fabs(x), &e);
      constexpr int digits = std::numeric_limits<T>::digits;
      T m = std::ldexp(f, digits);
      std::vector<unsigned long> chunks;
      const T base = T(4294967296.0);
      while (m != T(0))
        {
          const T low = std::fmod(m, base);
          chunks.push_back(static_cast<unsigned long>(low));
          m = (m - low) / base;
        }
      mpz_t z;
      mpz_init(z);
      for (size_t i = chunks.size(); i-- > 0; )
        {
          mpz_mul_2exp(z, z, 32);
          mpz_add_ui(z, z, chunks[i]);
        }
      if (x < T(0))
        mpz_neg(z, z);
      mpq_set_z(r.q_, z);
      mpz_clear(z);
      const int exponent = e - digits;
      if (exponent >= 0)
        mpq_mul_2exp(r.q_, r.q_, static_cast<mp_bitcnt_t>(exponent));
      else
        mpq_div_2exp(r.q_, r.q_, static_cast<mp_bitcnt_t>(-exponent));
      return r;
    }

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

    friend int compare(const Rational & a, const Rational & b) { return mpq_cmp(a.q_, b.q_); }
    int sign() const { return mpq_sgn(q_); }
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
  template <typename T>
  bool within_one_ulp(const T computed, const Rational & exact)
  {
    if (computed == T(0))
      return exact.sign() == 0;
    if ((computed < T(0) ? -1 : 1) != exact.sign())
      return false;
    const T up = std::nextafter(computed, std::numeric_limits<T>::infinity());
    const T down = std::nextafter(computed, -std::numeric_limits<T>::infinity());
    return compare(Rational::of(down), exact) <= 0 and compare(exact, Rational::of(up)) <= 0;
  }

  // ------------------------------------------------------------------------
  // Instances.
  // ------------------------------------------------------------------------
  template <typename T>
  struct Instance
  {
    size_t n = 0;
    std::vector<std::tuple<size_t, size_t, T>> edges;
    std::vector<bool> hidden;   // aligned with edges
  };

  enum class Family { Moderate, Ties, Subnormal, Three_Scales, Market };

  template <typename T>
  T draw_weight(const Family family, std::mt19937_64 & rng)
  {
    const auto small = [&rng](const int range) { return static_cast<int>(rng() % (2 * range + 1)) - range; };
    switch (family)
      {
      case Family::Moderate:
        return std::ldexp(static_cast<T>(small(50)), small(8));
      case Family::Ties:
        return static_cast<T>(small(3));
      case Family::Subnormal:
        return static_cast<T>(small(50)) * std::numeric_limits<T>::denorm_min()
               * static_cast<T>(1 << (rng() % 8));
      case Family::Three_Scales:
        {
          // Far enough apart that a plain sum loses the smaller scales.
          constexpr int gap = std::numeric_limits<T>::digits + 6;
          const int scale = static_cast<int>(rng() % 3) - 1;   // -1, 0, 1
          return std::ldexp(static_cast<T>(small(7)), scale * gap);
        }
      case Family::Market:
        {
          // -log of a rate between assets of random log-prices, with a fee.
          std::uniform_real_distribution<double> lp(-3.0, 3.0);
          return static_cast<T>(lp(rng) - lp(rng) - std::log(1.0 - 0.001));
        }
      }
    return T(0);
  }

  template <typename T>
  Instance<T> make_instance(const Family family, std::mt19937_64 & rng)
  {
    Instance<T> inst;
    inst.n = 1 + rng() % 6;
    const size_t m = rng() % (inst.n * inst.n + 3);
    const bool split = inst.n >= 4 and rng() % 3 == 0;   // two blocks joined one way only
    const bool filter = rng() % 3 == 0;
    for (size_t i = 0; i < m; ++i)
      {
        size_t u = rng() % inst.n;
        size_t v = rng() % inst.n;
        if (split and (u < inst.n / 2) != (v < inst.n / 2) and u > v)
          std::swap(u, v);   // only from the first block to the second
        inst.edges.emplace_back(u, v, draw_weight<T>(family, rng));
        inst.hidden.push_back(filter and rng() % 4 == 0);
      }
    if (family == Family::Three_Scales and inst.n >= 4 and rng() % 2 == 0)
      {
        // A ring whose exact total is zero, big, unit, -big, -unit, whose plain
        // sum loses the unit and comes out as -unit.
        constexpr int gap = std::numeric_limits<T>::digits + 6;
        const T big = std::ldexp(T(1), gap);
        const T unit = T(1 + rng() % 5);
        inst.edges.emplace_back(0, 1, big);
        inst.edges.emplace_back(1, 2, unit);
        inst.edges.emplace_back(2, 3, -big);
        inst.edges.emplace_back(3, 0, -unit);
        for (int k = 0; k < 4; ++k)
          inst.hidden.push_back(false);
      }
    return inst;
  }

  template <typename T>
  using Graph_T = List_Digraph<Graph_Node<int>, Graph_Arc<T>>;

  template <typename T>
  struct Built
  {
    Graph_T<T> g;
    std::vector<typename Graph_T<T>::Node *> nodes;
    std::vector<typename Graph_T<T>::Arc *> arcs;
    std::set<typename Graph_T<T>::Arc *> hidden;
  };

  // Nodes are inserted in the order `node_order`, arcs in `arc_order`, and
  // every weight is multiplied by 2^scale (exactly, for the instances used).
  template <typename T>
  void build(const Instance<T> & inst, Built<T> & b, const std::vector<size_t> & node_order,
             const std::vector<size_t> & arc_order, const int scale = 0)
  {
    b.nodes.assign(inst.n, nullptr);
    for (const size_t i : node_order)
      b.nodes[i] = b.g.insert_node(static_cast<int>(i));
    b.arcs.assign(inst.edges.size(), nullptr);
    for (const size_t k : arc_order)
      {
        const auto & [u, v, w] = inst.edges[k];
        b.arcs[k] = b.g.insert_arc(b.nodes[u], b.nodes[v], std::ldexp(w, scale));
        if (inst.hidden[k])
          b.hidden.insert(b.arcs[k]);
      }
  }

  template <typename T>
  void build(const Instance<T> & inst, Built<T> & b)
  {
    std::vector<size_t> nodes(inst.n);
    std::vector<size_t> arcs(inst.edges.size());
    for (size_t i = 0; i < nodes.size(); ++i)
      nodes[i] = i;
    for (size_t i = 0; i < arcs.size(); ++i)
      arcs[i] = i;
    build(inst, b, nodes, arcs);
  }

  template <typename T>
  struct Hide_Set
  {
    const std::set<typename Graph_T<T>::Arc *> * hidden = nullptr;

    bool operator()(typename Graph_T<T>::Arc * arc) const
    {
      return hidden == nullptr or hidden->count(arc) == 0;
    }
  };

  // ------------------------------------------------------------------------
  // Exhaustive oracle over the simple cycles of the visible arcs.
  // ------------------------------------------------------------------------
  struct Oracle
  {
    bool any_cycle = false;
    bool any_bounded = false;   // a cycle of at most `bound` arcs
    Rational best_total;        // minimum total over cycles of at most `bound` arcs
    size_t best_total_length = 0;  // shortest among exact total-cost minimizers
    Rational best_mean_total;   // the minimum mean is best_mean_total / best_mean_length
    size_t best_mean_length = 1;
  };

  template <typename T>
  Oracle oracle(const Instance<T> & inst, const size_t bound)
  {
    Oracle o;
    std::vector<bool> on_path(inst.n, false);
    std::vector<Rational> weights;
    for (const auto & e : inst.edges)
      weights.push_back(Rational::of(std::get<2>(e)));

    std::function<void(size_t, size_t, const Rational &, size_t)> dfs =
      [&](const size_t start, const size_t u, const Rational & total, const size_t length)
    {
      for (size_t k = 0; k < inst.edges.size(); ++k)
        {
          const auto & [a, b, w] = inst.edges[k];
          if (a != u or inst.hidden[k])
            continue;
          Rational t = total;
          t += weights[k];
          if (b == start)
            {
              const size_t len = length + 1;
              if (not o.any_cycle or mean_less(t, len, o.best_mean_total, o.best_mean_length))
                {
                  o.best_mean_total = t;
                  o.best_mean_length = len;
                }
              o.any_cycle = true;
              if (len <= bound and (not o.any_bounded or compare(t, o.best_total) < 0
                                    or (compare(t, o.best_total) == 0 and len < o.best_total_length)))
                {
                  o.best_total = t;
                  o.best_total_length = len;
                  o.any_bounded = true;
                }
              continue;
            }
          if (b < start or on_path[b])
            continue;
          on_path[b] = true;
          dfs(start, b, t, length + 1);
          on_path[b] = false;
        }
    };
    for (size_t s = 0; s < inst.n; ++s)
      {
        on_path[s] = true;
        dfs(s, s, Rational(), 0);
        on_path[s] = false;
      }
    return o;
  }

  // The exact total of a witness given as a list of arcs, which must be
  // consecutive, visible, simple and closed.
  template <typename T>
  bool exact_witness(const Built<T> & b, const DynList<typename Graph_T<T>::Arc *> & arcs,
                     Rational & total, size_t & length)
  {
    std::set<typename Graph_T<T>::Node *> seen;
    typename Graph_T<T>::Node * first = nullptr;
    typename Graph_T<T>::Node * current = nullptr;
    length = 0;
    total = Rational();
    for (auto it = arcs.get_it(); it.has_curr(); it.next_ne())
      {
        auto * arc = it.get_curr();
        if (b.hidden.count(arc) != 0)
          return false;
        auto * src = b.g.get_src_node(arc);
        if (first == nullptr)
          first = current = src;
        if (src != current or not seen.insert(src).second)
          return false;
        current = b.g.get_tgt_node(arc);
        total += Rational::of(arc->get_info());
        ++length;
      }
    return length > 0 and current == first;
  }

  // ------------------------------------------------------------------------
  // The checks, for one instance.
  // ------------------------------------------------------------------------
  /** @brief Check the exact short optimum and shortest tie against GMP rationals. */
  template <typename T>
  void check_short(const Instance<T> & inst, const Built<T> & b, const size_t bound)
  {
    using G = Graph_T<T>;
    const Oracle o = oracle(inst, bound);
    const auto r = most_negative_cycle_up_to_3<G, Dft_Dist<G>, Hide_Set<T>>(
        b.g, bound, Dft_Dist<G>(), Hide_Set<T>{&b.hidden});
    ASSERT_EQ(r.has_cycle, o.any_bounded);
    if (not r.has_cycle)
      return;

    Rational total;
    size_t length = 0;
    ASSERT_TRUE(exact_witness(b, r.cycle_arcs, total, length));
    ASSERT_EQ(length, r.length);
    ASSERT_EQ(length, o.best_total_length);
    ASSERT_EQ(compare(total, o.best_total), 0);
    ASSERT_TRUE(within_one_ulp(r.total_cost, total));
    ASSERT_EQ(r.is_negative(), total.sign() < 0);
  }

  template <typename T>
  void check_bounded(const Instance<T> & inst, const Built<T> & b, const size_t bound,
                     const char * family, const size_t trial)
  {
    using G = Graph_T<T>;
    const Oracle o = oracle(inst, bound);
    const auto r = most_negative_cycle_bounded<G, Dft_Dist<G>, Hide_Set<T>>(
        b.g, bound, Dft_Dist<G>(), Hide_Set<T>{&b.hidden});
    ASSERT_EQ(r.has_cycle, o.any_bounded) << family << " trial=" << trial << " L=" << bound;
    if (not r.has_cycle)
      return;

    Rational total;
    size_t length = 0;
    ASSERT_TRUE(exact_witness(b, r.cycle_arcs, total, length)) << family << " trial=" << trial;
    ASSERT_EQ(length, r.length) << family << " trial=" << trial;
    ASSERT_LE(length, bound) << family << " trial=" << trial;
    ASSERT_TRUE(within_one_ulp(r.total_cost, total)) << family << " trial=" << trial;
    ASSERT_EQ(r.is_negative(), total.sign() < 0) << family << " trial=" << trial;

    // The gap is rigorous: the optimum is no cheaper than total - gap.
    ASSERT_GE(r.optimality_gap, T(0)) << family << " trial=" << trial;
    ASSERT_LE(compare(total - o.best_total, Rational::of(r.optimality_gap)), 0)
        << family << " trial=" << trial << " L=" << bound;
    ASSERT_EQ(r.is_exact, r.optimality_gap == T(0)) << family << " trial=" << trial;
    if (r.is_exact)
      ASSERT_EQ(compare(total, o.best_total), 0) << family << " trial=" << trial;
  }

  template <typename T>
  void check_howard(const Instance<T> & inst, const Built<T> & b, const char * family,
                    const size_t trial)
  {
    using G = Graph_T<T>;
    const Oracle o = oracle(inst, inst.n);
    const auto h = howard_minimum_mean_cycle<G, Dft_Dist<G>, Hide_Set<T>>(
        b.g, Dft_Dist<G>(), Hide_Set<T>{&b.hidden});
    ASSERT_EQ(h.has_cycle, o.any_cycle) << family << " trial=" << trial;
    if (not h.has_cycle or h.used_karp)
      return;

    Rational total;
    size_t length = 0;
    ASSERT_TRUE(exact_witness(b, h.cycle_arcs, total, length)) << family << " trial=" << trial;
    ASSERT_EQ(length, h.cycle_length) << family << " trial=" << trial;
    ASSERT_TRUE(within_one_ulp(h.cycle_total_cost, total)) << family << " trial=" << trial;
    if (h.numeric_quality == Cycle_Numeric_Quality::Exact)
      ASSERT_TRUE(mean_equal(total, length, o.best_mean_total, o.best_mean_length))
          << family << " trial=" << trial;
  }

  template <typename T>
  void check_enumeration(const Instance<T> & inst, const Built<T> & b, const char * family,
                         const size_t trial)
  {
    using G = Graph_T<T>;
    const Oracle o = oracle(inst, inst.n);
    const auto cycles = find_disjoint_negative_cycles<G, Dft_Dist<G>, Hide_Set<T>>(
        b.g, 3, Negative_Cycle_Exclusion::All_Arcs, Dft_Dist<G>(), Hide_Set<T>{&b.hidden});
    std::set<typename G::Arc *> used;
    for (auto it = cycles.get_it(); it.has_curr(); it.next_ne())
      {
        const auto & item = it.get_curr();
        const auto arcs = item.cycle.arcs();   // a list by value: keep it alive while iterating
        Rational total;
        size_t length = 0;
        ASSERT_TRUE(exact_witness(b, arcs, total, length)) << family << " trial=" << trial;
        ASSERT_EQ(length, item.length) << family << " trial=" << trial;
        ASSERT_LT(total.sign(), 0) << family << " trial=" << trial;   // exactly negative
        ASSERT_TRUE(within_one_ulp(item.total_cost, total)) << family << " trial=" << trial;
        for (auto ai = arcs.get_it(); ai.has_curr(); ai.next_ne())
          ASSERT_TRUE(used.insert(ai.get_curr()).second) << family << " trial=" << trial;
      }
    // A cycle of negative exact mean is a negative cycle: when the oracle
    // says there is none, nothing is reported.
    if (not o.any_cycle or o.best_mean_total.sign() >= 0)
      ASSERT_TRUE(cycles.is_empty()) << family << " trial=" << trial;
  }

  const std::pair<Family, const char *> families[] = {
    {Family::Moderate, "moderate"},
    {Family::Ties, "ties"},
    {Family::Subnormal, "subnormal"},
    {Family::Three_Scales, "three-scales"},
    {Family::Market, "market"},
  };

  template <typename T>
  void run_differential(const unsigned long long seed, const size_t trials)
  {
    std::mt19937_64 rng(seed);
    for (const auto & [family, name] : families)
      for (size_t trial = 0; trial < trials; ++trial)
        {
          const Instance<T> inst = make_instance<T>(family, rng);
          Built<T> b;
          build(inst, b);
          for (const size_t bound : {size_t(1), size_t(2), size_t(3), inst.n})
            check_bounded(inst, b, bound, name, trial);
          check_howard(inst, b, name, trial);
          check_enumeration(inst, b, name, trial);
          if (::testing::Test::HasFatalFailure())
            return;
        }
  }

  /** @brief Exercise all numeric families and short bounds with a fixed seed. */
  template <typename T>
  void run_short_differential(const unsigned long long seed)
  {
    std::mt19937_64 rng(seed);
    for (const auto & [family, name] : families)
      for (size_t trial = 0; trial < 200; ++trial)
        {
          SCOPED_TRACE(::testing::Message() << name << " trial=" << trial);
          const Instance<T> inst = make_instance<T>(family, rng);
          Built<T> b;
          build(inst, b);
          for (size_t bound = 1; bound <= 3; ++bound)
            {
              SCOPED_TRACE(::testing::Message() << "L=" << bound);
              check_short(inst, b, bound);
              if (::testing::Test::HasFatalFailure())
                return;
            }
        }
  }
} // namespace


// ---------------------------------------------------------------------------
// Differential checks against the exact oracle.
// ---------------------------------------------------------------------------

TEST(NegativeCyclesNumericTest, DoubleWeightsAgreeWithTheExactOracle)
{
  run_differential<double>(0x5EED0001ULL, 400);
}


TEST(NegativeCyclesNumericTest, FloatWeightsAgreeWithTheExactOracle)
{
  run_differential<float>(0x5EED0002ULL, 300);
}


TEST(NegativeCyclesNumericTest, LongDoubleWeightsAgreeWithTheExactOracle)
{
  run_differential<long double>(0x5EED0003ULL, 300);
}


TEST(NegativeCyclesNumericTest, ShortDoubleCyclesAgreeWithTheExactOracle)
{
  run_short_differential<double>(0xD40001ULL);
}


TEST(NegativeCyclesNumericTest, ShortFloatCyclesAgreeWithTheExactOracle)
{
  run_short_differential<float>(0xD40002ULL);
}


TEST(NegativeCyclesNumericTest, ShortLongDoubleCyclesAgreeWithTheExactOracle)
{
  run_short_differential<long double>(0xD40003ULL);
}


TEST(NegativeCyclesNumericTest, ShortOptimaSurvivePermutationAndExactScaling)
{
  std::mt19937_64 rng(0xD45CA1E);
  for (const auto family : {Family::Moderate, Family::Ties, Family::Three_Scales, Family::Market})
    for (size_t trial = 0; trial < 200; ++trial)
      {
        SCOPED_TRACE(::testing::Message() << "family=" << int(family) << " trial=" << trial);
        const Instance<double> inst = make_instance<double>(family, rng);
        std::vector<size_t> nodes(inst.n), arcs(inst.edges.size());
        for (size_t i = 0; i < nodes.size(); ++i)
          nodes[i] = i;
        for (size_t i = 0; i < arcs.size(); ++i)
          arcs[i] = i;
        for (const int scale : {-7, 0, 9})
          {
            std::shuffle(nodes.begin(), nodes.end(), rng);
            std::shuffle(arcs.begin(), arcs.end(), rng);
            auto scaled = inst;
            for (auto & edge : scaled.edges)
              std::get<2>(edge) = std::ldexp(std::get<2>(edge), scale);
            Built<double> b;
            build(scaled, b, nodes, arcs);
            for (size_t bound = 1; bound <= 3; ++bound)
              {
                SCOPED_TRACE(::testing::Message() << "scale=" << scale << " L=" << bound);
                check_short(scaled, b, bound);
                if (::testing::Test::HasFatalFailure())
                  return;
              }
          }
      }
}


// ---------------------------------------------------------------------------
// Invariance (stage B6 of auditoria-profunda-arbitraje-2026-10-02.md).
// ---------------------------------------------------------------------------

// Multiplying every weight by a power of two is exact (no overflow and no
// subnormal results for these families), and rounding commutes with it, so
// the bounded search must take the same decisions: the same cycle, with its
// cost and gap scaled exactly.
TEST(NegativeCyclesNumericTest, BoundedSearchIsInvariantUnderScalingByPowersOfTwo)
{
  std::mt19937_64 rng(0x5CA1E);
  for (const auto family : {Family::Moderate, Family::Ties, Family::Three_Scales, Family::Market})
    for (size_t trial = 0; trial < 300; ++trial)
      {
        const Instance<double> inst = make_instance<double>(family, rng);
        std::vector<size_t> nodes(inst.n);
        std::vector<size_t> arcs(inst.edges.size());
        for (size_t i = 0; i < nodes.size(); ++i)
          nodes[i] = i;
        for (size_t i = 0; i < arcs.size(); ++i)
          arcs[i] = i;
        for (const int scale : {-7, 9})
          {
            Built<double> plain;
            Built<double> scaled;
            build(inst, plain, nodes, arcs, 0);
            build(inst, scaled, nodes, arcs, scale);
            using G = Graph_T<double>;
            const size_t bound = std::max<size_t>(1, inst.n);
            const auto a = most_negative_cycle_bounded<G, Dft_Dist<G>, Hide_Set<double>>(
                plain.g, bound, Dft_Dist<G>(), Hide_Set<double>{&plain.hidden});
            const auto b = most_negative_cycle_bounded<G, Dft_Dist<G>, Hide_Set<double>>(
                scaled.g, bound, Dft_Dist<G>(), Hide_Set<double>{&scaled.hidden});
            ASSERT_EQ(a.has_cycle, b.has_cycle) << "trial=" << trial;
            if (not a.has_cycle)
              continue;
            ASSERT_EQ(b.total_cost, std::ldexp(a.total_cost, scale)) << "trial=" << trial;
            ASSERT_EQ(b.optimality_gap, std::ldexp(a.optimality_gap, scale)) << "trial=" << trial;
            ASSERT_EQ(a.is_exact, b.is_exact) << "trial=" << trial;
            ASSERT_EQ(a.matches_relaxed_bound, b.matches_relaxed_bound) << "trial=" << trial;
            ASSERT_EQ(a.length, b.length) << "trial=" << trial;
          }
      }
}


// The order in which arcs and nodes are inserted changes how ties are broken
// and the order of the sums, but not the exact optimum: whenever a result is
// certified (bounded search) or verified exactly (Howard), its value is the
// oracle's, whatever the order.
TEST(NegativeCyclesNumericTest, CertifiedResultsDoNotDependOnArcOrNodeOrder)
{
  std::mt19937_64 rng(0x0D3E5);
  for (const auto family : {Family::Moderate, Family::Ties, Family::Three_Scales, Family::Market})
    for (size_t trial = 0; trial < 200; ++trial)
      {
        const Instance<double> inst = make_instance<double>(family, rng);
        const Oracle o = oracle(inst, inst.n);
        std::vector<size_t> nodes(inst.n);
        std::vector<size_t> arcs(inst.edges.size());
        for (size_t i = 0; i < nodes.size(); ++i)
          nodes[i] = i;
        for (size_t i = 0; i < arcs.size(); ++i)
          arcs[i] = i;
        for (int shuffle = 0; shuffle < 4; ++shuffle)
          {
            std::shuffle(nodes.begin(), nodes.end(), rng);
            std::shuffle(arcs.begin(), arcs.end(), rng);
            Built<double> b;
            build(inst, b, nodes, arcs);
            using G = Graph_T<double>;

            const auto r = most_negative_cycle_bounded<G, Dft_Dist<G>, Hide_Set<double>>(
                b.g, std::max<size_t>(1, inst.n), Dft_Dist<G>(), Hide_Set<double>{&b.hidden});
            if (r.is_exact)
              {
                Rational total;
                size_t length = 0;
                ASSERT_TRUE(exact_witness(b, r.cycle_arcs, total, length));
                ASSERT_EQ(compare(total, o.best_total), 0) << "trial=" << trial;
              }

            const auto h = howard_minimum_mean_cycle<G, Dft_Dist<G>, Hide_Set<double>>(
                b.g, Dft_Dist<G>(), Hide_Set<double>{&b.hidden});
            if (h.has_cycle and not h.used_karp and h.numeric_quality == Cycle_Numeric_Quality::Exact)
              {
                Rational total;
                size_t length = 0;
                ASSERT_TRUE(exact_witness(b, h.cycle_arcs, total, length));
                ASSERT_TRUE(mean_equal(total, length, o.best_mean_total, o.best_mean_length))
                    << "trial=" << trial;
              }
          }
      }
}


TEST(NegativeCyclesNumericTest, ExactUpperBoundBeyondTheRangeIsInfinity)
{
  // CodeRabbit, PR #103: upper() stepped its bound with nextafter() until it
  // was not below the exact value. Above DBL_MAX the step reaches infinity,
  // whose negation the exact check cannot add, and it looped forever.
  using negative_cycles_detail::Exact_Sum;
  const double max = std::numeric_limits<double>::max();

  Exact_Sum<double> above;
  ASSERT_TRUE(above.add(max));
  ASSERT_TRUE(above.add(1.0));   // DBL_MAX + 1, an expansion of finite components
  EXPECT_EQ(above.upper(), std::numeric_limits<double>::infinity());

  // Finite bounds are unchanged: the value itself when representable, the
  // next number up otherwise, also at the edge of the range.
  Exact_Sum<double> at;
  ASSERT_TRUE(at.add(max));
  EXPECT_EQ(at.upper(), max);

  Exact_Sum<double> just_above_one;
  ASSERT_TRUE(just_above_one.add(1.0));
  ASSERT_TRUE(just_above_one.add(0x1p-60));
  EXPECT_EQ(just_above_one.upper(), std::nextafter(1.0, 2.0));

  Exact_Sum<double> below_minus_max;   // -DBL_MAX - 1: the bound -DBL_MAX is finite
  ASSERT_TRUE(below_minus_max.add(-max));
  ASSERT_TRUE(below_minus_max.add(-1.0));
  EXPECT_EQ(below_minus_max.upper(), -max);

  // Independent audit of stage C, E1. After an add() that overflowed, a
  // component is -inf and the value is meaningless; upper() stepped up from
  // -DBL_MAX one ulp at a time and never ended. It is now infinity. The
  // overflow uses the largest component, which is a long double where
  // FLT_EVAL_METHOD is not 0: there twice -DBL_MAX does not overflow.
  using Component = Exact_Sum<double>::Component;
  const Component component_max = std::numeric_limits<Component>::max();
  Exact_Sum<double> broken;
  ASSERT_TRUE(broken.add(-component_max));
  ASSERT_FALSE(broken.add(-component_max));
  EXPECT_EQ(broken.upper(), std::numeric_limits<double>::infinity());
}
