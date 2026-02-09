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
 * @file sparse_table_test.cc
 * @brief Comprehensive test suite for Sparse Table implementation
 *
 * Tests Gen_Sparse_Table, Sparse_Table (min), and Max_Sparse_Table (max)
 * against brute-force baselines with random and adversarial inputs.
 *
 * TEST CATEGORIES:
 * ================
 * 1. Edge cases (single element, two elements, all-equal)
 * 2. Basic correctness (small known arrays, point queries)
 * 3. Brute-force stress tests (random arrays, random queries)
 * 4. Custom idempotent operations (GCD, bitwise AND, bitwise OR)
 * 5. Construction from all container types (Array, vector, DynList, init-list)
 * 6. Copy/move semantics and swap
 * 7. Exception safety (out-of-range, invalid ranges)
 * 8. Large-scale performance tests
 * 9. Numerical edge cases (negative values, INT_MIN/INT_MAX, doubles)
 *
 * COMPILE:
 *   g++ -std=c++20 -O2 -I.. -o sparse_table_test sparse_table_test.cc
 *
 * RUN:
 *   ./sparse_table_test [seed]
 *
 *   If seed is omitted, a random seed based on time is used.
 */

# include <cstdio>
# include <iostream>
# include <iomanip>
# include <chrono>
# include <cassert>
# include <cmath>
# include <string>
# include <sstream>
# include <random>
# include <numeric>
# include <climits>
# include <cfloat>
# include <functional>
# include <algorithm>
# include <vector>

# include <tpl_sparse_table.H>
# include <tpl_array.H>
# include <tpl_dynList.H>

using namespace std;
using namespace Aleph;

// ============================================================================
// Test Infrastructure
// ============================================================================

static int tests_passed = 0;
static int tests_failed = 0;
static int total_tests = 0;

# define TEST(name) \
  do { \
    total_tests++; \
    cout << "  Testing: " << name << "... " << flush; \
  } while(0)

# define PASS() \
  do { \
    tests_passed++; \
    cout << "\033[32mPASS\033[0m\n"; \
  } while(0)

# define FAIL(msg) \
  do { \
    tests_failed++; \
    cout << "\033[31mFAIL\033[0m (" << msg << ")\n"; \
  } while(0)

# define CHECK(cond, msg) \
  do { \
    if (!(cond)) { FAIL(msg); return; } \
  } while(0)

# define CHECK_EQ(a, b, msg) \
  do { \
    if ((a) != (b)) { \
      stringstream ss; \
      ss << msg << " (expected " << (b) << ", got " << (a) << ")"; \
      FAIL(ss.str()); \
      return; \
    } \
  } while(0)

# define CHECK_THROWS(ExType, expr, msg) \
  do { \
    bool caught = false; \
    try { expr; } catch (const ExType &) { caught = true; } \
    if (!caught) { FAIL(msg); return; } \
  } while(0)

class Timer
{
  chrono::high_resolution_clock::time_point start;
public:
  Timer() : start(chrono::high_resolution_clock::now()) {}
  double elapsed_ms() const
  {
    auto end = chrono::high_resolution_clock::now();
    return chrono::duration<double, milli>(end - start).count();
  }
};

// Global RNG — seeded from main()
static mt19937 rng;

// ============================================================================
// Brute-force baselines
// ============================================================================

template <typename T>
T brute_min(const vector<T> & v, size_t l, size_t r)
{
  T m = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    m = min(m, v[i]);
  return m;
}

template <typename T>
T brute_max(const vector<T> & v, size_t l, size_t r)
{
  T m = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    m = max(m, v[i]);
  return m;
}

int brute_gcd(const vector<int> & v, size_t l, size_t r)
{
  int g = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    g = gcd(g, v[i]);
  return g;
}

int brute_and(const vector<int> & v, size_t l, size_t r)
{
  int a = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    a &= v[i];
  return a;
}

int brute_or(const vector<int> & v, size_t l, size_t r)
{
  int a = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    a |= v[i];
  return a;
}

// ============================================================================
// 1. Edge Cases
// ============================================================================

void test_empty_table()
{
  TEST("empty table");

  Sparse_Table<int> st(std::vector<int>{});
  CHECK_EQ(st.size(), 0u, "size");
  CHECK(st.is_empty(), "is_empty");
  CHECK_EQ(st.num_levels(), 0u, "levels");

  CHECK_THROWS(std::out_of_range, st.get(0),
               "get(0) on empty should throw");
  CHECK_THROWS(std::out_of_range, st.query(0, 0),
               "query(0,0) on empty should throw");

  PASS();
}

void test_single_element()
{
  TEST("single element — min");
  Sparse_Table<int> st = {42};
  CHECK_EQ(st.size(), 1u, "size");
  CHECK_EQ(st.num_levels(), 1u, "levels");
  CHECK_EQ(st.get(0), 42, "get(0)");
  CHECK_EQ(st.query(0, 0), 42, "query(0,0)");
  CHECK(!st.is_empty(), "not empty");
  PASS();
}

void test_single_element_max()
{
  TEST("single element — max");
  Max_Sparse_Table<int> st = {-7};
  CHECK_EQ(st.query(0, 0), -7, "query(0,0)");
  PASS();
}

void test_two_elements()
{
  TEST("two elements — min/max");
  Sparse_Table<int> mn = {10, 3};
  Max_Sparse_Table<int> mx = {10, 3};
  CHECK_EQ(mn.query(0, 1), 3, "min[0,1]");
  CHECK_EQ(mn.query(0, 0), 10, "min[0,0]");
  CHECK_EQ(mn.query(1, 1), 3, "min[1,1]");
  CHECK_EQ(mx.query(0, 1), 10, "max[0,1]");
  CHECK_EQ(mx.query(0, 0), 10, "max[0,0]");
  CHECK_EQ(mx.query(1, 1), 3, "max[1,1]");
  PASS();
}

void test_all_equal()
{
  TEST("all-equal array (n=100)");
  vector<int> v(100, 77);
  Sparse_Table<int> st(v);
  for (size_t l = 0; l < 100; l += 13)
    for (size_t r = l; r < 100; r += 17)
      CHECK_EQ(st.query(l, r), 77, "all-equal query");
  PASS();
}

void test_sorted_ascending()
{
  TEST("sorted ascending (min = leftmost, max = rightmost)");
  vector<int> v(50);
  iota(v.begin(), v.end(), 1); // 1, 2, ..., 50
  Sparse_Table<int> mn(v);
  Max_Sparse_Table<int> mx(v);
  CHECK_EQ(mn.query(0, 49), 1, "min entire");
  CHECK_EQ(mx.query(0, 49), 50, "max entire");
  CHECK_EQ(mn.query(10, 30), 11, "min sub");
  CHECK_EQ(mx.query(10, 30), 31, "max sub");
  PASS();
}

void test_sorted_descending()
{
  TEST("sorted descending");
  vector<int> v(50);
  for (int i = 0; i < 50; ++i) v[i] = 50 - i;
  Sparse_Table<int> mn(v);
  Max_Sparse_Table<int> mx(v);
  CHECK_EQ(mn.query(0, 49), 1, "min entire");
  CHECK_EQ(mx.query(0, 49), 50, "max entire");
  CHECK_EQ(mn.query(0, 0), 50, "min first");
  CHECK_EQ(mx.query(49, 49), 1, "max last");
  PASS();
}

void test_power_of_two_sizes()
{
  TEST("power-of-two sizes (1, 2, 4, 8, 16, 32, 64)");
  for (size_t sz : {1u, 2u, 4u, 8u, 16u, 32u, 64u})
    {
      vector<int> v(sz);
      for (size_t i = 0; i < sz; ++i)
        v[i] = static_cast<int>(rng() % 10000);
      Sparse_Table<int> st(v);
      CHECK_EQ(st.size(), sz, "size mismatch");
      int bmin = *min_element(v.begin(), v.end());
      CHECK_EQ(st.query(0, sz - 1), bmin, "full-range min");
    }
  PASS();
}

void test_non_power_of_two_sizes()
{
  TEST("non-power-of-two sizes (3, 5, 7, 10, 13, 17, 31, 33, 63, 65, 100)");
  for (size_t sz : {3u, 5u, 7u, 10u, 13u, 17u, 31u, 33u, 63u, 65u, 100u})
    {
      vector<int> v(sz);
      for (size_t i = 0; i < sz; ++i)
        v[i] = static_cast<int>(rng() % 10000);
      Sparse_Table<int> st(v);
      CHECK_EQ(st.size(), sz, "size mismatch");
      int bmin = *min_element(v.begin(), v.end());
      CHECK_EQ(st.query(0, sz - 1), bmin, "full-range min");
    }
  PASS();
}

// ============================================================================
// 2. Basic Correctness — known arrays
// ============================================================================

void test_known_min_array()
{
  TEST("known array min queries");
  //                  0  1  2  3  4  5  6  7  8  9
  Sparse_Table<int> st = {5, 2, 4, 7, 1, 3, 6, 8, 0, 9};

  CHECK_EQ(st.query(0, 0), 5, "[0,0]");
  CHECK_EQ(st.query(0, 1), 2, "[0,1]");
  CHECK_EQ(st.query(0, 9), 0, "[0,9]");
  CHECK_EQ(st.query(4, 4), 1, "[4,4]");
  CHECK_EQ(st.query(3, 5), 1, "[3,5]");
  CHECK_EQ(st.query(6, 8), 0, "[6,8]");
  CHECK_EQ(st.query(8, 9), 0, "[8,9]");
  CHECK_EQ(st.query(1, 3), 2, "[1,3]");
  CHECK_EQ(st.query(5, 7), 3, "[5,7]");
  CHECK_EQ(st.query(0, 4), 1, "[0,4]");
  PASS();
}

void test_known_max_array()
{
  TEST("known array max queries");
  Max_Sparse_Table<int> st = {5, 2, 4, 7, 1, 3, 6, 8, 0, 9};

  CHECK_EQ(st.query(0, 9), 9, "[0,9]");
  CHECK_EQ(st.query(0, 3), 7, "[0,3]");
  CHECK_EQ(st.query(4, 7), 8, "[4,7]");
  CHECK_EQ(st.query(6, 8), 8, "[6,8]");
  CHECK_EQ(st.query(8, 9), 9, "[8,9]");
  CHECK_EQ(st.query(2, 2), 4, "[2,2]");
  PASS();
}

void test_get_all_elements()
{
  TEST("get() returns correct element for all positions");
  vector<int> v = {10, -3, 42, 0, 7, -99, 88, 1};
  Sparse_Table<int> st(v);
  for (size_t i = 0; i < v.size(); ++i)
    CHECK_EQ(st.get(i), v[i], "get mismatch");
  PASS();
}

void test_values_reconstruction()
{
  TEST("values() reconstructs original array");
  vector<int> v = {10, -3, 42, 0, 7, -99, 88, 1, 55, -20};
  Sparse_Table<int> st(v);
  auto vals = st.values();
  CHECK_EQ(vals.size(), v.size(), "values size");
  for (size_t i = 0; i < v.size(); ++i)
    CHECK_EQ(vals(i), v[i], "values mismatch");
  PASS();
}

// ============================================================================
// 3. Brute-force stress tests — random data
// ============================================================================

void test_stress_min_small()
{
  TEST("stress: Sparse_Table (min) n=200, 5000 random queries");
  const size_t N = 200;
  const int Q = 5000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng() % 200001) - 100000;

  Sparse_Table<int> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int expected = brute_min(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "query(" << a << "," << b << "): expected " << expected
             << ", got " << got << " (q=" << q << ")";
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_stress_max_small()
{
  TEST("stress: Max_Sparse_Table (max) n=200, 5000 random queries");
  const size_t N = 200;
  const int Q = 5000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng() % 200001) - 100000;

  Max_Sparse_Table<int> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int expected = brute_max(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "query(" << a << "," << b << "): expected " << expected
             << ", got " << got;
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_stress_min_medium()
{
  TEST("stress: Sparse_Table (min) n=10000, 50000 random queries");
  const size_t N = 10000;
  const int Q = 50000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng());

  Sparse_Table<int> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int expected = brute_min(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "query(" << a << "," << b << "): expected " << expected
             << ", got " << got;
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_stress_all_point_queries()
{
  TEST("stress: all point queries match original (n=500)");
  const size_t N = 500;
  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng());

  Sparse_Table<int> st(v);
  for (size_t i = 0; i < N; ++i)
    {
      CHECK_EQ(st.query(i, i), v[i], "point query mismatch");
      CHECK_EQ(st.get(i), v[i], "get mismatch");
    }
  PASS();
}

void test_stress_all_pairs_small()
{
  TEST("stress: ALL (l,r) pairs for n=80 — exhaustive");
  const size_t N = 80;
  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng() % 1000) - 500;

  Sparse_Table<int> mn(v);
  Max_Sparse_Table<int> mx(v);

  for (size_t l = 0; l < N; ++l)
    for (size_t r = l; r < N; ++r)
      {
        int exp_min = brute_min(v, l, r);
        int exp_max = brute_max(v, l, r);
        if (mn.query(l, r) != exp_min)
          {
            stringstream ss;
            ss << "min query(" << l << "," << r << "): expected "
               << exp_min << ", got " << mn.query(l, r);
            FAIL(ss.str());
            return;
          }
        if (mx.query(l, r) != exp_max)
          {
            stringstream ss;
            ss << "max query(" << l << "," << r << "): expected "
               << exp_max << ", got " << mx.query(l, r);
            FAIL(ss.str());
            return;
          }
      }
  PASS();
}

// ============================================================================
// 4. Custom idempotent operations (GCD, AND, OR)
// ============================================================================

struct Gcd_Op
{
  int operator()(const int & a, const int & b) const noexcept
  {
    return gcd(a, b);
  }
};

struct And_Op
{
  int operator()(const int & a, const int & b) const noexcept
  {
    return a & b;
  }
};

struct Or_Op
{
  int operator()(const int & a, const int & b) const noexcept
  {
    return a | b;
  }
};

void test_gcd_known()
{
  TEST("GCD sparse table — known values");
  Gen_Sparse_Table<int, Gcd_Op> st = {12, 18, 24, 36, 60, 48};
  CHECK_EQ(st.query(0, 1), 6, "gcd(12,18)");
  CHECK_EQ(st.query(0, 5), 6, "gcd(all)");
  CHECK_EQ(st.query(2, 4), 12, "gcd(24,36,60)");
  CHECK_EQ(st.query(3, 5), 12, "gcd(36,60,48)");
  CHECK_EQ(st.query(4, 4), 60, "gcd(60)");
  PASS();
}

void test_gcd_stress()
{
  TEST("GCD sparse table — random stress n=300, 10000 queries");
  const size_t N = 300;
  const int Q = 10000;

  vector<int> v(N);
  uniform_int_distribution<int> dist(1, 100000);
  for (size_t i = 0; i < N; ++i)
    v[i] = dist(rng);

  Gen_Sparse_Table<int, Gcd_Op> st(v);

  uniform_int_distribution<size_t> qdist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = qdist(rng), b = qdist(rng);
      if (a > b) swap(a, b);
      int expected = brute_gcd(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "gcd query(" << a << "," << b << "): expected "
             << expected << ", got " << got;
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_bitwise_and_stress()
{
  TEST("AND sparse table — random stress n=200, 5000 queries");
  const size_t N = 200;
  const int Q = 5000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng() & 0x7FFFFFFF);

  Gen_Sparse_Table<int, And_Op> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int expected = brute_and(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "AND query(" << a << "," << b << "): expected "
             << expected << ", got " << got;
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_bitwise_or_stress()
{
  TEST("OR sparse table — random stress n=200, 5000 queries");
  const size_t N = 200;
  const int Q = 5000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng() & 0x7FFFFFFF);

  Gen_Sparse_Table<int, Or_Op> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int expected = brute_or(v, a, b);
      int got = st.query(a, b);
      if (got != expected)
        {
          stringstream ss;
          ss << "OR query(" << a << "," << b << "): expected "
             << expected << ", got " << got;
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

// ============================================================================
// 5. Construction from all container types
// ============================================================================

void test_construct_from_array()
{
  TEST("construct from Array<int>");
  Array<int> arr = {9, 1, 7, 3, 5};
  Sparse_Table<int> st(arr);
  CHECK_EQ(st.size(), 5u, "size");
  CHECK_EQ(st.query(0, 4), 1, "min");
  CHECK_EQ(st.get(2), 7, "get(2)");
  PASS();
}

void test_construct_from_vector()
{
  TEST("construct from std::vector<int>");
  vector<int> v = {9, 1, 7, 3, 5};
  Sparse_Table<int> st(v);
  CHECK_EQ(st.size(), 5u, "size");
  CHECK_EQ(st.query(0, 4), 1, "min");
  PASS();
}

void test_construct_from_dynlist()
{
  TEST("construct from DynList<int>");
  DynList<int> dl = {9, 1, 7, 3, 5};
  Sparse_Table<int> st(dl);
  CHECK_EQ(st.size(), 5u, "size");
  CHECK_EQ(st.query(0, 4), 1, "min");
  PASS();
}

void test_construct_from_initlist()
{
  TEST("construct from initializer_list");
  Sparse_Table<int> st = {9, 1, 7, 3, 5};
  CHECK_EQ(st.size(), 5u, "size");
  CHECK_EQ(st.query(0, 4), 1, "min");
  PASS();
}

void test_construct_uniform_value()
{
  TEST("construct with uniform init_val (n=50, val=-5)");
  Gen_Sparse_Table<int, Min_Op<int>> st(50, -5);
  CHECK_EQ(st.size(), 50u, "size");
  CHECK_EQ(st.query(0, 49), -5, "min");
  CHECK_EQ(st.query(20, 30), -5, "sub min");
  PASS();
}

void test_all_constructors_agree()
{
  TEST("all constructors produce identical query results");
  vector<int> raw = {15, 8, 23, 4, 42, 1, 17, 9, 30, 6};

  Array<int> arr(raw.size());
  for (size_t i = 0; i < raw.size(); ++i)
    arr.append(raw[i]);

  DynList<int> dl;
  for (size_t i = 0; i < raw.size(); ++i)
    dl.append(raw[i]);

  Sparse_Table<int> st_vec(raw);
  Sparse_Table<int> st_arr(arr);
  Sparse_Table<int> st_dl(dl);
  Sparse_Table<int> st_il = {15, 8, 23, 4, 42, 1, 17, 9, 30, 6};

  uniform_int_distribution<size_t> dist(0, raw.size() - 1);
  for (int q = 0; q < 200; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      int v0 = st_vec.query(a, b);
      CHECK_EQ(st_arr.query(a, b), v0, "arr disagrees");
      CHECK_EQ(st_dl.query(a, b), v0, "dl disagrees");
      CHECK_EQ(st_il.query(a, b), v0, "il disagrees");
    }
  PASS();
}

// ============================================================================
// 6. Copy, move, and swap
// ============================================================================

void test_copy_constructor()
{
  TEST("copy constructor");
  Sparse_Table<int> orig = {5, 2, 8, 1, 9};
  Sparse_Table<int> copy(orig);
  CHECK_EQ(copy.size(), orig.size(), "size");
  CHECK_EQ(copy.query(0, 4), orig.query(0, 4), "min");
  CHECK_EQ(copy.get(3), 1, "get(3)");
  // Verify independence: original still works
  CHECK_EQ(orig.query(1, 3), 1, "orig query");
  PASS();
}

void test_move_constructor()
{
  TEST("move constructor");
  Sparse_Table<int> orig = {5, 2, 8, 1, 9};
  size_t orig_sz = orig.size();
  int orig_min = orig.query(0, 4);
  Sparse_Table<int> moved(std::move(orig));
  CHECK_EQ(moved.size(), orig_sz, "size");
  CHECK_EQ(moved.query(0, 4), orig_min, "min");
  PASS();
}

void test_copy_assignment()
{
  TEST("copy assignment");
  Sparse_Table<int> a = {5, 2, 8, 1, 9};
  Sparse_Table<int> b = {100, 200};
  b = a;
  CHECK_EQ(b.size(), 5u, "size");
  CHECK_EQ(b.query(0, 4), 1, "min");
  CHECK_EQ(a.query(0, 4), 1, "orig still works");
  PASS();
}

void test_move_assignment()
{
  TEST("move assignment");
  Sparse_Table<int> a = {5, 2, 8, 1, 9};
  Sparse_Table<int> b = {100, 200};
  b = std::move(a);
  CHECK_EQ(b.size(), 5u, "size");
  CHECK_EQ(b.query(0, 4), 1, "min");
  PASS();
}

void test_swap()
{
  TEST("swap");
  Sparse_Table<int> a = {1, 2, 3};
  Sparse_Table<int> b = {10, 20, 30, 40};
  a.swap(b);
  CHECK_EQ(a.size(), 4u, "a size");
  CHECK_EQ(b.size(), 3u, "b size");
  CHECK_EQ(a.query(0, 3), 10, "a min");
  CHECK_EQ(b.query(0, 2), 1, "b min");
  PASS();
}

// ============================================================================
// 7. Exception safety
// ============================================================================

void test_query_r_out_of_range()
{
  TEST("query throws out_of_range when r >= n");
  Sparse_Table<int> st = {1, 2, 3, 4, 5};
  CHECK_THROWS(std::out_of_range, st.query(0, 5),
               "should throw for r=5, n=5");
  CHECK_THROWS(std::out_of_range, st.query(0, 100),
               "should throw for r=100");
  PASS();
}

void test_query_l_greater_than_r()
{
  TEST("query throws out_of_range when l > r");
  Sparse_Table<int> st = {1, 2, 3, 4, 5};
  CHECK_THROWS(std::out_of_range, st.query(3, 2),
               "should throw for l=3, r=2");
  PASS();
}

void test_get_out_of_range()
{
  TEST("get throws out_of_range when i >= n");
  Sparse_Table<int> st = {1, 2, 3};
  CHECK_THROWS(std::out_of_range, st.get(3),
               "should throw for i=3, n=3");
  CHECK_THROWS(std::out_of_range, st.get(1000),
               "should throw for i=1000");
  PASS();
}

void test_boundary_queries_valid()
{
  TEST("boundary queries that should NOT throw");
  Sparse_Table<int> st = {1, 2, 3, 4, 5};
  // These should all succeed
  int v;
  v = st.query(0, 0); CHECK_EQ(v, 1, "q(0,0)");
  v = st.query(4, 4); CHECK_EQ(v, 5, "q(4,4)");
  v = st.query(0, 4); CHECK_EQ(v, 1, "q(0,4)");
  v = st.get(0);      CHECK_EQ(v, 1, "get(0)");
  v = st.get(4);      CHECK_EQ(v, 5, "get(4)");
  PASS();
}

// ============================================================================
// 8. Numerical edge cases
// ============================================================================

void test_negative_values()
{
  TEST("negative values");
  Sparse_Table<int> st = {-5, -2, -8, -1, -9};
  CHECK_EQ(st.query(0, 4), -9, "min all");
  CHECK_EQ(st.query(0, 2), -8, "min [0,2]");
  Max_Sparse_Table<int> mx = {-5, -2, -8, -1, -9};
  CHECK_EQ(mx.query(0, 4), -1, "max all");
  PASS();
}

void test_int_extremes()
{
  TEST("INT_MIN/INT_MAX values");
  Sparse_Table<int> st = {INT_MAX, 0, INT_MIN, 42, INT_MAX};
  CHECK_EQ(st.query(0, 4), INT_MIN, "min with INT_MIN");
  CHECK_EQ(st.query(0, 1), 0, "min [0,1]");
  CHECK_EQ(st.query(3, 4), 42, "min [3,4]");
  Max_Sparse_Table<int> mx = {INT_MAX, 0, INT_MIN, 42, INT_MAX};
  CHECK_EQ(mx.query(0, 4), INT_MAX, "max with INT_MAX");
  CHECK_EQ(mx.query(1, 3), 42, "max [1,3]");
  PASS();
}

void test_double_values()
{
  TEST("double values (including negative and close values)");
  Sparse_Table<double> st = {3.14, 2.71, 1.41, 1.73, 0.577};
  CHECK(st.query(0, 4) == 0.577, "min double");
  CHECK(st.query(0, 1) == 2.71, "min [0,1]");
  CHECK(st.query(2, 3) == 1.41, "min [2,3]");
  Max_Sparse_Table<double> mx = {3.14, 2.71, 1.41, 1.73, 0.577};
  CHECK(mx.query(0, 4) == 3.14, "max double");
  PASS();
}

void test_double_stress()
{
  TEST("stress: double min/max n=500, 5000 queries");
  const size_t N = 500;
  const int Q = 5000;

  vector<double> v(N);
  uniform_real_distribution<double> dist(-1e9, 1e9);
  for (size_t i = 0; i < N; ++i)
    v[i] = dist(rng);

  Sparse_Table<double> mn(v);
  Max_Sparse_Table<double> mx(v);

  uniform_int_distribution<size_t> qdist(0, N - 1);
  for (int q = 0; q < Q; ++q)
    {
      size_t a = qdist(rng), b = qdist(rng);
      if (a > b) swap(a, b);
      double exp_min = brute_min(v, a, b);
      double exp_max = brute_max(v, a, b);
      if (mn.query(a, b) != exp_min || mx.query(a, b) != exp_max)
        {
          stringstream ss;
          ss << "double query(" << a << "," << b << ") mismatch";
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

void test_long_long_values()
{
  TEST("long long values");
  Sparse_Table<long long> st = {
    1LL << 60, -(1LL << 59), 0LL, 1LL << 50, -(1LL << 62)
  };
  CHECK_EQ(st.query(0, 4), -(1LL << 62), "min ll");
  CHECK_EQ(st.query(0, 0), 1LL << 60, "get(0)");
  Max_Sparse_Table<long long> mx = {
    1LL << 60, -(1LL << 59), 0LL, 1LL << 50, -(1LL << 62)
  };
  CHECK_EQ(mx.query(0, 4), 1LL << 60, "max ll");
  PASS();
}

// ============================================================================
// 9. Performance tests
// ============================================================================

void test_performance_build()
{
  TEST("performance: build n=1,000,000");
  const size_t N = 1000000;
  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng());

  Timer timer;
  Sparse_Table<int> st(v);
  double ms = timer.elapsed_ms();

  CHECK_EQ(st.size(), N, "size");
  cout << "[" << fixed << setprecision(1) << ms << " ms] " << flush;
  CHECK(ms < 5000.0, "build should complete in < 5s");
  PASS();
}

void test_performance_queries()
{
  TEST("performance: 1,000,000 queries on n=100,000");
  const size_t N = 100000;
  const int Q = 1000000;

  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng());

  Sparse_Table<int> st(v);

  uniform_int_distribution<size_t> dist(0, N - 1);
  volatile int sink = 0; // prevent optimization

  Timer timer;
  for (int q = 0; q < Q; ++q)
    {
      size_t a = dist(rng), b = dist(rng);
      if (a > b) swap(a, b);
      sink = st.query(a, b);
    }
  double ms = timer.elapsed_ms();

  cout << "[" << fixed << setprecision(1) << ms << " ms] " << flush;
  CHECK(ms < 10000.0, "1M queries should complete in < 10s");
  (void)sink;
  PASS();
}

void test_performance_build_large()
{
  TEST("performance: build n=5,000,000");
  const size_t N = 5000000;
  vector<int> v(N);
  for (size_t i = 0; i < N; ++i)
    v[i] = static_cast<int>(rng());

  Timer timer;
  Sparse_Table<int> st(v);
  double ms = timer.elapsed_ms();

  CHECK_EQ(st.size(), N, "size");
  // Spot-check a random query
  size_t a = 1000, b = 4999000;
  int got = st.query(a, b);
  int expected = *min_element(v.begin() + a, v.begin() + b + 1);
  CHECK_EQ(got, expected, "spot-check query");

  cout << "[" << fixed << setprecision(1) << ms << " ms] " << flush;
  CHECK(ms < 20000.0, "build should complete in < 20s");
  PASS();
}

// ============================================================================
// 10. Idempotency and overlap correctness
// ============================================================================

void test_overlapping_ranges_idempotent()
{
  TEST("overlapping sub-ranges give correct result (idempotency)");
  // The key property: Op(table[k][l], table[k][r-2^k+1]) is correct
  // because Op(x, x) = x for idempotent operations.
  // This test specifically exercises the overlap.
  vector<int> v = {10, 3, 7, 1, 8, 5, 2, 9, 4, 6};
  Sparse_Table<int> st(v);

  // Range [0, 5] has length 6. k = floor(log2(6)) = 2, 2^2 = 4.
  // So we combine table[2][0] (covers [0,3]) and table[2][2] (covers [2,5]).
  // Overlap is [2,3]. The idempotency of min ensures correctness.
  CHECK_EQ(st.query(0, 5), 1, "min [0,5] with overlap");

  // Range [2, 8] has length 7. k = floor(log2(7)) = 2, 2^2 = 4.
  // Combines table[2][2] (covers [2,5]) and table[2][5] (covers [5,8]).
  // Overlap is [5,5].
  CHECK_EQ(st.query(2, 8), 1, "min [2,8]");

  // Range [3, 9] — length 7.
  CHECK_EQ(st.query(3, 9), 1, "min [3,9]");

  // Range [4, 9] — length 6. min is 2 (position 6).
  CHECK_EQ(st.query(4, 9), 2, "min [4,9]");

  PASS();
}

void test_num_levels_correctness()
{
  TEST("num_levels() = floor(log2(n)) + 1");
  for (size_t n : {1u, 2u, 3u, 4u, 5u, 7u, 8u, 15u, 16u, 17u, 100u, 1024u})
    {
      vector<int> v(n, 0);
      Sparse_Table<int> st(v);
      size_t expected = static_cast<size_t>(std::bit_width(n));
      if (st.num_levels() != expected)
        {
          stringstream ss;
          ss << "n=" << n << ": expected " << expected
             << " levels, got " << st.num_levels();
          FAIL(ss.str());
          return;
        }
    }
  PASS();
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char *argv[])
{
  unsigned seed;
  if (argc > 1)
    seed = static_cast<unsigned>(atoi(argv[1]));
  else
    seed = static_cast<unsigned>(
      chrono::high_resolution_clock::now().time_since_epoch().count());

  rng.seed(seed);

  cout << "╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║      Sparse Table Test Suite                                ║\n";
  cout << "║      Testing Gen_Sparse_Table, Sparse_Table, Max_Sparse    ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Seed: " << seed << "\n\n";

  cout << "=== 1. Edge Cases ===\n";
  test_empty_table();
  test_single_element();
  test_single_element_max();
  test_two_elements();
  test_all_equal();
  test_sorted_ascending();
  test_sorted_descending();
  test_power_of_two_sizes();
  test_non_power_of_two_sizes();

  cout << "\n=== 2. Basic Correctness ===\n";
  test_known_min_array();
  test_known_max_array();
  test_get_all_elements();
  test_values_reconstruction();

  cout << "\n=== 3. Brute-Force Stress Tests ===\n";
  test_stress_min_small();
  test_stress_max_small();
  test_stress_min_medium();
  test_stress_all_point_queries();
  test_stress_all_pairs_small();

  cout << "\n=== 4. Custom Idempotent Operations ===\n";
  test_gcd_known();
  test_gcd_stress();
  test_bitwise_and_stress();
  test_bitwise_or_stress();

  cout << "\n=== 5. Construction from All Container Types ===\n";
  test_construct_from_array();
  test_construct_from_vector();
  test_construct_from_dynlist();
  test_construct_from_initlist();
  test_construct_uniform_value();
  test_all_constructors_agree();

  cout << "\n=== 6. Copy, Move, Swap ===\n";
  test_copy_constructor();
  test_move_constructor();
  test_copy_assignment();
  test_move_assignment();
  test_swap();

  cout << "\n=== 7. Exception Safety ===\n";
  test_query_r_out_of_range();
  test_query_l_greater_than_r();
  test_get_out_of_range();
  test_boundary_queries_valid();

  cout << "\n=== 8. Numerical Edge Cases ===\n";
  test_negative_values();
  test_int_extremes();
  test_double_values();
  test_double_stress();
  test_long_long_values();

  cout << "\n=== 9. Performance ===\n";
  test_performance_build();
  test_performance_queries();
  test_performance_build_large();

  cout << "\n=== 10. Idempotency & Structure ===\n";
  test_overlapping_ranges_idempotent();
  test_num_levels_correctness();

  cout << "\n";
  cout << "══════════════════════════════════════════════════════════════\n";
  cout << "  RESULTS: " << tests_passed << "/" << total_tests << " passed";
  if (tests_failed > 0)
    cout << ", \033[31m" << tests_failed << " FAILED\033[0m";
  else
    cout << " — \033[32mALL PASS\033[0m";
  cout << "\n";
  cout << "══════════════════════════════════════════════════════════════\n";

  return tests_failed > 0 ? 1 : 0;
}
