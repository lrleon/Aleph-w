
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
 * @file primes.cc
 * @brief Tests for Primes
 */
# include <gtest/gtest.h>

# include <primes.H>

using namespace std;
using namespace testing;
using namespace Aleph;
using namespace Primes;

TEST(Prime, test_prime)
{
  ASSERT_FALSE(is_prime(1));
  ASSERT_TRUE(is_prime(2));
  ASSERT_TRUE(is_prime(3));
  ASSERT_TRUE(is_prime(5));
  ASSERT_TRUE(is_prime(7));
  ASSERT_TRUE(is_prime(11));
  ASSERT_TRUE(is_prime(17));
  ASSERT_TRUE(is_prime(19));

  ASSERT_FALSE(is_prime(4));
  ASSERT_FALSE(is_prime(6));
  ASSERT_FALSE(is_prime(12));
  ASSERT_FALSE(is_prime(15));
}

TEST(Prime, list)
{
  // Verify all entries in primeList are actually prime
  for (size_t i = 0; i < numPrimes; ++i)
    {
      ASSERT_TRUE(is_prime(primeList[i])) << "primeList[" << i << "] = " << primeList[i] << " is not prime";
    }
}

TEST(Prime, database)
{
  ASSERT_TRUE(check_primes_database());
}

TEST(Prime, next_prime_semantics)
{
  ASSERT_EQ(next_prime(0), 2UL);
  ASSERT_EQ(next_prime(1), 2UL);
  ASSERT_EQ(next_prime(2), 2UL);
  ASSERT_EQ(next_prime(3), 3UL);
  ASSERT_EQ(next_prime(4), 5UL);
  ASSERT_EQ(next_prime(5), 5UL);
  ASSERT_EQ(next_prime(6), 7UL);

  for (unsigned long n = 0; n < 2000; ++n)
    {
      const unsigned long p = next_prime(n);
      ASSERT_TRUE(is_prime(p)) << "next_prime(" << n << ") = " << p << " is not prime";
      ASSERT_GE(p, n) << "next_prime(" << n << ") = " << p << " < " << n;
      ASSERT_EQ(next_prime(p), p);
    }

  for (size_t i = 0; i < 200 && i < numPrimes; ++i)
    ASSERT_EQ(next_prime(primeList[i]), primeList[i]);
}
