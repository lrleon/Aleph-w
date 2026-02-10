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
 * @file disjoint_sparse_table_test.cc
 * @brief Comprehensive test suite for Disjoint Sparse Table implementation
 *
 * Tests Gen_Disjoint_Sparse_Table, Sum_Disjoint_Sparse_Table, and
 * Product_Disjoint_Sparse_Table against brute-force baselines with
 * random and adversarial inputs.
 *
 * TEST CATEGORIES:
 * ================
 * 1. Edge cases (empty, single element, two elements, all-equal, sorted)
 * 2. Basic correctness (small known arrays, point queries)
 * 3. Brute-force stress tests (random arrays, random queries)
 * 4. Custom operations (XOR, min cross-validation with Sparse_Table)
 * 5. Construction from all container types (Array, vector, DynList, init-list)
 * 6. Copy/move semantics and swap
 * 7. Exception safety (out-of-range, invalid ranges)
 * 8. Numerical edge cases (negative values, overflow-prone, doubles)
 * 9. Performance tests
 * 10. Cross-validation with classical Sparse Table (idempotent ops)
 *
 * COMPILE:
 *   g++ -std=c++20 -O2 -I.. -o disjoint_sparse_table_test \
 *       disjoint_sparse_table_test.cc
 *
 * RUN:
 *   ./disjoint_sparse_table_test [seed]
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

# include <tpl_disjoint_sparse_table.H>
# include <tpl_sparse_table.H>
# include <tpl_array.H>
# include <tpl_dynList.H>
# include <string>

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
T brute_sum(const vector<T> & v, size_t l, size_t r)
{
  T s = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    s += v[i];
  return s;
}

template <typename T>
T brute_product(const vector<T> & v, size_t l, size_t r)
{
  T p = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    p *= v[i];
  return p;
}

template <typename T>
T brute_min(const vector<T> & v, size_t l, size_t r)
{
  T m = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    m = min(m, v[i]);
  return m;
}

unsigned brute_xor(const vector<unsigned> & v, size_t l, size_t r)
{
  unsigned x = v[l];
  for (size_t i = l + 1; i <= r; ++i)
    x ^= v[i];
  return x;
}

// ============================================================================
// Custom functors
// ============================================================================

struct Xor_Op
{
  constexpr unsigned operator()(unsigned a, unsigned b) const noexcept
  {
    return a ^ b;
  }
};

// ============================================================================
// 1. Edge Cases
// ============================================================================

void test_empty_table()
{
  TEST("empty table");

  Sum_Disjoint_Sparse_Table<int> st(std::vector<int>{});
  CHECK_EQ(st.size(), 0u, "size");
  CHECK(st.is_empty(), "is_empty");
  CHECK_EQ(st.num_levels(), 0u, "levels");

  CHECK_THROWS(std::out_of_range, st.get(0),
               "get(0) on empty should throw");
  CHECK_THROWS(std::out_of_range, st.query(0, 0),
               "query(0,0) on empty should throw");

  PASS();
}

void test_single_element_sum()
{
  TEST("single element — sum");
  Sum_Disjoint_Sparse_Table<int> st = {42};
  CHECK_EQ(st.size(), 1u, "size");
  CHECK_EQ(st.num_levels(), 0u, "levels");
  CHECK_EQ(st.get(0), 42, "get(0)");
  CHECK_EQ(st.query(0, 0), 42, "query(0,0)");
  CHECK(!st.is_empty(), "not empty");
  PASS();
}

void test_single_element_product()
{
  TEST("single element — product");
  Product_Disjoint_Sparse_Table<long long> st = {7LL};
  CHECK_EQ(st.query(0, 0), 7LL, "query(0,0)");
  PASS();
}

void test_two_elements()
{
  TEST("two elements — sum and product");
  Sum_Disjoint_Sparse_Table<int> s = {10, 3};
  Product_Disjoint_Sparse_Table<int> p = {10, 3};
  CHECK_EQ(s.query(0, 1), 13, "sum[0,1]");
  CHECK_EQ(s.query(0, 0), 10, "sum[0,0]");
  CHECK_EQ(s.query(1, 1), 3, "sum[1,1]");
  CHECK_EQ(p.query(0, 1), 30, "prod[0,1]");
  CHECK_EQ(p.query(0, 0), 10, "prod[0,0]");
  CHECK_EQ(p.query(1, 1), 3, "prod[1,1]");
  PASS();
}

void test_all_equal()
{
  TEST("all-equal array (n=100, val=5)");
  vector<int> v(100, 5);
  Sum_Disjoint_Sparse_Table<int> st(v);
  for (size_t l = 0; l < 100; l += 13)
    for (size_t r = l; r < 100; r += 17)
      CHECK_EQ(st.query(l, r), static_cast<int>(r - l + 1) * 5,
               "all-equal sum query");
  PASS();
}

void test_sorted_ascending()
{
  TEST("sorted ascending — sum");
  vector<int> v(50);
  iota(v.begin(), v.end(), 1); // 1, 2, ..., 50
  Sum_Disjoint_Sparse_Table<int> st(v);
  // sum(1..50) = 50*51/2 = 1275
  CHECK_EQ(st.query(0, 49), 1275, "sum entire");
  // sum(11..31) = sum(1..31) - sum(1..10) = 31*32/2 - 10*11/2 = 496 - 55 = 441
  CHECK_EQ(st.query(10, 30), 441, "sum sub");
  PASS();
}

void test_sorted_descending()
{
  TEST("sorted descending — product (small)");
  // 5, 4, 3, 2, 1
  Product_Disjoint_Sparse_Table<long long> st = {5LL, 4LL, 3LL, 2LL, 1LL};
  CHECK_EQ(st.query(0, 4), 120LL, "prod entire = 5!");
  CHECK_EQ(st.query(0, 2), 60LL, "prod[0,2] = 5*4*3");
  CHECK_EQ(st.query(2, 4), 6LL, "prod[2,4] = 3*2*1");
  CHECK_EQ(st.query(1, 3), 24LL, "prod[1,3] = 4*3*2");
  PASS();
}

void test_power_of_two_sizes()
{
  TEST("power-of-two sizes (1, 2, 4, 8, 16, 32, 64)");
  for (size_t sz : {1u, 2u, 4u, 8u, 16u, 32u, 64u})
    {
      vector<int> v(sz);
      for (size_t i = 0; i < sz; ++i)
        v[i] = static_cast<int>(rng() % 100);
      Sum_Disjoint_Sparse_Table<int> st(v);
      CHECK_EQ(st.size(), sz, "size mismatch");
      int bf = brute_sum(v, 0, sz - 1);
      CHECK_EQ(st.query(0, sz - 1), bf, "full-range sum");
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
        v[i] = static_cast<int>(rng() % 100);
      Sum_Disjoint_Sparse_Table<int> st(v);
      CHECK_EQ(st.size(), sz, "size mismatch");
      int bf = brute_sum(v, 0, sz - 1);
      CHECK_EQ(st.query(0, sz - 1), bf, "full-range sum");
    }
  PASS();
}

// ============================================================================
// 2. Basic Correctness — known arrays
// ============================================================================

void test_known_sum_array()
{
  TEST("known array sum queries");
  //                                 0  1  2  3  4  5  6  7  8  9
  Sum_Disjoint_Sparse_Table<int> st = {5, 2, 4, 7, 1, 3, 6, 8, 0, 9};

  CHECK_EQ(st.query(0, 0), 5, "[0,0]");
  CHECK_EQ(st.query(0, 1), 7, "[0,1]");
  CHECK_EQ(st.query(0, 9), 45, "[0,9]");
  CHECK_EQ(st.query(4, 4), 1, "[4,4]");
  CHECK_EQ(st.query(3, 5), 11, "[3,5]");
  CHECK_EQ(st.query(6, 8), 14, "[6,8]");
  CHECK_EQ(st.query(8, 9), 9, "[8,9]");
  CHECK_EQ(st.query(1, 3), 13, "[1,3]");
  CHECK_EQ(st.query(5, 7), 17, "[5,7]");
  CHECK_EQ(st.query(0, 4), 19, "[0,4]");
  PASS();
}

void test_known_product_array()
{
  TEST("known array product queries");
  Product_Disjoint_Sparse_Table<long long> st =
    {2LL, 3LL, 5LL, 7LL, 11LL};

  CHECK_EQ(st.query(0, 4), 2310LL, "[0,4]");
  CHECK_EQ(st.query(0, 0), 2LL, "[0,0]");
  CHECK_EQ(st.query(1, 3), 105LL, "[1,3]");
  CHECK_EQ(st.query(2, 4), 385LL, "[2,4]");
  CHECK_EQ(st.query(0, 2), 30LL, "[0,2]");
  CHECK_EQ(st.query(3, 4), 77LL, "[3,4]");
  PASS();
}

void test_get_all_elements()
{
  TEST("get() returns correct element for all positions");
  vector<int> v = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> st(v);
  for (size_t i = 0; i < v.size(); ++i)
    CHECK_EQ(st.get(i), v[i], "get mismatch");
  PASS();
}

void test_values_reconstruct()
{
  TEST("values() reconstructs original array");
  vector<int> v = {7, 3, 9, 1, 5, 8, 2};
  Sum_Disjoint_Sparse_Table<int> st(v);
  auto vals = st.values();
  CHECK_EQ(vals.size(), v.size(), "size mismatch");
  for (size_t i = 0; i < v.size(); ++i)
    CHECK_EQ(vals(i), v[i], "value mismatch");
  PASS();
}

// ============================================================================
// 3. Brute-Force Stress Tests
// ============================================================================

void test_stress_sum_small()
{
  TEST("stress: sum n=200, 5000 random queries");
  const size_t n = 200;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 1000) - 500;
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (int q = 0; q < 5000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(st.query(l, r), brute_sum(v, l, r), "stress sum mismatch");
    }
  PASS();
}

void test_stress_product_small()
{
  TEST("stress: product n=50, 3000 random queries (small values)");
  const size_t n = 50;
  vector<long long> v(n);
  for (auto & x : v) x = static_cast<long long>(rng() % 5) + 1; // 1..5
  Product_Disjoint_Sparse_Table<long long> st(v);

  for (int q = 0; q < 3000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(st.query(l, r), brute_product(v, l, r),
               "stress product mismatch");
    }
  PASS();
}

void test_stress_sum_large()
{
  TEST("stress: sum n=10000, 50000 random queries");
  const size_t n = 10000;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 200) - 100;
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (int q = 0; q < 50000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(st.query(l, r), brute_sum(v, l, r), "stress large mismatch");
    }
  PASS();
}

void test_stress_point_queries()
{
  TEST("stress: all point queries match original (n=500)");
  const size_t n = 500;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 10000);
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (size_t i = 0; i < n; ++i)
    {
      CHECK_EQ(st.query(i, i), v[i], "point query mismatch");
      CHECK_EQ(st.get(i), v[i], "get mismatch");
    }
  PASS();
}

void test_stress_exhaustive_small()
{
  TEST("stress: ALL (l,r) pairs for n=80 — exhaustive sum");
  const size_t n = 80;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 100) - 50;
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (size_t l = 0; l < n; ++l)
    for (size_t r = l; r < n; ++r)
      CHECK_EQ(st.query(l, r), brute_sum(v, l, r), "exhaustive mismatch");
  PASS();
}

// ============================================================================
// 4. Custom Operations
// ============================================================================

void test_xor_known()
{
  TEST("XOR disjoint sparse table — known values");
  Gen_Disjoint_Sparse_Table<unsigned, Xor_Op> st =
    {0xA3u, 0x5Fu, 0x12u, 0xB7u, 0x8Cu};

  CHECK_EQ(st.query(0, 0), 0xA3u, "[0,0]");
  CHECK_EQ(st.query(0, 1), 0xA3u ^ 0x5Fu, "[0,1]");
  CHECK_EQ(st.query(0, 4), 0xA3u ^ 0x5Fu ^ 0x12u ^ 0xB7u ^ 0x8Cu, "[0,4]");
  CHECK_EQ(st.query(1, 3), 0x5Fu ^ 0x12u ^ 0xB7u, "[1,3]");
  CHECK_EQ(st.query(2, 4), 0x12u ^ 0xB7u ^ 0x8Cu, "[2,4]");
  PASS();
}

void test_xor_stress()
{
  TEST("XOR stress n=300, 10000 queries");
  const size_t n = 300;
  vector<unsigned> v(n);
  for (auto & x : v) x = static_cast<unsigned>(rng());
  Gen_Disjoint_Sparse_Table<unsigned, Xor_Op> st(v);

  for (int q = 0; q < 10000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(st.query(l, r), brute_xor(v, l, r), "xor mismatch");
    }
  PASS();
}

void test_min_cross_validation()
{
  TEST("min via DST cross-validated with Sparse_Table");
  const size_t n = 200;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 10000);

  Gen_Disjoint_Sparse_Table<int, Min_Op<int>> dst(v);
  Sparse_Table<int> st(v);

  for (int q = 0; q < 5000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(dst.query(l, r), st.query(l, r),
               "min cross-validation mismatch");
    }
  PASS();
}

void test_max_cross_validation()
{
  TEST("max via DST cross-validated with Max_Sparse_Table");
  const size_t n = 200;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 10000);

  Gen_Disjoint_Sparse_Table<int, Max_Op<int>> dst(v);
  Max_Sparse_Table<int> st(v);

  for (int q = 0; q < 5000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      CHECK_EQ(dst.query(l, r), st.query(l, r),
               "max cross-validation mismatch");
    }
  PASS();
}

// ============================================================================
// 5. Construction from All Container Types
// ============================================================================

void test_construct_from_array()
{
  TEST("construct from Array<int>");
  vector<int> raw = {3, 1, 4, 1, 5, 9, 2, 6};
  Array<int> arr(raw.size());
  for (size_t i = 0; i < raw.size(); ++i)
    arr.append(raw[i]);
  Sum_Disjoint_Sparse_Table<int> st(arr);
  CHECK_EQ(st.query(0, 7), brute_sum(raw, 0, 7), "sum mismatch");
  PASS();
}

void test_construct_from_vector()
{
  TEST("construct from std::vector<int>");
  vector<int> raw = {3, 1, 4, 1, 5, 9, 2, 6};
  Sum_Disjoint_Sparse_Table<int> st(raw);
  CHECK_EQ(st.query(0, 7), brute_sum(raw, 0, 7), "sum mismatch");
  PASS();
}

void test_construct_from_dynlist()
{
  TEST("construct from DynList<int>");
  vector<int> raw = {3, 1, 4, 1, 5, 9, 2, 6};
  DynList<int> dl;
  for (size_t i = 0; i < raw.size(); ++i)
    dl.append(raw[i]);
  Sum_Disjoint_Sparse_Table<int> st(dl);
  CHECK_EQ(st.query(0, 7), brute_sum(raw, 0, 7), "sum mismatch");
  PASS();
}

void test_construct_from_init_list()
{
  TEST("construct from initializer_list");
  Sum_Disjoint_Sparse_Table<int> st = {3, 1, 4, 1, 5, 9, 2, 6};
  CHECK_EQ(st.query(0, 7), 31, "sum mismatch");
  PASS();
}

void test_construct_uniform()
{
  TEST("construct with uniform init_val (n=50, val=-5)");
  Sum_Disjoint_Sparse_Table<int> st(50, -5);
  CHECK_EQ(st.query(0, 49), -250, "sum of 50 * -5");
  CHECK_EQ(st.query(10, 19), -50, "sum of 10 * -5");
  CHECK_EQ(st.query(25, 25), -5, "single element");
  PASS();
}

void test_construct_all_identical()
{
  TEST("all constructors produce identical query results");
  vector<int> raw = {7, 2, 9, 4, 6, 1, 8, 3, 5};

  Array<int> arr(raw.size());
  for (size_t i = 0; i < raw.size(); ++i) arr.append(raw[i]);
  DynList<int> dl;
  for (size_t i = 0; i < raw.size(); ++i) dl.append(raw[i]);

  Sum_Disjoint_Sparse_Table<int> from_vec(raw);
  Sum_Disjoint_Sparse_Table<int> from_arr(arr);
  Sum_Disjoint_Sparse_Table<int> from_dl(dl);
  Sum_Disjoint_Sparse_Table<int> from_il = {7, 2, 9, 4, 6, 1, 8, 3, 5};

  for (size_t l = 0; l < raw.size(); ++l)
    for (size_t r = l; r < raw.size(); ++r)
      {
        int expected = from_vec.query(l, r);
        CHECK_EQ(from_arr.query(l, r), expected, "arr != vec");
        CHECK_EQ(from_dl.query(l, r), expected, "dl != vec");
        CHECK_EQ(from_il.query(l, r), expected, "il != vec");
      }
  PASS();
}

// ============================================================================
// 6. Copy, Move, Swap
// ============================================================================

void test_copy_constructor()
{
  TEST("copy constructor");
  vector<int> v = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> orig(v);
  Sum_Disjoint_Sparse_Table<int> copy(orig);

  CHECK_EQ(copy.size(), orig.size(), "size");
  for (size_t l = 0; l < v.size(); ++l)
    for (size_t r = l; r < v.size(); ++r)
      CHECK_EQ(copy.query(l, r), orig.query(l, r), "query mismatch");
  PASS();
}

void test_move_constructor()
{
  TEST("move constructor");
  vector<int> v = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> orig(v);
  int full_sum = orig.query(0, 4);

  Sum_Disjoint_Sparse_Table<int> moved(std::move(orig));
  CHECK_EQ(moved.size(), 5u, "size");
  CHECK_EQ(moved.query(0, 4), full_sum, "query after move");
  PASS();
}

void test_copy_assignment()
{
  TEST("copy assignment");
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> a(v1);
  Sum_Disjoint_Sparse_Table<int> b(v2);

  a = b;
  CHECK_EQ(a.size(), 5u, "size");
  CHECK_EQ(a.query(0, 4), b.query(0, 4), "query");
  PASS();
}

void test_move_assignment()
{
  TEST("move assignment");
  vector<int> v1 = {1, 2, 3};
  vector<int> v2 = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> a(v1);
  Sum_Disjoint_Sparse_Table<int> b(v2);
  int expected = b.query(0, 4);

  a = std::move(b);
  CHECK_EQ(a.size(), 5u, "size");
  CHECK_EQ(a.query(0, 4), expected, "query");
  PASS();
}

void test_swap()
{
  TEST("swap");
  Sum_Disjoint_Sparse_Table<int> a = {1, 2, 3};
  Sum_Disjoint_Sparse_Table<int> b = {10, 20, 30, 40};

  int a_sum = a.query(0, 2);
  int b_sum = b.query(0, 3);
  size_t a_sz = a.size();
  size_t b_sz = b.size();

  a.swap(b);
  CHECK_EQ(a.size(), b_sz, "a.size after swap");
  CHECK_EQ(b.size(), a_sz, "b.size after swap");
  CHECK_EQ(a.query(0, 3), b_sum, "a.query after swap");
  CHECK_EQ(b.query(0, 2), a_sum, "b.query after swap");
  PASS();
}

// ============================================================================
// 7. Exception Safety
// ============================================================================

void test_exception_r_out_of_range()
{
  TEST("query throws out_of_range when r >= n");
  Sum_Disjoint_Sparse_Table<int> st = {1, 2, 3, 4, 5};
  CHECK_THROWS(std::out_of_range, st.query(0, 5),
               "r=5 >= n=5 should throw");
  CHECK_THROWS(std::out_of_range, st.query(0, 100),
               "r=100 should throw");
  PASS();
}

void test_exception_l_greater_than_r()
{
  TEST("query throws out_of_range when l > r");
  Sum_Disjoint_Sparse_Table<int> st = {1, 2, 3, 4, 5};
  CHECK_THROWS(std::out_of_range, st.query(3, 2),
               "l=3 > r=2 should throw");
  PASS();
}

void test_exception_get_out_of_range()
{
  TEST("get throws out_of_range when i >= n");
  Sum_Disjoint_Sparse_Table<int> st = {1, 2, 3};
  CHECK_THROWS(std::out_of_range, st.get(3), "i=3 >= n=3 should throw");
  CHECK_THROWS(std::out_of_range, st.get(100), "i=100 should throw");
  PASS();
}

void test_boundary_queries_no_throw()
{
  TEST("boundary queries that should NOT throw");
  Sum_Disjoint_Sparse_Table<int> st = {1, 2, 3, 4, 5};
  bool ok = true;
  try
    {
      st.query(0, 0);
      st.query(4, 4);
      st.query(0, 4);
      st.get(0);
      st.get(4);
    }
  catch (...)
    {
      ok = false;
    }
  CHECK(ok, "boundary queries should not throw");
  PASS();
}

// ============================================================================
// 8. Numerical Edge Cases
// ============================================================================

void test_negative_values()
{
  TEST("negative values — sum");
  Sum_Disjoint_Sparse_Table<int> st = {-10, -5, -20, -1, -15};
  CHECK_EQ(st.query(0, 4), -51, "sum of negatives");
  CHECK_EQ(st.query(1, 3), -26, "partial sum of negatives");
  PASS();
}

void test_int_limits()
{
  TEST("INT_MAX values — sum (beware overflow)");
  // Use long long to avoid overflow
  Sum_Disjoint_Sparse_Table<long long> st =
    {1000000000LL, 2000000000LL, 3000000000LL, 4000000000LL};
  CHECK_EQ(st.query(0, 3), 10000000000LL, "large sum");
  CHECK_EQ(st.query(1, 2), 5000000000LL, "partial large sum");
  PASS();
}

void test_double_sum()
{
  TEST("double values — sum");
  vector<double> v = {1.5, 2.3, -0.8, 4.1, 3.7};
  Sum_Disjoint_Sparse_Table<double> st(v);

  double expected = 1.5 + 2.3 + (-0.8) + 4.1 + 3.7;
  double result = st.query(0, 4);
  CHECK(abs(result - expected) < 1e-10, "double sum mismatch");
  PASS();
}

void test_double_product()
{
  TEST("double values — product");
  vector<double> v = {0.5, 2.0, 3.0, 0.1, 10.0};
  Product_Disjoint_Sparse_Table<double> st(v);

  double expected = 0.5 * 2.0 * 3.0 * 0.1 * 10.0;
  double result = st.query(0, 4);
  CHECK(abs(result - expected) < 1e-10, "double product mismatch");

  // Partial product
  double partial = 2.0 * 3.0 * 0.1;
  CHECK(abs(st.query(1, 3) - partial) < 1e-10, "partial product mismatch");
  PASS();
}

void test_stress_double_sum()
{
  TEST("stress: double sum n=500, 5000 queries");
  const size_t n = 500;
  vector<double> v(n);
  uniform_real_distribution<double> dist(-100.0, 100.0);
  for (auto & x : v) x = dist(rng);
  Sum_Disjoint_Sparse_Table<double> st(v);

  for (int q = 0; q < 5000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      double bf = 0;
      for (size_t i = l; i <= r; ++i) bf += v[i];
      double result = st.query(l, r);
      CHECK(abs(result - bf) < 1e-6 * max(1.0, abs(bf)),
            "double stress mismatch");
    }
  PASS();
}

// ============================================================================
// 9. Performance
// ============================================================================

void test_perf_build()
{
  TEST("performance: build n=1,000,000");
  const size_t n = 1000000;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 1000000);

  Timer t;
  Sum_Disjoint_Sparse_Table<int> st(v);
  double ms = t.elapsed_ms();

  // Sanity check
  CHECK_EQ(st.size(), n, "size");
  cout << "[" << fixed << setprecision(1) << ms << " ms] ";
  PASS();
}

void test_perf_queries()
{
  TEST("performance: 1,000,000 queries on n=100,000");
  const size_t n = 100000;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 1000);
  Sum_Disjoint_Sparse_Table<int> st(v);

  volatile long long sink = 0;
  Timer t;
  for (int q = 0; q < 1000000; ++q)
    {
      size_t l = rng() % n, r = rng() % n;
      if (l > r) swap(l, r);
      sink += st.query(l, r);
    }
  double ms = t.elapsed_ms();
  cout << "[" << fixed << setprecision(1) << ms << " ms] ";
  PASS();
}

void test_perf_build_large()
{
  TEST("performance: build n=5,000,000");
  const size_t n = 5000000;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 1000000);

  Timer t;
  Sum_Disjoint_Sparse_Table<int> st(v);
  double ms = t.elapsed_ms();

  CHECK_EQ(st.size(), n, "size");
  cout << "[" << fixed << setprecision(1) << ms << " ms] ";
  PASS();
}

// ============================================================================
// 10. Associativity & Disjointness Verification
// ============================================================================

void test_non_idempotent_correctness()
{
  TEST("sum is non-idempotent: overlapping would be wrong");
  // This test verifies that the DST works correctly for sum,
  // which a classical Sparse Table cannot handle.
  // sum(a, a) = 2a != a, so overlapping sub-ranges give wrong results.
  vector<int> v = {10, 20, 30, 40, 50};
  Sum_Disjoint_Sparse_Table<int> dst(v);

  // Verify all ranges against brute-force sum
  for (size_t l = 0; l < v.size(); ++l)
    for (size_t r = l; r < v.size(); ++r)
      CHECK_EQ(dst.query(l, r), brute_sum(v, l, r),
               "non-idempotent correctness");
  PASS();
}

void test_num_levels()
{
  TEST("num_levels() matches expected formula");
  // For n <= 1: levels = 0
  // For n >= 2: levels = bit_width(n - 1)
  Sum_Disjoint_Sparse_Table<int> s0(std::vector<int>{});
  CHECK_EQ(s0.num_levels(), 0u, "n=0");

  Sum_Disjoint_Sparse_Table<int> s1 = {42};
  CHECK_EQ(s1.num_levels(), 0u, "n=1");

  Sum_Disjoint_Sparse_Table<int> s2 = {1, 2};
  CHECK_EQ(s2.num_levels(), 1u, "n=2");

  Sum_Disjoint_Sparse_Table<int> s4 = {1, 2, 3, 4};
  CHECK_EQ(s4.num_levels(), 2u, "n=4");

  Sum_Disjoint_Sparse_Table<int> s5 = {1, 2, 3, 4, 5};
  CHECK_EQ(s5.num_levels(), 3u, "n=5");

  Sum_Disjoint_Sparse_Table<int> s8 = {1, 2, 3, 4, 5, 6, 7, 8};
  CHECK_EQ(s8.num_levels(), 3u, "n=8");

  vector<int> v9(9, 1);
  Sum_Disjoint_Sparse_Table<int> s9(v9);
  CHECK_EQ(s9.num_levels(), 4u, "n=9");
  PASS();
}

// ============================================================================
// 11. Robustness & Algebraic Properties
// ============================================================================

void test_self_copy_assignment()
{
  TEST("self copy-assignment (st = st)");
  Sum_Disjoint_Sparse_Table<int> st = {5, 3, 8, 1, 7, 2, 9};
  int orig = st.query(0, 6);
  size_t orig_sz = st.size();

  auto & ref = st;
  st = ref;

  CHECK_EQ(st.size(), orig_sz, "size after self-assign");
  CHECK_EQ(st.query(0, 6), orig, "query after self-assign");
  for (size_t i = 0; i < st.size(); ++i)
    CHECK_EQ(st.get(i), st.get(i), "element intact");
  PASS();
}

void test_self_swap()
{
  TEST("self swap (st.swap(st))");
  Sum_Disjoint_Sparse_Table<int> st = {5, 3, 8, 1, 7, 2, 9};
  int orig = st.query(0, 6);
  size_t orig_sz = st.size();
  st.swap(st);
  CHECK_EQ(st.size(), orig_sz, "size after self-swap");
  CHECK_EQ(st.query(0, 6), orig, "query after self-swap");
  PASS();
}

void test_get_equals_point_query()
{
  TEST("get(i) == query(i,i) for all i (n=300)");
  const size_t n = 300;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 10000) - 5000;
  Sum_Disjoint_Sparse_Table<int> st(v);
  for (size_t i = 0; i < n; ++i)
    CHECK_EQ(st.get(i), st.query(i, i), "get != point query");
  PASS();
}

void test_splitting_composability()
{
  TEST("splitting: query(l,r) == op(query(l,m), query(m+1,r))");
  const size_t n = 60;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 100) - 50;
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (size_t l = 0; l < n; ++l)
    for (size_t r = l + 1; r < n; ++r)
      {
        int expected = st.query(l, r);
        // Every valid split must produce the same result
        for (size_t m = l; m < r; ++m)
          {
            int combined = st.query(l, m) + st.query(m + 1, r);
            CHECK_EQ(combined, expected, "split composability mismatch");
          }
      }
  PASS();
}

void test_splitting_product()
{
  TEST("splitting: product query(l,r) == query(l,m) * query(m+1,r)");
  const size_t n = 30;
  vector<long long> v(n);
  for (auto & x : v) x = static_cast<long long>(rng() % 4) + 1; // 1..4
  Product_Disjoint_Sparse_Table<long long> st(v);

  for (size_t l = 0; l < n; ++l)
    for (size_t r = l + 1; r < n; ++r)
      {
        long long expected = st.query(l, r);
        for (size_t m = l; m < r; ++m)
          {
            long long combined = st.query(l, m) * st.query(m + 1, r);
            CHECK_EQ(combined, expected, "product split mismatch");
          }
      }
  PASS();
}

void test_prefix_sum_consistency()
{
  TEST("prefix sum: query(0,r) == sum of get(i) for i in [0,r]");
  const size_t n = 200;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 200) - 100;
  Sum_Disjoint_Sparse_Table<int> st(v);

  int running = 0;
  for (size_t r = 0; r < n; ++r)
    {
      running += st.get(r);
      CHECK_EQ(st.query(0, r), running, "prefix sum mismatch");
    }
  PASS();
}

void test_adversarial_zigzag()
{
  TEST("adversarial: zigzag pattern (alternating high/low)");
  const size_t n = 100;
  vector<int> v(n);
  for (size_t i = 0; i < n; ++i)
    v[i] = (i % 2 == 0) ? 1000 : -1000;

  Sum_Disjoint_Sparse_Table<int> st(v);
  for (size_t l = 0; l < n; ++l)
    for (size_t r = l; r < n; r += 7)
      CHECK_EQ(st.query(l, r), brute_sum(v, l, r), "zigzag mismatch");
  PASS();
}

void test_adversarial_single_outlier()
{
  TEST("adversarial: single non-zero among zeros");
  const size_t n = 128;
  for (size_t outlier_pos = 0; outlier_pos < n; outlier_pos += 11)
    {
      vector<int> v(n, 0);
      v[outlier_pos] = 42;
      Sum_Disjoint_Sparse_Table<int> st(v);

      for (size_t l = 0; l < n; ++l)
        for (size_t r = l; r < n; r += 13)
          {
            int expected = (l <= outlier_pos && outlier_pos <= r) ? 42 : 0;
            CHECK_EQ(st.query(l, r), expected, "outlier mismatch");
          }
    }
  PASS();
}

void test_adversarial_plateau_with_spike()
{
  TEST("adversarial: plateau with single spike at every position");
  const size_t n = 50;
  for (size_t spike = 0; spike < n; ++spike)
    {
      vector<int> v(n, 1);
      v[spike] = 1000;
      Sum_Disjoint_Sparse_Table<int> st(v);

      CHECK_EQ(st.query(0, n - 1),
               static_cast<int>(n - 1) + 1000, "full range with spike");
      if (spike > 0)
        CHECK_EQ(st.query(0, spike - 1),
                 static_cast<int>(spike), "before spike");
      if (spike < n - 1)
        CHECK_EQ(st.query(spike + 1, n - 1),
                 static_cast<int>(n - 1 - spike), "after spike");
    }
  PASS();
}

void test_sliding_window()
{
  TEST("sliding window queries (width=10) across n=200");
  const size_t n = 200;
  const size_t w = 10;
  vector<int> v(n);
  for (auto & x : v) x = static_cast<int>(rng() % 1000) - 500;
  Sum_Disjoint_Sparse_Table<int> st(v);

  for (size_t l = 0; l + w - 1 < n; ++l)
    CHECK_EQ(st.query(l, l + w - 1), brute_sum(v, l, l + w - 1),
             "sliding window mismatch");
  PASS();
}

// String concatenation: non-numeric, non-idempotent associative op
struct String_Concat_Op
{
  string operator()(const string & a, const string & b) const
  {
    return a + b;
  }
};

void test_string_concatenation()
{
  TEST("string concatenation (non-numeric associative op)");
  vector<string> words = {"the", " ", "quick", " ", "brown", " ", "fox"};
  Gen_Disjoint_Sparse_Table<string, String_Concat_Op> st(words);

  CHECK_EQ(st.query(0, 6), string("the quick brown fox"), "[0,6]");
  CHECK_EQ(st.query(0, 0), string("the"), "[0,0]");
  CHECK_EQ(st.query(2, 4), string("quick brown"), "[2,4]");
  CHECK_EQ(st.query(4, 6), string("brown fox"), "[4,6]");
  CHECK_EQ(st.query(0, 2), string("the quick"), "[0,2]");
  CHECK_EQ(st.get(0), string("the"), "get(0)");
  CHECK_EQ(st.get(6), string("fox"), "get(6)");

  // Splitting composability for strings
  for (size_t l = 0; l < words.size(); ++l)
    for (size_t r = l + 1; r < words.size(); ++r)
      {
        string expected = st.query(l, r);
        for (size_t m = l; m < r; ++m)
          {
            string combined = st.query(l, m) + st.query(m + 1, r);
            CHECK_EQ(combined, expected, "string split mismatch");
          }
      }
  PASS();
}

void test_string_stress()
{
  TEST("string concat stress (n=100, all pairs)");
  const size_t n = 100;
  vector<string> v(n);
  for (size_t i = 0; i < n; ++i)
    v[i] = string(1, 'a' + static_cast<char>(i % 26));
  Gen_Disjoint_Sparse_Table<string, String_Concat_Op> st(v);

  for (size_t l = 0; l < n; ++l)
    for (size_t r = l; r < n; ++r)
      {
        string bf;
        for (size_t i = l; i <= r; ++i) bf += v[i];
        CHECK_EQ(st.query(l, r), bf, "string stress mismatch");
      }
  PASS();
}

// ============================================================================
// main
// ============================================================================

int main(int argc, char * argv[])
{
  unsigned seed;
  if (argc > 1)
    seed = static_cast<unsigned>(atoi(argv[1]));
  else
    seed = static_cast<unsigned>(
      chrono::system_clock::now().time_since_epoch().count());

  rng.seed(seed);

  cout << "╔══════════════════════════════════════════════════════════════╗\n";
  cout << "║      Disjoint Sparse Table Test Suite                       ║\n";
  cout << "║      Testing Gen_Disjoint_Sparse_Table, Sum, Product        ║\n";
  cout << "╚══════════════════════════════════════════════════════════════╝\n";
  cout << "  Seed: " << seed << "\n\n";

  cout << "=== 1. Edge Cases ===\n";
  test_empty_table();
  test_single_element_sum();
  test_single_element_product();
  test_two_elements();
  test_all_equal();
  test_sorted_ascending();
  test_sorted_descending();
  test_power_of_two_sizes();
  test_non_power_of_two_sizes();

  cout << "\n=== 2. Basic Correctness ===\n";
  test_known_sum_array();
  test_known_product_array();
  test_get_all_elements();
  test_values_reconstruct();

  cout << "\n=== 3. Brute-Force Stress Tests ===\n";
  test_stress_sum_small();
  test_stress_product_small();
  test_stress_sum_large();
  test_stress_point_queries();
  test_stress_exhaustive_small();

  cout << "\n=== 4. Custom Operations & Cross-Validation ===\n";
  test_xor_known();
  test_xor_stress();
  test_min_cross_validation();
  test_max_cross_validation();

  cout << "\n=== 5. Construction from All Container Types ===\n";
  test_construct_from_array();
  test_construct_from_vector();
  test_construct_from_dynlist();
  test_construct_from_init_list();
  test_construct_uniform();
  test_construct_all_identical();

  cout << "\n=== 6. Copy, Move, Swap ===\n";
  test_copy_constructor();
  test_move_constructor();
  test_copy_assignment();
  test_move_assignment();
  test_swap();

  cout << "\n=== 7. Exception Safety ===\n";
  test_exception_r_out_of_range();
  test_exception_l_greater_than_r();
  test_exception_get_out_of_range();
  test_boundary_queries_no_throw();

  cout << "\n=== 8. Numerical Edge Cases ===\n";
  test_negative_values();
  test_int_limits();
  test_double_sum();
  test_double_product();
  test_stress_double_sum();

  cout << "\n=== 9. Performance ===\n";
  test_perf_build();
  test_perf_queries();
  test_perf_build_large();

  cout << "\n=== 10. Associativity & Disjointness ===\n";
  test_non_idempotent_correctness();
  test_num_levels();

  cout << "\n=== 11. Robustness & Algebraic Properties ===\n";
  test_self_copy_assignment();
  test_self_swap();
  test_get_equals_point_query();
  test_splitting_composability();
  test_splitting_product();
  test_prefix_sum_consistency();
  test_adversarial_zigzag();
  test_adversarial_single_outlier();
  test_adversarial_plateau_with_spike();
  test_sliding_window();
  test_string_concatenation();
  test_string_stress();

  cout << "\n══════════════════════════════════════════════════════════════\n";
  cout << "  RESULTS: " << tests_passed << "/" << total_tests << " passed";
  if (tests_failed == 0)
    cout << " — \033[32mALL PASS\033[0m\n";
  else
    cout << " — \033[31m" << tests_failed << " FAILED\033[0m\n";
  cout << "══════════════════════════════════════════════════════════════\n";

  return tests_failed == 0 ? 0 : 1;
}
