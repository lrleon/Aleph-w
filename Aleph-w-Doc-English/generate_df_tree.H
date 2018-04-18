
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file generate_df_tree.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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
/** 
  @brief Calculate points of cut. 
*/
  DynDlist<typename GT::Node*> node_list;
  compute_cut_nodes(g, node_list);
/** 
  @brief Copy df 
*/
  depth_first_traversal(g, src, &visitar_df);
/** 
  @brief Copy low 
*/
  depth_first_traversal(g, src, &visitar_low); 

/** 
  @brief encompassing tree. 
*/
  Grafo tree;

/** 
  @brief Calculate encompassig tree in depth. 
*/
  find_depth_first_spanning_tree <GT> (g, src, tree);

/** 
  @brief Calculate arches no encompassing. 
*/
  DynDlist<No_Tree_Arc> arc_list;
  generate_non_tree_arcs(g, arc_list);

  typename GT::Node * td =  static_cast<typename GT::Node *>(NODE_COOKIE(src));

  Tree_Node<Key> * rd = Graph_To_Tree_Node () <Grafo, Key, Convertir>(tree, td);

  generate_tree <Tree_Node<Key>, Write_Low> (rd, f);

  write_non_tree_arcs(arc_list, rd, f);
}
