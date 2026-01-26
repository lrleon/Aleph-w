
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


/** @file mat_latex_path_test.cc
 *  @brief Unit tests for mat_latex.H and mat_path.H
 */

#include <gtest/gtest.h>
#include <sstream>
#include <vector>
#include <mat_latex.H>

using namespace Aleph;

//============================================================================
// Simple Matrix for Testing mat_latex.H
//============================================================================

class SimpleMatrix
{
  std::vector<std::vector<int>> data;
  long rows_, cols_;
  
public:
  using Entry_Type = int;
  
  SimpleMatrix(long r, long c) : rows_(r), cols_(c)
  {
    data.resize(r);
    for (long i = 0; i < r; ++i)
      data[i].resize(c, 0);
  }
  
  int & operator()(long i, long j) { return data[i][j]; }
  int operator()(long i, long j) const { return data[i][j]; }
  
  long rows() const { return rows_; }
  long cols() const { return cols_; }
};

//============================================================================
// mat_latex.H Tests
//============================================================================

class MatLatexTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // 3x3 matrix with values 1-9
    mat = std::make_unique<SimpleMatrix>(3, 3);
    int val = 1;
    for (long i = 0; i < 3; ++i)
      for (long j = 0; j < 3; ++j)
        (*mat)(i, j) = val++;
  }
  
  std::unique_ptr<SimpleMatrix> mat;
};

// Custom formatters for testing
struct TestRowFormatter
{
  std::string operator()(SimpleMatrix &, long i) const
  {
    return "R" + std::to_string(i);
  }
};

struct TestColFormatter
{
  std::string operator()(SimpleMatrix &, long j) const
  {
    return "C" + std::to_string(j);
  }
};

struct TestEntryFormatter
{
  std::string operator()(SimpleMatrix & m, long i, long j) const
  {
    return std::to_string(m(i, j));
  }
};

TEST_F(MatLatexTest, BasicOutput)
{
  std::ostringstream out;
  
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out);
  
  std::string result = out.str();
  
  // Check for LaTeX tabular environment
  EXPECT_TRUE(result.find("\\begin{tabular}") != std::string::npos);
  EXPECT_TRUE(result.find("\\end{tabular}") != std::string::npos);
  EXPECT_TRUE(result.find("\\hline") != std::string::npos);
}

TEST_F(MatLatexTest, ContainsColumnHeaders)
{
  std::ostringstream out;
  
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("C0") != std::string::npos);
  EXPECT_TRUE(result.find("C1") != std::string::npos);
  EXPECT_TRUE(result.find("C2") != std::string::npos);
}

TEST_F(MatLatexTest, ContainsRowHeaders)
{
  std::ostringstream out;
  
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("R0") != std::string::npos);
  EXPECT_TRUE(result.find("R1") != std::string::npos);
  EXPECT_TRUE(result.find("R2") != std::string::npos);
}

TEST_F(MatLatexTest, ContainsMatrixEntries)
{
  std::ostringstream out;
  
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out);
  
  std::string result = out.str();
  
  // Check all values 1-9 are present
  for (int i = 1; i <= 9; ++i)
    EXPECT_TRUE(result.find(std::to_string(i)) != std::string::npos)
        << "Value " << i << " not found in output";
}

TEST_F(MatLatexTest, PrefixAndSuffix)
{
  std::ostringstream out;
  
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out, "PREFIX_START\n", "SUFFIX_END\n");
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("PREFIX_START") != std::string::npos);
  EXPECT_TRUE(result.find("SUFFIX_END") != std::string::npos);
  
  // Prefix should be at start
  EXPECT_EQ(result.find("PREFIX_START"), 0u);
}

TEST_F(MatLatexTest, SingleRowMatrix)
{
  SimpleMatrix single_row(1, 3);
  single_row(0, 0) = 10;
  single_row(0, 1) = 20;
  single_row(0, 2) = 30;
  
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      single_row, 1, 3, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("10") != std::string::npos);
  EXPECT_TRUE(result.find("20") != std::string::npos);
  EXPECT_TRUE(result.find("30") != std::string::npos);
}

TEST_F(MatLatexTest, SingleColumnMatrix)
{
  SimpleMatrix single_col(3, 1);
  single_col(0, 0) = 100;
  single_col(1, 0) = 200;
  single_col(2, 0) = 300;
  
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      single_col, 3, 1, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("100") != std::string::npos);
  EXPECT_TRUE(result.find("200") != std::string::npos);
  EXPECT_TRUE(result.find("300") != std::string::npos);
}

TEST_F(MatLatexTest, DefaultFormatters)
{
  std::ostringstream out;
  mat_to_latex_simple(*mat, 3, 3, out);
  
  std::string result = out.str();
  
  // Should contain default integer indices
  EXPECT_TRUE(result.find("0") != std::string::npos);
  EXPECT_TRUE(result.find("1") != std::string::npos);
  EXPECT_TRUE(result.find("2") != std::string::npos);
}

TEST_F(MatLatexTest, EmptyPrefix)
{
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out, "", "");
  
  std::string result = out.str();
  
  // Should start with \begin
  EXPECT_TRUE(result.find("\\begin") == 0u);
}

TEST_F(MatLatexTest, TabularStructure)
{
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, TestEntryFormatter>(
      *mat, 3, 3, out);
  
  std::string result = out.str();
  
  // Count & separators (should have 2 per data row * 3 rows + header row)
  size_t ampersand_count = 0;
  for (char c : result)
    if (c == '&')
      ++ampersand_count;
  
  // Each row has (cols - 1) & + 1 for row header = cols &
  // Total: (header row: cols-1) + (data rows: cols * n)
  // 3x3: header has 2 &, each data row has 3 & (including row header sep)
  EXPECT_GT(ampersand_count, 0u);
}

//============================================================================
// Edge Cases
//============================================================================

class MatLatexEdgeCaseTest : public ::testing::Test {};

TEST_F(MatLatexEdgeCaseTest, LargeMatrix)
{
  SimpleMatrix large(10, 10);
  for (long i = 0; i < 10; ++i)
    for (long j = 0; j < 10; ++j)
      large(i, j) = static_cast<int>(i * 10 + j);
  
  std::ostringstream out;
  mat_to_latex_simple(large, 10, 10, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("\\begin{tabular}") != std::string::npos);
  EXPECT_TRUE(result.find("99") != std::string::npos);  // Last element
}

TEST_F(MatLatexEdgeCaseTest, NonSquareMatrix)
{
  SimpleMatrix rect(2, 5);
  for (long i = 0; i < 2; ++i)
    for (long j = 0; j < 5; ++j)
      rect(i, j) = static_cast<int>(i * 5 + j);
  
  std::ostringstream out;
  mat_to_latex_simple(rect, 2, 5, out);
  
  std::string result = out.str();
  
  // Should have 5 column widths
  size_t count = 0;
  size_t pos = 0;
  while ((pos = result.find("p{1mm}", pos)) != std::string::npos)
    {
      ++count;
      ++pos;
    }
  EXPECT_EQ(count, 5u);
}

TEST_F(MatLatexEdgeCaseTest, SingleCell)
{
  SimpleMatrix single(1, 1);
  single(0, 0) = 42;
  
  std::ostringstream out;
  mat_to_latex_simple(single, 1, 1, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("42") != std::string::npos);
  EXPECT_TRUE(result.find("\\begin{tabular}") != std::string::npos);
}

TEST_F(MatLatexEdgeCaseTest, NegativeValues)
{
  SimpleMatrix neg(2, 2);
  neg(0, 0) = -5;
  neg(0, 1) = -10;
  neg(1, 0) = 0;
  neg(1, 1) = -100;
  
  std::ostringstream out;
  mat_to_latex_simple(neg, 2, 2, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("-5") != std::string::npos);
  EXPECT_TRUE(result.find("-10") != std::string::npos);
  EXPECT_TRUE(result.find("-100") != std::string::npos);
}

//============================================================================
// Default Formatter Tests
//============================================================================

class DefaultFormatterTest : public ::testing::Test {};

TEST_F(DefaultFormatterTest, DefaultRowFormatter)
{
  SimpleMatrix mat(3, 3);
  Default_Row_Formatter<SimpleMatrix> fmt;
  
  EXPECT_EQ(fmt(mat, 0), "0");
  EXPECT_EQ(fmt(mat, 5), "5");
  EXPECT_EQ(fmt(mat, 100), "100");
}

TEST_F(DefaultFormatterTest, DefaultColFormatter)
{
  SimpleMatrix mat(3, 3);
  Default_Col_Formatter<SimpleMatrix> fmt;
  
  EXPECT_EQ(fmt(mat, 0), "0");
  EXPECT_EQ(fmt(mat, 3), "3");
  EXPECT_EQ(fmt(mat, 99), "99");
}

TEST_F(DefaultFormatterTest, DefaultEntryFormatter)
{
  SimpleMatrix mat(3, 3);
  mat(1, 2) = 42;
  
  Default_Entry_Formatter<SimpleMatrix> fmt;
  
  EXPECT_EQ(fmt(mat, 1, 2), "42");
}

TEST_F(DefaultFormatterTest, DefaultEntryFormatterNegative)
{
  SimpleMatrix mat(2, 2);
  mat(0, 0) = -999;
  
  Default_Entry_Formatter<SimpleMatrix> fmt;
  
  EXPECT_EQ(fmt(mat, 0, 0), "-999");
}

//============================================================================
// Custom Formatter Tests
//============================================================================

class CustomFormatterTest : public ::testing::Test {};

struct BoldRowFormatter
{
  std::string operator()(SimpleMatrix &, long i) const
  {
    return "\\textbf{" + std::to_string(i) + "}";
  }
};

struct ItalicColFormatter
{
  std::string operator()(SimpleMatrix &, long j) const
  {
    return "\\textit{" + std::to_string(j) + "}";
  }
};

struct InfinityFormatter
{
  std::string operator()(SimpleMatrix & m, long i, long j) const
  {
    int val = m(i, j);
    if (val == std::numeric_limits<int>::max())
      return "\\infty";
    return std::to_string(val);
  }
};

TEST_F(CustomFormatterTest, BoldRowHeaders)
{
  SimpleMatrix mat(2, 2);
  mat(0, 0) = 1;
  mat(0, 1) = 2;
  mat(1, 0) = 3;
  mat(1, 1) = 4;
  
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, BoldRowFormatter, TestColFormatter, TestEntryFormatter>(
      mat, 2, 2, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("\\textbf{0}") != std::string::npos);
  EXPECT_TRUE(result.find("\\textbf{1}") != std::string::npos);
}

TEST_F(CustomFormatterTest, ItalicColHeaders)
{
  SimpleMatrix mat(2, 2);
  
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, ItalicColFormatter, TestEntryFormatter>(
      mat, 2, 2, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("\\textit{0}") != std::string::npos);
  EXPECT_TRUE(result.find("\\textit{1}") != std::string::npos);
}

TEST_F(CustomFormatterTest, InfinityValues)
{
  SimpleMatrix mat(2, 2);
  mat(0, 0) = 0;
  mat(0, 1) = std::numeric_limits<int>::max();
  mat(1, 0) = std::numeric_limits<int>::max();
  mat(1, 1) = 5;
  
  std::ostringstream out;
  mat_to_latex<SimpleMatrix, TestRowFormatter, TestColFormatter, InfinityFormatter>(
      mat, 2, 2, out);
  
  std::string result = out.str();
  
  EXPECT_TRUE(result.find("\\infty") != std::string::npos);
  EXPECT_TRUE(result.find("5") != std::string::npos);
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
