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

/** @file ntt_test.cc
 *  @brief Tests for the industrial NTT core, CRT exact multiplier, and
 *         high-level algebra helpers.
 */

# include <gtest/gtest.h>

# include <algorithm>
# include <chrono>
# include <cstdlib>
# include <random>
# include <string>

# include <modular_arithmetic.H>
# include <ntt.H>
# include <thread_pool.H>

using namespace Aleph;

namespace
{
  void
  set_env_var(const char * const name,
              const char * const value)
  {
# if defined(_WIN32)
    const int rc = _putenv_s(name, value);
# else
    const int rc = setenv(name, value, 1);
# endif
    if (rc != 0)
      std::abort();
  }

  void
  unset_env_var(const char * const name)
  {
# if defined(_WIN32)
    const int rc = _putenv_s(name, "");
# else
    const int rc = unsetenv(name);
# endif
    if (rc != 0)
      std::abort();
  }

  class ScopedEnvVar
  {
    std::string name_;
    bool had_old_ = false;
    std::string old_value_;

  public:
    ScopedEnvVar(const char * const name,
                 const char * const value)
      : name_(name)
    {
      if (const char *current = std::getenv(name); current != nullptr)
        {
          had_old_ = true;
          old_value_ = current;
        }

      set_env_var(name, value);
    }

    ~ScopedEnvVar()
    {
      if (had_old_)
        set_env_var(name_.c_str(), old_value_.c_str());
      else
        unset_env_var(name_.c_str());
    }
  };

  template <typename NTTType>
  Array<uint64_t>
  modded_copy(const Array<uint64_t> & input)
  {
    Array<uint64_t> output = Array<uint64_t>::create(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      output(i) = input[i] % NTTType::mod;
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_multiply(const Array<uint64_t> & a,
                 const Array<uint64_t> & b)
  {
    if (a.is_empty() or b.is_empty())
      return {};

    Array<uint64_t> output = Array<uint64_t>::create(a.size() + b.size() - 1);
    for (size_t i = 0; i < output.size(); ++i)
      output(i) = 0;

    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        {
          const uint64_t term =
              mod_mul(a[i] % NTTType::mod, b[j] % NTTType::mod, NTTType::mod);
          output(i + j) = static_cast<uint64_t>(
              (static_cast<__uint128_t>(output[i + j]) + term) % NTTType::mod);
        }

    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_transform(const Array<uint64_t> & input,
                  const bool invert)
  {
    if (input.is_empty())
      return {};

    const size_t n = input.size();
    const uint64_t mod = NTTType::mod;
    const uint64_t root =
        invert ?
          mod_inv(NTTType::primitive_root_of_unity(n), mod) :
          NTTType::primitive_root_of_unity(n);
    const uint64_t inv_n = invert ? mod_inv(static_cast<uint64_t>(n), mod) : 1;

    Array<uint64_t> output = Array<uint64_t>::create(n);
    for (size_t k = 0; k < n; ++k)
      {
        uint64_t sum = 0;
        for (size_t j = 0; j < n; ++j)
          {
            const uint64_t exponent = static_cast<uint64_t>(
                (static_cast<__uint128_t>(j) * k) % n);
            const uint64_t twiddle = mod_exp(root, exponent, mod);
            const uint64_t term = mod_mul(input[j] % mod, twiddle, mod);
            sum = static_cast<uint64_t>(
                (static_cast<__uint128_t>(sum) + term) % mod);
          }
        output(k) = invert ? mod_mul(sum, inv_n, mod) : sum;
      }

    return output;
  }

  std::string
  u128_to_string(__uint128_t value)
  {
    if (value == 0)
      return "0";

    std::string digits;
    while (value > 0)
      {
        const auto digit = static_cast<unsigned>(value % 10);
        digits.push_back(static_cast<char>('0' + digit));
        value /= 10;
      }

    std::reverse(digits.begin(), digits.end());
    return digits;
  }

  Array<__uint128_t>
  naive_exact_multiply(const Array<uint64_t> & a,
                       const Array<uint64_t> & b)
  {
    if (a.is_empty() or b.is_empty())
      return {};

    Array<__uint128_t> output = Array<__uint128_t>::create(a.size() + b.size() - 1);
    for (size_t i = 0; i < output.size(); ++i)
      output(i) = 0;

    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        output(i + j) += static_cast<__uint128_t>(a[i]) * b[j];

    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_negacyclic(const Array<uint64_t> & a,
                   const Array<uint64_t> & b)
  {
    if (a.size() != b.size())
      throw std::invalid_argument("naive_negacyclic: mismatched sizes");

    Array<uint64_t> output = Array<uint64_t>::create(a.size());
    for (size_t i = 0; i < output.size(); ++i)
      output(i) = 0;

    for (size_t i = 0; i < a.size(); ++i)
      for (size_t j = 0; j < b.size(); ++j)
        {
          const uint64_t term =
              mod_mul(a[i] % NTTType::mod, b[j] % NTTType::mod, NTTType::mod);
          const size_t pos = i + j;
          const size_t slot = pos < a.size() ? pos : pos - a.size();
          if (pos < a.size())
            output(slot) = static_cast<uint64_t>(
                (static_cast<__uint128_t>(output[slot]) + term) % NTTType::mod);
          else
            output(slot) = output[slot] >= term ?
                output[slot] - term :
                NTTType::mod - (term - output[slot]);
        }

    return output;
  }

  template <uint64_t Base>
  Array<uint64_t>
  normalize_bigint_digits(const Array<uint64_t> & input)
  {
    Array<uint64_t> output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      output.append(input[i]);

    while (not output.is_empty() and output.get_last() == 0)
      static_cast<void>(output.remove_last());

    if (output.is_empty())
      {
        output = Array<uint64_t>::create(1);
        output(0) = 0;
      }

    return output;
  }

  Array<uint64_t>
  decimal_digits_from_string(const std::string & value)
  {
    if (value.empty())
      return Array<uint64_t>({0});

    Array<uint64_t> digits;
    digits.reserve(value.size());
    for (size_t i = value.size(); i > 0; --i)
      {
        const char c = value[i - 1];
        if (c < '0' or c > '9')
          throw std::invalid_argument("decimal_digits_from_string: non-digit");
        digits.append(static_cast<uint64_t>(c - '0'));
      }

    return normalize_bigint_digits<10>(digits);
  }

  template <uint64_t Base>
  Array<uint64_t>
  naive_bigint_multiply(const Array<uint64_t> & a,
                        const Array<uint64_t> & b)
  {
    const Array<uint64_t> lhs = normalize_bigint_digits<Base>(a);
    const Array<uint64_t> rhs = normalize_bigint_digits<Base>(b);

    if ((lhs.size() == 1 and lhs[0] == 0)
        or (rhs.size() == 1 and rhs[0] == 0))
      return Array<uint64_t>({0});

    Array<__uint128_t> coeffs = Array<__uint128_t>::create(lhs.size() + rhs.size() - 1);
    for (size_t i = 0; i < coeffs.size(); ++i)
      coeffs(i) = 0;

    for (size_t i = 0; i < lhs.size(); ++i)
      for (size_t j = 0; j < rhs.size(); ++j)
        coeffs(i + j) += static_cast<__uint128_t>(lhs[i]) * rhs[j];

    Array<uint64_t> output;
    output.reserve(coeffs.size() + 2);
    __uint128_t carry = 0;
    for (size_t i = 0; i < coeffs.size(); ++i)
      {
        const __uint128_t total = coeffs[i] + carry;
        output.append(static_cast<uint64_t>(total % Base));
        carry = total / Base;
      }

    while (carry > 0)
      {
        output.append(static_cast<uint64_t>(carry % Base));
        carry /= Base;
      }

    return normalize_bigint_digits<Base>(output);
  }

  Array<uint64_t>
  all_nines_square_digits(const size_t n)
  {
    if (n == 0)
      return Array<uint64_t>({0});

    Array<uint64_t> output = Array<uint64_t>::create(2 * n);
    output(0) = 1;
    for (size_t i = 1; i < n; ++i)
      output(i) = 0;
    output(n) = 8;
    for (size_t i = n + 1; i < output.size(); ++i)
      output(i) = 9;
    return output;
  }

  template <typename NTTType>
  void
  trim_trailing_zeros_mod(Array<uint64_t> & poly)
  {
    while (not poly.is_empty() and poly.get_last() % NTTType::mod == 0)
      static_cast<void>(poly.remove_last());
  }

  template <typename NTTType>
  Array<uint64_t>
  normalize_mod_poly(const Array<uint64_t> & input)
  {
    Array<uint64_t> output;
    output.reserve(input.size());
    for (size_t i = 0; i < input.size(); ++i)
      output.append(input[i] % NTTType::mod);
    trim_trailing_zeros_mod<NTTType>(output);
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  series_prefix_mod(const Array<uint64_t> & input,
                    const size_t n)
  {
    Array<uint64_t> output = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      output(i) = i < input.size() ? input[i] % NTTType::mod : 0;
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  poly_derivative_ref(const Array<uint64_t> & coeffs)
  {
    if (coeffs.size() <= 1)
      return {};

    Array<uint64_t> output = Array<uint64_t>::create(coeffs.size() - 1);
    for (size_t i = 1; i < coeffs.size(); ++i)
      output(i - 1) = mod_mul(coeffs[i] % NTTType::mod,
                              static_cast<uint64_t>(i) % NTTType::mod,
                              NTTType::mod);
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  poly_integral_ref(const Array<uint64_t> & coeffs,
                    const size_t n)
  {
    Array<uint64_t> output = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      output(i) = 0;

    for (size_t i = 0; i < coeffs.size() and i + 1 < n; ++i)
      output(i + 1) = mod_mul(coeffs[i] % NTTType::mod,
                              mod_inv(static_cast<uint64_t>(i + 1),
                                      NTTType::mod),
                              NTTType::mod);
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_multiply_trunc(const Array<uint64_t> & a,
                       const Array<uint64_t> & b,
                       const size_t n)
  {
    return series_prefix_mod<NTTType>(naive_multiply<NTTType>(a, b), n);
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_poly_inverse(const Array<uint64_t> & coeffs,
                     const size_t n)
  {
    if (n == 0)
      return {};

    Array<uint64_t> f = series_prefix_mod<NTTType>(coeffs, n);
    Array<uint64_t> g = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      g(i) = 0;

    const uint64_t inv0 = mod_inv(f[0], NTTType::mod);
    g(0) = inv0;
    for (size_t i = 1; i < n; ++i)
      {
        uint64_t sum = 0;
        for (size_t j = 1; j <= i; ++j)
          {
            const uint64_t term =
                mod_mul(f[j], g[i - j], NTTType::mod);
            sum = static_cast<uint64_t>(
                (static_cast<__uint128_t>(sum) + term) % NTTType::mod);
          }

        g(i) = mod_mul(sum == 0 ? 0 : NTTType::mod - sum,
                       inv0, NTTType::mod);
      }

    return g;
  }

  template <typename NTTType>
  std::pair<Array<uint64_t>, Array<uint64_t>>
  naive_poly_divmod(const Array<uint64_t> & dividend,
                    const Array<uint64_t> & divisor)
  {
    Array<uint64_t> remainder = normalize_mod_poly<NTTType>(dividend);
    const Array<uint64_t> normalized_divisor = normalize_mod_poly<NTTType>(divisor);

    if (normalized_divisor.is_empty())
      throw std::invalid_argument("naive_poly_divmod: zero divisor");

    if (remainder.is_empty() or remainder.size() < normalized_divisor.size())
      return {{}, remainder};

    Array<uint64_t> quotient = Array<uint64_t>::create(
        remainder.size() - normalized_divisor.size() + 1);
    for (size_t i = 0; i < quotient.size(); ++i)
      quotient(i) = 0;

    const uint64_t inv_lead =
        mod_inv(normalized_divisor.get_last(), NTTType::mod);
    while (not remainder.is_empty()
           and remainder.size() >= normalized_divisor.size())
      {
        const size_t shift = remainder.size() - normalized_divisor.size();
        const uint64_t factor =
            mod_mul(remainder.get_last(), inv_lead, NTTType::mod);
        quotient(shift) = factor;

        for (size_t i = 0; i < normalized_divisor.size(); ++i)
          {
            const size_t pos = shift + i;
            const uint64_t term = mod_mul(factor,
                                          normalized_divisor[i],
                                          NTTType::mod);
            remainder(pos) =
                remainder[pos] >= term ?
                  remainder[pos] - term :
                  NTTType::mod - (term - remainder[pos]);
          }

        trim_trailing_zeros_mod<NTTType>(remainder);
      }

    return {normalize_mod_poly<NTTType>(quotient), remainder};
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_poly_log(const Array<uint64_t> & coeffs,
                 const size_t n)
  {
    if (n == 0)
      return {};

    const Array<uint64_t> derivative =
        poly_derivative_ref<NTTType>(series_prefix_mod<NTTType>(coeffs, n));
    const Array<uint64_t> inverse = naive_poly_inverse<NTTType>(coeffs, n - 1);
    return poly_integral_ref<NTTType>(
        naive_multiply_trunc<NTTType>(derivative, inverse, n - 1), n);
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_poly_exp(const Array<uint64_t> & coeffs,
                 const size_t n)
  {
    if (n == 0)
      return {};

    const Array<uint64_t> f = series_prefix_mod<NTTType>(coeffs, n);
    Array<uint64_t> g = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      g(i) = 0;
    g(0) = 1;

    for (size_t m = 1; m < n; ++m)
      {
        uint64_t sum = 0;
        for (size_t i = 1; i <= m; ++i)
          {
            const uint64_t weighted =
                mod_mul(static_cast<uint64_t>(i) % NTTType::mod,
                        f[i], NTTType::mod);
            const uint64_t term =
                mod_mul(weighted, g[m - i], NTTType::mod);
            sum = static_cast<uint64_t>(
                (static_cast<__uint128_t>(sum) + term) % NTTType::mod);
          }

        g(m) = mod_mul(sum,
                       mod_inv(static_cast<uint64_t>(m), NTTType::mod),
                       NTTType::mod);
      }

    return g;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_poly_power(const Array<uint64_t> & coeffs,
                   const uint64_t k,
                   const size_t n)
  {
    if (n == 0)
      return {};

    if (k == 0)
      {
        Array<uint64_t> output = Array<uint64_t>::create(n);
        for (size_t i = 0; i < n; ++i)
          output(i) = 0;
        output(0) = 1;
        return output;
      }

    Array<uint64_t> result = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      result(i) = 0;
    result(0) = 1;

    const Array<uint64_t> base = series_prefix_mod<NTTType>(coeffs, n);
    for (uint64_t iter = 0; iter < k; ++iter)
      result = naive_multiply_trunc<NTTType>(result, base, n);

    return result;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_multipoint_eval(const Array<uint64_t> & coeffs,
                        const Array<uint64_t> & points)
  {
    Array<uint64_t> output = Array<uint64_t>::create(points.size());
    for (size_t i = 0; i < points.size(); ++i)
      {
        uint64_t value = 0;
        const uint64_t x = points[i] % NTTType::mod;
        for (size_t j = coeffs.size(); j > 0; --j)
          {
            value = mod_mul(value, x, NTTType::mod);
            value = static_cast<uint64_t>(
                (static_cast<__uint128_t>(value)
                 + coeffs[j - 1] % NTTType::mod) % NTTType::mod);
          }
        output(i) = value;
      }
    return output;
  }

  template <typename NTTType>
  Array<uint64_t>
  naive_interpolate(const Array<uint64_t> & points,
                    const Array<uint64_t> & values)
  {
    Array<uint64_t> result;

    for (size_t i = 0; i < points.size(); ++i)
      {
        Array<uint64_t> basis = {1};
        uint64_t denom = 1;
        const uint64_t xi = points[i] % NTTType::mod;

        for (size_t j = 0; j < points.size(); ++j)
          if (i != j)
            {
              const uint64_t xj = points[j] % NTTType::mod;
              basis = naive_multiply<NTTType>(
                  basis,
                  Array<uint64_t>({xj == 0 ? 0 : NTTType::mod - xj, 1}));
              denom = mod_mul(denom,
                              xi >= xj ? xi - xj : NTTType::mod - (xj - xi),
                              NTTType::mod);
            }

        const uint64_t scale =
            mod_mul(values[i] % NTTType::mod,
                    mod_inv(denom, NTTType::mod),
                    NTTType::mod);
        Array<uint64_t> scaled = Array<uint64_t>::create(basis.size());
        for (size_t k = 0; k < basis.size(); ++k)
          scaled(k) = mod_mul(basis[k] % NTTType::mod,
                              scale, NTTType::mod);

        const size_t out_size = std::max(result.size(), scaled.size());
        Array<uint64_t> sum = Array<uint64_t>::create(out_size);
        for (size_t k = 0; k < out_size; ++k)
          {
            const uint64_t lhs = k < result.size() ? result[k] : 0;
            const uint64_t rhs = k < scaled.size() ? scaled[k] : 0;
            sum(k) = static_cast<uint64_t>(
                (static_cast<__uint128_t>(lhs) + rhs) % NTTType::mod);
          }
        result = normalize_mod_poly<NTTType>(sum);
      }

    return result;
  }

  void
  expect_equal_arrays(const Array<uint64_t> & lhs,
                      const Array<uint64_t> & rhs,
                      const char * const ctx)
  {
    ASSERT_EQ(lhs.size(), rhs.size()) << ctx;
    for (size_t i = 0; i < lhs.size(); ++i)
      EXPECT_EQ(lhs[i], rhs[i]) << ctx << " index=" << i;
  }

  void
  expect_equal_arrays_u128(const Array<__uint128_t> & lhs,
                           const Array<__uint128_t> & rhs,
                           const char * const ctx)
  {
    ASSERT_EQ(lhs.size(), rhs.size()) << ctx;
    for (size_t i = 0; i < lhs.size(); ++i)
      EXPECT_TRUE(lhs[i] == rhs[i])
        << ctx << " index=" << i
        << " lhs=" << u128_to_string(lhs[i])
        << " rhs=" << u128_to_string(rhs[i]);
  }
}

class NTTIndustrialTest : public ::testing::Test
{
protected:
  using DefaultNTT = NTT<>;
  using AlternateNTT = NTT<469762049ULL, 3ULL>;

  std::mt19937_64 rng_{123456789ULL};

  Array<uint64_t>
  random_poly(const size_t n)
  {
    Array<uint64_t> output = Array<uint64_t>::create(n);
    for (size_t i = 0; i < n; ++i)
      output(i) = rng_();
    return output;
  }
};

TEST_F(NTTIndustrialTest, ReportsSupportedSizesAndRoots)
{
  EXPECT_EQ(DefaultNTT::max_transform_size(), 1ULL << 23);
  EXPECT_TRUE(DefaultNTT::supports_size(1));
  EXPECT_TRUE(DefaultNTT::supports_size(8));
  EXPECT_TRUE(DefaultNTT::supports_size(7));
  EXPECT_TRUE(DefaultNTT::supports_size(14));
  EXPECT_TRUE(DefaultNTT::supports_size(17));
  EXPECT_TRUE(DefaultNTT::supports_size(1ULL << 23));
  EXPECT_FALSE(DefaultNTT::supports_size(0));
  EXPECT_FALSE(DefaultNTT::supports_size(3));
  EXPECT_FALSE(DefaultNTT::supports_size(10));
  EXPECT_FALSE(DefaultNTT::supports_size((1ULL << 23) + 2));

  const uint64_t root8 = DefaultNTT::primitive_root_of_unity(8);
  EXPECT_EQ(mod_exp(root8, 8, DefaultNTT::mod), 1ULL);
  EXPECT_NE(mod_exp(root8, 4, DefaultNTT::mod), 1ULL);

  const uint64_t root7 = DefaultNTT::primitive_root_of_unity(7);
  EXPECT_EQ(mod_exp(root7, 7, DefaultNTT::mod), 1ULL);
  EXPECT_NE(root7, 1ULL);
  EXPECT_EQ(DefaultNTT::primitive_root_of_unity(1), 1ULL);
}

TEST_F(NTTIndustrialTest, SimdBackendNameIsRecognized)
{
  ScopedEnvVar auto_mode("ALEPH_NTT_SIMD", "auto");

  const std::string backend = DefaultNTT::simd_backend_name();
  EXPECT_TRUE(backend == "scalar" or backend == "avx2" or backend == "neon");
  EXPECT_FALSE(DefaultNTT::avx2_dispatch_available()
               and DefaultNTT::neon_dispatch_available());
}

TEST_F(NTTIndustrialTest, SequentialRoundTripMatchesModuloProjection)
{
  const Array<size_t> sizes = {1, 2, 4, 8, 16, 64, 256, 1024};

  for (size_t i = 0; i < sizes.size(); ++i)
    for (size_t sample = 0; sample < 6; ++sample)
      {
        Array<uint64_t> values = random_poly(sizes[i]);
        const Array<uint64_t> expected = modded_copy<DefaultNTT>(values);

        DefaultNTT::transform(values, false);
        DefaultNTT::transform(values, true);

        expect_equal_arrays(values, expected, "Default sequential round trip");
      }
}

TEST_F(NTTIndustrialTest, ForcedSimdBackendsMatchScalarResults)
{
  const Array<uint64_t> input = random_poly(1 << 10);
  const Array<uint64_t> lhs = random_poly(300);
  const Array<uint64_t> rhs = random_poly(280);

  Array<uint64_t> scalar_transform = input;
  Array<uint64_t> scalar_product;
  {
    ScopedEnvVar scalar_mode("ALEPH_NTT_SIMD", "scalar");
    EXPECT_EQ(std::string(DefaultNTT::simd_backend_name()), "scalar");
    DefaultNTT::transform(scalar_transform, false);
    scalar_product = DefaultNTT::multiply(lhs, rhs);
  }

  {
    ScopedEnvVar avx2_mode("ALEPH_NTT_SIMD", "avx2");
    Array<uint64_t> avx2_transform = input;
    DefaultNTT::transform(avx2_transform, false);
    const Array<uint64_t> avx2_product = DefaultNTT::multiply(lhs, rhs);

    if (DefaultNTT::avx2_dispatch_available())
      EXPECT_EQ(std::string(DefaultNTT::simd_backend_name()), "avx2");
    else
      EXPECT_EQ(std::string(DefaultNTT::simd_backend_name()), "scalar");

    expect_equal_arrays(avx2_transform, scalar_transform, "AVX2 forced transform");
    expect_equal_arrays(avx2_product, scalar_product, "AVX2 forced multiply");
  }

  {
    ScopedEnvVar neon_mode("ALEPH_NTT_SIMD", "neon");
    Array<uint64_t> neon_transform = input;
    DefaultNTT::transform(neon_transform, false);
    const Array<uint64_t> neon_product = DefaultNTT::multiply(lhs, rhs);

    if (DefaultNTT::neon_dispatch_available())
      EXPECT_EQ(std::string(DefaultNTT::simd_backend_name()), "neon");
    else
      EXPECT_EQ(std::string(DefaultNTT::simd_backend_name()), "scalar");

    expect_equal_arrays(neon_transform, scalar_transform, "NEON forced transform");
    expect_equal_arrays(neon_product, scalar_product, "NEON forced multiply");
  }
}

TEST_F(NTTIndustrialTest, PolynomialEvalAndDivmodMatchNaiveReference)
{
  const Array<uint64_t> poly = {5, 3, 7, 11};
  const Array<uint64_t> points = {0, 1, 9, DefaultNTT::mod + 5ULL};
  const Array<uint64_t> expected_eval =
      naive_multipoint_eval<DefaultNTT>(poly, points);
  const Array<uint64_t> actual_eval =
      DefaultNTT::multipoint_eval(poly, points);
  expect_equal_arrays(actual_eval, expected_eval, "Multipoint eval");
  EXPECT_EQ(DefaultNTT::poly_eval(poly, 9), expected_eval[2]);

  const Array<uint64_t> dividend = {3, 4, 5, 6, 7};
  const Array<uint64_t> divisor = {1, 2, 1};
  const auto expected = naive_poly_divmod<DefaultNTT>(dividend, divisor);
  const auto actual = DefaultNTT::poly_divmod(dividend, divisor);
  expect_equal_arrays(actual.first, expected.first, "poly_divmod quotient");
  expect_equal_arrays(actual.second, expected.second, "poly_divmod remainder");
}

TEST_F(NTTIndustrialTest, PolynomialInverseMatchesNaiveReferenceAndIdentity)
{
  const Array<uint64_t> coeffs = {3, 4, 5, 6};
  const size_t n = 8;

  const Array<uint64_t> expected = naive_poly_inverse<DefaultNTT>(coeffs, n);
  const Array<uint64_t> actual = DefaultNTT::poly_inverse(coeffs, n);
  expect_equal_arrays(actual, expected, "poly_inverse");

  const Array<uint64_t> product = DefaultNTT::multiply(coeffs, actual);
  const Array<uint64_t> prefix = series_prefix_mod<DefaultNTT>(product, n);
  Array<uint64_t> identity = Array<uint64_t>::create(n);
  for (size_t i = 0; i < n; ++i)
    identity(i) = 0;
  identity(0) = 1;
  expect_equal_arrays(prefix, identity, "inverse identity");
}

TEST_F(NTTIndustrialTest, PolynomialLogExpAndPowerMatchNaiveReference)
{
  const Array<uint64_t> unit = {1, 4, 7, 2, 3};
  const size_t n = 7;

  const Array<uint64_t> expected_log = naive_poly_log<DefaultNTT>(unit, n);
  const Array<uint64_t> actual_log = DefaultNTT::poly_log(unit, n);
  expect_equal_arrays(actual_log, expected_log, "poly_log");

  const Array<uint64_t> recovered = DefaultNTT::poly_exp(actual_log, n);
  expect_equal_arrays(recovered, series_prefix_mod<DefaultNTT>(unit, n),
                      "exp(log(f))");

  const Array<uint64_t> expo_input = {0, 5, 3, 1, 4};
  const Array<uint64_t> expected_exp = naive_poly_exp<DefaultNTT>(expo_input, n);
  const Array<uint64_t> actual_exp = DefaultNTT::poly_exp(expo_input, n);
  expect_equal_arrays(actual_exp, expected_exp, "poly_exp");

  const Array<uint64_t> expected_power =
      naive_poly_power<DefaultNTT>(unit, 3, n);
  const Array<uint64_t> actual_power =
      DefaultNTT::poly_power(unit, 3, n);
  expect_equal_arrays(actual_power, expected_power, "poly_power");
}

TEST_F(NTTIndustrialTest, PolynomialSqrtRecoversKnownSquares)
{
  const size_t n = 8;

  const Array<uint64_t> root = series_prefix_mod<DefaultNTT>(
      Array<uint64_t>({5, 7, 3, 2}), n);
  const Array<uint64_t> square =
      naive_multiply_trunc<DefaultNTT>(root, root, n);
  const Array<uint64_t> recovered = DefaultNTT::poly_sqrt(square, n);
  expect_equal_arrays(recovered, root, "poly_sqrt non-zero constant");

  const Array<uint64_t> shifted_root = series_prefix_mod<DefaultNTT>(
      Array<uint64_t>({0, 5, 4, 1}), n);
  const Array<uint64_t> shifted_square =
      naive_multiply_trunc<DefaultNTT>(shifted_root, shifted_root, n);
  const Array<uint64_t> shifted_recovered =
      DefaultNTT::poly_sqrt(shifted_square, n);
  expect_equal_arrays(shifted_recovered, shifted_root,
                      "poly_sqrt shifted root");
}

TEST_F(NTTIndustrialTest, PolynomialInterpolationRoundTripMatchesNaiveReference)
{
  const Array<uint64_t> coeffs = {9, 7, 5, 3, 1};
  const Array<uint64_t> points = {0, 1, 2, 5, 11};
  const Array<uint64_t> values =
      naive_multipoint_eval<DefaultNTT>(coeffs, points);

  const Array<uint64_t> actual_values =
      DefaultNTT::multipoint_eval(coeffs, points);
  expect_equal_arrays(actual_values, values, "multipoint_eval");

  const Array<uint64_t> expected_poly =
      naive_interpolate<DefaultNTT>(points, values);
  const Array<uint64_t> actual_poly =
      DefaultNTT::interpolate(points, values);
  expect_equal_arrays(actual_poly, expected_poly, "interpolate");

  const Array<uint64_t> round_trip =
      DefaultNTT::multipoint_eval(actual_poly, points);
  expect_equal_arrays(round_trip, values, "interpolate/eval round trip");
}

TEST_F(NTTIndustrialTest, BigintMultiplyMatchesKnownProductsAndNaiveReference)
{
  const Array<uint64_t> decimal_lhs =
      decimal_digits_from_string("12345678901234567890");
  const Array<uint64_t> decimal_rhs =
      decimal_digits_from_string("98765432109876543210");
  const Array<uint64_t> decimal_expected =
      decimal_digits_from_string("1219326311370217952237463801111263526900");
  const Array<uint64_t> decimal_actual =
      DefaultNTT::bigint_multiply<10>(decimal_lhs, decimal_rhs);
  expect_equal_arrays(decimal_actual, decimal_expected, "decimal bigint product");

  const Array<uint64_t> base1000_lhs = {999, 123, 456, 7};
  const Array<uint64_t> base1000_rhs = {777, 1, 5};
  expect_equal_arrays(DefaultNTT::bigint_multiply<1000>(base1000_lhs, base1000_rhs),
                      naive_bigint_multiply<1000>(base1000_lhs, base1000_rhs),
                      "base-1000 bigint product");

  const Array<uint64_t> base2p15_lhs = {32767, 5, 1024, 77};
  const Array<uint64_t> base2p15_rhs = {32767, 7, 4096};
  expect_equal_arrays(DefaultNTT::bigint_multiply<(1ULL << 15)>(base2p15_lhs,
                                                                base2p15_rhs),
                      naive_bigint_multiply<(1ULL << 15)>(base2p15_lhs,
                                                          base2p15_rhs),
                      "base-2^15 bigint product");
}

TEST_F(NTTIndustrialTest, ParallelBigintMultiplyMatchesSequentialAndNormalizes)
{
  ThreadPool pool(4);

  const Array<uint64_t> lhs = {9, 9, 0, 0};
  const Array<uint64_t> rhs = {9, 9, 0};
  const Array<uint64_t> expected = {1, 0, 8, 9};

  const Array<uint64_t> sequential = DefaultNTT::bigint_multiply<10>(lhs, rhs);
  const Array<uint64_t> parallel =
      DefaultNTT::pbigint_multiply<10>(pool, lhs, rhs);
  expect_equal_arrays(sequential, expected, "normalized bigint product");
  expect_equal_arrays(parallel, sequential, "parallel bigint product");

  const Array<uint64_t> zero_left =
      DefaultNTT::bigint_multiply<10>(Array<uint64_t>(), rhs);
  const Array<uint64_t> zero_right =
      DefaultNTT::bigint_multiply<10>(lhs, Array<uint64_t>({0, 0, 0}));
  expect_equal_arrays(zero_left, Array<uint64_t>({0}), "empty bigint is zero");
  expect_equal_arrays(zero_right, Array<uint64_t>({0}), "zero bigint product");
}

TEST_F(NTTIndustrialTest, BigintMultiplyLargeDecimalPattern)
{
  const size_t digits = 4096;
  Array<uint64_t> nines = Array<uint64_t>::create(digits);
  for (size_t i = 0; i < digits; ++i)
    nines(i) = 9;

  const Array<uint64_t> actual = DefaultNTT::bigint_multiply<10>(nines, nines);
  const Array<uint64_t> expected = all_nines_square_digits(digits);
  expect_equal_arrays(actual, expected, "all-9 decimal square");
}

TEST_F(NTTIndustrialTest, NegacyclicMultiplyMatchesNaiveReference)
{
  const Array<size_t> sizes = {2, 4, 8, 1024};

  for (size_t i = 0; i < sizes.size(); ++i)
    {
      Array<uint64_t> lhs = random_poly(sizes[i]);
      Array<uint64_t> rhs = random_poly(sizes[i]);
      const Array<uint64_t> expected = naive_negacyclic<DefaultNTT>(lhs, rhs);
      const Array<uint64_t> actual = DefaultNTT::negacyclic_multiply(lhs, rhs);
      expect_equal_arrays(actual, expected, "negacyclic multiply");
    }

  const Array<uint64_t> demo_lhs = {1, 1, 1, 1};
  const Array<uint64_t> demo_rhs = {1, 1, 0, 0};
  const Array<uint64_t> expected_demo = {0, 2, 2, 2};
  expect_equal_arrays(DefaultNTT::negacyclic_multiply(demo_lhs, demo_rhs),
                      expected_demo,
                      "negacyclic demo product");
}

TEST_F(NTTIndustrialTest, BigintMillionDigitPatternIsGuardedByEnvironment)
{
  if (const char * env = std::getenv("ENABLE_PERF_TESTS");
      env == nullptr or std::string(env) != "1")
    {
      GTEST_SKIP() << "Set ENABLE_PERF_TESTS=1 to run million-digit bigint checks";
    }

  const size_t digits = 1000000;
  Array<uint64_t> nines = Array<uint64_t>::create(digits);
  for (size_t i = 0; i < digits; ++i)
    nines(i) = 9;

  const Array<uint64_t> product = DefaultNTT::bigint_multiply<10>(nines, nines);
  ASSERT_EQ(product.size(), 2 * digits);
  EXPECT_EQ(product[0], 1ULL);
  EXPECT_EQ(product[digits - 1], 0ULL);
  EXPECT_EQ(product[digits], 8ULL);
  EXPECT_EQ(product[product.size() - 1], 9ULL);
}

TEST_F(NTTIndustrialTest, AlternatePrimeRoundTripMatchesModuloProjection)
{
  const Array<size_t> sizes = {1, 2, 4, 32, 128};

  for (size_t i = 0; i < sizes.size(); ++i)
    for (size_t sample = 0; sample < 4; ++sample)
      {
        Array<uint64_t> values = random_poly(sizes[i]);
        const Array<uint64_t> expected = modded_copy<AlternateNTT>(values);

        AlternateNTT::transform(values, false);
        AlternateNTT::transform(values, true);

        expect_equal_arrays(values, expected, "Alternate sequential round trip");
      }
}

TEST_F(NTTIndustrialTest, BluesteinMatchesNaiveTransformOnSupportedSizes)
{
  const Array<size_t> sizes = {7, 14, 17, 119};

  for (size_t i = 0; i < sizes.size(); ++i)
    for (size_t sample = 0; sample < 3; ++sample)
      {
        Array<uint64_t> input = random_poly(sizes[i]);

        const Array<uint64_t> expected_forward =
            naive_transform<DefaultNTT>(input, false);
        const Array<uint64_t> expected_inverse =
            naive_transform<DefaultNTT>(expected_forward, true);

        Array<uint64_t> actual_forward = input;
        DefaultNTT::transform(actual_forward, false);
        expect_equal_arrays(actual_forward, expected_forward,
                            "Bluestein forward transform");

        DefaultNTT::transform(actual_forward, true);
        expect_equal_arrays(actual_forward, modded_copy<DefaultNTT>(input),
                            "Bluestein inverse round trip");
        expect_equal_arrays(expected_inverse, modded_copy<DefaultNTT>(input),
                            "Naive inverse round trip");
      }
}

TEST_F(NTTIndustrialTest, MultiplyMatchesNaiveConvolution)
{
  const Array<size_t> lhs_sizes = {1, 2, 3, 5, 8, 17, 32};
  const Array<size_t> rhs_sizes = {1, 2, 4, 7, 9, 17, 24};

  for (size_t i = 0; i < lhs_sizes.size(); ++i)
    for (size_t j = 0; j < rhs_sizes.size(); ++j)
      {
        Array<uint64_t> lhs = random_poly(lhs_sizes[i]);
        Array<uint64_t> rhs = random_poly(rhs_sizes[j]);
        const Array<uint64_t> expected = naive_multiply<DefaultNTT>(lhs, rhs);
        const Array<uint64_t> got = DefaultNTT::multiply(lhs, rhs);

        expect_equal_arrays(got, expected, "Static multiply");
      }
}

TEST_F(NTTIndustrialTest, PlanMatchesStaticTransformAndMultiply)
{
  typename DefaultNTT::Plan plan(64);

  Array<uint64_t> input = random_poly(64);
  Array<uint64_t> static_result = input;
  Array<uint64_t> plan_result = input;

  DefaultNTT::transform(static_result, false);
  plan.transform(plan_result, false);
  expect_equal_arrays(plan_result, static_result, "Plan forward transform");

  DefaultNTT::transform(static_result, true);
  plan.transform(plan_result, true);
  expect_equal_arrays(plan_result, static_result, "Plan inverse transform");

  Array<uint64_t> lhs = random_poly(20);
  Array<uint64_t> rhs = random_poly(17);
  const Array<uint64_t> expected = DefaultNTT::multiply(lhs, rhs);
  const Array<uint64_t> got = plan.multiply(lhs, rhs);
  expect_equal_arrays(got, expected, "Plan multiply");
}

TEST_F(NTTIndustrialTest, BatchTransformMatchesItemWiseTransform)
{
  typename DefaultNTT::Plan plan(16);
  Array<Array<uint64_t>> batch;
  Array<Array<uint64_t>> original;

  for (size_t i = 0; i < 4; ++i)
    {
      Array<uint64_t> item = random_poly(16);
      batch.append(item);
      original.append(item);
    }

  Array<Array<uint64_t>> expected = original;
  for (size_t i = 0; i < expected.size(); ++i)
    DefaultNTT::transform(expected(i), false);

  plan.transform_batch(batch, false);
  for (size_t i = 0; i < batch.size(); ++i)
    expect_equal_arrays(batch[i], expected[i], "Batch forward transform");

  plan.transform_batch(batch, true);
  for (size_t i = 0; i < batch.size(); ++i)
    expect_equal_arrays(batch[i], modded_copy<DefaultNTT>(original[i]),
                        "Batch inverse transform");
}

TEST_F(NTTIndustrialTest, BluesteinPlanMatchesStaticAndBatchVariants)
{
  typename DefaultNTT::Plan plan(7);

  Array<uint64_t> input = random_poly(7);
  Array<uint64_t> expected = naive_transform<DefaultNTT>(input, false);
  Array<uint64_t> actual = input;
  plan.transform(actual, false);
  expect_equal_arrays(actual, expected, "Bluestein plan forward transform");

  plan.transform(actual, true);
  expect_equal_arrays(actual, modded_copy<DefaultNTT>(input),
                      "Bluestein plan inverse round trip");

  Array<Array<uint64_t>> batch;
  Array<Array<uint64_t>> reference;
  for (size_t i = 0; i < 3; ++i)
    {
      Array<uint64_t> item = random_poly(7);
      batch.append(item);
      Array<uint64_t> transformed = item;
      DefaultNTT::transform(transformed, false);
      reference.append(transformed);
    }

  plan.transform_batch(batch, false);
  for (size_t i = 0; i < batch.size(); ++i)
    expect_equal_arrays(batch[i], reference[i], "Bluestein batch transform");
}

TEST_F(NTTIndustrialTest, ParallelTransformAndMultiplyMatchSequentialResults)
{
  ThreadPool pool(4);

  Array<uint64_t> input = random_poly(1 << 12);
  Array<uint64_t> sequential = input;
  Array<uint64_t> parallel = input;

  DefaultNTT::transform(sequential, false);
  DefaultNTT::ptransform(pool, parallel, false);
  expect_equal_arrays(parallel, sequential, "Parallel forward transform");

  DefaultNTT::transform(sequential, true);
  DefaultNTT::ptransform(pool, parallel, true);
  expect_equal_arrays(parallel, sequential, "Parallel inverse transform");

  Array<uint64_t> lhs = random_poly(300);
  Array<uint64_t> rhs = random_poly(280);
  const Array<uint64_t> expected = DefaultNTT::multiply(lhs, rhs);
  const Array<uint64_t> got = DefaultNTT::pmultiply(pool, lhs, rhs);
  expect_equal_arrays(got, expected, "Parallel multiply");
}

TEST_F(NTTIndustrialTest, ParallelBatchMatchesSequentialBatch)
{
  ThreadPool pool(4);
  typename DefaultNTT::Plan plan(256);

  Array<Array<uint64_t>> batch;
  for (size_t i = 0; i < 6; ++i)
    batch.append(random_poly(256));

  Array<Array<uint64_t>> sequential = batch;
  Array<Array<uint64_t>> parallel = batch;

  plan.transform_batch(sequential, false);
  plan.ptransform_batch(pool, parallel, false);

  for (size_t i = 0; i < sequential.size(); ++i)
    expect_equal_arrays(parallel[i], sequential[i], "Parallel batch transform");
}

TEST_F(NTTIndustrialTest, MultiplyInplaceReplacesLeftOperand)
{
  Array<uint64_t> lhs = {1, 2, 3};
  const Array<uint64_t> rhs = {4, 5, 6};
  const Array<uint64_t> expected = DefaultNTT::multiply(lhs, rhs);

  DefaultNTT::multiply_inplace(lhs, rhs);
  expect_equal_arrays(lhs, expected, "multiply_inplace");
}

TEST_F(NTTIndustrialTest, ExactMultiplyReportsSupportedProductLengths)
{
  EXPECT_TRUE(NTTExact::supports_product_size(1));
  EXPECT_TRUE(NTTExact::supports_product_size(7));
  EXPECT_TRUE(NTTExact::supports_product_size(1024));
  EXPECT_TRUE(NTTExact::supports_product_size((1U << 20) + 3));
  EXPECT_FALSE(NTTExact::supports_product_size(0));
  EXPECT_FALSE(NTTExact::supports_product_size((1U << 21) + 1));
}

TEST_F(NTTIndustrialTest, ExactMultiplyMatchesNaiveReferenceAboveSinglePrime)
{
  const Array<uint64_t> lhs = {
    1000000000000ULL,
    DefaultNTT::mod + 77ULL,
    1234567890123ULL
  };
  const Array<uint64_t> rhs = {
    777777777777ULL,
    42ULL,
    DefaultNTT::mod + 11ULL
  };

  const Array<__uint128_t> expected = naive_exact_multiply(lhs, rhs);
  const Array<__uint128_t> got = NTTExact::multiply(lhs, rhs);
  expect_equal_arrays_u128(got, expected, "Exact multiply");
}

TEST_F(NTTIndustrialTest, ExactMultiplyAcceptsNearTwoToSixtyThreeWhenBoundFits)
{
  const Array<uint64_t> lhs = {
    (1ULL << 63) - 25ULL,
    0ULL,
    (1ULL << 63) - 17ULL
  };
  const Array<uint64_t> rhs = {7ULL, 1ULL};

  const Array<__uint128_t> expected = naive_exact_multiply(lhs, rhs);
  const Array<__uint128_t> got = NTTExact::multiply(lhs, rhs);
  expect_equal_arrays_u128(got, expected, "Near-2^63 exact multiply");
}

TEST_F(NTTIndustrialTest, ParallelExactMultiplyMatchesSequentialAndPrimeResidues)
{
  ThreadPool pool(4);

  Array<uint64_t> lhs = Array<uint64_t>::create(24);
  Array<uint64_t> rhs = Array<uint64_t>::create(19);
  for (size_t i = 0; i < lhs.size(); ++i)
    lhs(i) = DefaultNTT::mod + 1000ULL + (rng_() % 1000000ULL);
  for (size_t i = 0; i < rhs.size(); ++i)
    rhs(i) = DefaultNTT::mod + 500ULL + (rng_() % 2000000ULL);

  const Array<__uint128_t> sequential = NTTExact::multiply(lhs, rhs);
  const Array<__uint128_t> parallel = NTTExact::pmultiply(pool, lhs, rhs);
  expect_equal_arrays_u128(parallel, sequential, "Parallel exact multiply");

  const Array<uint64_t> mod0 = NTT<998244353ULL, 3ULL>::multiply(lhs, rhs);
  const Array<uint64_t> mod1 = NTT<469762049ULL, 3ULL>::multiply(lhs, rhs);
  const Array<uint64_t> mod2 = NTT<1004535809ULL, 3ULL>::multiply(lhs, rhs);

  ASSERT_EQ(sequential.size(), mod0.size());
  for (size_t i = 0; i < sequential.size(); ++i)
    {
      EXPECT_EQ(static_cast<uint64_t>(sequential[i] % 998244353ULL), mod0[i])
        << "Residue mod 998244353 at index " << i;
      EXPECT_EQ(static_cast<uint64_t>(sequential[i] % 469762049ULL), mod1[i])
        << "Residue mod 469762049 at index " << i;
      EXPECT_EQ(static_cast<uint64_t>(sequential[i] % 1004535809ULL), mod2[i])
        << "Residue mod 1004535809 at index " << i;
    }
}

TEST_F(NTTIndustrialTest, ExactMultiplyRejectsInputsOutsideCRTRange)
{
  const Array<uint64_t> lhs = {(1ULL << 63) - 1ULL};
  const Array<uint64_t> rhs = {(1ULL << 63) - 1ULL};
  EXPECT_THROW(static_cast<void>(NTTExact::multiply(lhs, rhs)),
               std::invalid_argument);
}

TEST_F(NTTIndustrialTest, ErrorPathsAreValidated)
{
  Array<uint64_t> empty;
  Array<uint64_t> bad = {1, 2, 3};
  Array<uint64_t> unsupported = {1, 2, 3, 4, 5, 6};
  Array<Array<uint64_t>> mismatched_batch = {{1, 2, 3, 4}, {1, 2}};

  EXPECT_THROW(DefaultNTT::transform(empty, false), std::invalid_argument);
  EXPECT_THROW(DefaultNTT::transform(bad, false), std::invalid_argument);
  EXPECT_THROW(DefaultNTT::transform(unsupported, false),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::primitive_root_of_unity(3)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::primitive_root_of_unity(10)),
               std::invalid_argument);
  EXPECT_THROW(([]()
                {
                  typename DefaultNTT::Plan invalid_plan(0);
                  (void) invalid_plan;
                }()),
               std::invalid_argument);

  typename DefaultNTT::Plan small_plan(4);
  EXPECT_THROW(small_plan.multiply(Array<uint64_t>({1, 2, 3}),
                                   Array<uint64_t>({4, 5, 6})),
               std::invalid_argument);
  EXPECT_THROW(small_plan.transform_batch(mismatched_batch, false),
               std::invalid_argument);

  ThreadPool pool(2);
  EXPECT_THROW(small_plan.ptransform_batch(pool, mismatched_batch, false),
               std::invalid_argument);

  EXPECT_THROW(static_cast<void>(DefaultNTT::poly_inverse(Array<uint64_t>({0, 1}), 4)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::poly_divmod(Array<uint64_t>({1, 2}),
                                                         Array<uint64_t>())),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::poly_log(Array<uint64_t>({2, 1}), 4)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::poly_exp(Array<uint64_t>({1, 1}), 4)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::poly_sqrt(Array<uint64_t>({3}), 4)),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::interpolate(Array<uint64_t>({1, 1}),
                                                         Array<uint64_t>({2, 3}))),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::negacyclic_multiply(Array<uint64_t>(),
                                                                 Array<uint64_t>())),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::negacyclic_multiply(Array<uint64_t>({1, 2}),
                                                                 Array<uint64_t>({1, 2, 3, 4}))),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::negacyclic_multiply(Array<uint64_t>({1, 2, 3}),
                                                                 Array<uint64_t>({4, 5, 6}))),
               std::invalid_argument);
  EXPECT_THROW(([]()
                {
                  Array<uint64_t> lhs = Array<uint64_t>::create(16);
                  Array<uint64_t> rhs = Array<uint64_t>::create(16);
                  for (size_t i = 0; i < 16; ++i)
                    {
                      lhs(i) = static_cast<uint64_t>(i & 1U);
                      rhs(i) = static_cast<uint64_t>((i + 1) & 1U);
                    }
                  static_cast<void>(NTT<17ULL, 3ULL>::negacyclic_multiply(lhs, rhs));
                }()),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::bigint_multiply<10>(Array<uint64_t>({10}),
                                                                 Array<uint64_t>({1}))),
               std::invalid_argument);
  EXPECT_THROW(static_cast<void>(DefaultNTT::bigint_multiply<(1ULL << 48)>(
                                     Array<uint64_t>({(1ULL << 48) - 1,
                                                      (1ULL << 48) - 1,
                                                      (1ULL << 48) - 1}),
                                     Array<uint64_t>({(1ULL << 48) - 1,
                                                      (1ULL << 48) - 1,
                                                      (1ULL << 48) - 1}))),
               std::invalid_argument);

  EXPECT_THROW(static_cast<void>(NTTExact::multiply(Array<uint64_t>({1ULL << 63,
                                                                     1ULL << 63}),
                                                    Array<uint64_t>({1ULL << 63,
                                                                     1ULL << 63}))),
               std::invalid_argument);
}

TEST_F(NTTIndustrialTest, PerformanceRegressionIsGuardedByEnvironment)
{
  if (std::getenv("ENABLE_PERF_TESTS") == nullptr)
    GTEST_SKIP() << "Set ENABLE_PERF_TESTS=1 to run NTT performance checks";

  ThreadPool pool(4);
  const size_t n = 1 << 20;
  Array<uint64_t> lhs = Array<uint64_t>::create(n);
  Array<uint64_t> rhs = Array<uint64_t>::create(n);

  for (size_t i = 0; i < n; ++i)
    {
      lhs(i) = static_cast<uint64_t>(i);
      rhs(i) = static_cast<uint64_t>(n - i);
    }

  const auto start = std::chrono::steady_clock::now();
  const Array<uint64_t> product = DefaultNTT::pmultiply(pool, lhs, rhs);
  const auto end = std::chrono::steady_clock::now();

  ASSERT_EQ(product.size(), 2 * n - 1);

  const auto elapsed =
      std::chrono::duration_cast<std::chrono::milliseconds>(end - start)
          .count();
  EXPECT_LE(elapsed, 2500)
      << "NTT performance regression detected";
}
