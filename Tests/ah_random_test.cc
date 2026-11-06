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
 * @file ah_random_test.cc
 * @brief Gsl_Compatible_Rng against GSL, bit by bit, but for the reals of
 * flat() and gaussian() where GSL fuses multiply-adds, and for those of
 * gaussian() and exponential() where the logarithm of the tests is not
 * that of GSL, as with MSVC and clang-cl.
 *
 * Each test seeds a Gsl_Compatible_Rng and a gsl_rng_mt19937 alike, and
 * draws from both, in an order that another generator picks, so that a
 * draw that took one number more or less from one of them would show in
 * every later one.
 */

#include <gtest/gtest.h>
#include <ah-random.H>
#include <gsl/gsl_randist.h>
#include <gsl/gsl_rng.h>

#include <algorithm>
#include <climits>
#include <cmath>
#include <cstdint>
#include <iomanip>
#include <memory>
#include <numeric>
#include <random>
#include <vector>

using namespace Aleph;

namespace
{
  struct Gsl_Free
  {
    void operator () (gsl_rng * r) const noexcept { gsl_rng_free(r); }
  };

  // Whether the tests take the logarithm of GSL, that of the same C
  // library: not with MSVC or clang-cl, whose programs take that of the
  // UCRT, while the GSL of MSYS2 takes that of its own C runtime, which
  // may differ in the last bit
#if defined(_MSC_VER)
  constexpr bool same_log_as_gsl = false;
#else
  constexpr bool same_log_as_gsl = true;
#endif

  ::testing::AssertionResult same_or_close(const double ours, const double gsl,
                                           const bool exactly)
  {
    if (exactly ? ours == gsl
                : std::abs(ours - gsl) <= 1e-9 * std::max(1.0, std::abs(gsl)))
      return ::testing::AssertionSuccess();
    return ::testing::AssertionFailure() << std::setprecision(17) << ours << " is not "
                                         << gsl;
  }

  // Whether a draw of flat() or gaussian() is the one of GSL: bit by bit
  // where GSL rounds each product, as the distributions build it for
  // x86-64, and the logarithm of gaussian() is that of GSL; elsewhere,
  // where GSL may fuse multiply-adds, as for arm64, or the logarithm
  // differs, up to its last bits, which the logarithm of a Gaussian near
  // the edge of the circle amplifies (6.6e-13 at most in the draws of
  // these tests where GSL fuses them)
  ::testing::AssertionResult same_real(const double ours, const double gsl,
                                       const bool takes_log = false)
  {
#if defined(__x86_64__) || defined(_M_X64)
    return same_or_close(ours, gsl, same_log_as_gsl or not takes_log);
#else
    return same_or_close(ours, gsl, false);
#endif
  }

  // Whether a draw of exponential(), a product of its logarithm, which no
  // multiply-add fuses, is the one of GSL: bit by bit where the
  // logarithm is that of GSL; elsewhere, up to its last bits
  ::testing::AssertionResult same_exponential(const double ours, const double gsl)
  {
    return same_or_close(ours, gsl, same_log_as_gsl);
  }

  // A product rounded to double, as gaussian() and flat() round theirs
  double rounded(const double product)
  {
    volatile double v = product;
    return v;
  }

  using Gsl_Ptr = std::unique_ptr<gsl_rng, Gsl_Free>;

  Gsl_Ptr make_gsl() { return Gsl_Ptr(gsl_rng_alloc(gsl_rng_mt19937)); }

  // Seeds of every kind: 0, which GSL takes as 4357, small ones, the
  // limits of 32 bits, and above them where unsigned long has 64 bits
  std::vector<unsigned long> seeds()
  {
    std::vector<unsigned long> s = {0, 1, 2, 4357, 5489, 12345, 0x7fffffffUL,
                                    0xfffffffeUL, 0xffffffffUL, 1730000000UL};
    if constexpr (sizeof(unsigned long) > 4)
      {
        s.push_back(static_cast<unsigned long>(0x100000000ULL));
        s.push_back(static_cast<unsigned long>(0x123456789abcULL));
        s.push_back(ULONG_MAX);
      }
    return s;
  }

  // Bounds for uniform_int(): 1, small ones, powers of two and their
  // neighbors, and the largest ones
  std::vector<unsigned long> bounds()
  {
    std::vector<unsigned long> b = {1, 2, 3, 5, 7, 10, 100, 1000, 1023, 1024, 1025,
                                    65535, 65536, 1000003, 0x7fffffffUL, 0x80000000UL,
                                    0x80000001UL, 0xfffffffeUL, 0xffffffffUL};
    return b;
  }
}

TEST(GslCompatibleRng, TheDefaultSeedIsTheOneOfGsl)
{
  Gsl_Ptr r = make_gsl(); // no gsl_rng_set(): the seed 0, so 4357
  Gsl_Compatible_Rng g;
  for (int i = 0; i < 10000; ++i)
    ASSERT_EQ(g.get(), gsl_rng_get(r.get())) << "at " << i;

  EXPECT_EQ(Gsl_Compatible_Rng::min(), gsl_rng_min(r.get()));
  EXPECT_EQ(Gsl_Compatible_Rng::max(), gsl_rng_max(r.get()));
}

TEST(GslCompatibleRng, EveryDrawIsTheOneOfGsl)
{
  Gsl_Ptr r = make_gsl();
  const auto bs = bounds();
  size_t draws = 0;
  for (unsigned long s : seeds())
    {
      gsl_rng_set(r.get(), s);
      Gsl_Compatible_Rng g(s);
      std::mt19937 order(static_cast<std::uint32_t>(s ^ 0x5a5a5a5aUL));
      for (int i = 0; i < 20000; ++i, ++draws)
        switch (order() % 8)
          {
          case 0:
            ASSERT_EQ(g(), gsl_rng_get(r.get())) << "seed " << s << " at " << i;
            break;
          case 1:
            ASSERT_EQ(g.get(), gsl_rng_get(r.get())) << "seed " << s << " at " << i;
            break;
          case 2:
            {
              const unsigned long n = bs[order() % bs.size()];
              ASSERT_EQ(g.uniform_int(n), gsl_rng_uniform_int(r.get(), n))
                << "seed " << s << " at " << i << ", n = " << n;
              break;
            }
          case 3:
            ASSERT_EQ(g.uniform(), gsl_rng_uniform(r.get())) << "seed " << s << " at " << i;
            break;
          case 4:
            ASSERT_EQ(g.uniform_pos(), gsl_rng_uniform_pos(r.get()))
              << "seed " << s << " at " << i;
            break;
          case 5:
            ASSERT_TRUE(same_real(g.flat(-3.5, 7.25), gsl_ran_flat(r.get(), -3.5, 7.25)))
              << "seed " << s << " at " << i;
            break;
          case 6:
            ASSERT_TRUE(same_real(g.gaussian(2.5), gsl_ran_gaussian(r.get(), 2.5), true))
              << "seed " << s << " at " << i;
            break;
          default:
            ASSERT_TRUE(same_exponential(g.exponential(0.75),
                                         gsl_ran_exponential(r.get(), 0.75)))
              << "seed " << s << " at " << i;
            break;
          }
    }
  EXPECT_EQ(draws, 20000 * seeds().size());
}

TEST(GslCompatibleRng, TheRealDrawsRoundEachProduct)
{
  // flat() and gaussian() are the formulas of GSL with each product
  // rounded, bit by bit, whatever the options of the build: a fused
  // multiply-add would show here on a platform that has them
  for (unsigned long s : seeds())
    {
      Gsl_Compatible_Rng g(s), twin(s);
      for (int i = 0; i < 2000; ++i)
        {
          const double a = 0.1, b = 1.7; // their products are not exact
          const double u = twin.uniform();
          ASSERT_EQ(g.flat(a, b), rounded(a * (1 - u)) + rounded(b * u))
            << "seed " << s << " at " << i;

          double x, y, r2;
          do
            {
              x = -1 + 2 * twin.uniform_pos();
              y = -1 + 2 * twin.uniform_pos();
              r2 = rounded(x * x) + rounded(y * y);
            }
          while (r2 > 1.0 or r2 == 0);
          ASSERT_EQ(g.gaussian(2.5), 2.5 * y * std::sqrt(-2.0 * std::log(r2) / r2))
            << "seed " << s << " at " << i;
        }
      EXPECT_EQ(g, twin);
    }
}

TEST(GslCompatibleRng, SeedingAgainIsAsGslRngSet)
{
  Gsl_Ptr r = make_gsl();
  Gsl_Compatible_Rng g(99);
  for (unsigned long s : seeds())
    {
      (void) g.get(); // a state that the seed must forget
      g.seed(s);
      gsl_rng_set(r.get(), s);
      for (int i = 0; i < 1000; ++i)
        ASSERT_EQ(g.get(), gsl_rng_get(r.get())) << "seed " << s << " at " << i;
    }

  // 0 is 4357, and else only the low 32 bits count: 2^32 seeds with 0
  EXPECT_EQ(Gsl_Compatible_Rng(0), Gsl_Compatible_Rng(4357));
  if constexpr (sizeof(unsigned long) > 4)
    {
      EXPECT_EQ(Gsl_Compatible_Rng(static_cast<unsigned long>(0x100000005ULL)),
                Gsl_Compatible_Rng(5));
      EXPECT_FALSE(Gsl_Compatible_Rng(static_cast<unsigned long>(0x100000000ULL))
                   == Gsl_Compatible_Rng(4357));
    }
}

TEST(GslCompatibleRng, TheNumbersOfTheStandard)
{
  // The 10000th number of a Mersenne Twister seeded with 5489, which the
  // standard requires of std::mt19937: the same on every platform, without
  // GSL
  Gsl_Compatible_Rng g(5489);
  Gsl_Compatible_Rng::result_type x = 0;
  for (int i = 0; i < 10000; ++i)
    x = g();
  EXPECT_EQ(x, 4123659995u);
}

TEST(GslCompatibleRng, TheNumbersAreThoseOfStdMt19937)
{
  // The generator keeps the Mersenne Twister itself: its numbers are those
  // of a std::mt19937 seeded as GSL seeds, with 4357 for 0, and else with
  // the low 32 bits, which may be 0, through several generations of the
  // state, and after seeding both again
  const auto mt_seed = [] (const unsigned long x)
    {
      return x == 0 ? std::uint32_t(4357) : static_cast<std::uint32_t>(x & 0xffffffffUL);
    };
  for (const unsigned long s : seeds())
    {
      Gsl_Compatible_Rng g(s);
      std::mt19937 e(mt_seed(s));
      for (int i = 0; i < 3 * 624 + 7; ++i)
        ASSERT_EQ(g(), e()) << "seed " << s << ", number " << i;
      g.seed(s + 1);
      e.seed(mt_seed(s + 1));
      for (int i = 0; i < 700; ++i)
        ASSERT_EQ(g(), e()) << "seed " << s << " + 1, number " << i;
    }

  // Its state is 624 words of 32 bits and the position of the next one
  EXPECT_EQ(sizeof(Gsl_Compatible_Rng), 624 * sizeof(std::uint32_t) + sizeof(int));
}

TEST(GslCompatibleRng, UniformIntRejectsZeroAndTooLargeBounds)
{
  Gsl_Compatible_Rng g;
  EXPECT_THROW((void) g.uniform_int(0), std::domain_error);
  if constexpr (sizeof(unsigned long) > 4)
    {
      EXPECT_THROW((void) g.uniform_int(static_cast<unsigned long>(0x100000000ULL)),
                   std::domain_error);
    }

  // the largest bound is right, and 1 always gives 0
  for (int i = 0; i < 1000; ++i)
    {
      EXPECT_LT(g.uniform_int(Gsl_Compatible_Rng::max()), Gsl_Compatible_Rng::max());
      EXPECT_EQ(g.uniform_int(1), 0u);
    }
}

TEST(GslCompatibleRng, RangesOfTheDraws)
{
  Gsl_Compatible_Rng g(7);
  for (int i = 0; i < 100000; ++i)
    {
      const double u = g.uniform();
      ASSERT_GE(u, 0.0);
      ASSERT_LT(u, 1.0);
      const double p = g.uniform_pos();
      ASSERT_GT(p, 0.0);
      ASSERT_LT(p, 1.0);
      const double f = g.flat(-2.0, 3.0);
      ASSERT_GE(f, -2.0);
      ASSERT_LE(f, 3.0);
      ASSERT_GE(g.exponential(1.5), 0.0);
      ASSERT_LT(g.uniform_int(10), 10u);
    }
}

TEST(GslCompatibleRng, MomentsOfTheDistributions)
{
  // the mean and the variance of 200000 draws, within a few of their
  // standard errors
  Gsl_Compatible_Rng g(2026);
  const int n = 200000;
  double s = 0, s2 = 0, e = 0, f = 0;
  std::vector<int> counts(10, 0);
  for (int i = 0; i < n; ++i)
    {
      const double x = g.gaussian(2.0);
      s += x;
      s2 += x * x;
      e += g.exponential(3.0);
      f += g.flat(1.0, 5.0);
      ++counts[g.uniform_int(10)];
    }
  EXPECT_NEAR(s / n, 0.0, 0.03);
  EXPECT_NEAR(s2 / n, 4.0, 0.05);
  EXPECT_NEAR(e / n, 3.0, 0.03);
  EXPECT_NEAR(f / n, 3.0, 0.02);
  for (int c : counts)
    EXPECT_NEAR(c, n / 10, 600);
}

TEST(GslCompatibleRng, ACopyGoesOnWithTheSameNumbers)
{
  Gsl_Compatible_Rng g(31);
  for (int i = 0; i < 100; ++i)
    (void) g();

  Gsl_Compatible_Rng c = g;
  EXPECT_EQ(c, g);
  for (int i = 0; i < 1000; ++i)
    ASSERT_EQ(c.uniform_int(1000), g.uniform_int(1000));

  (void) c();
  EXPECT_FALSE(c == g);
}

TEST(GslCompatibleRng, FeedsTheStandardLibrary)
{
  static_assert(std::uniform_random_bit_generator<Gsl_Compatible_Rng>);

  std::vector<int> v(100);
  std::iota(v.begin(), v.end(), 0);
  Gsl_Compatible_Rng a(5), b(5);
  auto w = v;
  std::shuffle(v.begin(), v.end(), a);
  std::shuffle(w.begin(), w.end(), b);
  EXPECT_EQ(v, w);
  EXPECT_TRUE(std::is_permutation(v.begin(), v.end(), w.begin()));

  std::uniform_real_distribution<double> d(0.0, 1.0);
  const double x = d(a);
  EXPECT_GE(x, 0.0);
  EXPECT_LT(x, 1.0);
}
