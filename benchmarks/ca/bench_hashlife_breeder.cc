/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon
*/

/**
 * @file bench_hashlife_breeder.cc
 * @brief Perf-gate anchor: Hashlife advanced 2^20 generations.
 *
 * The roadmap calls for a quadratically growing "breeder" so that Hashlife's
 * node table keeps expanding and cannot fully memoise the run, which is what
 * makes the wall time large and stable enough to gate on. A periodic emitter
 * such as a glider gun would compress almost perfectly and collapse to a
 * sub-millisecond, jitter-dominated measurement.
 *
 * To get the same effect with a pattern that is guaranteed valid and fully
 * deterministic (no fragile hand-typed RLE), this anchor seeds a fixed-seed
 * chaotic soup. Its long transient forces Hashlife to canonicalise a large,
 * ever-changing set of nodes while a single exponential `advance(20)` jumps
 * 2^20 generations, exercising the engine the way a true breeder would.
 */

#include <cstddef>
#include <cstdint>
#include <random>

#include <benchmarks/ca/bench_support.H>
#include <tpl_ca_hashlife.H>

using namespace Aleph::CA;
using namespace Aleph::CA::Bench;

int main()
{
  constexpr std::int64_t soup_side = 256;
  constexpr unsigned advance_log2 = 20;  // 2^20 generations in one exponential jump.
  constexpr std::uint64_t soup_seed = 0xB7EED50FCA000001ull;
  const double generations = static_cast<double>(std::uint64_t{1} << advance_log2);

  const Bench_Result result = measure("hashlife_breeder", generations, resolve_repeats(3), [&]
  {
    Hashlife_Engine engine;
    std::mt19937_64 rng(soup_seed);
    std::bernoulli_distribution alive(0.5);
    for (std::int64_t y = 0; y < soup_side; ++y)
      for (std::int64_t x = 0; x < soup_side; ++x)
        if (alive(rng))
          engine.set_alive(x, y);
    engine.advance(advance_log2);
  });

  emit_json(result);
  return 0;
}