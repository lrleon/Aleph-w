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

/** @file tpl_euclidian_graph.H
 *  @brief Euclidean graph with 2D coordinates.
 *
 *  Graph where nodes have 2D positions and arc weights
 *  are Euclidean distances.
 *
 *  @see point_utils.H Point utilities
 *  @ingroup Graphs
 *  @author Leandro Rabindranath León
 */

# ifndef TPL_EUCLIDIAN_GRAPH_H
# define TPL_EUCLIDIAN_GRAPH_H

# include <tpl_graph.H>
# include <point.H>
# include <ah-errors.H>

namespace Aleph
{

  /**

     @ingroup Graphs
   */
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
};  // End class Euclidian_Node


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
}; // End class Euclidian_Arc

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
    copy_graph(*this, 
	       const_cast<Euclidian_Graph <Node, Arc>&> (euclidian_graph), 
	       false);
  }

  Node * insert_node(Node * node) override
  {
    return Graph::insert_node(node);
  }

  Node * insert_node(const Node_Type & info)
  {
    return insert_node(new Node(info));
  }

  Node * insert_node(const Point & position)
  {
    return insert_node(new Node(position));
  }

  Node * insert_node(const Node_Type & info, const Point & position)
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
};  // End class Euclidian_Graph

  template <class __Euclidian_Node, class __Euclidian_Arc>
class Euclidian_Digraph 
    : public Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>
{
public:

  Euclidian_Digraph()
  {
    Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::digraph = true;
  }

  Euclidian_Digraph
  (const Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc>& euclidian_digraph)
      : Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>(euclidian_digraph)
  {
    Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::digraph = true;
  }

  Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & 
  operator = (Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & eg)
  {
    return Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::operator = (eg);
  }
  };  // End class Euclidian_Digraph

  template <class Node, class Arc>
  Node * Euclidian_Graph<Node, Arc>::search_node(const Point & point)
  {
    for (typename Euclidian_Graph<Node, Arc>::Node_Iterator it; 
	 it.has_curr(); it.next_ne())
      {
        Euclidian_Graph<Node, Arc>::Node * curr = it.get_curr();
        if (curr->get_position() == point)
          return curr;
      }
    return nullptr;
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
        ptr_east_point(nullptr), ptr_north_point(nullptr), 
	ptr_west_point(nullptr), ptr_south_point(nullptr)
    {
      // Empty
    }

    Abstract_Euclidian_Plane(__Euclidian_Graph & graph)
      : x_node_ratio(0), y_node_ratio(0), x_scale(1), y_scale(1),
        ptr_east_point(nullptr), ptr_north_point(nullptr), 
	ptr_west_point(nullptr), ptr_south_point(nullptr)
    {
      if (graph.get_num_nodes() < 1)
        return;

      typename __Euclidian_Graph::Node_Iterator itor(graph);
      points.append(itor.get_curr()->get_position());
      ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point = 
	&points.access(0);

      for (int i = 1 ; itor.has_curr(); itor.next_ne(), ++i)
        {
          const Point & p = itor.get_curr()->get_position();
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
      ah_domain_error_if(node == nullptr) << "node is nullptr";
      points.append(node->get_position());
      Point & p = points.top();
      if (points.size() == 1)
        ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point = 
	  &points.access(0);
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
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_west_point;
    }

    const Point & get_north_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_north_point;
    }

    const Point & get_east_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_east_point;
    }

    const Point & get_south_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_south_point;
    }

    Geom_Number get_width() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_east_point->get_x() - ptr_west_point->get_x();
    }

    Geom_Number get_height() const
    {
      if (points.size() < 1)
        return Geom_Number(0);
      return ptr_north_point->get_y() - ptr_south_point->get_y();
    }

    const Geom_Number & get_x_node_ratio() const { return x_node_ratio; }

    void set_x_node_ratio(const Geom_Number & _x_node_ratio) 
    { x_node_ratio = _x_node_ratio; }

    const Geom_Number & get_y_node_ratio() const { return y_node_ratio; }

    void set_y_node_ratio(const Geom_Number & _y_node_ratio) 
    { y_node_ratio = _y_node_ratio; }

    const Geom_Number & get_x_scale() const { return x_scale; }

    void set_x_scale(const Geom_Number & _x_scale) { x_scale = _x_scale; }

    const Geom_Number & get_y_scale() const { return y_scale; }

    void set_y_scale(const Geom_Number & _y_scale) { y_scale = _y_scale; }

  }; // End class Abstract_Euclidian_Plane

} // End namespace Aleph

# endif // TPL_EUCLIDIAN_GRAPH_H

