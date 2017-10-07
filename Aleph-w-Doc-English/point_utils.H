
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
  @file point_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
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


/** 
  @brief Calculate the area of the triangle formed by points a, b y c 
*/
inline const Geom_Number area_of_triangle(const Point & a, 
					  const Point & b, 
					  const Point & c)
{
  return mpq_abs(area_of_parallelogram(a, b, c))/2;
}


/** 
  @brief Check if the three points are collinear.  
*/
inline const bool points_are_colinear(const Point & a, 
				      const Point & b, 
				      const Point & c)
{
  return area_of_parallelogram(a, b, c) == 0;
}


/** 
  @brief Returns true if c is to the left of the segment a--b 
*/
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


/** 
  @brief Returns true if c is to the left of the segment a - b
  or the three points are collinear. 
*/
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
  
      /* If ab is not vertical*/
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
