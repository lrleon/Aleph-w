
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
  @file tpl_.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef QUAD_TREE_H
# define QUAD_TREE_H

# include <tpl_quad_node.H>

/** @brief Clase que modela un Quad Tree. */
class Quad_Tree <const size_t Max_Num_Points_Per_Node>
{
public:
  typedef Quad_Node<Max_Num_Points_Per_Node> Node;

private:
  Node * root;

  Point * insert(Node *& r, const Point & p)
  {
    assert(r->contains(p));

    if (r->is_leaf())
      {
        if (not r->is_full())
          return &r->add_point(p);
        else
          r->split();
      }

    Node::Quad quad = r->get_quad_to(p);
    switch(quad)
      {
      case NW: return insert(NW_CHILD(r), p);
      case NE: return insert(NE_CHILD(r), p);
      case SW: return insert(SW_CHILD(r), p);
      case SE: return insert(SE_CHILD(r), p);
      }
  }

  void empty(Node *& r)
  {
    if (r == NULL)
      return;

    empty(NW_CHILD(r));
    empty(NE_CHILD(r));
    empty(SW_CHILD(r));
    empty(SE_CHILD(r));

    delete r;
    r = NULL;
  }
 

  template <class Op>
  void operate_on_nodes(Node * r, Op & op)
  {
    if (r == NULL)
      return;

    op(r);

    operate_on_nodes<Op> (NW_CHILD(r), op);
    operate_on_nodes<Op> (NE_CHILD(r), op);
    operate_on_nodes<Op> (SW_CHILD(r), op);
    operate_on_nodes<Op> (SE_CHILD(r), op);
  }

public:
  Quad_Tree()
    : root(new Node)
  {
    // Empty
  }

  Quad_Tree(const Geom_Number & min_x, const Geom_Number & max_x,
	    const Geom_Number & min_y, const Geom_Number & max_y)
    : root(new Node(min_x, max_x, min_y, max_y));
  {
    // Empty
  }

  ~Quad_Tree()
  {
    empty(root);
  }

  Node * get_root()
  {
    return root;
  }

/**
  @brief Enter a point in the Quad_Tree
  
  @param p Point you want to insert
  
  @return A pointer to the point inserted.
*/
  Point * insert(const Point & p)
  {
    if (not root->contains(p))
      return NULL;

    return insert(root, p);
  }

/**
  @brief Search a point in the Quad_Tree
  
  @param p Point you want to search
  
  @return A pointer to the point found or NULL if not present.
*/
  Point * search(const Point & p)
  {
    Node * aux = root;

    while (not aux->is_leaf())
      {
        Node::Quad quad = aux->get_quad_to(p);

        switch(quad)
          {
          case NW: aux = NW_CHILD(aux); break;
          case NE: aux = NE_CHILD(aux); break;
          case SW: aux = SW_CHILD(aux); break;
          case SE: aux = SE_CHILD(aux); break;
          default: throw std::domain_error("p is not in tree region");
          }
      }
    return aux->search_point(p);
  }

  void remove(const Point & p)
  {
    Node * aux = root;

    while (not aux->is_leaf())
      {
        Node::Quad quad = aux->get_quad_to(p);

        switch(quad)
          {
          case NW: aux = NW_CHILD(aux); break;
          case NE: aux = NE_CHILD(aux); break;
          case SW: aux = SW_CHILD(aux); break;
          case SE: aux = SE_CHILD(aux); break;
          default: throw std::domain_error("p is not in tree region");
          }
      }

    if (not aux->remove_point(p))
      return;

    Node * parent = PARENT(aux);

    if (NW_CHILD(parent)->get_num_points() +
        NE_CHILD(parent)->get_num_points() +
        SW_CHILD(parent)->get_num_points() +
        SE_CHILD(parent)->get_num_points() < Max_Num_Points_Per_Node)
      parent->join();
  }

/** @brief Removes all nodes of Quad_Tree and leaves the root. */
  void empty()
  {
    empty(NW_CHILD(root));
    empty(NE_CHILD(root));
    empty(SW_CHILD(root));
    empty(SE_CHILD(root));
    root->empty();
  }

  template <class Op>
  void operate_on_nodes(Op & op)
  {
    operate_on_nodes <Op> (root, op);
  }

  template <class Op>
  void operate_on_nodes(Op && op = Op())
  { 
    operate_on_nodes <Op> (op);
  }
};

# endif // QUAD_TREE_H
