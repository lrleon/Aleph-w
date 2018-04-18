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

# ifndef HASHPARS_H
# define HASHPARS_H

namespace HashParameters
{
  const unsigned int numPrimes = 28;

  const unsigned long primeList[ /* numPrimes */ ] =
  {
    53,         97,           193,       389,       769,
    1543,       3079,         6151,      12289,     24593,
    49157,      98317,        196613,    393241,    786433,
    1572869,    3145739,      6291469,   12582917,  25165843, 
    50331653,   100663319,    201326611, 402653189, 805306457, 
    1610612741, 3221225473UL, 4294967291UL 
  };

  const unsigned int DefaultIndex = 0; 

  const unsigned long DefaultPrime = primeList[DefaultIndex];

  /* 
     Searches the next prime number immediately greater than
     n. Returns -1 if not found. Otherwise, returns the index within
     primeList 
  */
  inline int nextPrimeIndex(unsigned long n)
    {
      int i;

      for (i = 0; i < numPrimes; i++)
	if (primeList[i] > n)
	  return i;
      return -1;
    }


  /* 
     Analogous to nextPrimeIndex but the immediately lesser than n 
  */
  inline int prevPrimeIndex(unsigned long n)
    {
      int i;

      for (i = 1; i < numPrimes; i++)
	if (primeList[i] > n)
	  return i - 1;
            
      return -1;
    }

  
}; /* namespace HashPars */



# endif /* HASHPARS_H */



