
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
# ifndef POLYGON_H
# define POLYGON_H

# include <dlink.H>
# include <point.H>

using namespace std;

using namespace Aleph;

class Vertex : public Point, public Dlink
{
public:

  Vertex() { /* empty */ }

  Vertex(const Point & point) : Point(point) { /* empty */ }

  Vertex(const Vertex & vertex) : Point(vertex), Dlink() { /* empty */ }

  static Vertex * dlink_to_vertex(Dlink * link) 
  {
    return static_cast<Vertex*>(link);
  }

  static const Vertex * dlink_to_vertex(const Dlink * link) 
  {
    return static_cast<const Vertex*>(link);
  }

  const Vertex & prev_vertex() const
  {
    assert(not this->is_empty());

    if (this->is_unitarian())
      throw std::domain_error("There is an only vertex");

    return * dlink_to_vertex(this->get_prev());
  }

  const Vertex & next_vertex() const
  {
    assert(not this->is_empty());

    if (this->is_unitarian())
      throw std::domain_error("There is an only vertex");

    return * dlink_to_vertex(this->get_next());
  }

  // TODO: next_segment y prev_segment
};


class Regular_Polygon;

class Polygon : public Geom_Object
{
  Dlink vertex_list;

  size_t num_vertex;

  bool __is_closed; // indica si el polígono ya ha sido cerrado

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
    while (not vertex_list.is_empty())
      delete Vertex::dlink_to_vertex(vertex_list.remove_next());

    num_vertex = 0;
    __is_closed = false;
  }

  void copy_points(const Polygon & poly) 
  {
    Dlink * list = const_cast<Dlink *>(&poly.vertex_list);

    for (Dlink::Iterator it(list); it.has_current(); it.next())
      vertex_list.
	append(new Vertex(*Vertex::dlink_to_vertex(it.get_current())));
  }

  void copy_regular_polygon(const Regular_Polygon & poly);

public:

  Polygon() : num_vertex(0), __is_closed(false) { /* empty */ }

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

  Polygon(Polygon && poly)
    : Polygon()
  {
    vertex_list.swap(poly.vertex_list);
    std::swap(num_vertex, poly.num_vertex);
    std::swap(__is_closed, poly.__is_closed);
    std::swap(lowest, poly.lowest);
    std::swap(highest, poly.highest);
    std::swap(leftmost, poly.leftmost);
    std::swap(rightmost, poly.rightmost);
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

  Polygon & operator = (Polygon && poly)
  {
    vertex_list.swap(poly.vertex_list);
    std::swap(num_vertex, poly.num_vertex);
    std::swap(__is_closed, poly.__is_closed);
    std::swap(lowest, poly.lowest);
    std::swap(highest, poly.highest);
    std::swap(leftmost, poly.leftmost);
    std::swap(rightmost, poly.rightmost);
    
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
    Vertex_Iterator(const Polygon & poly) : Dlink::Iterator(poly.vertex_list) 
    {
      if (poly.vertex_list.is_empty())
	throw std::domain_error("Polygon has not any vertex");
    }

    Vertex & get_current_vertex()
    {
      return *Vertex::dlink_to_vertex(get_current());
    }
  };

  class Segment_Iterator : public Dlink::Iterator
  {
    Polygon & poly;

  public:

    Segment_Iterator(const Polygon & __poly) 
      : Dlink::Iterator(__poly.vertex_list), poly(const_cast<Polygon&>(__poly))
    {
      if (poly.vertex_list.is_unitarian_or_empty())
	throw std::domain_error("Polygon has less than two vertex");
    }

    bool has_current() const
    {
      if (this->is_in_last())
	return poly.is_closed() ? true : false;

      return Dlink::Iterator::has_current();
    }

    Segment get_current_segment() const 
    {
      if (not poly.is_closed() and this->is_in_last())
	throw std::domain_error("Segment iterator is in the last point and "
				"it is not closed");
      
      Vertex * src = Vertex::dlink_to_vertex(this->get_current());

          // decidir punto tgt según que se esté en el último punto o no
      const Vertex & tgt = 
	this->is_in_last() ? poly.get_first_vertex() : src->next_vertex();
      
      return Segment(*src, tgt);
    }
  };

  bool vertex_belong_polygon(const Vertex & v) const
  {
    for (Vertex_Iterator it(*this); it.has_current(); it.next())
      if (&it.get_current_vertex() == &v)
	return true;
    
    return false;
  }
  
  const Vertex & get_first_vertex() const
  {
    if (vertex_list.is_empty())
      throw std::domain_error("Polygon has not any vertex");

    return * Vertex::dlink_to_vertex(vertex_list.get_next());
  }

  const Vertex & get_last_vertex() const
  {
    if (vertex_list.is_empty())
      throw std::domain_error("Polygon has not any vertex");

    return * Vertex::dlink_to_vertex(vertex_list.get_prev());
  }

  const Vertex & get_next_vertex(const Vertex & v) const
  {
    // TODO: ¿Validar si v pertenece al polígono?
    
    return (v.get_next() == &vertex_list) ?
      Vertex::dlink_to_vertex(&vertex_list)->next_vertex() :
      v.next_vertex();
  }

  const Vertex & get_prev_vertex(const Vertex & v) const
  {
    // TODO: ¿Validar si v pertenece al polígono?
    
    return (v.get_prev() == &vertex_list) ?
      Vertex::dlink_to_vertex(&vertex_list)->prev_vertex() :
      v.prev_vertex();
  }

  Segment get_first_segment() 
  {
    if (vertex_list.is_unitarian_or_empty())
      throw std::domain_error("polygon has less than two vertex");

    const Vertex & first_vertex = get_first_vertex();

    return Segment(first_vertex, first_vertex.next_vertex());
  }

  Segment get_last_segment() 
  {
    if (vertex_list.is_unitarian_or_empty())
      throw std::domain_error("polygon has less than two vertex");

    const Vertex & last_vertex = get_last_vertex();

    return Segment(last_vertex.prev_vertex(), last_vertex);
  }

      // retorna true si sg intersecta con algún lado del polígono
  bool intersects_with(const Segment & sg)
  {
        // recorrer todos los lados y verificar que no haya intersección
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

        // verificar si el nuevo punto es colineal con el último segmento
    if (num_vertex > 1)
      {
	Segment last_sg = get_last_segment();
	if (point.is_colinear_with(last_sg))
	  {
	    if (point.is_inside(last_sg))
	      throw std::domain_error("new vertex is inside of last polygon's "
				      "segment");

	        // punto colineal ==> lo substituimos el último por este nuevo
	    Vertex & last_vertex = const_cast<Vertex&>(get_last_vertex());

	    Point & last_point = last_vertex;

	    last_point = point;

	    update_extreme_points(point);

	    return;
	  }
      }

    if (num_vertex >= 3) // si hay segmentos == debemos verificar que el
			 // nuevo punto no cause un cruze
      {
	    // nuevo lado que conformaría el nuevo punto
	const Segment new_side(get_last_vertex(), point);

	    // verificar si hay intersección con los demás lados excepto el
	    // penúltimo (con el cual comparte un punto)
	for (Segment_Iterator it(*this); true; it.next())
	  {
	    const Segment curr_side = it.get_current_segment();

	    if (curr_side == get_last_segment())
	      break;
	
	    if (curr_side.intersects_with(new_side))
	      throw std::domain_error("new side intersects");
	  }
      }

        // insertar nuevo vértice
    vertex_list.append(new Vertex(point));

    update_extreme_points(point);

    ++num_vertex;
  }

  void add_vertex(const Geom_Number & x, const Geom_Number & y)
  {
    add_vertex(Point(x, y));
  }
  
  void remove_vertex(const Vertex & v)
  {
    if (not vertex_belong_polygon(v))
      throw std::domain_error("Vertex does not belong to polygon");

    // TODO: Resolver casos de colinealidad
    
    const_cast<Vertex&>(v).del();
    --num_vertex;
    delete &v;
  }

  void close()
  {
    if (__is_closed)
      throw std::domain_error("Polygon is already closed");

    if (num_vertex >= 4)
      {
            // verificar si hay intersección con los demás lados excepto el
            // primero y el penúltimo (con el cual se comparten los puntos
            // extremos)

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

  bool contains_to(const Point & p)
  {
    if (not __is_closed)
      throw std::domain_error("Polygon is not closed");

    Segment_Iterator it(*this);

    bool test = p.is_to_left_from(it.get_current_segment());

    it.next();
    
    for ( /* nothing */ ; it.has_current(); it.next())
      if (p.is_to_left_from(it.get_current_segment()) != test)
	return false;

    return true;
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

  double beta; // ángulo entre vértices

public:

  Regular_Polygon() : side_size(0), num_vertex(0) { /* empty */  }

  Regular_Polygon(const Point &  c, 
		  const double & side_sz, 
		  const size_t & n,
		  const double & ang = 0)
    : center(c), side_size(side_sz), num_vertex(n), angle(ang), beta(2*PI/n)
  {
    if (n < 3)
      throw std::domain_error("Polygon sides is less than 3");

        // calcula radio usando ley de los senos con los angulos entre los
        // vértices (que son iguales) y el ángulo entre el lado y el radio
    const double alpha = (PI - beta)/2; // ángulo entre radio y lado
    
    r = side_size*sin(alpha)/sin(beta);
  }

  const double & get_side_size() const { return side_size; }

  const Point & get_center() const { return center;}

  const size_t & size() const { return num_vertex; }

  const double & radius() const { return r; }

  bool is_closed() const { return true; }

  /*
    retorna el i-ésimo punto del polígono. Punto 0 corresponde al centro
    - P(0, r), donde r es el radio.
  */
  const Point get_vertex(const size_t & i) const
  {
    if (i >= num_vertex)
      {
	std::string msg = "vertex " + to_string(i) + " is greater than " +
	  to_string(num_vertex);
	throw std::out_of_range(msg);
      }

        // primer segmento es la vertical negativa desde el
        // centro. Punto inicial es es punto destino de este segmento
        // con origen en el centro 
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

      // Este iterador se realiza para posibilitar código genérico que use
      // polígonos irregulares
  class Vertex_Iterator
  {
    const Regular_Polygon & poly;

    size_t curr;

        // para proporcionar compatibilidad con Polygon::Vertex_Iterator
    Vertex vertex; 

  public:

    Vertex_Iterator(const Regular_Polygon & __poly)
      : poly(__poly), curr(0)
    {
      // empty 
    }

    bool has_current() const { return curr < poly.size(); }

    Vertex & get_current_vertex() 
    {
      if (not has_current())
	throw std::overflow_error("Iterator has not current");

      vertex = poly.get_vertex(curr);

      return vertex;
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

      // Este iterador se realiza para posibilitar código genérico que use
      // polígonos irregulares
  class Segment_Iterator
  {
    const Regular_Polygon & poly;

    size_t curr;

  public:

    Segment_Iterator(const Regular_Polygon & __poly)
      : poly(__poly), curr(0)
    {
      // empty 
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

      // por razones de simplicdad, usamos como puntos extremos los
      // cardinales de la circunferencia
  
  Point lowest_point() const { return center + Point(0, -r); }

  Point highest_point() const { return center + Point(0, r); }

  Point leftmost_point() const { return center + Point(-r, 0); }

  Point rightmost_point() const { return center + Point(r, 0); }
};

// TODO: debe haber error side_size parece ser siempre igual a r

inline void Polygon::copy_regular_polygon(const Regular_Polygon & poly)
{
  assert(num_vertex == 0 and not __is_closed);

  for (size_t i = 0; i < poly.size(); ++i)
    add_vertex(poly.get_vertex(i));

  close();
}

# endif // POLYGON_H
