

/*
                          Aleph_w

  Data structures & Algorithms
  version 1.9d
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2022 Leandro Rabindranath Leon

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

    bool has_curr() const { return curr < poly.size(); }

    Vertex & get_current_vertex() 
    {
      if (not has_curr())
	throw std::overflow_error("Iterator has not current");

      vertex = poly.get_vertex(curr);

      return vertex;
    }

    void next_ne() noexcept { ++curr; }

    void next()
    {
      if (not has_curr())
	throw std::overflow_error("Iterator has not current");
      next_ne();
    }

    void prev()
    {
      --curr;

      if (not has_curr())
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

    bool has_curr() const { return curr < poly.size(); }

    const Segment get_current_segment() const
    {
      if (not has_curr())
	throw std::overflow_error("Iterator has not current");

      return Segment(poly.get_vertex(curr), 
		     poly.get_vertex((curr + 1) % poly.size()));
    }

    void next_ne() noexcept { ++curr; }

    void next()
    {
      if (not has_curr())
	throw std::overflow_error("Iterator has not current");
      next_ne();
    }

    void prev()
    {
      --curr;

      if (not has_curr())
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
