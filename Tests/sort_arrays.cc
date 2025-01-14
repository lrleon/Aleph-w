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
# include <gtest/gtest.h>

# include <ah-string-utils.H>
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
