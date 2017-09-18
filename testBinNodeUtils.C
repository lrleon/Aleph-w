# include <stdlib.h>
# include <time.h>
# include <iostream>
# include <tpl_dynArray.H>
# include <tpl_binTree.H>
# include <tpl_binNodeUtils.H>


using namespace std;
using namespace Aleph;


static void printNode(BinTree<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

static void print_node(BinTree<int>::Node* node, int, int)
{
  cout << node->get_key() << " ";
}

static void print_node(BinTree<int>::Node* node, int, bool)
{
  cout << node->get_key() << " ";
}


DynArray<int> preorder;

static void fill_preorder(BinTree<int>::Node* node, int, int pos)
{
  preorder[pos] = node->get_key();
}


DynArray<int> inorder;

static void fill_inorder(BinTree<int>::Node* node, int, int pos)
{
  inorder[pos] = node->get_key();
}


DynArray<int> postorder;

static void fill_postorder(BinTree<int>::Node* node, int, int pos)
{
  postorder[pos] = node->get_key();
}


struct Clave
{
  int operator () (BinNode<int> * p) const
  {
    return p->get_key();
  }
};


struct Cargar_Clave
{
  void operator () (BinNode<int> * p, istream & input) const
  {
    input >> p->get_key();
  }
};


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

  cout << "testBinNodeUtils " << n << " " << t << endl;

  BinTree<int> tree;
  BinTree<int>::Node *node;
  int i;

  int insCount = 0, delCount = 0;

  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.search(value);
      if (node == NULL)
	{
	  cout << value << " ";
	  insCount++;
	  node = new BinTree<int>::Node (value);
	  tree.insert(node);
	}
    }

  cout << endl << insCount << " Items inserted" << endl;

  /* LLena el arreglo prefijo */
  cout << "Prefijo :";
  int n_preorder = preOrderRec(tree.getRoot(), fill_preorder);
  preOrderRec(tree.getRoot(), printNode);
  cout << endl;

  BinNode<int> * tree_copy = 
    preorder_to_bst<BinNode<int> >(preorder, 0, preorder.size() - 1);

  assert(areEquivalents(tree.getRoot(), tree_copy));

  destroyRec(tree_copy);

  /* LLena el arreglo infijo */
  inOrderRec(tree.getRoot(), fill_inorder);
  cout << "Infijo: ";
  int n_inorder = inOrderRec(tree.getRoot(), print_node);
  cout << endl;

  /* LLena el arreglo sufijo */
  cout << "Sufijo: ";
  postOrderRec(tree.getRoot(), fill_postorder);
  int n_postorder = postOrderRec(tree.getRoot(), print_node);
  cout << endl;

  assert(n_preorder == insCount);
  assert(n_inorder == insCount);
  assert(n_postorder == insCount);

  try /* Copia el arbol binario */
    {
      BinNode<int> * aux = copyRec(tree.getRoot());
 
      assert( areEquivalents(tree.getRoot(), aux) );
	
      destroyRec(aux);
    }
  catch (exception & e) { cout << e.what() << endl; }
  catch (...) { cout << "Unknown exception" << endl; }

  cout << "Recorrido por niveles" << endl;
  level_traverse(tree.getRoot(), [] (BinNode<int> * p)
		 {
		   cout << p->get_key() << " ";
		   return true;
		 });
  cout << endl << endl;

  /* construye un arbol a partir de los recorridos prefijo e infijo */
  BinNode<int> * new_root = build_tree<BinNode, int>
    (preorder, 0, n_preorder - 1, inorder, 0, n_inorder - 1); 

  assert(areEquivalents(new_root, tree.getRoot()));

  BinNode<int> * n_root = build_postorder<BinNode, int>
    (postorder, 0, n_postorder - 1, inorder, 0, n_inorder - 1); 

  assert(areEquivalents(n_root, tree.getRoot()));

  /* Elimina items al azar */
  for (i = 0; i < n; i++)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      node = tree.remove(value);
      if (node not_eq NULL)
	{
	  delCount++;
	  delete node;
	}
    }
    
  cout << delCount << " Items removed" << endl;

  /* Libera la memoria del arbol original */
  try { destroyRec(tree.getRoot()); }
  catch (exception & e) { cout << e.what() << endl; }
  catch (...) { cout << "Unknown exception" << endl; }

  BinNode<int> * t1_rec = NULL, * t2_rec = NULL;

  /* Busque un numero que no este dentro del arbol para particionar */
  while (true)
    {
      value = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
      if (searchInBinTree(new_root, value) == NULL)
	break;
    }

  cout << "Arbol ";
  inOrderRec(new_root, print_node); 
  cout << endl
       << "sera particionado recursivamente segun clave " << value << endl;

  /* particione recursivamente */
  split_key_rec(new_root, value, t1_rec, t2_rec); new_root = NULL;

  BinNode<int> * t1_it = NULL, * t2_it = NULL;

  inOrderRec(t1_rec, print_node); 
  cout << "|" << value << "| ";
  inOrderRec(t2_rec, print_node); 
  cout << endl << endl;

  /* construye un arbol a partir de los recorridos prefijo e infijo */
  new_root = build_tree<BinNode, int>
    (preorder, 0, n_preorder - 1, inorder, 0, n_inorder - 1); 

  cout << "Arbol ";
  inOrderRec(new_root, print_node); 
  cout << endl
       << "sera particionado iterativamente segun clave " << value << endl;

  /* particion recursivamente */
  split_key(new_root, value, t1_it, t2_it);

  inOrderRec(t1_it, print_node); 
  cout << "|" << value << "| ";
  inOrderRec(t2_it, print_node); 
  cout << endl << endl;

  if (not areEquivalents(t1_rec, t1_it))
    AH_ERROR("Lados izquierdos de las particiones no son iguales");

  if (not areEquivalents(t2_rec, t2_it))
    AH_ERROR("Lados derechos de las particiones no son iguales");

  cout << 
    "Resultado de la particion recursiva es identico a la particion iterativa" 
       << endl;

  if (t1_rec not_eq NULL)
    destroyRec(t1_rec);
  if (t2_rec not_eq NULL)
    destroyRec(t2_rec);

  if (t1_it not_eq NULL)
    destroyRec(t1_it);
  if (t2_it not_eq NULL)
    destroyRec(t2_it);

  {
    BinNode<int> * t_rot = NULL;
    DynArray<int> values(n);
    cout << "Insercion recursiva de " << n << " nodos en la raiz ..." << endl;

    for (i = 0; i < n; i++)
      {
	values[i] = 1+(int) (n*100.0*rand()/(RAND_MAX+1.0));
	node = searchInBinTree(t_rot, values[i]);
	if (node == NULL)
	  {
	    cout << values[i] << " ";
	    node = new BinNode<int> (values[i]);
	    t_rot = insert_root(t_rot, node);
	  }
      }

    assert(check_bst(t_rot));

    cout << endl << "Terminado" << endl;

    BinNode<int> * t_it = NULL;
    cout << "Insercion iterativa de " << n << " nodos en la raiz ..." << endl;

    for (i = 0; i < n; i++)
      {
	node = searchInBinTree(t_it, values[i]);
	if (node == NULL)
	  {
	    cout << values[i] << " ";
	    node = new BinNode<int> (values[i]);
	    t_it = insert_root(t_it, node);
	  }
      }

    assert(check_bst(t_it));

    cout << endl << "Terminado" << endl;

    cout << "Comparando resultado recursivo con el iterativo ... " << endl;
    if (areEquivalents(t_rot, t_it))
      cout << "Arboles resultantes son iguales" << endl;
    else
      cout << "Arboles resultantes son diferentes" << endl;

    {
      ofstream out("bintree.tree");
      save_tree(t_it, out);
    }

    ifstream input("bintree.tree");
    BinNode<int> * t_load = load_tree<BinNode<int>>(input);

    cout << "Comparando arbol cargado con el iterativo ... " << endl;
    if (areEquivalents(t_load, t_it))
      cout << "Arboles resultantes son iguales" << endl;
    else
      cout << "Arboles resultantes son diferentes" << endl;
    
    destroyRec(t_rot);
    destroyRec(t_it);
  }
}
