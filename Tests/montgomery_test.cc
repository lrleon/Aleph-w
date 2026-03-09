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

/** @file montgomery_test.cc
 *  @brief Tests for Montgomery reduction helpers.
 */

# include <gtest/gtest.h>

# include <random>

# include <modular_arithmetic.H>

using namespace Aleph;

namespace
{
  void
  expect_round_trip(const uint64_t mod,
                    const Array<uint64_t> & values)
  {
    const MontgomeryCtx ctx = montgomery_ctx(mod);
    for (size_t i = 0; i < values.size(); ++i)
      EXPECT_EQ(from_mont(to_mont(values[i], ctx), ctx), values[i] % mod)
        << "mod=" << mod << " value=" << values[i];
  }

  void
  expect_products(const uint64_t mod,
                  const Array<uint64_t> & lhs_values,
                  const Array<uint64_t> & rhs_values)
  {
    const MontgomeryCtx ctx = montgomery_ctx(mod);
    for (size_t i = 0; i < lhs_values.size(); ++i)
      for (size_t j = 0; j < rhs_values.size(); ++j)
        {
          const uint64_t lhs = lhs_values[i];
          const uint64_t rhs = rhs_values[j];
          const uint64_t got =
              from_mont(mont_mul(to_mont(lhs, ctx), to_mont(rhs, ctx), ctx),
                        ctx);
          EXPECT_EQ(got, mod_mul(lhs % mod, rhs % mod, mod))
            << "mod=" << mod << " lhs=" << lhs << " rhs=" << rhs;
        }
  }
}

TEST(MontgomeryTest, RejectsInvalidModuli)
{
  EXPECT_THROW(static_cast<void>(montgomery_ctx(0)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(montgomery_ctx(1)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(montgomery_ctx(2)), std::invalid_argument);
  EXPECT_THROW(static_cast<void>(montgomery_ctx(100)), std::invalid_argument);
}

TEST(MontgomeryTest, RoundTripOnFriendlyPrimes)
{
  const Array<uint64_t> mods =
      {998244353ULL, 469762049ULL, 1004535809ULL};

  for (size_t i = 0; i < mods.size(); ++i)
    {
      const uint64_t mod = mods[i];
      expect_round_trip(mod, {0ULL, 1ULL, 2ULL, mod - 2, mod - 1,
                              mod, mod + 1, mod * 2 + 7});
    }
}

TEST(MontgomeryTest, MultiplicationMatchesModMul)
{
  const Array<uint64_t> mods =
      {998244353ULL, 469762049ULL, 1004535809ULL};

  for (size_t i = 0; i < mods.size(); ++i)
    {
      const uint64_t mod = mods[i];
      expect_products(mod, {0ULL, 1ULL, 2ULL, mod - 2, mod - 1, mod + 5},
                       {0ULL, 1ULL, 3ULL, mod - 3, mod - 1, mod + 9});

      std::mt19937_64 rng(mod);
      const MontgomeryCtx ctx = montgomery_ctx(mod);
      for (size_t sample = 0; sample < 128; ++sample)
        {
          const uint64_t lhs = rng();
          const uint64_t rhs = rng();
          const uint64_t got =
              from_mont(mont_mul(to_mont(lhs, ctx), to_mont(rhs, ctx), ctx),
                        ctx);
          EXPECT_EQ(got, mod_mul(lhs % mod, rhs % mod, mod))
            << "mod=" << mod << " lhs=" << lhs << " rhs=" << rhs;
        }
    }
}

TEST(MontgomeryTest, ExponentiationMatchesModExp)
{
  const Array<uint64_t> mods =
      {998244353ULL, 469762049ULL, 1004535809ULL};
  const Array<uint64_t> exponents =
      {0ULL, 1ULL, 2ULL, 7ULL, 31ULL, 123456ULL};

  for (size_t i = 0; i < mods.size(); ++i)
    {
      const uint64_t mod = mods[i];
      const MontgomeryCtx ctx = montgomery_ctx(mod);
      const Array<uint64_t> bases =
          {0ULL, 1ULL, 2ULL, 5ULL, mod - 2, mod - 1, mod + 11};

      for (size_t j = 0; j < bases.size(); ++j)
        for (size_t k = 0; k < exponents.size(); ++k)
          {
            const uint64_t base = bases[j];
            const uint64_t exp = exponents[k];
            const uint64_t got =
                from_mont(mont_exp(to_mont(base, ctx), exp, ctx), ctx);
            EXPECT_EQ(got, mod_exp(base % mod, exp, mod))
              << "mod=" << mod << " base=" << base << " exp=" << exp;
          }
    }
}

TEST(MontgomeryTest, HandlesLargeOddModulusNearUint64Limit)
{
  constexpr uint64_t mod = 18446744073709551557ULL;
  const MontgomeryCtx ctx = montgomery_ctx(mod);

  expect_round_trip(mod, {0ULL, 1ULL, 2ULL, mod - 2, mod - 1});
  expect_products(mod, {mod - 1, mod - 2, mod - 3},
                   {mod - 4, mod - 5, mod - 6});

  EXPECT_EQ(from_mont(mont_exp(to_mont(mod - 1, ctx), 5, ctx), ctx),
            mod_exp(mod - 1, 5, mod));
}
