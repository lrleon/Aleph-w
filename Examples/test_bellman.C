
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
# include <limits>
# include <Bellman_Ford.H>


using namespace std;

using namespace Aleph;

    template <class GT>
void print_pred(GT & g, DynArray<typename GT::Node*> & pred)
{
  for (Node_Iterator<GT> it(g); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << " ";
  cout << endl;

  const unsigned int & n = pred.size();

  for (int i = 0; i < n; ++i)
    {
      typename GT::Node * p = pred[i];
      cout << (p != NULL ? p->get_info() : "-");
      //      if (arcs[i] == NULL)
      //	cout << " (" << i << ") ";
      cout << " ";
    }
  cout << endl;
} 

# define INDENT "    "

typedef List_Digraph<Graph_Node<string>, Graph_Arc<double>> Grafo;

struct Distancia
{
  typedef double Distance_Type;

  Distance_Type & operator () (Grafo::Arc * a) const
  {
    return a->get_info();
  }

  static void set_zero(Grafo::Arc * a) { a->get_info() = 0; }
};

void insertar_arco(Grafo &       grafo, 
		   const string & src_name, 
		   const string & tgt_name,
		   const double & distancia)
{
  auto n1 = grafo.search_node([&src_name] (auto p) 
			      { return p->get_info() == src_name; });
  if (n1 == NULL)
    n1 = grafo.insert_node(src_name);

  auto n2 = grafo.search_node([&tgt_name] (auto p)
			      { return p->get_info() == tgt_name; });
  if (n2 == NULL)
    n2 = grafo.insert_node(tgt_name);

  grafo.insert_arc(n1, n2, distancia);
}

Grafo build_test_graph_1()
{
  Grafo g;

  g.insert_node(string("A"));
  g.insert_node(string("B"));
  g.insert_node(string("C"));
  g.insert_node(string("D"));
  g.insert_node(string("E"));
  g.insert_node(string("F"));
  g.insert_node(string("G"));
  g.insert_node(string("H"));
  g.insert_node(string("I"));

  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "C", "A", 1);  // -11 para hacer ciclo negativo
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "F", "D", -1);
  insertar_arco(g, "F", "C", -1);
  insertar_arco(g, "F", "E", 1);
  insertar_arco(g, "D", "F", 3);
  insertar_arco(g, "D", "H", 4); 
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", -1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 2);
  insertar_arco(g, "G", "I", 3);

  return g;
}

Grafo build_test_graph_2()
{
  Grafo g;
  g.insert_node(string("A"));
  g.insert_node(string("B"));
  g.insert_node(string("C"));
  g.insert_node(string("D"));
  g.insert_node(string("E"));
  g.insert_node(string("F"));
  g.insert_node(string("G"));
  g.insert_node(string("H"));
  g.insert_node(string("I"));

  insertar_arco(g, "A", "B", 2);
  insertar_arco(g, "A", "F", 5);
  insertar_arco(g, "B", "F", 1);
  insertar_arco(g, "B", "D", 3);
  insertar_arco(g, "C", "A", 1);  
  insertar_arco(g, "C", "E", 4);  
  insertar_arco(g, "F", "D", -2);
  insertar_arco(g, "F", "C", -18);
  insertar_arco(g, "F", "E", 5);
  insertar_arco(g, "D", "F", 3);
  insertar_arco(g, "D", "H", 4); 
  insertar_arco(g, "E", "G", 2);
  insertar_arco(g, "E", "I", -2);
  insertar_arco(g, "G", "D", 3);
  insertar_arco(g, "G", "F", -1);
  insertar_arco(g, "G", "H", 2);
  insertar_arco(g, "H", "D", -2);
  insertar_arco(g, "H", "G", -1);
  insertar_arco(g, "I", "G", 2);
  insertar_arco(g, "I", "H", 3);

  return g;
}

typedef Path<Grafo> Camino;

void imprimir_camino(Camino & path)
{
  cout << endl
       << "Camino: ";

  if (path.is_empty())
    {
      cout << "vacio" << endl;
      return;
    }

  cout << path.get_first_node()->get_info();
  for (Camino::Iterator itor(path); itor.has_current_arc(); itor.next())
    {
      Grafo::Arc * a = itor.get_current_arc();
      Grafo::Node * tgt = (Grafo::Node*) a->tgt_node;
      cout << " " << a->get_info() << " --> " 
	   << tgt->get_info();
    }
  cout << endl;
}

void imprimir_arbol(Grafo & g)
{
  for (Grafo::Node_Iterator i(g); i.has_curr(); i.next())
    for (Grafo::Node_Arc_Iterator j(i.get_curr()); j.has_curr(); j.next())
      {
	Grafo::Arc * a = j.get_curr();
	cout << g.get_src_node(a)->get_info() << " " 
	     << a->get_info() << " -->" 
	     << g.get_tgt_node(a)->get_info() << endl;
      }
  cout << endl;
}

void test(const Grafo & g)
{
  Bellman_Ford<Grafo, Distancia> bf(g);

  cout << "Normal painting " << endl;
  bool has_neg_cycle = bf.paint_spanning_tree(g.get_first_node());
  if (has_neg_cycle)
    {
      cout << "El grafo tiene un ciclo negativo " << endl;
      Path<Grafo> cycle =  bf.test_negative_cycle(g.get_first_node());
      imprimir_camino(cycle);


      cout << "Verificando ciclo por algoritmo general" << endl;
      cycle = bf.test_negative_cycle();
      if (cycle.is_empty())
	AH_ERROR("No se encontro el ciclo");
      else
	imprimir_camino(cycle);

      cout << "Verificando ciclo negativo por algoritmo basado en cola" << endl;
      tuple<Path<Grafo>, size_t> c =
	bf.search_negative_cycle(g.get_first_node(), 0.7, 100);
      if (get<0>(c).is_empty())
	AH_ERROR("No se encontro el ciclo");
      else
	imprimir_camino(get<0>(c));

      cout << "Todo OK" << endl;
    }
  else
    {
      Grafo tree;
      bf.build_tree(tree);
      cout << endl
	   << "Arbol abarcador segun Bellman-Ford" << endl;
      imprimir_arbol(tree);
    }

  cout << endl
       << "Fast painting " << endl;
  has_neg_cycle = bf.faster_paint_spanning_tree(g.get_first_node());
  if (has_neg_cycle)
    {
      cout << "El grafo tiene un ciclo negativo " << endl
	   << "Buscando ciclo con test_negative_cycle()" << endl;
      Path<Grafo> cycle =  bf.test_negative_cycle(g.get_first_node());
      imprimir_camino(cycle);

      cout << "Buscando ciclo con search_negative_cycle()" << endl;
      cycle = bf.search_negative_cycle(g.get_first_node());
      imprimir_camino(cycle);
    }
  else
    {
      Grafo tree;
      bf.build_tree(tree);
      cout << endl
	   << "Arbol abarcador segun Bellman-Ford" << endl;
      imprimir_arbol(tree);
    }
}

int main()
{
  {
    Grafo g = build_test_graph_2();
    test(g);
  }

  cout << "========================================================" << endl;

  {
    Grafo g = build_test_graph_1();
    test(g);
  }
}
