
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
  @file polygon.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef POLYGONE_H
# define POLYGONE_H

# include <dlink.H>
# include <point.H>

using namespace std;

using namespace Aleph;


class Vertex : public Point, public Dlink
{

public:

  Vertex() { /* Empty */ }

  Vertex(const Point & point) : Point(point) { /* Empty */ }

  static Vertex * dlink_to_vertex(Dlink * link) 
  {
    Vertex * v = static_cast<Vertex *>(link);

    const Point p = *v;

    return static_cast<Vertex *>(link);
  }

  Vertex & prev_vertex() 
  {
    assert(not this->is_Empty());

    if (this->is_unitarian())
      throw std::domain_error("There is an only vertex");

    return * dlink_to_vertex(this->get_prev());
  }

  Vertex & next_vertex() 
  {
    assert(not this->is_Empty());

    if (this->is_unitarian())
      throw std::domain_error("There is an only vertex");

    return * dlink_to_vertex(this->get_next());
  }

};


class Regular_Polygon;


class Polygon : public Geom_Object
{
  Dlink vertex_list;

  size_t num_vertex;

/**
  @brief It indicates whether the polygon has been closed
*/
  bool __is_closed;

  Point lowest;
  Point highest;
  Point leftmost;
  Point rightmost;

  void update_extreme_points(const Point & point)
  {
    if (num_vertex == 0)
      {
	leftmost = rightmost = lowest = highest = point;

	return;
      }

    if (point.get_x() < leftmost.get_x())
      leftmost = point;

    if (point.get_x() > rightmost.get_x())
      rightmost = point;

    if (point.get_y() < lowest.get_y())
      lowest = point;

    if (point.get_y() > highest.get_y())
      highest = point;
  }

  void delete_points()
  {
    while (not vertex_list.is_Empty())
      delete Vertex::dlink_to_vertex(vertex_list.remove_next());

    num_vertex = 0;
    __is_closed = false;
  }

  void copy_points(const Polygon & poly) 
  {
    Dlink * list = (Dlink*) &poly.vertex_list;

    for (Dlink::Iterator it(list); it.has_current(); it.next())
      vertex_list.
	append(new Vertex(*Vertex::dlink_to_vertex(it.get_current())));
  }

  void copy_regular_polygon(const Regular_Polygon & poly);

public:

  Polygon() : num_vertex(0), __is_closed(false) { /* Empty */ }

  ~Polygon()
  {
    delete_points();
  }

  Polygon(const Polygon & poly) 
    : Geom_Object(poly),
      num_vertex(poly.num_vertex), __is_closed(poly.__is_closed), 
      lowest(poly.lowest), highest(poly.highest),
      leftmost(poly.leftmost), rightmost(poly.rightmost)
  {
    copy_points(poly);
  }

  Polygon(const Regular_Polygon & poly) 
    : num_vertex(0), __is_closed(false) 
  {
    copy_regular_polygon(poly);
  }

  Polygon & operator = (const Polygon & poly) 
  {
    if (this == &poly)
      return *this;

    delete_points();

    num_vertex = poly.num_vertex;
    __is_closed = poly.__is_closed;
    lowest      = poly.lowest;
    highest     = poly.highest;
    leftmost    = poly.leftmost;
    rightmost   = poly.rightmost;

    copy_points(poly);

    return *this;
  }

  Polygon & operator = (const Regular_Polygon & poly) 
  {
    delete_points();
    copy_regular_polygon(poly);

    return *this;
  }

  const Point & lowest_point() const { return lowest; }

  const Point & highest_point() const { return highest; }

  const Point & leftmost_point() const { return leftmost; }

  const Point & rightmost_point() const { return rightmost; }

  const bool & is_closed() const { return __is_closed; }

  const size_t & size() const { return num_vertex; }

  struct Vertex_Iterator : public Dlink::Iterator
  {
    Vertex_Iterator(Polygon & poly) : Dlink::Iterator(poly.vertex_list) 
    {
      if (poly.vertex_list.is_Empty())
	throw std::domain_error("Polygon has not any vertex");
    }

    Vertex * get_current_vertex()
    {
      return Vertex::dlink_to_vertex(get_current());
    }
  };

  class Segment_Iterator : public Dlink::Iterator
  {
    Polygon & poly;

  public:

    Segment_Iterator(Polygon & __poly) 
      : Dlink::Iterator(__poly.vertex_list), poly(__poly)
    {
      if (poly.vertex_list.is_unitarian_or_Empty())
	throw std::domain_error("Polygon has less than two vertex");
    }

    bool has_current() const
    {
      if (this->is_in_last())
	return poly.is_closed() ? true : false;

      return Dlink::Iterator::has_current();
    }

    Segment get_current_segment() 
    {
      if (not poly.is_closed() and this->is_in_last())
	throw std::domain_error("Segment iterator is in the last point and "
				"it is not closed");
      
      Vertex * src = Vertex::dlink_to_vertex(this->get_current());

    /* Tgt deciding point as it is the last point or not */
      Vertex & tgt = 
	this->is_in_last() ? poly.get_first_vertex() : src->next_vertex();
      
      return Segment(*src, tgt);
    }
  };

  Vertex & get_first_vertex() 
  {
    if (vertex_list.is_Empty())
      throw std::domain_error("Polygon has not have any vertex");

    return * Vertex::dlink_to_vertex(vertex_list.get_next());
  }

  Vertex & get_last_vertex() 
  {
    if (vertex_list.is_Empty())
      throw std::domain_error("Polygon has not have any vertex");

    return * Vertex::dlink_to_vertex(vertex_list.get_prev());
  }

  Segment get_first_segment() 
  {
    if (vertex_list.is_unitarian_or_Empty())
      throw std::domain_error("polygon has less than two vertex");

    Vertex & first_vertex = get_first_vertex();

    return Segment(first_vertex, first_vertex.next_vertex());
  }

  Segment get_last_segment() 
  {
    if (vertex_list.is_unitarian_or_Empty())
      throw std::domain_error("polygon has less than two vertex");

    Vertex & last_vertex = get_last_vertex();

    return Segment(last_vertex.prev_vertex(), last_vertex);
  }

/**
  @retrun Returns true if sg intersects a polygon side 
*/
  bool intersects_with(const Segment & sg)
  {
    /** Browse all sides and verify that no intersection */
    for (Segment_Iterator it(*this); it.has_current(); it.next())
      {
	const Segment & curr_side = it.get_current_segment();

	if (curr_side.intersects_with(sg))
	  return true;
      }

    return false;
  }

  void add_vertex(const Point & point)
  {
    if (__is_closed)
      throw std::domain_error("Polygon is already closed");

    /* Check if the new point is collinear with the last segment */
    if (num_vertex > 1)
      {
	Segment last_sg = get_last_segment();

	if (point.is_colinear_with(last_sg))
	  {
	    if (point.is_inside(last_sg))
	      throw std::domain_error("new vertex is inside of last polygon's "
				      "segment");

	  /* Collinear ==> we substitute the latter for this new */
	    Vertex & last_vertex = get_last_vertex();

	    Point & last_point = last_vertex;

	    last_point = point;

	    update_extreme_points(point);

	    return;
	  }
      }
      
    /* If there are segments, then we must verify that the new 
    point will not cause a crossing */
    if (num_vertex >= 3) 
      {

    /* New side to settle the new point */
	const Segment new_side(get_last_vertex(), point);

	  /** Check for intersection with the other sides except 
    the penultimate (with which it shares a point) */
	for (Segment_Iterator it(*this); true; it.next())
	  {
	    const Segment curr_side = it.get_current_segment();

	    if (curr_side == get_last_segment())
	      break;
	
	    if (curr_side.intersects_with(new_side))
	      throw std::domain_error("new side intersects");
	  }
      }

    /* Insert new vertex */
    vertex_list.append(new Vertex(point));

    update_extreme_points(point);

    ++num_vertex;
  }

  void close()
  {
    if (__is_closed)
      throw std::domain_error("Polygon is already closed");

    if (num_vertex >= 4)
      {

    /* Check for intersection with the other sides except the first
    and the penultimate (with which the end points are shared) */
	const Segment last_side(get_first_vertex(), get_last_vertex());

	Segment_Iterator it(*this); 
	
	for (it.next(); true; it.next())
	  {
	    const Segment curr_side = it.get_current_segment();

	    if (curr_side == get_last_segment())
	      break;
	
	    if (curr_side.intersects_with(last_side))
	      throw std::domain_error("closing causes an intersection");
	  }
      }
    
    __is_closed = true;
  }

  Polygon(const Triangle & tr) 
    : Geom_Object(tr), num_vertex(0), __is_closed(false)
  {
    add_vertex(tr.get_p1());
    add_vertex(tr.get_p2());
    add_vertex(tr.get_p2());

    close();
  }
};


class Regular_Polygon : public Geom_Object
{
  Point center;

  double side_size;

  size_t num_vertex;

  double angle;

  double r;

  double beta; /* Angle between vertices */

public:

  Regular_Polygon() : side_size(0), num_vertex(0) { /* Empty */  }

  Regular_Polygon(const Point &  c, 
		  const double & side_sz, 
		  const size_t & n,
		  const double & ang = 0)
    : center(c), side_size(side_sz), num_vertex(n), angle(ang), beta(2*PI/n)
  {
    if (n < 3)
      throw std::domain_error("Polygon sides is less than 3");

    /* Radius calculated using law of sines with angles between
    vertices (which are equal) and the angle between the face and the radius */
    const double alpha = (PI - beta)/2; /* Angle between radio and next */
    
    r = side_size*sin(alpha)/sin(beta);
  }

  const double & get_side_size() const { return side_size; }

  const Point & get_center() const { return center;}

  const size_t & size() const { return num_vertex; }

  const double & radius() const { return r; }

  bool is_closed() const { return true; }

/** 
  @brief Returns the i-th point of the polygon.
  0 corresponds to the center point - P (0, r), where r is the radius 
*/
  const Point get_vertex(const size_t & i) const
  {
    if (i >= num_vertex)
      throw std::out_of_range
	(gnu::autosprintf("vertex %d is greater than %d", i, num_vertex));

    /* First segment is the negative vertically from the center.
    Starting point is is about fate of this segment origin at the center */
    Segment sg(center, center - Point(0, r));

    sg.rotate(i*beta + angle);

    return sg.get_tgt_point();
  }

  const Point get_first_vertex() const { return get_vertex(0); }

  const Point get_last_vertex() const { return get_vertex(num_vertex - 1); }

  Segment get_first_segment() const
  {
    return Segment(get_vertex(0), get_vertex(1));
  }

  Segment get_last_segment() const
  {
    return Segment(get_vertex(0), get_vertex(num_vertex - 1));
  }

/** 
  @brief This iterator is performed to enable code
  generic to use irregular polygons
*/
  class Vertex_Iterator
  {
    const Regular_Polygon & poly;

    size_t curr;

    /* To provide compatibility with Polygon::Vertex_Iterator */
    Vertex vertex; 

  public:

    Vertex_Iterator(const Regular_Polygon & __poly)
      : poly(__poly), curr(0)
    {
      /* Empty */ 
    }

    bool has_current() const { return curr < poly.size(); }

    Vertex * get_current_vertex() 
    {
      if (not has_current())
	throw std::overflow_error("Iterator has not current");

      vertex = poly.get_vertex(curr);

      return &vertex;
    }

    void next()
    {
      if (not has_current())
	throw std::overflow_error("Iterator has not current");

      ++curr;
    }

    void prev()
    {
      --curr;

      if (not has_current())
	throw std::underflow_error("Iterator has not current");
    }
  };

/** 
  @brief This iterator is performed to enable code
  generic to use irregular polygons
*/
  class Segment_Iterator
  {
    const Regular_Polygon & poly;

    size_t curr;

  public:

    Segment_Iterator(const Regular_Polygon & __poly)
      : poly(__poly), curr(0)
    {
      /* Empty */
    }

    bool has_current() const { return curr < poly.size(); }

    const Segment get_current_segment() const
    {
      if (not has_current())
	throw std::overflow_error("Iterator has not current");

      return Segment(poly.get_vertex(curr), 
		     poly.get_vertex((curr + 1) % poly.size()));
    }

    void next()
    {
      if (not has_current())
	throw std::overflow_error("Iterator has not current");

      ++curr;
    }

    void prev()
    {
      --curr;

      if (not has_current())
	throw std::underflow_error("Iterator has not current");
    }
  };

    /* For reasons of simplicdad,
    We use as end points of the compass the circumference */
  
  Point lowest_point() const { return center + Point(0, -r); }

  Point highest_point() const { return center + Point(0, r); }

  Point leftmost_point() const { return center + Point(-r, 0); }

  Point rightmost_point() const { return center + Point(r, 0); }
};

    /* TODO: must have side_size error seems to be always the same ar*/

inline void Polygon::copy_regular_polygon(const Regular_Polygon & poly)
{
  assert(num_vertex == 0 and not __is_closed);

  for (int i = 0; i < poly.size(); ++i)
    add_vertex(poly.get_vertex(i));

  close();
}

# endif // POLYGONE_H
