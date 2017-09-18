

# include <math.h>


static const long double phi_1 = (1.0 + sqrt(5))/2.0;
static const long double phi_2 = (1.0 - sqrt(5))/2.0;

long fib(const int n)
{
  return (powl(phi_1, n) - powl(phi_2, n))/2.0;
}

    template <typename T>
const size_t fib_search(T * s, const T & x, const size_t & n)

{
  size_t fq = fib(n + 1);
  
  

  if (x < s[
}
