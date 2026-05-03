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
 * @file ca_rng_test.cc
 * @brief Phase 8 tests for `ca-rng.H`.
 *
 * Coverage:
 *  - `splitmix64` avalanche (small inputs spread across all 64 bits).
 *  - `cell_seed` is sensitive to every coordinate component, the step
 *    index and the master seed.
 *  - `Per_Thread_RNG::for_cell` is deterministic and yields identical
 *    streams across invocations.
 *  - `uniform_unit` / `uniform_int` produce values inside the
 *    documented ranges.
 */

#include <array>
#include <cstdint>
#include <random>
#include <set>

#include <gtest/gtest.h>

#include <ca-rng.H>
#include <ca-traits.H>

using namespace Aleph;
using namespace Aleph::CA;

TEST(CARng, SplitMix64IsDistinctForSmallInputs)
{
  // 1024 sequential inputs should never collide; the SplitMix64
  // mixing function is bijective by construction.
  std::set<std::uint64_t> seen;
  for (std::uint64_t x = 0; x < 1024; ++x)
    seen.insert(splitmix64(x));
  EXPECT_EQ(seen.size(), 1024u);
}

TEST(CARng, CellSeedIsSensitiveToCoord)
{
  Coord_Vec<2> a{0, 0};
  Coord_Vec<2> b{0, 1};
  Coord_Vec<2> c{1, 0};
  EXPECT_NE(cell_seed<2>(0xC0FFEE, /*step=*/3, a),
            cell_seed<2>(0xC0FFEE, /*step=*/3, b));
  EXPECT_NE(cell_seed<2>(0xC0FFEE, /*step=*/3, a),
            cell_seed<2>(0xC0FFEE, /*step=*/3, c));
  EXPECT_NE(cell_seed<2>(0xC0FFEE, /*step=*/3, b),
            cell_seed<2>(0xC0FFEE, /*step=*/3, c));
}

TEST(CARng, CellSeedIsSensitiveToStep)
{
  Coord_Vec<2> p{4, 7};
  EXPECT_NE(cell_seed<2>(0x12345, 0, p), cell_seed<2>(0x12345, 1, p));
  EXPECT_NE(cell_seed<2>(0x12345, 1, p), cell_seed<2>(0x12345, 2, p));
}

TEST(CARng, CellSeedIsSensitiveToMasterSeed)
{
  Coord_Vec<3> p{1, 2, 3};
  EXPECT_NE(cell_seed<3>(0xAAAA, /*step=*/5, p),
            cell_seed<3>(0xBBBB, /*step=*/5, p));
}

TEST(CARng, CellSeedIsDeterministic)
{
  Coord_Vec<2> p{17, 33};
  EXPECT_EQ(cell_seed<2>(42, 9, p), cell_seed<2>(42, 9, p));

  Cell_Context<2> ctx{9, p};
  EXPECT_EQ(cell_seed<2>(42, ctx), cell_seed<2>(42, 9, p));
}

TEST(CARng, PerThreadRngForCellIsReproducible)
{
  Per_Thread_RNG<std::mt19937_64> rng(0xDEADBEEFul);
  Coord_Vec<2> p{5, 11};
  auto e1 = rng.for_cell(/*step=*/4, p);
  auto e2 = rng.for_cell(/*step=*/4, p);
  for (int i = 0; i < 8; ++i)
    EXPECT_EQ(e1(), e2());
}

TEST(CARng, PerThreadRngDifferentForDifferentCells)
{
  Per_Thread_RNG<std::mt19937_64> rng(0x1234ul);
  auto e1 = rng.for_cell(/*step=*/0, Coord_Vec<2>{0, 0});
  auto e2 = rng.for_cell(/*step=*/0, Coord_Vec<2>{0, 1});
  // The probability of two independently seeded mt19937_64 producing
  // the same first output is ~2^-64; if it happens here the seeds are
  // colliding which would be a real bug.
  EXPECT_NE(e1(), e2());
}

TEST(CARng, PerThreadRngContextOverloadMatches)
{
  Per_Thread_RNG<std::mt19937_64> rng(0x99u);
  Cell_Context<2> ctx{7, Coord_Vec<2>{2, 3}};

  auto a = rng.for_cell(ctx);
  auto b = rng.for_cell(/*step=*/7, Coord_Vec<2>{2, 3});
  for (int i = 0; i < 4; ++i)
    EXPECT_EQ(a(), b());
}

TEST(CARng, PerThreadRngForThreadStepDistinctSubstreams)
{
  Per_Thread_RNG<std::mt19937_64> rng(0xABABul);
  auto a = rng.for_thread_step(0, 0);
  auto b = rng.for_thread_step(1, 0);
  auto c = rng.for_thread_step(0, 1);
  EXPECT_NE(a(), b());
  EXPECT_NE(a(), c());
  EXPECT_NE(b(), c());
}

TEST(CARng, UniformUnitInsideHalfOpenInterval)
{
  Per_Thread_RNG<std::mt19937_64> rng(0x42u);
  auto eng = rng.for_cell(/*step=*/0, Coord_Vec<1>{0});
  for (int i = 0; i < 1024; ++i)
    {
      const double u = uniform_unit(eng);
      EXPECT_GE(u, 0.0);
      EXPECT_LT(u, 1.0);
    }
}

TEST(CARng, UniformIntInsideRange)
{
  Per_Thread_RNG<std::mt19937_64> rng(0x42u);
  auto eng = rng.for_cell(/*step=*/0, Coord_Vec<1>{0});
  for (int i = 0; i < 256; ++i)
    {
      const std::size_t v = uniform_int(eng, 7, 17);
      EXPECT_GE(v, 7u);
      EXPECT_LE(v, 17u);
    }
}

TEST(CARng, UniformIntDegenerateRangeReturnsLow)
{
  Per_Thread_RNG<std::mt19937_64> rng(0x1u);
  auto eng = rng.for_cell(/*step=*/0, Coord_Vec<1>{0});
  EXPECT_EQ(uniform_int(eng, 5, 5), 5u);
  // hi < lo collapses to lo (saturating semantics).
  EXPECT_EQ(uniform_int(eng, 8, 3), 8u);
}

TEST(CARng, CellKeyFromCoordIsCoordSensitive)
{
  EXPECT_NE(cell_key_from_coord<2>({0, 0}), cell_key_from_coord<2>({0, 1}));
  EXPECT_NE(cell_key_from_coord<2>({0, 0}), cell_key_from_coord<2>({1, 0}));
  EXPECT_NE(cell_key_from_coord<3>({1, 2, 3}),
            cell_key_from_coord<3>({3, 2, 1}));
}
