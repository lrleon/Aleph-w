
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
# include <fstream>
# include <iostream>
# include <memory>
# include <string>
# include <ah-errors.H>
# include <tpl_graph.H>
# include <graph_to_tree.H>
# include <generate_tree.H>

using namespace std;

using namespace Aleph;

# define INDENT "    "

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

  Via() : nombre("Desconocido"), distancia(0), tipo(DESCONOCIDO) {}

  Via(int d) 
    : nombre("Desconocido"), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  Via(char * nom, int d) 
    : nombre(nom), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  Via(const string& nom, int d) 
    : nombre(nom), distancia(d), tipo(DESCONOCIDO) { /* empty */ }

  typedef int Distance_Type;

  Distance_Type& get_distance() { return distancia; }

  static constexpr Distance_Type Zero_Distance = 0;
};

typedef Graph_Node<Ciudad> Nodo_Ciudad;

typedef Graph_Arc<Via> Arco_Via;


typedef List_Graph<Nodo_Ciudad, Arco_Via> Mapa;

typedef List_Digraph<Nodo_Ciudad, Arco_Via> Dimapa;



struct Ciudad_Igual
{
  bool operator () (const Ciudad & c1, const Ciudad & c2) const
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

void insert_via(Mapa & mapa, 
		const string & c1, const string & c2,
		int distancia)
{
  Mapa::Node * n1 = buscar_ciudad(mapa, c1);

  if (n1 == nullptr)
    n1 = mapa.insert_node(c1);

  Mapa::Node * n2 = buscar_ciudad(mapa, c2);

  if (n2 == nullptr)
    n2 = mapa.insert_node(c2);

  string nombre_arco = n1->get_info().nombre + "--" + n2->get_info().nombre;

  mapa.insert_arc(n1, n2, Via(nombre_arco, distancia));
}


void imprimir_camino(Path<Mapa> & path)
{
  cout << endl
       << "Path: ";
  for (Path<Mapa>::Iterator itor(path); itor.has_curr(); itor.next())
    cout << itor.get_current_node()->get_info().nombre << "-";

  cout << endl;
}


void imprimir_mapa(Mapa & g)
{
  cout << endl
       << "Node list (" << g.get_num_nodes() << ")" << endl;

  for (Mapa::Node_Iterator node_itor(g); node_itor.has_curr(); 
       node_itor.next())
    cout << INDENT << node_itor.get_current_node()->get_info().nombre << endl;

  cout << endl
       << endl
       << "Arc list (" << g.get_num_arcs() << ")"
       << endl;

  for (Mapa::Arc_Iterator arc_itor(g); arc_itor.has_curr();
       arc_itor.next())
    {
      Mapa::Arc * arc = arc_itor.get_current_arc();
      cout << arc->get_info().nombre << " " << arc->get_info().distancia 
	   << " from " << g.get_src_node(arc)->get_info().nombre 
	   << " to " << g.get_tgt_node(arc)->get_info().nombre << endl;
    }

  cout << endl
       << endl
       << "Graph listing by nodes and for each node by arcs" 
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

void escribir_deway(int deway[], const size_t & n)
{
  for (size_t i = 0; i < n; ++i)
    {
      cout << deway[i];

      if (i < n - 1)
	cout << ".";
    }
}

int main()
{
  Mapa tree;

  construir_mapa(tree);

  imprimir_mapa(tree);

  Mapa::Node * c = buscar_ciudad(tree, "Merida");
  ah_runtime_error_if(c == nullptr)
    << "buscar_ciudad(tree, \"Merida\") returned nullptr";

  std::unique_ptr<Tree_Node<string>> t(
    Aleph::Graph_To_Tree_Node <Mapa, string, GT_Tree<Mapa> > () (tree, c));

  while (true)
    {
      cout << "Enter key to search (type \"quit\"): ";
      string clave;
      if (not std::getline(std::cin, clave))
        break;

      // Remove trailing CR if present (e.g. on Windows)
      if (not clave.empty() and clave.back() == '\r')
        clave.pop_back();

      if (clave.empty() or clave == "quit")
        break;

      const size_t Buf_Size = 512;
      int deway[Buf_Size];
      size_t dw_size = 0;

      Tree_Node<string> * p = 
		search_deway <Tree_Node<string> >(t.get(), clave, deway, Buf_Size,
		                                 dw_size);

	      if (p == nullptr)
		cout << clave << " was not found in the tree" << endl;
      else
	{
	  cout << clave << " has Deway number: ";
	  escribir_deway(deway, dw_size);
	  cout << endl;
	}
    } 

	  cout << "Exiting ... " << endl;

	  ofstream test("prueba.Tree", ios::trunc);
	  ah_runtime_error_if(not test)
	    << "could not open prueba.Tree for writing";

	  Aleph::generate_tree<Tree_Node<string>, Write_Ciudad> (t.get(), test);
}
