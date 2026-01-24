
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
 *  @ingroup Geometry
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
