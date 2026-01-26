
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
 * @file al-matrix.cc
 * @brief Tests for Al Matrix
 */
# include <gtest/gtest.h>

# include <ahSort.H>
# include <ah-string-utils.H>
# include <al-matrix.H>

using namespace std;
using namespace testing;
using namespace Aleph;

struct SmallDomains : testing::Test
{
  AlDomain<char> rd = { 'a', 'b', 'c', 'd', 'e' };
  AlDomain<string> cd = { "A", "B", "C", "D" };
  using V1 = Vector<char, int>;
  using V2 = Vector<string, int>;
  using Mat1 = Matrix<char, string, int>;
  using Mat2 = Matrix<string, char, int>;

  V1 v1_zero = { rd };
  V1 v1 = { rd, range<int>(rd.size()) };
  V1 v1_odd = { rd, range<int>(rd.size()).
		maps([] (auto i) { return (i % 2) == 0 ? 1 : 0; }) };
  V1 v1_even = { rd, range<int>(rd.size()).
		 maps([] (auto i) { return (i % 2) == 0 ? 0 : 1; }) };

  V2 v2_zero = { cd };
  V2 v2 = { cd, range<int>(cd.size()) };
  V2 v2_odd = { cd, range<int>(cd.size()).
		maps([] (auto i) { return (i % 2) == 0 ? 1 : 0; }) };
  V2 v2_even = { cd, range<int>(cd.size()).
		 maps([] (auto i) { return (i % 2) == 0 ? 0 : 1; }) };

  Mat1 m1_zero = { rd, cd };
  Mat1 m1_one = { rd, cd, rd.maps<DynList<int>>([&] (auto)
    { return range<int>(cd.size()); }) };
  Mat1 m1_odd = Mat1::create_by_rows(rd, rep<V2>(rd.size(), v2_odd));
  Mat1 m1_even = Mat1::create_by_rows(rd, rep<V2>(rd.size(), v2_even));

  Mat2 m2_zero = { cd, rd };
  Mat2 m2_one = { cd, rd, cd.maps<DynList<int>>([&] (auto)
    { return range<int>(cd.size()); }) };
  Mat2 m2_odd = Mat2::create_by_columns(rd, rep<V2>(cd.size(), v2_odd));
  Mat2 m2_even = Mat2::create_by_columns(rd, rep<V2>(cd.size(), v2_even));
};

TEST_F(SmallDomains, basic)
{
  EXPECT_EQ(m1_one.get_row_vector('a'), v2);
  EXPECT_EQ(m1_one.get_row_vector('b'), v2);
  EXPECT_EQ(m1_one.get_row_vector('c'), v2);
  EXPECT_EQ(m1_one.get_row_vector('d'), v2);
  EXPECT_EQ(m1_one.get_row_vector('e'), v2);

  EXPECT_EQ(m1_one.get_col_vector("A"), V1(rd));
  EXPECT_EQ(m1_one.get_col_vector("B"), V1(rd, rep(rd.size(), 1)));
  EXPECT_EQ(m1_one.get_col_vector("C"), V1(rd, rep(rd.size(), 2)));
  EXPECT_EQ(m1_one.get_col_vector("D"), V1(rd, rep(rd.size(), 3)));

  EXPECT_EQ(v1*m1_zero, V2(cd, { 0, 0, 0, 0}));
  EXPECT_EQ(m1_zero*v2, V1(rd, { 0, 0, 0, 0, 0}));

  //cout << m1_even.matrix_vector_mult(m2_even) << endl << endl;

  return;
  cout << v1_zero << endl << endl
       << v1 <<  endl << endl
       << v1_odd <<  endl << endl
       << v1_even <<  endl << endl
       << endl
       << v2_zero << endl << endl
       << v2 <<  endl << endl
       << v2_odd <<  endl << endl
       << v2_even <<  endl << endl
       << m1_even<< endl << endl
       << m1_odd << endl << endl
       << m2_even<< endl << endl
       << m1_odd << endl << endl
       << endl;
}

TEST_F(SmallDomains, identity)
{
  // using M1 = Matrix<char, char, int>;
  // using M2 = Matrix<string, string, int>;
  // cout << M1(rd, rd).identity() << endl
  //      << M2(cd, cd).identity() << endl;
}
