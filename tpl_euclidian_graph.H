
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in all
  copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
*/


/** @file tpl_euclidian_graph.H
 *  @brief Euclidean graph with 2D coordinates.
 *
 *  Graph where nodes have 2D positions and arc weights
 *  are Euclidean distances automatically computed from node positions.
 *
 *  @par Example: City network with geographic positions
 *  @code
 *  using Point2D = Point2D_Num<double>;
 *  using EG = Euclidian_Graph<Graph_Node<string>, Graph_Arc<Empty_Class>, Point2D>;
 *  
 *  EG city_map;
 *  
 *  // Add cities with lat/lon coordinates
 *  auto nyc = city_map.insert_node("NYC", Point2D(40.7128, -74.0060));
 *  auto boston = city_map.insert_node("Boston", Point2D(42.3601, -71.0589));
 *  auto dc = city_map.insert_node("DC", Point2D(38.9072, -77.0369));
 *  
 *  // Insert arc - distance computed automatically from positions
 *  city_map.insert_arc(nyc, boston);
 *  
 *  // Get computed Euclidean distance
 *  auto arc = city_map.search_arc(nyc, boston);
 *  double distance = city_map.get_distance(arc);
 *  @endcode
 *
 *  @par Example: Nearest neighbor search
 *  @code
 *  auto query_pos = Point2D(41.0, -73.0);
 *  
 *  double min_dist = numeric_limits<double>::max();
 *  EG::Node* nearest = nullptr;
 *  
 *  for (Node_Iterator<EG> it(city_map); it.has_curr(); it.next()) {
 *    auto node = it.get_curr();
 *    double dist = query_pos.distance_with(city_map.get_position(node));
 *    if (dist < min_dist) {
 *      min_dist = dist;
 *      nearest = node;
 *    }
 *  }
 *  @endcode
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
  class Euclidian_Node : public Graph_Node<Node_Info>
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

    Euclidian_Node(Euclidian_Node *node)
      : Graph_Node<Node_Info>(node), position(node->get_position())
    {
      /* Empty */
    }

    Point &get_position() { return position; }

    const Point &get_position() const { return position; }
  }; // End class Euclidian_Node


  template <typename Arc_Info>
  class Euclidian_Arc : public Graph_Arc<Arc_Info>
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

    Euclidian_Arc(void *src, void *tgt, const Arc_Info & info)
      : Graph_Arc<Arc_Info>(src, tgt, info)
    {
      /* Empty */
    }

    Euclidian_Arc(void *src, void *tgt)
      : Graph_Arc<Arc_Info>(src, tgt)
    {
      /* Empty */
    }
  }; // End class Euclidian_Arc

  template <class __Euclidian_Node, class __Euclidian_Arc>
  class Euclidian_Graph : public List_Graph<__Euclidian_Node, __Euclidian_Arc>
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

    Euclidian_Graph(const Euclidian_Graph<Node, Arc> & euclidian_graph)
      : Graph()
    {
      copy_graph(*this,
                 const_cast<Euclidian_Graph<Node, Arc> &>(euclidian_graph),
                 false);
    }

    Node * insert_node(Node *node) noexcept override
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

    Geom_Number get_distance(Arc *arc)
    {
      const Point & src_point = this->get_src_node(arc)->get_position();
      const Point & tgt_point = this->get_tgt_node(arc)->get_position();
      return src_point.distance_with(tgt_point);
    }

    Euclidian_Graph<Node, Arc> &operator =(Euclidian_Graph<Node, Arc> & eg)
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
  }; // End class Euclidian_Graph

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
    (const Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & euclidian_digraph)
      : Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>(euclidian_digraph)
    {
      Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::digraph = true;
    }

    Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> &
    operator =(Euclidian_Digraph<__Euclidian_Node, __Euclidian_Arc> & eg)
    {
      return Euclidian_Graph<__Euclidian_Node, __Euclidian_Arc>::operator =(eg);
    }
  }; // End class Euclidian_Digraph

  template <class Node, class Arc>
  Node * Euclidian_Graph<Node, Arc>::search_node(const Point & point)
  {
    for (typename Euclidian_Graph<Node, Arc>::Node_Iterator it(*this);
         it.has_curr(); it.next_ne())
      {
        Euclidian_Graph<Node, Arc>::Node *curr = it.get_curr();
        if (curr->get_position() == point)
          return curr;
      }
    return nullptr;
  }

  template <class __Euclidian_Graph>
  class Abstract_Euclidian_Plane
  {
    Point *ptr_east_point;
    Point *ptr_north_point;
    Point *ptr_west_point;
    Point *ptr_south_point;

    DynArray<Point> points;

    Geom_Number x_node_ratio;
    Geom_Number y_node_ratio;

    Geom_Number x_scale;
    Geom_Number y_scale;

  public:
    Abstract_Euclidian_Plane()
      : ptr_east_point(nullptr), ptr_north_point(nullptr), ptr_west_point(nullptr), ptr_south_point(nullptr),
        x_node_ratio(0), y_node_ratio(0),
        x_scale(1), y_scale(1)
    {
      // Empty
    }

    Abstract_Euclidian_Plane(__Euclidian_Graph & graph)
      : ptr_east_point(nullptr), ptr_north_point(nullptr), ptr_west_point(nullptr), ptr_south_point(nullptr),
        x_node_ratio(0), y_node_ratio(0),
        x_scale(1), y_scale(1)
    {
      if (graph.get_num_nodes() < 1)
        return;

      typename __Euclidian_Graph::Node_Iterator itor(graph);
      points.append(itor.get_curr()->get_position());
      ptr_west_point = ptr_north_point = ptr_east_point = ptr_south_point =
                                                          &points.access(0);

      for (int i = 1; itor.has_curr(); itor.next_ne(), ++i)
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

    Point &add_point(typename __Euclidian_Graph::Node *node)
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

    const Point &get_west_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_west_point;
    }

    const Point &get_north_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_north_point;
    }

    const Point &get_east_point() const
    {
      ah_logic_error_if(points.size() < 1) << "There are no points on plane";
      return *ptr_east_point;
    }

    const Point &get_south_point() const
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

    const Geom_Number &get_x_node_ratio() const { return x_node_ratio; }

    void set_x_node_ratio(const Geom_Number & _x_node_ratio)
    {
      x_node_ratio = _x_node_ratio;
    }

    const Geom_Number &get_y_node_ratio() const { return y_node_ratio; }

    void set_y_node_ratio(const Geom_Number & _y_node_ratio)
    {
      y_node_ratio = _y_node_ratio;
    }

    const Geom_Number &get_x_scale() const { return x_scale; }

    void set_x_scale(const Geom_Number & _x_scale) { x_scale = _x_scale; }

    const Geom_Number &get_y_scale() const { return y_scale; }

    void set_y_scale(const Geom_Number & _y_scale) { y_scale = _y_scale; }
  }; // End class Abstract_Euclidian_Plane
} // End namespace Aleph

# endif // TPL_EUCLIDIAN_GRAPH_H
