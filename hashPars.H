
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


/** @file hashPars.H
 *  @brief Hash table parameters.
 *  @ingroup Hash
 *  @author Leandro Rabindranath León
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



