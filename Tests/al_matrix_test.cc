/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/

/**
 * @file al_matrix_test.cc
 * @brief Exhaustive tests for al-matrix.H (sparse matrix with generic domains)
 *
 * Tests cover:
 * - Construction (empty, initializer list, DynList)
 * - Entry access and modification (get_entry, set_entry)
 * - Sparse storage semantics (zero entries not stored)
 * - Transpose
 * - Row/column extraction (as list and as vector)
 * - Matrix-vector and vector-matrix multiplication
 * - Matrix addition and scalar multiplication
 * - Identity matrix
 * - Equality comparison
 * - Static factory methods (create_by_rows, create_by_columns)
 * - Outer product
 */

#include <gtest/gtest.h>

#include <cmath>
#include <string>
#include <vector>

#include <al-domain.H>
#include <al-vector.H>
#include <al-matrix.H>

using namespace Aleph;

namespace
{
  using Domain = AlDomain<int>;
  using DomainPtr = std::shared_ptr<Domain>;
  using Mat = Matrix<int, int, double>;
  using Vec = Vector<int, double>;

  // Helper to create a domain with elements 0..n-1
  Domain make_domain(int n)
  {
    Domain d;
    for (int i = 0; i < n; ++i)
      d.insert(i);
    return d;
  }

  // Helper to create a shared_ptr domain with elements 0..n-1
  DomainPtr make_shared_domain(int n)
  {
    auto d = std::make_shared<Domain>();
    for (int i = 0; i < n; ++i)
      d->insert(i);
    return d;
  }

  // String domain for testing generic types
  using StrDomain = AlDomain<std::string>;
  using StrDomainPtr = std::shared_ptr<StrDomain>;
  using StrMat = Matrix<std::string, std::string, double>;
  using StrVec = Vector<std::string, double>;

} // namespace

// =============================================================================
// Construction Tests
// =============================================================================

TEST(MatrixConstruction, EmptyMatrixHasZeroEntries)
{
  Domain rows = make_domain(3);
  Domain cols = make_domain(4);

  Mat m(rows, cols);

  // All entries should be zero
  for (int r = 0; r < 3; ++r)
    for (int c = 0; c < 4; ++c)
      EXPECT_DOUBLE_EQ(m.get_entry(r, c), 0.0);
}

TEST(MatrixConstruction, InitializerListConstructor)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 2), 3.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 5.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 6.0);
}

TEST(MatrixConstruction, InitializerListWithZerosNotStored)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);

  Mat m(rows, cols, {
    {1.0, 0.0},
    {0.0, 2.0}
  });

  // Diagonal matrix - only 2 non-zero entries should be stored
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 2.0);
}

TEST(MatrixConstruction, InitializerListRowMismatchThrows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  EXPECT_THROW(
    Mat m(rows, cols, {
      {1.0, 2.0, 3.0}  // only 1 row, expected 2
    }),
    std::range_error
  );
}

TEST(MatrixConstruction, InitializerListColMismatchThrows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  EXPECT_THROW(
    Mat m(rows, cols, {
      {1.0, 2.0},       // 2 cols, expected 3
      {4.0, 5.0, 6.0}
    }),
    std::range_error
  );
}

// =============================================================================
// Entry Access and Modification
// =============================================================================

TEST(MatrixEntries, SetAndGetEntry)
{
  Domain rows = make_domain(3);
  Domain cols = make_domain(3);
  Mat m(rows, cols);

  m.set_entry(1, 2, 42.5);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 42.5);

  // Other entries remain zero
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(2, 2), 0.0);
}

TEST(MatrixEntries, SetEntryToZeroRemovesIt)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);
  Mat m(rows, cols);

  m.set_entry(0, 0, 5.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 5.0);

  m.set_entry(0, 0, 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 0.0);
}

TEST(MatrixEntries, SetEntryWithinEpsilonIsZero)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);
  Mat m(rows, cols, 1e-7);  // epsilon = 1e-7

  m.set_entry(0, 0, 1e-8);  // smaller than epsilon
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 0.0);
}

TEST(MatrixEntries, OverwriteEntry)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);
  Mat m(rows, cols);

  m.set_entry(0, 1, 10.0);
  m.set_entry(0, 1, 20.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 20.0);
}

// =============================================================================
// Epsilon Handling
// =============================================================================

TEST(MatrixEpsilon, GetAndSetEpsilon)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);
  Mat m(rows, cols);

  EXPECT_DOUBLE_EQ(m.get_epsilon(), 1e-7);

  m.set_epsilon(1e-5);
  EXPECT_DOUBLE_EQ(m.get_epsilon(), 1e-5);
}

TEST(MatrixEpsilon, NegativeEpsilonThrows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(2);
  Mat m(rows, cols);

  EXPECT_THROW(m.set_epsilon(-1.0), std::domain_error);
}

// =============================================================================
// Transpose
// =============================================================================

TEST(MatrixTranspose, TransposeSwapsRowsAndCols)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  Mat mt = m.transpose();

  // Original: 2x3, Transpose: 3x2
  EXPECT_DOUBLE_EQ(mt.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(mt.get_entry(1, 0), 2.0);
  EXPECT_DOUBLE_EQ(mt.get_entry(2, 0), 3.0);
  EXPECT_DOUBLE_EQ(mt.get_entry(0, 1), 4.0);
  EXPECT_DOUBLE_EQ(mt.get_entry(1, 1), 5.0);
  EXPECT_DOUBLE_EQ(mt.get_entry(2, 1), 6.0);
}

TEST(MatrixTranspose, DoubleTransposeIsIdentity)
{
  Domain d = make_domain(3);

  Mat m(d, d, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0}
  });

  Mat mtt = m.transpose().transpose();

  EXPECT_TRUE(m == mtt);
}

// =============================================================================
// Row and Column Extraction
// =============================================================================

TEST(MatrixExtraction, GetRowAsList)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  DynList<double> row0 = m.get_row_as_list(0);
  std::vector<double> v0;
  row0.for_each([&v0](double x) { v0.push_back(x); });

  ASSERT_EQ(v0.size(), 3U);
  EXPECT_DOUBLE_EQ(v0[0], 1.0);
  EXPECT_DOUBLE_EQ(v0[1], 2.0);
  EXPECT_DOUBLE_EQ(v0[2], 3.0);
}

TEST(MatrixExtraction, GetColAsList)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  DynList<double> col1 = m.get_col_as_list(1);
  std::vector<double> v1;
  col1.for_each([&v1](double x) { v1.push_back(x); });

  ASSERT_EQ(v1.size(), 2U);
  EXPECT_DOUBLE_EQ(v1[0], 2.0);
  EXPECT_DOUBLE_EQ(v1[1], 5.0);
}

TEST(MatrixExtraction, GetRowVector)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  Vec row1 = m.get_row_vector(1);

  EXPECT_DOUBLE_EQ(row1.get_entry(0), 4.0);
  EXPECT_DOUBLE_EQ(row1.get_entry(1), 5.0);
  EXPECT_DOUBLE_EQ(row1.get_entry(2), 6.0);
}

TEST(MatrixExtraction, GetColVector)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  Vec col2 = m.get_col_vector(2);

  EXPECT_DOUBLE_EQ(col2.get_entry(0), 3.0);
  EXPECT_DOUBLE_EQ(col2.get_entry(1), 6.0);
}

TEST(MatrixExtraction, InvalidRowThrows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);
  Mat m(rows, cols);

  EXPECT_THROW(m.get_row_as_list(5), std::domain_error);
  EXPECT_THROW(m.get_row_vector(5), std::domain_error);
}

TEST(MatrixExtraction, InvalidColThrows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);
  Mat m(rows, cols);

  EXPECT_THROW(m.get_col_as_list(5), std::domain_error);
  EXPECT_THROW(m.get_col_vector(5), std::domain_error);
}

// =============================================================================
// Matrix-Vector Multiplication
// =============================================================================

TEST(MatrixVectorMult, MatrixTimesVector)
{
  Domain d = make_domain(3);

  // | 1 2 3 |     | 1 |     | 1*1 + 2*2 + 3*3 |     | 14 |
  // | 4 5 6 |  *  | 2 |  =  | 4*1 + 5*2 + 6*3 |  =  | 32 |
  // | 7 8 9 |     | 3 |     | 7*1 + 8*2 + 9*3 |     | 50 |

  Mat m(d, d, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0}
  });

  Vec v(d);
  v.set_entry(0, 1.0);
  v.set_entry(1, 2.0);
  v.set_entry(2, 3.0);

  Vec result = m * v;

  EXPECT_DOUBLE_EQ(result.get_entry(0), 14.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 32.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2), 50.0);
}

TEST(MatrixVectorMult, VectorTimesMatrix)
{
  Domain d = make_domain(3);

  // [1 2 3] * | 1 2 3 |  = [1*1+2*4+3*7, 1*2+2*5+3*8, 1*3+2*6+3*9]
  //           | 4 5 6 |  = [30, 36, 42]
  //           | 7 8 9 |

  Mat m(d, d, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0},
    {7.0, 8.0, 9.0}
  });

  Vec v(d);
  v.set_entry(0, 1.0);
  v.set_entry(1, 2.0);
  v.set_entry(2, 3.0);

  Vec result = v * m;

  EXPECT_DOUBLE_EQ(result.get_entry(0), 30.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 36.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2), 42.0);
}

TEST(MatrixVectorMult, SparseMultiplication)
{
  Domain d = make_domain(3);

  // Sparse matrix with only diagonal
  Mat m(d, d);
  m.set_entry(0, 0, 2.0);
  m.set_entry(1, 1, 3.0);
  m.set_entry(2, 2, 4.0);

  Vec v(d);
  v.set_entry(0, 1.0);
  v.set_entry(1, 2.0);
  v.set_entry(2, 3.0);

  Vec result = m.mult_matrix_vector_sparse(v);

  EXPECT_DOUBLE_EQ(result.get_entry(0), 2.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 6.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2), 12.0);
}

TEST(MatrixVectorMult, DotProductMethod)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Vec v(d);
  v.set_entry(0, 5.0);
  v.set_entry(1, 6.0);

  Vec result = m.mult_matrix_vector_dot_product(v);

  // [1*5 + 2*6, 3*5 + 4*6] = [17, 39]
  EXPECT_DOUBLE_EQ(result.get_entry(0), 17.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 39.0);
}

TEST(MatrixVectorMult, DomainMismatchThrows)
{
  Domain d2 = make_domain(2);
  Domain d3 = make_domain(3);

  Mat m(d2, d3);  // 2x3 matrix
  Vec v(d2);      // vector of size 2, but should be 3 for multiplication

  EXPECT_THROW(m * v, std::domain_error);
}

// =============================================================================
// Matrix Addition
// =============================================================================

TEST(MatrixAddition, AddTwoMatrices)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d, {
    {5.0, 6.0},
    {7.0, 8.0}
  });

  Mat sum = m1 + m2;

  EXPECT_DOUBLE_EQ(sum.get_entry(0, 0), 6.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(0, 1), 8.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(1, 0), 10.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(1, 1), 12.0);
}

TEST(MatrixAddition, AddInPlace)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d, {
    {1.0, 1.0},
    {1.0, 1.0}
  });

  m1 += m2;

  EXPECT_DOUBLE_EQ(m1.get_entry(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(0, 1), 3.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(1, 1), 5.0);
}

TEST(MatrixAddition, AddSparseMatrices)
{
  Domain d = make_domain(3);

  Mat m1(d, d);
  m1.set_entry(0, 0, 1.0);
  m1.set_entry(2, 2, 2.0);

  Mat m2(d, d);
  m2.set_entry(1, 1, 3.0);
  m2.set_entry(2, 2, 4.0);

  Mat sum = m1 + m2;

  EXPECT_DOUBLE_EQ(sum.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(1, 1), 3.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(2, 2), 6.0);
  EXPECT_DOUBLE_EQ(sum.get_entry(0, 1), 0.0);
}

// =============================================================================
// Scalar Multiplication
// =============================================================================

TEST(MatrixScalar, MultiplyByScalar)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  m.mult_by_scalar(2.0);

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 4.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 6.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 8.0);
}

TEST(MatrixScalar, ScalarTimesMatrix)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat result = 3.0 * m;

  EXPECT_DOUBLE_EQ(result.get_entry(0, 0), 3.0);
  EXPECT_DOUBLE_EQ(result.get_entry(0, 1), 6.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1, 0), 9.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1, 1), 12.0);
}

// =============================================================================
// Identity Matrix
// =============================================================================

TEST(MatrixIdentity, CreateIdentity)
{
  Domain d = make_domain(3);
  Mat m(d, d);

  Mat id = m.identity();

  for (int i = 0; i < 3; ++i)
    for (int j = 0; j < 3; ++j)
      EXPECT_DOUBLE_EQ(id.get_entry(i, j), (i == j) ? 1.0 : 0.0);
}

TEST(MatrixIdentity, IdentityTimesVectorIsVector)
{
  Domain d = make_domain(3);
  Mat m(d, d);
  Mat id = m.identity();

  Vec v(d);
  v.set_entry(0, 5.0);
  v.set_entry(1, 10.0);
  v.set_entry(2, 15.0);

  Vec result = id * v;

  EXPECT_DOUBLE_EQ(result.get_entry(0), 5.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 10.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2), 15.0);
}

// =============================================================================
// Equality Comparison
// =============================================================================

TEST(MatrixEquality, EqualMatrices)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  EXPECT_TRUE(m1 == m2);
  EXPECT_FALSE(m1 != m2);
}

TEST(MatrixEquality, UnequalMatrices)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d, {
    {1.0, 2.0},
    {3.0, 5.0}  // different
  });

  EXPECT_FALSE(m1 == m2);
  EXPECT_TRUE(m1 != m2);
}

TEST(MatrixEquality, EqualWithinEpsilon)
{
  Domain d = make_domain(2);

  Mat m1(d, d, 1e-5);
  m1.set_entry(0, 0, 1.0);

  Mat m2(d, d, 1e-5);
  m2.set_entry(0, 0, 1.0 + 1e-6);  // within epsilon

  EXPECT_TRUE(m1 == m2);
}

// =============================================================================
// Static Factory Methods
// =============================================================================

TEST(MatrixFactory, CreateByRows)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Vec row0(cols);
  row0.set_entry(0, 1.0);
  row0.set_entry(1, 2.0);
  row0.set_entry(2, 3.0);

  Vec row1(cols);
  row1.set_entry(0, 4.0);
  row1.set_entry(1, 5.0);
  row1.set_entry(2, 6.0);

  DynList<Vec> row_list;
  row_list.append(row0);
  row_list.append(row1);

  Mat m = Mat::create_by_rows(rows, row_list);

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 2), 3.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 5.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 6.0);
}

TEST(MatrixFactory, CreateByColumns)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Vec col0(rows);
  col0.set_entry(0, 1.0);
  col0.set_entry(1, 4.0);

  Vec col1(rows);
  col1.set_entry(0, 2.0);
  col1.set_entry(1, 5.0);

  Vec col2(rows);
  col2.set_entry(0, 3.0);
  col2.set_entry(1, 6.0);

  DynList<Vec> col_list;
  col_list.append(col0);
  col_list.append(col1);
  col_list.append(col2);

  Mat m = Mat::create_by_columns(cols, col_list);

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 2), 3.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 5.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 6.0);
}

// =============================================================================
// Row/Column List Conversion
// =============================================================================

TEST(MatrixConversion, ToRowList)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  DynList<Vec> rows = m.to_rowlist();

  std::vector<Vec> row_vec;
  rows.for_each([&row_vec](const Vec & v) { row_vec.push_back(v); });

  ASSERT_EQ(row_vec.size(), 2U);
  EXPECT_DOUBLE_EQ(row_vec[0].get_entry(0), 1.0);
  EXPECT_DOUBLE_EQ(row_vec[0].get_entry(1), 2.0);
  EXPECT_DOUBLE_EQ(row_vec[1].get_entry(0), 3.0);
  EXPECT_DOUBLE_EQ(row_vec[1].get_entry(1), 4.0);
}

TEST(MatrixConversion, ToColList)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  DynList<Vec> cols = m.to_collist();

  std::vector<Vec> col_vec;
  cols.for_each([&col_vec](const Vec & v) { col_vec.push_back(v); });

  ASSERT_EQ(col_vec.size(), 2U);
  EXPECT_DOUBLE_EQ(col_vec[0].get_entry(0), 1.0);
  EXPECT_DOUBLE_EQ(col_vec[0].get_entry(1), 3.0);
  EXPECT_DOUBLE_EQ(col_vec[1].get_entry(0), 2.0);
  EXPECT_DOUBLE_EQ(col_vec[1].get_entry(1), 4.0);
}

// =============================================================================
// Set Vector as Row/Column
// =============================================================================

TEST(MatrixSetVector, SetVectorAsRow)
{
  Domain d = make_domain(3);
  Mat m(d, d);

  Vec row(d);
  row.set_entry(0, 10.0);
  row.set_entry(1, 20.0);
  row.set_entry(2, 30.0);

  m.set_vector_as_row(1, row);

  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 10.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 20.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 30.0);

  // Other rows remain zero
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(2, 0), 0.0);
}

TEST(MatrixSetVector, SetVectorAsCol)
{
  Domain d = make_domain(3);
  Mat m(d, d);

  Vec col(d);
  col.set_entry(0, 10.0);
  col.set_entry(1, 20.0);
  col.set_entry(2, 30.0);

  m.set_vector_as_col(2, col);

  EXPECT_DOUBLE_EQ(m.get_entry(0, 2), 10.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 20.0);
  EXPECT_DOUBLE_EQ(m.get_entry(2, 2), 30.0);

  // Other columns remain zero
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 0.0);
}

// =============================================================================
// Outer Product
// =============================================================================

TEST(MatrixOuterProduct, BasicOuterProduct)
{
  Domain d = make_domain(3);

  Vec v1(d);
  v1.set_entry(0, 1.0);
  v1.set_entry(1, 2.0);
  v1.set_entry(2, 3.0);

  Vec v2(d);
  v2.set_entry(0, 4.0);
  v2.set_entry(1, 5.0);
  v2.set_entry(2, 6.0);

  Mat result = outer_product<int, int, double>(v1, v2);

  // result[i,j] = v1[i] * v2[j]
  EXPECT_DOUBLE_EQ(result.get_entry(0, 0), 4.0);
  EXPECT_DOUBLE_EQ(result.get_entry(0, 1), 5.0);
  EXPECT_DOUBLE_EQ(result.get_entry(0, 2), 6.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1, 0), 8.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1, 1), 10.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1, 2), 12.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2, 0), 12.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2, 1), 15.0);
  EXPECT_DOUBLE_EQ(result.get_entry(2, 2), 18.0);
}

// =============================================================================
// Generic Domain Types (String)
// =============================================================================

TEST(MatrixGenericDomain, StringDomainMatrix)
{
  StrDomain rows;
  rows.insert("A");
  rows.insert("B");

  StrDomain cols;
  cols.insert("x");
  cols.insert("y");
  cols.insert("z");

  StrMat m(rows, cols);
  m.set_entry("A", "x", 1.0);
  m.set_entry("A", "y", 2.0);
  m.set_entry("B", "z", 3.0);

  EXPECT_DOUBLE_EQ(m.get_entry("A", "x"), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry("A", "y"), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry("A", "z"), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry("B", "x"), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry("B", "y"), 0.0);
  EXPECT_DOUBLE_EQ(m.get_entry("B", "z"), 3.0);
}

TEST(MatrixGenericDomain, StringDomainTranspose)
{
  StrDomain rows;
  rows.insert("A");
  rows.insert("B");

  StrDomain cols;
  cols.insert("x");
  cols.insert("y");

  StrMat m(rows, cols);
  m.set_entry("A", "x", 1.0);
  m.set_entry("A", "y", 2.0);
  m.set_entry("B", "x", 3.0);
  m.set_entry("B", "y", 4.0);

  StrMat mt = m.transpose();

  EXPECT_DOUBLE_EQ(mt.get_entry("x", "A"), 1.0);
  EXPECT_DOUBLE_EQ(mt.get_entry("y", "A"), 2.0);
  EXPECT_DOUBLE_EQ(mt.get_entry("x", "B"), 3.0);
  EXPECT_DOUBLE_EQ(mt.get_entry("y", "B"), 4.0);
}

// =============================================================================
// String Representation
// =============================================================================

TEST(MatrixString, ToStrDoesNotCrash)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  std::string s = m.to_str();
  EXPECT_FALSE(s.empty());
}

// =============================================================================
// Domain Accessors
// =============================================================================

TEST(MatrixDomain, GetRowAndColDomain)
{
  Domain rows = make_domain(2);
  Domain cols = make_domain(3);

  Mat m(rows, cols);

  EXPECT_EQ(&m.get_row_domain(), &rows);
  EXPECT_EQ(&m.get_col_domain(), &cols);
}

TEST(MatrixDomain, RowAndColDomainList)
{
  Domain rows = make_domain(3);
  Domain cols = make_domain(2);

  Mat m(rows, cols);

  DynList<int> row_list = m.row_domain_list();
  DynList<int> col_list = m.col_domain_list();

  std::vector<int> rv, cv;
  row_list.for_each([&rv](int x) { rv.push_back(x); });
  col_list.for_each([&cv](int x) { cv.push_back(x); });

  ASSERT_EQ(rv.size(), 3U);
  ASSERT_EQ(cv.size(), 2U);

  // Should be sorted
  EXPECT_EQ(rv[0], 0);
  EXPECT_EQ(rv[1], 1);
  EXPECT_EQ(rv[2], 2);
  EXPECT_EQ(cv[0], 0);
  EXPECT_EQ(cv[1], 1);
}

// =============================================================================
// Matrix Subtraction
// =============================================================================

TEST(MatrixSubtraction, SubtractTwoMatrices)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {5.0, 6.0},
    {7.0, 8.0}
  });

  Mat m2(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat diff = m1 - m2;

  EXPECT_DOUBLE_EQ(diff.get_entry(0, 0), 4.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(0, 1), 4.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(1, 0), 4.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(1, 1), 4.0);
}

TEST(MatrixSubtraction, SubtractInPlace)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {5.0, 6.0},
    {7.0, 8.0}
  });

  Mat m2(d, d, {
    {1.0, 1.0},
    {1.0, 1.0}
  });

  m1 -= m2;

  EXPECT_DOUBLE_EQ(m1.get_entry(0, 0), 4.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(0, 1), 5.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(1, 0), 6.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(1, 1), 7.0);
}

TEST(MatrixSubtraction, SubtractSparseMatrices)
{
  Domain d = make_domain(3);

  Mat m1(d, d);
  m1.set_entry(0, 0, 5.0);
  m1.set_entry(2, 2, 10.0);

  Mat m2(d, d);
  m2.set_entry(1, 1, 3.0);
  m2.set_entry(2, 2, 4.0);

  Mat diff = m1 - m2;

  EXPECT_DOUBLE_EQ(diff.get_entry(0, 0), 5.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(1, 1), -3.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(2, 2), 6.0);
  EXPECT_DOUBLE_EQ(diff.get_entry(0, 1), 0.0);
}

TEST(MatrixSubtraction, SubtractFromItself)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat zero = m - m;

  EXPECT_DOUBLE_EQ(zero.get_entry(0, 0), 0.0);
  EXPECT_DOUBLE_EQ(zero.get_entry(0, 1), 0.0);
  EXPECT_DOUBLE_EQ(zero.get_entry(1, 0), 0.0);
  EXPECT_DOUBLE_EQ(zero.get_entry(1, 1), 0.0);
}

TEST(MatrixSubtraction, DomainMismatchThrows)
{
  Domain d1 = make_domain(2);
  Domain d2 = make_domain(2);  // different domain object

  Mat m1(d1, d1);
  Mat m2(d2, d2);

  EXPECT_THROW(m1 - m2, std::domain_error);
  EXPECT_THROW(m1 -= m2, std::domain_error);
}

// =============================================================================
// Copy and Move Semantics
// =============================================================================

TEST(MatrixCopyMove, CopyConstructor)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(m1);  // copy construct

  // Values should be equal
  EXPECT_TRUE(m1 == m2);

  // Modifying m2 should not affect m1
  m2.set_entry(0, 0, 100.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(0, 0), 100.0);
}

TEST(MatrixCopyMove, CopyAssignment)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d);  // empty matrix
  m2 = m1;       // copy assign

  EXPECT_TRUE(m1 == m2);

  // Modifying m2 should not affect m1
  m2.set_entry(1, 1, 200.0);
  EXPECT_DOUBLE_EQ(m1.get_entry(1, 1), 4.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(1, 1), 200.0);
}

TEST(MatrixCopyMove, SelfAssignment)
{
  Domain d = make_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  m = m;  // self-assignment

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 0), 3.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 4.0);
}

TEST(MatrixCopyMove, MoveConstructor)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(std::move(m1));  // move construct

  EXPECT_DOUBLE_EQ(m2.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(1, 0), 3.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(1, 1), 4.0);
}

TEST(MatrixCopyMove, MoveAssignment)
{
  Domain d = make_domain(2);

  Mat m1(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Mat m2(d, d);
  m2 = std::move(m1);  // move assign

  EXPECT_DOUBLE_EQ(m2.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(1, 0), 3.0);
  EXPECT_DOUBLE_EQ(m2.get_entry(1, 1), 4.0);
}

TEST(MatrixCopyMove, CopyPreservesEpsilon)
{
  Domain d = make_domain(2);
  Mat m1(d, d, 1e-5);  // custom epsilon

  Mat m2(m1);
  EXPECT_DOUBLE_EQ(m2.get_epsilon(), 1e-5);

  Mat m3(d, d);
  m3 = m1;
  EXPECT_DOUBLE_EQ(m3.get_epsilon(), 1e-5);
}

// =============================================================================
// Shared Pointer Domain Tests
// =============================================================================

TEST(MatrixSharedPtr, ConstructWithSharedPtrDomains)
{
  auto rows = make_shared_domain(2);
  auto cols = make_shared_domain(3);

  Mat m(rows, cols, {
    {1.0, 2.0, 3.0},
    {4.0, 5.0, 6.0}
  });

  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 2), 6.0);

  // Verify we can get the domain pointers back
  EXPECT_EQ(m.get_row_domain_ptr(), rows);
  EXPECT_EQ(m.get_col_domain_ptr(), cols);
}

TEST(MatrixSharedPtr, DomainOutlivesLocalScope)
{
  Mat m(make_shared_domain(2), make_shared_domain(2));
  m.set_entry(0, 0, 42.0);
  m.set_entry(1, 1, 99.0);

  // Domain was created inline but matrix keeps it alive via shared_ptr
  EXPECT_DOUBLE_EQ(m.get_entry(0, 0), 42.0);
  EXPECT_DOUBLE_EQ(m.get_entry(1, 1), 99.0);
  EXPECT_EQ(m.get_row_domain().size(), 2U);
}

TEST(MatrixSharedPtr, CopiedMatrixSharesDomain)
{
  auto d = make_shared_domain(2);
  Mat m1(d, d);
  m1.set_entry(0, 0, 1.0);

  Mat m2 = m1;

  // Both matrices share the same domain pointer
  EXPECT_EQ(m1.get_row_domain_ptr(), m2.get_row_domain_ptr());
  EXPECT_EQ(m1.get_col_domain_ptr(), m2.get_col_domain_ptr());
}

TEST(MatrixSharedPtr, VectorWithSharedPtrDomain)
{
  auto d = make_shared_domain(3);

  Vec v(d);
  v.set_entry(0, 1.0);
  v.set_entry(1, 2.0);
  v.set_entry(2, 3.0);

  EXPECT_DOUBLE_EQ(v.get_entry(0), 1.0);
  EXPECT_DOUBLE_EQ(v.get_entry(1), 2.0);
  EXPECT_DOUBLE_EQ(v.get_entry(2), 3.0);
  EXPECT_EQ(v.get_domain_ptr(), d);
}

TEST(MatrixSharedPtr, MatrixVectorMultWithSharedDomains)
{
  auto d = make_shared_domain(2);

  Mat m(d, d, {
    {1.0, 2.0},
    {3.0, 4.0}
  });

  Vec v(d);
  v.set_entry(0, 1.0);
  v.set_entry(1, 2.0);

  Vec result = m * v;

  // [1*1 + 2*2, 3*1 + 4*2] = [5, 11]
  EXPECT_DOUBLE_EQ(result.get_entry(0), 5.0);
  EXPECT_DOUBLE_EQ(result.get_entry(1), 11.0);
}

// =============================================================================
// Domain Identity for Matrix Multiplication
// =============================================================================

TEST(MatrixMultDomainIdentity, SameDomainWorks)
{
  auto d = make_shared_domain(2);

  Mat A(d, d, {{1.0, 2.0}, {3.0, 4.0}});
  Mat B(d, d, {{5.0, 6.0}, {7.0, 8.0}});

  // Same domain pointer - should work
  Mat C = A.vector_matrix_mult(B);

  // A*B = [[1*5+2*7, 1*6+2*8], [3*5+4*7, 3*6+4*8]] = [[19, 22], [43, 50]]
  EXPECT_DOUBLE_EQ(C.get_entry(0, 0), 19.0);
  EXPECT_DOUBLE_EQ(C.get_entry(0, 1), 22.0);
  EXPECT_DOUBLE_EQ(C.get_entry(1, 0), 43.0);
  EXPECT_DOUBLE_EQ(C.get_entry(1, 1), 50.0);
}

TEST(MatrixMultDomainIdentity, DifferentDomainsThrows)
{
  auto d1 = make_shared_domain(2);  // {0, 1}
  auto d2 = make_shared_domain(2);  // {0, 1} - same content but different object

  Mat A(d1, d1, {{1.0, 2.0}, {3.0, 4.0}});
  Mat B(d2, d2, {{5.0, 6.0}, {7.0, 8.0}});

  // Different domain pointers - should throw even though sizes match
  EXPECT_THROW(A.vector_matrix_mult(B), std::domain_error);
  EXPECT_THROW(A.matrix_vector_mult(B), std::domain_error);
}

TEST(MatrixMultDomainIdentity, ChainedMultiplicationWithSharedDomain)
{
  auto rows = make_shared_domain(2);
  auto middle = make_shared_domain(3);
  auto cols = make_shared_domain(2);

  // A: 2x3, B: 3x2
  Mat A(rows, middle, {{1.0, 2.0, 3.0}, {4.0, 5.0, 6.0}});
  Mat B(middle, cols, {{1.0, 2.0}, {3.0, 4.0}, {5.0, 6.0}});

  // A*B should work: column domain of A == row domain of B (both are 'middle')
  Mat C = A.vector_matrix_mult(B);

  // Result is 2x2
  EXPECT_DOUBLE_EQ(C.get_entry(0, 0), 22.0);  // 1*1 + 2*3 + 3*5
  EXPECT_DOUBLE_EQ(C.get_entry(0, 1), 28.0);  // 1*2 + 2*4 + 3*6
  EXPECT_DOUBLE_EQ(C.get_entry(1, 0), 49.0);  // 4*1 + 5*3 + 6*5
  EXPECT_DOUBLE_EQ(C.get_entry(1, 1), 64.0);  // 4*2 + 5*4 + 6*6
}
