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
* @file math_nt_test.cc
* @brief Tests for Number Theory and Modular Arithmetic features.
*/

# include <gtest/gtest.h>
# include <chrono>
# include <cstdlib>
# include <random>
# include <algorithm>
# include <numeric>
# include <modular_arithmetic.H>

# include <primality.H>
# include <pollard_rho.H>
# include <modular_combinatorics.H>
# include <modular_linalg.H>
# include <tpl_dynMat.H>

using namespace Aleph;

TEST(MathNT, ModMulAndExp)
{
  uint64_t m = 1000000007;
  EXPECT_EQ(mod_mul(2, 3, m), 6);
  EXPECT_EQ(mod_exp(2, 10, m), 1024);
  
  // Test mod_exp with m = 1
  EXPECT_EQ(mod_exp(2, 10, 1), 0ULL);
  EXPECT_EQ(mod_exp(2, 0, 1), 0ULL);
  
  uint64_t big_m = 4000000000000000000ULL; // > 32 bits
  uint64_t a = 3000000000000000000ULL;
  uint64_t b = 2000000000000000000ULL;
  // 3e18 * 2e18 = 6e36
  // 6e36 % 4e18 = 0
  EXPECT_EQ(mod_mul(a, b, big_m), 0ULL);
  
  EXPECT_EQ(mod_mul(a + 1, b, big_m), 2000000000000000000ULL);
}

TEST(MathNT, ExtGCDAndInv)
{
  int64_t x, y;
  int64_t g = ext_gcd<int64_t>(30, 20, x, y);
  EXPECT_EQ(g, 10);
  EXPECT_EQ(30 * x + 20 * y, 10);

  EXPECT_EQ(mod_inv(3, 11), 4); // 3 * 4 = 12 = 1 mod 11
  EXPECT_THROW(mod_inv(2, 4), std::invalid_argument);
}

TEST(MathNT, CRT)
{
  Array<uint64_t> rem = {2, 3, 2};
  Array<uint64_t> mod = {3, 5, 7};
  // x = 2 mod 3
  // x = 3 mod 5
  // x = 2 mod 7
  // Answer is 23
  EXPECT_EQ(crt(rem, mod), 23);
}

TEST(MathNT, MillerRabin)
{
  EXPECT_FALSE(miller_rabin(1));
  EXPECT_TRUE(miller_rabin(2));
  EXPECT_TRUE(miller_rabin(3));
  EXPECT_FALSE(miller_rabin(4));
  EXPECT_TRUE(miller_rabin(97));
  EXPECT_FALSE(miller_rabin(100));
  EXPECT_TRUE(miller_rabin(1000000007));
  EXPECT_TRUE(miller_rabin(2147483647)); 

  // Property-based checks with fixed seed
  std::mt19937_64 rng(42);
  for (int i = 0; i < 100; ++i)
  {
    uint64_t n = rng() % 1000000;
    if (n < 2) continue;
    bool is_p = miller_rabin(n);
    
    // Simple primality oracle for validation
    bool oracle = true;
    for (uint64_t d = 2; d * d <= n; ++d)
      if (n % d == 0) { oracle = false; break; }
    EXPECT_EQ(is_p, oracle) << "n=" << n;
  }
}

TEST(MathNT, PollardRho)
{
  auto factors = pollard_rho(12);
  Array<uint64_t> exp12 = {2, 2, 3};
  EXPECT_EQ(factors, exp12);

  auto factors_prime = pollard_rho(97);
  Array<uint64_t> exp97 = {97};
  EXPECT_EQ(factors_prime, exp97);

  auto big_factors = pollard_rho(1000000007ULL * 97ULL);
  Array<uint64_t> exp_big = {97, 1000000007};
  EXPECT_EQ(big_factors, exp_big);

  // Randomized property check
  std::mt19937_64 rng(1337);
  for (int i = 0; i < 20; ++i)
  {
    uint64_t n = rng() % 1000000000000ULL + 2;
    auto res = pollard_rho(n);
    
    uint64_t prod = 1;
    for (const auto f : res)
    {
      EXPECT_TRUE(miller_rabin(f)) << "Factor " << f << " of " << n << " is not prime";
      prod *= f;
    }
    EXPECT_EQ(prod, n) << "Product of factors doesn't match original n";
  }
}

TEST(MathNT, PerformanceRegression)
{
  if (not std::getenv("ENABLE_PERF_TESTS"))
    GTEST_SKIP() << "Skipping performance regression (set ENABLE_PERF_TESTS to enable)";

  // 1. Pollard Rho on a large composite (product of two 31-bit primes)
  uint64_t p1 = 2147483647; // prime
  uint64_t p2 = 1000000007; // prime
  uint64_t n = p1 * p2;

  auto start = std::chrono::steady_clock::now();
  auto factors = pollard_rho(n);
  auto end = std::chrono::steady_clock::now();
  
  auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
  
  EXPECT_EQ(factors.size(), 2);
  EXPECT_TRUE(std::find(factors.begin(), factors.end(), p1) != factors.end());
  EXPECT_TRUE(std::find(factors.begin(), factors.end(), p2) != factors.end());
  
  EXPECT_LE(duration, 500) << "Pollard-Rho performance regression detected";

}

TEST(MathNT, ModularCombinatorics)
{
  ModularCombinatorics mc(10, 1000000007);
  EXPECT_EQ(mc.nCk(5, 2), 10);
  EXPECT_EQ(mc.nCk(5, 0), 1);
  EXPECT_EQ(mc.nCk(5, 6), 0);
  EXPECT_EQ(mc.nCk(0, 0), 1);
  EXPECT_EQ(mc.nCk(10, 5), 252);
  
  // Bounds check
  EXPECT_THROW(mc.nCk(11, 2), std::out_of_range);

  // Lucas Theorem test
  ModularCombinatorics mc_small(10, 3); // Modulo 3
  // C(5, 2) = 10 = 1 mod 3
  EXPECT_EQ(mc_small.lucas_nCk(5, 2), 1);
  EXPECT_EQ(mc_small.lucas_nCk(5, 0), 1);
  EXPECT_EQ(mc_small.lucas_nCk(0, 0), 1);
}

TEST(MathNT, ModularLinalg)
{
  Array<Array<uint64_t>> m_data;
  m_data.append(Array<uint64_t>({1, 2}));
  m_data.append(Array<uint64_t>({3, 4}));

  ModularMatrix mat(m_data, 5); // modulo 5
  // det = 1*4 - 2*3 = 4 - 6 = -2 = 3 mod 5
  EXPECT_EQ(mat.determinant(), 3);

  auto inv_opt = mat.inverse();
  ASSERT_TRUE(inv_opt.has_value());
  auto inv_mat = inv_opt->get();
  
  // Check A^-1 * A = I
  EXPECT_EQ((inv_mat[0][0] * 1 + inv_mat[0][1] * 3) % 5, 1);
  EXPECT_EQ((inv_mat[0][0] * 2 + inv_mat[0][1] * 4) % 5, 0);
  EXPECT_EQ((inv_mat[1][0] * 1 + inv_mat[1][1] * 3) % 5, 0);
  EXPECT_EQ((inv_mat[1][0] * 2 + inv_mat[1][1] * 4) % 5, 1);
}

TEST(MathNT, ModularSparseMatrix)
{
  // Sparse matrix with DynMatrix
  DynMatrix<uint64_t> m_data(3, 3, 0);
  m_data.write(0, 0, 2);
  m_data.write(1, 1, 3);
  m_data.write(2, 2, 4);

  ModularSparseMatrix mat(m_data, 7); // modulo 7
  // det = 2*3*4 = 24 = 3 mod 7
  EXPECT_EQ(mat.determinant(), 3);

  auto inv_opt = mat.inverse();
  ASSERT_TRUE(inv_opt.has_value());
  auto inv_mat = inv_opt->get();

  // Inverses of 2, 3, 4 mod 7 are 4, 5, 2
  EXPECT_EQ(inv_mat.read(0, 0), 4);
  EXPECT_EQ(inv_mat.read(1, 1), 5);
  EXPECT_EQ(inv_mat.read(2, 2), 2);
  EXPECT_EQ(inv_mat.read(0, 1), 0);
}

TEST(MathNT, ModularSparseMatrixRowSwap)
{
  // Matrix that requires a row swap for pivoting
  // [ 0 1 ]
  // [ 1 1 ] mod 5
  DynMatrix<uint64_t> m_data(2, 2, 0);
  m_data.write(0, 1, 1);
  m_data.write(1, 0, 1);
  m_data.write(1, 1, 1);

  ModularSparseMatrix mat(m_data, 5);
  // det = 0*1 - 1*1 = -1 = 4 mod 5
  EXPECT_EQ(mat.determinant(), 4);

  auto inv_opt = mat.inverse();
  ASSERT_TRUE(inv_opt.has_value());
  auto inv_mat = inv_opt->get();

  // Check inverse: [ 4 1 ] [ 0 1 ] = [ 1 0 ]
  //                [ 1 0 ] [ 1 1 ] = [ 0 1 ]
  EXPECT_EQ(inv_mat.read(0, 0), 4);
  EXPECT_EQ(inv_mat.read(0, 1), 1);
  EXPECT_EQ(inv_mat.read(1, 0), 1);
  EXPECT_EQ(inv_mat.read(1, 1), 0);
}

TEST(MathNT, ModularSparseMatrixSingular)
{
  // Singular matrix
  // [ 1 2 ]
  // [ 2 4 ] mod 7
  DynMatrix<uint64_t> m_data(2, 2, 0);
  m_data.write(0, 0, 1);
  m_data.write(0, 1, 2);
  m_data.write(1, 0, 2);
  m_data.write(1, 1, 4);

  ModularSparseMatrix mat(m_data, 7);
  EXPECT_EQ(mat.determinant(), 0);
  EXPECT_FALSE(mat.inverse().has_value());
}
