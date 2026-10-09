
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
 * @file gmpfrxx_test.cc
 * @brief Move construction, move assignment and swap of mpz_class, mpq_class
 *        and mpfr_class, and their effect on the containers.
 */

#include <gtest/gtest.h>

#include <gmpfrxx.h>
#include <htlist.H>
#include <tpl_array.H>
#include <tpl_sort_utils.H>

#include <type_traits>
#include <utility>
#include <vector>

// eepicgeom.C, inside libAleph, declares tiny_keys extern: every program that
// links it defines the variable. This test does not use eepicgeom, but some
// linkers (AppleClang) take a GMP template instantiation from eepicgeom.C.o,
// which then needs tiny_keys.
bool tiny_keys = false;

using namespace Aleph;

namespace
{
  // Values with many limbs, so that copying one must allocate.
  mpz_class big_integer()
  {
    mpz_class z;
    mpz_ui_pow_ui(z.get_mpz_t(), 3, 4000);
    return z;
  }

  mpq_class big_rational()
  {
    mpq_class q;
    mpz_ui_pow_ui(mpq_numref(q.get_mpq_t()), 3, 4000);
    mpz_ui_pow_ui(mpq_denref(q.get_mpq_t()), 5, 3000);
    return q;
  }

  /* Counts the allocations GMP and MPFR make, through the memory hook of
     GMP. The counting functions wrap the ones installed before, so memory
     taken with one set is freed correctly with the other. A copy of a value
     that does not fit in the destination shows up as a reallocation; moves
     and swaps make none. */
  class Gmp_Alloc_Counter
  {
    static inline void *(*def_alloc)(size_t) = nullptr;
    static inline void *(*def_realloc)(void *, size_t, size_t) = nullptr;
    static inline void (*def_free)(void *, size_t) = nullptr;
    static inline long allocs = 0;
    static inline long reallocs = 0;

    static void *count_alloc(size_t n) { ++allocs; return def_alloc(n); }
    static void *count_realloc(void *p, size_t old_n, size_t n)
    {
      ++reallocs;
      return def_realloc(p, old_n, n);
    }
    static void count_free(void *p, size_t n) { def_free(p, n); }

  public:
    Gmp_Alloc_Counter()
    {
      mpfr_mp_memory_cleanup();  // MPFR caches the memory functions of GMP
      mp_get_memory_functions(&def_alloc, &def_realloc, &def_free);
      mp_set_memory_functions(count_alloc, count_realloc, count_free);
      reset();
    }

    ~Gmp_Alloc_Counter()
    {
      mpfr_mp_memory_cleanup();
      mp_set_memory_functions(def_alloc, def_realloc, def_free);
    }

    void reset() { allocs = reallocs = 0; }
    long num_allocs() const { return allocs; }
    long num_reallocs() const { return reallocs; }
    long total() const { return allocs + reallocs; }
  };
}

TEST(Gmpfrxx, noexcept_specifications)
{
  // Move assignment and swap exchange the structs: nothing can throw.
  static_assert(std::is_nothrow_move_assignable_v<mpz_class>);
  static_assert(std::is_nothrow_move_assignable_v<mpq_class>);
  static_assert(std::is_nothrow_move_assignable_v<mpfr_class>);
  static_assert(std::is_nothrow_swappable_v<mpz_class>);
  static_assert(std::is_nothrow_swappable_v<mpq_class>);
  static_assert(std::is_nothrow_swappable_v<mpfr_class>);

  // Move construction re-initializes the source. For a rational (the limb of
  // the denominator) and a float (the significand) that allocates, so they
  // are not noexcept, as in gmpxx.h. GMP 6.2 initializes integers lazily.
  static_assert(std::is_move_constructible_v<mpq_class>);
  static_assert(not std::is_nothrow_move_constructible_v<mpq_class>);
  static_assert(std::is_move_constructible_v<mpfr_class>);
  static_assert(not std::is_nothrow_move_constructible_v<mpfr_class>);
#if __GNU_MP_VERSION > 6 || (__GNU_MP_VERSION == 6 && __GNU_MP_VERSION_MINOR >= 2)
  static_assert(std::is_nothrow_default_constructible_v<mpz_class>);
  static_assert(std::is_nothrow_move_constructible_v<mpz_class>);
#endif
}

TEST(Gmpfrxx, move_construction_takes_the_value_and_leaves_zero)
{
  mpz_class z = big_integer();
  const mpz_class z_copy = z;
  mpz_class z2(std::move(z));
  EXPECT_EQ(z2, z_copy);
  EXPECT_EQ(z, 0);
  z = 5;  // the moved-from object is usable
  EXPECT_EQ(z, 5);

  mpq_class q = big_rational();
  const mpq_class q_copy = q;
  mpq_class q2(std::move(q));
  EXPECT_EQ(q2, q_copy);
  EXPECT_EQ(q, 0);
  EXPECT_EQ(q.get_den(), 1);
  q = mpq_class("2/3");
  EXPECT_EQ(q, mpq_class("2/3"));

  mpfr_class f(1.5, 256);
  mpfr_class f2(std::move(f));
  EXPECT_EQ(f2.get_d(), 1.5);
  EXPECT_EQ(f2.get_prec(), 256ul);
  EXPECT_EQ(f.get_d(), 0.0);
  EXPECT_EQ(f.get_prec(), 256ul);  // zero, with the same precision
  f = 2.5;
  EXPECT_EQ(f.get_d(), 2.5);
}

TEST(Gmpfrxx, move_assignment_exchanges_the_values)
{
  mpq_class a = big_rational(), b("2/3");
  const mpq_class a_copy = a;
  b = std::move(a);
  EXPECT_EQ(b, a_copy);
  EXPECT_EQ(a, mpq_class("2/3"));

  // The precision travels with the value; copy assignment would keep the
  // precision of the destination.
  mpfr_class x(1.5, 64), y(2.5, 256);
  y = std::move(x);
  EXPECT_EQ(y.get_d(), 1.5);
  EXPECT_EQ(y.get_prec(), 64ul);
  EXPECT_EQ(x.get_d(), 2.5);
  EXPECT_EQ(x.get_prec(), 256ul);
}

TEST(Gmpfrxx, self_move_assignment_is_harmless)
{
  mpq_class q = big_rational();
  const mpq_class q_copy = q;
  mpq_class & same = q;
  q = std::move(same);
  EXPECT_EQ(q, q_copy);

  mpfr_class f(1.5, 128);
  mpfr_class & same_f = f;
  f = std::move(same_f);
  EXPECT_EQ(f.get_d(), 1.5);
  EXPECT_EQ(f.get_prec(), 128ul);
}

TEST(Gmpfrxx, swap)
{
  mpz_class a = big_integer(), b = 7;
  swap(a, b);  // the overload of gmpfrxx.h, found by ADL
  EXPECT_EQ(a, 7);
  EXPECT_EQ(b, big_integer());
  std::swap(a, b);  // the generic one works too
  EXPECT_EQ(a, big_integer());
  EXPECT_EQ(b, 7);

  mpfr_class x(1.5, 64), y(2.5, 256);
  x.swap(y);
  EXPECT_EQ(x.get_d(), 2.5);
  EXPECT_EQ(x.get_prec(), 256ul);
  EXPECT_EQ(y.get_d(), 1.5);
  EXPECT_EQ(y.get_prec(), 64ul);
}

TEST(Gmpfrxx, moving_does_not_copy_the_limbs)
{
  mpq_class src = big_rational(), dst;
  Gmp_Alloc_Counter counter;

  dst = src;  // a copy must grow the limbs of the destination
  EXPECT_GT(counter.total(), 0);

  counter.reset();
  mpq_class moved(std::move(src));  // re-initializing src: one allocation
  EXPECT_EQ(counter.num_reallocs(), 0);
  EXPECT_LE(counter.num_allocs(), 2);

  counter.reset();
  dst = std::move(moved);  // an exchange
  EXPECT_EQ(counter.total(), 0);

  counter.reset();
  swap(dst, src);
  EXPECT_EQ(counter.total(), 0);

  mpz_class z = big_integer();
  counter.reset();
  mpz_class z2(std::move(z));
#if __GNU_MP_VERSION > 6 || (__GNU_MP_VERSION == 6 && __GNU_MP_VERSION_MINOR >= 2)
  EXPECT_EQ(counter.total(), 0);
#else
  EXPECT_EQ(counter.num_reallocs(), 0);
#endif
  counter.reset();
  z = std::move(z2);
  EXPECT_EQ(counter.total(), 0);

  mpfr_class f(1.5, 4096);
  counter.reset();
  mpfr_class f2(std::move(f));  // re-initializing f: one allocation
  EXPECT_EQ(counter.num_reallocs(), 0);
  EXPECT_LE(counter.num_allocs(), 1);
  counter.reset();
  f = std::move(f2);
  EXPECT_EQ(counter.total(), 0);
}

TEST(Gmpfrxx, rational_from_an_rvalue_integer_takes_its_limbs)
{
  mpz_class z = big_integer();
  const mpz_class z_copy = z;
  Gmp_Alloc_Counter counter;

  mpq_class q(std::move(z));
  EXPECT_EQ(counter.num_reallocs(), 0);
  EXPECT_EQ(q.get_num(), z_copy);
  EXPECT_EQ(q.get_den(), 1);
  EXPECT_EQ(z, 0);

  z = z_copy;
  q = mpq_class("2/3");
  counter.reset();
  q = std::move(z);  // by exchange with the numerator
  EXPECT_EQ(counter.total(), 0);
  EXPECT_EQ(q, z_copy);
  EXPECT_EQ(z, 2);
}

TEST(Gmpfrxx, array_growth_moves_the_items)
{
  // MemArray reallocates its storage by move assignment: growing an Array of
  // big rationals copies no limb. The values are built before counting.
  std::vector<mpq_class> values(100, big_rational());
  Array<mpq_class> a;
  Gmp_Alloc_Counter counter;
  for (auto & v : values)
    a.append(std::move(v));
  EXPECT_EQ(counter.num_reallocs(), 0);
  EXPECT_EQ(a.size(), 100u);
  EXPECT_EQ(a[99], big_rational());
}

TEST(Gmpfrxx, list_append_moves_the_items)
{
  std::vector<mpq_class> values(100, big_rational());
  DynList<mpq_class> l;
  Gmp_Alloc_Counter counter;
  for (auto & v : values)
    l.append(std::move(v));
  EXPECT_EQ(counter.num_reallocs(), 0);
  EXPECT_EQ(l.size(), 100u);
  EXPECT_EQ(l.get_last(), big_rational());
}

TEST(Gmpfrxx, sorting_moves_the_items)
{
  Array<mpq_class> a;
  for (int i = 99; i >= 0; --i)
    a.append(big_rational() + i);
  Gmp_Alloc_Counter counter;
  quicksort_op(a);
  EXPECT_EQ(counter.num_reallocs(), 0);
  for (int i = 0; i < 100; ++i)
    EXPECT_EQ(a[i], big_rational() + i);
}
