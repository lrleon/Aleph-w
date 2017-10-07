
# include <iostream>

# include <aleph.H>
# include <tpl_cache.H>

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

  Cache<unsigned, unsigned> cache(&hashFct, n);

  int i;
  for (i = 0; i < n; i++)
    cache.insert(i, i);

  Cache<unsigned, unsigned>::Cache_Entry * cache_entry;
  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);

      assert(cache_entry not_eq NULL);
      assert(cache_entry->get_key() == i);

      assert(cache_entry->get_data() == i);
    }

  for (i = 0; i < n; i++)
    cache.insert(i, i);

  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);
      cache.lock_entry(cache_entry);
    }

  try
    {
      for (i = 0; i < n; i++)
	cache.insert(i, i);
    }
  catch (exception & e)
    {
      cout << e.what() << endl;
    }

  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);

      assert(cache_entry not_eq NULL);
      assert(cache_entry->get_key() == i);

      assert(cache_entry->get_data() == i);
    }

  for (i = 0; i < n; i++)
    {
      cache_entry = cache.search(i);
      cache.unlock_entry(cache_entry);
      cache.remove(cache_entry);
    }


  cache.expand(n/2);
}
