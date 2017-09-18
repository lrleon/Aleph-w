
# ifndef GENHASHTABLE_H
# define GENHASHTABLE_H


/**
   GenHashTable base class. Basic utilities for hash tables.

   More than an abstract class, this class is a container for global
   definitions and functions that could be used for other classes
   implementing hash tables 
   @author Leandro Leon
   @version 1.0
 */
class GenHashTable
{

protected:

  GenHashTable() { /* Empty */ }

public:

  /// Number of primes saved in primeList array
  static const unsigned int numPrimes = 28;

/// Array of prime numbers
  static const unsigned long primeList[];

  /// Default index in primeList array
  static const unsigned int DefaultIndex = 0; 

  /// Default prime number (this is primeList[DefaultIndex])
  static const unsigned long DefaultPrime; // primeList[DefaultIndex]

  /** 
      Searches in array primeList the next prime number immediately greater. 
      @param n the base number. Result will be the next prime number
      immediately greater than n.
      @return -1 if computation fails (n is greater than
      primeList[numPrimes-1]; the next prime number immediately greater than
      n otherwise 
  */
  static int nextPrimeIndex(unsigned long n)
    {
      unsigned int i;

      for (i = 0; i < numPrimes; i++)
	if (primeList[i] > n)
	  return i;
      return -1;
    }

  /**
     Searches in array primeList the previous prime number immediately
     lesser.
     @param n the base number. Result will be the previous prime number
     immediately lesser than n 
     @return -1 if computation fails (n is greater than
     primeList[numPrimes-1]; the previous prime number immediately greater than
     n otherwise 
  */
  static int prevPrimeIndex(unsigned long n)
    {
      unsigned int i;

      for (i = 1; i < numPrimes; i++)
	if (primeList[i] > n)
	  return i - 1;
            
      return -1;
    }

  
}; /* GenHashTable */



# endif /* GENHASHTABLE_H */
