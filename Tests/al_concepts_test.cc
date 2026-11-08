
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
 * @file al_concepts_test.cc
 * @brief The concepts of al-domain.H and al-vector.H: the keys and the
 * numbers that AlDomain, Vector and Matrix take, and what each member asks
 * of them.
 */

#include <gtest/gtest.h>

#include <gmpxx.h>

#include <string>
#include <type_traits>

#include <al-matrix.H>

using namespace Aleph;

namespace
{
  // A key without <: hashed by its bytes and compared with ==
  struct Unordered
  {
    int x = 0;
    bool operator ==(const Unordered &) const = default;
  };

  // A key without ==
  struct NoEquality
  {
    int x = 0;
  };

  // A key without a default constructor
  struct NoDefault
  {
    int x;
    NoDefault(int v) : x(v) {}
    bool operator ==(const NoDefault &) const = default;
    bool operator <(const NoDefault & o) const { return x < o.x; }
  };

  // A key that cannot be copied
  struct NoCopy
  {
    int x = 0;
    NoCopy() = default;
    NoCopy(const NoCopy &) = delete;
    NoCopy & operator =(const NoCopy &) = delete;
    bool operator ==(const NoCopy &) const = default;
    bool operator <(const NoCopy & o) const { return x < o.x; }
  };

  // A number that adds, subtracts and multiplies, but does not divide
  struct NoDivision
  {
    double v = 0;
    NoDivision() = default;
    NoDivision(int i) : v(i) {}
    NoDivision(double d) : v(d) {}
    NoDivision operator -() const { return NoDivision(-v); }
    NoDivision & operator +=(const NoDivision & o) { v += o.v; return *this; }
    NoDivision & operator -=(const NoDivision & o) { v -= o.v; return *this; }
    NoDivision & operator *=(const NoDivision & o) { v *= o.v; return *this; }
    friend NoDivision operator +(NoDivision a, const NoDivision & b) { return a += b; }
    friend NoDivision operator -(NoDivision a, const NoDivision & b) { return a -= b; }
    friend NoDivision operator *(NoDivision a, const NoDivision & b) { return a *= b; }
    friend bool operator ==(const NoDivision & a, const NoDivision & b) { return a.v == b.v; }
    friend bool operator <(const NoDivision & a, const NoDivision & b) { return a.v < b.v; }
    friend bool operator <=(const NoDivision & a, const NoDivision & b) { return a.v <= b.v; }
    friend bool operator >=(const NoDivision & a, const NoDivision & b) { return a.v >= b.v; }
  };

  // A number that only compares: an entry can be set and read, nothing more
  struct OnlyCompares
  {
    double v = 0;
    OnlyCompares() = default;
    OnlyCompares(int i) : v(i) {}
    OnlyCompares(double d) : v(d) {}
    OnlyCompares operator -() const { return OnlyCompares(-v); }
    friend bool operator <(const OnlyCompares & a, const OnlyCompares & b) { return a.v < b.v; }
    friend bool operator <=(const OnlyCompares & a, const OnlyCompares & b) { return a.v <= b.v; }
    friend bool operator >=(const OnlyCompares & a, const OnlyCompares & b) { return a.v >= b.v; }
  };

  // A requires-expression outside a template is ill-formed where its
  // requirements are not valid, instead of false: the checks go through
  // these concepts, whose parameters make them dependent
  template <class K> concept makes_a_domain = requires { typename AlDomain<K>; };
  template <class K, class N> concept makes_a_vector = requires { typename Vector<K, N>; };
  template <class R, class C, class N>
  concept makes_a_matrix = requires { typename Matrix<R, C, N>; };
  template <class D> concept has_keys = requires (const D & d) { d.keys(); };
  template <class D> concept has_to_list = requires (const D & d) { d.to_list(); };
  template <class D> concept has_to_str = requires (const D & d) { d.to_str(); };
  template <class D> concept has_print = requires (const D & d) { d.print(); };
  template <class M>
  concept has_row_domain_list = requires (const M & m) { m.row_domain_list(); };
  template <class V, class N> concept can_divide = requires (const V & v, const N & x) { v / x; };
  template <class V, class N>
  concept can_divide_by_scalar = requires (V & v, const N & x) { v.divide_by_scalar(x); };
  template <class V> concept can_add = requires (const V & v) { v + v; };
  template <class V> concept can_multiply = requires (const V & v) { v * v; };
  template <class V> concept can_compare = requires (const V & v) { v == v; };
  template <class V> concept can_negate = requires (const V & v) { -v; };
  template <class M, class V>
  concept can_multiply_a_vector = requires (const M & m, const V & v) { m * v; };
}

TEST(AlConcepts, TheKeysOfADomain)
{
  static_assert(AlDomainKey<int> and AlDomainKey<char> and AlDomainKey<std::string>);
  static_assert(AlDomainKey<Unordered>);
  static_assert(not AlDomainKey<NoEquality>);
  static_assert(not AlDomainKey<NoDefault>);
  static_assert(not AlDomainKey<NoCopy>);

  static_assert(AlOrderedDomainKey<int> and AlOrderedDomainKey<std::string>);
  static_assert(not AlOrderedDomainKey<Unordered>);

  static_assert(AlStreamable<int> and AlStreamable<std::string>);
  static_assert(not AlStreamable<Unordered>);

  // The classes take only those keys
  static_assert(makes_a_domain<Unordered>);
  static_assert(not makes_a_domain<NoEquality>);
  static_assert(not makes_a_vector<NoEquality, double>);
  static_assert(not makes_a_matrix<int, NoEquality, double>);
  static_assert(not makes_a_matrix<NoDefault, int, double>);
  SUCCEED();
}

TEST(AlConcepts, TheNumbersOfAVector)
{
  static_assert(AlNumber<int> and AlNumber<float> and AlNumber<double>
                and AlNumber<long double> and AlNumber<mpq_class>);
  static_assert(AlNumber<NoDivision> and AlNumber<OnlyCompares>);
  static_assert(not AlNumber<std::string>);
  static_assert(not AlNumber<Unordered>);

  static_assert(AlAdditiveNumber<double> and AlAdditiveNumber<mpq_class>
                and AlAdditiveNumber<NoDivision>);
  static_assert(not AlAdditiveNumber<OnlyCompares>);

  static_assert(AlMultiplicativeNumber<double> and AlMultiplicativeNumber<mpq_class>
                and AlMultiplicativeNumber<NoDivision>);
  static_assert(not AlMultiplicativeNumber<OnlyCompares>);

  static_assert(AlDivisibleNumber<double> and AlDivisibleNumber<int>
                and AlDivisibleNumber<mpq_class>);
  static_assert(not AlDivisibleNumber<NoDivision>);

  // The classes take only those numbers
  static_assert(not makes_a_vector<int, std::string>);
  static_assert(not makes_a_matrix<int, int, std::string>);
  SUCCEED();
}

TEST(AlConcepts, KeysWithoutOrderSetAndAddEntries)
{
  // A key without < makes a domain and vectors that set, read and add
  // entries; what takes the keys in their order is not there
  AlDomain<Unordered> d;
  (void) d.insert(Unordered{1});
  (void) d.insert(Unordered{2});
  Vector<Unordered, double> v(d), w(d);
  v.set_entry(Unordered{1}, 2.0);
  w.set_entry(Unordered{1}, 3.0);
  w.set_entry(Unordered{2}, 1.0);
  v += w;
  EXPECT_EQ(v.get_entry(Unordered{1}), 5.0);
  EXPECT_EQ(v.get_entry(Unordered{2}), 1.0);
  EXPECT_EQ(v * w, 16.0);   // 5*3 + 1*1
  v[Unordered{2}] = 4.0;
  EXPECT_EQ(v(Unordered{2}), 4.0);

  static_assert(has_keys<AlDomain<int>> and has_to_str<Vector<int, double>>);
  static_assert(not has_keys<AlDomain<Unordered>>);
  static_assert(not has_to_list<AlDomain<Unordered>>);
  static_assert(not has_to_list<Vector<Unordered, double>>);
  static_assert(not has_to_str<Vector<Unordered, double>>);
  static_assert(not has_print<Vector<Unordered, double>>);
  static_assert(not std::is_constructible_v<Vector<Unordered, double>,
                                            const AlDomain<Unordered> &,
                                            const DynList<double> &>);

  Matrix<Unordered, Unordered, double> m(d, d);
  m.set_entry(Unordered{1}, Unordered{2}, 7.0);
  EXPECT_EQ(m.get_row_vector(Unordered{1}).get_entry(Unordered{2}), 7.0);
  EXPECT_EQ(m.transpose().get_entry(Unordered{2}, Unordered{1}), 7.0);
  static_assert(not has_row_domain_list<Matrix<Unordered, Unordered, double>>);
  static_assert(not has_to_str<Matrix<Unordered, Unordered, double>>);
}

TEST(AlConcepts, NumbersWithoutDivisionAddAndMultiply)
{
  // A number without / adds and multiplies; the division is not there
  const IntRange d(3);
  Vector<int, NoDivision> v(d), w(d);
  v.set_entry(0, NoDivision(2.0));
  w.set_entry(0, NoDivision(3.0));
  w.set_entry(2, NoDivision(1.0));
  EXPECT_EQ((v + w).get_entry(0).v, 5.0);
  EXPECT_EQ((w - v).get_entry(0).v, 1.0);
  EXPECT_EQ((v * w).v, 6.0);
  EXPECT_EQ((NoDivision(2.0) * w).get_entry(2).v, 2.0);

  static_assert(can_divide<Vector<int, double>, double>);
  static_assert(not can_divide<Vector<int, NoDivision>, NoDivision>);
  static_assert(not can_divide_by_scalar<Vector<int, NoDivision>, NoDivision>);
  // to_str() writes the numbers with std::to_string(), and print() with <<
  static_assert(not has_to_str<Vector<int, NoDivision>>);
  static_assert(not has_print<Vector<int, NoDivision>>);
  static_assert(has_to_list<Vector<int, NoDivision>>);
}

TEST(AlConcepts, NumbersThatOnlyCompareSetAndReadEntries)
{
  // A number without arithmetic makes a vector and a matrix whose entries
  // are set and read; sums, products and comparisons are not there
  const IntRange d(2);
  Vector<int, OnlyCompares> v(d);
  v.set_entry(1, OnlyCompares(4.0));
  EXPECT_EQ(v.get_entry(1).v, 4.0);
  EXPECT_EQ(v.get_entry(0).v, 0.0);
  v.set_entry(1, OnlyCompares(0.0));
  EXPECT_EQ(v.search_entry(1), nullptr);

  static_assert(can_add<Vector<int, double>> and can_multiply<Vector<int, double>>
                and can_compare<Vector<int, double>> and can_negate<Vector<int, double>>);
  static_assert(not can_add<Vector<int, OnlyCompares>>);
  static_assert(not can_multiply<Vector<int, OnlyCompares>>);
  static_assert(not can_compare<Vector<int, OnlyCompares>>);
  static_assert(not can_negate<Vector<int, OnlyCompares>>);

  Matrix<int, int, OnlyCompares> m(d, d);
  m.set_entry(0, 1, OnlyCompares(2.0));
  EXPECT_EQ(m.get_entry(0, 1).v, 2.0);
  static_assert(not can_add<Matrix<int, int, OnlyCompares>>);
  static_assert(not can_multiply_a_vector<Matrix<int, int, OnlyCompares>,
                                        Vector<int, OnlyCompares>>);
}

TEST(AlConcepts, ExactRationalsComputeExactly)
{
  // mpq_class is an AlNumber: the entries add, multiply and divide exactly
  const IntRange d(2);
  Vector<int, mpq_class> v(d), w(d);
  v.set_entry(0, mpq_class(1, 3));
  w.set_entry(0, mpq_class(1, 6));
  w.set_entry(1, mpq_class(2, 5));
  EXPECT_EQ((v + w).get_entry(0), mpq_class(1, 2));
  EXPECT_EQ(v * w, mpq_class(1, 18));
  EXPECT_EQ((w / mpq_class(2)).get_entry(1), mpq_class(1, 5));

  Matrix<int, int, mpq_class> m(d, d);
  m.set_entry(0, 0, mpq_class(1, 2));
  m.set_entry(1, 0, mpq_class(3));
  const Vector<int, mpq_class> p = m * v;
  EXPECT_EQ(p.get_entry(0), mpq_class(1, 6));
  EXPECT_EQ(p.get_entry(1), mpq_class(1));

  // to_str() writes the numbers with std::to_string(), which has none for
  // mpq_class
  static_assert(not has_to_str<Vector<int, mpq_class>>);
}
