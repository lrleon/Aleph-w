
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
 * @file opbintree_test.cc
 * @brief build_optimal_tree() gives a binary search tree of the least
 *        weighted cost
 */

#include <gtest/gtest.h>

#include <random>

#include <opBinTree.H>
#include <tpl_array.H>
#include <tpl_binNode.H>
#include <tpl_binNodeUtils.H>

using namespace Aleph;

namespace
{
  using Node = BinNode<int>;

  // The cost of a tree whose keys are the indexes of p: the sum of the
  // weight of each key times its depth, with the root at depth 1. `next`
  // follows the keys in order: it ends as the count of the keys only if
  // they are 0, 1, 2... in the inorder of the tree
  double cost_of(Node * root, const Array<double> & p, const int depth, int & next)
  {
    if (root == Node::NullPtr)
      return 0;
    double cost = cost_of(LLINK(root), p, depth + 1, next);
    if (KEY(root) == next)
      ++next;
    cost += p(KEY(root)) * depth;
    return cost + cost_of(RLINK(root), p, depth + 1, next);
  }

  // The least cost of a binary search tree of those weights, by the
  // recurrence of the dynamic programming with every root tried: O(n^3)
  double least_cost(const Array<double> & p)
  {
    const size_t n = p.size();
    Array<double> cost((n + 2) * (n + 2), 0.0);
    auto at = [n, &cost] (const size_t i, const size_t j) -> double & { return cost(i * (n + 2) + j); };
    for (size_t len = 1; len <= n; ++len)
      for (size_t i = 1; i + len - 1 <= n; ++i)
        {
          const size_t j = i + len - 1;
          double sum = 0;
          for (size_t k = i; k <= j; ++k)
            sum += p(k - 1);
          double best = at(i, i - 1) + at(i + 1, j);
          for (size_t r = i + 1; r <= j; ++r)
            best = std::min(best, at(i, r - 1) + at(r + 1, j));
          at(i, j) = best + sum;
        }
    return n == 0 ? 0 : at(1, n);
  }

  void expect_an_optimal_tree(Array<double> & p)
  {
    const size_t n = p.size();
    Array<int> keys(n, 0);
    for (size_t i = 0; i < n; ++i)
      keys(i) = static_cast<int>(i);

    Node * root = build_optimal_tree<Node, int>(n == 0 ? nullptr : &keys(0),
                                                n == 0 ? nullptr : &p(0), n);
    int next = 0;
    const double cost = cost_of(root, p, 1, next);
    EXPECT_EQ(next, static_cast<int>(n)) << "the keys are not in order, with " << n << " keys";
    EXPECT_NEAR(cost, least_cost(p), 1e-9) << "with " << n << " keys";
    destroyRec(root);
  }
}

TEST(OptimalBinTree, WithoutKeysItIsEmpty)
{
  Array<double> p;
  EXPECT_EQ((build_optimal_tree<Node, int>(nullptr, nullptr, 0)), Node::NullPtr);
  expect_an_optimal_tree(p);
}

// Random weights, against the least cost that trying every root gives
TEST(OptimalBinTree, ItsCostIsTheLeastOne)
{
  std::mt19937 rng(20261125);
  std::uniform_real_distribution<double> weight(0.0, 1.0);
  for (size_t n = 1; n <= 40; ++n)
    for (int trial = 0; trial < 5; ++trial)
      {
        Array<double> p(n, 0.0);
        for (size_t i = 0; i < n; ++i)
          p(i) = weight(rng);
        expect_an_optimal_tree(p);
      }
}

// Equal weights, weights of zero and one key far heavier than the others
TEST(OptimalBinTree, ItsCostIsTheLeastOneWithTiesAndZeros)
{
  for (size_t n = 1; n <= 33; ++n)
    {
      Array<double> equal(n, 0.25);
      expect_an_optimal_tree(equal);

      Array<double> zeros(n, 0.0);
      expect_an_optimal_tree(zeros);

      Array<double> some(n, 0.0);
      for (size_t i = 0; i < n; i += 3)
        some(i) = 1.0;
      expect_an_optimal_tree(some);

      Array<double> heavy(n, 0.001);
      heavy(n - 1) = 1000.0;
      expect_an_optimal_tree(heavy);
    }
}
