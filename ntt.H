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

/** @file ntt.H
 *  @brief Number Theoretic Transform (NTT) for fast polynomial multiplication.
 *
 *  Provides an efficient implementation of the Number Theoretic Transform
 *  for modular polynomial multiplication, particularly suited for competitive
 *  programming (e.g., modulo 998244353).
 *
 *  @ingroup Algorithms
 */

# ifndef NTT_H
# define NTT_H

# include <cstdint>
# include <algorithm>
# include <tpl_array.H>
# include <modular_arithmetic.H>

namespace Aleph
{

/** @brief Number Theoretic Transform environment.
 *
 *  Configured with a specific prime modulus and its primitive root.
 *  Default parameters use the standard CP prime 998244353.
 *
 *  @tparam MOD Prime modulus.
 *  @tparam ROOT Primitive root modulo MOD.
 */
template <uint64_t MOD = 998244353, uint64_t ROOT = 3>
class NTT
{
    /** @brief Bit reversal permutation helper.
     *
     *  Reorders the array elements in bit-reversed order to facilitate
     *  the iterative butterfly computation.
     *
     *  @param[in,out] a Array to permute.
     */
    static void bit_reverse(Array<uint64_t> & a)
    {
        const size_t n = a.size();
        for (size_t i = 1, j = 0; i < n; ++i)
          {
            size_t bit = n >> 1;
            for (; j & bit; bit >>= 1)
              j ^= bit;
            j ^= bit;
            if (i < j)
              std::swap(a(i), a(j));
          }
    }

public:
    /** @brief Forward or Inverse Number Theoretic Transform.
     *
     *  Computes the NTT of the input array in-place.
     *
     *  @param[in,out] a The input polynomial coefficients (modified in-place).
     *  @param[in] invert true for Inverse NTT, false for Forward NTT.
     */
    static void transform(Array<uint64_t> & a, bool invert)
    {
        const size_t n = a.size();
        if (n <= 1) return;

        // Check if n is a power of 2
        ah_invalid_argument_if((n & (n - 1)) != 0)
          << "NTT::transform: input size " << n << " must be a power of two";

        // Check if (MOD - 1) is divisible by n
        ah_invalid_argument_if((MOD - 1) % static_cast<uint64_t>(n) != 0)
          << "NTT::transform: size " << n
          << " must divide MOD - 1 (" << (MOD - 1) << ")";

        bit_reverse(a);

        for (size_t len = 2; len <= n; len <<= 1)
          {
            uint64_t wlen = mod_exp(ROOT, (MOD - 1) / len, MOD);
            if (invert)
              wlen = mod_inv(wlen, MOD);

            for (size_t i = 0; i < n; i += len)
              {
                uint64_t w = 1;
                for (size_t j = 0; j < len / 2; ++j)
                  {
                    const uint64_t u = a[i + j];
                    const uint64_t v = mod_mul(a[i + j + len / 2], w, MOD);
                    a(i + j) = (u + v) % MOD;
                    a(i + j + len / 2) = (u - v + MOD) % MOD;
                    w = mod_mul(w, wlen, MOD);
                  }
              }
          }

        if (invert)
          {
            const uint64_t inv_n = mod_inv(n, MOD);
            for (size_t i = 0; i < n; ++i)
              a(i) = mod_mul(a[i], inv_n, MOD);
          }
    }

    /** @brief Multiply two polynomials using NTT.
     *
     *  Computes the discrete convolution of two polynomials modulo MOD.
     *
     *  @param[in] a First polynomial coefficients.
     *  @param[in] b Second polynomial coefficients.
     *  @return The convolution of a and b modulo MOD.
     */
    static Array<uint64_t> multiply(Array<uint64_t> const& a, Array<uint64_t> const& b)
    {
        if (a.is_empty() or b.is_empty()) return {};
        
        size_t n = 1;
        while (n < a.size() + b.size() - 1) 
            n <<= 1;

        // Compute maximum NTT length supported by MOD
        uint64_t m_minus_1 = MOD - 1;
        size_t max_ntt_len = 1;
        while (m_minus_1 > 0 and (m_minus_1 % 2 == 0))
          {
            max_ntt_len <<= 1;
            m_minus_1 /= 2;
          }

        ah_invalid_argument_if(n > max_ntt_len)
          << "NTT::multiply: product size " << n << " exceeds maximum supported length "
          << max_ntt_len << " for MOD " << MOD;
            
        Array<uint64_t> fa = a, fb = b;
        fa.reserve(n);
        fb.reserve(n);
        for (size_t i = a.size(); i < n; ++i) fa.append(0);
        for (size_t i = b.size(); i < n; ++i) fb.append(0);

        transform(fa, false);
        transform(fb, false);

        for (size_t i = 0; i < n; i++)
            fa(i) = mod_mul(fa[i], fb[i], MOD);

        transform(fa, true);

        // Resize to actual degree + 1
        Array<uint64_t> res;
        res.reserve(a.size() + b.size() - 1);
        for (size_t i = 0; i < a.size() + b.size() - 1; ++i)
          res.append(fa[i]);
        return res;
    }
};

} // namespace Aleph

# endif // NTT_H
