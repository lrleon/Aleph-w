# include <iostream>
# include <time.h>
# include <aleph.H>
# include <tpl_dynArray.H>
# include <tpl_treapRk.H>
# include <tpl_binNodeUtils.H>

using namespace std;
using namespace Aleph;

DynArray<unsigned long> rand_sequence;

long aleatorio()
{
  // entre 1 y 1000
  unsigned long r = 1+ (int) (1000.0*rand()/(RAND_MAX+1.0));

  rand_sequence[rand_sequence.size()] = r;

  return r;
}

void print_aleatorio_and_reset_dynarray()
{
  cout << endl
       << "Secuencia aleatorios: ";

  for (int i = 0; i < rand_sequence.size(); i++)
    cout << " " << (long) rand_sequence[i];
  
  cout << endl;

  rand_sequence.cut(0);
}

void printNode(Treap_Rk<int>::Node *node, int, int)
{
  cout << node->get_key() << " ";
}

void printPrio(Treap_Rk<int>::Node *node, int, int)
{
  cout << node->getPriority() << " ";
}


int main(int argn, char *argc[])
{
  int n = 10;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);

  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testTreap_Rk " << n << " " << t << endl;

  DynArray<int> keys;
  Treap_Rk<int> tree;
  Treap_Rk<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";

  for (i = 0; i < n; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.search(value);
	}
      while (node not_eq NULL);

      cout << value << " ";
      node = new Treap_Rk<int>::Node (value);
      tree.insert(node);
      keys[i] = value;
    }

  assert(is_treap(tree.getRoot()));

  assert(check_rank_tree(tree.getRoot()));

  cout << endl << endl
       << "Preorden" << endl;

  preOrderRec(tree.getRoot(), printNode);

  cout << endl << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl << endl;

  for (i = 0; i < n; i++)
    {
      node = tree.select(i);
      cout << node->get_key() << " ";
    }

  cout << endl << endl;

  cout << "Lista de posiciones infijas" << endl;

  for (i = 0; i < n; ++i)
    {
      std::pair<int,Treap_Rk<int>::Node*> pos = tree.position(keys[i]);
      cout << keys[i] << "<-->" << pos.first << endl;
    }

  tree.position(rand());

  print_aleatorio_and_reset_dynarray(); 

  cout << endl << endl;

  for (i = 0; i < n/2; i++)
    { 
      do
	{
	  value = (int) (10.0*n*rand()/(RAND_MAX+1.0));
	  node = tree.remove(value);
	} 
      while (node == NULL);

      cout << value << " ";
      delete node;
    }

  cout << endl << "verifying Treap_Rk after deletions ... " 
       << endl;
  assert(is_treap(tree.getRoot()));
  assert(check_rank_tree(tree.getRoot()));
  cout << " done" << endl;

  cout << "Preorden" << endl;
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "inorden prio" << endl;
  inOrderRec(tree.getRoot(), printPrio);
  cout << endl;

  cout << "The path length is " << internal_path_length(tree.getRoot())
       << endl;

  cout << "Recorrido por iterador" << endl;
  for (Treap_Rk<int>::Iterator it(tree); it.has_current(); it.next())
    cout << KEY(it.get_current()) << " ";
  cout << endl;
    
  n = tree.size();
  size_t beg = n/4;
  size_t end = 3*n/4;
  cout << "Eliminacion de rango [" << beg << " .. " << end << "]" << endl;
  Treap_Rk<int>::Node * removed_tree = tree.remove(beg, end);

  assert(is_treap(tree.getRoot()));
  assert(is_treap(removed_tree));

  cout << "Arbol restante" << endl;
  inOrderRec(tree.getRoot(), printNode);
  cout << endl;

  cout << "Arbol eliminado" << endl;
  inOrderRec(removed_tree, printNode);
  cout << endl;

  destroyRec(removed_tree); 
  destroyRec(tree.getRoot()); 

  cout << endl << "testTreap_Rk " << n << " " << t << endl;
}







