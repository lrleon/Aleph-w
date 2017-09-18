  
# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <aleph.H>
# include <tpl_avlTreeThRkC.H>

using namespace ThreadNode_Utils;

using namespace std;

long long int sumaPorHilos = 0;
int cont = 0;
long long int sumaInOrderRec = 0; 

static void print(AvlTreeThRk<int>::Node *p) 
{
  if(!(cont%10))sumaInOrderRec += p->get_key();  
  signed char d = p->getDiff();
  cout << "(" << p->get_key() << "," << (int)d << ")";
  cont++;
}


int main(int argn, char *argc[])
{
  int n = 1000;
  unsigned int t = time(0);
  int value;

  if (argn > 1)
    n = atoi(argc[1]);
  if (argn > 2)
    t = atoi(argc[2]);

  srand(t);

  cout << "testAvlThRkTree " << n << " " << t << endl;

  AvlTreeThRk<int> tree;
  AvlTreeThRk<int>::Node *node;
  int i;

  cout << "Inserting " << n << " random values in treee ...\n";
  int j,k,m;
  k = 0;
  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node != NULL)
	cout << "(" << value << ")";
      else
	{
	  cout << ".";
	  node = new AvlTreeThRk<int>::Node (value);
	  tree.insert(node);
          k++;
	}
    }
  cout << endl << "verifying avl Rk tree after insertions ... " 
       << endl;
  assert(tree.verifyAvl());
  assert(tree.verifyRank());
  cout << " done" << endl;
  
  AvlTreeThRk<int> tree2;

  cout << "Inserting " << n/2 << " random values in treee ...\n";
  
  for (i = 0; i < n/3 ; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree2.search(value);
      if (node != NULL)
	cout << "(" << value << ")";
      else
	{
	  cout << ".";
	  node = new AvlTreeThRk<int>::Node (value);
	  tree2.insert(node);
          
	}
    }
  cout << endl << "verifying avl Rk tree after insertions ... " 
       << endl;
  assert(tree2.verifyAvl());
  assert(tree2.verifyRank());
  cout << " done" << endl;
  
  tree.concatenate(tree2);
  assert(tree2.verifyAvl());
  assert(tree2.verifyRank());
  assert(tree.verifyAvl());
  assert(tree.verifyRank());
  
  m = 0;
  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node == NULL)
	cout << "(" << value << ")" ;
      else
	{
	  delete node;
          m++;
	  // assert(tree.verifyAvl());
	  cout << "." ;
	}
    }
    
  cout << endl << "verifying avl tree after deletions ... " ;
  assert(tree.verifyAvl());
  assert(tree.verifyRank());
  cout << " done" << endl;
  
  AvlTreeThRk<int>::Iterator itor(tree);

  cout<< " Recorrido de Arbol usando hilos...."<< endl;
  j = 0;
  
  for (itor.reset_first(); itor.has_current(); itor.next())
    {
      node = itor.get_current();
      cout << "[" << node->get_key() << "] ";
      if(!(j%10)) sumaPorHilos += node->get_key();
      j++;
    }

  // preOrderStack(tree.getRoot(), print);
  inOrderRec(tree.getRoot(), print);
  printf("\n");
  
cout<<endl; 
  cout<< "    Nodos recorridos:         "<<j<<endl
      << "    Nodos insertados:         "<<k<<endl
      << "    Nodos eliminados:         "<<m<<endl
      << "    Nodos restantes:          "<<(k-m)<<endl
      << "    Suma en recorrido rec:    "<<sumaInOrderRec<<endl
      << "    Suma en recorrido hilado: "<<sumaPorHilos<<endl<<endl;
  
  cout << endl << "verifying avl tree search codition ... " ;
  //assert(check_bst(tree.getRoot())); 
  cout << " done" << endl<<endl;
  cout << endl << "verifying avl Rk tree after deletions ... " ;
  assert(tree.verifyAvl());
  //assert(tree.verifyRank());
  cout << " done" << endl;
  cout << "Numero de nodos del arbol: "<<tree.size()<<endl;  
  destroyRec(tree.getRoot());
  cout << "sizeof(AvlTreeThRk::Node)=" << sizeof(AvlTreeThRk<int>::Node) << endl;
  cout << "testAvlTreeThRk " << n << " " << t << endl;


}





