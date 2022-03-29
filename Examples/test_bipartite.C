

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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
# include <tpl_bipartite.H>

typedef List_Graph< Graph_Node<string>, Graph_Arc<Empty_Class> > Grafo;

void crear_lazo(Grafo & g, const string & src_name, const string & tgt_name)
{
  Grafo::Node * src = g.find_node(src_name);
  if (src == NULL)
    src = g.insert_node(src_name);

  Grafo::Node * tgt = g.find_node(tgt_name);
  if (tgt == NULL)
    tgt = g.insert_node(tgt_name);

  g.insert_arc(src, tgt);
}


void crear_grafo(Grafo & g)
{
  crear_lazo(g, "A", "1");
  crear_lazo(g, "A", "2");
  crear_lazo(g, "A", "4");

  crear_lazo(g, "B", "1");
  crear_lazo(g, "B", "2");
  crear_lazo(g, "B", "5");

  crear_lazo(g, "C", "2");
  crear_lazo(g, "C", "4");
  crear_lazo(g, "C", "7");

  crear_lazo(g, "D", "3");
  crear_lazo(g, "D", "5");
  crear_lazo(g, "D", "6");

  crear_lazo(g, "E", "1");
  crear_lazo(g, "E", "4");
  crear_lazo(g, "E", "6");
  crear_lazo(g, "E", "7");
  

  crear_lazo(g, "F", "5");
  crear_lazo(g, "F", "6");
  crear_lazo(g, "F", "7");

  crear_lazo(g, "G", "4");
  crear_lazo(g, "G", "6");
  crear_lazo(g, "G", "7");

  //  crear_lazo(g, "G", "x");

//   crear_lazo(g, "x", "7");
//   crear_lazo(g, "x", "u");
//   crear_lazo(g, "x", "y");
//   crear_lazo(g, "x", "z");
//   crear_lazo(g, "y", "z");
//   crear_lazo(g, "u", "z");

}

int main()
{
  Grafo g;

  crear_grafo(g);

  DynDlist<Grafo::Node *> l, r;

  Compute_Bipartite <Grafo> () (g, l, r);

  for (DynDlist<Grafo::Node *>::Iterator it(l); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << ", ";
  cout << endl;

  for (DynDlist<Grafo::Node *>::Iterator it(r); it.has_curr(); it.next())
    cout << it.get_curr()->get_info() << ", ";
  cout << endl;

  DynDlist<Grafo::Arc *> m;

  Compute_Maximum_Cardinality_Bipartite_Matching <Grafo> () (g, m);

  for (DynDlist<Grafo::Arc *>::Iterator it(m); it.has_curr(); it.next())
    {
      Grafo::Arc * a = it.get_curr();
      Grafo::Node * src = g.get_src_node(a);
      Grafo::Node * tgt = g.get_tgt_node(a);
      cout << src->get_info() << "--" << tgt->get_info() << endl;
    }
  cout << endl;

}
