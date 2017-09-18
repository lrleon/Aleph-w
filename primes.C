# include <primes.H>

namespace Primes
{

const unsigned long primeList[] =
  {
    5,          11,           23,        47,        97,
    197,        397,          797,       1597,      3203,
    6421,       12853,        25717,     51437,     102877,
    205759,     411527,       823117,    1646237,   3292489,
    6584983,    13169977,     26339969,  52679969,  105359939, 
    210719881,  421439783,    842879579, 1685759167, 3371518343 /*, 
								  6743036717ul, 13486073473ul, 26972146961ul, 53944293929ul, 107888587883ul */
  };

const unsigned int numPrimes = 30;

static int nextPrimeIndex(unsigned long n)
{
  unsigned int i;

  for (i = 0; i < numPrimes; i++)
    if (primeList[i] >= n)
      return i;
  return -1;
}

static int prevPrimeIndex(unsigned long n)
{
  unsigned int i;

  for (i = 1; i < numPrimes; i++)
    if (primeList[i] > n)
      return i - 1;
            
  return -1;
}

const unsigned long DefaultPrime = primeList[0];

unsigned long next_prime(unsigned long n)
{ 
  unsigned int prime_index = nextPrimeIndex(n);

  return primeList[prime_index];
}

bool check_primes_database()
{
  for (int i = 1; i < numPrimes; ++i)
    if (primeList[i] <= 2*primeList[i - 1])
      return false;
  return true;
}


}
