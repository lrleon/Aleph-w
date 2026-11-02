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
 * @file graph_exhaustive_matching_test.cc
 * @brief Matchings of maximum weight and of minimum cost on every small
 *        multigraph, against brute force.
 *
 * Every matching of a multigraph of up to 3 nodes and 3 arcs, with every
 * assignment of the weights -1, 0 and 1 to its arcs, of one of up to 4
 * nodes with each set of hidden arcs, and of random graphs of up to 8
 * nodes, is weighed by brute force over the sets of its arcs; loops
 * never match, and of parallel arcs any may. The
 * matchings of Blossom_Weighted.H and Min_Cost_Matching.H must be
 * matchings of the arcs that the filter shows, with the weight or the
 * cost and the cardinality that they report, and the best ones, also
 * when the cardinality comes first.
 */

#include <algorithm>
#include <limits>
#include <random>
#include <set>
#include <stdexcept>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include <tpl_graph.H>
#include <Blossom_Weighted.H>
#include <Min_Cost_Matching.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Hide_Mask;
using graph_filters::Weight_Of;

namespace
{
  /// The best matchings of s among the arcs that hidden shows, by brute
  /// force: the greatest weight, the least cost, and those two among the
  /// matchings of the most arcs; whether a perfect one exists, and the
  /// least cost of one
  struct Best
  {
    long long max_weight = 0;
    long long min_cost = 0;
    size_t max_size = 0;
    long long max_weight_of_max_size = std::numeric_limits<long long>::lowest();
    long long min_cost_of_max_size = std::numeric_limits<long long>::max();
    bool perfect = false;
    long long min_cost_perfect = std::numeric_limits<long long>::max();
  };

  Best best_matchings(const Spec & s, unsigned long hidden)
  {
    Best b;
    const size_t m = s.arcs.size();
    for (unsigned long sub = 0; sub < (1UL << m); ++sub)
      {
        if (sub & hidden)
          continue;
        std::vector<bool> used(size_t(s.n), false);
        long long w = 0;
        size_t size = 0;
        bool ok = true;
        for (size_t i = 0; i < m and ok; ++i)
          if (sub & (1UL << i))
            {
              const int u = s.arcs[i][0], v = s.arcs[i][1];
              ok = u != v and not used[size_t(u)] and not used[size_t(v)];
              used[size_t(u)] = used[size_t(v)] = true;
              w += s.w[i];
              ++size;
            }
        if (not ok)
          continue;
        b.max_weight = std::max(b.max_weight, w);
        b.min_cost = std::min(b.min_cost, w);
        if (size > b.max_size)
          {
            b.max_size = size;
            b.max_weight_of_max_size = w;
            b.min_cost_of_max_size = w;
          }
        else if (size == b.max_size)
          {
            b.max_weight_of_max_size = std::max(b.max_weight_of_max_size, w);
            b.min_cost_of_max_size = std::min(b.min_cost_of_max_size, w);
          }
        if (2 * size == size_t(s.n))
          {
            b.perfect = true;
            b.min_cost_perfect = std::min(b.min_cost_perfect, w);
          }
      }
    if (b.max_size == 0)
      b.max_weight_of_max_size = b.min_cost_of_max_size = 0;
    return b;
  }

  /// What is wrong with the arcs of l as a matching of the shown arcs of
  /// s of weight w and size k; empty if nothing
  template <class GT>
  std::string matching_error(const Spec & s, const DynDlist<typename GT::Arc *> & l,
                             unsigned long hidden, long long w, size_t k)
  {
    std::vector<bool> used(size_t(s.n), false);
    long long total = 0;
    size_t size = 0;
    for (auto it = l.get_it(); it.has_curr(); it.next_ne())
      {
        const int i = num(it.get_curr());
        if (i < 0 or size_t(i) >= s.arcs.size() or not graph_filters::shown(hidden, i))
          return "a hidden arc, or one that is not of the graph";
        const int u = s.arcs[size_t(i)][0], v = s.arcs[size_t(i)][1];
        if (u == v or used[size_t(u)] or used[size_t(v)])
          return "not a matching";
        used[size_t(u)] = used[size_t(v)] = true;
        total += s.w[size_t(i)];
        ++size;
      }
    if (total != w or size != k)
      return "it reports " + std::to_string(w) + " and " + std::to_string(k)
        + " arcs for " + std::to_string(total) + " and " + std::to_string(size);
    return "";
  }

  /// Calls f with each assignment to the arcs of s of weights from lo to
  /// hi
  template <class F>
  void for_each_weighting(Spec s, long long lo, long long hi, F && f)
  {
    s.w.assign(s.arcs.size(), lo);
    while (true)
      {
        f(static_cast<const Spec &>(s));
        size_t i = 0;
        while (i < s.w.size() and s.w[i] == hi)
          s.w[i++] = lo;
        if (i == s.w.size())
          return;
        ++s.w[i];
      }
  }

  struct Matching_Checks
  {
    Check max_weight{"compute_maximum_weight_general_matching()"};
    Check max_weight_card{"compute_maximum_weight_general_matching(), the cardinality first"};
    Check min_cost{"compute_minimum_cost_general_matching()"};
    Check min_cost_card{"compute_minimum_cost_general_matching(), the cardinality first"};
    Check perfect{"compute_minimum_cost_perfect_general_matching()"};
    Check functors{"the functors and the blossom_ aliases"};

    void expect() const
    {
      for (const Check * c : {&max_weight, &max_weight_card, &min_cost, &min_cost_card,
                              &perfect, &functors})
        c->expect();
    }
  };

  /// Runs the matchings on s with weights s.w, hiding the arcs of hidden
  template <class GT>
  void check_matchings(Matching_Checks & c, const Spec & s, unsigned long hidden)
  {
    using Dist = Weight_Of<GT, long long>;
    using Filter = Hide_Mask<GT>;
    Built<GT> b(s);
    Dist d;
    d.weights = &s.w;
    const Best want = best_matchings(s, hidden);
    const std::string at = hidden == 0 ? "" : "hiding " + std::to_string(hidden) + ": ";

    auto run = [&] (Check & check, auto && match, long long want_value, size_t want_size,
                    bool any_size)
      {
        check.run();
        DynDlist<typename GT::Arc *> l;
        const auto [value, size] = match(l);
        if (auto e = matching_error<GT>(s, l, hidden, value, size); not e.empty())
          check.fail(s, at + e);
        else if (value != want_value or (not any_size and size != want_size))
          check.fail(s, at + "gives " + std::to_string(value) + " with " + std::to_string(size)
                     + " arcs, not " + std::to_string(want_value) + " with "
                     + std::to_string(want_size));
      };

    run(c.max_weight, [&] (auto & l)
      {
        const auto r = compute_maximum_weight_general_matching<GT, Dist, Filter>
          (b.g, l, d, Filter{hidden});
        return std::pair(r.total_weight, r.cardinality);
      }, want.max_weight, 0, true);

    run(c.max_weight_card, [&] (auto & l)
      {
        const auto r = compute_maximum_weight_general_matching<GT, Dist, Filter>
          (b.g, l, d, Filter{hidden}, true);
        return std::pair(r.total_weight, r.cardinality);
      }, want.max_weight_of_max_size, want.max_size, false);

    run(c.min_cost, [&] (auto & l)
      {
        const auto r = compute_minimum_cost_general_matching<GT, Dist, Filter>
          (b.g, l, d, Filter{hidden});
        return std::pair(r.total_cost, r.cardinality);
      }, want.min_cost, 0, true);

    run(c.min_cost_card, [&] (auto & l)
      {
        const auto r = compute_minimum_cost_general_matching<GT, Dist, Filter>
          (b.g, l, d, Filter{hidden}, true);
        return std::pair(r.total_cost, r.cardinality);
      }, want.min_cost_of_max_size, want.max_size, false);

    c.perfect.run();
    {
      DynDlist<typename GT::Arc *> l;
      const auto r = compute_minimum_cost_perfect_general_matching<GT, Dist, Filter>
        (b.g, l, d, Filter{hidden});
      if (r.feasible != want.perfect)
        c.perfect.fail(s, at + (r.feasible ? "finds a perfect matching that is not"
                                           : "finds no perfect matching"));
      else if (not r.feasible and not l.is_empty())
        c.perfect.fail(s, at + "leaves arcs without a perfect matching");
      else if (r.feasible)
        {
          if (auto e = matching_error<GT>(s, l, hidden, r.total_cost, r.cardinality); not e.empty())
            c.perfect.fail(s, at + e);
          else if (r.total_cost != want.min_cost_perfect or 2 * r.cardinality != size_t(s.n))
            c.perfect.fail(s, at + "costs " + std::to_string(r.total_cost) + ", not "
                           + std::to_string(want.min_cost_perfect));
        }
    }

    // The other entry points give the same
    if (hidden == 0)
      {
        c.functors.run();
        DynDlist<typename GT::Arc *> l1, l2, l3, l4, l5;
        const auto r1 = blossom_maximum_weight_matching<GT, Dist>(b.g, l1, d);
        const auto r2 = Compute_Maximum_Weight_General_Matching<GT, Dist>(d)(b.g, l2);
        const auto r3 = blossom_minimum_cost_matching<GT, Dist>(b.g, l3, d);
        const auto r4 = Compute_Minimum_Cost_General_Matching<GT, Dist>(d)(b.g, l4);
        const auto r5 = Compute_Minimum_Cost_Perfect_General_Matching<GT, Dist>(d)(b.g, l5);
        if (r1.total_weight != want.max_weight or r2.total_weight != want.max_weight
            or r3.total_cost != want.min_cost or r4.total_cost != want.min_cost
            or r5.feasible != want.perfect
            or (r5.feasible and r5.total_cost != want.min_cost_perfect))
          c.functors.fail(s, "an entry point gives another answer");
      }
  }
}

template <class GT>
class ExhaustiveMatching : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveMatching, Undirected_Kinds);

TYPED_TEST(ExhaustiveMatching, EveryWeightingHasItsBestMatchings)
{
  Matching_Checks c;
  for_each_spec(domain(3, 3), false, [&] (const Spec & spec)
    {
      for_each_weighting(spec, -1, 1, [&] (const Spec & s)
        {
          check_matchings<TypeParam>(c, s, 0);
        });
    });
  c.expect();
}

TYPED_TEST(ExhaustiveMatching, MatchingsRespectTheFilter)
{
  Matching_Checks c;
  for_each_spec(domain(4, 3), false, [&] (const Spec & spec)
    {
      // Distinct weights, against the order of the arcs
      Spec s = spec;
      s.w.clear();
      for (size_t i = 0; i < s.arcs.size(); ++i)
        s.w.push_back(static_cast<long long>(s.arcs.size() - i) - 1);
      for (unsigned long hidden = 1; hidden < (1UL << s.num_arcs()); ++hidden)
        check_matchings<TypeParam>(c, s, hidden);
    });
  c.expect();
}

TYPED_TEST(ExhaustiveMatching, RandomGraphsHaveTheirBestMatchings)
{
  Matching_Checks c;
  std::mt19937 rng(seed(734));
  const int graphs = heavy() ? 2000 : 200;
  for (int k = 0; k < graphs; ++k)
    {
      const int n = 4 + int(rng() % 5);
      const int m = n - 1 + int(rng() % unsigned(n));
      Spec s = random_spec(rng, n, std::min(m, 14), false);
      random_weights(rng, s, -5, 9);
      check_matchings<TypeParam>(c, s, 0);
    }
  c.expect();
}

template <class GT>
class ExhaustiveMatchingOnDigraphs : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveMatchingOnDigraphs, Directed_Kinds);

TYPED_TEST(ExhaustiveMatchingOnDigraphs, MatchingsRejectADigraph)
{
  using GT = TypeParam;
  GT g;
  auto * p = g.insert_node(0);
  auto * q = g.insert_node(1);
  g.insert_arc(p, q, 0);
  DynDlist<typename GT::Arc *> l;
  EXPECT_THROW(compute_maximum_weight_general_matching(g, l), std::domain_error);
  EXPECT_THROW(compute_minimum_cost_general_matching(g, l), std::domain_error);
  EXPECT_THROW(compute_minimum_cost_perfect_general_matching(g, l), std::domain_error);
}
