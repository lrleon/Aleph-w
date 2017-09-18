# include <iostream>

# include <tpl_dynBinHeap.H>
# include <tpl_dynListQueue.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <Prim.H>
# include <Kruskal.H>

# define INDENT "    "

using namespace Aleph;

struct Ciudad 
{
  enum Tipo_Ciudad { CAPITAL, CIUDAD, PUEBLO, CASERIO, CRUZ, DESCONOCIDO };

  string nombre;

  Tipo_Ciudad tipo;

  Ciudad() : tipo(DESCONOCIDO) { /* empty */ }

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

  Via(const string& nom, int d) 
    : nombre(nom), distancia(d), tipo(DESCONOCIDO) { /* empty */ }
};


typedef Graph_Node<Ciudad> Nodo_Ciudad;

typedef Graph_Arc<Via> Arco_Via;


typedef List_Graph<Nodo_Ciudad, Arco_Via> Mapa;

typedef List_Digraph<Nodo_Ciudad, Arco_Via> Dimapa;


struct Distancia_Via
{
  typedef int Distance_Type;

  static const Distance_Type Zero_Distance = 0;

  static const Distance_Type Max_Distance;

  Distance_Type operator () (Mapa::Arc * a) const
  {
    return a->get_info().distancia;
  }
};


const int Distancia_Via::Max_Distance = INT_MAX;


struct Ciudad_Igual
{
  bool operator () (const Ciudad & c1, const Ciudad & c2) const
  {
    return c1.nombre == c2.nombre;
  }

  bool operator () (const Ciudad & c1, const string & nom) const
  {
    return c1.nombre == nom;
  }
};


struct Arco_Igual
{
  bool operator () (const Via & v1, const Via & v2) const
  {
    return true;
  }
};


Mapa::Node * buscar_ciudad(Mapa & mapa, const string & nombre)
{
  return mapa.search_node <string, Ciudad_Igual> (nombre);
}

bool visitar(Mapa & g, Mapa::Node * node, Mapa::Arc * arc)
{
  cout << "Estoy en  " << node->get_info().nombre << " viniendo desde " 
       << (arc != NULL ? g.get_connected_node(arc, node)->get_info().nombre : 
	   "NULO") << endl ;

  return false;
}

struct Visitar
{
  Mapa::Node * dest;

  Visitar(Mapa::Node * tgt = NULL) : dest(tgt) { /* empty */ }

  bool operator () (Mapa & g, Mapa::Node * p, Mapa::Arc * a)
  {
    cout << "Estoy en  " << p->get_info().nombre << " viniendo desde " 
	 << (a != NULL ? g.get_connected_node(a, p)->get_info().nombre : 
	     "NULO") << endl ;

    if (dest != NULL)
      if (p == dest)
	{
	  cout << "Se alcanzó el nodo destino " << dest->get_info().nombre 
	       << endl;
	  return true;
	}

  return false;
  }
};

struct Contar_Arcos
{
  int count;

  Contar_Arcos() : count (0) { /* empty */ }

  bool operator () (Mapa::Arc * a)
  {
    count++;
    return true;
  }
};


struct Total_Dist
{
  int dist;

  Total_Dist () : dist(0) { /* empty */ }

  bool operator () (Mapa::Arc * a)
  {
    if (a != NULL)
      dist += a->get_info().distancia;

    return true;
  }
};

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


typedef Path<Mapa> Camino;

void imprimir_camino(Camino & path)
{
  cout << endl
       << "Camino: ";

  for (Camino::Iterator itor(path); itor.has_current(); itor.next())
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
  insert_via(g, "San Cristobal", "Rubio", 22);
  insert_via(g, "Rubio", "San Antonio", 48);
  insert_via(g, "Rubio", "Caparo", 150);
  insert_via(g, "Sacramento", "El Canton", 38);
  insert_via(g, "La Fria", "El Vigia", 86);
  insert_via(g, "El Vigia", "Santa Barbara", 59);
  insert_via(g, "El Vigia", "Merida", 79);
  insert_via(g, "La Fria", "Machiques", 252);
  insert_via(g, "Machiques", "Maracaibo", 130);
  insert_via(g, "Machiques", "Santa Barbara", 295);
  insert_via(g, "Maracaibo", "Paraguaipos", 55);
  insert_via(g, "Maracaibo", "Coro", 254);
  insert_via(g, "Maracaibo", "Valera", 201);
  insert_via(g, "Valera", "Merida", 167);
  insert_via(g, "Valera", "Carora", 120);
  insert_via(g, "Carora", "Barquisimeto", 102);
  insert_via(g, "Merida", "Barinas", 180);
  insert_via(g, "Barinas", "Caparo", 200);
  insert_via(g, "Barinas", "Guanare", 94);
  insert_via(g, "Caracas", "Barcelona", 310);
  insert_via(g, "Caracas", "San Juan", 139);
  insert_via(g, "Guanare", "Barquisimeto", 170);
  insert_via(g, "Barquisimeto", "San Fernando", 526);
  insert_via(g, "Barinas", "San Fernando", 547);
  insert_via(g, "Caparo", "San Cristobal", 201);
  insert_via(g, "Coro", "Valencia", 252);
  insert_via(g, "Valencia", "Barquisimeto", 220);
  insert_via(g, "Valencia", "Maracay", 49);
  insert_via(g, "Valencia", "San Carlos", 100);
  insert_via(g, "Maracay", "Caracas", 109);
  insert_via(g, "San Felipe", "Maracay", 315);
  insert_via(g, "San Felipe", "San Carlos", 241);
  insert_via(g, "San Felipe", "Barquisimeto", 86);
  insert_via(g, "San Felipe", "San Juan", 222);
  insert_via(g, "Guanare", "San Carlos", 173);
  insert_via(g, "San Juan", "San Fernando", 261);
  insert_via(g, "Barcelona", "Pto La Cruz", 10);
  insert_via(g, "Pto La Cruz", "Cumana", 82);
  insert_via(g, "Cumana", "Maturin", 199);
  insert_via(g, "Pto Ordaz", "Maturin", 171);
  insert_via(g, "Pto Ordaz", "Ciudad Bolivar", 107);
  insert_via(g, "El Tigre", "Ciudad Bolivar", 130);
  insert_via(g, "El Tigre", "Barcelona", 166);
  insert_via(g, "El Tigre", "San Juan", 435);
}


int main()
{
  Mapa g;

  construir_mapa(g);

  imprimir_mapa(g);

  Mapa tree1;
  
  Mapa tree2;
  Prim_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree2);
  int sum2 = Total_Cost <Mapa, Distancia_Via> () (tree2);
  cout << endl
       << "Arbol abarcador segun Prim (" << sum2 << ")" << endl;
  imprimir_mapa(tree2);

  
  Mapa tree3;
  Kruskal_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree3);
  int sum3 = Total_Cost <Mapa, Distancia_Via> () (tree3);
  cout << endl
       << "Arbol abarcador segun Kruskal (" << sum3 << ")" << endl;
  imprimir_mapa(tree2);
}


