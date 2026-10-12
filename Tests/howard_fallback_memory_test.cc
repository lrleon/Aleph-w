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
 * @file howard_fallback_memory_test.cc
 * @brief The memory that the fallback of howard_minimum_mean_cycle() to
 *        Karp's algorithm reserves, measured on the allocations themselves
 *        (audit 2026-10-02, stage C).
 *
 * This program replaces the global allocation functions: while the probe is
 * active they record the two largest requests. The replacement stays out of
 * the other test programs. A sanitizer that brings its own allocator defines
 * those functions in its runtime (ThreadSanitizer and MemorySanitizer link
 * them into the program, so a second definition does not link): under one,
 * the replacement is left out and the tests are skipped.
 */

// Self-containment check: must stay the first include.
# include <Howard_Min_Mean_Cycle.H>

# include <gtest/gtest.h>

# include <cstdio>
# include <cstdlib>
# include <new>
# include <random>
# include <tuple>
# include <vector>

# include <tpl_graph.H>

# include "negative_cycles_test_support.H"

using namespace Aleph;
using namespace Negative_Cycles_Test_Support;

# if defined(__SANITIZE_ADDRESS__) or defined(__SANITIZE_THREAD__) \
     or defined(__SANITIZE_HWADDRESS__)
#   define PROBE_ALLOCATIONS 0
# elif defined(__has_feature)
#   if __has_feature(address_sanitizer) or __has_feature(thread_sanitizer) \
       or __has_feature(memory_sanitizer) or __has_feature(hwaddress_sanitizer)
#     define PROBE_ALLOCATIONS 0
#   endif
# endif
# ifndef PROBE_ALLOCATIONS
#   define PROBE_ALLOCATIONS 1
# endif

namespace
{
  // The two largest allocation requests while `active`. Plain variables: the
  // tests record in a single thread, and the functions below cannot allocate.
  struct Allocation_Probe
  {
    bool active = false;
    size_t largest = 0;
    size_t second = 0;

    void start() noexcept
    {
      largest = 0;
      second = 0;
      active = true;
    }

    void stop() noexcept
    {
      active = false;
    }
  };

  Allocation_Probe probe;
} // namespace

# if PROBE_ALLOCATIONS

// Not inlined: GCC cannot then pair the malloc behind `operator new` with the
// free behind `operator delete` and warn about a mismatch.
# if defined(__GNUC__)
#   define PROBE_NOINLINE __attribute__((noinline))
# else
#   define PROBE_NOINLINE
# endif

namespace
{
  void * probed_malloc(const std::size_t size) noexcept
  {
    if (probe.active)
      {
        if (size > probe.largest)
          {
            probe.second = probe.largest;
            probe.largest = size;
          }
        else if (size > probe.second)
          probe.second = size;
      }
    return std::malloc(size == 0 ? 1 : size);
  }
} // namespace

// Every form without an alignment: the nothrow ones too, which gtest uses
// and a sanitizer would otherwise serve with its own allocator, mismatched
// with the free() below.
PROBE_NOINLINE void * operator new(const std::size_t size)
{
  if (void * p = probed_malloc(size))
    return p;
  std::fputs("howard_fallback_memory_test: out of memory\n", stderr);
  std::abort();
}

PROBE_NOINLINE void * operator new[](const std::size_t size) { return operator new(size); }
PROBE_NOINLINE void * operator new(const std::size_t size, const std::nothrow_t &) noexcept
{
  return probed_malloc(size);
}
PROBE_NOINLINE void * operator new[](const std::size_t size, const std::nothrow_t &) noexcept
{
  return probed_malloc(size);
}
PROBE_NOINLINE void operator delete(void * p) noexcept { std::free(p); }
PROBE_NOINLINE void operator delete[](void * p) noexcept { std::free(p); }
PROBE_NOINLINE void operator delete(void * p, std::size_t) noexcept { std::free(p); }
PROBE_NOINLINE void operator delete[](void * p, std::size_t) noexcept { std::free(p); }
PROBE_NOINLINE void operator delete(void * p, const std::nothrow_t &) noexcept { std::free(p); }
PROBE_NOINLINE void operator delete[](void * p, const std::nothrow_t &) noexcept { std::free(p); }

# endif // PROBE_ALLOCATIONS

namespace
{
  // What Array reserves for `count` elements: the smallest power of two not
  // below it.
  size_t reserved(const size_t count)
  {
    size_t p = 1;
    while (p < count)
      p <<= 1;
    return p;
  }

  // A single strongly connected component: a ring through the n nodes, plus
  // one random chord out of each node.
  template <class GT, typename W, class Weight>
  Built_Graph_T<GT> ring(const size_t n, std::mt19937_64 & rng, Weight weight)
  {
    std::vector<std::tuple<size_t, size_t, W>> arcs;
    for (size_t i = 0; i < n; ++i)
      {
        arcs.emplace_back(i, (i + 1) % n, weight(rng));
        arcs.emplace_back(i, static_cast<size_t>(rng() % n), weight(rng));
      }
    return build_graph_generic<GT, W>(n, arcs);
  }

  // Run the fallback (iteration limit 0) under the probe, with the limit
  // `limit`, with or without the witness.
  template <bool With_Witness, class GT>
  Howard_Mean_Cycle_Result<GT, typename Dft_Dist<GT>::Distance_Type>
  probed_fallback(const GT & g, const size_t limit)
  {
    using D = Dft_Dist<GT>;
    using S = Dft_Show_Arc<GT>;
    probe.start();
    auto r = howard_detail::minimum_mean_cycle<GT, D, S, With_Witness>(g, D(), S(), 0, limit);
    probe.stop();
    return r;
  }

  template <bool With_Witness, class GT>
  bool refused(const GT & g, const size_t limit)
  {
    using D = Dft_Dist<GT>;
    using S = Dft_Show_Arc<GT>;
    try
      {
        (void) howard_detail::minimum_mean_cycle<GT, D, S, With_Witness>(g, D(), S(), 0, limit);
      }
    catch (const std::length_error &)
      {
        return true;
      }
    return false;
  }
} // namespace


TEST(HowardFallbackMemoryTest, TheLimitCountsTheTablesKarpReserves)
{
  if (not PROBE_ALLOCATIONS)
    GTEST_SKIP() << "a sanitizer provides the allocation functions";

  // For one component of n nodes, Karp's tables are the largest allocations
  // of the whole call: (n + 1) * n sums and, with the witness, as many
  // predecessor positions, each table rounded up by Array to a power of two
  // (1101 * 1100 = 1211100 entries take 2^21). The limit has to count exactly
  // that: the call passes with it and is refused one byte below.
  std::mt19937_64 rng(0xC3);
  const auto small = [](std::mt19937_64 & r) { return static_cast<long long>(r() % 2001) - 1000; };
  const auto unit = [](std::mt19937_64 & r) { return std::uniform_real_distribution<double>(-1, 1)(r); };

  for (const size_t n : {120u, 400u, 1100u})
    {
      const size_t entries = reserved((n + 1) * n);

      auto ib = ring<Graph, long long>(n, rng, small);
      const size_t sums = entries * sizeof(long long);
      const size_t preds = entries * sizeof(size_t);
      const auto r = probed_fallback<true>(ib.g, sums + preds);
      ASSERT_TRUE(r.used_karp) << "n=" << n;
      EXPECT_EQ(probe.largest, sums) << "n=" << n;
      EXPECT_EQ(probe.second, preds) << "n=" << n;
      EXPECT_TRUE(refused<true>(ib.g, sums + preds - 1)) << "n=" << n;

      const auto v = probed_fallback<false>(ib.g, sums);
      ASSERT_TRUE(v.used_karp) << "n=" << n;
      EXPECT_EQ(v.minimum_mean, r.minimum_mean) << "n=" << n;
      EXPECT_EQ(probe.largest, sums) << "n=" << n;
      EXPECT_LT(probe.second, sums / 4) << "n=" << n;   // no second table
      EXPECT_TRUE(refused<false>(ib.g, sums - 1)) << "n=" << n;

      auto fb = ring<Float_Graph, double>(n, rng, unit);
      const size_t dsums = entries * sizeof(double);
      const auto f = probed_fallback<true>(fb.g, dsums + preds);
      ASSERT_TRUE(f.used_karp) << "n=" << n;
      EXPECT_EQ(probe.largest, dsums) << "n=" << n;
      EXPECT_TRUE(refused<true>(fb.g, dsums + preds - 1)) << "n=" << n;
    }
}


TEST(HowardFallbackMemoryTest, SumsBeyondLongLongTakeSixteenBytesAndTheLimitKnows)
{
  if (not PROBE_ALLOCATIONS)
    GTEST_SKIP() << "a sanitizer provides the allocation functions";

  // Weights of about 2^61: n * max|w| leaves long long, so the sums of the
  // table take 128 bits where the compiler has them, 8 checked bytes
  // otherwise. Only the value: the cost of a witness could leave long long.
  std::mt19937_64 rng(0x16);
  const auto big = [](std::mt19937_64 & r)
  {
    const long long m = (1LL << 61) + static_cast<long long>(r() % (1ULL << 60));
    return r() % 2 ? m : -m;
  };
  const size_t n = 300;
  auto built = ring<Graph, long long>(n, rng, big);
# if ALEPH_KARP_INT128
  const size_t sums = reserved((n + 1) * n) * 16;
# else
  const size_t sums = reserved((n + 1) * n) * sizeof(long long);
# endif
  Howard_Mean_Cycle_Result<Graph, long long> v;
  try
    {
      v = probed_fallback<false>(built.g, sums);
    }
  catch (const std::overflow_error &)   // only possible with checked long long sums
    {
      probe.stop();
      GTEST_SKIP() << "the checked long long sums overflowed";
    }
  ASSERT_TRUE(v.used_karp);
  EXPECT_EQ(probe.largest, sums);
  EXPECT_TRUE(refused<false>(built.g, sums - 1));
}


TEST(HowardFallbackMemoryTest, NodesOutsideTheCyclesCostTheFallbackNothing)
{
  if (not PROBE_ALLOCATIONS)
    GTEST_SKIP() << "a sanitizer provides the allocation functions";

  // Audit 2026-10-02, H5, and the acceptance of stage C: two components of
  // three nodes among thousands of isolated ones. Howard falls back (the
  // scaled bias of the first component overflows), and no allocation of the
  // whole call grows with the square of the number of nodes. The largest
  // ones are the snapshot's, linear in it; a table over all the nodes would
  // take hundreds of MiB.
  const long long big = 3100000000000000000LL;
  for (const size_t isolated : {1000u, 3000u})
    {
      const size_t nodes = 6 + isolated;
      auto built = build_graph(nodes, {{0, 1, big}, {1, 2, -big}, {2, 0, 1},
                                       {3, 4, -1}, {4, 5, -1}, {5, 3, -2}});
      probe.start();
      const auto r = howard_minimum_mean_cycle(built.g);
      probe.stop();
      ASSERT_TRUE(r.used_karp) << "isolated=" << isolated;
      ASSERT_EQ(r.fallback_reason, Howard_Fallback_Reason::Arithmetic_Overflow);
      EXPECT_EQ(r.minimum_mean, -4.0L / 3.0L);
      EXPECT_LT(probe.largest, 64 * nodes) << "isolated=" << isolated;   // linear, not quadratic
    }
}
