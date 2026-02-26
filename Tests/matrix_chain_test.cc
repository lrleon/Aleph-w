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
 * @file matrix_chain_test.cc
 * @brief Tests for Matrix_Chain.H.
 */

# include <gtest/gtest.h>

# include <limits>
# include <random>

# include <Matrix_Chain.H>

using namespace Aleph;

namespace
{
  size_t brute_cost(const Array<size_t> & dims, const size_t i, const size_t j)
  {
    if (i == j)
      return 0;

    size_t best = std::numeric_limits<size_t>::max();
    for (size_t k = i; k < j; ++k)
      {
        const size_t left = brute_cost(dims, i, k);
        const size_t right = brute_cost(dims, k + 1, j);
        const size_t here = dims[i] * dims[k + 1] * dims[j + 1];
        const size_t total = left + right + here;
        if (total < best)
          best = total;
      }
    return best;
  }

  size_t split_cost(const Array<size_t> & dims,
                    const Array<Array<size_t>> & split,
                    const size_t i, const size_t j)
  {
    if (i == j)
      return 0;
    const size_t k = split[i][j];
    return split_cost(dims, split, i, k)
      + split_cost(dims, split, k + 1, j)
      + dims[i] * dims[k + 1] * dims[j + 1];
  }
} // namespace


TEST(MatrixChain, CLRS_Example)
{
  // CLRS classic: dims = {30, 35, 15, 5, 10, 20, 25} -> 15125
  Array<size_t> dims = {30, 35, 15, 5, 10, 20, 25};
  auto r = matrix_chain_order(dims);
  EXPECT_EQ(r.min_multiplications, 15125u);
  EXPECT_FALSE(r.parenthesization.empty());
}

TEST(MatrixChain, SingleMatrix)
{
  Array<size_t> dims = {10, 20};
  auto r = matrix_chain_order(dims);
  EXPECT_EQ(r.min_multiplications, 0u);
  EXPECT_EQ(r.parenthesization, "A1");
}

TEST(MatrixChain, TwoMatrices)
{
  Array<size_t> dims = {10, 20, 30};
  auto r = matrix_chain_order(dims);
  EXPECT_EQ(r.min_multiplications, 6000u); // 10*20*30
}

TEST(MatrixChain, ThreeMatrices)
{
  // A1: 10x30, A2: 30x5, A3: 5x60
  // (A1 A2) A3: 10*30*5 + 10*5*60 = 1500 + 3000 = 4500
  // A1 (A2 A3): 30*5*60 + 10*30*60 = 9000 + 18000 = 27000
  Array<size_t> dims = {10, 30, 5, 60};
  auto r = matrix_chain_order(dims);
  EXPECT_EQ(r.min_multiplications, 4500u);
}

TEST(MatrixChain, MinCostOnly)
{
  Array<size_t> dims = {30, 35, 15, 5, 10, 20, 25};
  EXPECT_EQ(matrix_chain_min_cost(dims), 15125u);
}

TEST(MatrixChain, InvalidInput)
{
  Array<size_t> too_short = {10};
  EXPECT_THROW(matrix_chain_order(too_short), std::domain_error);

  Array<size_t> empty;
  EXPECT_THROW(matrix_chain_order(empty), std::domain_error);

  Array<size_t> zero_dim = {10, 0, 5};
  EXPECT_THROW(matrix_chain_order(zero_dim), std::domain_error);
}

TEST(MatrixChain, ParenthesizationWellFormed)
{
  Array<size_t> dims = {30, 35, 15, 5, 10, 20, 25};
  auto r = matrix_chain_order(dims);

  // Count that parenthesization contains A1..A6
  for (int i = 1; i <= 6; ++i)
    {
      std::string name = "A" + std::to_string(i);
      EXPECT_NE(r.parenthesization.find(name), std::string::npos)
        << "Missing " << name << " in: " << r.parenthesization;
    }

  // Count balanced parentheses
  int depth = 0;
  for (char c : r.parenthesization)
    {
      if (c == '(') ++depth;
      else if (c == ')') --depth;
      EXPECT_GE(depth, 0);
    }
  EXPECT_EQ(depth, 0);
}

TEST(MatrixChain, IdenticalDimensions)
{
  // All square matrices of same size: any order is equally optimal
  Array<size_t> dims = {5, 5, 5, 5, 5};
  auto r = matrix_chain_order(dims);
  // 4 matrices, each 5x5. Any parenthesization costs (n-1)*5^3 = 3*125 = 375
  EXPECT_EQ(r.min_multiplications, 375u);
}

TEST(MatrixChain, SplitTableMatchesReportedCost)
{
  Array<size_t> dims = {9, 4, 8, 3, 6, 7};
  auto r = matrix_chain_order(dims);
  const size_t n = dims.size() - 1;
  EXPECT_EQ(split_cost(dims, r.split, 0, n - 1), r.min_multiplications);
}

TEST(MatrixChain, RandomSmallVsBruteForce)
{
  std::mt19937 rng(20260226);
  for (int trial = 0; trial < 120; ++trial)
    {
      const size_t n = 2 + rng() % 6; // 2..7 matrices
      Array<size_t> dims;
      dims.reserve(n + 1);
      for (size_t i = 0; i <= n; ++i)
        dims.append(1 + static_cast<size_t>(rng() % 25));

      const auto r = matrix_chain_order(dims);
      const size_t brute = brute_cost(dims, 0, n - 1);
      EXPECT_EQ(r.min_multiplications, brute);
      EXPECT_EQ(split_cost(dims, r.split, 0, n - 1), r.min_multiplications);
    }
}

TEST(MatrixChain, OverflowThrows)
{
  const size_t M = std::numeric_limits<size_t>::max();
  Array<size_t> dims = {M, M, M};
  EXPECT_THROW(matrix_chain_order(dims), std::runtime_error);
}
