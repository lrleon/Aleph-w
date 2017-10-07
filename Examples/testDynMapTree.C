
# include <iostream>
# include <btreepic_avl.H>
# include <tpl_dynMapTree.H>

# include <stdlib.h>
# include <autosprintf.h>

using namespace std;
using namespace Aleph;


struct Print_Key
{
  string operator () (const int & i)
  {
    gnu::autosprintf ret("%d", i);
    return ret;
  }
};


template <template <class, class> class Tree>
void test_tree(int n)
{
  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      s.insert(i, i);

    for (int i = 0; i < n; ++i)
      s.remove(i);

    assert(s.is_empty() and s.size() == 0);
  }

  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      {
	int value = rand();
	s.insert(i, value); 
      }
    
    cout << s.size() << " nodes " << endl;
  }

  {
    DynMapTree<int, int, Tree> s;

    for (int i = 0; i < n; ++i)
      {
	s[i]++;
	s[i] += 2;
      }

    assert(s.all([] (auto p) { return p.second == 3; }));
    
    cout << s.size() << " nodes " << endl;
  }
}

int main(int argn, char *argv[])
{ 
  int n = argn > 1 ? atoi(argv[1]) : 1000;

  unsigned int t = time(0);

  if (argn > 2)
    t = atoi(argv[2]);

  srand(t);

  cout << argv[0] << " " << n << " " << t << endl;

  test_tree <Splay_Tree> (n);

  test_tree <Avl_Tree> (n);

  test_tree <Rb_Tree> (n);

  //test_tree <BinTree> (n);

  test_tree <Rand_Tree> (n);
  
  test_tree <Treap> (n);

  test_tree <Treap_Rk> (n);

}
