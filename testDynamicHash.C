
# include <tpl_linHash.H>
# include <aleph.H>
# include <iostream>
# include <time.h>
# include <stdlib.h>

using namespace std;

static unsigned long hashFct(const unsigned long& key)
{
  return (unsigned long) key;
}

struct Entry :  public LinearHashTableVtl<unsigned long>::Bucket
{
  unsigned long val;

  Entry(unsigned long k, unsigned long v) 
    : LinearHashTableVtl<unsigned long>::Bucket(k), val( v )
    {
      /* EMPTY */
    }
};

int main(int argn, char *argc[])
{ 
  const unsigned long numNodes = 10000;

  unsigned long  i, n = argc[1] ? atol(argc[1]) : numNodes;

  unsigned long  value;

  DynArray<unsigned int> keys(n);

  unsigned int t = time(NULL);

  if (argn > 2)
    t = atol(argc[2]);

  cout << "testDynamicHash " << n << " " << t << endl;

  srand(t);

  LinearHashTableVtl<unsigned long> hashTable(hashFct, 3, 7);
  Entry* bucket;

  cout << "Inserting..." << endl;

  for (i = 0; i < n; i++)
    {
      value = keys[i] = (unsigned long)  (n * (rand() /(RAND_MAX + 1.0))) ;
      bucket = new Entry (keys[i], i);
      hashTable.insert(bucket);
    }
  
  cout << endl << "Number of Expansions  : " << hashTable.getNumExpansions() 
       << endl << "Number of Contractions: "
       << hashTable.getNumContractions() << endl;

  cout << endl << "Searching..." << endl;

  for (i = 0; i < n; i++)
    {
      bucket = static_cast<Entry*>(hashTable.search(keys[i]));
      value = keys[i];
      if (bucket == NULL)
	{
	  cout << endl << "Error key " << keys[i] << " not found" <<  endl;
	  abort();
	}
    }
  
  cout << endl << "Deleting..." << endl;

  try
    {
      for (i = 0; i < n; i++)
	{
	  bucket = static_cast<Entry*>(hashTable.search(keys[i]));
	  if (bucket != NULL)
	    {
	      hashTable.remove(bucket);
	      delete bucket;
	    }
	  else
	    AH_ERROR("%u th key %u not found\n", (int) i, (int) keys[i]);
	}  
    }
  catch (exception& exc)
    {
      cout << exc.what() << " exception has been thrown" << endl;       
    }
  catch (...)
    {
      cout << " unknown exception has been thrown" << endl; 
    }

  cout << endl
       << "Number of Expansions  : " << hashTable.getNumExpansions() << endl
       << "Number of Contractions: " << hashTable.getNumContractions() 
       << endl;
  
  assert(hashTable.getNumItems() == 0);
  
}


