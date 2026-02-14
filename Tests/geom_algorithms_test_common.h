#ifndef ALEPH_TEST_GEOM_ALGORITHMS_TEST_COMMON_H
#define ALEPH_TEST_GEOM_ALGORITHMS_TEST_COMMON_H

#include <gtest/gtest.h>
#include <geom_algorithms.H>
#include "geometry_visual_golden.h"

namespace
{

bool polygon_contains_vertex(const Polygon & poly, const Point & p)
{
  if (poly.size() == 0)
    return false;

  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    if (it.get_current_vertex() == p)
      return true;

  return false;
}

bool matches_unordered_pair(const Point & a, const Point & b,
                            const Point & u, const Point & v)
{
  return (a == u and b == v) or (a == v and b == u);
}

struct TriKey
{
  size_t a;
  size_t b;
  size_t c;
};

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

Array<TriKey>
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

Point circumcenter_of(const Point & a, const Point & b, const Point & c)
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

  return {
      (a2 * (by - cy) + b2 * (cy - ay) + c2 * (ay - by)) / den,
      (a2 * (cx - bx) + b2 * (ax - cx) + c2 * (bx - ax)) / den
  };
}

Geom_Number polygon_signed_area(const Polygon & poly)
{
  return GeomPolygonUtils::signed_double_area(poly) / 2;
}

Geom_Number polygon_area(const Polygon & poly)
{
  Geom_Number a = polygon_signed_area(poly);
  if (a < 0) { a = -a; }
  return a;
}

bool is_ccw(const Polygon & poly)
{
  return GeomPolygonUtils::signed_double_area(poly) > 0;
}

Geom_Number triangle_area(const Triangle & t)
{
  Geom_Number a = t.area();
  if (a < 0) { a = -a; }
  return a;
}

bool all_points_inside_or_on(const DynList<Point> & points, const Polygon & hull)
{
  for (auto it = points.get_it(); it.has_curr(); it.next_ne())
    {
      auto loc = PointInPolygonWinding::locate(hull, it.get_curr());
      if (loc == PointInPolygonWinding::Location::Outside)
        return false;
    }
  return true;
}

bool polygon_is_convex(const Polygon & poly)
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
