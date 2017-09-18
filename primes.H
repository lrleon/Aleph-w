# ifndef PRIMES_H
# define PRIMES_H

# include <ahDefs.H>

namespace Primes
{

  extern const unsigned long DefaultPrime;

  extern const unsigned int numPrimes;

  extern const unsigned long primeList[];

  extern unsigned long next_prime(unsigned long n);

  extern bool check_primes_database();

}; /* namespace Primes */

# endif /* PRIMES_H */
