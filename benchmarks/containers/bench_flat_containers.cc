/*
                          Aleph_w

  Data structures & Algorithms
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_flat_containers.cc
 * @brief Benchmarks for the flat containers (v5.5.0) against their
 *        tree-based Aleph counterparts and the STL.
 *
 * Compares:
 *  - FlatSet vs DynSetTree vs std::set  (bulk build, incremental insert,
 *    lookup, iteration)
 *  - FlatMap vs DynMapTree vs std::map  (same axes)
 *  - SmallVector vs std::vector         (many short-lived small sequences)
 *  - RingBuffer vs std::deque           (bounded sliding window)
 *
 * This is an informative benchmark (not part of the CA perf gate). Sizes
 * can be tuned via argv: bench_flat_containers [lookup_n] [insert_n].
 */

#include <algorithm>
#include <chrono>
#include <cstdio>
#include <cstdlib>
#include <deque>
#include <map>
#include <numeric>
#include <random>
#include <set>
#include <string>
#include <vector>

#include <tpl_dynSetTree.H>
#include <tpl_dynMapTree.H>
#include <tpl_flat_map.H>
#include <tpl_flat_set.H>
#include <tpl_ring_buffer.H>
#include <tpl_small_vector.H>

using namespace Aleph;

namespace
{

volatile long sink = 0;  // keeps results observable for the optimizer

// Runs fn `repeats` times and returns the best wall-clock milliseconds.
template <class Fn>
double best_ms(Fn fn, int repeats = 3)
{
  double best = 1e300;
  for (int r = 0; r < repeats; ++r)
    {
      const auto t0 = std::chrono::steady_clock::now();
      fn();
      const auto t1 = std::chrono::steady_clock::now();
      const double ms =
        std::chrono::duration<double, std::milli>(t1 - t0).count();
      best = std::min(best, ms);
    }
  return best;
}

void row(const char *container, const char *op, double ms)
{
  std::printf("  %-12s %-24s %10.2f ms\n", container, op, ms);
}

std::vector<long> shuffled_keys(size_t n, unsigned seed)
{
  std::vector<long> keys(n);
  std::iota(keys.begin(), keys.end(), 0L);
  std::shuffle(keys.begin(), keys.end(), std::mt19937(seed));
  return keys;
}

void bench_sets(size_t lookup_n, size_t insert_n)
{
  std::printf("\n== Ordered sets (lookup/iteration size %zu, incremental "
              "insert size %zu) ==\n", lookup_n, insert_n);

  const auto keys = shuffled_keys(lookup_n, 1u);
  const auto ins_keys = shuffled_keys(insert_n, 2u);

  // Bulk build (the flat container's natural loading mode).
  row("FlatSet", "bulk build",
      best_ms([&] { FlatSet<long> s(keys.begin(), keys.end());
                    sink += static_cast<long>(s.size()); }));
  row("DynSetTree", "bulk build",
      best_ms([&] { DynSetTree<long> s;
                    for (const long k : keys) s.insert(k);
                    sink += static_cast<long>(s.size()); }));
  row("std::set", "bulk build",
      best_ms([&] { std::set<long> s(keys.begin(), keys.end());
                    sink += static_cast<long>(s.size()); }));

  // Incremental random insertion (the flat container's worst case).
  row("FlatSet", "random insert",
      best_ms([&] { FlatSet<long> s;
                    for (const long k : ins_keys) s.insert(k);
                    sink += static_cast<long>(s.size()); }));
  row("DynSetTree", "random insert",
      best_ms([&] { DynSetTree<long> s;
                    for (const long k : ins_keys) s.insert(k);
                    sink += static_cast<long>(s.size()); }));
  row("std::set", "random insert",
      best_ms([&] { std::set<long> s;
                    for (const long k : ins_keys) s.insert(k);
                    sink += static_cast<long>(s.size()); }));

  // Lookup (half hits, half misses) on prebuilt containers.
  FlatSet<long> fs(keys.begin(), keys.end());
  DynSetTree<long> ts;
  for (const long k : keys)
    ts.insert(k);
  std::set<long> ss(keys.begin(), keys.end());
  const long span = 2 * static_cast<long>(lookup_n);

  row("FlatSet", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += fs.contains(k);
                    sink += hits; }));
  row("DynSetTree", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += ts.contains(k);
                    sink += hits; }));
  row("std::set", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += ss.count(k);
                    sink += hits; }));

  // Full iteration.
  row("FlatSet", "iterate + sum",
      best_ms([&] { long acc = 0;
                    for (const long k : fs) acc += k;
                    sink += acc; }));
  row("DynSetTree", "iterate + sum",
      best_ms([&] { long acc = 0;
                    ts.for_each([&acc] (const long &k) { acc += k; });
                    sink += acc; }));
  row("std::set", "iterate + sum",
      best_ms([&] { long acc = 0;
                    for (const long k : ss) acc += k;
                    sink += acc; }));
}

void bench_maps(size_t lookup_n, size_t insert_n)
{
  std::printf("\n== Ordered maps (lookup size %zu, incremental insert size "
              "%zu) ==\n", lookup_n, insert_n);

  const auto keys = shuffled_keys(lookup_n, 3u);
  const auto ins_keys = shuffled_keys(insert_n, 4u);

  row("FlatMap", "random insert",
      best_ms([&] { FlatMap<long, long> m;
                    for (const long k : ins_keys) m.insert(k, 2 * k);
                    sink += static_cast<long>(m.size()); }));
  row("DynMapTree", "random insert",
      best_ms([&] { DynMapTree<long, long> m;
                    for (const long k : ins_keys) m.insert(k, 2 * k);
                    sink += static_cast<long>(m.size()); }));
  row("std::map", "random insert",
      best_ms([&] { std::map<long, long> m;
                    for (const long k : ins_keys) m.insert({k, 2 * k});
                    sink += static_cast<long>(m.size()); }));

  FlatMap<long, long> fm;
  for (const long k : keys)
    fm.insert(k, 2 * k);
  DynMapTree<long, long> tm;
  for (const long k : keys)
    tm.insert(k, 2 * k);
  std::map<long, long> sm;
  for (const long k : keys)
    sm.insert({k, 2 * k});
  const long span = 2 * static_cast<long>(lookup_n);

  row("FlatMap", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += fm.contains(k);
                    sink += hits; }));
  row("DynMapTree", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += tm.contains(k);
                    sink += hits; }));
  row("std::map", "lookup 50% hits",
      best_ms([&] { long hits = 0;
                    for (long k = 0; k < span; ++k) hits += sm.count(k);
                    sink += hits; }));

  row("FlatMap", "iterate + sum values",
      best_ms([&] { long acc = 0;
                    for (auto [k, v] : fm) acc += v;
                    sink += acc; }));
  row("DynMapTree", "iterate + sum values",
      best_ms([&] { long acc = 0;
                    tm.for_each([&acc] (const std::pair<long, long> &p)
                    { acc += p.second; });
                    sink += acc; }));
  row("std::map", "iterate + sum values",
      best_ms([&] { long acc = 0;
                    for (const auto &[k, v] : sm) acc += v;
                    sink += acc; }));
}

void bench_small_vectors(size_t rounds)
{
  std::printf("\n== Small sequences (%zu rounds of 8 appends each) ==\n",
              rounds);

  row("SmallVector", "build 8-elem seq",
      best_ms([&] { long acc = 0;
                    for (size_t r = 0; r < rounds; ++r)
                      {
                        SmallVector<long, 8> v;  // inline: no allocation
                        for (long i = 0; i < 8; ++i)
                          v.append(i + static_cast<long>(r));
                        acc += v.get_last();
                      }
                    sink += acc; }));
  row("std::vector", "build 8-elem seq",
      best_ms([&] { long acc = 0;
                    for (size_t r = 0; r < rounds; ++r)
                      {
                        std::vector<long> v;  // always allocates
                        for (long i = 0; i < 8; ++i)
                          v.push_back(i + static_cast<long>(r));
                        acc += v.back();
                      }
                    sink += acc; }));
}

void bench_ring_buffers(size_t stream_n)
{
  std::printf("\n== Sliding window over a stream of %zu samples "
              "(window 1024) ==\n", stream_n);
  constexpr size_t window = 1024;

  row("RingBuffer", "put_overwrite stream",
      best_ms([&] { RingBuffer<long> rb(window);
                    for (size_t i = 0; i < stream_n; ++i)
                      rb.put_overwrite(static_cast<long>(i));
                    sink += rb.get_last(); }));
  row("std::deque", "bounded push/pop",
      best_ms([&] { std::deque<long> dq;
                    for (size_t i = 0; i < stream_n; ++i)
                      {
                        dq.push_back(static_cast<long>(i));
                        if (dq.size() > window)
                          dq.pop_front();
                      }
                    sink += dq.back(); }));
}

}  // namespace

int main(int argc, char *argv[])
{
  const size_t lookup_n = argc > 1 ? std::strtoul(argv[1], nullptr, 10)
                                   : 200000;
  const size_t insert_n = argc > 2 ? std::strtoul(argv[2], nullptr, 10)
                                   : 20000;

  std::printf("Flat containers benchmark (best of 3 runs)\n");
  bench_sets(lookup_n, insert_n);
  bench_maps(lookup_n, insert_n);
  bench_small_vectors(200000);
  bench_ring_buffers(5000000);
  std::printf("\n(sink=%ld)\n", static_cast<long>(sink));
  return 0;
}