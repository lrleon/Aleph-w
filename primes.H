
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


/** @file primes.H
 *  @brief Prime number utilities for hash tables and mathematical operations.
 *
 *  This file provides functions for working with prime numbers, primarily
 *  used for determining optimal hash table sizes. Prime-sized tables
 *  reduce clustering in hash collision resolution.
 *
 *  ## Features
 *
 *  - Pre-computed prime number database for fast lookups
 *  - Primality testing via trial division
 *  - Next prime computation for dynamic sizing
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  #include <primes.H>
 *
 *  // Check if a number is prime
 *  bool prime = Primes::is_prime(17);  // true
 *
 *  // Get next prime for hash table sizing
 *  size_t table_size = Primes::next_prime(1000);  // Returns 1009
 *
 *  // Get next prime greater than n
 *  unsigned long p = Primes::next_prime_number_greater_than(100);  // 101
 *  ```
 *
 *  @see tpl_lhash.H Linear hash tables using primes
 *  @see tpl_odhash.H Open-addressed hash tables
 *
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
 */

# ifndef PRIMES_H
# define PRIMES_H

# include <cmath>
# include <ahDefs.H>
# include <ah-errors.H>

namespace Primes
{

  /// Default prime number used when no specific size is requested
  extern const unsigned long DefaultPrime;

  /// Number of primes in the pre-computed database
  extern const size_t numPrimes;

  /// Pre-computed array of prime numbers for fast lookup
  extern const unsigned long primeList[];

  /**
   * @brief Find the smallest prime number >= n from the database.
   *
   * Performs binary search on the pre-computed prime database to find
   * a suitable prime for hash table sizing.
   *
   * @param n Minimum value for the prime
   * @return Smallest prime p such that p >= n
   *
   * @note Uses pre-computed database, O(log numPrimes) complexity
   */
  extern unsigned long next_prime(unsigned long n);

  /**
   * @brief Verify the integrity of the prime database.
   *
   * Checks that all numbers in primeList are actually prime.
   * Used for testing and validation.
   *
   * @return true if all entries in primeList are prime
   */
  extern bool check_primes_database();

  /**
   * @brief Test if a number is prime using trial division.
   *
   * Checks divisibility by all integers from 2 to sqrt(n).
   *
   * @param n Number to test for primality
   * @return true if n is prime, false otherwise
   *
   * @note O(sqrt(n)) complexity - use next_prime() for hash table sizing
   *
   * @par Example
   * @code
   * Primes::is_prime(17);  // true
   * Primes::is_prime(18);  // false
   * Primes::is_prime(2);   // true
   * Primes::is_prime(1);   // false
   * @endcode
   */
  inline bool is_prime(unsigned long n)
  {
    if (n <= 1) 
      return false;
    
    for (int i = 2; i <= sqrt(n); i++) 
      if (n % i == 0) 
        return false;
    
    return true;
  }

  /**
   * @brief Find the next prime strictly greater than n.
   *
   * Unlike next_prime() which uses a database, this function computes
   * primes dynamically using trial division. Useful when the database
   * doesn't contain large enough primes.
   *
   * @param n Find prime greater than this value
   * @return Smallest prime p such that p > n
   *
   * @throw std::domain_error if n is too large (would overflow)
   *
   * @note O(sqrt(p) * (p-n)/2) worst case - slower than next_prime()
   *
   * @par Example
   * @code
   * next_prime_number_greater_than(10);   // 11
   * next_prime_number_greater_than(11);   // 13
   * next_prime_number_greater_than(100);  // 101
   * @endcode
   */
  inline unsigned long next_prime_number_greater_than(unsigned long n)
  {
    if (n <= 1)
      return 2;

    if (n == 2)
      return 3;

    const unsigned long max = std::numeric_limits<unsigned long>::max();

    ah_domain_error_if(n == max)
      << "next_prime_number_greater_than: overflow error";

    // if n is even, add 1
    if (n % 2 == 0)
      ++n;

    ah_domain_error_if(n == max)
      << "next_prime_number_greater_than: overflow error";

    unsigned long p = n + 2;
    while (not is_prime(p))
      {
        ah_domain_error_if(p + 1 == max or p + 2 == max)
          << "next_prime_number_greater_than: overflow error";
        p += 2;
      }

    return p;
  }
  
}; /* namespace Primes */

# endif /* PRIMES_H */
