
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
 *  @brief Legacy geometric utilities — use point.H functions directly.
 *
 *  All functions in this file are deprecated. Prefer the canonical
 *  implementations in point.H: area_of_parallelogram(), orientation(),
 *  on_segment(), segments_intersect(), segment_intersection_point().
 *
 *  @deprecated Use point.H functions directly.
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */

# ifndef POINT_UTILS_H
# define POINT_UTILS_H

# include <point.H>

[[deprecated("Use area_of_triangle() from point.H")]]
inline Geom_Number area_of_triangle_legacy(const Point & a,
                                           const Point & b,
                                           const Point & c)
{
  return area_of_triangle(a, b, c);
}

[[deprecated("Use points_are_colinear via Point::is_colinear_with()")]]
inline bool points_are_colinear(const Point & a,
                                const Point & b,
                                const Point & c)
{
  return area_of_parallelogram(a, b, c) == 0;
}

[[deprecated("Use orientation() from point.H")]]
inline bool c_is_to_left_(const Point & a, const Point & b, const Point & c)
{
  return area_of_parallelogram(a, b, c) > 0;
}

[[deprecated("Use orientation() from point.H")]]
inline bool is_clockwise(const Point & a, const Point & b, const Point & c)
{
  return area_of_parallelogram(a, b, c) < 0;
}

[[deprecated("Use orientation() from point.H")]]
inline bool left_on(const Point & a, const Point & b, const Point & c)
{
  return area_of_parallelogram(a, b, c) >= 0;
}

[[deprecated("Use Segment::intersects_properly_with() from point.H")]]
inline bool proper_intersection(const Point & a, const Point & b,
                                const Point & c, const Point & d)
{
  if (points_are_colinear(a, b, c) or
      points_are_colinear(a, b, d) or
      points_are_colinear(c, d, a) or
      points_are_colinear(c, d, b))
    return false;

  return (c_is_to_left_(a, b, c) xor c_is_to_left_(a, b, d)) and
         (c_is_to_left_(c, d, a) xor c_is_to_left_(c, d, b));
}

[[deprecated("Use Point::is_between() from point.H")]]
inline bool between(const Point & a, const Point & b, const Point & c)
{
  if (not points_are_colinear(a, b, c))
    return false;

  if (a.get_x() != b.get_x())
    return (((a.get_x() <= c.get_x()) and (c.get_x() <= b.get_x())) or
            ((a.get_x() >= c.get_x()) and (c.get_x() >= b.get_x())));

  return (((a.get_y() <= c.get_y()) and (c.get_y() <= b.get_y())) or
          ((a.get_y() >= c.get_y()) and (c.get_y() >= b.get_y())));
}

[[deprecated("Use Segment::intersects_with() from point.H")]]
inline bool intersect_improp(const Point & a, const Point & b,
                             const Point & c, const Point & d)
{
  return between(a, b, c) or between(a, b, d) or
         between(c, d, a) or between(c, d, b);
}

[[deprecated("Use segments_intersect() from point.H")]]
inline bool intersectp(const Point & a, const Point & b,
                       const Point & c, const Point & d)
{
  if (proper_intersection(a, b, c, d))
    return true;

  return intersect_improp(a, b, c, d);
}


# endif // POINT_UTILS_H
