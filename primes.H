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
# ifndef PRIMES_H
# define PRIMES_H

# include <cmath>
# include <ahDefs.H>
# include <ah-errors.H>

namespace Primes
{

  extern const unsigned long DefaultPrime;

  extern const size_t numPrimes;

  extern const unsigned long primeList[];

  extern unsigned long next_prime(unsigned long n);

  extern bool check_primes_database();

  // return true if n is a prime number
  inline bool is_prime(unsigned long n)
  {
    if (n <= 1) 
      return false;
    
    for (int i = 2; i <= sqrt(n); i++) 
      if (n % i == 0) 
        return false;
    
    return true;
  }

  // return the next prime number greater than n. Throws an
  // overflow_error if n is too large
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
