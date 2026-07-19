
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

namespace Aleph
{

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
  const Orientation abc = orientation(a, b, c);
  const Orientation abd = orientation(a, b, d);
  const Orientation cda = orientation(c, d, a);
  const Orientation cdb = orientation(c, d, b);
  if (abc == Orientation::COLLINEAR or abd == Orientation::COLLINEAR or
      cda == Orientation::COLLINEAR or cdb == Orientation::COLLINEAR)
    return false;

  return abc != abd and cda != cdb;
}

[[deprecated("Use Point::is_between() from point.H")]]
inline bool between(const Point & a, const Point & b, const Point & c)
{
  return c.is_between(a, b);
}

[[deprecated("Use Segment::intersects_with() from point.H")]]
inline bool intersect_improp(const Point & a, const Point & b,
                             const Point & c, const Point & d)
{
  return c.is_between(a, b) or d.is_between(a, b) or
         a.is_between(c, d) or b.is_between(c, d);
}

[[deprecated("Use segments_intersect() from point.H")]]
inline bool intersectp(const Point & a, const Point & b,
                       const Point & c, const Point & d)
{
  return segments_intersect(a, b, c, d);
}

} // namespace Aleph

// Preserve source compatibility for legacy unqualified calls while keeping
// every implementation in the library namespace.
using Aleph::area_of_triangle_legacy;
using Aleph::between;
using Aleph::c_is_to_left_;
using Aleph::intersect_improp;
using Aleph::intersectp;
using Aleph::is_clockwise;
using Aleph::left_on;
using Aleph::points_are_colinear;
using Aleph::proper_intersection;


# endif // POINT_UTILS_H
