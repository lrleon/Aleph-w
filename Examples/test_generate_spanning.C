
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
    return to_string(a->get_info().w);
  }
};


Grafo::Node * bn(Grafo * g, const string & str)
{
  return g->search_node([&str] (Grafo::Node * p)
		      {
			return p->get_info().str == str;
		      });
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


int main()
{
  Grafo * g = build_graph();

  Grafo tree1;

  Kruskal_Min_Spanning_Tree <Grafo, Distancia> () (*g, tree1);

  ofstream out("test.gra", ios::out);

  generate_cross_spanning_tree
    <Grafo, Nodo_String, Arco_String>
    (*g, 6, 20, 20, out);
}

