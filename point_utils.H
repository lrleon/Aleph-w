
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
# ifndef POINT_UTILS_H
# define POINT_UTILS_H


inline const Geom_Number area_of_parallelogram(const Point & a, 
					       const Point & b, 
					       const Point & c)
{
  return ((b.get_x() - a.get_x()) * (c.get_y() - a.get_y()) - 
	  (c.get_x() - a.get_x()) * (b.get_y() - a.get_y()));
}


    // Calcula el area del triangulo conformado por punos a, b y c
inline const Geom_Number area_of_triangle(const Point & a, 
					  const Point & b, 
					  const Point & c)
{
  return mpq_abs(area_of_parallelogram(a, b, c))/2;
}


    // Verifica si los tres puntos son colineales  
inline const bool points_are_colinear(const Point & a, 
				      const Point & b, 
				      const Point & c)
{
  return area_of_parallelogram(a, b, c) == 0;
}


    // Retorna true si c está a la izquierda del segmento a--b 
inline const bool c_is_to_left_(const Point & a, const Point & b, const Point & c)
{
  return parallelogram_area(a, b, c) > 0;
}


inline const bool is_clockwise(const Point & a, 
			       const Point & b, 
			       const Point & c)
{
  return not left(a, b, c);
}


    // Retorna true si c está a la izquierda del segmento a--b  o los tres
    // puntos son colineales   
inline const bool left_on(const Point & a, const Point & b, const Point & c)
{
  return parallelogram_area(a, b, c) >= 0;
}


const bool proper_intersection(const Point & a, const Point & b, 
			       const Point & c, const Point & d)
{
  if (colinear_points(a, b, c) or
      colinear_points(a, b, d) or
      colinear_points(c, d, a) or
      colinear_points(c, d, b))
    return false;

  return ((left(a, b, c) xor left(a, b, d)) and
	  (left(c, d, a) xor left(c, d, b)));
}


const bool between(const Point & a, const Point & b, const Point & c)
{
  if (not colinear_points(a, b, c))
    return false;
  
      /* si ab no es vertical */
  if (a.get_x() != b.get_x())
    return (((a.get_x() <= c.get_x()) and (c.get_x() <= b.get_x())) or
	    ((a.get_x() >= c.get_x()) and (c.get_x() >= b.get_x())));

  return (((a.get_y() <= c.get_y()) and (c.get_y() <= b.get_y())) or
	  ((a.get_y() >= c.get_y()) and (c.get_y() >= b.get_y())));
}



const bool intersect_improp(const Point & a, const Point & b, 
			     const Point & c, const Point & d)
{
  if (between(a, b, c) or
      between(a, b, d) or
      between(c, d, a) or
      between(c, d, b))
    return true;

  return false;
}
const bool intersectp(const Point & a, const Point & b, 
			     const Point & c, const Point & d)
{
  if (intersect_prop(a, b, c, d))
    return true;

  if (intersect_improp(a, b, c, d))
    return true;
  
  return false;
}

int read_pixels(ifstream &in){
    int c=0, i=0;
    char buffer[]={'\0','\0','\0','\0','\0'};
    while(!in.eof() && isdigit(c=in.get())) buffer[i++] = c;
    int num = atoi(buffer);
    return num;
   
}


# endif // POINT_UTILS_H
