
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|         

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon & Alejandro Mujica

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
# include <topological_sort.H>


typedef List_Digraph<Graph_Node<string>, Graph_Arc<Empty_Class> > Digrafo;

Digrafo::Node * nodo(Digrafo & g, const string & str)
{
  Digrafo::Node * p = g.find_node(str);

  if (p == NULL)
    return g.insert_node(str);

  return p;
}

void build_digraph(Digrafo & g)
{
  g.insert_arc(nodo(g, "A"), nodo(g, "B"));
  g.insert_arc(nodo(g, "A"), nodo(g, "I"));
  g.insert_arc(nodo(g, "A"), nodo(g, "F"));
  g.insert_arc(nodo(g, "B"), nodo(g, "F"));
  g.insert_arc(nodo(g, "C"), nodo(g, "F"));
  g.insert_arc(nodo(g, "C"), nodo(g, "G"));
  g.insert_arc(nodo(g, "D"), nodo(g, "H"));
  g.insert_arc(nodo(g, "F"), nodo(g, "I"));
  g.insert_arc(nodo(g, "F"), nodo(g, "J"));
  g.insert_arc(nodo(g, "G"), nodo(g, "J"));
  g.insert_arc(nodo(g, "H"), nodo(g, "J"));
  g.insert_arc(nodo(g, "H"), nodo(g, "K"));
  g.insert_arc(nodo(g, "H"), nodo(g, "L"));
  g.insert_arc(nodo(g, "I"), nodo(g, "P"));
  g.insert_arc(nodo(g, "J"), nodo(g, "M"));
  g.insert_arc(nodo(g, "J"), nodo(g, "N"));
  g.insert_arc(nodo(g, "K"), nodo(g, "N"));
  g.insert_arc(nodo(g, "K"), nodo(g, "O"));
  g.insert_arc(nodo(g, "L"), nodo(g, "O"));
  g.insert_arc(nodo(g, "L"), nodo(g, "S"));
  g.insert_arc(nodo(g, "M"), nodo(g, "P"));
  g.insert_arc(nodo(g, "M"), nodo(g, "Q"));
  g.insert_arc(nodo(g, "N"), nodo(g, "R"));
  g.insert_arc(nodo(g, "O"), nodo(g, "R"));
  g.insert_arc(nodo(g, "O"), nodo(g, "S"));
}

template <template <class> class List>
void imprime_lista(List<Digrafo::Node*> & l)
{
  for (typename List<Digrafo::Node*>::Iterator it(l); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << " -> ";
  cout << endl;
}

void imprime_rangos(DynList<DynList<Digrafo::Node*>> & l)
{
  int level = 0;
  for (auto i = l.get_it(); i.has_curr(); i.next(), level++)
    {
      DynList<Digrafo::Node*> & l = i.get_curr();
      cout << level << ": ";
      for (DynList<Digrafo::Node*>::Iterator j(l); j.has_curr(); j.next())
	cout << j.get_curr()->get_info() << " -> ";
      cout << endl;
    }
}

int main()
{
  Digrafo g;
  build_digraph(g);

  {
    DynDlist<Digrafo::Node*> list;
    Topological_Sort <Digrafo> () (g, list);

    imprime_lista(list);  

    list.empty();
  }

  {
    DynList<Digrafo::Node*> list;

    list = Q_Topological_Sort<Digrafo>().perform<DynList>(g);
    
    imprime_lista(list);
  }

  DynList<DynList<Digrafo::Node*>> rangos =
    Q_Topological_Sort <Digrafo> () . ranks(g);

  imprime_rangos(rangos);
} 
