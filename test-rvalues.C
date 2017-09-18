# include <iostream>
# include <tpl_agraph.H>
# include <tpl_dynMapTree.H>

using namespace std; 

size_t V = 1000;

template <class GT>
GT create_graph()
{
  GT g;
  DynArray<typename GT::Node*> nodes; nodes.reserve(V);
  for (int i = 0; i < V; ++i)
    nodes(i) = g.insert_node(i);

  for (int i = 0; i < V - 1; ++i)
    {
      typename GT::Node * src = nodes(i);
      for (int j = i + 1; j < V; ++j)
	g.insert_arc(src, nodes(j), i + j);
    }

  return g;
}

template <class GT>
bool check(GT & g)
{
  int i = 0;
  for (typename GT::Node_Iterator it(g); it.has_curr(); it.next())
    if (it.get_curr()->get_info() != i++)
      {
	cout << "Inconsistencia en el nodo " << i - 1 << endl;
	abort();
      }

  for (typename GT::Arc_Iterator it(g); it.has_curr(); it.next())
    {
      typename GT::Arc * a = it.get_curr();
      typename GT::Node * src = g.get_src_node(a);
      typename GT::Node * tgt = g.get_tgt_node(a);
      if (a->get_info() != src->get_info() + tgt->get_info())
	{
	  cout << "Inconsistencia en el arco " << a->get_info() << endl;
	  abort();
	}
    }
  
  return true;
}

template <class GT>
void test()
{
  cout << "R value ctor test" << endl;
  GT lg = create_graph <GT> ();
  check(lg);
  cout << "done" << endl << endl;

  {
    cout << "L value ctor test" << endl;
    GT ng = lg;
    check(ng);
    cout << "done" << endl << endl;
  }

  {
    cout << "L value = test" << endl;
    GT lg1;
    lg1 = lg;
    check(lg1);
    cout << "done" << endl << endl;
  }

  cout << "R value = test" << endl;
  lg = create_graph <GT> ();
  check(lg);
  cout << "done" << endl << endl;
}


template <class L>
L create_list(int beg = 0, int end = V - 1)
{
  L l;
  for (int i = beg; i <= end; ++i)
    l.append(i);

  return l;
}


template <class L>
bool check_list(const L & l)
{
  int i = l.get_first();
  for (typename L::Iterator it(l); it.has_curr(); it.next())
    if (it.get_curr() != i++)
      {
	cout << "Inconsistencia en el nodo " << i - 1 
	     << "(" << it.get_curr() << ")" << endl;
	abort();
      }

  return true;
}

template <class L>
void print_list(const L & l)
{
  for (typename L::Iterator it(l); it.has_curr(); it.next())
    cout << it.get_curr() << " ";
  cout << endl;
}

template <class L>
void test_list()
{
  cout << "R value ctor test" << endl;
  L l = create_list <L> ();
  check_list(l);
  cout << "done" << endl << endl;

  {
    cout << "L value ctor test" << endl;
    L ll = l;
    check_list(ll);
    cout << "done" << endl << endl;
  }

  {
    cout << "L value = test" << endl;
    L ll1;
    ll1 = l;
    check_list(ll1);
    cout << "done" << endl << endl;
  }

  cout << "R value = test" << endl;
  l = create_list <L> ();
  check_list(l);
  cout << "done" << endl << endl;
  
  {
    cout << "R value list append test" << endl;
    l.append(create_list <DynList<>> (V, 2*V - 1));
    check_list(l);
    cout << endl;
  }

  {
    cout << "R value list insert test" << endl;
    l.insert(create_list <DynList<>> (-V, -1));
    check_list(l);
    cout << endl;
  }

  {
    cout << "L value list append test" << endl;
    L ll = create_list <DynList<>> (2*V, 3*V - 1);
    l.append(ll);
    check_list(l);
    cout << endl;
  }
  print_list(l);
  {
    cout << "L value list insert test" << endl;
    L ll = create_list <DynList<>> (-2*V-1, -V - 1);
    l.insert(ll);
    print_list(l);
    check_list(l);
    cout << endl;
  }
}


template <class Tree>
void test_map_tree(size_t n)
{
  cout << "Probando con contenedor tipo arbol" << endl;

  typedef void (*Print)(Tree & t);
  Print print = [/* Lambda */] (Tree & t)
    {
      t.for_each([/* Lambda */] (const std::pair<int, int> & p)
		 {
		   cout << p.first << "," << p.second << " "; 
		 });
    } ;

  Tree (*create_tree)(int) = [/* Lambda */] (int n) -> Tree
    {
      Tree t;
      for (int i = 0; i < n; ++i)
	t.insert(i, i+1);
      return t;
    };

  
  Tree tree;
  for (int i = 0; i < n; ++i)
    tree.insert(i, i);

  Tree t1 = tree;
  
  Tree t2 = (*create_tree)(n);

  t2 = (*create_tree)(2*n);

  print(t2) ;

  t1 = t2;
  
  print(t1);

  cout << endl;

  cout << "Probando diferentes combinaciones de insert\n"
       << endl
       << "L val L val\n";

  Tree tt;
  int i = n + 1, j = n + 2;
  tt.insert(i, j);
    
  cout << "\n\nL val R val\n";
  i++;
  tt.insert(i, j + 1);

  cout << "\n\nR val L val\n";
  tt.insert(i + 3, j);

  cout << "\n\nR val R val\n";
  tt.insert(i + 6, j + 7);  

  cout << endl << endl;
  (*print)(tt); cout << endl;
}

int main(int, char * argc[])
{
  if (argc[1])
    V = atoi(argc[1]);

  test_map_tree <DynMapBinTree<int,int>> (V);

    return 0;

  cout << "Testing DynList" << endl;
  test_list <DynList<>> ();
  cout << endl; exit(0);
  
  cout << "Testing List_Graph" << endl;
  test <List_Graph<>> ();
  cout << endl;

  cout << "Testing List_Digraph" << endl;
  test <List_Digraph<>> ();
  cout << endl;

  cout << "Testing List_SGraph" << endl;
  test <List_SGraph<>> ();
  cout << endl;

  cout << "Testing List_SDigraph" << endl;
  test <List_SDigraph<>> ();
  cout << endl;

  cout << "Testing Array_Graph" << endl;
  test <Array_Graph<>> ();
  cout << endl;

  cout << "Testing Array_Digraph" << endl;
  test <Array_Digraph<>> ();
  cout << endl;

}
