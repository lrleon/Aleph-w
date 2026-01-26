
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
 * @file dynmat_test.cc
 * @brief Comprehensive test suite for DynMatrix<T>
 *
 * This test file provides exhaustive coverage of DynMatrix functionality:
 * - Basic operations (read, write, access)
 * - Constructors (default, parametrized, copy, move)
 * - Assignment operators (copy, move)
 * - Dimension queries (rows, cols, size, is_square, is_empty)
 * - Iterator functionality
 * - Functional methods (traverse, maps, filter, foldl, etc.)
 * - Sparse storage behavior
 * - Edge cases and boundary conditions
 */

#include <gtest/gtest.h>

#include <string>
#include <memory>
#include <vector>
#include <stdexcept>
#include <cmath>

#include <tpl_dynMat.H>
#include <ahFunctional.H>

using namespace std;
using namespace testing;
using namespace Aleph;

// =============================================================================
// Test Fixtures
// =============================================================================

constexpr size_t SMALL_N = 3;
constexpr size_t SMALL_M = 4;
constexpr size_t MEDIUM_N = 10;
constexpr size_t MEDIUM_M = 15;
constexpr size_t LARGE_N = 100;

/// Fixture with a small integer matrix
struct SmallIntMatrix : public Test
{
  DynMatrix<int> mat;

  SmallIntMatrix() : mat(SMALL_N, SMALL_M, 0) {}
};

/// Fixture with a pre-filled matrix
struct FilledMatrix : public Test
{
  DynMatrix<int> mat;

  FilledMatrix() : mat(SMALL_N, SMALL_M, 0)
  {
    for (size_t i = 0; i < SMALL_N; ++i)
      for (size_t j = 0; j < SMALL_M; ++j)
        mat.write(i, j, static_cast<int>(i * SMALL_M + j));
  }
};

/// Fixture with a square matrix
struct SquareMatrix : public Test
{
  DynMatrix<double> mat;

  SquareMatrix() : mat(4, 4, 0.0)
  {
    // Identity-like diagonal
    for (size_t i = 0; i < 4; ++i)
      mat.write(i, i, 1.0);
  }
};

/// Type that counts constructions/destructions
struct Counted
{
  static int constructions;
  static int destructions;
  static int copies;
  static int moves;

  int value;

  static void reset()
  {
    constructions = destructions = copies = moves = 0;
  }

  explicit Counted(int v = 0) : value(v) { ++constructions; }
  Counted(const Counted& other) : value(other.value)
  {
    ++constructions;
    ++copies;
  }
  Counted(Counted&& other) noexcept : value(other.value)
  {
    ++constructions;
    ++moves;
  }
  ~Counted() { ++destructions; }

  Counted& operator=(const Counted& other)
  {
    value = other.value;
    ++copies;
    return *this;
  }
  Counted& operator=(Counted&& other) noexcept
  {
    value = other.value;
    ++moves;
    return *this;
  }

  bool operator==(const Counted& other) const { return value == other.value; }
  bool operator!=(const Counted& other) const { return value != other.value; }
};

int Counted::constructions = 0;
int Counted::destructions = 0;
int Counted::copies = 0;
int Counted::moves = 0;

// =============================================================================
// Constructor Tests
// =============================================================================

TEST(DynMatrix, default_constructor_creates_empty_matrix)
{
  DynMatrix<int> mat;
  EXPECT_TRUE(mat.is_empty());
  EXPECT_EQ(mat.rows(), 0);
  EXPECT_EQ(mat.cols(), 0);
  EXPECT_EQ(mat.size(), 0);
}

TEST(DynMatrix, parametrized_constructor_creates_correct_dimensions)
{
  DynMatrix<int> mat(5, 7);

  EXPECT_EQ(mat.rows(), 5);
  EXPECT_EQ(mat.cols(), 7);
  EXPECT_EQ(mat.size(), 35);
  EXPECT_FALSE(mat.is_empty());
  EXPECT_FALSE(mat.is_square());
}

TEST(DynMatrix, parametrized_constructor_with_custom_default_value)
{
  DynMatrix<int> mat(3, 3, 42);

  EXPECT_EQ(mat.get_default_value(), 42);
  EXPECT_EQ(mat.read(0, 0), 42);
  EXPECT_EQ(mat.read(1, 2), 42);
}

TEST(DynMatrix, constructor_throws_on_zero_dimensions)
{
  EXPECT_THROW(DynMatrix<int>(0, 5), std::invalid_argument);
  EXPECT_THROW(DynMatrix<int>(5, 0), std::invalid_argument);
  EXPECT_THROW(DynMatrix<int>(0, 0), std::invalid_argument);
}

TEST(DynMatrix, constructor_with_various_types)
{
  DynMatrix<int> mi(2, 3);
  DynMatrix<double> md(3, 4);
  DynMatrix<string> ms(2, 2, "default");
  DynMatrix<vector<int>> mv(2, 2);

  EXPECT_EQ(mi.rows(), 2);
  EXPECT_EQ(md.cols(), 4);
  EXPECT_EQ(ms.read(0, 0), "default");
  EXPECT_TRUE(mv.read(0, 0).empty());
}

// =============================================================================
// Copy Constructor Tests
// =============================================================================

TEST_F(FilledMatrix, copy_constructor_creates_independent_copy)
{
  DynMatrix<int> copy(mat);

  EXPECT_EQ(copy.rows(), mat.rows());
  EXPECT_EQ(copy.cols(), mat.cols());

  for (size_t i = 0; i < SMALL_N; ++i)
    for (size_t j = 0; j < SMALL_M; ++j)
      EXPECT_EQ(copy.read(i, j), mat.read(i, j));

  // Modify original, copy should be unchanged
  mat.write(0, 0, 999);
  EXPECT_NE(copy.read(0, 0), mat.read(0, 0));
}

TEST(DynMatrix, copy_constructor_empty_matrix)
{
  DynMatrix<int> empty;
  DynMatrix<int> copy(empty);

  EXPECT_TRUE(copy.is_empty());
}

TEST_F(SmallIntMatrix, copy_constructor_sparse_matrix)
{
  // Write only a few entries
  mat.write(0, 0, 1);
  mat.write(2, 3, 2);

  DynMatrix<int> copy(mat);

  EXPECT_EQ(copy.read(0, 0), 1);
  EXPECT_EQ(copy.read(2, 3), 2);
  EXPECT_EQ(copy.read(1, 1), 0);  // Default value
}

// =============================================================================
// Move Constructor Tests
// =============================================================================

TEST_F(FilledMatrix, move_constructor_transfers_ownership)
{
  size_t original_rows = mat.rows();
  size_t original_cols = mat.cols();
  int original_value = mat.read(1, 2);

  DynMatrix<int> moved(std::move(mat));

  EXPECT_EQ(moved.rows(), original_rows);
  EXPECT_EQ(moved.cols(), original_cols);
  EXPECT_EQ(moved.read(1, 2), original_value);
  EXPECT_TRUE(mat.is_empty());
}

TEST(DynMatrix, move_constructor_empty_matrix)
{
  DynMatrix<int> empty;
  DynMatrix<int> moved(std::move(empty));

  EXPECT_TRUE(moved.is_empty());
}

TEST(DynMatrix, move_constructor_is_noexcept)
{
  EXPECT_TRUE(std::is_nothrow_move_constructible_v<DynMatrix<int>>);
}

// =============================================================================
// Copy Assignment Tests
// =============================================================================

TEST_F(FilledMatrix, copy_assignment_replaces_contents)
{
  DynMatrix<int> other(2, 2);
  other.write(0, 0, 100);

  other = mat;

  EXPECT_EQ(other.rows(), SMALL_N);
  EXPECT_EQ(other.cols(), SMALL_M);
  EXPECT_EQ(other.read(1, 2), mat.read(1, 2));
}

TEST_F(FilledMatrix, copy_assignment_self_assignment_is_safe)
{
  mat = mat;

  EXPECT_EQ(mat.rows(), SMALL_N);
  EXPECT_EQ(mat.read(0, 0), 0);
}

TEST(DynMatrix, copy_assignment_different_dimensions)
{
  DynMatrix<int> source(5, 5, 7);
  DynMatrix<int> dest(2, 3, 0);

  dest = source;

  EXPECT_EQ(dest.rows(), 5);
  EXPECT_EQ(dest.cols(), 5);
  EXPECT_EQ(dest.read(2, 2), 7);
}

// =============================================================================
// Move Assignment Tests
// =============================================================================

TEST_F(FilledMatrix, move_assignment_transfers_ownership)
{
  DynMatrix<int> dest(2, 2);
  size_t original_rows = mat.rows();

  dest = std::move(mat);

  EXPECT_EQ(dest.rows(), original_rows);
  EXPECT_TRUE(mat.is_empty());
}

TEST(DynMatrix, move_assignment_is_noexcept)
{
  EXPECT_TRUE(std::is_nothrow_move_assignable_v<DynMatrix<int>>);
}

// =============================================================================
// Swap Tests
// =============================================================================

TEST(DynMatrix, swap_exchanges_contents)
{
  DynMatrix<int> mat1(3, 4, 1);
  DynMatrix<int> mat2(5, 6, 2);

  mat1.write(0, 0, 10);
  mat2.write(0, 0, 20);

  mat1.swap(mat2);

  EXPECT_EQ(mat1.rows(), 5);
  EXPECT_EQ(mat1.cols(), 6);
  EXPECT_EQ(mat1.read(0, 0), 20);

  EXPECT_EQ(mat2.rows(), 3);
  EXPECT_EQ(mat2.cols(), 4);
  EXPECT_EQ(mat2.read(0, 0), 10);
}

TEST(DynMatrix, swap_with_empty_matrix)
{
  DynMatrix<int> mat1(3, 3, 5);
  DynMatrix<int> mat2;

  mat1.swap(mat2);

  EXPECT_TRUE(mat1.is_empty());
  EXPECT_EQ(mat2.rows(), 3);
}

TEST(DynMatrix, swap_is_noexcept)
{
  DynMatrix<int> m1, m2;
  EXPECT_TRUE(noexcept(m1.swap(m2)));
}

// =============================================================================
// Dimension and Query Tests
// =============================================================================

TEST_F(SmallIntMatrix, rows_returns_correct_value)
{
  EXPECT_EQ(mat.rows(), SMALL_N);
}

TEST_F(SmallIntMatrix, cols_returns_correct_value)
{
  EXPECT_EQ(mat.cols(), SMALL_M);
}

TEST_F(SmallIntMatrix, size_returns_product_of_dimensions)
{
  EXPECT_EQ(mat.size(), SMALL_N * SMALL_M);
}

TEST(DynMatrix, is_square_true_for_square_matrix)
{
  DynMatrix<int> mat(5, 5);
  EXPECT_TRUE(mat.is_square());
}

TEST(DynMatrix, is_square_false_for_non_square_matrix)
{
  DynMatrix<int> mat(3, 5);
  EXPECT_FALSE(mat.is_square());
}

TEST(DynMatrix, is_empty_true_for_default_constructed)
{
  DynMatrix<int> mat;
  EXPECT_TRUE(mat.is_empty());
}

TEST_F(SmallIntMatrix, is_empty_false_for_initialized_matrix)
{
  EXPECT_FALSE(mat.is_empty());
}

// =============================================================================
// Read/Write Tests
// =============================================================================

TEST_F(SmallIntMatrix, write_and_read_single_entry)
{
  mat.write(1, 2, 42);

  EXPECT_EQ(mat.read(1, 2), 42);
}

TEST_F(SmallIntMatrix, read_unwritten_entry_returns_default)
{
  EXPECT_EQ(mat.read(0, 0), 0);
  EXPECT_EQ(mat.read(2, 3), 0);
}

TEST_F(SmallIntMatrix, write_returns_reference_to_entry)
{
  int& ref = mat.write(1, 1, 10);

  EXPECT_EQ(ref, 10);
  ref = 20;
  EXPECT_EQ(mat.read(1, 1), 20);
}

TEST_F(SmallIntMatrix, read_throws_on_out_of_bounds)
{
  EXPECT_THROW(mat.read(SMALL_N, 0), std::out_of_range);
  EXPECT_THROW(mat.read(0, SMALL_M), std::out_of_range);
  EXPECT_THROW(mat.read(100, 100), std::out_of_range);
}

TEST_F(SmallIntMatrix, write_throws_on_out_of_bounds)
{
  EXPECT_THROW(mat.write(SMALL_N, 0, 1), std::out_of_range);
  EXPECT_THROW(mat.write(0, SMALL_M, 1), std::out_of_range);
}

TEST_F(SmallIntMatrix, read_ne_no_bounds_check)
{
  mat.write(1, 2, 42);
  EXPECT_EQ(mat.read_ne(1, 2), 42);
  EXPECT_EQ(mat.read_ne(0, 0), 0);
}

TEST(DynMatrix, write_with_move_semantics)
{
  DynMatrix<string> mat(2, 2);
  string value = "hello";

  mat.write(0, 0, std::move(value));

  EXPECT_EQ(mat.read(0, 0), "hello");
  EXPECT_TRUE(value.empty());  // moved-from
}

// =============================================================================
// Access and operator() Tests
// =============================================================================

TEST_F(SmallIntMatrix, access_after_allocate)
{
  mat.allocate();
  mat(1, 2) = 99;

  EXPECT_EQ(mat.access(1, 2), 99);
  EXPECT_EQ(mat(1, 2), 99);
}

TEST_F(SmallIntMatrix, access_const_version)
{
  mat.allocate();
  mat(0, 0) = 42;

  const DynMatrix<int>& const_mat = mat;
  EXPECT_EQ(const_mat(0, 0), 42);
}

TEST_F(FilledMatrix, operator_parens_read_write)
{
  mat.allocate();

  mat(0, 0) = 100;
  EXPECT_EQ(mat(0, 0), 100);

  const DynMatrix<int>& cmat = mat;
  EXPECT_EQ(cmat(0, 0), 100);
}

// =============================================================================
// Fill Tests
// =============================================================================

TEST_F(SmallIntMatrix, fill_sets_all_entries)
{
  mat.fill(42);

  for (size_t i = 0; i < SMALL_N; ++i)
    for (size_t j = 0; j < SMALL_M; ++j)
      EXPECT_EQ(mat.read(i, j), 42);
}

TEST(DynMatrix, fill_overwrites_existing_values)
{
  DynMatrix<int> mat(3, 3, 0);
  mat.write(1, 1, 100);

  mat.fill(5);

  EXPECT_EQ(mat.read(1, 1), 5);
  EXPECT_EQ(mat.read(0, 0), 5);
}

// =============================================================================
// Transpose Tests
// =============================================================================

TEST_F(FilledMatrix, transpose_swaps_dimensions)
{
  auto transposed = mat.transpose();

  EXPECT_EQ(transposed.rows(), SMALL_M);
  EXPECT_EQ(transposed.cols(), SMALL_N);
}

TEST_F(FilledMatrix, transpose_swaps_values)
{
  auto transposed = mat.transpose();

  for (size_t i = 0; i < SMALL_N; ++i)
    for (size_t j = 0; j < SMALL_M; ++j)
      EXPECT_EQ(transposed.read(j, i), mat.read(i, j));
}

TEST_F(SquareMatrix, transpose_diagonal_unchanged)
{
  auto transposed = mat.transpose();

  for (size_t i = 0; i < 4; ++i)
    EXPECT_EQ(transposed.read(i, i), mat.read(i, i));
}

TEST(DynMatrix, transpose_sparse_matrix)
{
  DynMatrix<int> mat(3, 5, 0);
  mat.write(0, 4, 1);
  mat.write(2, 1, 2);

  auto transposed = mat.transpose();

  EXPECT_EQ(transposed.read(4, 0), 1);
  EXPECT_EQ(transposed.read(1, 2), 2);
  EXPECT_EQ(transposed.read(0, 0), 0);
}

// =============================================================================
// set_dimension Tests
// =============================================================================

TEST_F(FilledMatrix, set_dimension_clears_data)
{
  mat.set_dimension(5, 5);

  EXPECT_EQ(mat.rows(), 5);
  EXPECT_EQ(mat.cols(), 5);
  // Data should be cleared
  EXPECT_EQ(mat.read(0, 0), 0);
}

TEST_F(SmallIntMatrix, set_dimension_preserves_default_value)
{
  mat.set_default_initial_value(99);
  mat.set_dimension(2, 2);

  EXPECT_EQ(mat.read(0, 0), 99);
}

// =============================================================================
// Default Value Tests
// =============================================================================

TEST_F(SmallIntMatrix, set_default_initial_value)
{
  mat.set_default_initial_value(42);

  EXPECT_EQ(mat.get_default_value(), 42);
}

TEST(DynMatrix, default_value_affects_unwritten_reads)
{
  DynMatrix<int> mat(3, 3, 100);

  EXPECT_EQ(mat.read(0, 0), 100);
  EXPECT_EQ(mat.read(2, 2), 100);
}

TEST(DynMatrix, changing_default_affects_future_reads)
{
  DynMatrix<int> mat(3, 3, 0);

  EXPECT_EQ(mat.read(1, 1), 0);

  mat.set_default_initial_value(99);

  // Unwritten entries should now return new default
  EXPECT_EQ(mat.read(2, 2), 99);
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST(DynMatrix, equality_same_matrices)
{
  DynMatrix<int> mat1(3, 3, 5);
  DynMatrix<int> mat2(3, 3, 5);

  EXPECT_TRUE(mat1 == mat2);
  EXPECT_FALSE(mat1 != mat2);
}

TEST(DynMatrix, equality_different_dimensions)
{
  DynMatrix<int> mat1(3, 3);
  DynMatrix<int> mat2(3, 4);

  EXPECT_FALSE(mat1 == mat2);
  EXPECT_TRUE(mat1 != mat2);
}

TEST(DynMatrix, equality_different_values)
{
  DynMatrix<int> mat1(3, 3, 0);
  DynMatrix<int> mat2(3, 3, 0);

  mat1.write(1, 1, 5);

  EXPECT_FALSE(mat1 == mat2);
  EXPECT_TRUE(mat1 != mat2);
}

TEST_F(FilledMatrix, equality_with_copy)
{
  DynMatrix<int> copy(mat);

  EXPECT_TRUE(mat == copy);
}

// =============================================================================
// Iterator Tests
// =============================================================================

TEST_F(SmallIntMatrix, iterator_basic_traversal)
{
  mat.fill(1);

  auto it = mat.get_it();
  size_t count = 0;

  while (it.has_curr())
    {
      EXPECT_EQ(it.get_curr(), 1);
      it.next();
      ++count;
    }

  EXPECT_EQ(count, mat.size());
}

TEST_F(FilledMatrix, iterator_visits_in_row_major_order)
{
  auto it = mat.get_it();

  for (size_t i = 0; i < SMALL_N; ++i)
    for (size_t j = 0; j < SMALL_M; ++j)
      {
        ASSERT_TRUE(it.has_curr());
        EXPECT_EQ(it.get_row(), i);
        EXPECT_EQ(it.get_col(), j);
        EXPECT_EQ(it.get_curr(), mat.read(i, j));
        it.next();
      }

  EXPECT_FALSE(it.has_curr());
}

TEST_F(SmallIntMatrix, iterator_get_curr_throws_when_exhausted)
{
  auto it = mat.get_it();

  // Exhaust the iterator
  while (it.has_curr())
    it.next();

  EXPECT_THROW(it.get_curr(), std::overflow_error);
}

TEST_F(SmallIntMatrix, iterator_next_throws_when_exhausted)
{
  auto it = mat.get_it();

  while (it.has_curr())
    it.next();

  EXPECT_THROW(it.next(), std::overflow_error);
}

TEST_F(SmallIntMatrix, iterator_reset)
{
  auto it = mat.get_it();

  it.next();
  it.next();
  it.reset();

  EXPECT_EQ(it.get_row(), 0);
  EXPECT_EQ(it.get_col(), 0);
}

// =============================================================================
// Traverse Tests
// =============================================================================

TEST_F(FilledMatrix, traverse_visits_all_elements)
{
  size_t count = 0;
  int sum = 0;

  mat.traverse([&](int val)
  {
    ++count;
    sum += val;
    return true;
  });

  EXPECT_EQ(count, mat.size());

  // Sum of 0 to N*M-1
  int expected = (SMALL_N * SMALL_M - 1) * SMALL_N * SMALL_M / 2;
  EXPECT_EQ(sum, expected);
}

TEST_F(FilledMatrix, traverse_can_stop_early)
{
  size_t count = 0;

  bool result = mat.traverse([&](int)
  {
    ++count;
    return count < 5;
  });

  EXPECT_FALSE(result);
  EXPECT_EQ(count, 5);
}

TEST_F(SmallIntMatrix, traverse_on_sparse_matrix)
{
  mat.write(0, 0, 10);
  mat.write(2, 3, 20);

  int sum = 0;
  mat.traverse([&](int val)
  {
    sum += val;
    return true;
  });

  EXPECT_EQ(sum, 30);  // 10 + 20, rest are 0
}

TEST_F(SmallIntMatrix, traverse_allocated_visits_allocated_blocks)
{
  // Write two entries - this allocates the blocks containing them
  mat.write(0, 0, 10);
  mat.write(1, 1, 20);

  size_t count = 0;
  int sum = 0;

  mat.traverse_allocated([&](int val)
  {
    ++count;
    sum += val;
    return true;
  });

  // Note: traverse_allocated visits all entries in allocated blocks,
  // not just explicitly written entries. The exact count depends on
  // the DynArray block size configuration.
  EXPECT_GE(count, 2u);  // At least the 2 written entries
  EXPECT_GE(sum, 30);    // At least 10 + 20 from written entries
}

// =============================================================================
// Functional Methods Tests (inherited from mixins)
// =============================================================================

TEST_F(FilledMatrix, for_each_visits_all)
{
  int sum = 0;
  mat.for_each([&sum](int val) { sum += val; });

  int expected = (SMALL_N * SMALL_M - 1) * SMALL_N * SMALL_M / 2;
  EXPECT_EQ(sum, expected);
}

TEST_F(FilledMatrix, all_returns_true_when_all_match)
{
  // All values are >= 0
  bool result = mat.all([](int val) { return val >= 0; });
  EXPECT_TRUE(result);
}

TEST_F(FilledMatrix, all_returns_false_when_any_fails)
{
  // Not all values are < 5
  bool result = mat.all([](int val) { return val < 5; });
  EXPECT_FALSE(result);
}

TEST_F(FilledMatrix, exists_returns_true_when_found)
{
  bool result = mat.exists([](int val) { return val == 5; });
  EXPECT_TRUE(result);
}

TEST_F(FilledMatrix, exists_returns_false_when_not_found)
{
  bool result = mat.exists([](int val) { return val == 999; });
  EXPECT_FALSE(result);
}

TEST_F(FilledMatrix, foldl_accumulates)
{
  int sum = mat.foldl(0, [](int acc, int val) { return acc + val; });

  int expected = (SMALL_N * SMALL_M - 1) * SMALL_N * SMALL_M / 2;
  EXPECT_EQ(sum, expected);
}

// =============================================================================
// Type Aliases Tests
// =============================================================================

TEST(DynMatrix, type_aliases_are_correct)
{
  using Matrix = DynMatrix<int>;

  static_assert(std::is_same_v<Matrix::Item_Type, int>);
  static_assert(std::is_same_v<Matrix::Key_Type, int>);
  static_assert(std::is_same_v<Matrix::Set_Type, Matrix>);
}

// =============================================================================
// Memory Management Tests
// =============================================================================

TEST(DynMatrix, destructor_frees_all_memory)
{
  Counted::reset();

  {
    DynMatrix<Counted> mat(5, 5, Counted(0));
    mat.allocate();
    for (size_t i = 0; i < 5; ++i)
      for (size_t j = 0; j < 5; ++j)
        mat.write(i, j, Counted(static_cast<int>(i * 5 + j)));
  }

  // All elements should be destroyed
  EXPECT_EQ(Counted::constructions, Counted::destructions);
}

TEST(DynMatrix, allocate_reserves_all_entries)
{
  DynMatrix<int> mat(10, 10);
  mat.allocate();

  // Should be able to access any entry without write
  for (size_t i = 0; i < 10; ++i)
    for (size_t j = 0; j < 10; ++j)
      mat(i, j) = static_cast<int>(i * 10 + j);

  EXPECT_EQ(mat(5, 5), 55);
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST(DynMatrix, single_element_matrix)
{
  DynMatrix<int> mat(1, 1, 42);

  EXPECT_EQ(mat.rows(), 1);
  EXPECT_EQ(mat.cols(), 1);
  EXPECT_EQ(mat.size(), 1);
  EXPECT_TRUE(mat.is_square());
  EXPECT_EQ(mat.read(0, 0), 42);
}

TEST(DynMatrix, single_row_matrix)
{
  DynMatrix<int> mat(1, 10, 0);

  EXPECT_EQ(mat.rows(), 1);
  EXPECT_EQ(mat.cols(), 10);
  EXPECT_FALSE(mat.is_square());

  for (size_t j = 0; j < 10; ++j)
    mat.write(0, j, static_cast<int>(j));

  EXPECT_EQ(mat.read(0, 5), 5);
}

TEST(DynMatrix, single_column_matrix)
{
  DynMatrix<int> mat(10, 1, 0);

  EXPECT_EQ(mat.rows(), 10);
  EXPECT_EQ(mat.cols(), 1);

  for (size_t i = 0; i < 10; ++i)
    mat.write(i, 0, static_cast<int>(i));

  EXPECT_EQ(mat.read(5, 0), 5);
}

TEST(DynMatrix, very_sparse_matrix)
{
  DynMatrix<int> mat(1000, 1000, 0);

  // Write only a few entries
  mat.write(0, 0, 1);
  mat.write(999, 999, 2);
  mat.write(500, 500, 3);

  EXPECT_EQ(mat.read(0, 0), 1);
  EXPECT_EQ(mat.read(999, 999), 2);
  EXPECT_EQ(mat.read(500, 500), 3);
  EXPECT_EQ(mat.read(100, 100), 0);  // Default
}

// =============================================================================
// Stress Tests
// =============================================================================

TEST(DynMatrix, stress_large_matrix)
{
  DynMatrix<int> mat(LARGE_N, LARGE_N, 0);

  // Write diagonal
  for (size_t i = 0; i < LARGE_N; ++i)
    mat.write(i, i, static_cast<int>(i));

  // Verify
  for (size_t i = 0; i < LARGE_N; ++i)
    {
      ASSERT_EQ(mat.read(i, i), static_cast<int>(i));
      if (i + 1 < LARGE_N)
        ASSERT_EQ(mat.read(i, i + 1), 0);
    }
}

TEST(DynMatrix, stress_copy_large_matrix)
{
  DynMatrix<int> original(LARGE_N, LARGE_N, 0);

  for (size_t i = 0; i < LARGE_N; ++i)
    original.write(i, i, static_cast<int>(i));

  DynMatrix<int> copy(original);

  EXPECT_EQ(copy.rows(), LARGE_N);

  for (size_t i = 0; i < LARGE_N; ++i)
    ASSERT_EQ(copy.read(i, i), static_cast<int>(i));
}

// =============================================================================
// Special Value Tests
// =============================================================================

TEST(DynMatrix, floating_point_matrix)
{
  DynMatrix<double> mat(3, 3, 0.0);

  mat.write(0, 0, 1.5);
  mat.write(1, 1, 2.7);
  mat.write(2, 2, 3.9);

  EXPECT_DOUBLE_EQ(mat.read(0, 0), 1.5);
  EXPECT_DOUBLE_EQ(mat.read(1, 1), 2.7);
  EXPECT_DOUBLE_EQ(mat.read(2, 2), 3.9);
}

TEST(DynMatrix, string_matrix)
{
  DynMatrix<string> mat(2, 2, "empty");

  mat.write(0, 0, "hello");
  mat.write(1, 1, "world");

  EXPECT_EQ(mat.read(0, 0), "hello");
  EXPECT_EQ(mat.read(0, 1), "empty");
  EXPECT_EQ(mat.read(1, 0), "empty");
  EXPECT_EQ(mat.read(1, 1), "world");
}

// =============================================================================
// Regression Tests
// =============================================================================

TEST(DynMatrix, regression_equality_different_shaped_same_size)
{
  // 2x6 vs 3x4 - same total size, different shape
  DynMatrix<int> mat1(2, 6, 0);
  DynMatrix<int> mat2(3, 4, 0);

  EXPECT_FALSE(mat1 == mat2);  // Should not be equal
}

TEST(DynMatrix, regression_move_assignment_returns_reference)
{
  DynMatrix<int> mat1(2, 2);
  DynMatrix<int> mat2(3, 3);

  DynMatrix<int>& ref = (mat1 = std::move(mat2));

  EXPECT_EQ(&ref, &mat1);
}

// =============================================================================
// Main
// =============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}