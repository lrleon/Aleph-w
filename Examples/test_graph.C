
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/
# include <iostream>

# include <ahSort.H>
# include <tpl_dynBinHeap.H>
# include <tpl_dynListQueue.H>
# include <tpl_graph.H>
# include <tpl_graph_utils.H>
# include <tpl_test_connectivity.H>
# include <tpl_test_cycle.H>
# include <tpl_test_acyclique.H>
# include <tpl_test_path.H>
# include <tpl_components.H>
# include <tpl_spanning_tree.H>
# include <Kruskal.H>
# include <Prim.H>
# include <Dijkstra.H>
# include <Bellman_Ford.H>

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

  Distancia_Via() { /* empty */ }

  Distancia_Via(const Distancia_Via&){ /* empty */ }
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
  bool operator () (const Via &, const Via &) const
  {
    return true;
  }
};


Mapa::Node * buscar_ciudad(Mapa & mapa, const string & nombre)
{
  return mapa.search_node([&nombre] (auto p) 
			  { return p->get_info().nombre == nombre; });
}

bool visitar(const Mapa & g, Mapa::Node * node, Mapa::Arc * arc)
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

  bool operator () (const Mapa & g, Mapa::Node * p, Mapa::Arc * a)
  {
    cout << "Estoy en  " << p->get_info().nombre << " viniendo desde " 
	 << (a != NULL ? g.get_connected_node(a, p)->get_info().nombre : 
	     "NULO") << endl ;

    if (dest != NULL)
      if (p == dest)
	{
	  cout << "Se alcanzo el nodo destino " << dest->get_info().nombre 
	       << endl;
	  return true;
	}

  return false;
  }

  bool operator () (Mapa &, Mapa::Node * p)
  {
    cout << "Estoy en  " << p->get_info().nombre << endl ;

    if (dest != NULL)
      if (p == dest)
	{
	  cout << "Se alcanzo el nodo destino " << dest->get_info().nombre 
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

  bool operator () (Mapa::Arc *)
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

  for (Camino::Iterator itor(path); itor.has_curr(); itor.next())
    cout << itor.get_current_node()->get_info().nombre << "-";

  cout << endl;
}


void imprimir_mapa(Mapa & g)
{
  cout << endl
       << "Listado de nodos (" << g.get_num_nodes() << ")" << endl;

  for (Mapa::Node_Iterator node_itor(g); node_itor.has_curr(); 
       node_itor.next())
    cout << INDENT << node_itor.get_current_node()->get_info().nombre << endl;

  cout << endl
       << endl
       << "Listado de arcos (" << g.get_num_arcs() << ")"
       << endl;

  for (Mapa::Arc_Iterator arc_itor(g); arc_itor.has_curr();
       arc_itor.next())
    {
      Mapa::Arc * arc = arc_itor.get_current_arc();
      cout << arc->get_info().nombre << " " << arc->get_info().distancia 
	   << " de " << g.get_src_node(arc)->get_info().nombre 
	   << " a " << g.get_tgt_node(arc)->get_info().nombre << endl;
    }

  for (auto arc : g.arcs())
    cout << arc->get_info().nombre << " " << arc->get_info().distancia 
	 << " de " << g.get_src_node(arc)->get_info().nombre 
	 << " a " << g.get_tgt_node(arc)->get_info().nombre << endl;

  cout << endl
       << endl
       << "Listado del grafo por nodos y en cada nodo por arcos" 
       << endl;
  for (Mapa::Node_Iterator node_itor(g); node_itor.has_curr(); 
       node_itor.next())
    {
      Mapa::Node * src_node = node_itor.get_current_node();
      cout << src_node->get_info().nombre << endl;
      for (Mapa::Node_Arc_Iterator itor(node_itor.get_current_node());
	   itor.has_curr(); itor.next())
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

  buscar_ciudad(g, "Valencia")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "Caracas")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "Coro")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "San Felipe")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "Guanare")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "Maracay")->get_info().tipo = Ciudad::CAPITAL;
  buscar_ciudad(g, "Maracaibo")->get_info().tipo = Ciudad::CAPITAL;
}

void test_functional(Mapa & mapa)
{
  mapa.for_each_node([] (Mapa::Node * p)
		     {
		       cout << p->get_info().nombre << endl;
		     });

  {
    cout << endl
	 << "First five nodes:" << endl;
    int k = 0;
    mapa.all_nodes([&k] (Mapa::Node * p)
		   {
		     cout << p->get_info().nombre << endl;
		     return ++k < 5;
		   });
  }

  {
    cout << endl
	 << "Mapping to string and sorting" << endl;
    sort(mapa.template nodes_map <std::string>([] (Mapa::Node * p)
    {
      return p->get_info().nombre;
    })).for_each([] (const std::string & str)
		 {
		   cout << str << endl;
		 });
  }

  {
    cout << endl
	 << "Folding string length" << endl;
    size_t len = 0;
    len = mapa.foldl_nodes<size_t>(len, [] (const size_t & acc, Mapa::Node * p)
			   {
			     return acc + p->get_info().nombre.size();
			   });
    cout << "Total length = " << len << endl;
  }

  {
    cout << endl
	 << "Testing for_each_arc" << endl;
    mapa.for_each_arc([] (Mapa::Arc * a)
		      {
			cout << a->get_info().nombre << " " 
			     << a->get_info().distancia << endl;
		      });
  }

  {
    int k = 0;
    cout << endl
	 << "Testing all_arc with 10 first arcs" << endl;
    mapa.all_arcs([&k] (Mapa::Arc * a)
		    {
		      cout << k << " " << a->get_info().nombre << endl;
		      return ++k < 10;
		    });
  }

  {
    cout << endl
	 << "testing arcs mapping to string" << endl;
    sort(mapa.template arcs_map<string>([] (Mapa::Arc * a)
				   {
				     return a->get_info().nombre;
				   })).for_each([] (const string & str)
      {	
	cout << str << endl; 
      });      
  }

  {
    cout << endl
	 << "Folding total distance of all arcs" << endl;
    int d = 0;
    d = mapa.template foldl_arcs<int>(d, [] (const int & dist, Mapa::Arc * a)
    {
      cout << "dist = " << dist << " + " 
      << a->get_info().distancia << endl;
      return dist + a->get_info().distancia;
    });
    cout << "Total distance folded = " << d << endl;
  }

  {
    cout << endl
	 << "Super test of for_each_node and inside for_each_arc" << endl;
    mapa.for_each_node([&mapa] (Mapa::Node * p)
    {
      cout << p->get_info().nombre << ":" << endl;
      mapa.for_each_arc(p, [] (Mapa::Arc * a)
			{
			  cout << "    " << a->get_info().nombre << endl;
			});
    });
  }

  {
    cout << endl
	 << "Super test forall_node and inside for_each_arc bounded to " << endl
	 << "10 nodes and 2 arcs by node" << endl;
    int v = 0;
    mapa.all_nodes([&v, &mapa] (Mapa::Node * p)
    {
      cout << v << " : " << p->get_info().nombre << ":" << endl;
      int e = 0;
      mapa.all_arcs(p, [&e] (Mapa::Arc * a)
		    {
		      cout << "    " << e << " : "
			   << a->get_info().nombre << endl;
		      return ++e < 2;
		    });
      return ++v < 10;
    });
  }

  {
    cout << "Mapping of arcs" << endl
	 << endl;
    mapa.for_each_node([&mapa](Mapa::Node * p)
    {
      cout << p->get_info().nombre << endl;
      sort(mapa.template arcs_map<string> (p, [] (Mapa::Arc * a)
				 {
				   return a->get_info().nombre;
				 })).for_each([] (const string & str)
	{
	  cout << "    " << str << endl;
	});
    });
  }

  {
    cout << endl 
	 << "Folding total distance through fold_arcs" << endl;
    int dist = 0;
    mapa.reset_arcs();
    mapa.for_each_node([&mapa, &dist] (Mapa::Node * p)
    {
      dist += mapa.foldl_arcs<int>(p, 0, [] (const int & d, Mapa::Arc * a)
			     {
			       if (IS_ARC_VISITED(a, Breadth_First))
				 return d;
			       cout << "dist = " << d << " + "  
				    << a->get_info().distancia << endl;
			       ARC_BITS(a).set_bit(Breadth_First, true);
			       return d + a->get_info().distancia;
			     });
    });
    mapa.reset_arcs();
    cout << endl
	 << "Distancia = " << dist << endl;
  }

  {
    cout << endl
	 << "Testing for_each_node wrapper for CAPITAL cities" << endl;
    for_each_node<Mapa>(mapa,
		  [] (Mapa::Node * p) 
		  {
		    cout << p->get_info().nombre << endl;
		  }, 
		  [] (Mapa::Node * p) -> bool
		  {
		    return p->get_info().tipo == Ciudad::CAPITAL;
		  });
  }

  {
    cout << endl
	 << "Testing for_each_arc for arcs having more of 200 Km" << endl;
    for_each_arc<Mapa>(mapa,
		 [] (Mapa::Arc * a)
		 {
		   cout << a->get_info().nombre << " " 
			<< a->get_info().distancia << endl;
		 },
		 [] (Mapa::Arc * a)
		 {
		   return a->get_info().distancia >= 200;
		 });
  }

  {
    mapa.reset_arcs();
    cout << endl
	 << "Testing super for_each_arc for roads longer mode of 200" << endl;
    for_each_node<Mapa>(mapa, [&mapa] (Mapa::Node * p)
	     {
	       for_each_arc<Mapa>(mapa, p, 
			    [] (Mapa::Arc * a)
			    {
			      if (IS_ARC_VISITED(a, Depth_First))
				return;
			      ARC_BITS(a).set_bit(Depth_First, true);
			      const Mapa::Arc_Type & info = a->get_info();
			      cout << info.nombre 
				   << " " << info.distancia << endl;
			    },
			    [] (Mapa::Arc * a)
			    {
			      return a->get_info().distancia >= 200;
			    });
	     });
  }

  {
    cout << endl
	 << "Testing forall_node for the first five Capitals" << endl;
    int k = 0;
    forall_node<Mapa>(mapa, [&k] (Mapa::Node * p)
		{
		  cout << p->get_info().nombre << endl;
		  return ++k < 5;
		},
		[] (Mapa::Node * p)
		{
		  return p->get_info().tipo == Ciudad::CAPITAL;
		});
  }

  {
    cout << endl
	 << "Testing filtered map_nodes for capitals" << endl;
    sort(nodes_map<Mapa, string>(mapa, [] (Mapa::Node * p)
	      {
		return p->get_info().nombre;
	      },
	      [] (Mapa::Node * p)
	      {
		return p->get_info().tipo == Ciudad::CAPITAL;
	      })).for_each([] (const string & str)
			   {
			     cout << str << endl;
			   });
  }

  {
    cout << endl
	 << "Testing filtered map_arcs for distances longer that 200" << endl;
    sort(arcs_map<Mapa, string>(mapa, [] 
				(Mapa::Arc * a)
			   {
			     const Mapa::Arc_Type & info = a->get_info();
			     return info.nombre + " : " + 
			       to_str(info.distancia);
			   },
				[] (Mapa::Arc * a)
			   {
			     return a->get_info().distancia >= 200;
			   })).for_each([] (const string & str)
				       {
					 cout << str << endl;
				       });
  }

  {
    cout << endl
	 << "Testing super filtered map_arcs for diatances longer than 250"
	 << endl 
	 << "and involving capitals" << endl;
    for_each_node<Mapa>(mapa, [&mapa] (Mapa::Node * p)
    {
      cout << p->get_info().nombre << endl;
      sort(arcs_map<Mapa, string>(mapa, p, [] (Mapa::Arc * a)
      {
	const Mapa::Arc_Type & info = a->get_info();
	return info.nombre + " : " + to_str(info.distancia);
      }, [] (Mapa::Arc * a)
         {
	   return a->get_info().distancia >= 200;
	 })).for_each([] (const string & str)
		     {
		       cout << "    " << str << endl;
		     });
    }, [] (Mapa::Node * p)
		  {
		    return p->get_info().tipo == Ciudad::CAPITAL;
		  });
  }

  {
    cout << endl
	 << "Testing filtered foldl_nodes on name length of capitals" << endl;
    int l = foldl_nodes<Mapa, int>(mapa, 0, [] (int sz, Mapa::Node * p)
			{
			  return sz + p->get_info().nombre.size();
			}, [] (Mapa::Node * p)
			{
			  return p->get_info().tipo == Ciudad::CAPITAL;
			});
    cout << "Length of capital names is " << l << endl;			
  }

  {
    cout << endl
	 << "Folding total distance for arcs longer than 200" << endl;
    int l = foldl_arcs<Mapa, int>(mapa, 0, [] (int sz, Mapa::Arc * a)
				  {
				    return sz + a->get_info().distancia;
				  },
				  [] (Mapa::Arc * a)
				  {
				    return a->get_info().distancia >= 200;
				  });
    cout << "Total distance is " << l << endl;
  }

  {
    cout << endl
	 << "Super folding total distance for arcs longer than 200" << endl;
    int l = 0;
    mapa.reset_arcs();
    for_each_node<Mapa>(mapa, [&l, &mapa] (Mapa::Node * p)
		  {
		    l += foldl_arcs<Mapa,int>(mapa, p, 0, 
					      [] (int sz, Mapa::Arc * a)
                    {
		      if (IS_ARC_VISITED(a, Depth_First))
			return sz;
		      ARC_BITS(a).set_bit(Depth_First, true);
		      return sz + a->get_info().distancia;
		    }, [] (Mapa::Arc * a)
                    {
		      return a->get_info().distancia >= 200;
		    });
		  });
    cout <<  "Total distance is " << l << endl;
  }

  {
    cout << endl
	 << "Testing partition of nodes between Capital and not" << endl;
    typedef std::pair<DynList<Mapa::Node*>, DynList<Mapa::Node*>> Pair;
    Pair part = 
      partition<DynList<Mapa::Node*>>
      (mapa.nodes(), [] (Mapa::Node * p)
			  {
			    return p->get_info().tipo == Ciudad::CAPITAL;
			  });
    cout << "Capitals : " << endl;
    for_each(part.first, /* Lambda */ [] (Mapa::Node * p)
	     {
	       cout << "    " << p->get_info().nombre << endl;
	     });
    cout << "Non capitals :" << endl;
    for_each(part.second, /* Lambda */ [] (Mapa::Node * p)
	     {
	       cout << "    " << p->get_info().nombre << endl;
	     });
   }

   {
     cout << endl
	  << "Testing arcs()" << endl;
     sort(mapa.arcs()).for_each(/* Lambda */ [] (Mapa::Arc * a)
				{
				  cout << a->get_info().nombre << endl;
				});
   }

   {
     cout << endl
	  << "Testing Super arcs(p) " << endl;
     sort(mapa.nodes(), [] (Mapa::Node * p1, Mapa::Node * p2) 
	  {
	    return p1->get_info().nombre < p2->get_info().nombre;
	  }).for_each(/* Lambda */ [&mapa] (Mapa::Node * p)
       {
	 cout << p->get_info().nombre << endl;
	 sort(mapa.arcs(p), [] (Mapa::Arc * a1, Mapa::Arc * a2)
	      {
		return a1->get_info().nombre < a2->get_info().nombre;
	      }).for_each(/* Lambda */ [] (Mapa::Arc * a)
	   {
	     cout << "    " << a->get_info().nombre << endl;
	   });
       });
   }
}


int main()
{
  Mapa g;

  construir_mapa(g);

  test_functional(g);

  {
    cout << "Recorrido en profundidad " << endl;
    size_t n = Depth_First_Traversal <Mapa, Visitar> () (g, Visitar());
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en profundidad " << endl;
    n = Depth_First_Traversal <Mapa, Visitar> () 
      (g, Visitar(buscar_ciudad(g, "Paraguaipos")));
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en profundidad " << endl;
    n = Depth_First_Traversal <Mapa> () (g);
    cout << n << " nodos visitados" << endl;
  }

  {
    cout << "Recorrido en amplitud " << endl;
    size_t n = breadth_first_traversal (g, &visitar);
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en amplitud " << endl;
    n = Breadth_First_Traversal<Mapa, Visitar> () (g, Visitar());
    cout << n << " nodos visitados" << endl;

    cout << "Recorrido en amplitud " << endl;
    n = Breadth_First_Traversal<Mapa, Visitar> () 
      (g, Visitar(buscar_ciudad(g, "Carora")));
    cout << n << " nodos visitados" << endl;
  }

  {
    if (Test_Connectivity <Mapa> () (g))
      cout << "El grafo es conectado";
    else
      cout << "El grafo no es conectado";
    cout << endl;
  }

  {
    Contar_Arcos contar;
    if (Test_Connectivity <Mapa, Contar_Arcos> () (g, contar))
      cout << "El grafo es conectado";
    else
      cout << "El grafo no es conectado";
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Test_For_Cycle <Mapa> () (g, buscar_ciudad(g, "San Cristobal")))
      cout << "Hay un ciclo desde San Cristobal" << endl;
    else
      cout << "No hay un ciclo desde San Cristobal" << endl;
  }

  {
    Contar_Arcos contar;
    if (Test_For_Cycle <Mapa, Contar_Arcos> (contar) 
	(g, buscar_ciudad(g, "San Cristobal")))
      cout << "Hay un ciclo desde San Cristobal" << endl;
    else
      cout << "No hay un ciclo desde San Cristobal" << endl;
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Is_Graph_Acyclique <Mapa> () (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
  }

  {
    if (Is_Graph_Acyclique <Mapa> () (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
  }

  {
    Contar_Arcos contar;
    if (Is_Graph_Acyclique <Mapa, Contar_Arcos> (contar) (g))
      cout << "El grafo es aciclico (es un arbol)" << endl;
    else
      cout << "El grafo tiene ciclos" << endl;
    cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    if (Test_For_Path <Mapa> () (g, buscar_ciudad(g, "El Vigia"), 
				 buscar_ciudad(g, "Carora")))
      cout << "Existe un camino desde El Vigia hacia Carora" << endl;
  }

  {
     Contar_Arcos contar;
     if (Test_For_Path <Mapa, Contar_Arcos> (contar) 
	 (g, buscar_ciudad(g, "El Vigia"), buscar_ciudad(g, "Carora")))
       cout << "Existe un camino desde El Vigia hacia Carora" << endl;
     cout << "Se vieron " << contar.count << " arcos" << endl;
  }

  {
    auto cmp = [] (const Mapa::Node * p1, const Mapa::Node * p2)
      {
	return p1->get_info().nombre < p2->get_info().nombre; 
      };	
    DynList<Mapa> subgrafos;

    Inconnected_Components <Mapa> () (g, subgrafos);
    subgrafos.mutable_for_each([cmp] (Mapa & m) { m.sort_nodes(cmp); });

    subgrafos.for_each([] (const Mapa & m)
    {
      m.nodes().for_each([] (auto p) { cout << p->get_info().nombre << " "; });
      cout << endl;
    });
    cout << endl;

    DynList<Mapa> subs = inconnected_components(g);
    subs.mutable_for_each([cmp] (Mapa & m) { m.sort_nodes(cmp); });
    
    subs.for_each([] (const Mapa & m)
    {
      m.nodes().for_each([] (auto p) { cout << p->get_info().nombre << " "; });
      cout << endl;
    });
    cout << endl;

    assert(eq(subgrafos, subs, [] (const Mapa & m1, const Mapa & m2)
    	      {
    		return eq(m1.nodes(), m2.nodes(), [] (auto p1, auto p2)
                  {
		    return p1->get_info().nombre == p2->get_info().nombre; 
		  });
    	      }));

    zipEq(subgrafos, subs).for_each([] (auto & p)
    {
      zipEq(p.first.nodes(), p.second.nodes()).for_each([] (auto p) 
      {
	assert(p.first->get_info().nombre == p.second->get_info().nombre);
      });
    });
  }

  {
    Mapa tree;
    Find_Depth_First_Spanning_Tree <Mapa> () (g, tree);
  }

  {
    Mapa tree;
    Total_Dist total;
    (Find_Depth_First_Spanning_Tree <Mapa, Total_Dist> (total)) (g, tree);
    cout << "La distancia total del arbol es : " << total.dist << endl;
  }


  Path<Mapa> path_b = find_path_breadth_first(g, buscar_ciudad(g, "El Vigia"), 
					      buscar_ciudad(g, "Carora")); 
  cout << "Camino por amplitud : ";
  imprimir_camino(path_b);

  Find_Path_Breadth_First<Mapa> ()  (g, buscar_ciudad(g, "El Vigia"), 
				     buscar_ciudad(g, "Carora"), path_b); 
  cout << "Camino por amplitud : ";
  imprimir_camino(path_b);

  //  return 0;

  Path<Mapa> path =
    Find_Path_Depth_First <Mapa> () (g, buscar_ciudad(g, "El Vigia"), 
				     buscar_ciudad(g, "Carora")); 
  cout << "Camino por profundidad : ";
  imprimir_camino(path);

  //  Find_Path_Depth_First <Mapa>()(g, g.search_node("El Vigia"), 
  //			  g.search_node("Carora"), path); 

  //  return 0;

  imprimir_mapa(g);

  Mapa tree1;
  Kruskal_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree1);
  int sum1 = Total_Cost<Mapa, Distancia_Via> () (tree1);
  cout << endl
       << "Arbol abarcador segun Kruskal (" << sum1 << ")" << endl;
  imprimir_mapa(tree1);

  Mapa tree2;
  Prim_Min_Spanning_Tree <Mapa, Distancia_Via> () (g, tree2);
  int sum2 = Total_Cost<Mapa, Distancia_Via>() (tree2);
  cout << endl
       << "Arbol abarcador segun Prim (" << sum2 << ")" << endl;
  imprimir_mapa(tree2);

  if (sum1 != sum2)
    EXIT("Costes totales de arboles difieren %d %d", sum1, sum2);

  Mapa tree3;

  Dijkstra_Min_Paths<Mapa, Distancia_Via> () (g, g.get_first_node(), tree3);
  cout << endl
       << "Arbol abarcador segun Dijkstra" << endl;
  imprimir_mapa(tree3);

  Path<Mapa> min_path1 = find_path_depth_first(tree3, tree3.get_first_node(), 
					       buscar_ciudad(tree3, "Carora"));
  if (not min_path1.is_empty())
    {
      cout << "Camino encontrado en Dijkstra: " 
	   << tree3.get_first_node()->get_info().nombre << " - Carora" << endl;
      imprimir_camino(min_path1);
      cout << endl << endl;
    }
  else
    EXIT("Error no se encontro camino!");

   
  // TODO: hacer una copia hacia un digrafo yt verificar resultados con
  // los dos algoritmos dijkstra y bellman

# ifdef nada

  Mapa tree4;

  Bellman_Ford_Min_Spanning_Tree<Mapa, Distancia_Via> ()
    (g, g.get_first_node(), tree4);
  cout << endl
       << "Arbol abarcador segun Bellman-Ford" << endl;
  imprimir_mapa(tree4);

  Path<Mapa> min_path2 = 
    find_path_depth_first(tree4, tree4.get_first_node(), 
			  buscar_ciudad(tree4, "Carora"));
  if (not min_path2.is_empty())
    {
      cout << "Camino encontrado en Dijkstra: " 
	   << tree4.get_first_node()->get_info().nombre << " - Carora" << endl;
      imprimir_camino(min_path2);
      cout << endl << endl;
    }
  else
    EXIT("Error no se encontró camino!");

# endif 

  Path<Mapa> aux_path = min_path1;

  min_path1 = aux_path;

  DynDlist<Path<Mapa> > test;

  Path<Mapa> path2(g);
  Dijkstra_Min_Paths<Mapa, Distancia_Via> ().find_min_path
    (g, buscar_ciudad(g, "Guanare"), buscar_ciudad(g, "San Fernando"), path2);


  cout << "Camino encontrado:" << endl;
  imprimir_camino(path2);
  cout << endl << endl;

  (Dijkstra_Min_Paths <Mapa, Distancia_Via> ()).find_min_path 
    (g, buscar_ciudad(g, "Guanare"), buscar_ciudad(g, "San Fernando"), path2);

  cout << "Camino encontrado:" << endl;
  imprimir_camino(path2);
  cout << endl << endl;

# ifdef nada

  Mat_Mapa matriz(g);

  Ady_Mat<Mat_Mapa, int> distancias1(matriz, &via_a_distancia);

  Ady_Mat<Mat_Mapa, int> distancias2(distancias1);

  Mapa gc1 = g;

  Mapa gc2;

  gc2 = g;

  g = gc1;

  imprimir_mapa(g);

  Dimapa dimapa;

  gc2 = dimapa;

  EXIT("");

# endif

}


