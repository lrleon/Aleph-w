# include <iostream>
# include <aleph.H>

# include "protected_cache.H"

using namespace std;

static size_t hashFct(const unsigned & i)
{
  return i;
}

int main(int argn, char *argc[])
{
  int n = 16;
  unsigned int t = time(0);

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testCache " << n << " " << t << endl;

  Protected_Cache<unsigned, int> cache(&hashFct, n);

  int i;
  for (i = 0; i < n; i++)
    cache.insert(i, i);

  try
    {
      for (i = 0; i < n; i++)
	cache.insert(i, i);
    }
  catch (exception & e)
    {
      cout << e.what() << endl;
    }

  Protected_Cache<unsigned, int>::Cache_Entry * cache_entry;

  try
    {
      n--;
      cache_entry = cache.search(n);
      cache.remove(cache_entry);
      n++;

      for (i = 0; i < n - 1; i++)
	{
	  cache.insert(i, n);
	  cache_entry = cache.search_and_lock(i);
	  while (cache_entry not_eq NULL)
	    {
	      cout << "(" << cache_entry->get_key() << ","
		   << cache_entry->get_data() << ")" << endl;
	      cache_entry = cache.search_next_and_lock(cache_entry);
	    }
	}
    }
  catch (exception & e)
    {
      cout << e.what() << endl << endl;
      exit(1);
    }

  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);
      if (cache_entry not_eq NULL)
	cache.remove(cache_entry);
    }


  cache.expand(n/2);


}
