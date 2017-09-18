# include <stdlib.h>

# include <iostream>
# include <btreepic_avl.H>
# include <tpl_dynSetTree.H>

# include <autosprintf.h>

using namespace std;
using namespace Aleph;

struct Imprimir
{
  int i; 

  Imprimir() : i(0) { /* empty */ }

  void operator () (const int & k)
  {
    cout << "(" << k << "," << i++ << ") ";
  }
};

struct Verificar
{
  int pos;
  int m;
  int last_key;

  Verificar(int __m) : pos(0), m(__m) { /* empty */ }

  void operator () (const int & k) 
  {
    int old_key = last_key;
    last_key = k;
    if (pos++ % m == 0)
      return;

    if (last_key != k)
      AH_ERROR("Clave %d en posicion %d es distinta clave %d", 
	    k, pos - 1, old_key);
  }
};


struct Print_Key
{
  string operator () (const int & i)
  {
    gnu::autosprintf ret("%i ", i);
    return ret;
  }
};


template <template <class, class> class Tree>
void iterate(DynSetTree<int, Tree> & tree)
{
  for (typename DynSetTree<int, Tree>::Iterator it(tree); it.has_curr(); 
       it.next())
    cout << it.get_curr() << " ";
  cout << endl << endl;
}

template <template <class, class> class Tree>
DynSetTree<int, Tree> test_tree(int n, int m)
{
  {
    DynSetTree<int, Tree> s;

    for (int i = 0; i < n; ++i)
      s.insert(i);

    assert(compute_cardinality_rec(s.get_root_node()) == s.size());
    assert(s.verify());

    for (int i = 0; i < n; ++i)
      s.remove(i);

    int i = 0;
    s.for_each([&i] (int k)
	       {
		 cout << "(" << k << "," << i++ << ") ";
	       });

    cout << endl;
  }

  {
    DynSetTree<int, Tree> s;

    int counter = 0;

    for (int i = 0; i < n; ++i)
      {
	int value = rand() % 1000;
	for (int j = 0; j < m; ++j)
	  { s.insert_dup(value); counter++; }

	for (int j = 0; j < m/4; ++j)
	  {
	    int c = s.remove(value); counter--;
	    assert(c == counter);
	  } 
      }

    assert(compute_cardinality_rec(s.get_root_node()) == s.size());
    assert(s.size() == n*(m - m/4));
    assert(s.verify());

    int i = 0;
    s.for_each ([&i] (int k)
		{
		 cout << "(" << k << "," << i++ << ") ";
		});
    s.template for_each <Verificar> (Verificar(m));

    cout << endl;
  }
  
  DynSetTree<int, Tree> aux;

  {
    DynSetTree<int, Tree> s;

    for (int i = 0; i < n; ++i)
      for (int j = 0; j < m; ++j)
	{
	  s.insert_dup(i);
	  int value = rand() % (i + 1);
	  assert(s.exist(value));
	}

    assert(compute_cardinality_rec(s.get_root_node()) == s.size());

    DynSetTree<int, Tree> s1;
    DynSetTree<int, Tree>   aux1 = DynSetTree<int, Tree> (s1) ;
    Imprimir imp;
    aux1.template for_each <Imprimir> (imp);

    for (int i = 0; i < n; ++i)
      assert(s.exist(rand() % n));

    for (int i = 0; i < n; ++i)
      s.remove(i);

    assert(s.verify());
    assert(compute_cardinality_rec(s.get_root_node()) == s.size());
    assert(s.size() == n*m - n);

    s.template for_each <Imprimir> (imp);

    s.template for_each <Verificar> (Verificar(m));

    cout << endl;

    aux.swap(s);
  }

  DynSetTree<int, Tree> ret;
   for (int i = 0; i < n; ++i)
      ret.insert(i);
  return ret;
}

int main(int argn, char *argv[])
{ 
  int n = argn > 1 ? atoi(argv[1]) : 1000;

  int m = argn > 2 ? atoi(argv[2]) : 10;

  unsigned int t = time(0);

  if (argn > 3)
    t = atoi(argv[3]);

  srand(t);

  cout << argv[0] << " " << n << " " << m << " " << t << endl;

  {
    DynSetTree<int, Splay_Tree> tree = test_tree <Splay_Tree> (n, m);
  }

  {
    DynSetTree<int, Avl_Tree> tree = test_tree <Avl_Tree> (n, m);
  }
  {
    DynSetTree<int, Rb_Tree> tree = test_tree <Rb_Tree> (n, m);
  }
  {
    DynSetTree<int, BinTree> tree = test_tree <BinTree> (n, m);
  }
  {
    DynSetTree<int, Rand_Tree> tree = test_tree <Rand_Tree> (n, m);
    cout << "**** Iterate" << endl;
    iterate<Rand_Tree>(tree);
    cout << endl;
  }
  {
    DynSetTree<int, Treap> tree = test_tree <Treap> (n, m);
  }
  {
    DynSetTree<int, Treap_Rk> tree = test_tree <Treap_Rk> (n, m);
  }
}
