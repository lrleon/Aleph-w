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

/** @file modular_combinatorics.H
 *  @brief Modular combinatorics utilities for computing binomial coefficients.
 *
 *  This header provides tools for computing combinations (n choose k) modulo 
 *  a prime number. It uses precomputed factorials and modular inverse 
 *  factorials to achieve O(1) time complexity for queries within a 
 *  precomputed range.
 *
 *  For values of n and k that exceed the precomputed range (typically when 
 *  the modulus is small), Lucas' Theorem is provided to compute the result 
 *  efficiently.
 *
 *  @ingroup Algorithms
 *  @author Leandro Rabindranath Leon
 */

# ifndef MODULAR_COMBINATORICS_H
# define MODULAR_COMBINATORICS_H

# include <cstdint>
# include <ah-errors.H>
# include <tpl_array.H>
# include <modular_arithmetic.H>
# include <primality.H>

namespace Aleph
{
  /** @brief High-performance combinatorics operations modulo a prime.
   *
   *  The ModularCombinatorics class encapsulates the state required to 
   *  compute binomial coefficients modulo a prime@f$P@f$. 
   *
   *  It performs an initial precomputation of:
   *  - Factorials:@f$n! \pmod{P}@f$
   *  - Inverse Factorials:@f$(n!)^{-1} \pmod{P}@f$
   *
   *  This allows computing@f$\binom{n}{k} \equiv \frac{n!}{k!(n-k)!} \pmod{P}@f$
   *  in O(1) time per query.
   *
   *  @note The modulus @b must be a prime number for the modular inverse to 
   *        be well-defined for all factorials within@f$[1, P-1]@f$.
   */
  class ModularCombinatorics
  {
    uint64_t mod_;            ///< Prime modulus@f$P@f$.
    Array<uint64_t> fact_;    ///< Precomputed factorials array.
    Array<uint64_t> invFact_; ///< Precomputed inverse factorials array.

  public:
    /** @brief Construct and precompute factorials up to 'max_n' modulo 'p'.
     *
     *  @param[in] max_n Maximum value for@f$n@f$to be used in O(1) @ref nCk queries.
     *  @param[in] p     The prime modulus.
     *
     *  @pre `p` must be a prime number.
     *  @throws std::invalid_argument if `p` is not prime (verified via Miller-Rabin).
     *
     *  @par **Complexity**: Time O(max_n + log p), Space O(max_n).
     */
    ModularCombinatorics(size_t max_n, const uint64_t p) : mod_(p)
    {
      ah_invalid_argument_if(not miller_rabin(mod_))
        << "ModularCombinatorics: modulus " << mod_ << " must be prime";

      if (max_n >= mod_)
        max_n = static_cast<size_t>(mod_ - 1);

      fact_.reserve(max_n + 1);
      invFact_.reserve(max_n + 1);

      fact_.append(1);
      for (size_t i = 1; i <= max_n; i++)
        fact_.append(mod_mul(fact_[i - 1], i, mod_));

      invFact_.putn(max_n + 1);
      invFact_(max_n) = mod_inv(fact_[max_n], mod_);
      for (size_t i = max_n; i > 0; i--)
        invFact_(i - 1) = mod_mul(invFact_[i], i, mod_);
    }

    /** @brief Calculate@f$\binom{n}{k} \pmod{P}@f$in constant time.
     *
     *  Computes the binomial coefficient using the precomputed tables.
     *
     *  @param[in] n Total number of items.
     *  @param[in] k Number of items to choose.
     *  @return The value of@f$\binom{n}{k} \pmod{P}@f$.
     *
     *  @note Requires@f$n@f$to be within the `max_n` range provided at construction.
     *  @throws ah_out_of_range_error if@f$n@f$exceeds the precomputed range.
     *
     *  @par **Complexity**: Time O(1).
     */
    [[nodiscard]] uint64_t nCk(const uint64_t n, const uint64_t k) const
    {
      if (k > n) return 0;
      ah_out_of_range_error_if(n >= fact_.size())
          << "ModularCombinatorics::nCk: n=" << n << " exceeds precomputed range "
          << fact_.size() - 1;

      const uint64_t num = fact_[n];
      const uint64_t den = mod_mul(invFact_[k], invFact_[n - k], mod_);
      return mod_mul(num, den, mod_);
    }

    /** @brief Calculate@f$\binom{n}{k} \pmod{P}@f$using Lucas' Theorem.
     *
     *  Provides a way to compute combinations when@f$n@f$and@f$k@f$are 
     *  very large (exceeding `max_n`) but the prime modulus@f$P@f$is small.
     *
     *  Lucas' Theorem states:
     *  @f[ \binom{n}{k} \equiv \prod_{i=0}^m \binom{n_i}{k_i} \pmod{P} @f]
     *  where@f$n_i@f$and@f$k_i@f$are the digits of@f$n@f$and@f$k@f$
     *  in base@f$P@f$.
     *
     *  @pre The instance must have been precomputed up to at least `mod_ - 1`.
     *
     *  @param[in] n Total number of items.
     *  @param[in] k Number of items to choose.
     *  @return The value of@f$\binom{n}{k} \pmod{P}@f$.
     *
     *  @par **Complexity**: Time@f$O(\log_P n)@f$modular multiplications.
     */
    [[nodiscard]] uint64_t lucas_nCk(const uint64_t n, const uint64_t k) const
    {
      if (k > n) return 0;
      if (k == 0) return 1;

      const uint64_t ni = n % mod_;
      const uint64_t ki = k % mod_;

      if (ki > ni) return 0;

      return mod_mul(nCk(ni, ki), lucas_nCk(n / mod_, k / mod_), mod_);
    }
  };
} // namespace Aleph

# endif // MODULAR_COMBINATORICS_H
