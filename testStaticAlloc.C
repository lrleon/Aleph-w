
# include <iostream>
# include <staticAlloc.H>
# include <stdlib.h>

using namespace std;

char buffer[4096];

int main()
{
  StaticAlloc alloc(&buffer[0], 4096);
  void *p;
  size_t size;

  for (int i = 0; i < 1000; i++)
    {
      size = 1+(int) (40.0*rand()/(RAND_MAX+1.0));
      try
	{
	  p = alloc.alloc(size);
	}
      catch (std::bad_alloc)
	{
	  cout << "std::bad_alloc exception in "<< i 
	       << " th memory request" << endl
	       << size << " bytes requested" << endl
	       << alloc.getAllocatedSize() << " bytes allocated" << endl; 
	}
    }
}
       
