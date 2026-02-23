
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
 * @file hungarian_test.cc
 * @brief Comprehensive tests for Hungarian.H
 *
 * Tests cover:
 * - Basic assignment problems (1x1, 2x2, 3x3, 4x4)
 * - Rectangular matrices
 * - Degenerate cases (all zeros, identical costs, diagonal)
 * - Numeric types (int, double, negative costs)
 * - Maximization
 * - API (initializer_list, free functions, consistency)
 * - Validation (permutation property, cost correctness)
 * - Stress tests (random large matrices)
 */

# include <gtest/gtest.h>
# include <random>
# include <set>
# include <Hungarian.H>
# include <tpl_dynMat.H>
# include <tpl_mincost.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Helper: build a DynMatrix from a 2D initializer list
// ============================================================================
template <typename T>
DynMatrix<T> make_matrix(initializer_list<initializer_list<T>> rows)
{
  size_t m = rows.size();
  size_t n = rows.begin()->size();
  DynMatrix<T> mat(m, n, T{0});
  mat.allocate();
  size_t i = 0;
  for (const auto & row : rows)
    {
      size_t j = 0;
      for (const auto & val : row)
        mat(i, j++) = val;
      ++i;
    }
  return mat;
}

// Helper: compute the actual cost of an assignment using the cost matrix
template <typename T>
T compute_assignment_cost(const DynMatrix<T> & cost,
                          const DynList<pair<size_t, size_t>> & pairs)
{
  T total = T{0};
  for (auto it = pairs.get_it(); it.has_curr(); it.next())
    {
      auto [r, c] = it.get_curr();
      total += cost.read(r, c);
    }
  return total;
}

// Helper: verify the assignment is a valid permutation
void verify_permutation(const Array<long> & row_to_col,
                        const Array<long> & col_to_row,
                        size_t rows, size_t cols)
{
  // Each assigned row maps to a unique column
  set<long> assigned_cols;
  for (size_t i = 0; i < rows; ++i)
    {
      long j = row_to_col[i];
      if (j >= 0)
        {
          EXPECT_LT(static_cast<size_t>(j), cols)
            << "Row " << i << " assigned to out-of-range column " << j;
          EXPECT_TRUE(assigned_cols.insert(j).second)
            << "Column " << j << " assigned to multiple rows";
        }
    }

  // Each assigned column maps back correctly
  for (size_t j = 0; j < cols; ++j)
    {
      long i = col_to_row[j];
      if (i >= 0)
        {
          EXPECT_LT(static_cast<size_t>(i), rows)
            << "Column " << j << " assigned to out-of-range row " << i;
          EXPECT_EQ(row_to_col[static_cast<size_t>(i)],
                    static_cast<long>(j))
            << "Inconsistent row/col mapping at row=" << i << " col=" << j;
        }
    }
}

// ============================================================================
// HungarianBasic: small matrices with known solutions
// ============================================================================

TEST(HungarianBasic, Matrix1x1)
{
  auto mat = make_matrix<int>({{42}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 42);
  EXPECT_EQ(ha.get_assignment(0), 0);
  EXPECT_EQ(ha.rows(), 1u);
  EXPECT_EQ(ha.cols(), 1u);
}

TEST(HungarianBasic, Matrix2x2)
{
  // Cost matrix:
  //   1  2
  //   3  0
  // Optimal: (0,0)=1, (1,1)=0 -> cost=1
  auto mat = make_matrix<int>({{1, 2}, {3, 0}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 1);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(compute_assignment_cost(mat, pairs), 1);
}

TEST(HungarianBasic, Matrix3x3)
{
  // Classic example:
  //  10  5 13
  //   3  9 18
  //  10  6 12
  // Optimal: (0,1)=5, (1,0)=3, (2,2)=12 -> cost=20
  auto mat = make_matrix<int>({{10, 5, 13}, {3, 9, 18}, {10, 6, 12}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 20);
}

TEST(HungarianBasic, Matrix4x4)
{
  // Example:
  //  82 83 69 92
  //  77 37 49 92
  //  11 69  5 86
  //   8  9 98 23
  // Optimal: (0,2)=69, (1,1)=37, (2,0)=11, (3,3)=23 -> cost=140
  auto mat = make_matrix<int>({
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  });
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 140);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(compute_assignment_cost(mat, pairs), 140);
}

TEST(HungarianBasic, RankOneMatrix4x4)
{
  // Rank-1 matrix c[i][j] = (i+1)*(j+1):
  //  1 2 3 4
  //  2 4 6 8
  //  3 6 9 12
  //  4 8 12 16
  // Optimal: anti-diagonal (3,2,1,0) -> 4+6+6+4 = 20
  auto mat = make_matrix<int>({
    {1, 2, 3, 4},
    {2, 4, 6, 8},
    {3, 6, 9, 12},
    {4, 8, 12, 16}
  });
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 20);
}

// ============================================================================
// HungarianRectangular: non-square matrices
// ============================================================================

TEST(HungarianRectangular, MoreColsThanRows_2x3)
{
  // 2 workers, 3 tasks. One task will be unassigned.
  //  1 2 3
  //  4 5 6
  // Optimal: assign to minimize cost -> (0,0)=1, (1,1)=5 -> cost=6
  auto mat = make_matrix<int>({{1, 2, 3}, {4, 5, 6}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 6);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 2u); // 2 pairs (one per row)
}

TEST(HungarianRectangular, MoreRowsThanCols_3x2)
{
  // 3 workers, 2 tasks. One worker will be unassigned.
  //  1 4
  //  2 5
  //  3 6
  // Optimal: (0,0)=1, (1,1)=5 -> cost=6
  auto mat = make_matrix<int>({{1, 4}, {2, 5}, {3, 6}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 6);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 2u); // min(rows, cols) assignments
}

TEST(HungarianRectangular, Wide_2x5)
{
  //  10 3 7 2 8
  //   5 9 1 6 4
  // Optimal: (0,3)=2, (1,2)=1 -> cost=3
  auto mat = make_matrix<int>({{10, 3, 7, 2, 8}, {5, 9, 1, 6, 4}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 3);
}

TEST(HungarianRectangular, Tall_5x2)
{
  //  10 5
  //   3 9
  //   7 1
  //   2 6
  //   8 4
  // Optimal: (3,0)=2, (2,1)=1 -> cost=3
  auto mat = make_matrix<int>({{10, 5}, {3, 9}, {7, 1}, {2, 6}, {8, 4}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 3);
}

// ============================================================================
// HungarianDegenerate: edge cases
// ============================================================================

TEST(HungarianDegenerate, AllZeros)
{
  auto mat = make_matrix<int>({{0, 0, 0}, {0, 0, 0}, {0, 0, 0}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 0);
}

TEST(HungarianDegenerate, AllSameCost)
{
  auto mat = make_matrix<int>({{7, 7, 7}, {7, 7, 7}, {7, 7, 7}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 21);
}

TEST(HungarianDegenerate, DiagonalZeros)
{
  // Diagonal is zero, off-diagonal is large
  auto mat = make_matrix<int>({{0, 99, 99}, {99, 0, 99}, {99, 99, 0}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 0);

  EXPECT_EQ(ha.get_assignment(0), 0);
  EXPECT_EQ(ha.get_assignment(1), 1);
  EXPECT_EQ(ha.get_assignment(2), 2);
}

TEST(HungarianDegenerate, Identity)
{
  // Cost = identity matrix
  auto mat = make_matrix<int>({{1, 0, 0}, {0, 1, 0}, {0, 0, 1}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 0);
}

TEST(HungarianDegenerate, SingleRow)
{
  auto mat = make_matrix<int>({{5, 3, 8, 1}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 1);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 1u);
}

TEST(HungarianDegenerate, SingleColumn)
{
  auto mat = make_matrix<int>({{5}, {3}, {8}, {1}});
  Hungarian_Assignment<int> ha(mat);

  EXPECT_EQ(ha.get_total_cost(), 1);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 1u);
}

// ============================================================================
// HungarianNumerics: different numeric types and negative costs
// ============================================================================

TEST(HungarianNumerics, IntegerCosts)
{
  auto mat = make_matrix<int>({{10, 5, 13}, {3, 9, 18}, {10, 6, 12}});
  Hungarian_Assignment<int> ha(mat);
  EXPECT_EQ(ha.get_total_cost(), 20);
}

TEST(HungarianNumerics, DoubleCosts)
{
  auto mat = make_matrix<double>({{1.5, 2.5}, {3.5, 0.5}});
  Hungarian_Assignment<double> ha(mat);
  EXPECT_DOUBLE_EQ(ha.get_total_cost(), 2.0);
}

TEST(HungarianNumerics, NegativeCosts)
{
  // All negative costs
  auto mat = make_matrix<int>({{-1, -2, -3}, {-4, -5, -6}, {-7, -8, -9}});
  Hungarian_Assignment<int> ha(mat);

  // Minimum cost: (0,2)=-3, (1,1)=-5, (2,0)=-7 or similar
  // The minimum sum is -3 + -5 + -7 = -15? Let's check:
  // Actually: (0,2)=-3, (1,0)=-4, (2,1)=-8 = -15
  // Or: (0,0)=-1, (1,1)=-5, (2,2)=-9 = -15
  // Or: (0,1)=-2, (1,2)=-6, (2,0)=-7 = -15
  // All permutations give -15 since c[i][j] = -(3*i + j + 1)
  // Actually no. Let me recompute:
  // row 0: -1 -2 -3
  // row 1: -4 -5 -6
  // row 2: -7 -8 -9
  // Minimum cost assignment minimizes sum:
  // (0,2)=-3, (1,1)=-5, (2,0)=-7: sum=-15
  // (0,0)=-1, (1,1)=-5, (2,2)=-9: sum=-15
  // (0,1)=-2, (1,0)=-4, (2,2)=-9: sum=-15
  // All give -15. This matrix has all-equal permutation sums.
  EXPECT_EQ(ha.get_total_cost(), -15);
}

TEST(HungarianNumerics, MixedSigns)
{
  auto mat = make_matrix<int>({{-5, 3}, {2, -7}});
  Hungarian_Assignment<int> ha(mat);

  // Options: (0,0)=-5 + (1,1)=-7 = -12  or  (0,1)=3 + (1,0)=2 = 5
  EXPECT_EQ(ha.get_total_cost(), -12);
}

TEST(HungarianNumerics, LargeCosts)
{
  auto mat = make_matrix<long>({{1000000, 2000000}, {3000000, 500000}});
  Hungarian_Assignment<long> ha(mat);

  // (0,0)=1000000 + (1,1)=500000 = 1500000
  EXPECT_EQ(ha.get_total_cost(), 1500000L);
}

// ============================================================================
// HungarianMaximization: maximize profit
// ============================================================================

TEST(HungarianMaximization, Basic3x3)
{
  auto mat = make_matrix<int>({{10, 5, 13}, {3, 9, 18}, {10, 6, 12}});
  auto result = hungarian_max_assignment(mat);

  // Maximum: (0,2)=13, (1,1)=9, (2,0)=10 = 32
  // Or: (0,0)=10, (1,2)=18, (2,1)=6 = 34
  // Let's check all 6 permutations:
  // (0,1,2): 10+9+12=31
  // (0,2,1): 10+18+6=34
  // (1,0,2): 5+3+12=20
  // (1,2,0): 5+18+10=33
  // (2,0,1): 13+3+6=22
  // (2,1,0): 13+9+10=32
  // Max is 34
  EXPECT_EQ(result.total_cost, 34);
}

TEST(HungarianMaximization, Rectangular)
{
  auto mat = make_matrix<int>({{1, 2, 3}, {4, 5, 6}});
  auto result = hungarian_max_assignment(mat);

  // Maximum: (0,1)=2, (1,2)=6 = 8 or (0,2)=3, (1,1)=5 = 8
  EXPECT_EQ(result.total_cost, 8);
}

// ============================================================================
// HungarianAPI: interface correctness
// ============================================================================

TEST(HungarianAPI, InitializerListConstructor)
{
  Hungarian_Assignment<int> ha({
    {10, 5, 13},
    {3,  9, 18},
    {10, 6, 12}
  });

  EXPECT_EQ(ha.get_total_cost(), 20);
  EXPECT_EQ(ha.rows(), 3u);
  EXPECT_EQ(ha.cols(), 3u);
  EXPECT_EQ(ha.dimension(), 3u);
}

TEST(HungarianAPI, FreeFunctionMinimization)
{
  auto mat = make_matrix<int>({{10, 5, 13}, {3, 9, 18}, {10, 6, 12}});
  auto result = hungarian_assignment(mat);

  EXPECT_EQ(result.total_cost, 20);
  EXPECT_EQ(result.orig_rows, 3u);
  EXPECT_EQ(result.orig_cols, 3u);

  auto pairs = result.get_pairs();
  EXPECT_EQ(pairs.size(), 3u);
}

TEST(HungarianAPI, GetPairsFiltersDummies)
{
  // Rectangular 2x4: only 2 real assignments
  auto mat = make_matrix<int>({{1, 2, 3, 4}, {5, 6, 7, 8}});
  Hungarian_Assignment<int> ha(mat);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 2u);

  // Verify all pairs are within original dimensions
  for (auto it = pairs.get_it(); it.has_curr(); it.next())
    {
      auto [r, c] = it.get_curr();
      EXPECT_LT(r, 2u);
      EXPECT_LT(c, 4u);
    }
}

TEST(HungarianAPI, ConsistencyCheck)
{
  auto mat = make_matrix<int>({
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  });
  Hungarian_Assignment<int> ha(mat);

  verify_permutation(ha.get_row_assignments(), ha.get_col_assignments(),
                     ha.rows(), ha.cols());

  // Verify cost matches sum of assigned entries
  auto pairs = ha.get_assignments();
  EXPECT_EQ(compute_assignment_cost(mat, pairs), ha.get_total_cost());
}

// ============================================================================
// HungarianValidation: permutation and cost correctness
// ============================================================================

TEST(HungarianValidation, PermutationProperty)
{
  // Every valid assignment is a permutation (for square matrices)
  auto mat = make_matrix<int>({
    {5, 9, 1},
    {10, 3, 2},
    {8, 7, 4}
  });
  Hungarian_Assignment<int> ha(mat);

  set<long> assigned;
  for (size_t i = 0; i < 3; ++i)
    {
      long col = ha.get_assignment(i);
      EXPECT_GE(col, 0);
      EXPECT_LT(col, 3);
      EXPECT_TRUE(assigned.insert(col).second)
        << "Column " << col << " assigned twice";
    }
}

TEST(HungarianValidation, CostMatchesSum)
{
  auto mat = make_matrix<int>({
    {25, 40, 35},
    {40, 60, 35},
    {20, 40, 25}
  });
  Hungarian_Assignment<int> ha(mat);

  auto pairs = ha.get_assignments();
  int actual = compute_assignment_cost(mat, pairs);
  EXPECT_EQ(actual, ha.get_total_cost());
}

TEST(HungarianValidation, BruteForce3x3)
{
  // Verify against brute-force for a 3x3 matrix
  auto mat = make_matrix<int>({
    {7, 3, 5},
    {2, 8, 1},
    {6, 4, 9}
  });

  // All 6 permutations of {0,1,2}
  int perms[6][3] = {
    {0,1,2}, {0,2,1}, {1,0,2}, {1,2,0}, {2,0,1}, {2,1,0}
  };

  int min_cost = numeric_limits<int>::max();
  for (auto & perm : perms)
    {
      int cost = 0;
      for (int i = 0; i < 3; ++i)
        cost += static_cast<int>(mat.read(i, perm[i]));
      min_cost = min(min_cost, cost);
    }

  Hungarian_Assignment<int> ha(mat);
  EXPECT_EQ(ha.get_total_cost(), min_cost);
}

TEST(HungarianValidation, BruteForce4x4)
{
  auto mat = make_matrix<int>({
    {15, 4, 8, 12},
    {7, 9, 3, 6},
    {10, 14, 2, 11},
    {5, 13, 1, 16}
  });

  // Brute-force: enumerate all 24 permutations
  int perm[4] = {0, 1, 2, 3};
  int min_cost = numeric_limits<int>::max();
  do
    {
      int cost = 0;
      for (int i = 0; i < 4; ++i)
        cost += static_cast<int>(mat.read(i, perm[i]));
      min_cost = min(min_cost, cost);
    }
  while (next_permutation(perm, perm + 4));

  Hungarian_Assignment<int> ha(mat);
  EXPECT_EQ(ha.get_total_cost(), min_cost);
}

// ============================================================================
// HungarianStress: random large matrices
// ============================================================================

TEST(HungarianStress, Random10x10)
{
  mt19937 rng(42);
  uniform_int_distribution<int> dist(1, 100);

  DynMatrix<int> mat(10, 10, 0);
  mat.allocate();
  for (size_t i = 0; i < 10; ++i)
    for (size_t j = 0; j < 10; ++j)
      mat(i, j) = dist(rng);

  Hungarian_Assignment<int> ha(mat);
  verify_permutation(ha.get_row_assignments(), ha.get_col_assignments(),
                     10, 10);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 10u);
  EXPECT_EQ(compute_assignment_cost(mat, pairs), ha.get_total_cost());
}

TEST(HungarianStress, Random50x50)
{
  mt19937 rng(123);
  uniform_int_distribution<int> dist(1, 1000);

  DynMatrix<int> mat(50, 50, 0);
  mat.allocate();
  for (size_t i = 0; i < 50; ++i)
    for (size_t j = 0; j < 50; ++j)
      mat(i, j) = dist(rng);

  Hungarian_Assignment<int> ha(mat);
  verify_permutation(ha.get_row_assignments(), ha.get_col_assignments(),
                     50, 50);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 50u);
  EXPECT_EQ(compute_assignment_cost(mat, pairs), ha.get_total_cost());
}

TEST(HungarianStress, Random100x100)
{
  mt19937 rng(456);
  uniform_int_distribution<int> dist(0, 10000);

  DynMatrix<int> mat(100, 100, 0);
  mat.allocate();
  for (size_t i = 0; i < 100; ++i)
    for (size_t j = 0; j < 100; ++j)
      mat(i, j) = dist(rng);

  Hungarian_Assignment<int> ha(mat);
  verify_permutation(ha.get_row_assignments(), ha.get_col_assignments(),
                     100, 100);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 100u);
  EXPECT_EQ(compute_assignment_cost(mat, pairs), ha.get_total_cost());
}

TEST(HungarianStress, RandomRectangular_30x50)
{
  mt19937 rng(789);
  uniform_int_distribution<int> dist(1, 500);

  DynMatrix<int> mat(30, 50, 0);
  mat.allocate();
  for (size_t i = 0; i < 30; ++i)
    for (size_t j = 0; j < 50; ++j)
      mat(i, j) = dist(rng);

  Hungarian_Assignment<int> ha(mat);
  verify_permutation(ha.get_row_assignments(), ha.get_col_assignments(),
                     30, 50);

  auto pairs = ha.get_assignments();
  EXPECT_EQ(pairs.size(), 30u);
  EXPECT_EQ(compute_assignment_cost(mat, pairs), ha.get_total_cost());
}

TEST(HungarianStress, CrossValidateWithMinCostFlow)
{
  // Cross-validate with solve_assignment() from tpl_mincost.H
  // for a small matrix where we can use both approaches
  auto mat = make_matrix<int>({
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  });

  Hungarian_Assignment<int> ha(mat);

  // Build std::vector for solve_assignment
  vector<vector<double>> std_costs = {
    {82, 83, 69, 92},
    {77, 37, 49, 92},
    {11, 69,  5, 86},
    { 8,  9, 98, 23}
  };

  auto flow_result = solve_assignment(std_costs);
  ASSERT_TRUE(flow_result.feasible);

  EXPECT_EQ(ha.get_total_cost(), static_cast<int>(flow_result.total_cost));
}

TEST(HungarianStress, CrossValidateRandom20x20)
{
  mt19937 rng(999);
  uniform_int_distribution<int> dist(1, 100);

  const size_t N = 20;

  DynMatrix<int> mat(N, N, 0);
  mat.allocate();
  vector<vector<double>> std_costs(N, vector<double>(N));

  for (size_t i = 0; i < N; ++i)
    for (size_t j = 0; j < N; ++j)
      {
        int val = dist(rng);
        mat(i, j) = val;
        std_costs[i][j] = static_cast<double>(val);
      }

  Hungarian_Assignment<int> ha(mat);
  auto flow_result = solve_assignment(std_costs);

  ASSERT_TRUE(flow_result.feasible);
  EXPECT_EQ(ha.get_total_cost(), static_cast<int>(flow_result.total_cost));
}
