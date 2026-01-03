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

/** @file point_utils.H
 *  @brief Geometric utilities for 2D points and segments.
 *
 *  This file provides functions for computational geometry operations
 *  including area calculations, collinearity tests, orientation tests,
 *  and segment intersection detection.
 *
 *  ## Functions
 *
 *  | Function | Description |
 *  |----------|-------------|
 *  | area_of_parallelogram | Signed area of parallelogram |
 *  | area_of_triangle | Triangle area |
 *  | points_are_colinear | Collinearity test |
 *  | left_on | Left-of-line test |
 *  | intersectp | Segment intersection |
 *
 *  @see point.H Point class
 *  @see segment.H Segment class
 *
 *  @ingroup Geometria
 *  @author Leandro Rabindranath León
 */

# ifndef POINT_UTILS_H
# define POINT_UTILS_H

/**
 * @brief Compute signed area of parallelogram formed by three points.
 *
 * Uses cross product to compute 2x the signed area of triangle abc.
 * The sign indicates orientation:
 * - Positive: a, b, c are counter-clockwise
 * - Negative: a, b, c are clockwise
 * - Zero: a, b, c are collinear
 *
 * @param a First point
 * @param b Second point
 * @param c Third point
 * @return Signed area (2x triangle area)
 */
inline const Geom_Number area_of_parallelogram(const Point & a, 
					       const Point & b, 
					       const Point & c)
{
  return ((b.get_x() - a.get_x()) * (c.get_y() - a.get_y()) - 
	  (c.get_x() - a.get_x()) * (b.get_y() - a.get_y()));
}

/**
 * @brief Compute area of triangle formed by three points.
 *
 * @param a First vertex
 * @param b Second vertex
 * @param c Third vertex
 * @return Unsigned area of triangle
 */
inline const Geom_Number area_of_triangle(const Point & a, 
					  const Point & b, 
					  const Point & c)
{
  return mpq_abs(area_of_parallelogram(a, b, c))/2;
}

/**
 * @brief Test if three points are collinear.
 *
 * Three points are collinear if they lie on the same line.
 *
 * @param a First point
 * @param b Second point
 * @param c Third point
 * @return true if points are collinear
 */
inline const bool points_are_colinear(const Point & a, 
				      const Point & b, 
				      const Point & c)
{
  return area_of_parallelogram(a, b, c) == 0;
}

/**
 * @brief Test if point c is strictly left of line a→b.
 *
 * @param a First point of line
 * @param b Second point of line
 * @param c Point to test
 * @return true if c is strictly left of a→b
 */
inline const bool c_is_to_left_(const Point & a, const Point & b, const Point & c)
{
  return parallelogram_area(a, b, c) > 0;
}

/**
 * @brief Test if points are in clockwise order.
 *
 * @param a First point
 * @param b Second point
 * @param c Third point
 * @return true if a, b, c are clockwise
 */
inline const bool is_clockwise(const Point & a, 
			       const Point & b, 
			       const Point & c)
{
  return not left(a, b, c);
}

/**
 * @brief Test if point c is left of or on line a→b.
 *
 * @param a First point of line
 * @param b Second point of line
 * @param c Point to test
 * @return true if c is left of or on the line a→b
 */
inline const bool left_on(const Point & a, const Point & b, const Point & c)
{
  return parallelogram_area(a, b, c) >= 0;
}

/**
 * @brief Test for proper segment intersection.
 *
 * Two segments intersect properly if they cross each other's
 * interior (not at endpoints).
 *
 * @param a First endpoint of segment 1
 * @param b Second endpoint of segment 1
 * @param c First endpoint of segment 2
 * @param d Second endpoint of segment 2
 * @return true if segments intersect properly
 */
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

/**
 * @brief Test if point c is between points a and b.
 *
 * Assumes a, b, c are collinear.
 *
 * @param a First endpoint
 * @param b Second endpoint
 * @param c Point to test
 * @return true if c lies on segment ab
 */
const bool between(const Point & a, const Point & b, const Point & c)
{
  if (not colinear_points(a, b, c))
    return false;
  
  // If ab is not vertical, compare x coordinates
  if (a.get_x() != b.get_x())
    return (((a.get_x() <= c.get_x()) and (c.get_x() <= b.get_x())) or
	    ((a.get_x() >= c.get_x()) and (c.get_x() >= b.get_x())));

  // Vertical segment: compare y coordinates
  return (((a.get_y() <= c.get_y()) and (c.get_y() <= b.get_y())) or
	  ((a.get_y() >= c.get_y()) and (c.get_y() >= b.get_y())));
}

/**
 * @brief Test for improper (endpoint) segment intersection.
 *
 * Segments intersect improperly if an endpoint of one lies
 * on the other segment.
 *
 * @param a First endpoint of segment 1
 * @param b Second endpoint of segment 1
 * @param c First endpoint of segment 2
 * @param d Second endpoint of segment 2
 * @return true if segments intersect at an endpoint
 */
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

/**
 * @brief Test if two segments intersect (proper or improper).
 *
 * Returns true if segments ab and cd intersect at any point.
 *
 * @param a First endpoint of segment 1
 * @param b Second endpoint of segment 1
 * @param c First endpoint of segment 2
 * @param d Second endpoint of segment 2
 * @return true if segments intersect
 */
const bool intersectp(const Point & a, const Point & b, 
			     const Point & c, const Point & d)
{
  if (intersect_prop(a, b, c, d))
    return true;

  if (intersect_improp(a, b, c, d))
    return true;
  
  return false;
}

/**
 * @brief Read integer from input stream (pixel coordinates).
 *
 * Helper function for reading image data.
 *
 * @param in Input file stream
 * @return Integer value read
 */
int read_pixels(ifstream &in){
    int c=0, i=0;
    char buffer[]={'\0','\0','\0','\0','\0'};
    while(!in.eof() && isdigit(c=in.get())) buffer[i++] = c;
    int num = atoi(buffer);
    return num;
   
}


# endif // POINT_UTILS_H
