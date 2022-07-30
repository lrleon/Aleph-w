
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
# ifndef SEGMENT_H
# define SEGMENT_H

# include "point.H"
//# include "points_utils.H"

class Segment
{

  const Point * a;
  const Point * b;

public:

  Segment() : a(nullptr), b(nullptr)
  { 
    /* empty */  
  }
  
  Segment(const Point * __a, const Point * __b) : a(__a), b(__b) 
  { 
    /* empty */ 
  }

  Segment(const Point & __a, const Point & __b) : a(&__a), b(&__b) 
  { 
    /* empty */ 
  }
  
  ~Segment()
  {
    /* empty */
  }
  
  const bool operator == (const Segment & segment) const 
  {
    return ((*a == *segment.a and *b == *segment.b) or
	    (*a == *segment.b and *b == *segment.a));
  }
  
  /* Verifica si el segmento intersecta otro segmento  */
  const bool intersect(const Segment & segment) const
  {
    return intersectp(*a, *b, *segment.a, *segment.b);
  }

  const Point * get_a() { return a ;} //Retorna el punto 'a' del segmento

  const Point * get_b() { return b; } //Retorna el punto 'b' del segmento


  /* Recibe la coordenada 'y' de una linea horizontal y devuelve */
  /*     el punto de interseccion con el segmento                */
  Point  horiz_line_inter(const Geom_Number & __y)
  {
    
    Geom_Number intersection_x;
    float m;

    m = (float)(a->get_y() - b->get_y()) / (float)(a->get_x() - b->get_x());

    intersection_x = (Geom_Number)((__y - b->get_y())/m + b->get_x());
    
    Point  intersection_point(intersection_x,__y);

    return intersection_point;
  }

  /* Retorna el punto mas alto entre a y b  */  
  const Point * highest_point()
  {

    if (a->get_y() > b->get_y())
      return a;
    else
      return b;
  }

  /* Retorna el punto mas bajo entre a y b  */  
  const Point * lowest_point()
  {
    if (a->get_y() < b->get_y())
      return a;
    else
      return b;
  }
};

# endif // SEGMENT_H
