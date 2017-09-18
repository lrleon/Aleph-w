
# include <iostream>
# include <hash-fct.H>

using namespace Aleph;
using namespace std;


template <typename Key, typename Data, size_t (*fct)(const Key & k)>
size_t wrapper(const std::pair<Key, Data> & p)
{
  return (*fct)(p.first);
}

int main(int argc, char *argv[])
{
  size_t val = 
    wrapper<int, int, dft_hash_fct<int>>(std::pair<int,int>(5,9));

  cout << val << endl;

  size_t (*fct)(const std::pair<int, int>&) = 
    wrapper<int, int, dft_hash_fct<int>>;

  cout <<  (*fct)(std::pair<int, int>(4,6)) << endl;

  return 0;
}
