/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
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
