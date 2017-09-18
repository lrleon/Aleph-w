
# include <iostream>
# include <tpl_dynLinHash.H>

using namespace Aleph;
using namespace std;

size_t hash_fct(const size_t & key)
{
  return key;
}


static void printPars(const DynLinearHashTable<size_t, size_t> & table)
{
  cout << "Table length = " << table.capacity() << endl
       << "Busy slots   = " << table.busy_slots() << endl
       << "Num items    = " << table.size() << endl
       << "Expansions   = " << table.expansions() << endl
       << "alpha        = " << (1.0* table.size())/(1.0*table.capacity())
       << endl;
}


int main(int argn, char *argc[])
{
  unsigned long n = argn > 1 ? atoi(argc[1]) : 1000;
  unsigned int  t = argn > 2 ? atoi(argc[2]) : time(NULL);
  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  {
    DynLinearHashTable<size_t, size_t> table(hash_fct, 10, 0.2, 0.5);

    DynArray<size_t> keys(n);

    for (int i = 0; i < n; i++)
      {
	keys[i] = rand();

	table.insert(keys[i], i);

	if (i % 1000 == 0)
	   printPars(table);
      }

    printPars(table);

    size_t * ptr;

    for (size_t i = 0; i < n; i++)
      {
	size_t k = keys[i];

	ptr = table.search(k);

	if (ptr != NULL)
	  table.remove(ptr);

	if (i % 1000 == 0)
	  printPars(table);
      }

    printPars(table);
  }
}
