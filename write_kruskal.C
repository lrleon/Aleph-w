# include <autosprintf.h>
# include <Kruskal.H>
# include <generate_spanning_tree_picture.H>

struct Nodo 
{
  string str;

  Nodo(const int & c)
  {
    str = c;
  }

  Nodo(const string & s) : str(s)
  {
    // empty
  }

  bool operator == (const Nodo & c) const
  {
    return c.str == str;
  }
};


struct Arco
{
  int w;

  Arco(const int & __w) : w(__w) { /* empty */ }

  Arco() { /* empty */ }
};


typedef List_Graph<Graph_Node<Nodo>, Graph_Arc<Arco> > Grafo;


struct Distancia
{
  typedef int Distance_Type;

  static const Distance_Type Zero_Distance = 0;

  int operator () (Grafo::Arc * a) const
  {
    return a->get_info().w;
  }
};


struct Nodo_String
{
  string operator () (Grafo::Node * p) const
  { 
    return p->get_info().str;
  }
};


struct Arco_String
{
  string operator () (Grafo::Arc * a) const
  { 
    return gnu::autosprintf("%d", a->get_info().w);
  }
};


Grafo::Node * bn(Grafo * g, const string & str)
{
  return g->search_node(Nodo(str));
}


void insertar_arco(Grafo *        g,
		   const string & s1, 
		   const string & s2, 
		   const int &    i)
{
  Grafo::Node * src = bn(g, s1);
  Grafo::Node * tgt = bn(g, s2);

  g->insert_arc(src, tgt, i);
}


Grafo * build_graph()
{
  Grafo * g = new Grafo;

  for (int i = 'A'; i <= 'P'; ++i)
    g->insert_node(new Grafo::Node(i));

  insertar_arco(g, "A", "B", 3);
  //  insertar_arco(g, "B", "A", 3);
  insertar_arco(g, "B", "C", 4);
  insertar_arco(g, "C", "D", 3);
  insertar_arco(g, "D", "E", 4);
  insertar_arco(g, "E", "K", 2);
  insertar_arco(g, "K", "P", 7);
  insertar_arco(g, "P", "J", 1);
  insertar_arco(g, "J", "K", 4);
  insertar_arco(g, "K", "D", 5);
  insertar_arco(g, "D", "J", 2);
  insertar_arco(g, "J", "I", 2);
  //  insertar_arco(g, "I", "J", 2);
  insertar_arco(g, "I", "D", 1);
  insertar_arco(g, "I", "C", 2);
  insertar_arco(g, "I", "H", 2);
  insertar_arco(g, "H", "C", 3);
  insertar_arco(g, "H", "B", 7);
  insertar_arco(g, "B", "G", 1);
  insertar_arco(g, "B", "M", 15);
  //  insertar_arco(g, "M", "B", 3);
  insertar_arco(g, "M", "G", 10);
  insertar_arco(g, "G", "A", 4);
  insertar_arco(g, "A", "F", 9);
  insertar_arco(g, "F", "G", 5);
  insertar_arco(g, "F", "L", 10);
  insertar_arco(g, "F", "M", 12);
  insertar_arco(g, "H", "M", 8);
  insertar_arco(g, "L", "M", 4);
  insertar_arco(g, "M", "N", 2);
  insertar_arco(g, "N", "H", 3);
  insertar_arco(g, "N", "I", 1);
  insertar_arco(g, "N", "O", 3);
  insertar_arco(g, "O", "I", 3);
  insertar_arco(g, "O", "J", 1);
  insertar_arco(g, "O", "P", 6);

  return g;    
}


    template <class GT, class Distance, class Compare> inline 
void write_kruskal_min_spanning_tree(GT & g, GT & tree)
{
  if (g.is_digraph())
    throw std::domain_error("g is a digraph");

  if (not test_connectivity(g))
    throw std::invalid_argument("Input graph is not conected");

  g.reset_bit_nodes(Aleph::Spanning_Tree);

  clear_graph(tree); // limpia grafo destino 

      // ordena arcos segun compare 
  g.template sort_arcs<Distance_Compare<GT, Distance, Compare> >(); 

  int i = 0, j = 0;

      /* Recorrer arcos ordenados de g hasta que numero de arcos de tree
	 sea igual a numero de nodos de g */     
  for (typename GT::Arc_Iterator arc_itor(g);       // Inicio
       tree.get_num_arcs() < g.get_num_nodes() - 1; // predicado
       arc_itor.next())                             // avance
    {
          // obtenga siguiente menor arco
      typename GT::Arc * arc = arc_itor.get_current_arc();

          // procesamiento de primer nodo del arco; con src
      typename GT::Node * g_src_node = g.get_src_node(arc);

      typename GT::Node * tree_src_node; // Nodo origen en tree
      if (not IS_NODE_VISITED(g_src_node, Aleph::Spanning_Tree)) 
	{     // No, crearlo en tree, atarlo al cookie y marcarlo visitado
	  NODE_BITS(g_src_node).set_bit(Aleph::Spanning_Tree, true); 
	  tree_src_node = tree.insert_node(g_src_node->get_info());
	  NODE_COOKIE(g_src_node)    = tree_src_node; 
	  NODE_COOKIE(tree_src_node) = g_src_node; 
	}
      else
	tree_src_node = 
	  static_cast<typename GT::Node*>(NODE_COOKIE(g_src_node));

          // procesamiento del otro nodo del arco; con tgt
      typename GT::Node * g_tgt_node = g.get_tgt_node(arc);
      typename GT::Node * tree_tgt_node; // Nodo destino en arbol abarcador
      if (not IS_NODE_VISITED(g_tgt_node, Aleph::Spanning_Tree))
	{ // No, crearlo tree y atarlo al cookie
	  NODE_BITS(g_tgt_node).set_bit(Aleph::Spanning_Tree, true); 
	  tree_tgt_node = tree.insert_node(g_tgt_node->get_info());
	  NODE_COOKIE(g_tgt_node)    = tree_tgt_node;
	  NODE_COOKIE(tree_tgt_node) = g_tgt_node;
	}
      else
	tree_tgt_node = 
	  static_cast<typename GT::Node*>(NODE_COOKIE(g_tgt_node));

          // inserte nuevo arco en tree 
      typename GT::Arc * arc_in_tree = 
	tree.insert_arc(tree_src_node, tree_tgt_node, arc->get_info());

      if (has_cycle(tree)) // verifique si nuevo arco causa un ciclo
	{    // Si
	  tree.remove_arc(arc_in_tree); // sacar arco e ir a procesar 
	  continue;                     // siguiente arco
	}

          // Actualice mapeo de arcos
      ARC_COOKIE(arc)         = arc_in_tree;
      ARC_COOKIE(arc_in_tree) = arc;

      if (i++ % 3 == 0)
	{
	  string name = gnu::autosprintf("kruskal-%d-aux.gra", j++);
	  ofstream out(name, ios::out);

	  generate_cross_spanning_tree
	    <Grafo, Nodo_String, Arco_String> (g, 6, 20, 20, out);
	}
    }

  string name = gnu::autosprintf("kruskal-%d-aux.gra", j++);
  ofstream out(name, ios::out);

  generate_cross_spanning_tree
    <Grafo, Nodo_String, Arco_String> (g, 6, 20, 20, out);
} 


    template <class GT, class Distance> inline 
void write_kruskal_min_spanning_tree(GT & g, GT & tree)
{
  typedef typename Distance::Distance_Type DT;

  write_kruskal_min_spanning_tree<GT, Distance, Aleph::less<DT> > (g, tree);
}



int main()
{
  Grafo * g = build_graph();

  Grafo tree1;

  write_kruskal_min_spanning_tree<Grafo, Distancia>(*g, tree1);
}

