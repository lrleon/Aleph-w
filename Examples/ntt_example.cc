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

/** @file ntt_example.cc
 *  @brief Demonstrates the industrial NTT and CRT exact APIs.
 */

# include <algorithm>
# include <iostream>
# include <string>

# include <ntt.H>
# include <print_rule.H>
# include <thread_pool.H>

using namespace Aleph;
using namespace std;

namespace
{
  string
  to_string_u128(__uint128_t value)
  {
    if (value == 0)
      return "0";

    string digits;
    while (value > 0)
      {
        digits.push_back(static_cast<char>('0' + static_cast<unsigned>(value % 10)));
        value /= 10;
      }

    reverse(digits.begin(), digits.end());
    return digits;
  }

  Array<uint64_t>
  decimal_digits_from_string(const string & value)
  {
    Array<uint64_t> digits;
    digits.reserve(value.size());
    for (size_t i = value.size(); i > 0; --i)
      digits.append(static_cast<uint64_t>(value[i - 1] - '0'));

    while (digits.size() > 1 and digits.get_last() == 0)
      static_cast<void>(digits.remove_last());
    return digits.is_empty() ? Array<uint64_t>({0}) : digits;
  }

  string
  decimal_digits_to_string(const Array<uint64_t> & digits)
  {
    if (digits.is_empty())
      return "0";

    size_t used = digits.size();
    while (used > 1 and digits[used - 1] == 0)
      --used;

    string value;
    value.reserve(used);
    for (size_t i = used; i > 0; --i)
      value.push_back(static_cast<char>('0' + digits[i - 1]));
    return value;
  }

  bool
  equal_digits(const Array<uint64_t> & lhs,
               const Array<uint64_t> & rhs)
  {
    if (lhs.size() != rhs.size())
      return false;

    for (size_t i = 0; i < lhs.size(); ++i)
      if (lhs[i] != rhs[i])
        return false;

    return true;
  }

  void
  print_coeffs(const char * const name,
               const Array<uint64_t> & poly)
  {
    cout << name << "(x) = ";
    for (size_t i = 0; i < poly.size(); ++i)
      {
        if (i > 0)
          cout << " + ";
        cout << poly[i];
        if (i > 0)
          cout << "x^" << i;
      }
    cout << '\n';
  }

  void
  print_batch(const Array<Array<uint64_t>> & batch)
  {
    for (size_t i = 0; i < batch.size(); ++i)
      {
        cout << "  item[" << i << "] = [";
        for (size_t j = 0; j < batch[i].size(); ++j)
          cout << batch[i][j] << (j + 1 == batch[i].size() ? "" : ", ");
        cout << "]\n";
      }
  }
}

int
main()
{
  using DefaultNTT = NTT<>;

  cout << "\n=== Number Theoretic Transform (NTT) ===\n\n";
  cout << "Active SIMD backend: " << DefaultNTT::simd_backend_name() << '\n';
  cout << "  AVX2 available: "
       << (DefaultNTT::avx2_dispatch_available() ? "yes" : "no") << '\n';
  cout << "  NEON available: "
       << (DefaultNTT::neon_dispatch_available() ? "yes" : "no") << "\n\n";

  cout << "[1] Static polynomial multiplication\n";
  print_rule();
  Array<uint64_t> a = {1, 2, 3, 4};
  Array<uint64_t> b = {5, 6, 7};
  const Array<uint64_t> product = DefaultNTT::multiply(a, b);
  print_coeffs("A", a);
  print_coeffs("B", b);
  print_coeffs("A * B mod 998244353", product);
  cout << '\n';

  cout << "[2] Exact CRT multiplication beyond a single modulus\n";
  print_rule();
  const Array<uint64_t> exact_lhs = {
    1000000000000ULL,
    DefaultNTT::mod + 77ULL,
    1234567890123ULL
  };
  const Array<uint64_t> exact_rhs = {
    777777777777ULL,
    42ULL,
    DefaultNTT::mod + 11ULL
  };
  const Array<__uint128_t> exact_product = NTTExact::multiply(exact_lhs, exact_rhs);
  cout << "Exact coefficients with CRT:\n  [";
  for (size_t i = 0; i < exact_product.size(); ++i)
    cout << to_string_u128(exact_product[i])
         << (i + 1 == exact_product.size() ? "" : ", ");
  cout << "]\n\n";

  cout << "[3] Reusable arbitrary-size plan (Bluestein)\n";
  print_rule();
  DefaultNTT::Plan plan(7);
  Array<uint64_t> signal = {3, 1, 4, 1, 5, 9, 2};
  Array<uint64_t> spectrum = signal;
  plan.transform(spectrum, false);
  cout << "Forward transform:\n  [";
  for (size_t i = 0; i < spectrum.size(); ++i)
    cout << spectrum[i] << (i + 1 == spectrum.size() ? "" : ", ");
  cout << "]\n";
  plan.transform(spectrum, true);
  cout << "Inverse transform recovers:\n  [";
  for (size_t i = 0; i < spectrum.size(); ++i)
    cout << spectrum[i] << (i + 1 == spectrum.size() ? "" : ", ");
  cout << "]\n\n";

  cout << "[4] Batch transforms\n";
  print_rule();
  Array<Array<uint64_t>> batch = {
      Array<uint64_t>({1, 0, 0, 0, 0, 0, 0}),
      Array<uint64_t>({0, 1, 0, 0, 0, 0, 0}),
      Array<uint64_t>({1, 1, 1, 1, 0, 0, 0})
  };
  plan.transform_batch(batch, false);
  print_batch(batch);
  cout << '\n';

  cout << "[5] Parallel multiplication with ThreadPool\n";
  print_rule();
  ThreadPool pool(4);
  const Array<uint64_t> parallel_product =
      DefaultNTT::pmultiply(pool, a, b);
  print_coeffs("Parallel A * B", parallel_product);

  const Array<__uint128_t> parallel_exact =
      NTTExact::pmultiply(pool, exact_lhs, exact_rhs);
  cout << "Parallel exact CRT product first coefficient: "
       << to_string_u128(parallel_exact[0]) << '\n';

  cout << "\n[6] Polynomial algebra modulo 998244353\n";
  print_rule();
  const Array<uint64_t> unit_series = {1, 4, 7, 2};
  const Array<uint64_t> log_series = DefaultNTT::poly_log(unit_series, 6);
  const Array<uint64_t> exp_series = DefaultNTT::poly_exp(log_series, 6);
  const Array<uint64_t> power_series = DefaultNTT::poly_power(unit_series, 3, 6);
  cout << "log(1 + 4x + 7x^2 + 2x^3) mod x^6:\n  [";
  for (size_t i = 0; i < log_series.size(); ++i)
    cout << log_series[i] << (i + 1 == log_series.size() ? "" : ", ");
  cout << "]\n";
  cout << "exp(log(series)) recovers:\n  [";
  for (size_t i = 0; i < exp_series.size(); ++i)
    cout << exp_series[i] << (i + 1 == exp_series.size() ? "" : ", ");
  cout << "]\n";
  cout << "series^3 mod x^6:\n  [";
  for (size_t i = 0; i < power_series.size(); ++i)
    cout << power_series[i] << (i + 1 == power_series.size() ? "" : ", ");
  cout << "]\n";

  const Array<uint64_t> interp_points = {0, 1, 2, 5};
  const Array<uint64_t> interp_values =
      DefaultNTT::multipoint_eval(Array<uint64_t>({9, 7, 5, 3}), interp_points);
  const Array<uint64_t> recovered_poly =
      DefaultNTT::interpolate(interp_points, interp_values);
  cout << "Interpolation from sampled values:\n  [";
  for (size_t i = 0; i < recovered_poly.size(); ++i)
    cout << recovered_poly[i]
         << (i + 1 == recovered_poly.size() ? "" : ", ");
  cout << "]\n";

  cout << "\n[7] Big integer multiplication in base 10\n";
  print_rule();
  const string decimal_a = "12345678901234567890";
  const string decimal_b = "98765432109876543210";
  const Array<uint64_t> bigint_a = decimal_digits_from_string(decimal_a);
  const Array<uint64_t> bigint_b = decimal_digits_from_string(decimal_b);
  const Array<uint64_t> bigint_product =
      DefaultNTT::bigint_multiply<10>(bigint_a, bigint_b);
  const Array<uint64_t> bigint_parallel =
      DefaultNTT::pbigint_multiply<10>(pool, bigint_a, bigint_b);
  cout << decimal_a << " * " << decimal_b << " =\n  "
       << decimal_digits_to_string(bigint_product) << '\n';
  cout << "Parallel product matches sequential: "
       << (equal_digits(bigint_parallel, bigint_product) ? "yes" : "no") << '\n';

  cout << "\n[8] Negacyclic multiplication modulo (x^4 + 1)\n";
  print_rule();
  const Array<uint64_t> neg_lhs = {1, 1, 1, 1};
  const Array<uint64_t> neg_rhs = {1, 1, 0, 0};
  const Array<uint64_t> neg_product =
      DefaultNTT::negacyclic_multiply(neg_lhs, neg_rhs);
  print_coeffs("A", neg_lhs);
  print_coeffs("B", neg_rhs);
  print_coeffs("A * B mod (x^4 + 1)", neg_product);

  cout << "\nDone.\n";
  return 0;
}
