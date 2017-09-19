#ifndef PRINT_CONTAINER_H
#define PRINT_CONTAINER_H

# include <iostream>

using namespace std;

namespace Aleph
{

    template <class Container> inline
void print_container(Container & c)
{
  if (c.size() == 0)
    {
      std::cout << "Container is empty" << std::endl;
      return;
    }

  cout << "(" <<  c.size() << ") " ;

  typename Container::iterator beg = c.begin();
  const typename Container::iterator end = c.end();

  for (typename Container::iterator it(beg); it != end; /* empty */)
    std::cout << *it++ << " ";

  std::cout << std::endl 
	    << std::endl; 
}


template <class Itor> inline
void print_range(Itor beg, const Itor & end)
{
  int n = 0;
  while (beg != end)
    {
      cout << *beg++ << " ";
      ++n;
    }

  std::cout << std::endl 
	    << "len = " << n << std::endl
	    << std::endl; 
}


}



#endif


