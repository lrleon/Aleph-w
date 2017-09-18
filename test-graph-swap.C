# include <tpl_agraph.H>


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

  return std::move(g);
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
	  cout << "Inconsistencia en el nodo " << i - 1 << endl;
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


int main(int, char * argc[])
{
  if (argc[1])
    V = atoi(argc[1]);
  
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
