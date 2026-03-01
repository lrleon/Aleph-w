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

/** @file pollard_rho.H
 *  @brief Integer factorization using Pollard's rho algorithm.
 *
 *  Provides a robust implementation of Pollard's rho algorithm for finding
 *  prime factors of 64-bit integers. It includes a random fallback to 
 *  prevent infinite loops on degenerate cases and uses recursive factor 
 *  extraction to find all prime factors of a number.
 *
 *  @ingroup Algorithms
 */

# ifndef POLLARD_RHO_H
# define POLLARD_RHO_H

# include <cstdint>
# include <numeric>
# include <random>

# include <ah-errors.H>
# include <tpl_array.H>
# include <ahSort.H>
# include <primality.H>
# include <modular_arithmetic.H>

namespace Aleph
{

namespace detail
{

/** @brief Inner function for Pollard's rho algorithm.
 *
 *  @param n The number to factor.
 *  @param seed The seed for the pseudo-random sequence.
 *  @param c The constant for the polynomial f(x) = x^2 + c.
 *  @return A non-trivial factor of n, or n if it fails to find one.
 */
[[nodiscard]] inline uint64_t pollard_rho_step(const uint64_t n, const uint64_t seed,
                                               const uint64_t c)
{
  uint64_t x = seed, y = seed, d = 1;
  auto f = [&](const uint64_t val) {
    const uint64_t v2 = mod_mul(val, val, n);
# if defined(__SIZEOF_INT128__)
    return static_cast<uint64_t>((static_cast<__uint128_t>(v2) + c) % n);
# else
    // Safe modular addition
    uint64_t c_mod = c % n;
    if (n - v2 <= c_mod) return c_mod - (n - v2);
    return v2 + c_mod;
# endif
  };

  while (d == 1)
    {
      x = f(x);
      y = f(f(y));
      const uint64_t diff = x > y ? x - y : y - x;
      d = std::gcd(diff, n);
      if (d == n)
        return n; // Failure for this seed/c combination
    }
  return d;
}

/** @brief Helper to repeatedly apply Pollard's rho until a factor is found.
 *
 *  Tries different random seed/constant pairs. Gives up after a bounded
 *  number of attempts to guarantee termination.
 *
 *  @param n Composite number to find a factor of.
 *  @return A non-trivial factor of n.
 *  @throws std::runtime_error if no factor found after max attempts.
 */
[[nodiscard]] inline uint64_t find_any_factor(const uint64_t n)
{
  if (n % 2 == 0) return 2;
  if (n % 3 == 0) return 3;
  if (n % 5 == 0) return 5;

    std::mt19937_64 rng(std::random_device{}());

    constexpr size_t max_attempts = 256;
    for (size_t attempt = 0; attempt < max_attempts; ++attempt)
      {
        const uint64_t seed = rng() % (n - 2) + 2;
        const uint64_t c = rng() % (n - 1) + 1;
        if (const uint64_t d = pollard_rho_step(n, seed, c); d != n)
          return d;
      }

    ah_runtime_error()
      << "pollard_rho: failed to find a factor of " << n
      << " after " << max_attempts << " attempts";
    __builtin_unreachable();
}

/** @brief Recursive extraction of all prime factors.
 */
inline void extract_prime_factors(uint64_t n, Array<uint64_t> & factors)
{
  if (n <= 1)
    return;
  if (miller_rabin(n))
    {
      factors.append(n);
      return;
    }

  const uint64_t factor = find_any_factor(n);
  extract_prime_factors(factor, factors);
  extract_prime_factors(n / factor, factors);
}

} // namespace detail

/** @brief Compute the prime factorization of a number using Pollard's rho.
 *
 *  @param n The number to factor.
 *  @return A sorted array containing the prime factors of n.
 */
[[nodiscard]] inline Array<uint64_t> pollard_rho(uint64_t n)
{
  Array<uint64_t> factors;
  detail::extract_prime_factors(n, factors);
  in_place_sort(factors);
  return factors;
}

} // namespace Aleph

# endif // POLLARD_RHO_H