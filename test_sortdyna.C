
# include <iostream>
# include <ahSort.H>

using namespace std;
using namespace Aleph;

int main(int argn, char *argc[])
{
  int n = argc[1] ? atoi(argc[1]) : 10;

  unsigned int t = 0;

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << argc[0] << " " << n << " " << t << endl;

  DynArray<int> array;

  for (int i = 0; i < n; ++i)
    array[i] = rand() % 1000;
  
  in_place_sort(array);

  for (int i = 0; i < n; ++i)
    {
      cout << array[i] << " ";
      // if (i > 0)
      // 	if (array[i] < array[i - 1])
      // 	  AH_ERROR("a[%d] = %d is lesser than a[%d] = %d", i, i + 1, 
      // 		array[i], array[i - 1]);
    }

  cout << endl;
}
