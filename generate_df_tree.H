/* 
  This file is part of Aleph-w library

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014, 2015, 2016, 2017, 2018

  Leandro Rabindranath Leon / Alejandro Mujica

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program.  If not, see
  <https://www.gnu.org/licenses/>.
*/

# include <tpl_graph_utils.H>
# include <tpl_tree_node.H>
# include <generate_tree.H>


static long global_counter = 0;


struct Clave
{
  int key;
  long count;
  long low;
};


struct Clave_Igual
{
  const bool operator () (const Clave & c1, const Clave & c2) const
    {
      return c1.key == c2.key;
    }  
};


struct Convertir
{
  void operator () (Grafo::Node * tnode, Tree_Node<Clave> * t)
  {
    Grafo::Node * gnode = static_cast<Grafo::Node *>(NODE_COOKIE(tnode));

    Clave & clave = t->get_key();
    clave.key   = tnode->get_info().clave;
    clave.count = gnode->get_info().df;
    clave.low   = gnode->get_info().low;
  }
};


struct Write_Node
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char str[2];

    str[0] = p->get_key().key;
    str[1] = '\0';
    return string(str);
  }
};

struct Write_Df
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    snprintf(buf, Buf_Size, "(%c,%ld)", p->get_key().key, p->get_key().count);
  
    return string(buf);
  }
};

struct Write_Low
{
  static const size_t Buf_Size = 512;

  const string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    if (p->get_key().low >= 0)
      snprintf(buf, Buf_Size, "%d,%ld,%ld", 
	       p->get_key().key, p->get_key().count, p->get_key().low);
    else
      snprintf(buf, Buf_Size, "%d,%ld,-", 
	       p->get_key().key, p->get_key().count);

    return string(buf);
  }
};



void visitar_df(Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().df  = global_counter++;
}

void visitar_low(Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().low  = (long) (nodo->cookie);
}


    template <class GT, class Key>
void write_df_low_tree(GT & g, typename GT::Node * src, ofstream & f)
{
      // calcular puntos de corte
  DynDlist<typename GT::Node*> node_list = compute_cut_nodes(g);

  depth_first_traversal(g, src, &visitar_df); // copiar df

  depth_first_traversal(g, src, &visitar_low); // copiar low

  Grafo tree = find_depth_first_spanning_tree(g, src);

      // calcular arcos no abarcadores
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  typename GT::Node * td =  static_cast<typename GT::Node *>(NODE_COOKIE(src));

  Tree_Node<Key> * rd = Graph_To_Tree_Node () <Grafo, Key, Convertir>(tree, td);

  generate_tree <Tree_Node<Key>, Write_Low> (rd, f);

  write_non_tree_arcs(arc_list, rd, f);
}

