
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
  @file tpl_euclidian_graph.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef TPL_EUCLIDIAN_GRAPH_H
# define TPL_EUCLIDIAN_GRAPH_H

# include <tpl_graph.H>
# include <point.H>

namespace Aleph
{

/** @ingroup Grafos */
      template <typename Node_Info>
class Euclidian_Node : public Graph_Node <Node_Info>
{
public:

  typedef Node_Info Item_Type;

  typedef Euclidian_Node Node;

  typedef Node_Info Node_Type;

private:

  Point position;

public:

  Euclidian_Node()
    : Graph_Node<Node_Info>(), position()
  {
    /* Empty */
  }

  Euclidian_Node(const Node_Info & info)
    : Graph_Node<Node_Info>(info), position()
  {
    /* Empty */
  }

  Euclidian_Node(const Point & _position)
    : Graph_Node<Node_Info>(), position(_position)
  {
    /* Empty */
  }

  Euclidian_Node(const Node_Info & info, const Point & _position)
    : Graph_Node<Node_Info>(info), position(_position)
  {
    /* Empty */
  }

  Euclidian_Node(Euclidian_Node * node)
    : Graph_Node<Node_Info>(node), position(node->get_position())
  {
    /* Empty */
  }

  Point & get_position() { return position; }

  const Point & get_position() const { return position; }
};  /* End class Euclidian_Node */


      template <typename Arc_Info>
class Euclidian_Arc : public Graph_Arc <Arc_Info>
{
public:

  typedef Arc_Info Arc_Item;

  typedef Euclidian_Arc Arc;

  typedef Arc_Info Arc_Type;

  Euclidian_Arc()
    : Graph_Arc<Arc_Info>()
  {
    /* Empty */
  }

  Euclidian_Arc(const Arc_Info & info)
    : Graph_Arc<Arc_Info>(info)
  {
    /* Empty */
  }

  Euclidian_Arc(void * src, void * tgt, const Arc_Info & info)
    : Graph_Arc<Arc_Info>(src, tgt, info)
  {
    /* Empty */
  }

  Euclidian_Arc(void * src, void * tgt)
    : Graph_Arc<Arc_Info>(src, tgt)
  {
    /* Empty */
  }
}; /* End class Euclidian_Arc */

    template <class __Euclidian_Node, class __Euclidian_Arc>
class Euclidian_Graph : public List_Graph <__Euclidian_Node, __Euclidian_Arc>
{
public:

  typedef __Euclidian_Node Node;

  typedef __Euclidian_Arc Arc;

  typedef List_Graph<Node, Arc> Graph;

  typedef typename Node::Node_Type Node_Type;

  typedef typename Arc::Arc_Type Arc_Type;

  Euclidian_Graph() : Graph()
  {
    /* Empty */
  }

  Euclidian_Graph(const Euclidian_Graph <Node, Arc> & euclidian_graph)
    : Graph()
  {
    copy_graph(*this, const_cast<Euclidian_Graph <Node, Arc> &> (euclidian_graph), false);
  }

  virtual Node * insert_node(Node * node)
  {
    return Graph::insert_node(node);
  }

  virtual Node * insert_node(const Node_Type & info)
  {
    return insert_node(new Node(info));
  }

  virtual Node * insert_node(const Point & position)
  {
    return insert_node(new Node(position));
  }

  virtual Node * insert_node(const Node_Type & info, const Point & position)
  {
    return insert_node(new Node(info, position));
  }

  Geom_Number get_distance(Arc * arc)
  {
    const Point & src_point = get_src_node(arc)->get_position();
    const Point & tgt_point = get_tgt_node(arc)->get_position();
    return src_point.distance_with(tgt_point);
  }

  Euclidian_Graph<Node, Arc> & operator = (Euclidian_Graph<Node, Arc> & eg)
  {
    if (this == &eg)
      return *this;
    copy_graph(*this, const_cast<Euclidian_Graph<Node, Arc> &>(eg), false);
    return *this;
  }

  virtual ~Euclidian_Graph()
  {
    clear_graph(*this);
  }

  Node * search_node(const Point &);
};  /* End class Euclidian_Graph */

  template <class __Euclidian_Node, class __Euclidian_Arc>
class Euclidian_Digraph 
    : public Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>
{
public:

  Euclidian_Digraph()
  {
    Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::digraph = true;
  }

  Euclidian_Digraph(const Euclidian_Digraph <__Euclidian_Node, __Euclidian_Arc> & euclidian_digraph)
      : Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>(euclidian_digraph)
    {
      Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::digraph = true;
    }

    Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & operator = (Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & eg)
    {
      return Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::operator = (eg);
    }

  };  /* End class Euclidian_Digraph */

  template <class Node, class Arc>
  Node * Euclidian_Graph<Node, Arc>::search_node(const Point & point)
  {
    for (typename Euclidian_Graph<Node, Arc>::Node_Iterator it; it.has_current(); it.next())
      {
        Euclidian_Graph<Node, Arc>::Node * curr = it.get_current();
        if (curr->get_position() == point)
          return curr;
      }
    return NULL;
  }

  template <class __Euclidian_Graph>
  class Abstract_Euclidian_Plane
  {
    Point * ptr_east_point;
    Point * ptr_north_point;
    Point * ptr_west_point;
    Point * ptr_south_point;

    DynArray<Point> points;

    Geom_Number x_node_ratio;
    Geom_Number y_node_ratio;

    Geom_Number x_scale;
    Geom_Number y_scale;

  public:
    Abstract_Euclidian_Plane()
      : x_node_ratio(0), y_node_ratio(0), x_scale(1), y_scale(1),
        ptr_east_point(NULL), ptr_north_point(NULL), ptr_west_point(NULL), ptr_south_point(NULL)
    {
      /* Empty */
    }

    Abstract_Euclidian_Plane(__Euclidian_Graph & graph)
      : x_node_ratio(0), y_node_ratio(0), x_scale(1), y_scale(1),
        ptr_east_point(NULL), ptr_north_point(NULL), ptr_west_point(NULL), ptr_south_point(NULL)
    {
      if (graph.get_num_nodes() < 1)
        return;

      typename __Euclidian_Graph::Node_Iterator itor(graph);
      points.append(itor.get_current()->get_position());
      ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point = &points.access(0);

      for (int i = 1 ; itor.has_current(); itor.next(), ++i)
        {
          const Point & p = itor.get_current()->get_position();
          points.append(p);
          if (p.get_x() < ptr_west_point->get_x())
            ptr_west_point = &points.access(i);
          if (p.get_y() > ptr_north_point->get_y())
            ptr_north_point = &points.access(i);
          if (p.get_x() > ptr_east_point->get_x())
            ptr_east_point = &points.access(i);
          if (p.get_y() < ptr_south_point->get_y())
            ptr_south_point = &points.access(i);
        }
    }

    ~Abstract_Euclidian_Plane()
    {
      /* Empty */
    }

    Point & add_point(typename __Euclidian_Graph::Node * node)
    {
      if (node == NULL)
        throw std::domain_error("node is NULL");
      points.append(node->get_position());
      Point & p = points.top();
      if (points.size() == 1)
        ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point = &points.access(0);
      else
        {
          if (p.get_x() < ptr_west_point->get_x())
            ptr_west_point = &p;
          if (p.get_y() > ptr_north_point->get_y())
            ptr_north_point = &p;
          if (p.get_x() > ptr_east_point->get_x())
            ptr_east_point = &p;
          if (p.get_y() < ptr_south_point->get_y())
            ptr_south_point = &p;
        }
      return p;
    }

    const Point & get_west_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_west_point;
    }

    const Point & get_north_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_north_point;
    }

    const Point & get_east_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_east_point;
    }

    const Point & get_south_point() const
    {
      if (points.size() < 1)
        throw std::logic_error("There are no points on plane");
      return *ptr_south_point;
    }

    const Geom_Number get_width() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_east_point->get_x() - ptr_west_point->get_x();
    }

    const Geom_Number get_height() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_north_point->get_y() - ptr_south_point->get_y();
    }

    const Geom_Number & get_x_node_ratio() const { return x_node_ratio; }

    void set_x_node_ratio(const Geom_Number & _x_node_ratio) { x_node_ratio = _x_node_ratio; }

    const Geom_Number & get_y_node_ratio() const { return y_node_ratio; }

    void set_y_node_ratio(const Geom_Number & _y_node_ratio) { y_node_ratio = _y_node_ratio; }

    const Geom_Number & get_x_scale() const { return x_scale; }

    void set_x_scale(const Geom_Number & _x_scale) { x_scale = _x_scale; }

    const Geom_Number & get_y_scale() const { return y_scale; }

    void set_y_scale(const Geom_Number & _y_scale) { y_scale = _y_scale; }

  }; /* End class Abstract_Euclidian_Plane */

} /* End namespace Aleph */

# endif // TPL_EUCLIDIAN_GRAPH_H
