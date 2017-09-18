# include <bloom-filter.H>

template <typename T>
ostream & operator << (ostream & s, const DynList<T> & l)
{
  if (l.is_empty())
    return s << "EMPTY";

  l.for_each([&s] (const T & i) { s << i << " "; });
  return s;
}

int main(int argc, char *argv[])
{
  const size_t n = atoi(argv[1]);
  const double p = atof(argv[2]);
  const unsigned long seed = atoi(argv[3]);

  Bloom_Filter<long> f(n, p, seed);
  Bloom_Filter<long> ff(f);

  ff.swap(f);

  auto t = f.estimate(n, p);
  auto m = get<0>(t);
  cout << "seeds = " << f.hash_seeds() << endl
       << "hashes(10) = " << f.hashes(10) << endl
       << "hashes(9)  = " << f.hashes(9) << endl
       << "common(10, 9) = " << f.common_hashes(10, 9) << endl
       << "m = " << m << " (" << m/8 + m% 8 << " bytes)" << endl
       << "k = " << f.get_k() << endl
       << "sizeof(size_t) = " << sizeof(size_t) << endl
       << endl
       << endl
       << "Inserting " << n << " items sequentially" << endl
       << endl;

  for (auto i = 0; i < n; ++i)
    f.append(i);

  auto x = f.get_x();
  cout << "Done!" << endl
       << "Bits set to 1 = " << x << " of " << m << " ("
       << 100.0*x/m << " %)" << endl    
       << endl
       << "Generating random searches" << endl;
  
  gsl_rng * r = gsl_rng_alloc (gsl_rng_mt19937);
  gsl_rng_set(r, seed % gsl_rng_max(r));
  size_t false_positive_count = 0;
  size_t failed_search_count = 0;
  for (auto i = 0; i < 10*n; ++i)
    {
      long val = gsl_rng_get(r);
      if (val >= 0 and val < n)
	{
	  if (not f.contains(val))
	    cout << "ERROR: " << val << " was not found" << endl;
	}
      else
	{
	  ++failed_search_count;
	  if (f.contains(val))
	    ++false_positive_count;
	}
    }

  cout << "done!" << endl
       << "Total searches  = " << 10*n << endl
       << "Failed searches = " << failed_search_count << endl
       << "False positives = " << false_positive_count << endl
       << "Error = " << 100.0*false_positive_count/failed_search_count 
       << " % " << endl
       << endl;

  gsl_rng_free(r);
}
