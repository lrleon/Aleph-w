
# include <iostream>
# include <time.h>
# include <stdlib.h>
# include <aleph.H>
# include <tpl_linHash.H>

/* TODO:
   1-. Test del() del iterador

*/

using namespace std;

using namespace Aleph;

struct Entry : public LinearHashTableVtl<unsigned long>::Bucket
{
  unsigned long val;

  Entry(unsigned long k, unsigned long v) 
    : LinearHashTableVtl<unsigned long>::Bucket(k), val( v )
    {
      /* EMPTY */
    }
};

void print_stats( LinearHashTableVtl<unsigned long> & table)
{
  cout << "Capacity  = " << table.capacity() << endl
       << "size       = " << table.size() << endl
       << "busy slots = " << table.busy_slots() << endl
       << "expansions = " << table.expansions() << endl
       << "alpha      = " << 1.0*table.size()/table.capacity() << endl;
}

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

  LinearHashTableVtl<unsigned long> table;
  Entry* bucket;

  print_stats(table);

  cout << "Inserting..." << endl;

  for (i = 0; i < n; i++)
    {
      do
	{
	  value = keys[i] = (unsigned long)  (10*n*(rand() /(RAND_MAX + 1.0)));
	}
      while (table.search(value) != NULL);	  

      cout << value << " ";

      bucket = new Entry (keys[i], i);
      table.insert(bucket);
    }

  cout << endl;

  table.print();
  
  print_stats(table);

  cout << endl << "Searching..." << endl;

  for (i = 0; i < n; i++)
    {
      value = keys[i];
      bucket = static_cast<Entry*>(table.search(value));
      if (bucket == NULL)
	{
	  cout << endl << "Error key " << keys[i] << " not found" <<  endl;
	  abort();
	}
    }

  cout << "Testing iterator" << endl;

  {
    long count = 0;
    for (LinearHashTableVtl<unsigned long>::Iterator it(table); it.has_curr();
	 it.next(), ++count)
      cout << it.get_curr()->get_key() << " ";
    if (count != table.size())
      AH_ERROR("Test not passed count = %ld != %ld", count, table.size());
  }
  
  cout << endl << "testing deleting ..." << endl;

  try
    {
      for (i = 0; i < n; i++)
	{
	  value = keys[i];
	  bucket = static_cast<Entry*>(table.search(value));
	  if (bucket != NULL)
	    {
	      table.remove(bucket);
	      delete bucket;
	    }
	  else
	    AH_ERROR("%u th key %u not found\n", (int) i, (int) keys[i]);
	}
      print_stats(table);
    }
  catch (exception& exc)
    {
      cout << exc.what() << " exception has been thrown" << endl;       
    }
  catch (...)
    {
      cout << " unknown exception has been thrown" << endl; 
    }

  assert(table.size() == 0);  
}


