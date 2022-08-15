
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

# include <limits>
# include <iostream>
# include <tpl_matgraph.H>
# include <mat_latex.H>
# include <tpl_graph_utils.H>
# include <latex_floyd.H>

using namespace std;

using namespace Aleph;

# define INDENT "    "

    template <typename M>
struct C_i
{
  string operator () (M & m, const long & i) const
  {
    return m(i)->get_info().nombre;
  }
};


    template <typename M>
struct C_ij
{
  string operator () (M & m, const long & i, const long & j)
  {
    return m(m(i, j))->get_info().nombre;
  }
};


    template <typename M>
struct Di_ij
{
  string operator () (M & mat, const long & i, const long & j)
  {
    typename M::Arc_Type::Distance_Type value = mat(i, j);

    if (value == numeric_limits<double>::infinity())
      return string("{$\\infty$}");

    if (value == 0)
      return string("{$0$}");

    char buf[256]; 

    snprintf(buf, 256, "{$%d$}", (int) value);

    return string(buf);
  }
};

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
};


struct Arco
{
  typedef double Distance_Type;

  static const double Max_Distance;

  static const double Zero_Distance;

  double distancia;

  Arco() : distancia(Max_Distance) { /* empty */ }

  Arco(const double & dist) : distancia(dist) 
  {
    /* empty */ 
  }

  const double & get_distance() const { return distancia; }

  operator double & () { return distancia; }
}; 

const Arco Arco_Zero(0);

const double Arco::Max_Distance = numeric_limits<double>::infinity();

const double Arco::Zero_Distance = 0;


typedef Graph_Node<Nodo> Node_Nodo;

typedef Graph_Arc<Arco> Arco_Arco;

typedef List_Digraph<Node_Nodo, Arco_Arco> Grafo;


void insertar_arco(Grafo &       grafo, 
		  const string & src_name, 
		  const string & tgt_name,
		  const double & distancia)
{
  Grafo::Node * n1 = grafo.find_node(Nodo(src_name));

  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  Grafo::Node * n2 = grafo.find_node(Nodo(tgt_name));

  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2, Arco(distancia));
}



void build_test_graph(Grafo & g)
{
  g.insert_node("A");
  g.insert_node("B");
  g.insert_node("C");
  g.insert_node("D");
  g.insert_node("E");
  g.insert_node("F");
  g.insert_node("G");
  g.insert_node("H");
  g.insert_node("I");

  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "C", "A", 1);  
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "F", "D", -2);
  insertar_arco(g, "F", "C", -1);
  insertar_arco(g, "F", "E", 2);
  insertar_arco(g, "D", "F", 2);
  insertar_arco(g, "D", "H", 4);
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", -1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 4);
  insertar_arco(g, "I", "H", 3);

}

void copia_Arco_Arco(Arco_Arco * arc, const long&, const long&, double& value)
{
  value = arc->get_info().get_distance();
}


int main() 
{
  Grafo list_graph;

  build_test_graph(list_graph);

  Ady_Mat<Grafo, double> cost_mat(list_graph);

  Ady_Mat<Grafo, long> path_mat(list_graph);

  ofstream out("mat-floyd.tex", ios::trunc); 

  floyd_all_shortest_paths_latex <Grafo, C_i, C_ij, Di_ij> 
    (list_graph, cost_mat, path_mat, out);
}
