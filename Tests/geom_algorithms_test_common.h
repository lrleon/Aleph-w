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

#ifndef ALEPH_TEST_GEOM_ALGORITHMS_TEST_COMMON_H
#define ALEPH_TEST_GEOM_ALGORITHMS_TEST_COMMON_H

#include <gtest/gtest.h>
#include <stdexcept>
#include <geom_algorithms.H>
#include "geometry_visual_golden.h"

namespace
{
  /**
   * @brief Checks if a polygon contains a specific vertex.
   *
   * @param poly The polygon to check.
   * @param p The vertex to look for.
   * @return true if the polygon contains the vertex, false otherwise.
   */
  [[maybe_unused]] bool polygon_contains_vertex(const Polygon & poly, const Point & p)
  {
    if (poly.size() == 0)
      return false;

    for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      if (it.get_current_vertex() == p)
        return true;

    return false;
  }

  /**
   * @brief Checks if two pairs of points match regardless of their order.
   *
   * @param a First point of the first pair.
   * @param b Second point of the first pair.
   * @param u First point of the second pair.
   * @param v Second point of the second pair.
   * @return true if {a, b} is equal to {u, v} or {v, u}, false otherwise.
   */
  [[maybe_unused]] bool matches_unordered_pair(const Point & a, const Point & b,
                                               const Point & u, const Point & v)
  {
    return (a == u and b == v) or (a == v and b == u);
  }

  /**
   * @brief Represents a triangle by the indices of its three vertices.
   */
  struct TriKey
  {
    size_t a; ///< Index of the first vertex.
    size_t b; ///< Index of the second vertex.
    size_t c; ///< Index of the third vertex.
  };

  /**
   * Sort three indices into ascending order in place.
   *
   * This function rearranges the values of a, b, and c so that a <= b <= c.
   *
   * @param a First index; will hold the smallest of the three on return.
   * @param b Second index; will hold the middle value on return.
   * @param c Third index; will hold the largest of the three on return.
   */
  void sort3(size_t & a, size_t & b, size_t & c)
  {
    if (a > b)
      {
        const size_t t = a;
        a = b;
        b = t;
      }
    if (b > c)
      {
        const size_t t = b;
        b = c;
        c = t;
      }
    if (a > b)
      {
        const size_t t = a;
        a = b;
        b = t;
      }
  }

  /**
   * @brief Converts triangulation results into a sorted array of canonical triangles.
   *
   * Each triangle's indices are sorted to ensure a unique representation.
   * The resulting array is then sorted to facilitate comparisons between triangulations.
   *
   * @param r The Delaunay triangulation result.
   * @return Array<TriKey> A sorted array of TriKey structures.
   */
  [[maybe_unused]] Array<TriKey>
  canonical_triangles(const DelaunayTriangulationBowyerWatson::Result & r)
  {
    Array<TriKey> out;
    out.reserve(r.triangles.size());
    for (size_t i = 0; i < r.triangles.size(); ++i)
      {
        size_t a = r.triangles(i).i;
        size_t b = r.triangles(i).j;
        size_t c = r.triangles(i).k;
        sort3(a, b, c);
        out.append(TriKey{a, b, c});
      }

    quicksort_op(out, [](const TriKey & x, const TriKey & y)
                   {
                     if (x.a != y.a)
                       return x.a < y.a;
                     if (x.b != y.b)
                       return x.b < y.b;
                     return x.c < y.c;
                   });
    return out;
  }

  /**
   * Compute the circumcenter of a triangle given its three vertices.
   *
   * @param a First vertex of the triangle.
   * @param b Second vertex of the triangle.
   * @param c Third vertex of the triangle.
   * @returns Point Coordinates of the circumcenter of the triangle.
   * @throws std::domain_error if the three points are collinear (triangle has zero area).
   */
  [[maybe_unused]] Point circumcenter_of(const Point & a, const Point & b, const Point & c)
  {
    const Geom_Number & ax = a.get_x();
    const Geom_Number & ay = a.get_y();
    const Geom_Number & bx = b.get_x();
    const Geom_Number & by = b.get_y();
    const Geom_Number & cx = c.get_x();
    const Geom_Number & cy = c.get_y();

    const Geom_Number a2 = ax * ax + ay * ay;
    const Geom_Number b2 = bx * bx + by * by;
    const Geom_Number c2 = cx * cx + cy * cy;

    const Geom_Number d = ax * (by - cy) + bx * (cy - ay) + cx * (ay - by);
    const Geom_Number den = d + d;

    if (den == 0)
      throw std::domain_error("circumcenter_of: collinear triangle (zero area)");

    return {
          (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / den,
          (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / den
        };
  }

  /**
   * Compute the polygon's signed area.
   *
   * @param poly Polygon whose signed area is computed.
   * @return Signed area: positive when vertices are in counter-clockwise order, negative when in clockwise order.
   */
  Geom_Number polygon_signed_area(const Polygon & poly)
  {
    return GeomPolygonUtils::signed_double_area(poly) / 2;
  }

  /**
   * @brief Computes the absolute area of a polygon.
   *
   * @param poly The polygon.
   * @return Geom_Number The non-negative area.
   */
  [[maybe_unused]] Geom_Number polygon_area(const Polygon & poly)
  {
    Geom_Number a = polygon_signed_area(poly);
    if (a < 0) { a = -a; }
    return a;
  }

  /**
   * @brief Checks if a polygon's vertices are in counter-clockwise order.
   *
   * @param poly The polygon.
   * @return true if the order is CCW, false otherwise.
   */
  [[maybe_unused]] bool is_ccw(const Polygon & poly)
  {
    return GeomPolygonUtils::signed_double_area(poly) > 0;
  }

  /**
   * @brief Computes the absolute area of a triangle.
   *
   * @param t The triangle.
   * @return Geom_Number The non-negative area.
   */
  [[maybe_unused]] Geom_Number triangle_area(const Triangle & t)
  {
    Geom_Number a = t.area();
    if (a < 0) { a = -a; }
    return a;
  }

  /**
   * @brief Verifies if all points in a list are located inside or on the boundary of a hull.
   *
   * @param points The list of points to check.
   * @param hull The polygon representing the hull.
   * @return true if all points are inside or on the hull, false if any point is outside.
   */
  [[maybe_unused]] bool all_points_inside_or_on(const DynList<Point> & points, const Polygon & hull)
  {
    for (auto it = points.get_it(); it.has_curr(); it.next_ne())
      if (auto loc = PointInPolygonWinding::locate(hull, it.get_curr());
        loc == PointInPolygonWinding::Location::Outside)
        return false;
    return true;
  }

  /**
   * Determine whether the polygon's vertex ordering defines a convex polygon.
   *
   * @param poly Polygon whose convexity is to be tested.
   * @returns `true` if the polygon is convex, `false` otherwise.
   */
  [[maybe_unused]] bool polygon_is_convex(const Polygon & poly)
  {
    Array<Point> verts;
    verts.reserve(poly.size());
    for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      verts.append(it.get_current_vertex());
    return GeomPolygonUtils::is_convex(verts);
  }
} // namespace

class GeomAlgorithmsTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Basic triangulation tests

#endif // ALEPH_TEST_GEOM_ALGORITHMS_TEST_COMMON_H