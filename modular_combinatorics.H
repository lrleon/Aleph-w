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
 *  @brief Modular combinatorics utilities.
 *
 *  Provides precomputation of factorials and inverse factorials for O(1)
 *  combinations modulo a prime. Also includes Lucas Theorem for large n, k.
 *
 *  @ingroup Algorithms
 */

#ifndef MODULAR_COMBINATORICS_H
#define MODULAR_COMBINATORICS_H

#include <cstdint>
#include <ah-errors.H>
#include <tpl_array.H>
#include "modular_arithmetic.H"

namespace Aleph
{

/** @brief Combinatorics operations modulo a prime.
 */
class ModularCombinatorics
{
    uint64_t mod_;
    Array<uint64_t> fact_;
    Array<uint64_t> invFact_;

public:
    /** @brief Precomputes factorials up to 'max_n' modulo 'p'.
     *
     *  @param max_n Maximum value for factorials.
     *  @param p Prime modulus.
     */
    ModularCombinatorics(size_t max_n, uint64_t p) : mod_(p)
    {
        if (max_n >= mod_)
            max_n = static_cast<size_t>(mod_ - 1);
            
        fact_.reserve(max_n + 1);
        invFact_.reserve(max_n + 1);
        
        fact_.append(1);
        for (size_t i = 1; i <= max_n; i++)
            fact_.append(mod_mul(fact_[i - 1], i, mod_));
            
        invFact_.putn(max_n + 1);
        invFact_(max_n) = mod_inv(fact_[max_n], mod_);
        for (size_t i = max_n; i >= 1; i--)
            invFact_(i - 1) = mod_mul(invFact_[i], i, mod_);
    }

    /** @brief Calculate nCk modulo p in O(1).
     *
     *  @param n Total items.
     *  @param k Chosen items.
     *  @return nCk % p.
     */
    [[nodiscard]] uint64_t nCk(uint64_t n, uint64_t k) const noexcept
    {
        if (k > n) return 0;
        const uint64_t num = fact_[n];
        const uint64_t den = mod_mul(invFact_[k], invFact_[n - k], mod_);
        return mod_mul(num, den, mod_);
    }

    /** @brief Calculate nCk modulo p using Lucas' theorem.
     *
     *  Useful when n and k are large, but p is small (and <= max_n).
     *
     *  @param n Total items.
     *  @param k Chosen items.
     *  @return nCk % p.
     */
    [[nodiscard]] uint64_t lucas_nCk(uint64_t n, uint64_t k) const noexcept
    {
        if (k > n) return 0;
        if (k == 0) return 1;

        const uint64_t ni = n % mod_;
        const uint64_t ki = k % mod_;
        
        if (ki > ni) return 0; // Lucas theorem property
        
        return mod_mul(nCk(ni, ki), lucas_nCk(n / mod_, k / mod_), mod_);
    }
};

} // namespace Aleph

#endif // MODULAR_COMBINATORICS_H