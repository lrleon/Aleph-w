# include <fstream>
# include <iostream>
# include <topological_sort.H>


typedef List_Digraph<Graph_Node<string>, Graph_Arc<Empty_Class> > Digrafo;

Digrafo::Node * nodo(Digrafo & g, const string & str)
{
  Digrafo::Node * p = g.search_node(str);

  if (p == NULL)
    return g.insert_node(str);

  return p;
}

void build_digraph(Digrafo & g)
{
  g.insert_arc(nodo(g, "A"), nodo(g, "B"));
  g.insert_arc(nodo(g, "A"), nodo(g, "I"));
  g.insert_arc(nodo(g, "A"), nodo(g, "F"));
  g.insert_arc(nodo(g, "B"), nodo(g, "F"));
  g.insert_arc(nodo(g, "C"), nodo(g, "F"));
  g.insert_arc(nodo(g, "C"), nodo(g, "G"));
  g.insert_arc(nodo(g, "D"), nodo(g, "H"));
  g.insert_arc(nodo(g, "F"), nodo(g, "I"));
  g.insert_arc(nodo(g, "F"), nodo(g, "J"));
  g.insert_arc(nodo(g, "G"), nodo(g, "J"));
  g.insert_arc(nodo(g, "H"), nodo(g, "J"));
  g.insert_arc(nodo(g, "H"), nodo(g, "K"));
  g.insert_arc(nodo(g, "H"), nodo(g, "L"));
  g.insert_arc(nodo(g, "I"), nodo(g, "P"));
  g.insert_arc(nodo(g, "J"), nodo(g, "M"));
  g.insert_arc(nodo(g, "J"), nodo(g, "N"));
  g.insert_arc(nodo(g, "K"), nodo(g, "N"));
  g.insert_arc(nodo(g, "K"), nodo(g, "O"));
  g.insert_arc(nodo(g, "L"), nodo(g, "O"));
  g.insert_arc(nodo(g, "L"), nodo(g, "S"));
  g.insert_arc(nodo(g, "M"), nodo(g, "P"));
  g.insert_arc(nodo(g, "M"), nodo(g, "Q"));
  g.insert_arc(nodo(g, "N"), nodo(g, "R"));
  g.insert_arc(nodo(g, "O"), nodo(g, "R"));
  g.insert_arc(nodo(g, "O"), nodo(g, "S"));
}


void imprime_lista(DynDlist<Digrafo::Node*> & l, const string & nom)
{
  ofstream out(nom.c_str(), ios::out);

  for (DynDlist<Digrafo::Node*>::Iterator it(l); it.has_current(); it.next())
    {
      out << it.get_current()->get_info();

      if (not it.is_in_last())
	out << "$\\rightarrow$";
    }

  out << endl;
}


int main()
{
  Digrafo g;

  build_digraph(g);

  DynDlist<Digrafo::Node*> list;

  Topological_Sort <Digrafo> () (g, list);

  imprime_lista(list, "top-1.tex");  

  list.empty();

  Topological_Sort <Digrafo> () (g, list);

  imprime_lista(list, "top-2.tex");  
}
