
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

# include <iostream>
# include <tpl_dynDlist.H>
# include <tpl_cut_nodes.H>
# include <graph_to_tree.H>
# include <generate_tree.H>

using namespace Aleph;

struct Empty_Arc {};

struct _Node
{
  int clave;
  long df;
  long low;

  _Node() {}

  _Node(const int & c) : clave(c), df(0) {}
};


struct node_igual
{
  bool operator () (const _Node & n1, const _Node & n2)
  {
    return n1.clave == n2.clave;
  }
};


typedef Graph_Node<_Node> Nodo;

typedef Graph_Arc<Empty_Arc> Arco;

typedef List_Graph<Nodo, Arco> Grafo;

Grafo::Node * buscar_nodo(Grafo & g, const int & c)
{
  return g.search_node([c] (Grafo::Node * p) 
		     { return p->get_info().clave == c; });
}

void insertar_arco(Grafo & g, Grafo::Node * src, Grafo::Node * tgt)
{
  g.insert_arc(src, tgt, Empty_Arc());
}

void construir_grafo(Grafo & g)
{
  Grafo::Node * n1 = g.insert_node(_Node(1));
  Grafo::Node * n2 = g.insert_node(_Node(2));
  Grafo::Node * n3 = g.insert_node(_Node(3));
  Grafo::Node * n4 = g.insert_node(_Node(4));
  Grafo::Node * n5 = g.insert_node(_Node(5));
  Grafo::Node * n6 = g.insert_node(_Node(6));
  Grafo::Node * n7 = g.insert_node(_Node(7));
  Grafo::Node * n8 = g.insert_node(_Node(8));
  Grafo::Node * n9 = g.insert_node(_Node(9));
  Grafo::Node * n10 = g.insert_node(_Node(10));
  Grafo::Node * n11 = g.insert_node(_Node(11));
  Grafo::Node * n12 = g.insert_node(_Node(12));
  Grafo::Node * n13 = g.insert_node(_Node(13));
  Grafo::Node * n14 = g.insert_node(_Node(14));
  Grafo::Node * n15 = g.insert_node(_Node(15));
  Grafo::Node * n16 = g.insert_node(_Node(16));
  Grafo::Node * n17 = g.insert_node(_Node(17));
  Grafo::Node * n18 = g.insert_node(_Node(18));
  Grafo::Node * n19 = g.insert_node(_Node(19));
  Grafo::Node * n20 = g.insert_node(_Node(20));
  Grafo::Node * n21 = g.insert_node(_Node(21));
  Grafo::Node * n22 = g.insert_node(_Node(22));
  Grafo::Node * n23 = g.insert_node(_Node(23));
  Grafo::Node * n24 = g.insert_node(_Node(24));
  Grafo::Node * n25 = g.insert_node(_Node(25));
  Grafo::Node * n26 = g.insert_node(_Node(26));
  Grafo::Node * n27 = g.insert_node(_Node(27));
  Grafo::Node * n28 = g.insert_node(_Node(28));

  insertar_arco(g, n1, n2);
  insertar_arco(g, n1, n3);
  insertar_arco(g, n1, n7);
  insertar_arco(g, n1, n14);
  insertar_arco(g, n1, n21);
  insertar_arco(g, n1, n22);

  insertar_arco(g, n7, n8);
  insertar_arco(g, n7, n12);
  insertar_arco(g, n7, n10);
  insertar_arco(g, n8, n10);
  insertar_arco(g, n8, n9);
  insertar_arco(g, n9, n10);
  insertar_arco(g, n9, n13);
  insertar_arco(g, n10, n11);
  insertar_arco(g, n10, n12);
  insertar_arco(g, n10, n13);
  insertar_arco(g, n11, n13);
  insertar_arco(g, n11, n12);

  insertar_arco(g, n2, n5);
  insertar_arco(g, n2, n4);
  insertar_arco(g, n5, n4);
  insertar_arco(g, n5, n6);
  insertar_arco(g, n6, n4);
  insertar_arco(g, n6, n3);
  insertar_arco(g, n3, n4);

  insertar_arco(g, n14, n15);
  insertar_arco(g, n14, n17);
  insertar_arco(g, n14, n16);
  insertar_arco(g, n17, n20);
  insertar_arco(g, n20, n19);
  insertar_arco(g, n19, n16);
  insertar_arco(g, n19, n18);
  insertar_arco(g, n18, n16);
  insertar_arco(g, n18, n15);

  insertar_arco(g, n21, n23);
  insertar_arco(g, n21, n22);
  insertar_arco(g, n22, n23);
  insertar_arco(g, n23, n28);
  insertar_arco(g, n23, n24);
  insertar_arco(g, n28, n24);
  insertar_arco(g, n24, n27);
  insertar_arco(g, n24, n25);
  insertar_arco(g, n28, n27);
  insertar_arco(g, n27, n25);
  insertar_arco(g, n25, n26);
  insertar_arco(g, n25, n27);
  insertar_arco(g, n26, n27);
}

static long global_counter = 0;

bool visitar_df(const Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().df  = global_counter++;

  return false;
}

bool visitar_low(const Grafo &, Grafo::Node * nodo, Grafo::Arc *)
{
  nodo->get_info().low  = (long) (NODE_COOKIE(nodo));

  return false;
}


struct Clave
{
  int key;
  long count;
  long low;
};


struct Clave_Igual
{
  bool operator () (const Clave & c1, const Clave & c2) const
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

  string operator () (Tree_Node<Clave> * p)
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

  string operator () (Tree_Node<Clave> * p)
  {
    char buf[Buf_Size];

    snprintf(buf, Buf_Size, "(%c,%ld)", p->get_key().key, p->get_key().count);
  
    return string(buf);
  }
};

struct Write_Low
{
  static const size_t Buf_Size = 512;

  string operator () (Tree_Node<Clave> * p)
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


# define INDENT "    "

void imprimir_grafo(Grafo & g)
{
  cout << endl
       << "Listado de nodos (" << g.get_num_nodes() << ")" << endl;

  for (Grafo::Node_Iterator it(g); it.has_current(); it.next())
    cout << INDENT << it.get_current_node()->get_info().clave << endl;

  cout << endl
       << endl
       << "Listado de arcos (" << g.get_num_arcs() << ")"
       << endl;

  for (Grafo::Arc_Iterator it(g); it.has_current();it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();
      cout << "Arco de " << g.get_src_node(arc)->get_info().clave
	   << " a " << g.get_tgt_node(arc)->get_info().clave << endl;
    }

  cout << endl
       << endl
       << "Listado del grafo por nodos y en cada nodo por arcos" 
       << endl;
  for (Grafo::Node_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Node * src_node = it.get_current_node();
      cout << src_node->get_info().clave << endl;
      for (Grafo::Node_Arc_Iterator itor(src_node); 
	   itor.has_current(); itor.next())
	{
	  Grafo::Arc * arc = itor.get_current_arc();

	  cout << INDENT 
	       << g.get_connected_node(arc, src_node)->get_info().clave
	       << endl;
	}
    }
  cout << endl;
}


typedef std::pair<Grafo::Node *, Grafo::Node *>  No_Tree_Arc;
    

/* Inspecciona los arcos de g en el cual se ha calculado un árbol
   abarcador. Si el arco no está en el árbol abarcador, entonces éste se
   inserta en arc_list */
void generate_non_tree_arcs(Grafo & g, // g donde se calculó árbol abarcador
			    DynDlist<No_Tree_Arc> & arc_list)
{
  for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      Grafo::Arc * tarc = static_cast<Grafo::Arc*>(ARC_COOKIE(arc));

      if (tarc == NULL) // pertenece arco al árbol abarcador?
	{     // No, insertarlo en arc_list
	      // obtener nodos origen y destino en el árbol abarcador
	  Grafo::Node * src = g.get_src_node(arc);
	  Grafo::Node * tgt = g.get_tgt_node(arc);

	  arc_list.append(No_Tree_Arc(src, tgt));
	}
    }
}

void write_deway_number(int deway [], const size_t & sz, ofstream & out)
{
  for (int i = 0; i < sz; ++i)
    {
      out << deway[i];

      if (i < sz - 1)
	out << ".";
    }
}


void write_non_tree_arcs(DynDlist<No_Tree_Arc> & list,
			 Tree_Node<Clave> *      tree,
			 ofstream &              out)
{
  const size_t Buf_Size = 512;

  DynDlist<No_Tree_Arc>::Iterator it(list);
  for (int i = 0; it.has_current(); it.next(), ++i)
    {
      No_Tree_Arc arc = it.get_current();
      Grafo::Node * src = arc.first;
      Grafo::Node * tgt = arc.second;

      int deway[Buf_Size];
      size_t n = 0;

      Clave key; key.key = src->get_info().clave;
      
      Tree_Node<Clave> * node = search_deway <Tree_Node<Clave>, Clave_Igual> 
	(tree, key, deway, Buf_Size, n);

      assert(node != NULL);

      out << "Dashed-Connexion ";
      write_deway_number(deway, n, out);
      out << " ";

      key.key = tgt->get_info().clave;
      
      node = search_deway <Tree_Node<Clave>, Clave_Igual> 
	(tree, key, deway, Buf_Size, n);

      assert(node != NULL);

      write_deway_number(deway, n, out);

      if (i % 2 == 0)
	out << " L";
      else
	out << " R";

      out << " " << endl;
    }
}

/* corregir, no se está colocando correctamente el low  */
 
void generate_low(Grafo & g, DynDlist<No_Tree_Arc> & list)
{
      // inicialmente cada low(node) = df(node)
  for (Grafo::Node_Iterator i(g); i.has_current(); i.next())
    i.get_current_node()->get_info().low = i.get_current_node()->get_info().df;

      // revisar arcos no-abacadores en búsqueda de df menores
  for (DynDlist<No_Tree_Arc>::Iterator it(list); it.has_current(); it.next())
    {
      No_Tree_Arc non_arc = it.get_current();

      Grafo::Node *& gsrc = non_arc.first;
      Grafo::Node *& gtgt = non_arc.second;

      long min_low = std::min(gsrc->get_info().df,  gtgt->get_info().df);
      
      gtgt->get_info().low = std::min(gtgt->get_info().low, min_low);

      gsrc->get_info().low = std::min(gtgt->get_info().df, min_low);
    }
}


struct _Reset_Node
{
  void operator () (Grafo&, Grafo::Node * p)
  {
    p->get_info().df = 0;
    p->get_info().low = -1;
  }
};

void reset_grafo(Grafo & g)
{
  Operate_On_Nodes <Grafo, _Reset_Node> () (g);
}

void imprimir_arcos(Grafo & g, const long & color)
{
  cout << "Listado de arcos con color " << color << endl;
  for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      if (g.get_counter(arc) == color)
	cout << "Arco de " << g.get_src_node(arc)->get_info().clave
	     << " a " << g.get_tgt_node(arc)->get_info().clave << endl;
    }
  cout << endl;
}

void imprimir_arcos_corte(Grafo & g)
{
  cout << "Listado de arcos de corte *** " << endl;
  for (Grafo::Arc_Iterator it(g); it.has_current(); it.next())
    {
      Grafo::Arc * arc = it.get_current_arc();

      if (g.get_control_bits(arc).get_bit(Aleph::Cut))
	cout << "Arco de " << g.get_src_node(arc)->get_info().clave
	     << " a " << g.get_tgt_node(arc)->get_info().clave 
	     << " con color " << g.get_counter(arc) << endl;

    }
  cout << endl;
}


void write_df_low_tree(Grafo & g, Grafo::Node * src, ofstream & f)
{
      // calcular puntos de corte
  const DynList<Grafo::Node*> node_list = compute_cut_nodes(g);

  auto n = paint_subgraphs<Grafo>(g, node_list);

  depth_first_traversal(g, src, &visitar_df); // copiar df

  depth_first_traversal(g, src, &visitar_low); // copiar low

  Grafo tree = find_depth_first_spanning_tree (g, src);

      // calcular arcos no abarcadores
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  Grafo::Node * td =  static_cast<Grafo::Node *>(NODE_COOKIE(src));

  Tree_Node<Clave> * rd =  
    Graph_To_Tree_Node <Grafo, Clave, Convertir> () (tree, td);

  generate_tree <Tree_Node<Clave>, Write_Low> (rd, f);

  write_non_tree_arcs(arc_list, rd, f);

  destroy_tree(rd);
}


int main()  
{
  {
    Grafo g; // grafo con un punto de corte

    construir_grafo(g); 

    ofstream f("test-cut.Tree", ios::trunc);

    write_df_low_tree(g, g.get_first_node(), f);

  }
}
