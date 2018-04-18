/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/
# ifndef PRIMES_H
# define PRIMES_H

# include <cmath>
# include <ahDefs.H>

namespace Primes
{

  extern const unsigned long DefaultPrime;

  extern const unsigned int numPrimes;

  extern const unsigned long primeList[];

  extern unsigned long next_prime(unsigned long n);

  extern bool check_primes_database();

inline bool is_prime(unsigned long n)
{
  if (n <= 3)
    return true;
  
  if (n % 2 == 0 or n % 3 == 0)
    return false;

  for (unsigned long d = 5; d*d <= n; d += 6)
    if (n % d == 0 or n % (d + 2) == 0)
      return false;

  return true;
}

}; /* namespace Primes */

# endif /* PRIMES_H */
