
namespace Aleph
{

    template <class Container>
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


template <class Itor>
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
