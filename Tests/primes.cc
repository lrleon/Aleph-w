
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

# include <gtest/gtest.h>

# include <primes.H>

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
  unsigned long n = 2;
  for (size_t i = 0; i < numPrimes; ++i)
    {
      n = next_prime(n);
      ASSERT_TRUE(is_prime(n));
      ASSERT_FALSE(is_prime(n + 1));
    }
}
