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
 * @file dp_optimizations_test.cc
 * @brief Tests for DP_Optimizations.H.
 */

# include <gtest/gtest.h>

# include <limits>
# include <random>
# include <chrono>
# include <numeric>
# include <ctime>
# include <cstdlib>

# include <DP_Optimizations.H>

using namespace Aleph;

namespace
{
  constexpr long long INF64 = std::numeric_limits<long long>::max() / 4;

  Array<Array<long long>> brute_partition_dp(const size_t groups,
                                             const size_t n,
                                             const auto & cost)
  {
    Array<Array<long long>> dp;
    dp.reserve(groups + 1);
    for (size_t g = 0; g <= groups; ++g)
      {
        Array<long long> row = Array<long long>::create(n + 1);
        for (size_t i = 0; i <= n; ++i)
          row(i) = INF64;
        dp.append(std::move(row));
      }

    dp(0)(0) = 0;

    for (size_t g = 1; g <= groups; ++g)
      for (size_t i = g; i <= n; ++i)
        for (size_t k = g - 1; k < i; ++k)
          {
            if (dp[g - 1][k] == INF64)
              continue;
            const long long cand = dp[g - 1][k] + cost(k, i);
            if (cand < dp[g][i])
              dp(g)(i) = cand;
          }

    return dp;
  }

  size_t brute_optimal_merge(const Array<size_t> & w)
  {
    const size_t n = w.size();
    if (n <= 1)
      return 0;

    Array<size_t> pref = Array<size_t>::create(n + 1);
    pref(0) = 0;
    for (size_t i = 0; i < n; ++i)
      pref(i + 1) = pref[i] + w[i];

    Array<Array<size_t>> dp;
    dp.reserve(n + 1);
    for (size_t i = 0; i <= n; ++i)
      {
        Array<size_t> row = Array<size_t>::create(n + 1);
        for (size_t j = 0; j <= n; ++j)
          row(j) = 0;
        dp.append(std::move(row));
      }

    for (size_t len = 2; len <= n; ++len)
      for (size_t i = 0; i + len <= n; ++i)
        {
          const size_t j = i + len;
          size_t best = std::numeric_limits<size_t>::max();
          const size_t seg = pref[j] - pref[i];
          for (size_t k = i + 1; k < j; ++k)
            {
              const size_t cand = dp[i][k] + dp[k][j] + seg;
              if (cand < best)
                best = cand;
            }
          dp(i)(j) = best;
        }

    return dp[0][n];
  }

  long long brute_windowed_dp(const Array<long long> & base, const size_t w)
  {
    if (base.size() == 0)
      return 0;
    if (base.size() > 1 and w == 0)
      return INF64;

    Array<long long> dp = Array<long long>::create(base.size());
    dp(0) = base[0];

    for (size_t i = 1; i < base.size(); ++i)
      {
        const size_t l = (i > w) ? (i - w) : 0;
        long long best = INF64;
        for (size_t j = l; j < i; ++j)
          if (dp[j] < best)
            best = dp[j];
        dp(i) = best + base[i];
      }

    return dp[base.size() - 1];
  }

  long long brute_weighted_sq_dist(const Array<long long> & xs,
                                   const Array<long long> & ws,
                                   const size_t i)
  {
    long long best = INF64;
    for (size_t j = 0; j < xs.size(); ++j)
      {
        const long long d = xs[i] - xs[j];
        const long long cand = d * d + ws[j];
        if (cand < best)
          best = cand;
      }
    return best;
  }
} // namespace


TEST(DPOptimizations, DivideConquerPartitionMatchesBruteForce)
{
  std::mt19937 rng(20260226);

  for (size_t trial = 0; trial < 120; ++trial)
    {
      const size_t n = 1 + (rng() % 36);
      const size_t groups = 1 + (rng() % std::min<size_t>(6, n));

      Array<long long> a;
      a.reserve(n);
      for (size_t i = 0; i < n; ++i)
        a.append(1 + static_cast<long long>(rng() % 11));

      Array<long long> pref = Array<long long>::create(n + 1);
      pref(0) = 0;
      for (size_t i = 0; i < n; ++i)
        pref(i + 1) = pref[i] + a[i];

      const auto cost = [&] (const size_t k, const size_t i) -> long long
      {
        const long long d = pref[i] - pref[k];
        return d * d;
      };

      const auto fast = divide_and_conquer_partition_dp<long long>(
        groups, n, cost, INF64
      );
      const auto slow = brute_partition_dp(groups, n, cost);

      EXPECT_EQ(fast.optimal_cost, slow[groups][n]);

      for (size_t g = 1; g <= groups; ++g)
        for (size_t i = g + 1; i <= n; ++i)
          EXPECT_LE(fast.split[g][i - 1], fast.split[g][i]);
    }
}

TEST(DPOptimizations, DivideConquerPartitionEdgeCases)
{
  const auto cost = [] (const size_t k, const size_t i) -> long long
  {
    const long long d = static_cast<long long>(i - k);
    return d * d;
  };

  const auto r0 = divide_and_conquer_partition_dp<long long>(0, 0, cost, INF64);
  EXPECT_EQ(r0.optimal_cost, 0);

  const auto r1 = divide_and_conquer_partition_dp<long long>(0, 5, cost, INF64);
  EXPECT_EQ(r1.optimal_cost, INF64);

  const auto r2 = divide_and_conquer_partition_dp<long long>(7, 3, cost, INF64);
  EXPECT_EQ(r2.optimal_cost, INF64);
}

TEST(DPOptimizations, KnuthOptimalMergeMatchesBruteForce)
{
  std::mt19937 rng(20260226 + 7);

  for (size_t trial = 0; trial < 150; ++trial)
    {
      const size_t n = rng() % 35;
      Array<size_t> w;
      w.reserve(n);
      for (size_t i = 0; i < n; ++i)
        w.append(static_cast<size_t>(rng() % 25));

      const auto fast = optimal_merge_knuth(w);
      const size_t slow = brute_optimal_merge(w);
      EXPECT_EQ(fast.optimal_cost, slow);

      if (n >= 2)
        for (size_t len = 3; len <= n; ++len)
          for (size_t i = 0; i + len <= n; ++i)
            {
              const size_t j = i + len;
              EXPECT_LE(fast.opt[i][j - 1], fast.opt[i][j]);
              EXPECT_LE(fast.opt[i][j], fast.opt[i + 1][j]);
            }
    }
}

TEST(DPOptimizations, KnuthOptimalMergeEdgeCases)
{
  Array<size_t> empty;
  auto e = optimal_merge_knuth(empty);
  EXPECT_EQ(e.optimal_cost, 0u);

  Array<size_t> one = {42};
  auto o = optimal_merge_knuth(one);
  EXPECT_EQ(o.optimal_cost, 0u);
}

TEST(DPOptimizations, ConvexHullTrickMatchesBruteForce)
{
  Convex_Hull_Trick<long long> cht;
  Array<Convex_Hull_Trick<long long>::Line> lines;

  const Array<long long> slopes = {7, 4, 2, -1, -3, -5};
  const Array<long long> intercepts = {-10, -4, -3, 0, 5, 9};

  for (size_t i = 0; i < slopes.size(); ++i)
    {
      cht.add_line(slopes[i], intercepts[i]);
      lines.append(Convex_Hull_Trick<long long>::Line{slopes[i], intercepts[i]});
    }

  for (long long x = -30; x <= 30; ++x)
    {
      long long best = INF64;
      for (size_t i = 0; i < lines.size(); ++i)
        {
          const long long v = lines[i].value_at(x);
          if (v < best)
            best = v;
        }
      EXPECT_EQ(cht.query(x), best);
    }

  cht.reset_query_cursor();
  for (long long x = -30; x <= 30; ++x)
    {
      long long best = INF64;
      for (size_t i = 0; i < lines.size(); ++i)
        {
          const long long v = lines[i].value_at(x);
          if (v < best)
            best = v;
        }
      EXPECT_EQ(cht.query_monotone(x), best);
    }
}

TEST(DPOptimizations, ConvexHullTrickSlopeChecksAndDuplicates)
{
  Convex_Hull_Trick<long long> cht;

  EXPECT_THROW(cht.query(0), std::runtime_error);

  cht.add_line(1, 5);
  cht.add_line(1, 7); // dominated (same slope, higher intercept)
  EXPECT_EQ(cht.size(), 1u);

  cht.add_line(1, 3); // better same slope should replace
  EXPECT_EQ(cht.size(), 1u);
  EXPECT_EQ(cht.query(10), 13);

  EXPECT_THROW(cht.add_line(2, 0), std::domain_error);
}

TEST(DPOptimizations, LiChaoMatchesBruteForce)
{
  Li_Chao_Tree<long long> lc(-200, 200);
  Array<Li_Chao_Tree<long long>::Line> lines;

  std::mt19937 rng(20260226 + 17);
  for (size_t i = 0; i < 80; ++i)
    {
      const long long m = static_cast<long long>(rng() % 51) - 25;
      const long long b = static_cast<long long>(rng() % 401) - 200;
      lc.add_line(m, b);
      lines.append(Li_Chao_Tree<long long>::Line{m, b});
    }

  for (long long x = -200; x <= 200; x += 3)
    {
      long long best = INF64;
      for (size_t i = 0; i < lines.size(); ++i)
        {
          const long long cand = lines[i].value_at(x);
          if (cand < best)
            best = cand;
        }
      EXPECT_EQ(lc.query(x), best);
    }
}

TEST(DPOptimizations, LiChaoDomainAndEmptyChecks)
{
  EXPECT_THROW(Li_Chao_Tree<long long>(5, 2), std::domain_error);

  Li_Chao_Tree<long long> lc(-10, 10);
  EXPECT_THROW(lc.query(0), std::runtime_error);

  lc.add_line(3, 1);
  EXPECT_THROW(lc.query(11), std::out_of_range);
}

TEST(DPOptimizations, MonotoneQueueDPMatchesBruteForce)
{
  std::mt19937 rng(20260226 + 31);

  for (size_t trial = 0; trial < 220; ++trial)
    {
      const size_t n = 1 + (rng() % 200);
      const size_t w = 1 + (rng() % 20);

      Array<long long> base;
      base.reserve(n);
      for (size_t i = 0; i < n; ++i)
        base.append(1 + static_cast<long long>(rng() % 50));

      const auto fast = monotone_queue_min_dp<long long>(base, w);
      const long long slow = brute_windowed_dp(base, w);

      EXPECT_EQ(fast.dp[base.size() - 1], slow);

      for (size_t i = 1; i < n; ++i)
        {
          EXPECT_LT(fast.parent[i], i);
          EXPECT_LE(i - fast.parent[i], w);
        }
    }
}

TEST(DPOptimizations, MonotoneQueueDPEdgeCases)
{
  Array<long long> empty;
  auto re = monotone_queue_min_dp<long long>(empty, 5);
  EXPECT_EQ(re.dp.size(), 0u);

  Array<long long> one = {17};
  auto r1 = monotone_queue_min_dp<long long>(one, 0);
  EXPECT_EQ(r1.dp.size(), 1u);
  EXPECT_EQ(r1.dp[0], 17);

  Array<long long> two = {1, 2};
  EXPECT_THROW(monotone_queue_min_dp<long long>(two, 0), std::domain_error);
}

TEST(DPOptimizations, WeightedSquaredDistanceMatchesBruteForce)
{
  Array<long long> xs = {-9, -4, -1, 0, 3, 8, 11};
  Array<long long> ws = {5, 2, 7, 4, 1, 9, 6};

  auto got = min_weighted_squared_distance_1d(xs, ws);
  ASSERT_EQ(got.size(), xs.size());

  for (size_t i = 0; i < xs.size(); ++i)
    EXPECT_EQ(got[i], brute_weighted_sq_dist(xs, ws, i));
}

TEST(DPOptimizations, WeightedSquaredDistanceRandomVsBrute)
{
  std::mt19937 rng(20260226 + 101);

  for (size_t trial = 0; trial < 120; ++trial)
    {
      const size_t n = 1 + (rng() % 60);

      Array<long long> xs;
      Array<long long> ws;
      xs.reserve(n);
      ws.reserve(n);

      for (size_t i = 0; i < n; ++i)
        {
          xs.append(static_cast<long long>(rng() % 101) - 50);
          ws.append(static_cast<long long>(rng() % 80));
        }

      const auto got = min_weighted_squared_distance_1d(xs, ws);
      for (size_t i = 0; i < n; ++i)
        EXPECT_EQ(got[i], brute_weighted_sq_dist(xs, ws, i));
    }
}

TEST(DPOptimizations, WeightedSquaredDistanceValidatesSizes)
{
  Array<long long> xs = {1, 2, 3};
  Array<long long> ws = {4, 5};
  EXPECT_THROW(min_weighted_squared_distance_1d(xs, ws), std::domain_error);
}

TEST(DPOptimizations, DivideConquerPartitionPerf)
{
  if (std::getenv("RUN_PERF_TESTS") == nullptr)
    GTEST_SKIP() << "Skipping performance test; set RUN_PERF_TESTS=1 to enable";

  // Deterministic performance check for D&C DP optimizer.
  // Complexity O(groups * n * log n). For these parameters, ~2.2M iterations.
  const size_t groups = 100;
  const size_t n = 2000;
  
  // Cost function lambda matching the pattern in correctness tests
  auto cost = [](size_t i, size_t j) -> long long 
  {
    const long long diff = static_cast<long long>(j - i);
    return diff * diff;
  };

  const std::clock_t start_clock = std::clock();
  const auto res = divide_and_conquer_partition_dp<long long>(groups, n, cost);
  const std::clock_t end_clock = std::clock();
  
  const double cpu_elapsed_ms = (static_cast<double>(end_clock - start_clock) * 1000.0) / CLOCKS_PER_SEC;
  
  // Conservative 500ms CPU-time threshold to avoid CI flakiness.
  EXPECT_LT(cpu_elapsed_ms, 500.0) 
    << "Performance regression: divide_and_conquer_partition_dp took " << cpu_elapsed_ms << "ms CPU time";
  
  // We split the range [0, n) into groups intervals. Given the cost(i, j) = (j-i)^2
  // model, the optimal cost res.optimal_cost is achieved with equal-sized splits,
  // yielding exactly cost(0, n) / groups, which is roughly n^2 / groups.
  EXPECT_EQ(res.optimal_cost, cost(0, n) / groups); 
}
