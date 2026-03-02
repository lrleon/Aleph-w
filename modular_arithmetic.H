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

/** @file modular_arithmetic.H
 *  @brief Safe modular arithmetic, extended Euclidean algorithm, and Chinese Remainder Theorem.
 *
 *  Provides 64-bit safe modular arithmetic using 128-bit integers internally to prevent
 *  overflow, along with standard number theory algorithms like extended GCD, modular inverse,
 *  and the Chinese Remainder Theorem.
 *
 *  @ingroup Algorithms
 */

# ifndef MODULAR_ARITHMETIC_H
# define MODULAR_ARITHMETIC_H

# include <cstdint>

# include <ah-errors.H>
# include <tpl_array.H>

namespace Aleph
{
  /** @brief Safe 64-bit modular multiplication.
   *
   *  Computes (a * b) % m without overflow.
   *  Uses 128-bit integer types internally when available.
   *
   *  @param a First multiplier.
   *  @param b Second multiplier.
   *  @param m Modulus.
   *  @return (a * b) % m.
   *  @throws std::invalid_argument if m == 0.
   */
  [[nodiscard]] inline uint64_t mod_mul(uint64_t a, uint64_t b, uint64_t m)
  {
    ah_invalid_argument_if(m == 0) << "mod_mul: modulus must be > 0";

# if defined(__SIZEOF_INT128__)
    return static_cast<uint64_t>((static_cast<__uint128_t>(a) * b) % m);
# else
    uint64_t res = 0;
    a %= m;
    while (b)
      {
        if (b & 1)
          {
            if (m - res <= a) res = a - (m - res);
            else res += a;
          }
        if (m - a <= a) a = a - (m - a);
        else a <<= 1;
        b >>= 1;
      }
    return res;
# endif
  }

  /** @brief Modular exponentiation.
   *
   *  Computes (base^exp) % m in O(log exp) time.
   *
   *  @param base The base.
   *  @param exp The exponent.
   *  @param m The modulus.
   *  @return (base^exp) % m.
   *  @throws std::invalid_argument if m == 0.
   */
  [[nodiscard]] inline uint64_t mod_exp(uint64_t base, uint64_t exp, const uint64_t m)
  {
    ah_invalid_argument_if(m == 0) << "mod_exp: modulus must be > 0";
    if (m == 1) return 0;
    uint64_t res = 1;
    base %= m;
    while (exp > 0)
      {
        if (exp & 1)
          res = mod_mul(res, base, m);
        base = mod_mul(base, base, m);
        exp >>= 1;
      }
    return res;
  }

  /** @brief Extended Euclidean Algorithm.
   *
   *  Computes gcd(a, b) and the coefficients x, y such that a*x + b*y = gcd(a, b).
   *
   *  @param a First number.
   *  @param b Second number.
   *  @param x Reference to store the coefficient of a.
   *  @param y Reference to store the coefficient of b.
   *  @return The greatest common divisor of a and b.
   */
  template <typename T>
    requires (std::is_integral_v<T> and std::is_signed_v<T>)
  [[nodiscard]] T ext_gcd(T a, T b, T & x, T & y) noexcept
  {
    if (b == 0)
      {
        x = 1;
        y = 0;
        return a;
      }
    T x1, y1;
    T d = ext_gcd(b, a % b, x1, y1);
    x = y1;
    y = x1 - static_cast<T>(a / b) * y1;
    return d;
  }

  /** @brief Modular Inverse.
   *
   *  Computes the modular inverse of a modulo m using an iterative
   *  extended Euclidean algorithm that works entirely in unsigned
   *  arithmetic. Safe for all 64-bit values including m >= 2^63.
   *
   *  Requires gcd(a, m) == 1 (i.e., a and m must be coprime).
   *
   *  @param a The number to find the inverse of.
   *  @param m The modulus (must be > 0).
   *  @return The modular inverse of a modulo m, in [0, m).
   *
   *  @throws std::domain_error if m == 0.
   *  @throws std::invalid_argument if the inverse does not exist
   *          (a and m are not coprime, or a == 0 mod m).
   */
  [[nodiscard]] inline uint64_t mod_inv(const uint64_t a, const uint64_t m)
  {
    ah_domain_error_if(m == 0) << "mod_inv: modulus cannot be 0";

    if (m == 1)
      return 0;

    const uint64_t a_mod = a % m;
    ah_invalid_argument_if(a_mod == 0)
      << "Modular inverse does not exist (" << a << " is 0 mod " << m << ")";

    // Iterative extended Euclidean algorithm using unsigned arithmetic.
    // We track t0 such that a * t0 ≡ r0 (mod m) at each step,
    // keeping t0 in [0, m) to avoid signed overflow.
    uint64_t r0 = a_mod, r1 = m;
    uint64_t t0 = 1, t1 = 0;

    while (r1 != 0)
      {
        const uint64_t q = r0 / r1;

        const uint64_t tmp_r = r0 - q * r1;
        r0 = r1;
        r1 = tmp_r;

        // t_new = t0 - q * t1 (mod m), computed in unsigned
        const uint64_t qt1 = mod_mul(q, t1, m);
        const uint64_t tmp_t = (t0 >= qt1) ? (t0 - qt1) : (m - (qt1 - t0));
        t0 = t1;
        t1 = tmp_t;
      }

    ah_invalid_argument_if(r0 != 1)
      << "Modular inverse does not exist (numbers " << a
      << " and " << m << " are not coprime)";

    return t0;
  }

  /** @brief Chinese Remainder Theorem (CRT).
   *
   *  Finds a number x such that x = rem[i] (mod mod[i]) for all i.
   *  Assumes all moduli are pairwise coprime.
   *
   *  @param rem Array of remainders.
   *  @param mod Array of moduli (must be pairwise coprime).
   *  @return The solution modulo the product of all moduli.
   *
   *  @throws std::invalid_argument if input arrays have different sizes.
   *  @throws std::overflow_error if the product of moduli overflows uint64_t.
   */
  [[nodiscard]] inline uint64_t crt(const Array<uint64_t> & rem,
                                    const Array<uint64_t> & mod)
  {
    ah_invalid_argument_if(rem.size() != mod.size())
      << "crt: arrays must have the same size (got " << rem.size()
      << " vs " << mod.size() << ")";

    const size_t n = rem.size();
    if (n == 0)
      return 0;

    // Compute product of all moduli with overflow detection
    uint64_t prod = 1;
    for (size_t i = 0; i < n; ++i)
      {
        ah_invalid_argument_if(mod[i] <= 1)
          << "crt: all moduli must be > 1 (got " << mod[i] << " at index " << i << ")";

        ah_overflow_error_if(prod > UINT64_MAX / mod[i])
          << "crt: product of moduli overflows uint64_t at index " << i;
        prod *= mod[i];
      }

    uint64_t result = 0;
    for (size_t i = 0; i < n; ++i)
      {
        const uint64_t p = prod / mod[i];
        const uint64_t inv = mod_inv(p, mod[i]);
        uint64_t term = mod_mul(rem[i], p, prod);
        term = mod_mul(term, inv, prod);
        if (result >= prod - term)
          result -= (prod - term);
        else
          result += term;
      }

    return result;
  }
} // namespace Aleph

# endif // MODULAR_ARITHMETIC_H
