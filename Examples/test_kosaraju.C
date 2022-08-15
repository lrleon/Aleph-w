
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
# include <kosaraju.H>

struct Nodo
{
  string nombre;

  Nodo() {}

  Nodo(const string & str) : nombre(str) { /* empty */ }

  Nodo(const char * str) : nombre(str) { /* empty */ }

  bool operator == (const Nodo & der) const 
  {
    return nombre == der.nombre; 
  }
};


typedef Graph_Node<Nodo> Node_Nodo;

typedef Graph_Arc<Empty_Class> Arco_Arco;

typedef List_Digraph<Node_Nodo, Arco_Arco> Grafo;


void insertar_arco(Grafo &        grafo, 
		   const string & src_name, 
		   const string & tgt_name)
{
  Grafo::Node * n1 = grafo.find_node(Nodo(src_name));

  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  Grafo::Node * n2 = grafo.find_node(Nodo(tgt_name));

  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2);
}



void build_test_graph_1(Grafo & g)
{
  g.insert_node("E");

  insertar_arco(g, "A", "B");
  insertar_arco(g, "A", "D");
  insertar_arco(g, "B", "C");
  insertar_arco(g, "C", "A");
  insertar_arco(g, "D", "E");
  insertar_arco(g, "E", "B");
  insertar_arco(g, "E", "D");

  insertar_arco(g, "E", "G");

  insertar_arco(g, "G", "F");
  insertar_arco(g, "F", "G");

  insertar_arco(g, "E", "H");

  insertar_arco(g, "H", "I");

  insertar_arco(g, "I", "J");
  insertar_arco(g, "J", "K");
  insertar_arco(g, "K", "I");
}

# define INDENT "    "

void print_graph(Grafo & g)
{
  for (Grafo::Node_Iterator node_it(g); node_it.has_curr(); node_it.next())
    {
      Grafo::Node * src = node_it.get_current_node();
      cout << src->get_info().nombre << endl;
      for (Grafo::Node_Arc_Iterator it(src); it.has_curr(); it.next())
	{
	  Grafo::Node * tgt = it.get_tgt_node();
	  cout << INDENT << " --> " << tgt->get_info().nombre << endl;
	}
    }
  cout << endl;
}


int main()
{
  Grafo g;

  build_test_graph_1(g);

  print_graph(g);

  {
    DynList<Grafo> list;
    DynList<Grafo::Arc*> arc_list;

    kosaraju_connected_components (g, list, arc_list);

    for (DynList<Grafo>::Iterator it(list); it.has_curr(); it.next())
      {
	Grafo & sg = it.get_curr();
	cout << "Bloque: ";
	print_graph(sg);
	cout << endl;
      }
  }
  
  {
    DynList<DynList<Grafo::Node*>> nodes = kosaraju_connected_components(g);

      for (auto it = nodes.get_it(); it.has_curr(); it.next())
      {
	DynList<Grafo::Node*> & l = it.get_curr();
	cout << "Bloque: ";
	for (auto t = l.get_it(); t.has_curr(); t.next())
	  cout << t.get_curr()->get_info().nombre << " ";
	cout << endl;
      }
  }  
}

