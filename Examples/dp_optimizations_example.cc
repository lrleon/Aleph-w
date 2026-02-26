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
 * @file dp_optimizations_example.cc
 * @brief Illustrated use cases for DP_Optimizations.H.
 */

# include <iomanip>
# include <iostream>

# include <DP_Optimizations.H>

using namespace Aleph;

namespace
{
  void rule()
  {
    std::cout << "------------------------------------------------------------\n";
  }

  void show_divide_and_conquer_dp()
  {
    std::cout << "[1] Divide & Conquer DP optimization\n";
    rule();

    // Partition route-demand prefixes into balanced segments.
    Array<long long> demand = {5, 2, 7, 4, 6, 3, 8, 5, 9, 4};
    const size_t n = demand.size();
    const size_t groups = 3;

    Array<long long> pref = Array<long long>::create(n + 1);
    pref(0) = 0;
    for (size_t i = 0; i < n; ++i)
      pref(i + 1) = pref[i] + demand[i];

    const auto cost = [&] (const size_t k, const size_t i) -> long long
    {
      const long long seg = pref[i] - pref[k];
      return seg * seg;
    };

    const auto res = divide_and_conquer_partition_dp<long long>(
      groups, n, cost
    );

    std::cout << "Demand sequence: ";
    for (size_t i = 0; i < n; ++i)
      std::cout << demand[i] << (i + 1 == n ? '\n' : ' ');

    std::cout << "Groups: " << groups << "\n";
    std::cout << "Optimal cost: " << res.optimal_cost << "\n";

    Array<size_t> bounds;
    bounds.append(n);

    size_t i = n;
    for (size_t g = groups; g >= 2; --g)
      {
        const size_t k = res.split[g][i];
        bounds.append(k);
        i = k;
      }
    bounds.append(0);

    std::cout << "Split points (prefix indices): ";
    for (size_t p = bounds.size(); p > 0; --p)
      {
        std::cout << bounds[p - 1];
        if (p > 1)
          std::cout << " -> ";
      }
    std::cout << "\n";

    rule();
    std::cout << "\n";
  }

  void show_knuth_optimization()
  {
    std::cout << "[2] Knuth optimization (optimal adjacent merge)\n";
    rule();

    Array<size_t> blocks = {18, 7, 11, 5, 20, 9, 14};
    const auto res = optimal_merge_knuth(blocks);

    std::cout << "Blocks: ";
    for (size_t i = 0; i < blocks.size(); ++i)
      std::cout << blocks[i] << (i + 1 == blocks.size() ? '\n' : ' ');

    std::cout << "Minimum merge cost: " << res.optimal_cost << "\n";
    std::cout << "Top split (k for [0,n)): "
              << res.opt[0][blocks.size()] << "\n";

    rule();
    std::cout << "\n";
  }

  void show_cht_and_li_chao_geometry()
  {
    std::cout << "[3] CHT + Li Chao (geometry-friendly lower envelopes)\n";
    rule();

    std::cout << "Monotone-slope CHT for min y = m*x + b:\n";
    Convex_Hull_Trick<long long> cht;
    cht.add_line(5, -20);
    cht.add_line(2, -3);
    cht.add_line(-1, 10);
    cht.add_line(-4, 25);

    for (long long x = -4; x <= 8; x += 3)
      std::cout << "  x=" << std::setw(3) << x
                << "  min=" << std::setw(5) << cht.query(x) << "\n";

    std::cout << "\nGeometric application: weighted squared distance\n";
    std::cout << "  min_j ((x_i - x_j)^2 + w_j) via Li Chao\n\n";

    Array<long long> xs = {-10, -6, -1, 0, 4, 9, 13};
    Array<long long> ws = {9, 3, 7, 2, 5, 4, 8};

    const auto best = min_weighted_squared_distance_1d(xs, ws);

    std::cout << "  "
              << std::left << std::setw(8) << "x_i"
              << std::setw(8) << "w_i"
              << std::setw(12) << "min value"
              << "\n";

    for (size_t i = 0; i < xs.size(); ++i)
      {
        std::cout << "  "
                  << std::left << std::setw(8) << xs[i]
                  << std::setw(8) << ws[i]
                  << std::setw(12) << best[i]
                  << "\n";
      }

    rule();
    std::cout << "\n";
  }

  void show_monotone_queue_dp()
  {
    std::cout << "[4] Monotone queue optimization (windowed transitions)\n";
    rule();

    // dp[i] = base[i] + min(dp[j]), j in [i-window, i-1]
    Array<long long> base = {6, 4, 7, 3, 5, 2, 8, 1, 4, 3};
    const size_t window = 3;

    const auto res = monotone_queue_min_dp<long long>(base, window);

    std::cout << "Base cost: ";
    for (size_t i = 0; i < base.size(); ++i)
      std::cout << base[i] << (i + 1 == base.size() ? '\n' : ' ');

    std::cout << "Window: " << window << "\n";
    std::cout << "Final minimum cost: " << res.dp[base.size() - 1] << "\n";

    Array<size_t> path;
    size_t i = base.size() - 1;
    path.append(i);
    while (i > 0)
      {
        i = res.parent[i];
        path.append(i);
      }

    std::cout << "Chosen chain: ";
    for (size_t p = path.size(); p > 0; --p)
      {
        std::cout << path[p - 1];
        if (p > 1)
          std::cout << " -> ";
      }
    std::cout << "\n";

    rule();
    std::cout << "\n";
  }
} // namespace

int main()
{
  std::cout << "\n=== DP Optimizations: D&C, Knuth, CHT/Li Chao, Monotone Queue ===\n\n";

  show_divide_and_conquer_dp();
  show_knuth_optimization();
  show_cht_and_li_chao_geometry();
  show_monotone_queue_dp();

  std::cout << "Done.\n";
  return 0;
}
