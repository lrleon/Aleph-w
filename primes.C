
# include <exception>
# include <string>

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

static size_t nextPrimeIndex(unsigned long n)
{
  unsigned int i;

  for (i = 0; i < numPrimes; i++)
    if (primeList[i] >= n)
      return i;
  throw std::overflow_error("next prime n out of max " + std::to_string(n));
}

static size_t prevPrimeIndex(unsigned long n)
{
  unsigned int i;

  for (i = 1; i < numPrimes; i++)
    if (primeList[i] > n)
      return i - 1;

  throw std::underflow_error("next prime n out of min 0");
}

const unsigned long DefaultPrime = primeList[0];

size_t next_prime(unsigned long n)
{
  unsigned int prime_index = nextPrimeIndex(n);

  return primeList[prime_index];
}

bool check_primes_database()
{
  for (size_t i = 1; i < numPrimes; ++i)
    if (primeList[i] <= 2*primeList[i - 1])
      return false;
  return true;
}


}
