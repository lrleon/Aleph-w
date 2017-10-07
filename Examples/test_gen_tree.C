# include <iostream>
# include <tpl_graph.H>
# include <graph_to_tree.H>
# include <generate_tree.H>

using namespace std;

# define INDENT "    "

using namespace Aleph;

struct Ciudad 
{
  enum Tipo_Ciudad { CAPITAL, CIUDAD, PUEBLO, CASERIO, CRUZ, DESCONOCIDO };

  string nombre;

  Tipo_Ciudad tipo;

  Ciudad() : tipo(DESCONOCIDO) { /* empty */ }

  Ciudad(const Ciudad & c) : nombre(c.nombre), tipo(c.tipo) { /* empty */ }

  Ciudad(const char * nom) : nombre(nom), tipo(DESCONOCIDO) { /* empty */ }

  Ciudad(char * nom) : nombre(nom), tipo(DESCONOCIDO) { /* empty */ }

  Ciudad(const string & str) : nombre(str), tipo(DESCONOCIDO) { /* empty */ }

  bool operator == (const Ciudad & c) const
  {
    return nombre == c.nombre;
  }
};

struct Via
{ 
  enum Tipo_Via { AUTOPISTA, CARRETERA1, CARRETERA2, CARRETERA3, GRANZON,
		  CHALANA, DESCONOCIDO };

  string nombre;
  int distancia; 
  Tipo_Via tipo;

  Via() : tipo(DESCONOCIDO) {}

  Via(int d) 
    : nombre("Desconocido"), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  Via(char * nom, int d) 
    : nombre(nom), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  Via(const string& nom, int d) 
    : nombre(nom), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  typedef int Distance_Type;

  Distance_Type& get_distance() { return distancia; }

  static const Distance_Type Zero_Distance = 0;
};

typedef Graph_Node<Ciudad> Nodo_Ciudad;

typedef Graph_Arc<Via> Arco_Via;


typedef List_Graph<Nodo_Ciudad, Arco_Via> Mapa;

typedef List_Digraph<Nodo_Ciudad, Arco_Via> Dimapa;



struct Ciudad_Igual
{
  bool operator () (const Ciudad & c1, const Ciudad & c2)
  {
    return c1.nombre == c2.nombre;
  }
};


Mapa::Node * buscar_ciudad(Mapa& mapa, const string & nombre)
{
  return mapa.search_node([&nombre] (Mapa::Node * p)
			{
			  return p->get_info().nombre == nombre;
			});
}

void insert_via(Mapa& mapa, 
		const string & c1, const string & c2,
		int distancia)
{
  Mapa::Node * n1 = buscar_ciudad(mapa, c1);

  if (n1 == NULL)
    n1 = mapa.insert_node(c1);

  Mapa::Node * n2 = buscar_ciudad(mapa, c2);

  if (n2 == NULL)
    n2 = mapa.insert_node(c2);

  string nombre_arco = n1->get_info().nombre + "--" + n2->get_info().nombre;

  mapa.insert_arc(n1, n2, Via(nombre_arco, distancia));
}


void imprimir_camino(Path<Mapa>& path)
{
  cout << endl
       << "Camino: ";
  for (Path<Mapa>::Iterator itor(path); itor.has_current(); itor.next())
    cout << itor.get_current_node()->get_info().nombre << "-";

  cout << endl;
}


void imprimir_mapa(Mapa & g)
{
  cout << endl
       << "Listado de nodos (" << g.get_num_nodes() << ")" << endl;

  for (Mapa::Node_Iterator node_itor(g); node_itor.has_current(); 
       node_itor.next())
    cout << INDENT << node_itor.get_current_node()->get_info().nombre << endl;

  cout << endl
       << endl
       << "Listado de arcos (" << g.get_num_arcs() << ")"
       << endl;

  for (Mapa::Arc_Iterator arc_itor(g); arc_itor.has_current();
       arc_itor.next())
    {
      Mapa::Arc * arc = arc_itor.get_current_arc();
      cout << arc->get_info().nombre << " " << arc->get_info().distancia 
	   << " de " << g.get_src_node(arc)->get_info().nombre 
	   << " a " << g.get_tgt_node(arc)->get_info().nombre << endl;
    }

  cout << endl
       << endl
       << "Listado del grafo por nodos y en cada nodo por arcos" 
       << endl;
  for (Mapa::Node_Iterator node_itor(g); node_itor.has_current(); 
       node_itor.next())
    {
      Mapa::Node * src_node = node_itor.get_current_node();
      cout << src_node->get_info().nombre << endl;
      for (Mapa::Node_Arc_Iterator itor(node_itor.get_current_node());
	   itor.has_current(); itor.next())
	{
	  Mapa::Arc * arc = itor.get_current_arc();
	  cout << INDENT << arc->get_info().distancia << " " 
	       << g.get_connected_node(arc, src_node)->get_info().nombre 
	       << endl;
	}
    }
  cout << endl;
}

void construir_mapa(Mapa& g)
{
  insert_via(g, "San Cristobal", "La Fria", 69);
  insert_via(g, "San Cristobal", "Sacramento", 113);
  insert_via(g, "San Cristobal", "San Antonio", 36);
  insert_via(g, "Rubio", "Caparo", 150);
  insert_via(g, "La Fria", "El Vigia", 86);
  insert_via(g, "El Vigia", "Santa Barbara", 59);
  insert_via(g, "El Vigia", "Merida", 79);
  insert_via(g, "La Fria", "Machiques", 252);
  insert_via(g, "Valera", "Merida", 167);
  insert_via(g, "Valera", "Carora", 120);
  insert_via(g, "Carora", "Barquisimeto", 102);
  insert_via(g, "Merida", "Barinas", 180);
  insert_via(g, "Barinas", "Guanare", 94);
}


  template <typename GT>
struct GT_Tree
{
  void operator () (typename GT::Node * g, Tree_Node<string> * t)
  {
    t->get_key() = g->get_info().nombre;
  }
};


struct Write_Ciudad
{
  const string operator () (Tree_Node<string> * p)
  {
    return p->get_key();
  }
};


int main()
{
  Mapa tree;

  construir_mapa(tree);

  imprimir_mapa(tree);

  Mapa::Node * c = buscar_ciudad(tree, "Merida");

  Tree_Node<string> * t = 
    Graph_To_Tree_Node <Mapa, string, GT_Tree<Mapa> > () (tree, c);

  ofstream test("prueba.Tree", ios::trunc);

  Aleph::generate_tree<Tree_Node<string>, Write_Ciudad> (t, test);

  delete t;
}
