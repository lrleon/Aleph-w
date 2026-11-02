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
 * @file graph_exhaustive_cut_test.cc
 * @brief Global minimum cuts on every small multigraph, against brute
 *        force.
 *
 * The weight of a minimum cut of a multigraph is the least, over the
 * splits of its nodes in two nonempty sides, of the weights of the arcs
 * between the sides; the oracle tries them all. Stoer-Wagner runs on
 * every multigraph of up to 3 nodes and 3 arcs, with every assignment of
 * the weights 0 to 2, and of 4 nodes with weights 1; Karger, which counts
 * arcs, on every multigraph of up to 4 nodes and 3 arcs and on random
 * graphs of up to 8 nodes: their cuts must be splits whose arcs
 * between the sides are those they give, of the least weight, or, for
 * Karger with few iterations, a weight that is not less.
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
#include <Stoer_Wagner.H>
#include <Karger.H>

#include "graph_filter_catalog.H"
#include "graph_harness.H"
#include "graph_oracles.H"

using namespace Aleph;
using namespace graph_harness;
namespace oracle = graph_oracles;
using graph_filters::Weight_Of;

namespace
{
  /// The weight of a minimum cut of s, with the weights w (1 each if w is
  /// empty), over every split of its nodes; for less than two nodes, 0
  long long min_cut(const Spec & s, const std::vector<long long> & w)
  {
    if (s.n < 2)
      return 0;
    long long best = std::numeric_limits<long long>::max();
    // Node 0 on the first side; the second side is not empty
    for (unsigned long side = 0; side < (1UL << (s.n - 1)); ++side)
      {
        if (side == (1UL << (s.n - 1)) - 1)
          continue;
        auto first = [&] (int x) { return x == 0 or ((side >> (x - 1)) & 1UL) != 0; };
        long long total = 0;
        for (size_t i = 0; i < s.arcs.size(); ++i)
          if (first(s.arcs[i][0]) != first(s.arcs[i][1]))
            total += w.empty() ? 1 : w[i];
        best = std::min(best, total);
      }
    return best;
  }

  /// What is wrong with vs, vt and cut as a cut of s of weight value: the
  /// sides split the nodes, nonempty, and the cut has the arcs between
  /// them, of that total weight; empty if nothing
  template <class GT>
  std::string cut_error(const Spec & s, const std::vector<long long> & w,
                        const DynList<typename GT::Node *> & vs,
                        const DynList<typename GT::Node *> & vt,
                        const DynList<typename GT::Arc *> & cut, long long value)
  {
    std::vector<int> side(size_t(s.n), -1);
    for (int k = 0; k < 2; ++k)
      for (auto it = (k == 0 ? vs : vt).get_it(); it.has_curr(); it.next_ne())
        {
          const int x = num(it.get_curr());
          if (x < 0 or x >= s.n or side[size_t(x)] != -1)
            return "a node is in both sides, or twice";
          side[size_t(x)] = k;
        }
    if (std::count(side.begin(), side.end(), -1) > 0 or vs.is_empty() or vt.is_empty())
      return "the sides do not split the nodes in two";
    std::multiset<int> want, got;
    long long total = 0;
    for (size_t i = 0; i < s.arcs.size(); ++i)
      if (side[size_t(s.arcs[i][0])] != side[size_t(s.arcs[i][1])])
        {
          want.insert(int(i));
          total += w.empty() ? 1 : w[i];
        }
    for (auto it = cut.get_it(); it.has_curr(); it.next_ne())
      got.insert(num(it.get_curr()));
    if (got != want)
      return "the arcs of the cut are not those between the sides";
    if (total != value)
      return "it says " + std::to_string(value) + " for a cut of " + std::to_string(total);
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
}

template <class GT>
class ExhaustiveCut : public ::testing::Test {};

TYPED_TEST_SUITE(ExhaustiveCut, Undirected_Kinds);

TYPED_TEST(ExhaustiveCut, StoerWagnerFindsAMinimumCut)
{
  using GT = TypeParam;
  using Dist = Weight_Of<GT, long long>;
  Check cut("Stoer_Wagner_Min_Cut");
  Check weight("Stoer_Wagner_Min_Cut::min_cut_weight()");
  Check few("Stoer_Wagner_Min_Cut on less than two nodes");

  // Every weighting of the graphs of 3 nodes, and the weights 1 on 4
  auto run = [&] (const Spec & s)
        {
          Built<GT> b(s);
          Dist d;
          d.weights = &s.w;
          Stoer_Wagner_Min_Cut<GT, Dist> sw(d);
          const long long want = min_cut(s, s.w);

          if (s.n < 2)
            {
              few.run();
              DynList<typename GT::Node *> vs, vt;
              DynList<typename GT::Arc *> arcs;
              try
                {
                  (void) sw(b.g, vs, vt, arcs);
                  few.fail(s, "cuts a graph of less than two nodes");
                }
              catch (const std::domain_error &) {}
              if (sw.min_cut_weight(b.g) != 0)
                few.fail(s, "min_cut_weight() is not 0");
              return;
            }

          cut.run();
          DynList<typename GT::Node *> vs, vt;
          DynList<typename GT::Arc *> arcs;
          const long long got = sw(b.g, vs, vt, arcs);
          if (auto e = cut_error<GT>(s, s.w, vs, vt, arcs, got); not e.empty())
            cut.fail(s, e);
          else if (got != want)
            cut.fail(s, "a cut of " + std::to_string(got) + ", not " + std::to_string(want));

          weight.run();
          if (const long long w = sw.min_cut_weight(b.g); w != want)
            weight.fail(s, std::to_string(w) + ", not " + std::to_string(want));
        };
  for_each_spec(domain(3, 3), false, [&] (const Spec & spec)
    {
      for_each_weighting(spec, 0, 2, run);
    });
  for_each_spec(domain(4, 3), false, [&] (const Spec & spec)
    {
      for_each_weighting(spec, 1, 1, run);
    });

  cut.expect();
  weight.expect();
  few.expect();
}

namespace
{
  /// Whether Karger takes s: two nodes or more, an arc, no loop, and one
  /// component
  bool karger_takes(const Spec & s)
  {
    return s.n >= 2 and not s.arcs.empty() and not s.has_loop()
      and oracle::components(s) == 1;
  }

  struct Karger_Checks
  {
    Check rejects{"Karger_Min_Cut, what it does not take"};
    Check plain{"Karger_Min_Cut::operator()"};
    Check many{"Karger_Min_Cut::operator(), many iterations"};
    Check size{"Karger_Min_Cut::compute_min_cut_size()"};
    Check fast{"Karger_Min_Cut::fast()"};

    void expect() const
    {
      for (const Check * c : {&rejects, &plain, &many, &size, &fast})
        c->expect();
    }
  };

  template <class GT>
  void check_karger(Karger_Checks & c, const Spec & s)
  {
    using Node = typename GT::Node;
    using Arc = typename GT::Arc;
    Built<GT> b(s);
    Karger_Min_Cut<GT> karger(seed(735));
    const std::vector<long long> unit;

    if (not karger_takes(s))
      {
        c.rejects.run();
        DynList<Node *> vs, vt;
        DynList<Arc *> cut;
        try
          {
            (void) karger(b.g, vs, vt, cut);
            c.rejects.fail(s, "cuts a graph that it does not take");
          }
        catch (const std::domain_error &) {}
        return;
      }

    const long long want = min_cut(s, unit);

    // Its iterations by default may miss the minimum: the cut must be one
    c.plain.run();
    {
      DynList<Node *> vs, vt;
      DynList<Arc *> cut;
      const long long got = static_cast<long long>(karger(b.g, vs, vt, cut));
      if (auto e = cut_error<GT>(s, unit, vs, vt, cut, got); not e.empty())
        c.plain.fail(s, e);
      else if (got < want)
        c.plain.fail(s, "a cut of " + std::to_string(got) + " under the minimum");
    }

    // Each iteration finds a given minimum cut with a probability of
    // 2 / (n (n - 1)) at least, so that 12 n^2 of them miss it with one
    // under 1e-12 for n <= 8
    const size_t iterations = 12 * size_t(s.n) * size_t(s.n);
    c.many.run();
    {
      DynList<Node *> vs, vt;
      DynList<Arc *> cut;
      const long long got = static_cast<long long>(karger(b.g, vs, vt, cut, iterations));
      if (auto e = cut_error<GT>(s, unit, vs, vt, cut, got); not e.empty())
        c.many.fail(s, e);
      else if (got != want)
        c.many.fail(s, "a cut of " + std::to_string(got) + ", not " + std::to_string(want));
    }

    c.size.run();
    if (const long long got = static_cast<long long>(karger.compute_min_cut_size(b.g, iterations)); got != want)
      c.size.fail(s, std::to_string(got) + ", not " + std::to_string(want));

    c.fast.run();
    {
      DynList<Node *> vs, vt;
      DynList<Arc *> cut;
      const long long got = static_cast<long long>(karger.fast(b.g, vs, vt, cut, 40));
      if (auto e = cut_error<GT>(s, unit, vs, vt, cut, got); not e.empty())
        c.fast.fail(s, e);
      else if (got != want)
        c.fast.fail(s, "a cut of " + std::to_string(got) + ", not " + std::to_string(want));
    }
  }
}

TYPED_TEST(ExhaustiveCut, KargerFindsAMinimumCut)
{
  Karger_Checks c;
  for_each_spec(domain(4, 3), false, [&] (const Spec & s) { check_karger<TypeParam>(c, s); });
  c.expect();
}

TYPED_TEST(ExhaustiveCut, KargerFindsAMinimumCutOfRandomGraphs)
{
  Karger_Checks c;
  std::mt19937 rng(seed(736));
  const int graphs = heavy() ? 1000 : 60;
  for (int k = 0; k < graphs; ++k)
    {
      const int n = 5 + int(rng() % 4);
      Spec s = random_spec(rng, n, n + int(rng() % unsigned(n)), false, false);
      check_karger<TypeParam>(c, s);
    }
  c.expect();
}
