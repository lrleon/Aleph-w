# include <iostream>
# include <tpl_graph.H>
# include <Tarjan.H>

struct Nodo
{
  string nombre;

  Nodo() {}

  Nodo(const string & str) : nombre(str) { /* empty */ }

  Nodo(char * str) : nombre(str) { /* empty */ }

  bool operator == (const Nodo & der) const 
  {
    return nombre == der.nombre; 
  }

  operator const std::string & () { return nombre; }
};


typedef Graph_Node<Nodo> Node_Nodo;

typedef Graph_Arc<long> Arco_Arco;

typedef List_Graph<Node_Nodo, Arco_Arco> Digrafo;


void insertar_arco(Digrafo &        grafo, 
		   const string & src_name, 
		   const string & tgt_name)
{
  Digrafo::Node * n1 = grafo.find_node(Nodo(src_name));

  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  Digrafo::Node * n2 = grafo.find_node(Nodo(tgt_name));

  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2);
}



void build_test_graph_1(Digrafo & g)
{
  g.emplace_node("E");

  insertar_arco(g, "A", "B");
  insertar_arco(g, "A", "D");
  insertar_arco(g, "B", "C");
  insertar_arco(g, "C", "A");
  insertar_arco(g, "D", "E");
  insertar_arco(g, "E", "B");
  insertar_arco(g, "D", "C");

  insertar_arco(g, "E", "G");

  insertar_arco(g, "G", "F");
  insertar_arco(g, "F", "G");

  insertar_arco(g, "E", "H");

  insertar_arco(g, "H", "I");

  insertar_arco(g, "I", "J");
  insertar_arco(g, "J", "K");
  insertar_arco(g, "K", "I");
  insertar_arco(g, "K", "L");
  insertar_arco(g, "L", "I");

  insertar_arco(g, "K", "O");
  insertar_arco(g, "O", "N");
  insertar_arco(g, "N", "M");
  insertar_arco(g, "O", "N");
  insertar_arco(g, "M", "O");
}

struct Directed
{
  Digrafo::Node * s;
    bool operator () (Digrafo::Arc * a) const
    { 
      return a->src_node == s;
    }
  Directed(Digrafo::Node * __s) : s(__s) { }
  };

# define INDENT "    "

void print_graph(Digrafo & g)
{
  for (Digrafo::Node_Iterator node_it(g); node_it.has_curr(); node_it.next())
    {
      Digrafo::Node * src = node_it.get_current_node();
      cout << src->get_info().nombre << endl;
      cout << "src = " << src->get_info().nombre << endl;
      for_each_in_arc<Digrafo>(src, [&g] (auto a)
 // for (Digrafo::Out_Iterator it(src); it.has_curr(); it.next())
	{
	  auto s = g.get_src_node(a);
	  auto t = g.get_tgt_node(a);
	  cout << s->get_info().nombre << " --> " << t->get_info().nombre;
	  cout << endl;
	  return true;
	});
    }
  cout << endl;
}


int main()
{
  Digrafo g;

  build_test_graph_1(g);

  print_graph(g);
}
