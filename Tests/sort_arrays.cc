
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
 * @file sort_arrays.cc
 * @brief Tests for Sort Arrays
 */

//
// Created by lrleon on 1/6/25.
//

# include <gsl/gsl_rng.h>

# include <chrono>
# include <vector>
# include <sstream>
# include <array>
# include <algorithm>
# include <functional>
# include <iostream>
# include <ranges>
# include <numeric>
# include <stdexcept>
# include <gtest/gtest.h>

# include <ah-string-utils.H>
# include <ah-stl-utils.H>
# include <ahSort.H>

using namespace Aleph;
using namespace std;
using namespace std::chrono;
using namespace testing;

struct SimpleArray : public Test
{
  vector <int> a = {3, 2, 1, 0, 4, 5, 6, 7, 8, 9};
  Array <int> b = {0, 1, 2, 3, 4, 5, 6, 7, 8, 9};
  Array <string> c = {"a", "b", "c", "d", "e", "f", "g", "h", "i", "j"};
};

struct TenVectors : public Test
{
  static vector <int> rand_vec(int n, const gsl_rng *rng)
  {
    vector <int> v(n);
    for (int i = 0; i < n; i++)
      v[i] = gsl_rng_uniform_int(rng, n);
    return v;
  }

  gsl_rng *rng = gsl_rng_alloc(gsl_rng_mt19937);

  vector <int> a = rand_vec(1000000, rng);
  vector <int> b = rand_vec(1000000, rng);
  vector <int> c = rand_vec(1000000, rng);
  vector <int> d = rand_vec(1000000, rng);
  vector <int> e = rand_vec(1000000, rng);
  vector <int> f = rand_vec(1000000, rng);
  vector <int> g = rand_vec(1000000, rng);
  vector <int> h = rand_vec(1000000, rng);
  vector <int> i = rand_vec(1000000, rng);
  vector <int> j = rand_vec(1000000, rng);

  ~TenVectors()
  {
    gsl_rng_free(rng);
  }
};

TEST_F(SimpleArray, sort)
{
  in_place_multisort_arrays(std::less <int>(), a, b, c);

  ASSERT_EQ(a, vector<int>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));

  ASSERT_EQ(to_vector(b), vector<int>({ 3, 2, 1, 0, 4, 5, 6, 7, 8, 9 }));

  ASSERT_EQ(to_vector(c),
            vector<string>({"d", "c", "b", "a", "e", "f", "g", "h", "i", "j"}));
}

TEST_F(TenVectors, sort)
{
  auto start = high_resolution_clock::now();
  in_place_multisort_arrays(std::less <int>(), a, b, c, d, e, f, g, h, i, j);
  auto stop = high_resolution_clock::now();
  auto duration = duration_cast <milliseconds>(stop - start);
  cout << "Time taken by function: "
    << duration.count() << " milliseconds" << endl;
}

TEST(MultiSortArrays, stable_with_duplicates)
{
  vector<int> keys   = {2, 1, 2, 1, 2};
  vector<char> aux   = {'a', 'b', 'c', 'd', 'e'};
  vector<int> expect_keys = {1, 1, 2, 2, 2};
  vector<char> expect_aux = {'b', 'd', 'a', 'c', 'e'}; // stable order within equal keys

  in_place_multisort_arrays(std::less<int>(), keys, aux);

  EXPECT_EQ(keys, expect_keys);
  EXPECT_EQ(aux, expect_aux);
}

TEST(MultiSortArrays, size_mismatch_throws)
{
  vector<int> keys = {1, 0};
  vector<int> aux  = {10};

  EXPECT_THROW(in_place_multisort_arrays(std::less<int>(), keys, aux), std::invalid_argument);
}
