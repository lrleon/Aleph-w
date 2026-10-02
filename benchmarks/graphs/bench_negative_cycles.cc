/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_negative_cycles.cc
 * @brief Benchmark and result validator for negative cycle detection:
 *        `Bellman_Ford` (Bellman_Ford.H) against `find_disjoint_negative_cycles()`
 *        and `most_negative_cycle_bounded()` (Negative_Cycles.H), with
 *        Karp's minimum mean cycle (Min_Mean_Cycle.H) as a reference.
 *
 * Usage:
 * @code
 * bench_negative_cycles [num_assets] [samples] [seed] [--check-only] [--only=<text>]
 * @endcode
 *
 * - `num_assets` in [19, 20000] (default 300);
 * - `samples` timed runs per measurement, in [1, 100000] (default 31; fewer
 *   than 30 are flagged as exploratory);
 * - `seed` in [0, 4294967295] (default 42);
 * - `--check-only` runs the validation pass only (no timing);
 * - `--only=<text>` keeps the graph families whose name contains `<text>`.
 *
 * Every argument must be a complete decimal integer in its range; anything
 * else is rejected before a graph is built (exit code 2).
 *
 * @par Graph families
 * `market-arbitrage` and `market-clean` model a crypto market: `n` assets,
 * four hub assets traded against every other one, plus random pairs, each pair
 * traded both ways with weight `-log(rate * (1 - fee))` taken from consistent
 * prices, so `market-clean` has no negative cycle and `market-arbitrage` has
 * five seeded triangles (as parallel arcs: the graphs are multigraphs).
 * The other families stress the structure the bounded search is expected to
 * exploit: `market-arbitrage-reversed` (same arcs, reverse insertion order),
 * `market-isolated-nodes` (as many isolated vertices as assets), `dag`
 * (acyclic, negative arcs), `components` (disconnected strongly connected
 * components, one in three holding a negative triangle), `dense` (complete
 * digraph on at most 120 assets), `market-reweighted` (fixed topology, eight
 * weight sets applied round-robin between the timed samples) and
 * `loop-isolated` (one negative self-loop among 50 isolated vertices per
 * asset, at most 20000; the bounded variants run whatever its size).
 *
 * @par Validation (untimed, before and apart from the measurements)
 * Each variant is run once per weight set and its result is checked against
 * the ground truth known by construction. A reported cycle must be closed,
 * simple, made of consecutive arcs of the graph and of strictly negative
 * cost (the reported cost must also match the arc sum); at most the requested
 * number of cycles, pairwise distinct, and pairwise arc-disjoint under
 * `All_Arcs`; at most `L` arcs for the bounded search. A negative cycle must
 * be found where one was seeded and none may be reported where none exists.
 * Results of the new algorithms that fail a check make the program exit with
 * code 1.
 *
 * `Bellman_Ford`'s witness extraction rebuilds the cycle from nodes and can
 * pick a different parallel arc, so on multigraphs it can return a cycle whose
 * cost is not negative (for instance `n = 19` with seeds 19 and 42, which are
 * always validated). Those results are printed as `INVALID` and counted as
 * known Bellman-Ford defects: they do not change the exit code, because fixing
 * the old extractor is a separate task. A missed or false detection is still
 * an error.
 *
 * @par Timing protocol
 * An untimed warmup (at least 2 runs, then until 30 ms have elapsed, at most
 * 200 runs), then `samples` timed runs; the table shows the median, the 90th
 * percentile (nearest rank) and the best time. Weight changes
 * of `market-reweighted` happen outside the timed region. The allocation
 * columns count calls to the global `operator new` during one untimed run.
 * The bounded variants are skipped above 1000 vertices (their worst case is
 * `O(V * L * (V + E))`), except for `loop-isolated`; so are the Karp variants,
 * whose table takes `O(V^2)` memory (about 9 bytes per entry for the value and
 * 25 with the witness, measured: 15 000 vertices would need 2 to 6 GB).
 *
 * @par Karp rows
 * `karp_minimum_mean_cycle_value()` and `karp_minimum_mean_cycle()` solve a
 * different problem: the cycle of minimum *mean* weight, not the cheapest one.
 * They are here to decide whether a faster mean-cycle backend (Howard's
 * algorithm) is worth having. Their witness is a closed *walk* that may go
 * around a cycle many times (594 arcs on `market-isolated-nodes`), as
 * documented in Min_Mean_Cycle.H, so the validator checks that it is closed,
 * made of consecutive arcs and has the reported total, length and mean, but
 * not that it is simple.
 */

#include <algorithm>
#include <charconv>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <functional>
#include <memory>
#include <new>
#include <random>
#include <string>
#include <system_error>
#include <tuple>
#include <vector>

#include <Bellman_Ford.H>
#include <Min_Mean_Cycle.H>
#include <Negative_Cycles.H>
#include <tpl_array.H>
#include <tpl_dynList.H>
#include <tpl_dynSetTree.H>
#include <tpl_graph.H>

#include "../ca/bench_support.H"

using namespace Aleph;

// ---------------------------------------------------------------------------
// Allocation probe: counts calls to the global operator new while `active`.
// ---------------------------------------------------------------------------

namespace
{
struct Alloc_Probe
{
  bool active = false;
  size_t calls = 0;
  size_t bytes = 0;
};

Alloc_Probe alloc_probe;
} // namespace

// The replacement functions are not inlined: GCC cannot then pair a malloc
// behind `operator new` with a free behind `operator delete` and warn about a
// mismatch (-Wmismatched-new-delete), and the probe costs the same everywhere.
#if defined(__GNUC__)
#define BENCH_NOINLINE __attribute__((noinline))
#else
#define BENCH_NOINLINE
#endif

BENCH_NOINLINE void * operator new(const std::size_t size)
{
  if (alloc_probe.active)
    {
      ++alloc_probe.calls;
      alloc_probe.bytes += size;
    }
  if (void * p = std::malloc(size == 0 ? 1 : size))
    return p;
  std::fputs("bench_negative_cycles: out of memory\n", stderr);
  std::abort();
}

BENCH_NOINLINE void * operator new[](const std::size_t size) { return operator new(size); }
BENCH_NOINLINE void operator delete(void * p) noexcept { std::free(p); }
BENCH_NOINLINE void operator delete[](void * p) noexcept { std::free(p); }
BENCH_NOINLINE void operator delete(void * p, std::size_t) noexcept { std::free(p); }
BENCH_NOINLINE void operator delete[](void * p, std::size_t) noexcept { std::free(p); }

namespace
{

using Graph = List_Digraph<Graph_Node<int>, Graph_Arc<double>>;
using Node = Graph::Node;
using Arc = Graph::Arc;
using Cycles = DynList<Negative_Cycle_Item<Graph, double>>;
using Bounded = Bounded_Cycle_Result<Graph, double>;
using Indexed = negative_cycles_detail::Indexed_Graph<Graph, double>;
using Exclusion = Negative_Cycle_Exclusion;

volatile long sink = 0;  // keeps results observable for the optimizer

// `sink += v` on a volatile is deprecated in C++20.
void keep(const long v) { sink = sink + v; }

constexpr size_t num_hubs = 4;
constexpr double fee = 0.001;              // 0.1 % per trade
constexpr size_t seeded_triangles = 5;
constexpr double triangle_profit = 0.012;  // -log of the gain of one triangle
constexpr size_t min_assets = num_hubs + 3 * seeded_triangles;
constexpr size_t max_assets = 20000;
constexpr size_t max_samples = 100000;
constexpr size_t min_regression_samples = 30;
constexpr size_t bounded_max_nodes = 1000;  // bounded variants are skipped above this
constexpr size_t dense_max_nodes = 120;
constexpr size_t reweight_rounds = 8;
constexpr size_t component_size = 12;
constexpr size_t loop_isolated_factor = 50;  // isolated vertices per asset
constexpr size_t loop_isolated_max = 20000;
constexpr size_t warmup_min_runs = 2;
constexpr size_t warmup_max_runs = 200;
constexpr double warmup_ms = 30.0;

// ---------------------------------------------------------------------------
// Argument parsing
// ---------------------------------------------------------------------------

// Parses the whole of `text` as a decimal unsigned integer in [lo, hi].
// No sign, blank, prefix or trailing character is accepted.
bool parse_uint(const char * text, const unsigned long long lo,
                const unsigned long long hi, unsigned long long & out)
{
  if (text == nullptr or *text == '\0')
    return false;
  const char * end = text + std::strlen(text);
  unsigned long long value = 0;
  const auto [ptr, ec] = std::from_chars(text, end, value);
  if (ec != std::errc() or ptr != end or value < lo or value > hi)
    return false;
  out = value;
  return true;
}

struct Options
{
  size_t assets = 300;
  size_t samples = 31;
  unsigned seed = 42;
  bool check_only = false;
  std::string only;
};

void usage(std::FILE * out)
{
  std::fprintf(out,
    "usage: bench_negative_cycles [num_assets] [samples] [seed] [--check-only] [--only=<text>]\n"
    "  num_assets  integer in [%zu, %zu]            (default 300)\n"
    "  samples     integer in [1, %zu]              (default 31; < %zu is exploratory)\n"
    "  seed        integer in [0, 4294967295]       (default 42)\n"
    "  --check-only   validate the results, do not time anything\n"
    "  --only=<text>  keep the graph families whose name contains <text>\n",
    min_assets, max_assets, max_samples, min_regression_samples);
}

// Returns true if the command line is valid; otherwise prints why.
bool parse_args(const int argc, char * argv[], Options & opt)
{
  struct Positional
  {
    const char * name;
    unsigned long long lo;
    unsigned long long hi;
  };
  const Positional positional[] =
    {{"num_assets", min_assets, max_assets},
     {"samples", 1, max_samples},
     {"seed", 0, 4294967295ULL}};

  size_t next = 0;
  for (int i = 1; i < argc; ++i)
    {
      const char * arg = argv[i];
      if (std::strcmp(arg, "--check-only") == 0)
        opt.check_only = true;
      else if (std::strncmp(arg, "--only=", 7) == 0)
        opt.only = arg + 7;
      else if (std::strcmp(arg, "--help") == 0 or std::strcmp(arg, "-h") == 0)
        {
          usage(stdout);
          std::exit(0);
        }
      else if (std::strncmp(arg, "--", 2) == 0)
        {
          std::fprintf(stderr, "unknown option '%s'\n", arg);
          return false;
        }
      else if (next >= 3)
        {
          std::fprintf(stderr, "unexpected argument '%s'\n", arg);
          return false;
        }
      else
        {
          const Positional & p = positional[next];
          unsigned long long value = 0;
          if (not parse_uint(arg, p.lo, p.hi, value))
            {
              std::fprintf(stderr,
                           "%s must be a decimal integer in [%llu, %llu], got '%s'\n",
                           p.name, p.lo, p.hi, arg);
              return false;
            }
          if (next == 0)
            opt.assets = static_cast<size_t>(value);
          else if (next == 1)
            opt.samples = static_cast<size_t>(value);
          else
            opt.seed = static_cast<unsigned>(value);
          ++next;
        }
    }
  return true;
}

// ---------------------------------------------------------------------------
// Graph specifications and instances
// ---------------------------------------------------------------------------

struct Arc_Spec
{
  size_t u;
  size_t v;
  double w;
  bool cut;   // seeded-triangle arc (market weights only)
};

struct Spec
{
  size_t nodes = 0;
  size_t isolated = 0;      // extra vertices without arcs
  Array<double> lp;         // log-price per node (market families)
  Array<Arc_Spec> arcs;     // in insertion order
  size_t seeded = 0;        // arc-disjoint negative cycles planted
  bool negative = false;    // ground truth: a negative cycle exists
};

double fee_term() { return -std::log(1.0 - fee); }

double market_weight(const Spec & s, const Arc_Spec & a)
{
  const double w = s.lp(a.v) - s.lp(a.u) + fee_term();
  return a.cut ? w - (fee_term() + triangle_profit / 3.0) : w;
}

void draw_prices(Spec & s, std::mt19937 & rng)
{
  std::uniform_real_distribution<double> log_price(-3.0, 3.0);
  for (size_t i = 0; i < s.nodes; ++i)
    s.lp(i) = log_price(rng);
}

void both_ways(Spec & s, const size_t u, const size_t v)
{
  s.arcs.append(Arc_Spec{u, v, market_weight(s, {u, v, 0, false}), false});
  s.arcs.append(Arc_Spec{v, u, market_weight(s, {v, u, 0, false}), false});
}

void plant_triangles(Spec & s)
{
  for (size_t t = 0; t < seeded_triangles; ++t)
    {
      const size_t a = num_hubs + 3 * t;
      const size_t b = a + 1;
      const size_t c = a + 2;
      if (c >= s.nodes)
        break;
      for (const auto & uv : {std::pair<size_t, size_t>{a, b}, {b, c}, {c, a}})
        {
          Arc_Spec arc{uv.first, uv.second, 0, true};
          arc.w = market_weight(s, arc);
          s.arcs.append(arc);
        }
      ++s.seeded;
    }
  s.negative = s.seeded > 0;
}

// The market of the original benchmark; the random draws keep their order so
// that a given (n, seed) is the same multigraph as before.
Spec market_spec(const size_t n, const bool arbitrage, const unsigned seed)
{
  std::mt19937 rng(seed);
  std::uniform_real_distribution<double> log_price(-3.0, 3.0);

  Spec s;
  s.nodes = n;
  for (size_t i = 0; i < n; ++i)
    s.lp.append(log_price(rng));

  for (size_t h = 0; h < num_hubs; ++h)
    for (size_t v = h + 1; v < n; ++v)
      both_ways(s, h, v);

  std::uniform_int_distribution<size_t> pick(num_hubs, n - 1);
  for (size_t k = 0; k < n; ++k)
    {
      const size_t u = pick(rng);
      const size_t v = pick(rng);
      if (u != v)
        both_ways(s, u, v);
    }

  if (arbitrage)
    plant_triangles(s);
  return s;
}

// Complete digraph with consistent prices and one planted triangle.
Spec dense_spec(const size_t n, const unsigned seed)
{
  std::mt19937 rng(seed + 1);
  Spec s;
  s.nodes = n;
  for (size_t i = 0; i < n; ++i)
    s.lp.append(0.0);
  draw_prices(s, rng);
  for (size_t u = 0; u < n; ++u)
    for (size_t v = 0; v < n; ++v)
      if (u != v)
        s.arcs.append(Arc_Spec{u, v, market_weight(s, {u, v, 0, false}), false});
  for (const auto & uv : {std::pair<size_t, size_t>{0, 1}, {1, 2}, {2, 0}})
    {
      Arc_Spec arc{uv.first, uv.second, 0, true};
      arc.w = market_weight(s, arc);
      s.arcs.append(arc);
    }
  s.seeded = 1;
  s.negative = true;
  return s;
}

// Forward arcs only, weights in [-1, 1]: negative arcs but no cycle at all.
Spec dag_spec(const size_t n, const unsigned seed)
{
  std::mt19937 rng(seed + 2);
  std::uniform_real_distribution<double> weight(-1.0, 1.0);
  Spec s;
  s.nodes = n;
  for (size_t i = 0; i + 1 < n; ++i)
    {
      std::uniform_int_distribution<size_t> pick(i + 1, n - 1);
      for (size_t k = 0; k < 4; ++k)
        s.arcs.append(Arc_Spec{i, pick(rng), weight(rng), false});
    }
  return s;
}

// Disjoint strongly connected components of `component_size` nodes with
// positive weights; one in three gets a negative triangle on its first three
// ring nodes (closing arc c -> a makes the triangle cost -0.01).
Spec components_spec(const size_t n, const unsigned seed)
{
  std::mt19937 rng(seed + 3);
  std::uniform_real_distribution<double> weight(0.5, 1.5);
  const size_t comps = std::max<size_t>(1, n / component_size);

  Spec s;
  s.nodes = comps * component_size;
  for (size_t c = 0; c < comps; ++c)
    {
      const size_t base = c * component_size;
      Array<double> ring;
      for (size_t i = 0; i < component_size; ++i)
        {
          ring.append(weight(rng));
          s.arcs.append(Arc_Spec{base + i, base + (i + 1) % component_size, ring(i), false});
        }
      std::uniform_int_distribution<size_t> pick(0, component_size - 1);
      for (size_t k = 0; k < component_size; ++k)
        {
          const size_t u = pick(rng);
          const size_t v = pick(rng);
          if (u != v)
            s.arcs.append(Arc_Spec{base + u, base + v, weight(rng), false});
        }
      if (c % 3 == 0)
        {
          s.arcs.append(Arc_Spec{base + 2, base, -(ring(0) + ring(1)) - 0.01, false});
          ++s.seeded;
        }
    }
  s.negative = s.seeded > 0;
  return s;
}

// One negative self-loop and nothing else but isolated vertices: the search
// has to cope with a graph that is almost all dead weight.
Spec loop_isolated_spec(const size_t n)
{
  Spec s;
  s.nodes = std::min(loop_isolated_factor * n, loop_isolated_max);
  s.arcs.append(Arc_Spec{0, 0, -1.0, false});
  s.seeded = 1;
  s.negative = true;
  return s;
}

// A graph plus the data the harness needs to run and judge variants on it.
struct Instance
{
  std::string name;
  Spec spec;
  bool bounded_is_cheap = false;   // run the bounded variants whatever the size
  Graph g;
  Array<Arc *> arcs;                    // arcs[i] is the graph arc of spec.arcs(i)
  Array<Array<double>> round_weights;   // empty for static instances
  size_t rounds = 1;

  /// Applies the weight set of round `r` (no-op for static instances).
  void apply_round(const size_t r)
  {
    if (round_weights.is_empty())
      return;
    const Array<double> & w = round_weights(r % rounds);
    for (size_t i = 0; i < arcs.size(); ++i)
      arcs(i)->get_info() = w(i);
  }
};

std::unique_ptr<Instance> make_instance(std::string name, Spec spec,
                                        const bool reversed = false)
{
  auto inst = std::make_unique<Instance>();
  inst->name = std::move(name);
  inst->spec = std::move(spec);
  const Spec & s = inst->spec;

  Array<Node *> nodes;
  const size_t total = s.nodes + s.isolated;
  nodes.reserve(total);
  for (size_t i = 0; i < total; ++i)
    nodes.append(inst->g.insert_node(static_cast<int>(i)));

  const size_t m = s.arcs.size();
  inst->arcs.reserve(m);
  for (size_t i = 0; i < m; ++i)
    inst->arcs.append(nullptr);
  for (size_t k = 0; k < m; ++k)
    {
      const size_t i = reversed ? m - 1 - k : k;
      const Arc_Spec & a = s.arcs(i);
      inst->arcs(i) = inst->g.insert_arc(nodes(a.u), nodes(a.v), a.w);
    }
  return inst;
}

// Market with a fixed topology and `reweight_rounds` price sets.
std::unique_ptr<Instance> make_reweighted(const size_t n, const unsigned seed)
{
  auto inst = make_instance("market-reweighted", market_spec(n, true, seed));
  Spec work = inst->spec;
  std::mt19937 rng(seed + 4);
  for (size_t r = 0; r < reweight_rounds; ++r)
    {
      draw_prices(work, rng);
      Array<double> w;
      w.reserve(work.arcs.size());
      for (size_t i = 0; i < work.arcs.size(); ++i)
        w.append(market_weight(work, work.arcs(i)));
      inst->round_weights.append(std::move(w));
    }
  inst->rounds = reweight_rounds;
  inst->apply_round(0);
  return inst;
}

// ---------------------------------------------------------------------------
// Results and validation
// ---------------------------------------------------------------------------

enum class Verdict { None, Detected, Valid, Invalid };
enum class Group { Detect, Witness, Bounded, Mean, Reference };

struct Outcome
{
  Verdict verdict = Verdict::None;
  size_t count = 0;            // cycles reported
  double cost = 0.0;           // cost of the first (or only) cycle
  size_t length = 0;           // arcs of that cycle
  const char * cert = "-";     // bounded search: "exact" / "no-cert"
  char why[128] = "";          // reason when Invalid
  size_t allocs = 0;           // operator new calls of one run
  size_t bytes = 0;
};

const char * verdict_name(const Verdict v)
{
  switch (v)
    {
    case Verdict::None: return "none";
    case Verdict::Detected: return "detected";
    case Verdict::Valid: return "valid";
    case Verdict::Invalid: return "INVALID";
    }
  return "?";
}

struct Limits
{
  size_t max_len = SIZE_MAX;    // arcs per cycle
  size_t max_count = SIZE_MAX;  // cycles requested
  bool disjoint = false;        // pairwise arc-disjoint (All_Arcs)
};

void invalid(Outcome & o, const char * why)
{
  o.verdict = Verdict::Invalid;
  std::snprintf(o.why, sizeof(o.why), "%s", why);
}

template <class T>
Array<T> to_array(const DynList<T> & list)
{
  Array<T> ret;
  list.for_each([&](const T & x) { ret.append(x); });
  return ret;
}

// Checks that `nodes`/`arcs` describe a closed cycle of `g` (simple, unless
// `simple` is false: a closed walk may repeat nodes). On success returns
// nullptr and sets `cost` to the arc sum.
const char * check_cycle(const Graph & g, const Array<Node *> & nodes,
                         const Array<Arc *> & arcs, long double & cost,
                         const bool simple = true)
{
  if (arcs.size() == 0)
    return "empty cycle";
  if (nodes.size() != arcs.size() + 1)
    return "node and arc counts do not match";
  if (nodes(0) != nodes(nodes.size() - 1))
    return "path is not closed";
  DynSetTree<Node *> seen;
  cost = 0;
  for (size_t i = 0; i < arcs.size(); ++i)
    {
      if (g.get_src_node(arcs(i)) != nodes(i) or g.get_tgt_node(arcs(i)) != nodes(i + 1))
        return "arc does not join consecutive nodes";
      if (simple and seen.insert(nodes(i)) == nullptr)
        return "node repeated: not a simple cycle";
      cost += arcs(i)->get_info();
    }
  return nullptr;
}

Outcome check_result(const Graph &, const bool detected, const Limits &)
{
  Outcome o;
  o.verdict = detected ? Verdict::Detected : Verdict::None;
  return o;
}

// The snapshot is a reference measurement, not a detector: nothing to check.
Outcome check_result(const Graph &, const Indexed &, const Limits &)
{
  return Outcome{};
}

Outcome check_result(const Graph & g, const Path<Graph> & p, const Limits &)
{
  Outcome o;
  if (p.is_empty())
    return o;
  o.count = 1;
  const auto nodes = to_array(p.nodes());
  const auto arcs = to_array(p.arcs());
  long double cost = 0;
  if (const char * why = check_cycle(g, nodes, arcs, cost))
    {
      invalid(o, why);
      return o;
    }
  o.cost = static_cast<double>(cost);
  o.length = arcs.size();
  if (not (cost < 0))
    {
      invalid(o, "witness cost is not negative");
      return o;
    }
  o.verdict = Verdict::Valid;
  return o;
}

Outcome check_result(const Graph & g, const std::tuple<Path<Graph>, size_t> & r,
                     const Limits & lim)
{
  return check_result(g, std::get<0>(r), lim);
}

Outcome check_result(const Graph & g, const Bounded & r, const Limits & lim)
{
  Outcome o;
  if (not r.has_cycle)
    return o;
  o.count = 1;
  o.cost = r.total_cost;
  o.length = r.length;
  o.cert = r.is_exact ? "exact" : "no-cert";
  const auto nodes = to_array(r.cycle_nodes);
  const auto arcs = to_array(r.cycle_arcs);
  long double cost = 0;
  if (const char * why = check_cycle(g, nodes, arcs, cost))
    invalid(o, why);
  else if (arcs.size() != r.length)
    invalid(o, "length field differs from the arc count");
  else if (arcs.size() > lim.max_len)
    invalid(o, "cycle longer than max_length");
  else if (std::fabs(cost - static_cast<long double>(r.total_cost)) > 1e-9L)
    invalid(o, "total_cost differs from the arc sum");
  else if ((cost < 0) != r.is_negative())
    invalid(o, "is_negative() disagrees with the arc sum");
  else
    o.verdict = cost < 0 ? Verdict::Valid : Verdict::None;
  return o;
}

// Karp's minimum mean cycle: the witness is a closed *walk* (it may go around
// a cycle many times, as documented in Min_Mean_Cycle.H), made of consecutive
// arcs of the graph, whose total, length and mean agree with the result.
Outcome check_result(const Graph & g, const Min_Mean_Cycle_Result<Graph, double> & r,
                     const Limits &)
{
  Outcome o;
  if (not r.has_cycle)
    return o;
  o.count = 1;
  o.cost = r.cycle_total_cost;
  o.length = r.cycle_length;
  const auto nodes = to_array(r.cycle_nodes);
  const auto arcs = to_array(r.cycle_arcs);
  long double cost = 0;
  if (const char * why = check_cycle(g, nodes, arcs, cost, false))
    invalid(o, why);
  else if (arcs.size() != r.cycle_length)
    invalid(o, "cycle_length differs from the arc count");
  else if (std::fabs(cost - static_cast<long double>(r.cycle_total_cost)) > 1e-9L)
    invalid(o, "cycle_total_cost differs from the arc sum");
  else if (std::fabs(cost / static_cast<long double>(arcs.size()) - r.minimum_mean) > 1e-9L)
    invalid(o, "the witness mean differs from minimum_mean");
  else
    o.verdict = cost < 0 ? Verdict::Valid : Verdict::None;
  return o;
}

Outcome check_result(const Graph &, const Min_Mean_Cycle_Value_Result & r, const Limits &)
{
  Outcome o;
  o.verdict = r.has_cycle and r.minimum_mean < 0 ? Verdict::Detected : Verdict::None;
  return o;
}

Outcome check_result(const Graph & g, const Cycles & items, const Limits & lim)
{
  Outcome o;
  o.count = items.size();
  if (items.is_empty())
    return o;
  if (items.size() > lim.max_count)
    {
      invalid(o, "more cycles than requested");
      return o;
    }

  Array<Array<Arc *>> all;
  for (auto it = items.get_it(); it.has_curr(); it.next_ne())
    {
      const auto & item = it.get_curr();
      const auto nodes = to_array(item.cycle.nodes());
      const auto arcs = to_array(item.cycle.arcs());
      long double cost = 0;
      if (const char * why = check_cycle(g, nodes, arcs, cost))
        {
          invalid(o, why);
          return o;
        }
      if (all.is_empty())
        {
          o.cost = item.total_cost;
          o.length = item.length;
        }
      if (arcs.size() != item.length)
        invalid(o, "length field differs from the arc count");
      else if (not (cost < 0))
        invalid(o, "cycle cost is not negative");
      else if (std::fabs(cost - static_cast<long double>(item.total_cost)) > 1e-9L)
        invalid(o, "total_cost differs from the arc sum");
      if (o.verdict == Verdict::Invalid)
        return o;
      all.append(arcs);
    }

  if (lim.disjoint)
    {
      DynSetTree<Arc *> used;
      for (size_t i = 0; i < all.size(); ++i)
        for (size_t k = 0; k < all(i).size(); ++k)
          if (used.insert(all(i)(k)) == nullptr)
            {
              invalid(o, "two cycles share an arc under All_Arcs");
              return o;
            }
    }
  else
    for (size_t i = 0; i < all.size(); ++i)
      for (size_t j = i + 1; j < all.size(); ++j)
        {
          if (all(i).size() != all(j).size())
            continue;
          bool same = true;
          for (size_t k = 0; same and k < all(i).size(); ++k)
            {
              bool found = false;
              for (size_t l = 0; not found and l < all(j).size(); ++l)
                found = all(i)(k) == all(j)(l);
              same = found;
            }
          if (same)
            {
              invalid(o, "the same cycle was reported twice");
              return o;
            }
        }

  o.verdict = Verdict::Valid;
  return o;
}

long weigh(const bool b) { return b; }
long weigh(const Path<Graph> & p) { return p.is_empty() ? 0 : static_cast<long>(p.size()); }
long weigh(const std::tuple<Path<Graph>, size_t> & r)
{
  return weigh(std::get<0>(r)) + static_cast<long>(std::get<1>(r));
}
long weigh(const Bounded & r) { return static_cast<long>(r.length); }
long weigh(const Min_Mean_Cycle_Result<Graph, double> & r)
{
  return static_cast<long>(r.cycle_length);
}
long weigh(const Min_Mean_Cycle_Value_Result & r) { return r.has_cycle ? 1 : 0; }
long weigh(const Cycles & c) { return static_cast<long>(c.size()); }
long weigh(const Indexed & ig) { return static_cast<long>(ig.arcs.size()); }

// ---------------------------------------------------------------------------
// Variants
// ---------------------------------------------------------------------------

struct Variant
{
  std::string name;
  Group group;
  bool bf_witness;                              // Bellman_Ford path extraction
  size_t bound;                                 // L of the bounded search, else 0
  bool small_only = false;                      // cost grows too fast to run on huge graphs
  std::function<long(Graph &)> run;             // the bare call, for timing
  std::function<Outcome(Graph &)> inspect;      // the call plus its validation
};

template <class Call>
Variant make_variant(std::string name, const Group group, const bool bf_witness,
                     const size_t bound, const Limits lim, Call call)
{
  Variant v;
  v.name = std::move(name);
  v.group = group;
  v.bf_witness = bf_witness;
  v.bound = bound;
  v.small_only = group == Group::Bounded or group == Group::Mean;
  v.run = [call](Graph & g) { return weigh(call(g)); };
  v.inspect = [call, lim](Graph & g)
  {
    alloc_probe.calls = 0;
    alloc_probe.bytes = 0;
    alloc_probe.active = true;
    const auto result = call(g);
    alloc_probe.active = false;
    const size_t calls = alloc_probe.calls;
    const size_t bytes = alloc_probe.bytes;
    Outcome o = check_result(g, result, lim);
    o.allocs = calls;
    o.bytes = bytes;
    return o;
  };
  return v;
}

DynList<Variant> make_variants()
{
  constexpr size_t K = seeded_triangles;
  DynList<Variant> vs;

  vs.append(make_variant("BF::has_negative_cycle() [classic]", Group::Detect,
    false, 0, Limits{},
    [](Graph & g) { return Bellman_Ford<Graph>(g).has_negative_cycle(); }));

  vs.append(make_variant("BF::test_negative_cycle() [classic+Tarjan]", Group::Witness,
    true, 0, Limits{},
    [](Graph & g) { return Bellman_Ford<Graph>(g).test_negative_cycle(); }));
  vs.append(make_variant("BF::search_negative_cycle() [SPFA]", Group::Witness,
    true, 0, Limits{},
    [](Graph & g) { return Bellman_Ford<Graph>(g).search_negative_cycle(); }));
  vs.append(make_variant("BF::search_negative_cycle(0.1, 1) [SPFA+early]", Group::Witness,
    true, 0, Limits{},
    [](Graph & g) { return Bellman_Ford<Graph>(g).search_negative_cycle(0.1, 1); }));

  vs.append(make_variant("find_disjoint(g, 1)", Group::Witness,
    false, 0, Limits{SIZE_MAX, 1, false},
    [](Graph & g) { return find_disjoint_negative_cycles(g, 1); }));
  vs.append(make_variant("find_disjoint(g, 5, Min_Weight_Arc)", Group::Witness,
    false, 0, Limits{SIZE_MAX, K, false},
    [](Graph & g)
    { return find_disjoint_negative_cycles(g, K, Exclusion::Min_Weight_Arc); }));
  vs.append(make_variant("find_disjoint(g, 5, All_Arcs)", Group::Witness,
    false, 0, Limits{SIZE_MAX, K, true},
    [](Graph & g)
    { return find_disjoint_negative_cycles(g, K, Exclusion::All_Arcs); }));

  for (const size_t L : {3, 6, 12})
    {
      char label[48];
      std::snprintf(label, sizeof(label), "most_negative_cycle_bounded(g, %zu)", L);
      vs.append(make_variant(label, Group::Bounded, false, L, Limits{L, SIZE_MAX, false},
        [L](Graph & g) { return most_negative_cycle_bounded(g, L); }));
    }

  vs.append(make_variant("karp_minimum_mean_cycle_value(g)", Group::Mean,
    false, 0, Limits{},
    [](Graph & g) { return karp_minimum_mean_cycle_value(g); }));
  vs.append(make_variant("karp_minimum_mean_cycle(g)", Group::Mean,
    false, 0, Limits{},
    [](Graph & g) { return karp_minimum_mean_cycle(g); }));

  vs.append(make_variant("snapshot only (index_graph)", Group::Reference,
    false, 0, Limits{},
    [](Graph & g)
    {
      Dft_Dist<Graph> distance;
      Dft_Show_Arc<Graph> show;
      return negative_cycles_detail::index_graph(g, distance, show, "bench");
    }));

  return vs;
}

enum class Judgment { Ok, Known_Defect, Unexpected };

// Compares an outcome with the ground truth of the instance.
Judgment judge(const Instance & inst, const Variant & v, const Outcome & o,
               const char *& reason)
{
  if (v.group == Group::Reference)
    return Judgment::Ok;

  if (not inst.spec.negative)
    {
      if (o.verdict == Verdict::None)
        return Judgment::Ok;
      reason = o.verdict == Verdict::Invalid ? o.why
                                              : "reported a negative cycle where none exists";
      return Judgment::Unexpected;
    }

  if (o.verdict == Verdict::Invalid)
    {
      reason = o.why;
      return v.bf_witness ? Judgment::Known_Defect : Judgment::Unexpected;
    }

  const bool detector = v.group == Group::Detect or v.name.find("_value(") != std::string::npos;
  const Verdict need = detector ? Verdict::Detected : Verdict::Valid;
  if (o.verdict == need)
    return Judgment::Ok;
  reason = "missed the negative cycle planted in the graph";
  return Judgment::Unexpected;
}

// ---------------------------------------------------------------------------
// Timing
// ---------------------------------------------------------------------------

struct Timing
{
  double median = 0.0;
  double p90 = 0.0;
  double best = 0.0;
};

Timing time_variant(Instance & inst, const Variant & v, const size_t samples)
{
  // Untimed warmup: at least `warmup_min_runs`, then until `warmup_ms` have
  // elapsed (at most `warmup_max_runs`), so that short variants also reach a
  // steady state (caches, branch predictors, clock frequency).
  double warmed = 0.0;
  for (size_t w = 0; w < warmup_min_runs or (warmed < warmup_ms and w < warmup_max_runs); ++w)
    {
      inst.apply_round(w);
      const auto t0 = std::chrono::steady_clock::now();
      keep(v.run(inst.g));
      warmed += std::chrono::duration<double, std::milli>(
                  std::chrono::steady_clock::now() - t0).count();
    }

  std::vector<double> ms;
  ms.reserve(samples);
  for (size_t i = 0; i < samples; ++i)
    {
      inst.apply_round(i);
      const auto t0 = std::chrono::steady_clock::now();
      const long r = v.run(inst.g);
      const auto t1 = std::chrono::steady_clock::now();
      keep(r);
      ms.push_back(std::chrono::duration<double, std::milli>(t1 - t0).count());
    }
  std::sort(ms.begin(), ms.end());

  Timing t;
  t.median = ms[ms.size() / 2];
  t.best = ms.front();
  const size_t rank = static_cast<size_t>(std::ceil(0.9 * static_cast<double>(ms.size())));
  t.p90 = ms[std::max<size_t>(rank, 1) - 1];
  return t;
}

// ---------------------------------------------------------------------------
// Reporting
// ---------------------------------------------------------------------------

struct Tally
{
  size_t unexpected = 0;
  size_t known_defects = 0;
};

const char * group_title(const Group g)
{
  switch (g)
    {
    case Group::Detect: return "detection only";
    case Group::Witness: return "with a witness cycle";
    case Group::Bounded: return "bounded search";
    case Group::Mean: return "minimum mean cycle (Karp: O(V * E) time, O(V^2) memory)";
    case Group::Reference: return "reference";
    }
  return "";
}

void print_header()
{
  std::printf("  %-47s %9s %9s %9s  %-8s %5s %12s %4s %-7s %7s %9s\n",
              "variant", "median ms", "p90 ms", "best ms", "verdict", "found",
              "cost", "len", "cert", "allocs", "KiB");
}

void print_row(const Variant & v, const Outcome & o, const Timing * t, const Judgment j)
{
  char times[96];
  if (t != nullptr)
    std::snprintf(times, sizeof(times), "%9.3f %9.3f %9.3f", t->median, t->p90, t->best);
  else
    std::snprintf(times, sizeof(times), "%9s %9s %9s", "-", "-", "-");

  char cost[24] = "-";
  if (o.count > 0)
    std::snprintf(cost, sizeof(cost), "%+.6g", o.cost);
  char len[16] = "-";
  if (o.count > 0)
    std::snprintf(len, sizeof(len), "%zu", o.length);
  // Detectors report no cycle and the snapshot is not a detector at all.
  const bool counts_cycles = v.group == Group::Witness or v.group == Group::Bounded
                             or (v.group == Group::Mean and v.name.find("_value(") == std::string::npos);
  char found[16] = "-";
  if (counts_cycles)
    std::snprintf(found, sizeof(found), "%zu", o.count);

  std::printf("  %-47s %s  %-8s %5s %12s %4s %-7s %7zu %9.1f%s\n", v.name.c_str(), times,
              v.group == Group::Reference ? "-" : verdict_name(o.verdict), found, cost, len,
              o.cert, o.allocs,
              static_cast<double>(o.bytes) / 1024.0,
              j == Judgment::Known_Defect ? "  <- known BF defect"
              : j == Judgment::Unexpected ? "  <- UNEXPECTED" : "");
}

// Validates every variant on `inst` (all its weight sets) and, if `samples`
// is positive, times it. Updates `tally`.
void report_instance(Instance & inst, const DynList<Variant> & variants,
                     const size_t samples, Tally & tally)
{
  std::printf("\n== %s: %zu vertices, %zu arcs, %s ==\n", inst.name.c_str(),
              inst.g.get_num_nodes(), inst.g.get_num_arcs(),
              inst.spec.negative ? "negative cycle planted" : "no negative cycle");
  print_header();

  Group last = Group::Reference;
  bool first = true;
  bool skipped_bounded = false;
  for (auto it = variants.get_it(); it.has_curr(); it.next_ne())
    {
      const Variant & v = it.get_curr();
      if (v.small_only and inst.g.get_num_nodes() > bounded_max_nodes
          and not (v.group == Group::Bounded and inst.bounded_is_cheap))
        {
          skipped_bounded = true;
          continue;
        }
      if (first or v.group != last)
        std::printf("  -- %s\n", group_title(v.group));
      first = false;
      last = v.group;

      Outcome shown;
      Judgment worst = Judgment::Ok;
      std::string reason;   // a copy: `why` may point into a dead Outcome
      for (size_t r = 0; r < inst.rounds; ++r)
        {
          inst.apply_round(r);
          const Outcome o = v.inspect(inst.g);
          const char * why = "";
          const Judgment j = judge(inst, v, o, why);
          if (r == 0 or (j > worst))
            {
              shown = o;
              if (j > worst)
                {
                  worst = j;
                  reason = why;
                }
            }
        }

      if (worst == Judgment::Unexpected)
        ++tally.unexpected;
      else if (worst == Judgment::Known_Defect)
        ++tally.known_defects;

      if (samples > 0)
        {
          const Timing t = time_variant(inst, v, samples);
          print_row(v, shown, &t, worst);
        }
      else
        print_row(v, shown, nullptr, worst);
      if (worst != Judgment::Ok)
        std::printf("      %s: %s\n",
                    worst == Judgment::Unexpected ? "UNEXPECTED" : "known defect",
                    reason.c_str());
    }
  if (skipped_bounded)
    std::printf("  (bounded and Karp variants skipped: more than %zu vertices)\n", bounded_max_nodes);
}

void print_environment(const Options & opt)
{
  std::printf("Negative cycle benchmark\n");
  std::printf("  compiler : %s\n", Aleph::CA::Bench::compiler_string().c_str());
  std::printf("  cpu      : %s\n", Aleph::CA::Bench::cpu_string().c_str());
#ifdef BENCH_BUILD_TYPE
  std::printf("  build    : %s\n", BENCH_BUILD_TYPE);
#endif
#ifdef BENCH_CXX_FLAGS
  std::printf("  flags    : %s\n", BENCH_CXX_FLAGS);
#endif
#if defined(__OPTIMIZE__)
  std::printf("  optimized: yes\n");
#else
  std::printf("  optimized: NO (timings are not meaningful)\n");
#endif
#if defined(NDEBUG)
  std::printf("  asserts  : off\n");
#else
  std::printf("  asserts  : ON (timings are not meaningful)\n");
#endif
  std::printf("  assets=%zu seed=%u samples=%zu%s\n", opt.assets, opt.seed, opt.samples,
              opt.check_only ? " (--check-only: nothing is timed)" : "");
  if (not opt.check_only)
    {
      std::printf("  protocol : untimed warmup (>= %zu runs, until %.0f ms, <= %zu runs), then "
                  "%zu timed runs; median, p90 (nearest rank), best\n",
                  warmup_min_runs, warmup_ms, warmup_max_runs, opt.samples);
      if (opt.samples < min_regression_samples)
        std::printf("  note     : fewer than %zu samples: exploratory only\n",
                    min_regression_samples);
    }
}


// The graph families of the requested instances, in report order.
struct Family
{
  const char * name;
  std::unique_ptr<Instance> (*build)(const Options &);
};

const Family families[] =
  {
    {"market-arbitrage",
     [](const Options & o) { return make_instance("market-arbitrage", market_spec(o.assets, true, o.seed)); }},
    {"market-clean",
     [](const Options & o) { return make_instance("market-clean", market_spec(o.assets, false, o.seed)); }},
    {"market-arbitrage-reversed",
     [](const Options & o)
     { return make_instance("market-arbitrage-reversed", market_spec(o.assets, true, o.seed), true); }},
    {"market-isolated-nodes",
     [](const Options & o)
     {
       Spec s = market_spec(o.assets, true, o.seed);
       s.isolated = o.assets;
       return make_instance("market-isolated-nodes", std::move(s));
     }},
    {"dag", [](const Options & o) { return make_instance("dag", dag_spec(o.assets, o.seed)); }},
    {"components",
     [](const Options & o) { return make_instance("components", components_spec(o.assets, o.seed)); }},
    {"dense",
     [](const Options & o)
     { return make_instance("dense", dense_spec(std::min(o.assets, dense_max_nodes), o.seed)); }},
    {"market-reweighted", [](const Options & o) { return make_reweighted(o.assets, o.seed); }},
    {"loop-isolated",
     [](const Options & o)
     {
       auto inst = make_instance("loop-isolated", loop_isolated_spec(o.assets));
       inst->bounded_is_cheap = true;
       return inst;
     }},
  };

} // namespace

int main(const int argc, char * argv[])
{
  Options opt;
  if (not parse_args(argc, argv, opt))
    {
      usage(stderr);
      return 2;
    }

  if (not opt.only.empty())
    {
      bool any = false;
      for (const Family & f : families)
        any = any or std::strstr(f.name, opt.only.c_str()) != nullptr;
      if (not any)
        {
          std::fprintf(stderr, "no graph family matches --only=%s\n", opt.only.c_str());
          return 2;
        }
    }

  print_environment(opt);
  const DynList<Variant> variants = make_variants();
  Tally tally;

  // Multigraph regressions of the old Bellman-Ford extractor: validation only.
  std::printf("\n#### regression inputs (n = 19; validation only)");
  for (const unsigned seed : {19u, 42u})
    {
      auto inst = make_instance("market-arbitrage n=19 seed=" + std::to_string(seed),
                                market_spec(19, true, seed));
      report_instance(*inst, variants, 0, tally);
    }

  std::printf("\n#### requested instances");
  for (const Family & f : families)
    {
      if (not opt.only.empty() and std::strstr(f.name, opt.only.c_str()) == nullptr)
        continue;
      auto inst = f.build(opt);
      report_instance(*inst, variants, opt.check_only ? 0 : opt.samples, tally);
    }

  std::printf("\nSummary: %zu unexpected result(s), %zu known Bellman-Ford witness defect(s)\n",
              tally.unexpected, tally.known_defects);
  if (tally.known_defects > 0)
    std::printf("  Known defects are INVALID witnesses of Bellman_Ford path extraction on multigraphs\n"
                "  (parallel arcs); fixing the old extractor is a separate task.\n");
  if (tally.unexpected > 0)
    {
      std::fprintf(stderr, "bench_negative_cycles: %zu unexpected result(s)\n", tally.unexpected);
      return 1;
    }
  return sink == -1 ? 1 : 0;
}
