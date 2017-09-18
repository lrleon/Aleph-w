
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



