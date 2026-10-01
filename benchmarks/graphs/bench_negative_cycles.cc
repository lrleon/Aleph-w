/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_negative_cycles.cc
 * @brief Compares negative cycle detection in `Bellman_Ford` (Bellman_Ford.H)
 *        against `find_disjoint_negative_cycles()` (Negative_Cycles.H).
 *
 * The graph models a crypto market: `n` assets, a few hub assets (think
 * USDT/BTC/ETH/BNB) traded against every other asset, plus random pairs.
 * Each pair is traded both ways with weight `-log(rate * (1 - fee))`,
 * derived from consistent prices, so the graph has no negative cycle
 * unless arbitrage triangles are seeded on purpose.
 *
 * Two scenarios are measured:
 *  - `arbitrage`: a few disjoint triangles are made negative;
 *  - `no-arbitrage`: consistent prices only, no negative cycle.
 *
 * Contenders (global search, i.e. over the whole graph):
 *  - `BF::has_negative_cycle()`        classic, detection only;
 *  - `BF::test_negative_cycle()`       classic + cycle extraction (Tarjan);
 *  - `BF::search_negative_cycle()`     SPFA + cycle extraction;
 *  - `BF::search_negative_cycle(f, s)` SPFA with early cycle checks;
 *  - `find_disjoint_negative_cycles(g, 1)` one round, with extraction;
 *  - `find_disjoint_negative_cycles(g, k)` k rounds (enumeration).
 *
 * Informative benchmark, not wired to any performance gate.
 * Usage: bench_negative_cycles [num_assets] [repeats] [seed]
 */

#include <algorithm>
#include <chrono>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <random>

#include <Bellman_Ford.H>
#include <Negative_Cycles.H>
#include <tpl_array.H>
#include <tpl_graph.H>

using namespace Aleph;

namespace
{

using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using Node = Graph::Node;

volatile long sink = 0;  // keeps results observable for the optimizer

constexpr size_t num_hubs = 4;
constexpr double fee = 0.001;            // 0.1 % per trade
constexpr size_t seeded_triangles = 5;
constexpr double triangle_profit = 0.012; // -log of the gain of one triangle

// Runs fn `repeats` times and returns the best wall-clock milliseconds.
template <class Fn>
double best_ms(Fn fn, const int repeats)
{
  double best = 1e300;
  for (int r = 0; r < repeats; ++r)
    {
      const auto t0 = std::chrono::steady_clock::now();
      fn();
      const auto t1 = std::chrono::steady_clock::now();
      best = std::min(best,
                      std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
  return best;
}

void row(const char * name, const double ms, const char * found)
{
  std::printf("  %-44s %10.3f ms   %s\n", name, ms, found);
}

// Builds the market graph. With `arbitrage`, `seeded_triangles` disjoint
// triangles among non-hub assets get arcs whose total weight is
// `-triangle_profit`.
Graph build_market(const size_t n, const bool arbitrage, const unsigned seed)
{
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> log_price(-3.0, 3.0);

  Graph g;
  Array<Node *> nodes;
  nodes.reserve(n);
  Array<double> lp;
  lp.reserve(n);
  for (size_t i = 0; i < n; ++i)
    {
      nodes.append(g.insert_node(static_cast<int>(i)));
      lp.append(log_price(rng));
    }

  const double f = -std::log(1.0 - fee);
  auto weight = [&](const size_t u, const size_t v) { return lp(v) - lp(u) + f; };
  auto both_ways = [&](const size_t u, const size_t v)
  {
    g.insert_arc(nodes(u), nodes(v), weight(u, v));
    g.insert_arc(nodes(v), nodes(u), weight(v, u));
  };

  for (size_t h = 0; h < num_hubs; ++h)
    for (size_t v = h + 1; v < n; ++v)
      both_ways(h, v);

  std::uniform_int_distribution<size_t> pick(num_hubs, n - 1);
  for (size_t k = 0; k < n; ++k)
    {
      const size_t u = pick(rng);
      const size_t v = pick(rng);
      if (u != v)
        both_ways(u, v);
    }

  if (arbitrage)
    for (size_t t = 0; t < seeded_triangles; ++t)
      {
        const size_t a = num_hubs + 3 * t;
        const size_t b = a + 1;
        const size_t c = a + 2;
        if (c >= n)
          break;
        const double cut = f + triangle_profit / 3.0;
        g.insert_arc(nodes(a), nodes(b), weight(a, b) - cut);
        g.insert_arc(nodes(b), nodes(c), weight(b, c) - cut);
        g.insert_arc(nodes(c), nodes(a), weight(c, a) - cut);
      }

  return g;
}

void run_scenario(const char * title, const size_t n, const bool arbitrage,
                  const int repeats, const unsigned seed)
{
  Graph g = build_market(n, arbitrage, seed);
  std::printf("\n== %s: %zu assets, %zu arcs ==\n", title,
              g.get_num_nodes(), g.get_num_arcs());

  // The measured callable sets `found`; time first, print after, so the
  // printed outcome never depends on argument evaluation order.
  bool found = false;
  auto measure = [&](const char * name, auto fn)
  {
    const double ms = best_ms(fn, repeats);
    row(name, ms, found ? "cycle found" : "no cycle");
  };

  measure("BF::has_negative_cycle() [classic]",
          [&] { found = Bellman_Ford<Graph>(g).has_negative_cycle();
                sink += found; });

  measure("BF::test_negative_cycle() [classic+Tarjan]",
          [&] { const auto p = Bellman_Ford<Graph>(g).test_negative_cycle();
                found = not p.is_empty();
                sink += static_cast<long>(p.size()); });

  measure("BF::search_negative_cycle() [SPFA]",
          [&] { const auto p = Bellman_Ford<Graph>(g).search_negative_cycle();
                found = not p.is_empty();
                sink += static_cast<long>(p.size()); });

  measure("BF::search_negative_cycle(0.1, 1) [SPFA+early]",
          [&] { const auto r = Bellman_Ford<Graph>(g).search_negative_cycle(0.1, 1);
                found = not std::get<0>(r).is_empty();
                sink += static_cast<long>(std::get<1>(r)); });

  measure("find_disjoint_negative_cycles(g, 1)",
          [&] { const auto l = find_disjoint_negative_cycles(g, 1);
                found = not l.is_empty();
                sink += static_cast<long>(l.size()); });

  size_t k_found = 0;
  const double ms_k =
    best_ms([&] { const auto l = find_disjoint_negative_cycles(g, seeded_triangles);
                  k_found = l.size();
                  sink += static_cast<long>(k_found); }, repeats);
  char label[64];
  std::snprintf(label, sizeof(label), "find_disjoint_negative_cycles(g, %zu)",
                seeded_triangles);
  char found_k[32];
  std::snprintf(found_k, sizeof(found_k), "%zu cycles", k_found);
  row(label, ms_k, found_k);
}

} // namespace

int main(int argc, char * argv[])
{
  const size_t n = argc > 1 ? std::strtoul(argv[1], nullptr, 10) : 300;
  const int repeats = argc > 2 ? std::atoi(argv[2]) : 5;
  const unsigned seed = argc > 3 ? static_cast<unsigned>(std::strtoul(argv[3], nullptr, 10)) : 42u;

  if (n < num_hubs + 3 * seeded_triangles)
    {
      std::fprintf(stderr, "num_assets must be at least %zu\n",
                   num_hubs + 3 * seeded_triangles);
      return 1;
    }

  std::printf("Negative cycle detection benchmark (best of %d runs, seed %u)\n",
              repeats, seed);
  run_scenario("arbitrage", n, true, repeats, seed);
  run_scenario("no-arbitrage", n, false, repeats, seed);

  return sink == -1 ? 1 : 0;
}
