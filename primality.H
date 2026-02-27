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

/** @file primality.H
 *  @brief Advanced primality testing algorithms.
 *
 *  Provides a robust 64-bit Miller-Rabin primality test using a deterministic
 *  set of bases to ensure 100% correctness for all 64-bit integers.
 *
 *  @ingroup Algorithms
 */

#ifndef PRIMALITY_H
#define PRIMALITY_H

#include <cstdint>
#include <initializer_list>
#include "modular_arithmetic.H"

namespace Aleph
{

/** @brief Helper function for Miller-Rabin test.
 *
 *  Checks if 'a' is a composite witness for 'n'.
 *
 *  @param n The number being tested.
 *  @param a The base to test.
 *  @param d The odd part of n-1 (n-1 = d * 2^s).
 *  @param s The power of 2 dividing n-1.
 *  @return true if 'n' is composite, false if 'n' is probably prime.
 */
inline bool check_composite(uint64_t n, uint64_t a, uint64_t d, int s) noexcept
{
    uint64_t x = mod_exp(a, d, n);
    if (x == 1 || x == n - 1)
        return false;
    for (int r = 1; r < s; r++)
    {
        x = mod_mul(x, x, n);
        if (x == n - 1)
            return false;
    }
    return true;
}

/** @brief Miller-Rabin primality test for 64-bit integers.
 *
 *  A deterministic primality test for any 64-bit unsigned integer.
 *  It uses a known sufficient set of 12 bases to guarantee correctness
 *  for all n < 2^64.
 *
 *  @param n The number to test for primality.
 *  @return true if n is prime, false otherwise.
 */
inline bool miller_rabin(uint64_t n) noexcept
{
    if (n < 2) return false;
    if (n == 2 || n == 3) return true;
    if (n % 2 == 0) return false;

    uint64_t d = n - 1;
    int s = 0;
    while ((d & 1) == 0)
    {
        d >>= 1;
        s++;
    }

    // These bases are sufficient for deterministic testing up to 2^64.
    for (uint64_t a : {2, 3, 5, 7, 11, 13, 17, 19, 23, 29, 31, 37})
    {
        if (n <= a)
            break;
        if (check_composite(n, a, d, s))
            return false;
    }
    return true;
}

} // namespace Aleph

#endif // PRIMALITY_H