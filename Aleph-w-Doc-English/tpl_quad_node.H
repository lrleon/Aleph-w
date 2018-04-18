
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
  @file tpl_quad_node.H
  @date 2002 - 2014
  @author Leandro León & Alejandro Mujica (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_QUAD_NODE_H
# define TPL_QUAD_NODE_H

# include <point.H>
# include <tpl_dynDlist.H>


# define PARENT(p) ((p)->get_parent())
# define NW_CHILD(p) ((p)->get_nw_child())
# define NE_CHILD(p) ((p)->get_ne_child())
# define SW_CHILD(p) ((p)->get_sw_child())
# define SE_CHILD(p) ((p)->get_se_child())
# define COLOR(p) ((p)->get_color())
# define LEVEL(p) ((p)->get_level())

/**
  @brief Class modeling a Node to Quad Tree.
  
  @param Max_Num_Points the maximum number of points that can be stored
  
*/
template <const size_t Max_Num_Points>
class Quad_Node
{
public:

/**
  @brief Colors for a Quad Node.
  
  @details Colors:
  White represents the idle leaf node.
  Black represents the busy leaf node.
  Gray represents the non-leaf node.
*/
  enum Color
  {
    White,
    Gray,
    Black,
    Num_Colors
  };

  /** @brief Quad addresses. */
  enum Quad
  {
    NW, NE, SW, SE, Num_Quads
  };

  /** @brief Sides to which you can go from a Quad anyone. */
  enum Side
  {
    North, South, East, West, Num_Sides
  };

private:
  DynDlist <Point> points;

  Quad_Node * parent;

  Quad_Node * nw_child;
  Quad_Node * ne_child;
  Quad_Node * sw_child;
  Quad_Node * se_child;

  Color color;

  unsigned long level;

    /* Datos para la región */
  Geom_Number min_x;
  Geom_Number max_x;

  Geom_Number min_y;
  Geom_Number max_y;

static Quad_Node * get_north_neighbor(Quad_Node<Info> * v)
{
  if (PARENT(v) == NULL)
    return NULL;

  if (v->is_sw_child())
    return NW_CHILD(PARENT(v));

  if (v->is_se_child())
    return NE_CHILD(PARENT(v));

  Quad_Node<Info> * u = get_north_neighbor(PARENT(v));

  if (u == NULL or u->is_leaf())
    return u;
  else if (v->is_nw_child())
    return SW_CHILD(u);
  else
    return SE_CHILD(u);
}

static Quad_Node * get_south_neighbor(Quad_Node<Info> * v)
{
  if (PARENT(v) == NULL)
    return NULL;

  if (v->is_nw_child())
    return SW_CHILD(PARENT(v));

  if (v->is_ne_child())
    return SE_CHILD(PARENT(v));

  Quad_Node<Info> * u = get_south_neighbor(PARENT(v));

  if (u == NULL or u->is_leaf())
    return u;
  else if (v->is_sw_child())
    return NW_CHILD(u);
  else
    return NE_CHILD(u);
}

static Quad_Node * get_east_neighbor(Quad_Node<Info> * v)
{
  if (PARENT(v) == NULL)
    return NULL;

  if (v->is_nw_child())
    return NE_CHILD(PARENT(v));

  if (v->is_sw_child())
    return SE_CHILD(PARENT(v));

  Quad_Node<Info> * u = get_east_neighbor(PARENT(v));

  if (u == NULL or u->is_leaf())
    return u;
  else if (v->is_ne_child())
    return NW_CHILD(u);
  else
    return SW_CHILD(u);
}

static Quad_Node * get_west_neighbor(Quad_Node<Info> * v)
{
  if (PARENT(v) == NULL)
    return NULL;

  if (v->is_ne_child())
    return NW_CHILD(PARENT(v));

  if (v->is_se_child())
    return SW_CHILD(PARENT(v));

  Quad_Node<Info> * u = get_west_neighbor(PARENT(v));

  if (u == NULL or u->is_leaf())
    return u;
  else if (v->is_nw_child())
    return NE_CHILD(u);
  else
    return SE_CHILD(u);
}

static void get_neighbors_by_side(Quad_Node * node,
            Side & side, DynDlist<Quad_Node *> & neighbors)
{
  if (node == NULL)
    return;

  if (node->is_leaf())
    {
      neighbors.append(node);
      return;
    }

  switch(side)
    {
    case Quad_Node<Info>::West:
      get_neighbors_by_side(NW_CHILD(node), side, neighbors);
      get_neighbors_by_side(SW_CHILD(node), side, neighbors);
      break;
    case Quad_Node<Info>::East:
      get_neighbors_by_side(NE_CHILD(node), side, neighbors);
      get_neighbors_by_side(SE_CHILD(node), side, neighbors);
      break;
    case Quad_Node<Info>::North:
      get_neighbors_by_side(NW_CHILD(node), side, neighbors);
      get_neighbors_by_side(NE_CHILD(node), side, neighbors);
      break;
    case Quad_Node<Info>::South:
      get_neighbors_by_side(SW_CHILD(node), side, neighbors);
      get_neighbors_by_side(SE_CHILD(node), side, neighbors);
      break;
    }
}

public:
  Quad_Node()
    : points(), parent(NULL), nw_child(NULL), ne_child(NULL), sw_child(NULL),
      se_child(NULL), color(White), level(1), min_x(0), max_x(0), min_y(0),
      max_y(0)
  {
    /* Empty */
  }

  Quad_Node(const Geom_Number & _min_x, const Geom_Number & _max_x,
            const Geom_Number & _min_y, const Geom_Number & _max_y,
            Quad_Node * _parent = NULL)
      : points(), parent(_parent), nw_child(NULL), ne_child(NULL),
        sw_child(NULL), se_child(NULL), color(White), level(1),
        min_x(_min_x), max_x(_max_x), min_y(_min_y), max_y(_max_y)
  {
    /* Empty */
  }

  Quad_Node(const Quad_Node &) = delete;

  ~Quad_Node()
  {
    /* Empty */
  }

/** @brief Pointer reference to the node parent, you can use the macro
PARENT(p) for both reading and writing. */
  Quad_Node *& get_parent()
  {
    return parent;
  }

/** @brief Pointer reference to node northwest son, you can use the macro
NW_CHILD(p) for both reading and writing. */
  Quad_Node *& get_nw_child()
  {
    return nw_child;
  }

/** @brief Pointer reference to node northeast son, you can use the macro
NE_CHILD(p) for both reading and writing. */
  Quad_Node *& get_ne_child()
  {
    return ne_child;
  }

/** @brief Reference pointer to node southwest child, you can use the macro
SW_CHILD(p) for both reading and writing. */
  Quad_Node *& get_sw_child()
  {
    return sw_child;
  }

/** @brief Pointer reference to node southeast son, you can use the macro
SE_CHILD(p) for both reading and writing. */
  Quad_Node *& get_se_child()
  {
    return se_child;
  }

/** @brief Reference to color node, the macro can be used
COLOR(p) for both reading and writing. */
  Color & get_color()
  {
    return color;
  }

/** @brief Reference to the level tree node, you can use the macro
LEVEL(p) for both reading and writing. */
  unsigned long & get_level()
  {
    return level;
  }

  bool is_leaf() const
  {
    return color != Gray;
  }

/** @brief Says if the node is northwest child. */
  bool is_nw_child() const
  {
    if (parent == NULL)
      return false;
    return NW_CHILD(parent) == this;
  }

/** @brief Says if the node is northeast son. */
  bool is_ne_child() const
  {
    if (parent == NULL)
      return false;
    return NE_CHILD(parent) == this;
  }

/** @brief Says if the node is southwest son. */
  bool is_sw_child() const
  {
    if (parent == NULL)
      return false;
    return SW_CHILD(parent) == this;
  }

/** @brief Says if the node is southeast son. */
  bool is_se_child() const
  {
    if (parent == NULL)
      return false;
    return SE_CHILD(parent) == this;
  }

/**
  @brief Specifies whether the region represented by the node containing point
  
  @param p Point to be tested
  
  @return true if the point is within the region
*/
  bool contains(const Point & p)
  {
    return p.get_x() >= min_x and p.get_x() < max_x and
           p.get_y() >= min_y and p.get_y() < max_y;
  }

    /* TODO: Documentar para esto */
  Quad get_quad_to(const Point & p) const
  {
    assert(nw_child != NULL);
    assert(ne_child != NULL);
    assert(sw_child != NULL);
    assert(se_child != NULL);

    if (not contains(p))
      throw std::domain_error("This node does not contain that point");

    if (nw_child.contains(p))
      return NW;
    if (ne_child.contains(p))
      return NE;
    if (sw_child.contains(p))
      return SW;
    if (se_child.contains(p))
      return SE;
  }

  Point & add_point(const Point & p)
  {
    color = Black;
    return points.append(p);
  }

  const size_t & get_num_points() const
  {
    return points.size();
  }

  bool is_full() const
  {
    return points.size() == Max_Num_Points;
  }

  const Geom_Number & get_min_x() const
  {
    return min_x;
  }

  const Geom_Number & get_max_x() const
  {
    return max_x;
  }

  const Geom_Number & get_min_x() const
  {
    return min_y;
  }

  const Geom_Number & get_min_x() const
  {
    return max_y;
  }

  Geom_Number get_width() const
  {
    return max_x - min_x;
  }

  Geom_Number get_height() const
  {
    return max_y - min_y;
  }

  Geom_Number get_mid_x() const
  {
    return (min_x + max_x) / 2;
  }

  Geom_Number get_mid_y() const
  {
    return (min_y + max_y) / 2;
  }

  void split()
  {
    nw_child = new Node(min_x, min_x + get_width() / 2,
                        min_y, min_y + get_height() / 2, this);
    nw_child->level = level + 1; 

    ne_child = new Node(min_x + get_width() / 2, max_x,
                        min_y, min_y + get_height() / 2, this);
    ne_child->level = level + 1;

    sw_child = new Node(min_x, min_x + get_width() / 2,
                        min_y + get_height() / 2, max_y, this);
    sw_child->level = level + 1;

    se_child = new Node(min_x + get_width() / 2, max_x,
                        min_y + get_height() / 2, max_y, this);
    se_child->level = level + 1;

    color = Gray;
    
    while (not points.is_empty())
      {
        Point point = points.remove_first();
        Node::Quad quad = r->get_quad_to(point);
        switch(quad)
          {
          case NW: nw_child->add_point(point); break;
          case NE: ne_child->add_point(point); break;
          case SW: sw_child->add_point(point); break;
          case SE: se_child->add_point(point); break;
          }
      }
  }

  void join()
  {
    DynDlist <Point> & nw_points = nw_child->points;
 
    while(not nw_points.is_empty());
      points.append(nw_points.remove_first());

    DynDlist <Point> & ne_points = ne_child->points;
 
    while(not ne_points.is_empty());
      points.append(ne_points.remove_first());

    DynDlist <Point> & sw_points = sw_child->points;
 
    while(not sw_points.is_empty());
      points.append(sw_points.remove_first());

    DynDlist <Point> & se_points = se_child->points;
 
    while(not se_points.is_empty());
      points.append(se_points.remove_first());

    delete nw_child;
    nw_child = NULL;

    delete ne_child;
    ne_child = NULL;

    delete sw_child;
    sw_child = NULL;

    delete se_child;
    se_child = NULL;

    color = points.is_empty() ? White : Black;
  }

  Point * search_point(const Point & p)
  {
    for (DynDlist<Point>::Iterator it(points); it.has_current(); it.next())
      {
        Point & curr = it.get_current();
        if (curr == p)
          return &curr;
      }
    return NULL;
  }

  bool remove_point(const Point & p)
  {
    for (DynDlist<Point>::Iterator it(points); it.has_current(); )
      {
        Point & curr = it.get_current();
        it.next();
        if (curr == p)
          {
            points.remove(curr);
            return true;
          }
      }
    return false;
  }

  void empty()
  {
    points.empty();
  }

/**
  @brief Search nodes adjacent to node "this" and stores them in a list.
  
  @param list The list where the pointers are stored to the nodes
  adjacent.
*/
  void get_neighbors(DynDlist<Quad_Node *> & list)
  {
    Quad_Node * north_neighbor = get_north_neighbor(this);
    get_neighbors_by_side(north_neighbor, South, neighbors);

    Quad_Node * south_neighbor = get_south_neighbor(this);
    get_neighbors_by_side(south_neighbor, North, neighbors);

    Quad_Node * east_neighbor = get_east_neighbor(this);
    get_neighbors_by_side(east_neighbor, West, neighbors);

    Quad_Node * west_neighbor = get_west_neighbor(this);
    get_neighbors_by_side(west_neighbor, East, neighbors);
  }

  Quad_Node & operator = (const Quad_Node &) = delete;
};

# endif // TPL_QUAD_NODE_H
