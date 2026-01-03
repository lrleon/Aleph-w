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

/** @file segment.H
 *  @brief 2D line segment representation and operations.
 *
 *  This file provides the Segment class for representing and manipulating
 *  line segments in 2D space. A segment is defined by two endpoint Point
 *  objects.
 *
 *  ## Features
 *
 *  - Segment-segment intersection testing
 *  - Horizontal line intersection computation
 *  - Endpoint access and comparison
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  Point p1(0, 0), p2(10, 10);
 *  Point p3(0, 10), p4(10, 0);
 *
 *  Segment s1(&p1, &p2);  // Diagonal from (0,0) to (10,10)
 *  Segment s2(&p3, &p4);  // Diagonal from (0,10) to (10,0)
 *
 *  if (s1.intersect(s2))
 *    std::cout << "Segments cross at (5,5)\n";
 *  ```
 *
 *  @see point.H Point class definition
 *  @see line.H Infinite line representation
 *  @see polygon.H Polygon operations
 *
 *  @ingroup Geometria
 *  @author Leandro Rabindranath León
 */

# ifndef SEGMENT_H
# define SEGMENT_H

# include "point.H"

/**
 * @brief 2D line segment defined by two endpoints.
 *
 * Represents a finite line segment in 2D Cartesian space. The segment
 * is defined by pointers to two Point objects (the endpoints).
 *
 * @note This class stores pointers to Points, not copies. The Points
 *       must remain valid for the lifetime of the Segment.
 *
 * @ingroup Geometria
 */
class Segment
{

  const Point * a;  ///< First endpoint of the segment
  const Point * b;  ///< Second endpoint of the segment

public:

  /**
   * @brief Default constructor creates an uninitialized segment.
   *
   * Both endpoints are set to nullptr. Must be assigned before use.
   */
  Segment() : a(nullptr), b(nullptr)
  { 
    /* empty */  
  }
  
  /**
   * @brief Construct segment from two point pointers.
   *
   * @param __a Pointer to first endpoint
   * @param __b Pointer to second endpoint
   */
  Segment(const Point * __a, const Point * __b) : a(__a), b(__b) 
  { 
    /* empty */ 
  }

  /**
   * @brief Construct segment from two point references.
   *
   * @param __a Reference to first endpoint (address taken)
   * @param __b Reference to second endpoint (address taken)
   *
   * @warning Points must outlive the Segment object.
   */
  Segment(const Point & __a, const Point & __b) : a(&__a), b(&__b) 
  { 
    /* empty */ 
  }
  
  /// Destructor (no-op, doesn't own the Points)
  ~Segment()
  {
    /* empty */
  }
  
  /**
   * @brief Test equality of two segments.
   *
   * Two segments are equal if they have the same endpoints
   * (regardless of order: {a,b} == {b,a}).
   *
   * @param segment Segment to compare with
   * @return true if segments have the same endpoints
   */
  const bool operator == (const Segment & segment) const 
  {
    return ((*a == *segment.a and *b == *segment.b) or
	    (*a == *segment.b and *b == *segment.a));
  }
  
  /**
   * @brief Test if this segment intersects another segment.
   *
   * Uses the cross-product method to determine if the segments
   * properly intersect (endpoints straddling each other's line).
   *
   * @param segment The other segment to test against
   * @return true if the segments intersect
   *
   * @see intersectp() in point.H for the underlying algorithm
   */
  const bool intersect(const Segment & segment) const
  {
    return intersectp(*a, *b, *segment.a, *segment.b);
  }

  /**
   * @brief Get the first endpoint.
   * @return Pointer to point a
   */
  const Point * get_a() { return a ;}

  /**
   * @brief Get the second endpoint.
   * @return Pointer to point b
   */
  const Point * get_b() { return b; }


  /**
   * @brief Compute intersection with a horizontal line.
   *
   * Given a horizontal line at y-coordinate __y, computes the
   * point where this segment intersects that line.
   *
   * @param __y The y-coordinate of the horizontal line
   * @return The intersection point
   *
   * @pre The segment must not be horizontal (would cause division by zero)
   * @pre The horizontal line must actually intersect the segment
   */
  Point horiz_line_inter(const Geom_Number & __y)
  {
    
    Geom_Number intersection_x;
    float m;

    m = (float)(a->get_y() - b->get_y()) / (float)(a->get_x() - b->get_x());

    intersection_x = (Geom_Number)((__y - b->get_y())/m + b->get_x());
    
    Point  intersection_point(intersection_x,__y);

    return intersection_point;
  }

  /**
   * @brief Get the endpoint with the larger y-coordinate.
   *
   * @return Pointer to the higher endpoint
   */
  const Point * highest_point()
  {

    if (a->get_y() > b->get_y())
      return a;
    else
      return b;
  }

  /**
   * @brief Get the endpoint with the smaller y-coordinate.
   *
   * @return Pointer to the lower endpoint
   */
  const Point * lowest_point()
  {
    if (a->get_y() < b->get_y())
      return a;
    else
      return b;
  }
};

# endif // SEGMENT_H
