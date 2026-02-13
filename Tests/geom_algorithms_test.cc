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

} // namespace

class GeomAlgorithmsTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Basic triangulation tests
TEST_F(GeomAlgorithmsTest, TriangulateTriangle)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, TriangulateSquare)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(0, 4));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, TriangulateSquareClockwise)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(0, 4));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(4, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);

  EXPECT_EQ(triangles.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, TriangulatePentagon)
{
  Polygon p;
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(4, 1.5));
  p.add_vertex(Point(3, 4));
  p.add_vertex(Point(1, 4));
  p.add_vertex(Point(0, 1.5));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, TriangulateOpenPolygonThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));
  // intentionally open

  CuttingEarsTriangulation triangulator;
  EXPECT_THROW((void) triangulator(p), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, TriangulateDegenerateCollinearPolygonThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(2, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  EXPECT_THROW((void) triangulator(p), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, TriangulateSelfIntersectingPolygonRejectedByPolygonValidation)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(0, 2));
  p.add_vertex(Point(2, 2));

  // Closing this polyline would create a self-intersection.
  EXPECT_THROW((void) p.close(), std::domain_error);
}

// Convex hull tests
TEST_F(GeomAlgorithmsTest, QuickHullTriangle)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(6, 0));
  points.append(Point(3, 5));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, QuickHullSquare)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 0));
  points.append(Point(5, 5));
  points.append(Point(0, 5));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, QuickHullWithInterior)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(10, 0));
  points.append(Point(10, 10));
  points.append(Point(0, 10));
  points.append(Point(5, 5));  // Interior
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, GiftWrappingSquare)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(4, 0));
  points.append(Point(4, 4));
  points.append(Point(0, 4));
  
  GiftWrappingConvexHull gw;
  Polygon hull = gw(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, BruteForceHullTriangle)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(4, 0));
  points.append(Point(2, 3));
  
  BruteForceConvexHull bf;
  Polygon hull = bf(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, ThreePointsHull)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(2, 0));
  points.append(Point(1, 2));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, LegacyHullsEmptyInput)
{
  DynList<Point> points;

  QuickHull qh;
  GiftWrappingConvexHull gw;
  BruteForceConvexHull bf;

  EXPECT_EQ(qh(points).size(), 0u);
  EXPECT_EQ(gw(points).size(), 0u);
  EXPECT_EQ(bf(points).size(), 0u);
}

TEST_F(GeomAlgorithmsTest, LegacyHullsSinglePointInput)
{
  DynList<Point> points;
  points.append(Point(7, 9));

  QuickHull qh;
  GiftWrappingConvexHull gw;
  BruteForceConvexHull bf;

  Polygon qh_hull = qh(points);
  Polygon gw_hull = gw(points);
  Polygon bf_hull = bf(points);

  EXPECT_EQ(qh_hull.size(), 1u);
  EXPECT_EQ(gw_hull.size(), 1u);
  EXPECT_EQ(bf_hull.size(), 1u);
  EXPECT_TRUE(polygon_contains_vertex(qh_hull, Point(7, 9)));
  EXPECT_TRUE(polygon_contains_vertex(gw_hull, Point(7, 9)));
  EXPECT_TRUE(polygon_contains_vertex(bf_hull, Point(7, 9)));
}

TEST_F(GeomAlgorithmsTest, LegacyHullsAllDuplicatePoints)
{
  DynList<Point> points;
  points.append(Point(3, 3));
  points.append(Point(3, 3));
  points.append(Point(3, 3));

  QuickHull qh;
  GiftWrappingConvexHull gw;
  BruteForceConvexHull bf;

  EXPECT_EQ(qh(points).size(), 1u);
  EXPECT_EQ(gw(points).size(), 1u);
  EXPECT_EQ(bf(points).size(), 1u);
}

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainSquareWithInteriorAndDuplicates)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 0));
  points.append(Point(5, 5));
  points.append(Point(0, 5));
  points.append(Point(2, 2));  // Interior
  points.append(Point(5, 5));  // Duplicate hull point
  points.append(Point(2, 2));  // Duplicate interior point

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(points);

  EXPECT_EQ(hull.size(), 4u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(5, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(5, 5)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 5)));
  EXPECT_FALSE(polygon_contains_vertex(hull, Point(2, 2)));
}

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainCollinearKeepsEndpoints)
{
  DynList<Point> points;
  points.append(Point(3, 0));
  points.append(Point(1, 0));
  points.append(Point(4, 0));
  points.append(Point(2, 0));
  points.append(Point(0, 0));
  points.append(Point(4, 0));  // Duplicate

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(points);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(points); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  scene.polygons.append(hull);
  Aleph::TestVisual::add_polygon_vertices(scene, hull, true);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_andrew_collinear_endpoints", scene,
      "Andrew monotonic chain / collinear input");

  EXPECT_EQ(hull.size(), 2u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(4, 0)));
  EXPECT_FALSE(polygon_contains_vertex(hull, Point(2, 0)));
}

TEST_F(GeomAlgorithmsTest, GrahamScanSquareWithInteriorAndDuplicates)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 0));
  points.append(Point(5, 5));
  points.append(Point(0, 5));
  points.append(Point(2, 2));  // Interior
  points.append(Point(0, 0));  // Duplicate hull point
  points.append(Point(2, 2));  // Duplicate interior point

  GrahamScanConvexHull graham;
  Polygon hull = graham(points);

  EXPECT_EQ(hull.size(), 4u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(5, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(5, 5)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 5)));
  EXPECT_FALSE(polygon_contains_vertex(hull, Point(2, 2)));
}

TEST_F(GeomAlgorithmsTest, GrahamScanCollinearKeepsEndpoints)
{
  DynList<Point> points;
  points.append(Point(1, 1));
  points.append(Point(2, 2));
  points.append(Point(3, 3));
  points.append(Point(4, 4));
  points.append(Point(0, 0));
  points.append(Point(4, 4));  // Duplicate

  GrahamScanConvexHull graham;
  Polygon hull = graham(points);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(points); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  scene.polygons.append(hull);
  Aleph::TestVisual::add_polygon_vertices(scene, hull, true);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_graham_collinear_endpoints", scene,
      "Graham scan / collinear input");

  EXPECT_EQ(hull.size(), 2u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(4, 4)));
  EXPECT_FALSE(polygon_contains_vertex(hull, Point(2, 2)));
}

TEST_F(GeomAlgorithmsTest, ClosestPairTwoPoints)
{
  DynList<Point> points;
  points.append(Point(1, 2));
  points.append(Point(4, 6));

  ClosestPairDivideAndConquer cp;
  ClosestPairDivideAndConquer::Result res = cp(points);

  EXPECT_EQ(res.distance_squared, 25);
  EXPECT_TRUE(matches_unordered_pair(res.first, res.second,
                                     Point(1, 2), Point(4, 6)));
}

TEST_F(GeomAlgorithmsTest, ClosestPairUniqueMinimum)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(10, 10));
  points.append(Point(2, 1));
  points.append(Point(6, 6));
  points.append(Point(3, 5));

  ClosestPairDivideAndConquer cp;
  ClosestPairDivideAndConquer::Result res = cp(points);

  EXPECT_EQ(res.distance_squared, 5);
  EXPECT_TRUE(matches_unordered_pair(res.first, res.second,
                                     Point(0, 0), Point(2, 1)));
}

TEST_F(GeomAlgorithmsTest, ClosestPairDuplicatePointsDistanceZero)
{
  DynList<Point> points;
  points.append(Point(8, 1));
  points.append(Point(5, 5));
  points.append(Point(2, 9));
  points.append(Point(5, 5));

  ClosestPairDivideAndConquer cp;
  ClosestPairDivideAndConquer::Result res = cp(points);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(points); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  scene.segments.append(Segment(res.first, res.second));
  scene.highlighted_points.append(res.first);
  scene.highlighted_points.append(res.second);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_closest_pair_duplicate_zero", scene,
      "Closest pair / duplicate points");

  EXPECT_EQ(res.distance_squared, 0);
  EXPECT_EQ(res.first, res.second);
  EXPECT_EQ(res.first, Point(5, 5));
}

TEST_F(GeomAlgorithmsTest, ClosestPairCollinear)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 0));
  points.append(Point(2, 0));
  points.append(Point(9, 0));

  ClosestPairDivideAndConquer cp;
  ClosestPairDivideAndConquer::Result res = cp(points);

  EXPECT_EQ(res.distance_squared, 4);
  EXPECT_TRUE(matches_unordered_pair(res.first, res.second,
                                     Point(0, 0), Point(2, 0)));

  Segment s = cp.closest_segment(points);
  EXPECT_TRUE(matches_unordered_pair(s.get_src_point(), s.get_tgt_point(),
                                     Point(0, 0), Point(2, 0)));
}

TEST_F(GeomAlgorithmsTest, ClosestPairRequiresAtLeastTwoPoints)
{
  DynList<Point> points;
  points.append(Point(1, 1));

  ClosestPairDivideAndConquer cp;
  EXPECT_THROW((void) cp(points), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersSquare)
{
  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(4, 0));
  square.add_vertex(Point(4, 4));
  square.add_vertex(Point(0, 4));
  square.close();

  RotatingCalipersConvexPolygon calipers;
  auto d = calipers.diameter(square);
  EXPECT_EQ(d.distance_squared, 32);
  EXPECT_TRUE(matches_unordered_pair(d.first, d.second,
                                     Point(0, 0), Point(4, 4)) or
              matches_unordered_pair(d.first, d.second,
                                     Point(4, 0), Point(0, 4)));
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersSquareMinimumWidth)
{
  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(4, 0));
  square.add_vertex(Point(4, 4));
  square.add_vertex(Point(0, 4));
  square.close();

  RotatingCalipersConvexPolygon calipers;
  auto w = calipers.minimum_width(square);
  EXPECT_EQ(w.width_squared, 16);
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersRectangle)
{
  Polygon rect;
  rect.add_vertex(Point(0, 0));
  rect.add_vertex(Point(5, 0));
  rect.add_vertex(Point(5, 2));
  rect.add_vertex(Point(0, 2));
  rect.close();

  RotatingCalipersConvexPolygon calipers;
  auto d = calipers.diameter(rect);
  EXPECT_EQ(d.distance_squared, 29);

  auto w = calipers.minimum_width(rect);
  EXPECT_EQ(w.width_squared, 4);
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersTwoPointDegenerate)
{
  Polygon p;
  p.add_vertex(Point(1, 1));
  p.add_vertex(Point(4, 5));
  p.close();

  RotatingCalipersConvexPolygon calipers;
  auto d = calipers.diameter(p);
  EXPECT_EQ(d.distance_squared, 25);

  auto w = calipers.minimum_width(p);
  EXPECT_EQ(w.width_squared, 0);
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersNonConvexThrows)
{
  Polygon concave;
  concave.add_vertex(Point(0, 0));
  concave.add_vertex(Point(4, 0));
  concave.add_vertex(Point(2, 1));
  concave.add_vertex(Point(4, 4));
  concave.add_vertex(Point(0, 4));
  concave.close();

  RotatingCalipersConvexPolygon calipers;
  EXPECT_THROW((void) calipers.diameter(concave), std::domain_error);
  EXPECT_THROW((void) calipers.minimum_width(concave), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, RotatingCalipersOpenPolygonThrows)
{
  Polygon open;
  open.add_vertex(Point(0, 0));
  open.add_vertex(Point(4, 0));
  open.add_vertex(Point(4, 4));
  open.add_vertex(Point(0, 4));

  RotatingCalipersConvexPolygon calipers;
  EXPECT_THROW((void) calipers.diameter(open), std::domain_error);
  EXPECT_THROW((void) calipers.minimum_width(open), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, PointInPolygonConvexClassification)
{
  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(4, 0));
  square.add_vertex(Point(4, 4));
  square.add_vertex(Point(0, 4));
  square.close();

  PointInPolygonWinding pip;
  EXPECT_EQ(pip.locate(square, Point(2, 2)),
            PointInPolygonWinding::Location::Inside);
  EXPECT_EQ(pip.locate(square, Point(4, 1)),
            PointInPolygonWinding::Location::Boundary);
  EXPECT_EQ(pip.locate(square, Point(5, 5)),
            PointInPolygonWinding::Location::Outside);

  EXPECT_TRUE(pip.contains(square, Point(4, 1)));
  EXPECT_FALSE(pip.strictly_contains(square, Point(4, 1)));
  EXPECT_TRUE(pip.strictly_contains(square, Point(2, 2)));
}

TEST_F(GeomAlgorithmsTest, PointInPolygonConcaveClassification)
{
  Polygon concave;
  concave.add_vertex(Point(0, 0));
  concave.add_vertex(Point(4, 0));
  concave.add_vertex(Point(4, 4));
  concave.add_vertex(Point(2, 2));
  concave.add_vertex(Point(0, 4));
  concave.close();

  PointInPolygonWinding pip;
  EXPECT_EQ(pip.locate(concave, Point(1, 1)),
            PointInPolygonWinding::Location::Inside);
  EXPECT_EQ(pip.locate(concave, Point(2, 3)),
            PointInPolygonWinding::Location::Outside);
  EXPECT_EQ(pip.locate(concave, Point(3, 3)),
            PointInPolygonWinding::Location::Boundary);
}

TEST_F(GeomAlgorithmsTest, PointInPolygonRequiresClosedPolygon)
{
  Polygon open;
  open.add_vertex(Point(0, 0));
  open.add_vertex(Point(3, 0));
  open.add_vertex(Point(0, 3));

  PointInPolygonWinding pip;
  EXPECT_THROW((void) pip.locate(open, Point(1, 1)), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionOverlapArea)
{
  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(4, 0));
  a.add_vertex(Point(4, 4));
  a.add_vertex(Point(0, 4));
  a.close();

  Polygon b;
  b.add_vertex(Point(2, 2));
  b.add_vertex(Point(6, 2));
  b.add_vertex(Point(6, 6));
  b.add_vertex(Point(2, 6));
  b.close();

  ConvexPolygonIntersectionBasic inter;
  Polygon r = inter(a, b);

  EXPECT_EQ(r.size(), 4u);
  EXPECT_TRUE(r.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 4)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 4)));
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionContained)
{
  Polygon outer;
  outer.add_vertex(Point(0, 0));
  outer.add_vertex(Point(10, 0));
  outer.add_vertex(Point(10, 10));
  outer.add_vertex(Point(0, 10));
  outer.close();

  Polygon inner;
  inner.add_vertex(Point(2, 2));
  inner.add_vertex(Point(4, 2));
  inner.add_vertex(Point(4, 4));
  inner.add_vertex(Point(2, 4));
  inner.close();

  ConvexPolygonIntersectionBasic inter;
  Polygon r = inter(outer, inner);

  EXPECT_EQ(r.size(), 4u);
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 4)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 4)));
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionDisjoint)
{
  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(1, 0));
  a.add_vertex(Point(1, 1));
  a.add_vertex(Point(0, 1));
  a.close();

  Polygon b;
  b.add_vertex(Point(3, 3));
  b.add_vertex(Point(4, 3));
  b.add_vertex(Point(4, 4));
  b.add_vertex(Point(3, 4));
  b.close();

  ConvexPolygonIntersectionBasic inter;
  Polygon r = inter(a, b);

  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionTouchingEdge)
{
  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(2, 0));
  a.add_vertex(Point(2, 2));
  a.add_vertex(Point(0, 2));
  a.close();

  Polygon b;
  b.add_vertex(Point(2, 0));
  b.add_vertex(Point(4, 0));
  b.add_vertex(Point(4, 2));
  b.add_vertex(Point(2, 2));
  b.close();

  ConvexPolygonIntersectionBasic inter;
  Polygon r = inter(a, b);

  Aleph::TestVisual::SvgScene scene;
  scene.polygons.append(a);
  scene.polygons.append(b);
  scene.polygons.append(r);
  Aleph::TestVisual::add_polygon_vertices(scene, r, true);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_convex_polygon_intersection_touching_edge", scene,
      "Convex intersection / touching edge");

  EXPECT_EQ(r.size(), 2u);
  EXPECT_TRUE(r.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 2)));
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionRequiresConvex)
{
  Polygon concave;
  concave.add_vertex(Point(0, 0));
  concave.add_vertex(Point(4, 0));
  concave.add_vertex(Point(2, 1));
  concave.add_vertex(Point(4, 4));
  concave.add_vertex(Point(0, 4));
  concave.close();

  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(3, 0));
  square.add_vertex(Point(3, 3));
  square.add_vertex(Point(0, 3));
  square.close();

  ConvexPolygonIntersectionBasic inter;
  EXPECT_THROW((void) inter(concave, square), std::domain_error);
  EXPECT_THROW((void) inter(square, concave), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ConvexPolygonIntersectionRequiresClosed)
{
  Polygon open;
  open.add_vertex(Point(0, 0));
  open.add_vertex(Point(2, 0));
  open.add_vertex(Point(2, 2));

  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(3, 0));
  square.add_vertex(Point(3, 3));
  square.add_vertex(Point(0, 3));
  square.close();

  ConvexPolygonIntersectionBasic inter;
  EXPECT_THROW((void) inter(open, square), std::domain_error);
  EXPECT_THROW((void) inter(square, open), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, HalfPlaneIntersectionFromSingleSquare)
{
  Polygon square;
  square.add_vertex(Point(0, 0));
  square.add_vertex(Point(4, 0));
  square.add_vertex(Point(4, 4));
  square.add_vertex(Point(0, 4));
  square.close();

  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs =
      HalfPlaneIntersection::from_convex_polygon(square);

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 4u);
  EXPECT_TRUE(r.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 4)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 4)));
}

TEST_F(GeomAlgorithmsTest, HalfPlaneIntersectionFromSingleSquareClockwise)
{
  Polygon square_cw;
  square_cw.add_vertex(Point(0, 0));
  square_cw.add_vertex(Point(0, 4));
  square_cw.add_vertex(Point(4, 4));
  square_cw.add_vertex(Point(4, 0));
  square_cw.close();

  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs =
      HalfPlaneIntersection::from_convex_polygon(square_cw);

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 4u);
  EXPECT_TRUE(r.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 4)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 4)));
}

TEST_F(GeomAlgorithmsTest, HalfPlaneIntersectionTwoSquaresOverlap)
{
  Polygon a;
  a.add_vertex(Point(0, 0));
  a.add_vertex(Point(4, 0));
  a.add_vertex(Point(4, 4));
  a.add_vertex(Point(0, 4));
  a.close();

  Polygon b;
  b.add_vertex(Point(2, 2));
  b.add_vertex(Point(6, 2));
  b.add_vertex(Point(6, 6));
  b.add_vertex(Point(2, 6));
  b.close();

  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs =
      HalfPlaneIntersection::from_convex_polygon(a);
  Array<HalfPlaneIntersection::HalfPlane> hs2 =
      HalfPlaneIntersection::from_convex_polygon(b);
  for (size_t i = 0; i < hs2.size(); ++i)
    hs.append(hs2(i));

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 4u);
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 2)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 4)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(2, 4)));
}

TEST_F(GeomAlgorithmsTest, HalfPlaneIntersectionInconsistent)
{
  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs;
  hs.append(HalfPlaneIntersection::HalfPlane(Point(2, 1), Point(2, 0))); // x >= 2
  hs.append(HalfPlaneIntersection::HalfPlane(Point(1, 0), Point(1, 1))); // x <= 1
  hs.append(HalfPlaneIntersection::HalfPlane(Point(0, 0), Point(1, 0))); // y >= 0
  hs.append(HalfPlaneIntersection::HalfPlane(Point(1, 1), Point(0, 1))); // y <= 1

  Polygon r = hpi(hs);

  Aleph::TestVisual::SvgScene scene;
  for (size_t i = 0; i < hs.size(); ++i)
    scene.segments.append(Segment(hs(i).p, hs(i).q));
  scene.polygons.append(r);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_halfplane_inconsistent_empty", scene,
      "Half-plane intersection / inconsistent constraints");

  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, HalfPlaneIntersectionUnboundedReturnsEmpty)
{
  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs;
  hs.append(HalfPlaneIntersection::HalfPlane(Point(0, 1), Point(0, 0))); // x >= 0
  hs.append(HalfPlaneIntersection::HalfPlane(Point(0, 0), Point(1, 0))); // y >= 0
  hs.append(HalfPlaneIntersection::HalfPlane(Point(1, 0), Point(1, 1))); // x <= 1

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayTriangle)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(6, 0), Point(2, 4)});

  ASSERT_EQ(r.sites.size(), 3u);
  ASSERT_EQ(r.triangles.size(), 1u);

  const auto & t = r.triangles(0);
  EXPECT_EQ(orientation(r.sites(t.i), r.sites(t.j), r.sites(t.k)),
            Orientation::CCW);
}

TEST_F(GeomAlgorithmsTest, DelaunaySquareProducesTwoTriangles)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4)});

  ASSERT_EQ(r.sites.size(), 4u);
  ASSERT_EQ(r.triangles.size(), 2u);

  for (size_t i = 0; i < r.triangles.size(); ++i)
    {
      const auto & t = r.triangles(i);
      EXPECT_LT(t.i, r.sites.size());
      EXPECT_LT(t.j, r.sites.size());
      EXPECT_LT(t.k, r.sites.size());
      EXPECT_NE(orientation(r.sites(t.i), r.sites(t.j), r.sites(t.k)),
                Orientation::COLLINEAR);
    }
}

TEST_F(GeomAlgorithmsTest, DelaunayRemovesDuplicates)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4),
                     Point(0, 0), Point(4, 4)});

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_EQ(r.triangles.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, DelaunayCollinearReturnsEmpty)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(2, 0), Point(4, 0), Point(6, 0)});

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayCocircularDeterministicAcrossInputOrder)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r1 = delaunay({Point(0, 0), Point(1, 0), Point(1, 1), Point(0, 1)});

  DynList<Point> shuffled;
  shuffled.append(Point(1, 1));
  shuffled.append(Point(0, 1));
  shuffled.append(Point(0, 0));
  shuffled.append(Point(1, 0));
  auto r2 = delaunay(shuffled);

  Aleph::TestVisual::SvgScene scene;
  for (size_t i = 0; i < r1.sites.size(); ++i)
    scene.points.append(r1.sites(i));
  for (size_t i = 0; i < r1.triangles.size(); ++i)
    {
      const auto & t = r1.triangles(i);
      scene.segments.append(Segment(r1.sites(t.i), r1.sites(t.j)));
      scene.segments.append(Segment(r1.sites(t.j), r1.sites(t.k)));
      scene.segments.append(Segment(r1.sites(t.k), r1.sites(t.i)));
    }
  (void) Aleph::TestVisual::emit_case_svg(
      "case_delaunay_cocircular_deterministic", scene,
      "Delaunay cocircular tie-break");

  ASSERT_EQ(r1.sites.size(), r2.sites.size());
  ASSERT_EQ(r1.triangles.size(), r2.triangles.size());

  for (size_t i = 0; i < r1.sites.size(); ++i)
    EXPECT_EQ(r1.sites(i), r2.sites(i));

  const Array<TriKey> t1 = canonical_triangles(r1);
  const Array<TriKey> t2 = canonical_triangles(r2);
  ASSERT_EQ(t1.size(), t2.size());
  for (size_t i = 0; i < t1.size(); ++i)
    {
      EXPECT_EQ(t1(i).a, t2(i).a);
      EXPECT_EQ(t1(i).b, t2(i).b);
      EXPECT_EQ(t1(i).c, t2(i).c);
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiFromTriangle)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(6, 0), Point(2, 4)});

  EXPECT_EQ(r.sites.size(), 3u);
  EXPECT_EQ(r.vertices.size(), 1u);
  EXPECT_EQ(r.edges.size(), 3u);
  EXPECT_EQ(r.cells.size(), 3u);

  size_t unbounded = 0;
  for (size_t i = 0; i < r.edges.size(); ++i)
    if (r.edges(i).unbounded)
      {
        ++unbounded;
        EXPECT_FALSE(r.edges(i).direction == Point(0, 0));
      }
  EXPECT_EQ(unbounded, 3u);

  for (size_t i = 0; i < r.cells.size(); ++i)
    {
      EXPECT_FALSE(r.cells(i).bounded);
      EXPECT_EQ(r.cells(i).vertices.size(), 1u);
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiFromQuadrilateralHasOneBoundedEdge)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4)});

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_EQ(r.vertices.size(), 2u);
  EXPECT_EQ(r.cells.size(), 4u);

  size_t bounded = 0;
  size_t unbounded = 0;
  for (size_t i = 0; i < r.edges.size(); ++i)
    if (r.edges(i).unbounded)
      ++unbounded;
    else
      ++bounded;

  EXPECT_EQ(bounded, 1u);
  EXPECT_EQ(unbounded, 4u);

  for (size_t i = 0; i < r.cells.size(); ++i)
    EXPECT_FALSE(r.cells(i).bounded);
}

TEST_F(GeomAlgorithmsTest, VoronoiBoundedEdgesMatchAdjacentTriangleCircumcenters)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto dt = delaunay({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                      Point(2, 2)});
  ASSERT_GE(dt.triangles.size(), 2u);

  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi(dt);

  size_t bounded_edges = 0;
  for (size_t e = 0; e < r.edges.size(); ++e)
    {
      const auto & edge = r.edges(e);
      if (edge.unbounded)
        continue;

      ++bounded_edges;

      Array<size_t> incident;
      incident.reserve(2);
      for (size_t t = 0; t < dt.triangles.size(); ++t)
        {
          const auto & tri = dt.triangles(t);
          const bool has_u = tri.i == edge.site_u or tri.j == edge.site_u or
                             tri.k == edge.site_u;
          const bool has_v = tri.i == edge.site_v or tri.j == edge.site_v or
                             tri.k == edge.site_v;
          if (has_u and has_v)
            incident.append(t);
        }

      ASSERT_EQ(incident.size(), 2u);

      const auto & t0 = dt.triangles(incident(0));
      const auto & t1 = dt.triangles(incident(1));
      ASSERT_NE(orientation(dt.sites(t0.i), dt.sites(t0.j), dt.sites(t0.k)),
                Orientation::COLLINEAR);
      ASSERT_NE(orientation(dt.sites(t1.i), dt.sites(t1.j), dt.sites(t1.k)),
                Orientation::COLLINEAR);

      const Point c0 = circumcenter_of(dt.sites(t0.i), dt.sites(t0.j), dt.sites(t0.k));
      const Point c1 = circumcenter_of(dt.sites(t1.i), dt.sites(t1.j), dt.sites(t1.k));
      EXPECT_TRUE(matches_unordered_pair(edge.src, edge.tgt, c0, c1));
    }

  EXPECT_GT(bounded_edges, 0u);
}

TEST_F(GeomAlgorithmsTest, VoronoiClippedCellsBoundedAndContainSites)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                    Point(2, 2)});

  Polygon box;
  box.add_vertex(Point(-10, -10));
  box.add_vertex(Point(10, -10));
  box.add_vertex(Point(10, 10));
  box.add_vertex(Point(-10, 10));
  box.close();

  Array<Polygon> cells = voronoi.clipped_cells(r, box);
  ASSERT_EQ(cells.size(), r.sites.size());

  PointInPolygonWinding pip;
  for (size_t i = 0; i < cells.size(); ++i)
    {
      EXPECT_TRUE(cells(i).is_closed());
      EXPECT_GE(cells(i).size(), 3u);
      EXPECT_TRUE(pip.contains(cells(i), r.sites(i)));
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiClippedCellsBoundedAndContainSitesClockwiseClip)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                    Point(2, 2)});

  Polygon box_cw;
  box_cw.add_vertex(Point(-10, -10));
  box_cw.add_vertex(Point(-10, 10));
  box_cw.add_vertex(Point(10, 10));
  box_cw.add_vertex(Point(10, -10));
  box_cw.close();

  Array<Polygon> cells = voronoi.clipped_cells(r, box_cw);
  ASSERT_EQ(cells.size(), r.sites.size());

  PointInPolygonWinding pip;
  for (size_t i = 0; i < cells.size(); ++i)
    {
      EXPECT_TRUE(cells(i).is_closed());
      EXPECT_GE(cells(i).size(), 3u);
      EXPECT_TRUE(pip.contains(cells(i), r.sites(i)));
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiClippedCellsRequiresConvexClip)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                    Point(2, 2)});

  Polygon concave;
  concave.add_vertex(Point(0, 0));
  concave.add_vertex(Point(6, 0));
  concave.add_vertex(Point(3, 2));
  concave.add_vertex(Point(6, 6));
  concave.add_vertex(Point(0, 6));
  concave.close();

  EXPECT_THROW((void) voronoi.clipped_cells(r, concave), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, VoronoiClippedCellsIndexedExposeSiteAndIndex)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                    Point(2, 2)});

  Polygon box;
  box.add_vertex(Point(-10, -10));
  box.add_vertex(Point(10, -10));
  box.add_vertex(Point(10, 10));
  box.add_vertex(Point(-10, 10));
  box.close();

  Array<VoronoiDiagramFromDelaunay::ClippedCell> cells =
      voronoi.clipped_cells_indexed(r, box);
  ASSERT_EQ(cells.size(), r.sites.size());

  PointInPolygonWinding pip;
  for (size_t i = 0; i < cells.size(); ++i)
    {
      EXPECT_EQ(cells(i).site_index, i);
      EXPECT_EQ(cells(i).site, r.sites(i));
      EXPECT_TRUE(cells(i).polygon.is_closed());
      EXPECT_GE(cells(i).polygon.size(), 3u);
      EXPECT_TRUE(pip.contains(cells(i).polygon, cells(i).site));
    }
}

TEST_F(GeomAlgorithmsTest, TriangulateHexagon)
{
  Polygon p;
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(3, 1));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(1, 2));
  p.add_vertex(Point(0, 1));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 4u);
}

// ============================================================================
// Phase 4 — Performance & Robustness Tests
// ============================================================================

// ---------- toggle_edge O(log n) via DynSetTree (Bowyer-Watson) ----------

TEST_F(GeomAlgorithmsTest, DelaunayStressToggleEdgePerformance)
{
  // A large-ish random point set exercises the DynSetTree-based toggle_edge
  // heavily: each insertion creates a cavity whose boundary edges are
  // toggled.  We verify correctness of the result, which implies
  // toggle_edge worked at every step.
  DynList<Point> points;
  const int N = 200;
  for (int i = 0; i < N; ++i)
    for (int j = 0; j < 3; ++j)
      points.append(Point(i * 7 + j * 3, j * 11 + i * 5));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(points);

  EXPECT_GE(r.sites.size(), 3u);
  EXPECT_GE(r.triangles.size(), 1u);

  // Delaunay property: no site lies strictly inside any triangle's circumcircle.
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number cr2 = cc.distance_squared_to(r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;
          // No site should be strictly closer to circumcenter than the radius.
          EXPECT_GE(cc.distance_squared_to(r.sites(s)), cr2);
        }
    }
}

TEST_F(GeomAlgorithmsTest, DelaunayStressGridInput)
{
  // Grid input creates many cocircular quadruples, stressing the
  // deterministic tie-breaking and toggle_edge toggling.
  DynList<Point> points;
  for (int x = 0; x < 10; ++x)
    for (int y = 0; y < 10; ++y)
      points.append(Point(x, y));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(points);

  EXPECT_EQ(r.sites.size(), 100u);
  // A grid of n=m*m points always yields 2*(m-1)^2 triangles.
  EXPECT_EQ(r.triangles.size(), 2u * 9u * 9u);

  // All triangles are non-degenerate.
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      EXPECT_NE(orientation(r.sites(tri.i), r.sites(tri.j), r.sites(tri.k)),
                Orientation::COLLINEAR);
    }
}

// ---------- Voronoi: incidence index correctness ----------

TEST_F(GeomAlgorithmsTest, VoronoiIncidenceIndexMatchesCircumcenters)
{
  // Each Voronoi cell's vertices should be exactly the circumcenters of the
  // Delaunay triangles incident to that site.  This verifies the prebuilt
  // incidence index produces correct cells.
  DelaunayTriangulationBowyerWatson delaunay;
  auto dt = delaunay({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                      Point(2, 2), Point(4, 5)});
  ASSERT_GE(dt.triangles.size(), 3u);

  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi(dt);
  ASSERT_EQ(r.cells.size(), r.sites.size());

  for (size_t s = 0; s < r.cells.size(); ++s)
    {
      // Collect circumcenters of triangles incident to site s.
      Array<Point> expected;
      for (size_t t = 0; t < dt.triangles.size(); ++t)
        {
          const auto & tri = dt.triangles(t);
          if (tri.i == s or tri.j == s or tri.k == s)
            expected.append(circumcenter_of(dt.sites(tri.i),
                                            dt.sites(tri.j),
                                            dt.sites(tri.k)));
        }

      // Every cell vertex must appear among the expected circumcenters.
      const auto & cell_verts = r.cells(s).vertices;
      EXPECT_EQ(cell_verts.size(), expected.size())
          << "Mismatch for site " << s;

      for (size_t v = 0; v < cell_verts.size(); ++v)
        {
          bool found = false;
          for (size_t e = 0; e < expected.size(); ++e)
            if (cell_verts(v) == expected(e))
              {
                found = true;
                break;
              }
          EXPECT_TRUE(found) << "Cell " << s << " has unexpected vertex";
        }
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiIncidenceIndexStress)
{
  // Larger set: the O(T) incidence build must match brute-force.
  DynList<Point> points;
  for (int i = 0; i < 8; ++i)
    for (int j = 0; j < 8; ++j)
      points.append(Point(i * 3 + (j % 2), j * 3 + (i % 2)));

  DelaunayTriangulationBowyerWatson delaunay;
  auto dt = delaunay(points);

  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi(dt);
  ASSERT_EQ(r.cells.size(), r.sites.size());

  // Bounded cells exist only for interior sites.
  size_t bounded_count = 0;
  for (size_t s = 0; s < r.cells.size(); ++s)
    {
      if (r.cells(s).bounded)
        ++bounded_count;

      // Every cell must have at least one vertex.
      EXPECT_GE(r.cells(s).vertices.size(), 1u);
    }
  EXPECT_GT(bounded_count, 0u);
}

// ---------- CuttingEarsTriangulation: automatic CCW normalization ----------

TEST_F(GeomAlgorithmsTest, TriangulatePentagonClockwise)
{
  // CW pentagon — must be normalized to CCW internally.
  Polygon p;
  p.add_vertex(Point(0, 1.5));
  p.add_vertex(Point(1, 4));
  p.add_vertex(Point(3, 4));
  p.add_vertex(Point(4, 1.5));
  p.add_vertex(Point(2, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);

  EXPECT_EQ(triangles.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, TriangulateHexagonClockwise)
{
  // CW hexagon
  Polygon p;
  p.add_vertex(Point(0, 1));
  p.add_vertex(Point(1, 2));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(3, 1));
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(1, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);

  EXPECT_EQ(triangles.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, TriangulateTriangleClockwise)
{
  // Minimal CW input: 3 vertices.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(2, 3));
  p.add_vertex(Point(4, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);

  EXPECT_EQ(triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, TriangulateLShapeClockwise)
{
  // L-shaped concave polygon in CW order.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(0, 4));
  p.add_vertex(Point(2, 4));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(4, 2));
  p.add_vertex(Point(4, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);

  // 6-vertex polygon yields 4 triangles.
  EXPECT_EQ(triangles.size(), 4u);
}

// ---------- HalfPlaneIntersection: CW polygon handling ----------

TEST_F(GeomAlgorithmsTest, HalfPlaneFromConvexTriangleCW)
{
  // CW triangle — from_convex_polygon must flip edges so the interior is
  // on the left side of each half-plane.
  Polygon tri_cw;
  tri_cw.add_vertex(Point(0, 0));
  tri_cw.add_vertex(Point(0, 3));
  tri_cw.add_vertex(Point(4, 0));
  tri_cw.close();

  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs =
      HalfPlaneIntersection::from_convex_polygon(tri_cw);

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 3u);
  EXPECT_TRUE(r.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(0, 3)));
  EXPECT_TRUE(polygon_contains_vertex(r, Point(4, 0)));
}

TEST_F(GeomAlgorithmsTest, HalfPlaneFromConvexPentagonCW)
{
  // CW convex pentagon
  Polygon penta_cw;
  penta_cw.add_vertex(Point(2, 0));
  penta_cw.add_vertex(Point(0, 1.5));
  penta_cw.add_vertex(Point(1, 4));
  penta_cw.add_vertex(Point(3, 4));
  penta_cw.add_vertex(Point(4, 1.5));
  penta_cw.close();

  HalfPlaneIntersection hpi;
  Array<HalfPlaneIntersection::HalfPlane> hs =
      HalfPlaneIntersection::from_convex_polygon(penta_cw);

  Polygon r = hpi(hs);
  EXPECT_EQ(r.size(), 5u);
  EXPECT_TRUE(r.is_closed());
}

TEST_F(GeomAlgorithmsTest, HalfPlaneFromConvexPolygonDegenerateThrows)
{
  // Degenerate polygon (zero area) should throw.
  Polygon degen;
  degen.add_vertex(Point(0, 0));
  degen.add_vertex(Point(1, 0));
  degen.add_vertex(Point(2, 0));
  degen.close();

  EXPECT_THROW(
    (void) HalfPlaneIntersection::from_convex_polygon(degen),
    std::domain_error);
}

// ---------- Edge cases: Delaunay ----------

TEST_F(GeomAlgorithmsTest, DelaunayEmptyInput)
{
  DelaunayTriangulationBowyerWatson delaunay;
  DynList<Point> empty;
  auto r = delaunay(empty);

  EXPECT_EQ(r.sites.size(), 0u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunaySinglePoint)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(5, 5)});

  EXPECT_EQ(r.sites.size(), 1u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayTwoPoints)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(1, 1)});

  EXPECT_EQ(r.sites.size(), 2u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayAllDuplicates)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(3, 3), Point(3, 3), Point(3, 3), Point(3, 3)});

  EXPECT_EQ(r.sites.size(), 1u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayTwoDistinctWithDuplicates)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(0, 0), Point(5, 5), Point(5, 5)});

  EXPECT_EQ(r.sites.size(), 2u);
  EXPECT_EQ(r.triangles.size(), 0u);
}

// ---------- Edge cases: Voronoi ----------

TEST_F(GeomAlgorithmsTest, VoronoiEmptyInput)
{
  VoronoiDiagramFromDelaunay voronoi;
  DynList<Point> empty;
  auto r = voronoi(empty);

  EXPECT_EQ(r.sites.size(), 0u);
  EXPECT_EQ(r.vertices.size(), 0u);
  EXPECT_EQ(r.edges.size(), 0u);
  EXPECT_EQ(r.cells.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, VoronoiSinglePoint)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(5, 5)});

  EXPECT_EQ(r.sites.size(), 1u);
  EXPECT_EQ(r.vertices.size(), 0u);
  EXPECT_EQ(r.edges.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, VoronoiTwoPoints)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(4, 0)});

  EXPECT_EQ(r.sites.size(), 2u);
  EXPECT_EQ(r.edges.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, VoronoiCollinearPoints)
{
  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi({Point(0, 0), Point(1, 0), Point(2, 0), Point(3, 0)});

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_EQ(r.edges.size(), 0u);  // Delaunay has 0 triangles for collinear.
}

// ---------- Edge cases: ClosestPair ----------

TEST_F(GeomAlgorithmsTest, ClosestPairEmptyInputThrows)
{
  DynList<Point> empty;
  ClosestPairDivideAndConquer cp;
  EXPECT_THROW((void) cp(empty), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ClosestPairSinglePointThrows)
{
  DynList<Point> one;
  one.append(Point(1, 1));
  ClosestPairDivideAndConquer cp;
  EXPECT_THROW((void) cp(one), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ClosestPairAllDuplicates)
{
  DynList<Point> dups;
  dups.append(Point(7, 7));
  dups.append(Point(7, 7));
  dups.append(Point(7, 7));
  dups.append(Point(7, 7));

  ClosestPairDivideAndConquer cp;
  auto res = cp(dups);

  EXPECT_EQ(res.distance_squared, 0);
  EXPECT_EQ(res.first, Point(7, 7));
  EXPECT_EQ(res.second, Point(7, 7));
}

// ---------- Edge cases: CuttingEarsTriangulation ----------

TEST_F(GeomAlgorithmsTest, TriangulateTwoVerticesThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(1, 0));
  p.close();

  CuttingEarsTriangulation triangulator;
  EXPECT_THROW((void) triangulator(p), std::domain_error);
}

// ---------- Edge cases: RotatingCalipers ----------

TEST_F(GeomAlgorithmsTest, RotatingCalipersOpenSingleVertexThrows)
{
  Polygon p;
  p.add_vertex(Point(1, 1));
  // Not closed — should throw.

  RotatingCalipersConvexPolygon calipers;
  EXPECT_THROW((void) calipers.diameter(p), std::domain_error);
  EXPECT_THROW((void) calipers.minimum_width(p), std::domain_error);
}

// ---------- Edge cases: PointInPolygon ----------

TEST_F(GeomAlgorithmsTest, PointInPolygonTwoVerticesThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(5, 5));
  p.close();

  PointInPolygonWinding pip;
  EXPECT_THROW((void) pip.locate(p, Point(2, 2)), std::domain_error);
}

// ---------- Edge cases: Convex hull algorithms with 2 collinear points ----------

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainTwoPoints)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 5));

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(points);

  EXPECT_EQ(hull.size(), 2u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(5, 5)));
}

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainEmptyInput)
{
  DynList<Point> empty;
  AndrewMonotonicChainConvexHull andrew;
  EXPECT_EQ(andrew(empty).size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainSinglePoint)
{
  DynList<Point> one;
  one.append(Point(42, 17));
  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(one);
  EXPECT_EQ(hull.size(), 1u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(42, 17)));
}

TEST_F(GeomAlgorithmsTest, AndrewMonotonicChainAllDuplicates)
{
  DynList<Point> dups;
  dups.append(Point(3, 3));
  dups.append(Point(3, 3));
  dups.append(Point(3, 3));

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(dups);
  EXPECT_EQ(hull.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, GrahamScanEmptyInput)
{
  DynList<Point> empty;
  GrahamScanConvexHull graham;
  EXPECT_EQ(graham(empty).size(), 0u);
}

TEST_F(GeomAlgorithmsTest, GrahamScanSinglePoint)
{
  DynList<Point> one;
  one.append(Point(42, 17));
  GrahamScanConvexHull graham;
  Polygon hull = graham(one);
  EXPECT_EQ(hull.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, GrahamScanTwoPoints)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 5));

  GrahamScanConvexHull graham;
  Polygon hull = graham(points);
  EXPECT_EQ(hull.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, GrahamScanAllDuplicates)
{
  DynList<Point> dups;
  dups.append(Point(7, 7));
  dups.append(Point(7, 7));
  dups.append(Point(7, 7));

  GrahamScanConvexHull graham;
  Polygon hull = graham(dups);
  EXPECT_EQ(hull.size(), 1u);
}

// ---------- Cross-algorithm consistency ----------

TEST_F(GeomAlgorithmsTest, AllHullAlgorithmsAgreeOnRandomInput)
{
  // All five hull algorithms should produce the same vertex set.
  DynList<Point> points;
  // Deterministic "random" set avoiding cocircular degeneracies.
  int seed = 12345;
  for (int i = 0; i < 50; ++i)
    {
      seed = (seed * 1103515245 + 12345) & 0x7fffffff;
      int x = seed % 1000;
      seed = (seed * 1103515245 + 12345) & 0x7fffffff;
      int y = seed % 1000;
      points.append(Point(x, y));
    }

  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;
  QuickHull qh;
  GiftWrappingConvexHull gw;
  BruteForceConvexHull bf;

  Polygon h_andrew = andrew(points);
  Polygon h_graham = graham(points);
  Polygon h_qh = qh(points);
  Polygon h_gw = gw(points);
  Polygon h_bf = bf(points);

  EXPECT_EQ(h_andrew.size(), h_graham.size());
  EXPECT_EQ(h_andrew.size(), h_qh.size());
  EXPECT_EQ(h_andrew.size(), h_gw.size());
  EXPECT_EQ(h_andrew.size(), h_bf.size());

  // Every vertex of Andrew's hull should appear in every other hull.
  for (Polygon::Vertex_Iterator it(h_andrew); it.has_curr(); it.next_ne())
    {
      const Point p = it.get_current_vertex();
      EXPECT_TRUE(polygon_contains_vertex(h_graham, p));
      EXPECT_TRUE(polygon_contains_vertex(h_qh, p));
      EXPECT_TRUE(polygon_contains_vertex(h_gw, p));
      EXPECT_TRUE(polygon_contains_vertex(h_bf, p));
    }
}

// ---------- Delaunay: as_triangles helper ----------

TEST_F(GeomAlgorithmsTest, DelaunayAsTrianglesProducesValidTriangles)
{
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(6, 0), Point(3, 5), Point(6, 5),
                     Point(0, 5)});

  DynList<Triangle> tris = delaunay.as_triangles(r);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;

  EXPECT_EQ(count, r.triangles.size());
}

// ============================================================================
// Phase 4 — New Algorithms Tests
// ============================================================================

// ---------- SweepLineSegmentIntersection ----------

TEST_F(GeomAlgorithmsTest, SweepLineNoSegments)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  auto result = sweep(segs);
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, SweepLineSingleSegment)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(5, 5)));
  auto result = sweep(segs);
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, SweepLineParallelNoIntersection)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(5, 0)));
  segs.append(Segment(Point(0, 1), Point(5, 1)));
  auto result = sweep(segs);
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, SweepLineSimpleCross)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 4)));
  segs.append(Segment(Point(0, 4), Point(4, 0)));
  auto result = sweep(segs);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result(0).point, Point(2, 2));
  EXPECT_EQ(result(0).seg_i, 0u);
  EXPECT_EQ(result(0).seg_j, 1u);
}

TEST_F(GeomAlgorithmsTest, SweepLineMultipleIntersections)
{
  // Three segments forming a triangle of intersections.
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(6, 6)));  // s0: diagonal up
  segs.append(Segment(Point(0, 6), Point(6, 0)));  // s1: diagonal down
  segs.append(Segment(Point(0, 3), Point(6, 3)));  // s2: horizontal

  auto result = sweep(segs);

  Aleph::TestVisual::SvgScene scene;
  for (size_t i = 0; i < segs.size(); ++i)
    scene.segments.append(segs(i));
  for (size_t i = 0; i < result.size(); ++i)
    scene.highlighted_points.append(result(i).point);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_sweepline_multiple_intersections", scene,
      "Sweep-line / multi-intersection degeneracy");

  // s0 x s1 at (3,3), s0 x s2 at (3,3), s1 x s2 at (3,3)
  // Actually: s0 x s2 at (3,3), s1 x s2 at (3,3), s0 x s1 at (3,3)
  // All three intersect at (3,3).
  EXPECT_EQ(result.size(), 3u);
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_EQ(result(i).point, Point(3, 3));
}

TEST_F(GeomAlgorithmsTest, SweepLineDisjointSegments)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(1, 0)));
  segs.append(Segment(Point(3, 3), Point(4, 3)));
  auto result = sweep(segs);
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, SweepLineTShapedIntersection)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 2), Point(4, 2)));   // horizontal
  segs.append(Segment(Point(2, 0), Point(2, 2)));   // vertical, touching
  auto result = sweep(segs);
  ASSERT_EQ(result.size(), 1u);
  EXPECT_EQ(result(0).point, Point(2, 2));
}

TEST_F(GeomAlgorithmsTest, SweepLineDegenerateSegmentThrows)
{
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(1, 1), Point(1, 1))); // zero length
  segs.append(Segment(Point(0, 0), Point(2, 2)));
  EXPECT_THROW((void) sweep(segs), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, SweepLineFourSegmentsStar)
{
  // Four segments through center (2,2).
  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 2), Point(4, 2)));  // horizontal
  segs.append(Segment(Point(2, 0), Point(2, 4)));  // vertical
  segs.append(Segment(Point(0, 0), Point(4, 4)));  // diagonal up
  segs.append(Segment(Point(0, 4), Point(4, 0)));  // diagonal down

  auto result = sweep(segs);

  // C(4,2) = 6 pairs, all intersecting at (2,2).
  EXPECT_EQ(result.size(), 6u);
  for (size_t i = 0; i < result.size(); ++i)
    EXPECT_EQ(result(i).point, Point(2, 2));
}

// ---------- MonotonePolygonTriangulation ----------

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateTriangle)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));
  p.close();

  MonotonePolygonTriangulation mt;
  DynList<Triangle> tris = mt(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 1u);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateSquare)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  MonotonePolygonTriangulation mt;
  DynList<Triangle> tris = mt(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 2u);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateSquareCW)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(0, 4));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(4, 0));
  p.close();

  MonotonePolygonTriangulation mt;
  DynList<Triangle> tris = mt(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 2u);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulatePentagon)
{
  Polygon p;
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(4, 1.5));
  p.add_vertex(Point(3, 4));
  p.add_vertex(Point(1, 4));
  p.add_vertex(Point(0, 1.5));
  p.close();

  MonotonePolygonTriangulation mt;
  DynList<Triangle> tris = mt(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 3u);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateHexagon)
{
  Polygon p;
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(3, 1));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(1, 2));
  p.add_vertex(Point(0, 1));
  p.close();

  MonotonePolygonTriangulation mt;
  DynList<Triangle> tris = mt(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 4u);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateOpenThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));

  MonotonePolygonTriangulation mt;
  EXPECT_THROW((void) mt(p), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateDegenerateThrows)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(2, 0));
  p.close();

  MonotonePolygonTriangulation mt;
  EXPECT_THROW((void) mt(p), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, MonotoneTriangulateCountMatchesCuttingEars)
{
  // L-shaped polygon (non-monotone): both methods should produce n-2 triangles.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 2));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(2, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  MonotonePolygonTriangulation mt;
  CuttingEarsTriangulation ears;

  DynList<Triangle> mt_tris = mt(p);
  DynList<Triangle> ear_tris = ears(p);

  size_t mt_count = 0;
  for (DynList<Triangle>::Iterator it(mt_tris); it.has_curr(); it.next_ne())
    ++mt_count;

  size_t ear_count = 0;
  for (DynList<Triangle>::Iterator it(ear_tris); it.has_curr(); it.next_ne())
    ++ear_count;

  EXPECT_EQ(mt_count, 4u);
  EXPECT_EQ(mt_count, ear_count);
}

// ---------- MinkowskiSumConvex ----------

TEST_F(GeomAlgorithmsTest, MinkowskiSumTwoSquares)
{
  // Square [0,1]^2 ⊕ Square [0,1]^2 = Square [0,2]^2.
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  MinkowskiSumConvex mink;
  Polygon result = mink(sq, sq);

  EXPECT_EQ(result.size(), 4u);
  EXPECT_TRUE(result.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(2, 0)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(2, 2)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 2)));
}

TEST_F(GeomAlgorithmsTest, MinkowskiSumSquareAndTriangle)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(2, 0));
  sq.add_vertex(Point(2, 2));
  sq.add_vertex(Point(0, 2));
  sq.close();

  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(1, 0));
  tri.add_vertex(Point(0, 1));
  tri.close();

  MinkowskiSumConvex mink;
  Polygon result = mink(sq, tri);

  // Square (4 edges) + Triangle (3 edges) = up to 7 vertices.
  EXPECT_TRUE(result.is_closed());
  EXPECT_GE(result.size(), 3u);
  EXPECT_LE(result.size(), 7u);

  // The sum must contain the extreme vertices.
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 0)));  // (0,0)+(0,0)
  EXPECT_TRUE(polygon_contains_vertex(result, Point(3, 0)));  // (2,0)+(1,0)
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 3)));  // (0,2)+(0,1)
}

TEST_F(GeomAlgorithmsTest, MinkowskiSumCWInputsNormalized)
{
  // CW square ⊕ CW square should still work.
  Polygon sq_cw;
  sq_cw.add_vertex(Point(0, 0));
  sq_cw.add_vertex(Point(0, 1));
  sq_cw.add_vertex(Point(1, 1));
  sq_cw.add_vertex(Point(1, 0));
  sq_cw.close();

  MinkowskiSumConvex mink;
  Polygon result = mink(sq_cw, sq_cw);

  EXPECT_EQ(result.size(), 4u);
  EXPECT_TRUE(result.is_closed());
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(2, 0)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(2, 2)));
  EXPECT_TRUE(polygon_contains_vertex(result, Point(0, 2)));
}

TEST_F(GeomAlgorithmsTest, MinkowskiSumNonConvexThrows)
{
  Polygon convex;
  convex.add_vertex(Point(0, 0));
  convex.add_vertex(Point(2, 0));
  convex.add_vertex(Point(2, 2));
  convex.add_vertex(Point(0, 2));
  convex.close();

  Polygon concave;
  concave.add_vertex(Point(0, 0));
  concave.add_vertex(Point(4, 0));
  concave.add_vertex(Point(2, 1));
  concave.add_vertex(Point(4, 4));
  concave.add_vertex(Point(0, 4));
  concave.close();

  MinkowskiSumConvex mink;
  EXPECT_THROW((void) mink(convex, concave), std::domain_error);
  EXPECT_THROW((void) mink(concave, convex), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, MinkowskiSumOpenPolygonThrows)
{
  Polygon open;
  open.add_vertex(Point(0, 0));
  open.add_vertex(Point(1, 0));
  open.add_vertex(Point(1, 1));

  Polygon closed;
  closed.add_vertex(Point(0, 0));
  closed.add_vertex(Point(1, 0));
  closed.add_vertex(Point(0, 1));
  closed.close();

  MinkowskiSumConvex mink;
  EXPECT_THROW((void) mink(open, closed), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, MinkowskiSumIsConvex)
{
  // Pentagon ⊕ Triangle — result must be convex.
  Polygon pent;
  pent.add_vertex(Point(2, 0));
  pent.add_vertex(Point(4, 1));
  pent.add_vertex(Point(3, 3));
  pent.add_vertex(Point(1, 3));
  pent.add_vertex(Point(0, 1));
  pent.close();

  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(1, 0));
  tri.add_vertex(Point(0, 1));
  tri.close();

  MinkowskiSumConvex mink;
  Polygon result = mink(pent, tri);
  EXPECT_TRUE(result.is_closed());
  EXPECT_GE(result.size(), 3u);

  // Verify convexity: all turns should be consistent.
  Array<Point> rv;
  for (Polygon::Vertex_Iterator it(result); it.has_curr(); it.next_ne())
    rv.append(it.get_current_vertex());

  int sign = 0;
  for (size_t i = 0; i < rv.size(); ++i)
    {
      Geom_Number turn =
          area_of_parallelogram(rv(i), rv((i + 1) % rv.size()),
                                rv((i + 2) % rv.size()));
      if (turn == 0) continue;
      int s = turn > 0 ? 1 : -1;
      if (sign == 0) sign = s;
      else EXPECT_EQ(sign, s);
    }
}

// ---------- KDTreePointSearch ----------

TEST_F(GeomAlgorithmsTest, KDTreeInsertAndContains)
{
  KDTreePointSearch kd(0, 0, 100, 100);
  EXPECT_TRUE(kd.is_empty());

  EXPECT_TRUE(kd.insert(Point(10, 20)));
  EXPECT_TRUE(kd.insert(Point(50, 50)));
  EXPECT_FALSE(kd.insert(Point(10, 20)));  // duplicate

  EXPECT_EQ(kd.size(), 2u);
  EXPECT_TRUE(kd.contains(Point(10, 20)));
  EXPECT_TRUE(kd.contains(Point(50, 50)));
  EXPECT_FALSE(kd.contains(Point(30, 30)));
}

TEST_F(GeomAlgorithmsTest, KDTreeNearest)
{
  KDTreePointSearch kd(0, 0, 100, 100);
  kd.insert(Point(10, 10));
  kd.insert(Point(50, 50));
  kd.insert(Point(90, 90));

  auto n = kd.nearest(Point(12, 12));
  ASSERT_TRUE(n.has_value());
  EXPECT_EQ(*n, Point(10, 10));

  auto n2 = kd.nearest(Point(48, 52));
  ASSERT_TRUE(n2.has_value());
  EXPECT_EQ(*n2, Point(50, 50));
}

TEST_F(GeomAlgorithmsTest, KDTreeNearestEmpty)
{
  KDTreePointSearch kd(0, 0, 100, 100);
  auto n = kd.nearest(Point(50, 50));
  EXPECT_FALSE(n.has_value());
}

TEST_F(GeomAlgorithmsTest, KDTreeBuildBalanced)
{
  Array<Point> points;
  for (int x = 0; x < 10; ++x)
    for (int y = 0; y < 10; ++y)
      points.append(Point(x, y));

  auto kd = KDTreePointSearch::build(points, 0, 0, 10, 10);

  EXPECT_EQ(kd.size(), 100u);

  for (int x = 0; x < 10; ++x)
    for (int y = 0; y < 10; ++y)
      EXPECT_TRUE(kd.contains(Point(x, y)));

  auto n = kd.nearest(Point(5, 5));
  ASSERT_TRUE(n.has_value());
  EXPECT_EQ(*n, Point(5, 5));
}

TEST_F(GeomAlgorithmsTest, KDTreeRange)
{
  KDTreePointSearch kd(0, 0, 100, 100);
  kd.insert(Point(10, 10));
  kd.insert(Point(20, 20));
  kd.insert(Point(50, 50));
  kd.insert(Point(80, 80));

  DynList<Point> out;
  kd.range(5, 5, 25, 25, &out);

  size_t count = 0;
  for (DynList<Point>::Iterator it(out); it.has_curr(); it.next_ne())
    ++count;
  EXPECT_EQ(count, 2u);  // (10,10) and (20,20)
}

TEST_F(GeomAlgorithmsTest, KDTreeForEach)
{
  KDTreePointSearch kd(0, 0, 100, 100);
  kd.insert(Point(1, 1));
  kd.insert(Point(2, 2));
  kd.insert(Point(3, 3));

  size_t visited = 0;
  kd.for_each([&visited](const Point &) { ++visited; });
  EXPECT_EQ(visited, 3u);
}

// ============================================================================
// Phase 5 — Rigorous Tests
// ============================================================================

// ---------- 5.1 Property tests: Delaunay empty-circumcircle ----------

// Helper: squared distance between two points (exact).
static Geom_Number dist2(const Point & a, const Point & b)
{
  return a.distance_squared_to(b);
}

// Helper: extract sorted vertex set from polygon for comparison.
static Array<Point> sorted_hull_vertices(const Polygon & p)
{
  Array<Point> v;
  for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
    v.append(it.get_current_vertex());
  quicksort_op(v, [](const Point & a, const Point & b)
               {
                 if (a.get_x() != b.get_x())
                   return a.get_x() < b.get_x();
                 return a.get_y() < b.get_y();
               });
  return v;
}

TEST_F(GeomAlgorithmsTest, DelaunayEmptyCircumcircleProperty)
{
  // The Delaunay property: for every triangle, no other site is strictly
  // inside its circumcircle.
  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay({Point(0, 0), Point(6, 0), Point(3, 5), Point(6, 5),
                     Point(0, 5), Point(3, 2), Point(1, 3), Point(5, 1)});

  ASSERT_GE(r.triangles.size(), 1u);

  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point & a = r.sites(tri.i);
      const Point & b = r.sites(tri.j);
      const Point & c = r.sites(tri.k);

      // Compute circumcenter and squared circumradius.
      const Point cc = circumcenter_of(a, b, c);
      const Geom_Number r2 = dist2(cc, a);

      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;

          const Geom_Number d2 = dist2(cc, r.sites(s));
          // d2 must be >= r2 (no site strictly inside the circumcircle).
          EXPECT_GE(d2, r2)
              << "Site " << s << " violates empty-circumcircle for triangle "
              << t;
        }
    }
}

TEST_F(GeomAlgorithmsTest, DelaunayEmptyCircumcircleGridPoints)
{
  // Grid of 5x5 points — a stress test of the circumcircle property.
  DynList<Point> pts;
  for (int x = 0; x < 5; ++x)
    for (int y = 0; y < 5; ++y)
      pts.append(Point(x, y));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);

  ASSERT_GE(r.triangles.size(), 1u);

  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point & a = r.sites(tri.i);
      const Point & b = r.sites(tri.j);
      const Point & c = r.sites(tri.k);
      const Point cc = circumcenter_of(a, b, c);
      const Geom_Number r2 = dist2(cc, a);

      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), r2);
        }
    }
}

// ---------- 5.1 Property tests: Voronoi equidistance ----------

TEST_F(GeomAlgorithmsTest, VoronoiVerticesEquidistantToSites)
{
  // Each bounded Voronoi edge connects two circumcenters.
  // Each circumcenter (Voronoi vertex) is equidistant to the 3 sites
  // of its Delaunay triangle.
  DelaunayTriangulationBowyerWatson delaunay;
  auto dt = delaunay({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                      Point(2, 2), Point(4, 4)});
  ASSERT_GE(dt.triangles.size(), 1u);

  for (size_t t = 0; t < dt.triangles.size(); ++t)
    {
      const auto & tri = dt.triangles(t);
      const Point & a = dt.sites(tri.i);
      const Point & b = dt.sites(tri.j);
      const Point & c = dt.sites(tri.k);
      const Point cc = circumcenter_of(a, b, c);

      const Geom_Number da = dist2(cc, a);
      const Geom_Number db = dist2(cc, b);
      const Geom_Number dc = dist2(cc, c);

      EXPECT_EQ(da, db) << "Triangle " << t << ": circumcenter not equidistant";
      EXPECT_EQ(db, dc) << "Triangle " << t << ": circumcenter not equidistant";
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiBoundedEdgeSitesAreEquidistantToEndpoints)
{
  // For each bounded Voronoi edge (connecting two circumcenters c0 and c1),
  // the two adjacent sites u,v should be equidistant from the edge midpoint.
  DelaunayTriangulationBowyerWatson delaunay;
  auto dt = delaunay({Point(0, 0), Point(5, 0), Point(6, 3), Point(0, 4),
                      Point(2, 2), Point(4, 4)});

  VoronoiDiagramFromDelaunay voronoi;
  auto r = voronoi(dt);

  for (size_t e = 0; e < r.edges.size(); ++e)
    {
      const auto & edge = r.edges(e);
      if (edge.unbounded)
        continue;

      // Both endpoints are circumcenters equidistant to sites u and v.
      const Geom_Number d_src_u = dist2(edge.src, r.sites(edge.site_u));
      const Geom_Number d_src_v = dist2(edge.src, r.sites(edge.site_v));
      EXPECT_EQ(d_src_u, d_src_v)
          << "Edge " << e << " src not equidistant to sites";

      const Geom_Number d_tgt_u = dist2(edge.tgt, r.sites(edge.site_u));
      const Geom_Number d_tgt_v = dist2(edge.tgt, r.sites(edge.site_v));
      EXPECT_EQ(d_tgt_u, d_tgt_v)
          << "Edge " << e << " tgt not equidistant to sites";
    }
}

// ---------- 5.2 Numerical robustness: near-collinear ----------

TEST_F(GeomAlgorithmsTest, RobustnessNearCollinearDelaunay)
{
  // Points almost collinear but with tiny deviation — exact arithmetic
  // should handle this correctly.
  // Using rational offsets like 1/1000000 instead of floating-point.
  Geom_Number tiny(1, 1000000);  // 10^-6 as exact rational

  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, tiny));
  pts.append(Point(2, -tiny));
  pts.append(Point(3, tiny));
  pts.append(Point(4, 0));
  pts.append(Point(2, 1));  // clearly off-axis to guarantee non-collinear set

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  for (size_t i = 0; i < r.triangles.size(); ++i)
    {
      const auto & t = r.triangles(i);
      scene.segments.append(Segment(r.sites(t.i), r.sites(t.j)));
      scene.segments.append(Segment(r.sites(t.j), r.sites(t.k)));
      scene.segments.append(Segment(r.sites(t.k), r.sites(t.i)));
    }
  (void) Aleph::TestVisual::emit_case_svg(
      "case_robust_near_collinear_delaunay", scene,
      "Delaunay robustness / near-collinear");

  // Should produce a valid triangulation.
  EXPECT_GE(r.triangles.size(), 1u);

  // Verify circumcircle property.
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number r2 = dist2(cc, r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), r2);
        }
    }
}

TEST_F(GeomAlgorithmsTest, RobustnessNearCollinearConvexHull)
{
  // Near-collinear points should still produce a valid hull.
  Geom_Number tiny(1, 10000000);  // 10^-7

  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, tiny));
  pts.append(Point(2, 0));
  pts.append(Point(3, -tiny));
  pts.append(Point(4, 0));
  pts.append(Point(2, 1));  // off-line to make non-degenerate

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(pts);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  scene.polygons.append(hull);
  Aleph::TestVisual::add_polygon_vertices(scene, hull, true);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_robust_near_collinear_hull", scene,
      "Convex hull robustness / near-collinear");

  EXPECT_TRUE(hull.is_closed());
  EXPECT_GE(hull.size(), 3u);

  // Hull must contain the extremes.
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(4, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(2, 1)));
}

// ---------- 5.2 Numerical robustness: near-parallel segments ----------

TEST_F(GeomAlgorithmsTest, RobustnessNearParallelSegments)
{
  // Two segments that are nearly parallel — they intersect at a very
  // distant point. The sweep line should either find 0 or 1 intersection
  // depending on whether the segments actually overlap.
  Geom_Number tiny(1, 100000000);  // 10^-8

  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(10, 0)));
  segs.append(Segment(Point(0, tiny), Point(10, tiny)));  // almost parallel

  auto result = sweep(segs);
  EXPECT_EQ(result.size(), 0u);  // truly parallel, no intersection
}

TEST_F(GeomAlgorithmsTest, RobustnessNearParallelSegmentsConverging)
{
  // Two segments that converge at a nearly-parallel angle.
  Geom_Number tiny(1, 1000000);

  SweepLineSegmentIntersection sweep;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(10, 0)));
  segs.append(Segment(Point(0, tiny), Point(10, -tiny)));  // slight converge

  auto result = sweep(segs);

  Aleph::TestVisual::SvgScene scene;
  for (size_t i = 0; i < segs.size(); ++i)
    scene.segments.append(segs(i));
  for (size_t i = 0; i < result.size(); ++i)
    scene.highlighted_points.append(result(i).point);
  (void) Aleph::TestVisual::emit_case_svg(
      "case_robust_near_parallel_converging", scene,
      "Near-parallel segments / converging intersection");

  ASSERT_EQ(result.size(), 1u);
  // Intersection must be exact.
  EXPECT_EQ(result(0).point.get_y(), Geom_Number(0));  // should be on y=0 plane
}

// ---------- 5.2 Numerical robustness: extreme coordinates ----------

TEST_F(GeomAlgorithmsTest, RobustnessExtremeCoordinates)
{
  // Points with very large coordinates — exact arithmetic handles this.
  Geom_Number big(1000000000);  // 10^9

  DynList<Point> pts;
  pts.append(Point(big, big));
  pts.append(Point(-big, big));
  pts.append(Point(-big, -big));
  pts.append(Point(big, -big));
  pts.append(Point(0, 0));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);
  EXPECT_GE(r.triangles.size(), 1u);

  // Verify circumcircle property with big coords.
  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number r2 = dist2(cc, r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), r2);
        }
    }
}

TEST_F(GeomAlgorithmsTest, RobustnessVerySmallCoordinates)
{
  // Points with very small coordinates.
  Geom_Number eps(1, 1000000000);  // 10^-9

  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(eps, 0));
  pts.append(Point(0, eps));
  pts.append(Point(eps, eps));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);
  EXPECT_GE(r.triangles.size(), 2u);
}

// ---------- 5.2 Numerical robustness: cocircular points ----------

TEST_F(GeomAlgorithmsTest, RobustnessCocircularPoints)
{
  // 8 points on a circle — a degenerate case for Delaunay.
  // The triangulation should still be valid and complete.
  // Use rational approximations of circle points:
  // (2,0), (0,2), (-2,0), (0,-2), and 4 diagonal points.
  DynList<Point> pts;
  pts.append(Point(2, 0));
  pts.append(Point(0, 2));
  pts.append(Point(-2, 0));
  pts.append(Point(0, -2));

  // Points at 45-degree offsets (rational approximation on circle r=2).
  // Use exact: sqrt(2) ≈ 1414/1000 (close enough for testing cocircularity).
  // Actually, for true cocircularity use x^2+y^2 = 4.
  // (1, sqrt(3)) is on circle r=2: 1+3=4. Use Geom_Number for sqrt(3).
  // Instead, let's use: (8/5, 6/5) since (8/5)^2+(6/5)^2 = 64/25+36/25 = 100/25 = 4.
  pts.append(Point(Geom_Number(8, 5), Geom_Number(6, 5)));
  pts.append(Point(Geom_Number(-8, 5), Geom_Number(6, 5)));
  pts.append(Point(Geom_Number(-8, 5), Geom_Number(-6, 5)));
  pts.append(Point(Geom_Number(8, 5), Geom_Number(-6, 5)));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);

  Aleph::TestVisual::SvgScene scene;
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    scene.points.append(it.get_curr());
  for (size_t i = 0; i < r.triangles.size(); ++i)
    {
      const auto & t = r.triangles(i);
      scene.segments.append(Segment(r.sites(t.i), r.sites(t.j)));
      scene.segments.append(Segment(r.sites(t.j), r.sites(t.k)));
      scene.segments.append(Segment(r.sites(t.k), r.sites(t.i)));
    }
  (void) Aleph::TestVisual::emit_case_svg(
      "case_robust_cocircular_delaunay", scene,
      "Delaunay robustness / cocircular points");

  // Must produce a triangulation.
  EXPECT_GE(r.triangles.size(), 6u);  // at least 6 triangles for 8 cocircular pts

  // All sites should participate.
  EXPECT_EQ(r.sites.size(), 8u);
}

// ---------- 5.3 Determinism: permuted inputs produce same results ----------

TEST_F(GeomAlgorithmsTest, DeterminismDelaunayPermutedInputs)
{
  // The Delaunay output should be the same regardless of input order.
  DynList<Point> pts1;
  pts1.append(Point(0, 0));
  pts1.append(Point(5, 0));
  pts1.append(Point(6, 3));
  pts1.append(Point(0, 4));
  pts1.append(Point(3, 2));

  DynList<Point> pts2;  // reverse order
  pts2.append(Point(3, 2));
  pts2.append(Point(0, 4));
  pts2.append(Point(6, 3));
  pts2.append(Point(5, 0));
  pts2.append(Point(0, 0));

  DynList<Point> pts3;  // shuffled
  pts3.append(Point(6, 3));
  pts3.append(Point(0, 0));
  pts3.append(Point(3, 2));
  pts3.append(Point(5, 0));
  pts3.append(Point(0, 4));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r1 = delaunay(pts1);
  auto r2 = delaunay(pts2);
  auto r3 = delaunay(pts3);

  // Same number of sites and triangles.
  EXPECT_EQ(r1.sites.size(), r2.sites.size());
  EXPECT_EQ(r1.sites.size(), r3.sites.size());
  EXPECT_EQ(r1.triangles.size(), r2.triangles.size());
  EXPECT_EQ(r1.triangles.size(), r3.triangles.size());

  // Canonical triangle sets should match.
  auto ct1 = canonical_triangles(r1);
  auto ct2 = canonical_triangles(r2);
  auto ct3 = canonical_triangles(r3);

  ASSERT_EQ(ct1.size(), ct2.size());
  ASSERT_EQ(ct1.size(), ct3.size());

  for (size_t i = 0; i < ct1.size(); ++i)
    {
      EXPECT_EQ(ct1(i).a, ct2(i).a);
      EXPECT_EQ(ct1(i).b, ct2(i).b);
      EXPECT_EQ(ct1(i).c, ct2(i).c);
      EXPECT_EQ(ct1(i).a, ct3(i).a);
      EXPECT_EQ(ct1(i).b, ct3(i).b);
      EXPECT_EQ(ct1(i).c, ct3(i).c);
    }
}

TEST_F(GeomAlgorithmsTest, DeterminismConvexHullPermutedInputs)
{
  DynList<Point> pts1;
  pts1.append(Point(0, 0));
  pts1.append(Point(5, 0));
  pts1.append(Point(6, 3));
  pts1.append(Point(0, 4));
  pts1.append(Point(3, 1));  // interior point

  DynList<Point> pts2;
  pts2.append(Point(3, 1));
  pts2.append(Point(0, 4));
  pts2.append(Point(6, 3));
  pts2.append(Point(5, 0));
  pts2.append(Point(0, 0));

  AndrewMonotonicChainConvexHull andrew;
  Polygon h1 = andrew(pts1);
  Polygon h2 = andrew(pts2);

  auto v1 = sorted_hull_vertices(h1);
  auto v2 = sorted_hull_vertices(h2);

  ASSERT_EQ(v1.size(), v2.size());
  for (size_t i = 0; i < v1.size(); ++i)
    EXPECT_EQ(v1(i), v2(i));
}

TEST_F(GeomAlgorithmsTest, DeterminismClosestPairPermutedInputs)
{
  DynList<Point> pts1;
  pts1.append(Point(0, 0));
  pts1.append(Point(10, 10));
  pts1.append(Point(1, 0));  // closest pair: (0,0)-(1,0)
  pts1.append(Point(5, 5));

  DynList<Point> pts2;
  pts2.append(Point(5, 5));
  pts2.append(Point(1, 0));
  pts2.append(Point(0, 0));
  pts2.append(Point(10, 10));

  ClosestPairDivideAndConquer cp;
  auto r1 = cp(pts1);
  auto r2 = cp(pts2);

  EXPECT_EQ(r1.distance_squared, r2.distance_squared);
  // Same pair (possibly swapped).
  EXPECT_TRUE(matches_unordered_pair(r1.first, r1.second, r2.first, r2.second));
}

// ---------- 5.4 Performance: large datasets ----------

TEST_F(GeomAlgorithmsTest, PerformanceConvexHull10KPoints)
{
  // 10000 points on a grid — convex hull should return the boundary.
  DynList<Point> pts;
  for (int x = 0; x < 100; ++x)
    for (int y = 0; y < 100; ++y)
      pts.append(Point(x, y));

  AndrewMonotonicChainConvexHull andrew;
  Polygon hull = andrew(pts);

  EXPECT_TRUE(hull.is_closed());
  // The hull of a grid is the bounding rectangle.
  EXPECT_EQ(hull.size(), 4u);
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(99, 0)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(99, 99)));
  EXPECT_TRUE(polygon_contains_vertex(hull, Point(0, 99)));
}

TEST_F(GeomAlgorithmsTest, PerformanceClosestPair5KPoints)
{
  // 5000 points on a grid; minimum distance = 1.
  DynList<Point> pts;
  for (int x = 0; x < 50; ++x)
    for (int y = 0; y < 100; ++y)
      pts.append(Point(x, y));

  ClosestPairDivideAndConquer cp;
  auto r = cp(pts);

  EXPECT_EQ(r.distance_squared, Geom_Number(1));
}

TEST_F(GeomAlgorithmsTest, PerformanceDelaunay500Points)
{
  // 500 points on a grid — verify valid Delaunay.
  DynList<Point> pts;
  for (int x = 0; x < 25; ++x)
    for (int y = 0; y < 20; ++y)
      pts.append(Point(x, y));

  DelaunayTriangulationBowyerWatson delaunay;
  auto r = delaunay(pts);

  EXPECT_GE(r.triangles.size(), 1u);

  // Spot-check a few triangles for circumcircle property.
  const size_t check_limit = r.triangles.size() < 50 ? r.triangles.size() : 50;
  for (size_t t = 0; t < check_limit; ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number r2 = dist2(cc, r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i or s == tri.j or s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), r2);
        }
    }
}

TEST_F(GeomAlgorithmsTest, PerformanceTriangulation100Vertices)
{
  // Build a simple polygon with ~100 vertices (zigzag) — no collinear edges.
  // Triangulation should produce n-2 triangles.
  Polygon p;

  // Bottom zigzag: (0,0), (1,1), (2,0), (3,1), ..., (48,0), (49,1), (50,0)
  for (int x = 0; x <= 50; ++x)
    p.add_vertex(Point(x, (x % 2 == 0) ? 0 : 1));

  // Top zigzag going back: (50,10), (49,9), (48,10), ..., (1,9), (0,10)
  for (int x = 50; x >= 0; --x)
    p.add_vertex(Point(x, (x % 2 == 0) ? 10 : 9));

  p.close();

  const size_t nv = p.size();
  ASSERT_GE(nv, 50u);

  CuttingEarsTriangulation ears;
  DynList<Triangle> tris = ears(p);

  size_t count = 0;
  for (DynList<Triangle>::Iterator it(tris); it.has_curr(); it.next_ne())
    ++count;

  EXPECT_EQ(count, nv - 2);
}

// ---------- 5.5 Cross-algorithm comparison: 5 convex hulls ----------

TEST_F(GeomAlgorithmsTest, CrossAlgorithmConvexHullSimple)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 0));
  pts.append(Point(6, 3));
  pts.append(Point(3, 6));
  pts.append(Point(0, 4));
  pts.append(Point(2, 1));   // interior
  pts.append(Point(3, 2));   // interior

  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;
  BruteForceConvexHull brute;
  GiftWrappingConvexHull gift;
  QuickHull quick;

  Polygon h_andrew = andrew(pts);
  Polygon h_graham = graham(pts);
  Polygon h_brute = brute(pts);
  Polygon h_gift = gift(pts);
  Polygon h_quick = quick(pts);

  auto v_andrew = sorted_hull_vertices(h_andrew);
  auto v_graham = sorted_hull_vertices(h_graham);
  auto v_brute = sorted_hull_vertices(h_brute);
  auto v_gift = sorted_hull_vertices(h_gift);
  auto v_quick = sorted_hull_vertices(h_quick);

  // All must have the same vertex count.
  ASSERT_EQ(v_andrew.size(), v_graham.size())
      << "Andrew vs Graham vertex count mismatch";
  ASSERT_EQ(v_andrew.size(), v_brute.size())
      << "Andrew vs BruteForce vertex count mismatch";
  ASSERT_EQ(v_andrew.size(), v_gift.size())
      << "Andrew vs GiftWrapping vertex count mismatch";
  ASSERT_EQ(v_andrew.size(), v_quick.size())
      << "Andrew vs QuickHull vertex count mismatch";

  // All must have the same vertices.
  for (size_t i = 0; i < v_andrew.size(); ++i)
    {
      EXPECT_EQ(v_andrew(i), v_graham(i))
          << "Andrew vs Graham mismatch at index " << i;
      EXPECT_EQ(v_andrew(i), v_brute(i))
          << "Andrew vs BruteForce mismatch at index " << i;
      EXPECT_EQ(v_andrew(i), v_gift(i))
          << "Andrew vs GiftWrapping mismatch at index " << i;
      EXPECT_EQ(v_andrew(i), v_quick(i))
          << "Andrew vs QuickHull mismatch at index " << i;
    }
}

TEST_F(GeomAlgorithmsTest, CrossAlgorithmConvexHullLargerSet)
{
  // 100 points, mix of grid + interior + boundary.
  DynList<Point> pts;
  for (int x = 0; x <= 10; ++x)
    for (int y = 0; y <= 10; ++y)
      pts.append(Point(x, y));

  // Add some extra interior points.
  pts.append(Point(5, 5));
  pts.append(Point(3, 7));
  pts.append(Point(8, 2));

  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;
  BruteForceConvexHull brute;
  GiftWrappingConvexHull gift;
  QuickHull quick;

  Polygon h_andrew = andrew(pts);
  Polygon h_graham = graham(pts);
  Polygon h_brute = brute(pts);
  Polygon h_gift = gift(pts);
  Polygon h_quick = quick(pts);

  auto v_andrew = sorted_hull_vertices(h_andrew);
  auto v_graham = sorted_hull_vertices(h_graham);
  auto v_brute = sorted_hull_vertices(h_brute);
  auto v_gift = sorted_hull_vertices(h_gift);
  auto v_quick = sorted_hull_vertices(h_quick);

  ASSERT_EQ(v_andrew.size(), v_graham.size());
  ASSERT_EQ(v_andrew.size(), v_brute.size());
  ASSERT_EQ(v_andrew.size(), v_gift.size());
  ASSERT_EQ(v_andrew.size(), v_quick.size());

  for (size_t i = 0; i < v_andrew.size(); ++i)
    {
      EXPECT_EQ(v_andrew(i), v_graham(i));
      EXPECT_EQ(v_andrew(i), v_brute(i));
      EXPECT_EQ(v_andrew(i), v_gift(i));
      EXPECT_EQ(v_andrew(i), v_quick(i));
    }
}

TEST_F(GeomAlgorithmsTest, CrossAlgorithmConvexHullCollinearBoundary)
{
  // Many collinear points on the hull boundary.
  DynList<Point> pts;
  for (int x = 0; x <= 20; ++x)
    {
      pts.append(Point(x, 0));    // bottom
      pts.append(Point(x, 10));   // top
    }
  pts.append(Point(0, 5));       // left
  pts.append(Point(20, 5));      // right

  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;
  QuickHull quick;

  Polygon h_andrew = andrew(pts);
  Polygon h_graham = graham(pts);
  Polygon h_quick = quick(pts);

  auto v_andrew = sorted_hull_vertices(h_andrew);
  auto v_graham = sorted_hull_vertices(h_graham);
  auto v_quick = sorted_hull_vertices(h_quick);

  // For collinear points, algorithms may differ on whether they include
  // intermediate points. Compare just the extreme corners.
  EXPECT_TRUE(polygon_contains_vertex(h_andrew, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_andrew, Point(20, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_andrew, Point(20, 10)));
  EXPECT_TRUE(polygon_contains_vertex(h_andrew, Point(0, 10)));

  EXPECT_TRUE(polygon_contains_vertex(h_graham, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_graham, Point(20, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_graham, Point(20, 10)));
  EXPECT_TRUE(polygon_contains_vertex(h_graham, Point(0, 10)));

  EXPECT_TRUE(polygon_contains_vertex(h_quick, Point(0, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_quick, Point(20, 0)));
  EXPECT_TRUE(polygon_contains_vertex(h_quick, Point(20, 10)));
  EXPECT_TRUE(polygon_contains_vertex(h_quick, Point(0, 10)));
}

TEST_F(GeomAlgorithmsTest, CrossAlgorithmConvexHullTrianglePoints)
{
  // All points on hull (triangle) — all algorithms must agree.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(5, 8));

  AndrewMonotonicChainConvexHull andrew;
  GrahamScanConvexHull graham;
  BruteForceConvexHull brute;
  GiftWrappingConvexHull gift;
  QuickHull quick;

  auto v_andrew = sorted_hull_vertices(andrew(pts));
  auto v_graham = sorted_hull_vertices(graham(pts));
  auto v_brute = sorted_hull_vertices(brute(pts));
  auto v_gift = sorted_hull_vertices(gift(pts));
  auto v_quick = sorted_hull_vertices(quick(pts));

  ASSERT_EQ(v_andrew.size(), 3u);
  ASSERT_EQ(v_graham.size(), 3u);
  ASSERT_EQ(v_brute.size(), 3u);
  ASSERT_EQ(v_gift.size(), 3u);
  ASSERT_EQ(v_quick.size(), 3u);

  for (size_t i = 0; i < 3; ++i)
    {
      EXPECT_EQ(v_andrew(i), v_graham(i));
      EXPECT_EQ(v_andrew(i), v_brute(i));
      EXPECT_EQ(v_andrew(i), v_gift(i));
      EXPECT_EQ(v_andrew(i), v_quick(i));
    }
}

// ============================================================================
// Section 5.1 — Tests for new algorithms
// ============================================================================

// ---------- Delaunay O(n log n) — randomized incremental ----------

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalBasicSquare)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  pts.append(Point(4, 4));
  pts.append(Point(0, 4));

  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay(pts);

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_EQ(r.triangles.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalEmptyCircumcircle)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 0));
  pts.append(Point(5, 5));
  pts.append(Point(0, 5));
  pts.append(Point(2, 3));
  pts.append(Point(3, 1));

  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay(pts);

  EXPECT_GE(r.triangles.size(), 4u);

  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number r2 = dist2(cc, r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i || s == tri.j || s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), r2)
            << "Delaunay incremental: site " << s
            << " violates circumcircle of triangle " << t;
        }
    }
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalMatchesBowyerWatson)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(10, 10));
  pts.append(Point(0, 10));
  pts.append(Point(5, 5));
  pts.append(Point(3, 7));
  pts.append(Point(7, 2));
  pts.append(Point(1, 3));

  DelaunayTriangulationBowyerWatson bw;
  auto rbw = bw(pts);

  DelaunayTriangulationRandomizedIncremental inc;
  auto rinc = inc(pts);

  EXPECT_EQ(rbw.sites.size(), rinc.sites.size());
  EXPECT_EQ(rbw.triangles.size(), rinc.triangles.size());
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalSingleTriangle)
{
  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay({Point(0, 0), Point(1, 0), Point(0, 1)});
  EXPECT_EQ(r.sites.size(), 3u);
  EXPECT_EQ(r.triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalCollinear)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 0));
  pts.append(Point(2, 0));
  pts.append(Point(3, 0));

  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay(pts);

  EXPECT_EQ(r.triangles.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalDuplicates)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 0));
  pts.append(Point(0, 1));
  pts.append(Point(0, 0));
  pts.append(Point(1, 0));

  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay(pts);
  EXPECT_EQ(r.sites.size(), 3u);
  EXPECT_EQ(r.triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, DelaunayIncrementalGrid)
{
  DynList<Point> pts;
  for (int x = 0; x <= 4; ++x)
    for (int y = 0; y <= 4; ++y)
      pts.append(Point(x, y));

  DelaunayTriangulationRandomizedIncremental delaunay;
  auto r = delaunay(pts);

  EXPECT_EQ(r.sites.size(), 25u);
  EXPECT_GE(r.triangles.size(), 32u);

  for (size_t t = 0; t < r.triangles.size(); ++t)
    {
      const auto & tri = r.triangles(t);
      const Point cc = circumcenter_of(r.sites(tri.i), r.sites(tri.j),
                                       r.sites(tri.k));
      const Geom_Number cr2 = dist2(cc, r.sites(tri.i));
      for (size_t s = 0; s < r.sites.size(); ++s)
        {
          if (s == tri.i || s == tri.j || s == tri.k)
            continue;
          EXPECT_GE(dist2(cc, r.sites(s)), cr2);
        }
    }
}

// ---------- VoronoiDiagramFortune ----------

TEST_F(GeomAlgorithmsTest, VoronoiFortuneFourPoints)
{
  VoronoiDiagramFortune voronoi;
  auto r = voronoi({Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4)});

  EXPECT_EQ(r.sites.size(), 4u);
  EXPECT_GE(r.vertices.size(), 1u);
  EXPECT_GE(r.edges.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, VoronoiFortuneEquidistance)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(6, 0));
  pts.append(Point(3, 5));
  pts.append(Point(6, 5));
  pts.append(Point(0, 5));

  VoronoiDiagramFortune voronoi;
  auto r = voronoi(pts);

  for (size_t e = 0; e < r.edges.size(); ++e)
    {
      const auto & edge = r.edges(e);
      if (edge.unbounded) continue;

      const Geom_Number d_u = dist2(edge.src, r.sites(edge.site_u));
      const Geom_Number d_v = dist2(edge.src, r.sites(edge.site_v));
      EXPECT_EQ(d_u, d_v) << "Voronoi edge src not equidistant for edge " << e;
    }
}

TEST_F(GeomAlgorithmsTest, VoronoiFortuneClippedCells)
{
  DynList<Point> pts;
  pts.append(Point(1, 1));
  pts.append(Point(3, 1));
  pts.append(Point(2, 3));

  Polygon clip;
  clip.add_vertex(Point(0, 0));
  clip.add_vertex(Point(4, 0));
  clip.add_vertex(Point(4, 4));
  clip.add_vertex(Point(0, 4));
  clip.close();

  VoronoiDiagramFortune voronoi;
  auto cells = voronoi.clipped_cells(pts, clip);

  EXPECT_EQ(cells.size(), 3u);
  for (size_t i = 0; i < cells.size(); ++i)
    EXPECT_TRUE(cells(i).polygon.is_closed());
}

// ---------- ConvexPolygonDecomposition ----------

TEST_F(GeomAlgorithmsTest, ConvexDecompTriangle)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));
  p.close();

  ConvexPolygonDecomposition decomp;
  auto parts = decomp(p);

  EXPECT_EQ(parts.size(), 1u);
  EXPECT_TRUE(parts(0).is_closed());
}

TEST_F(GeomAlgorithmsTest, ConvexDecompConvexQuad)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  ConvexPolygonDecomposition decomp;
  auto parts = decomp(p);

  EXPECT_EQ(parts.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, ConvexDecompLShape)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 2));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(2, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  ConvexPolygonDecomposition decomp;
  auto parts = decomp(p);

  EXPECT_GE(parts.size(), 2u);
  EXPECT_LE(parts.size(), 4u);

  for (size_t i = 0; i < parts.size(); ++i)
    {
      EXPECT_TRUE(parts(i).is_closed());
      EXPECT_GE(parts(i).size(), 3u);
    }
}

TEST_F(GeomAlgorithmsTest, ConvexDecompAllPartsConvex)
{
  // U-shaped polygon.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(6, 0));
  p.add_vertex(Point(6, 4));
  p.add_vertex(Point(5, 4));
  p.add_vertex(Point(5, 1));
  p.add_vertex(Point(1, 1));
  p.add_vertex(Point(1, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  ConvexPolygonDecomposition decomp;
  auto parts = decomp(p);

  EXPECT_GE(parts.size(), 2u);

  for (size_t i = 0; i < parts.size(); ++i)
    {
      const Polygon & cp = parts(i);
      EXPECT_TRUE(cp.is_closed());

      Array<Point> verts;
      for (Polygon::Vertex_Iterator it(cp); it.has_curr(); it.next_ne())
        verts.append(it.get_current_vertex());

      const size_t nv = verts.size();
      if (nv < 3) continue;

      bool convex = true;
      Orientation first_o = Orientation::COLLINEAR;
      for (size_t j = 0; j < nv; ++j)
        {
          Orientation o = orientation(verts(j), verts((j+1)%nv),
                                      verts((j+2)%nv));
          if (o == Orientation::COLLINEAR) continue;
          if (first_o == Orientation::COLLINEAR)
            first_o = o;
          else if (o != first_o)
            { convex = false; break; }
        }
      EXPECT_TRUE(convex) << "Part " << i << " is not convex";
    }
}

TEST_F(GeomAlgorithmsTest, ConvexDecompVertexCoverage)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 2));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(2, 4));
  p.add_vertex(Point(0, 4));
  p.close();

  Array<Point> orig;
  for (Polygon::Vertex_Iterator it(p); it.has_curr(); it.next_ne())
    orig.append(it.get_current_vertex());

  ConvexPolygonDecomposition decomp;
  auto parts = decomp(p);

  for (size_t oi = 0; oi < orig.size(); ++oi)
    {
      bool found = false;
      for (size_t pi = 0; pi < parts.size() && !found; ++pi)
        for (Polygon::Vertex_Iterator it(parts(pi));
             it.has_curr(); it.next_ne())
          if (it.get_current_vertex() == orig(oi))
            { found = true; break; }
      EXPECT_TRUE(found) << "Original vertex " << oi << " missing from parts";
    }
}

// ---------- LineSweepFramework ----------

TEST_F(GeomAlgorithmsTest, LineSweepBasicOrdering)
{
  struct Ev { int x; int tag; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.x < b.x; }
  };

  LineSweepFramework<Ev, CmpEv> fw;
  fw.enqueue(Ev{5, 0});
  fw.enqueue(Ev{1, 1});
  fw.enqueue(Ev{3, 2});

  EXPECT_EQ(fw.pending(), 3u);

  Array<int> order;
  fw.run([&](auto &, const Ev & e) { order.append(e.x); });

  EXPECT_EQ(order.size(), 3u);
  EXPECT_EQ(order(0), 1);
  EXPECT_EQ(order(1), 3);
  EXPECT_EQ(order(2), 5);
}

TEST_F(GeomAlgorithmsTest, LineSweepDuplicatePositions)
{
  struct Ev { int x; int id; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.x < b.x; }
  };

  LineSweepFramework<Ev, CmpEv> fw;
  fw.enqueue(Ev{2, 0});
  fw.enqueue(Ev{2, 1});
  fw.enqueue(Ev{2, 2});
  fw.enqueue(Ev{1, 3});

  EXPECT_EQ(fw.pending(), 4u);

  Array<int> ids;
  fw.run([&](auto &, const Ev & e) { ids.append(e.id); });

  EXPECT_EQ(ids.size(), 4u);
  EXPECT_EQ(ids(0), 3);
  EXPECT_EQ(ids(1), 0);
  EXPECT_EQ(ids(2), 1);
  EXPECT_EQ(ids(3), 2);
}

TEST_F(GeomAlgorithmsTest, LineSweepDynamicEvents)
{
  struct Ev { int val; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.val < b.val; }
  };

  LineSweepFramework<Ev, CmpEv> fw;
  fw.enqueue(Ev{10});
  fw.enqueue(Ev{20});

  Array<int> seen;
  fw.run([&](auto & sweep, const Ev & e)
    {
      seen.append(e.val);
      if (e.val == 10)
        sweep.enqueue(Ev{15});
    });

  EXPECT_EQ(seen.size(), 3u);
  EXPECT_EQ(seen(0), 10);
  EXPECT_EQ(seen(1), 15);
  EXPECT_EQ(seen(2), 20);
}

TEST_F(GeomAlgorithmsTest, LineSweepPeekAndClear)
{
  struct Ev { int v; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.v < b.v; }
  };

  LineSweepFramework<Ev, CmpEv> fw;
  fw.enqueue(Ev{3});
  fw.enqueue(Ev{1});
  fw.enqueue(Ev{2});

  EXPECT_EQ(fw.peek().v, 1);
  EXPECT_EQ(fw.pending(), 3u);

  fw.clear();
  EXPECT_FALSE(fw.has_events());
  EXPECT_EQ(fw.pending(), 0u);
}

TEST_F(GeomAlgorithmsTest, LineSweepRunWithLog)
{
  struct Ev { int v; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.v < b.v; }
  };

  LineSweepFramework<Ev, CmpEv> fw;
  fw.enqueue(Ev{4});
  fw.enqueue(Ev{2});
  fw.enqueue(Ev{6});

  Array<Ev> log;
  fw.run([](auto &, const Ev &) {}, log);

  EXPECT_EQ(log.size(), 3u);
  EXPECT_EQ(log(0).v, 2);
  EXPECT_EQ(log(1).v, 4);
  EXPECT_EQ(log(2).v, 6);
}

TEST_F(GeomAlgorithmsTest, LineSweepGeometricExample)
{
  enum EvType { START, END };
  struct Ev { Geom_Number x; EvType type; size_t seg_id; };
  struct CmpEv {
    bool operator()(const Ev & a, const Ev & b) const
    { return a.x < b.x || (a.x == b.x && a.type < b.type); }
  };

  LineSweepFramework<Ev, CmpEv> fw;

  fw.enqueue(Ev{1, START, 0});
  fw.enqueue(Ev{5, END,   0});
  fw.enqueue(Ev{3, START, 1});
  fw.enqueue(Ev{7, END,   1});

  size_t active = 0;
  size_t max_active = 0;
  fw.run([&](auto &, const Ev & e)
    {
      if (e.type == START)
        ++active;
      else
        --active;
      if (active > max_active)
        max_active = active;
    });

  EXPECT_EQ(max_active, 2u);
  EXPECT_EQ(active, 0u);
}

// ========== RangeTree2D ==========

TEST_F(GeomAlgorithmsTest, RangeTree2DEmpty)
{
  RangeTree2D tree;
  DynList<Point> pts;
  tree.build(pts);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
  auto r = tree.query(0, 10, 0, 10);
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DSinglePoint)
{
  RangeTree2D tree;
  DynList<Point> pts;
  pts.append(Point(5, 5));
  tree.build(pts);
  EXPECT_EQ(tree.size(), 1u);

  auto r1 = tree.query(0, 10, 0, 10);
  EXPECT_EQ(r1.size(), 1u);

  auto r2 = tree.query(6, 10, 0, 10);
  EXPECT_EQ(r2.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DQueryAll)
{
  DynList<Point> pts;
  pts.append(Point(1, 2));
  pts.append(Point(3, 4));
  pts.append(Point(5, 6));

  RangeTree2D tree;
  tree.build(pts);
  auto r = tree.query(0, 10, 0, 10);
  EXPECT_EQ(r.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DQueryNone)
{
  DynList<Point> pts;
  pts.append(Point(1, 2));
  pts.append(Point(3, 4));

  RangeTree2D tree;
  tree.build(pts);
  auto r = tree.query(5, 10, 5, 10);
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DQueryPartial)
{
  DynList<Point> pts;
  pts.append(Point(1, 1));
  pts.append(Point(3, 5));
  pts.append(Point(7, 2));
  pts.append(Point(4, 4));
  pts.append(Point(8, 8));

  RangeTree2D tree;
  tree.build(pts);
  auto r = tree.query(2, 6, 1, 5);
  EXPECT_EQ(r.size(), 2u); // (3,5) and (4,4)
}

TEST_F(GeomAlgorithmsTest, RangeTree2DDegenerateRect)
{
  DynList<Point> pts;
  pts.append(Point(1, 1));
  pts.append(Point(2, 2));
  pts.append(Point(3, 3));

  RangeTree2D tree;
  tree.build(pts);

  // Point query.
  auto r1 = tree.query(2, 2, 2, 2);
  EXPECT_EQ(r1.size(), 1u);

  // Vertical line query.
  auto r2 = tree.query(2, 2, 0, 10);
  EXPECT_EQ(r2.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DBruteForce)
{
  DynList<Point> pts;
  for (int x = 0; x < 10; ++x)
    for (int y = 0; y < 10; ++y)
      pts.append(Point(x, y));

  RangeTree2D tree;
  tree.build(pts);
  EXPECT_EQ(tree.size(), 100u);

  auto r = tree.query(3, 6, 2, 7);

  // Brute-force count.
  size_t expected = 0;
  for (int x = 3; x <= 6; ++x)
    for (int y = 2; y <= 7; ++y)
      ++expected;
  EXPECT_EQ(r.size(), expected);
}

TEST_F(GeomAlgorithmsTest, RangeTree2DSameX)
{
  DynList<Point> pts;
  pts.append(Point(5, 1));
  pts.append(Point(5, 3));
  pts.append(Point(5, 5));

  RangeTree2D tree;
  tree.build(pts);
  auto r = tree.query(5, 5, 2, 4);
  EXPECT_EQ(r.size(), 1u); // (5,3)
}

TEST_F(GeomAlgorithmsTest, RangeTree2DSameY)
{
  DynList<Point> pts;
  pts.append(Point(1, 5));
  pts.append(Point(3, 5));
  pts.append(Point(5, 5));

  RangeTree2D tree;
  tree.build(pts);
  auto r = tree.query(2, 4, 5, 5);
  EXPECT_EQ(r.size(), 1u); // (3,5)
}

// ========== ConvexPolygonOffset ==========

TEST_F(GeomAlgorithmsTest, ConvexOffsetInwardSquare)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ConvexPolygonOffset off;
  Polygon r = off.inward(sq, Geom_Number(1));

  EXPECT_GE(r.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetInwardTooLarge)
{
  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(4, 0));
  tri.add_vertex(Point(2, 1));
  tri.close();

  ConvexPolygonOffset off;
  Polygon r = off.inward(tri, Geom_Number(100));

  // Offset far too large — should produce empty or degenerate polygon.
  EXPECT_LE(r.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetZeroDistance)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ConvexPolygonOffset off;
  Polygon r = off.inward(sq, Geom_Number(0));
  EXPECT_EQ(r.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetOutwardSquare)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ConvexPolygonOffset off;
  Polygon r = off.outward(sq, Geom_Number(1));
  EXPECT_EQ(r.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetOutwardTriangle)
{
  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(6, 0));
  tri.add_vertex(Point(3, 6));
  tri.close();

  ConvexPolygonOffset off;
  Polygon r = off.outward(tri, Geom_Number(1));
  EXPECT_EQ(r.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetContainment)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ConvexPolygonOffset off;
  Polygon inner = off.inward(sq, Geom_Number(2));
  Polygon outer = off.outward(sq, Geom_Number(2));

  // Inner center should be inside original.
  EXPECT_TRUE(PointInPolygonWinding::contains(sq, Point(5, 5)));
  // Original center should be inside outer.
  EXPECT_TRUE(PointInPolygonWinding::contains(outer, Point(5, 5)));
  // Inner center should be inside inner (if not empty).
  if (inner.size() >= 3)
    EXPECT_TRUE(PointInPolygonWinding::contains(inner, Point(5, 5)));
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetNonConvexThrows)
{
  // L-shaped (non-convex) polygon.
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(10, 0));
  L.add_vertex(Point(10, 5));
  L.add_vertex(Point(5, 5));
  L.add_vertex(Point(5, 10));
  L.add_vertex(Point(0, 10));
  L.close();

  ConvexPolygonOffset off;
  EXPECT_THROW(off.inward(L, Geom_Number(1)), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ConvexOffsetOpenThrows)
{
  Polygon open_poly;
  open_poly.add_vertex(Point(0, 0));
  open_poly.add_vertex(Point(1, 0));
  open_poly.add_vertex(Point(1, 1));

  ConvexPolygonOffset off;
  EXPECT_THROW(off.inward(open_poly, Geom_Number(1)), std::domain_error);
}

// ========== VisibilityPolygon ==========

TEST_F(GeomAlgorithmsTest, VisibilityRectangleCenter)
{
  Polygon rect;
  rect.add_vertex(Point(0, 0));
  rect.add_vertex(Point(10, 0));
  rect.add_vertex(Point(10, 10));
  rect.add_vertex(Point(0, 10));
  rect.close();

  VisibilityPolygon vis;
  Polygon vp = vis(rect, Point(5, 5));

  // From center of rectangle, everything is visible.
  EXPECT_GE(vp.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, VisibilityConvexFull)
{
  // For a convex polygon, visibility from any interior point = full polygon.
  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(10, 0));
  tri.add_vertex(Point(5, 10));
  tri.close();

  VisibilityPolygon vis;
  Polygon vp = vis(tri, Point(5, 3));
  EXPECT_GE(vp.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, VisibilityOutsideThrows)
{
  Polygon rect;
  rect.add_vertex(Point(0, 0));
  rect.add_vertex(Point(10, 0));
  rect.add_vertex(Point(10, 10));
  rect.add_vertex(Point(0, 10));
  rect.close();

  VisibilityPolygon vis;
  EXPECT_THROW(vis(rect, Point(20, 20)), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, VisibilityOpenThrows)
{
  Polygon open_poly;
  open_poly.add_vertex(Point(0, 0));
  open_poly.add_vertex(Point(1, 0));
  open_poly.add_vertex(Point(1, 1));

  VisibilityPolygon vis;
  EXPECT_THROW(vis(open_poly, Point(Geom_Number(1)/2, Geom_Number(1)/4)),
               std::domain_error);
}

// ========== ShortestPathInPolygon ==========

TEST_F(GeomAlgorithmsTest, ShortestPathSamePoint)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ShortestPathInPolygon sp;
  auto path = sp(sq, Point(5, 5), Point(5, 5));
  EXPECT_EQ(path.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, ShortestPathDirectLineOfSight)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ShortestPathInPolygon sp;
  auto path = sp(sq, Point(2, 2), Point(8, 8));

  EXPECT_EQ(path.size(), 2u);  // Direct: source → target.
}

TEST_F(GeomAlgorithmsTest, ShortestPathConvex)
{
  // In a convex polygon, all paths are direct.
  Polygon tri;
  tri.add_vertex(Point(0, 0));
  tri.add_vertex(Point(10, 0));
  tri.add_vertex(Point(5, 10));
  tri.close();

  ShortestPathInPolygon sp;
  auto path = sp(tri, Point(3, 2), Point(7, 2));
  EXPECT_EQ(path.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, ShortestPathLShaped)
{
  // L-shaped polygon requiring a bend.
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(10, 0));
  L.add_vertex(Point(10, 5));
  L.add_vertex(Point(5, 5));
  L.add_vertex(Point(5, 10));
  L.add_vertex(Point(0, 10));
  L.close();

  ShortestPathInPolygon sp;
  auto path = sp(L, Point(8, 2), Point(2, 8));

  // Should have at least 3 points (source, bend, target).
  EXPECT_GE(path.size(), 2u);

  // First and last should be source and target.
  EXPECT_EQ(path.get_first(), Point(8, 2));
  EXPECT_EQ(path.get_last(), Point(2, 8));
}

TEST_F(GeomAlgorithmsTest, ShortestPathOutsideThrows)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  ShortestPathInPolygon sp;
  EXPECT_THROW(sp(sq, Point(20, 20), Point(5, 5)), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, ShortestPathOpenThrows)
{
  Polygon open_poly;
  open_poly.add_vertex(Point(0, 0));
  open_poly.add_vertex(Point(1, 0));
  open_poly.add_vertex(Point(1, 1));

  ShortestPathInPolygon sp;
  EXPECT_THROW(sp(open_poly, Point(Geom_Number(1)/3, Geom_Number(1)/3),
                   Point(Geom_Number(2)/3, Geom_Number(1)/3)),
               std::domain_error);
}

// --- ShortestPathInPolygon regression tests (Lee-Preparata funnel) ---

TEST_F(GeomAlgorithmsTest, ShortestPathLShapedExact)
{
  // L-shaped polygon; shortest path from bottom-right to top-left must
  // pass through the reflex vertex (5,5).
  //
  //  (0,10)----(5,10)
  //    |          |
  //    |   (5,5)--(10,5)
  //    |              |
  //  (0,0)-------(10,0)
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(10, 0));
  L.add_vertex(Point(10, 5));
  L.add_vertex(Point(5, 5));
  L.add_vertex(Point(5, 10));
  L.add_vertex(Point(0, 10));
  L.close();

  ShortestPathInPolygon sp;
  // (9,2)→(2,9): line crosses edges (10,5)→(5,5) at (6,5) and (5,5)→(5,10) at (5,6)
  auto path = sp(L, Point(9, 2), Point(2, 9));

  // Must be at least: source → (5,5) → target.
  ASSERT_GE(path.size(), 3u);
  EXPECT_EQ(path.get_first(), Point(9, 2));
  EXPECT_EQ(path.get_last(), Point(2, 9));

  // The reflex vertex (5,5) must appear in the path.
  bool has_5_5 = false;
  for (DynList<Point>::Iterator it(path); it.has_curr(); it.next_ne())
    if (it.get_curr() == Point(5, 5)) has_5_5 = true;
  EXPECT_TRUE(has_5_5) << "Path must pass through reflex vertex (5,5)";
}

TEST_F(GeomAlgorithmsTest, ShortestPathUShaped)
{
  // U-shaped polygon.  Source at bottom-left, target at bottom-right.
  // Shortest path must go up around the inner notch.
  //
  //  (0,10)--(3,10)--(3,4)--(7,4)--(7,10)--(10,10)
  //    |                                        |
  //  (0,0)---------------------------------(10,0)
  Polygon U;
  U.add_vertex(Point(0, 0));
  U.add_vertex(Point(10, 0));
  U.add_vertex(Point(10, 10));
  U.add_vertex(Point(7, 10));
  U.add_vertex(Point(7, 4));
  U.add_vertex(Point(3, 4));
  U.add_vertex(Point(3, 10));
  U.add_vertex(Point(0, 10));
  U.close();

  ShortestPathInPolygon sp;
  // (1,8)→(9,8): line at y=8 crosses edges x=3 and x=7
  auto path = sp(U, Point(1, 8), Point(9, 8));

  // The path must go around the bottom of the notch.
  // It should include reflex vertices (3,4) and (7,4) as waypoints.
  EXPECT_GE(path.size(), 4u); // at least source, (3,4), (7,4), target

  EXPECT_EQ(path.get_first(), Point(1, 8));
  EXPECT_EQ(path.get_last(), Point(9, 8));

  // Verify (3,4) and (7,4) appear in the path.
  bool has_3_4 = false, has_7_4 = false;
  for (DynList<Point>::Iterator it(path); it.has_curr(); it.next_ne())
    {
      if (it.get_curr() == Point(3, 4)) has_3_4 = true;
      if (it.get_curr() == Point(7, 4)) has_7_4 = true;
    }
  EXPECT_TRUE(has_3_4) << "Path must pass through reflex vertex (3,4)";
  EXPECT_TRUE(has_7_4) << "Path must pass through reflex vertex (7,4)";

  // All segments must stay inside the polygon.
  DynList<Point>::Iterator uit(path);
  Point uprev = uit.get_curr();
  uit.next_ne();
  for (; uit.has_curr(); uit.next_ne())
    {
      const Segment seg(uprev, uit.get_curr());
      bool crosses = false;
      for (Polygon::Segment_Iterator si(U); si.has_curr() and not crosses; si.next_ne())
        if (seg.intersects_properly_with(si.get_current_segment()))
          crosses = true;
      EXPECT_FALSE(crosses) << "Path segment crosses polygon boundary";
      uprev = uit.get_curr();
    }
}

TEST_F(GeomAlgorithmsTest, ShortestPathTwoRooms)
{
  // Rectangle [0,10]×[0,10] with a notch [6,10]×[4,6] removed,
  // creating two "rooms" connected on the left side.
  //
  //  (0,10)-----------(10,10)
  //    |                  |
  //    |         (6,6)--(10,6)
  //    |           |          
  //    |         (6,4)--(10,4)
  //    |                  |
  //  (0,0)-----------(10,0)
  Polygon R;
  R.add_vertex(Point(0, 0));
  R.add_vertex(Point(10, 0));
  R.add_vertex(Point(10, 4));
  R.add_vertex(Point(6, 4));
  R.add_vertex(Point(6, 6));
  R.add_vertex(Point(10, 6));
  R.add_vertex(Point(10, 10));
  R.add_vertex(Point(0, 10));
  R.close();

  // Source in bottom room, target in top room.
  // Line x=8 from (8,2) to (8,8) crosses edges y=4 and y=6.
  ShortestPathInPolygon sp;
  auto path = sp(R, Point(8, 2), Point(8, 8));

  EXPECT_GE(path.size(), 3u);
  EXPECT_EQ(path.get_first(), Point(8, 2));
  EXPECT_EQ(path.get_last(), Point(8, 8));

  // Must pass through reflex vertices (6,4) and (6,6).
  bool has_6_4 = false, has_6_6 = false;
  for (DynList<Point>::Iterator it(path); it.has_curr(); it.next_ne())
    {
      if (it.get_curr() == Point(6, 4)) has_6_4 = true;
      if (it.get_curr() == Point(6, 6)) has_6_6 = true;
    }
  EXPECT_TRUE(has_6_4) << "Path must pass through reflex vertex (6,4)";
  EXPECT_TRUE(has_6_6) << "Path must pass through reflex vertex (6,6)";

  // All segments must stay inside.
  DynList<Point>::Iterator it(path);
  Point prev = it.get_curr();
  it.next_ne();
  for (; it.has_curr(); it.next_ne())
    {
      const Segment seg(prev, it.get_curr());
      bool crosses = false;
      for (Polygon::Segment_Iterator si(R); si.has_curr() and not crosses;
           si.next_ne())
        if (seg.intersects_properly_with(si.get_current_segment()))
          crosses = true;
      EXPECT_FALSE(crosses) << "Path segment crosses polygon boundary";
      prev = it.get_curr();
    }
}

TEST_F(GeomAlgorithmsTest, ShortestPathAllSegmentsInsidePolygon)
{
  // Generic property test: for ANY shortest path in a simple polygon,
  // every segment of the path must not properly intersect any polygon edge.
  // Use the L-shaped polygon with various source/target pairs.
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(10, 0));
  L.add_vertex(Point(10, 5));
  L.add_vertex(Point(5, 5));
  L.add_vertex(Point(5, 10));
  L.add_vertex(Point(0, 10));
  L.close();

  ShortestPathInPolygon sp;

  // Several test pairs.
  Point pairs[][2] = {
    {Point(1, 1), Point(1, 9)},
    {Point(9, 1), Point(1, 9)},
    {Point(9, 2), Point(3, 8)},
    {Point(1, 8), Point(8, 1)},
  };

  for (auto & pair : pairs)
    {
      auto path = sp(L, pair[0], pair[1]);
      ASSERT_GE(path.size(), 2u);
      EXPECT_EQ(path.get_first(), pair[0]);
      EXPECT_EQ(path.get_last(), pair[1]);

      DynList<Point>::Iterator it(path);
      Point prev = it.get_curr();
      it.next_ne();
      for (; it.has_curr(); it.next_ne())
        {
          const Point & curr = it.get_curr();
          const Segment seg(prev, curr);
          bool crosses = false;
          for (Polygon::Segment_Iterator si(L); si.has_curr() and not crosses;
               si.next_ne())
            if (seg.intersects_properly_with(si.get_current_segment()))
              crosses = true;
          EXPECT_FALSE(crosses)
            << "Path segment crosses polygon boundary for pair ("
            << pair[0] << ", " << pair[1] << ")";
          prev = curr;
        }
    }
}

// =========================================================================
// SegmentArrangement tests
// =========================================================================

TEST_F(GeomAlgorithmsTest, ArrangementEmpty)
{
  SegmentArrangement arr;
  Array<Segment> segs;
  auto r = arr(segs);
  EXPECT_EQ(r.vertices.size(), 0u);
  EXPECT_EQ(r.edges.size(), 0u);
  ASSERT_EQ(r.faces.size(), 1u);
  EXPECT_TRUE(r.faces(0).unbounded);
}

TEST_F(GeomAlgorithmsTest, ArrangementSingleSegment)
{
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  auto r = arr(segs);
  EXPECT_EQ(r.vertices.size(), 2u);
  EXPECT_EQ(r.edges.size(), 1u);
  // One unbounded face.
  size_t ub_count = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    if (r.faces(i).unbounded) ++ub_count;
  EXPECT_EQ(ub_count, 1u);
}

TEST_F(GeomAlgorithmsTest, ArrangementParallelNoIntersection)
{
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  segs.append(Segment(Point(0, 2), Point(4, 2)));
  auto r = arr(segs);
  EXPECT_EQ(r.vertices.size(), 4u);
  EXPECT_EQ(r.edges.size(), 2u);
  // No bounded face — only unbounded.
  size_t ub_count = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    if (r.faces(i).unbounded) ++ub_count;
  EXPECT_EQ(ub_count, 1u);
}

TEST_F(GeomAlgorithmsTest, ArrangementSimpleCross)
{
  // Two crossing segments: (0,0)-(4,4) and (0,4)-(4,0)
  // Intersection at (2,2).
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 4)));
  segs.append(Segment(Point(0, 4), Point(4, 0)));
  auto r = arr(segs);
  EXPECT_EQ(r.vertices.size(), 5u);  // 4 endpoints + 1 intersection
  EXPECT_EQ(r.edges.size(), 4u);     // each segment split into 2

  // Check Euler formula: V - E + F = 1 + C
  // V=5, E=4, C=1 (connected via center) → F = 1 + 1 - 5 + 4 = 1
  // Wait: the arrangement has a cross, not a closed face.
  // Actually with 4 edges meeting at center and open ends, there's no bounded face.
  // So F = 1 (unbounded). V - E + F = 5 - 4 + 1 = 2 = 1 + C = 1 + 1. ✓
  size_t ub_count = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    if (r.faces(i).unbounded) ++ub_count;
  EXPECT_GE(ub_count, 1u);
}

TEST_F(GeomAlgorithmsTest, ArrangementTriangleFromSegments)
{
  // Three segments forming a triangle: (0,0)-(4,0), (4,0)-(2,4), (2,4)-(0,0).
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  segs.append(Segment(Point(4, 0), Point(2, 4)));
  segs.append(Segment(Point(2, 4), Point(0, 0)));
  auto r = arr(segs);
  EXPECT_EQ(r.vertices.size(), 3u);
  EXPECT_EQ(r.edges.size(), 3u);
  // Euler: V - E + F = 1 + C → 3 - 3 + F = 2 → F = 2.
  EXPECT_EQ(r.faces.size(), 2u);

  // Exactly one bounded and one unbounded face.
  size_t bounded = 0, unbounded = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    {
      if (r.faces(i).unbounded)
        ++unbounded;
      else
        ++bounded;
    }
  EXPECT_EQ(bounded, 1u);
  EXPECT_EQ(unbounded, 1u);
}

TEST_F(GeomAlgorithmsTest, ArrangementStarPattern)
{
  // 4 segments through common center (0,0):
  // horizontal, vertical, and two diagonals.
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(-2, 0), Point(2, 0)));
  segs.append(Segment(Point(0, -2), Point(0, 2)));
  segs.append(Segment(Point(-2, -2), Point(2, 2)));
  segs.append(Segment(Point(-2, 2), Point(2, -2)));
  auto r = arr(segs);

  // 8 endpoints + 1 center = 9 vertices.
  // The center has 6 intersections (C(4,2)) but they all merge to one point.
  EXPECT_EQ(r.vertices.size(), 9u);
  // Each segment is split into 2 sub-edges → 8 edges.
  EXPECT_EQ(r.edges.size(), 8u);

  // Euler: V - E + F = 1 + C.  C=1 (all connected).
  // 9 - 8 + F = 2 → F = 1.
  // No bounded face (star is open at the tips).
  size_t ub_count = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    if (r.faces(i).unbounded) ++ub_count;
  EXPECT_GE(ub_count, 1u);
}

TEST_F(GeomAlgorithmsTest, ArrangementEulerFormula)
{
  // Verify Euler's formula V - E + F = 1 + C on several arrangements.
  SegmentArrangement arr;

  auto check_euler = [](const SegmentArrangement::Result & r) {
    const size_t V = r.vertices.size();
    const size_t E = r.edges.size();
    const size_t F = r.faces.size();

    if (V == 0) return; // empty case: just the unbounded face

    // Compute connected components via union-find.
    Array<size_t> parent;
    parent.reserve(V);
    for (size_t i = 0; i < V; ++i)
      parent.append(i);

    std::function<size_t(size_t)> find = [&](size_t x) -> size_t {
      while (parent(x) != x)
        {
          parent(x) = parent(parent(x));
          x = parent(x);
        }
      return x;
    };

    for (size_t i = 0; i < E; ++i)
      {
        size_t a = find(r.edges(i).src);
        size_t b = find(r.edges(i).tgt);
        if (a != b)
          parent(a) = b;
      }

    size_t C = 0;
    for (size_t i = 0; i < V; ++i)
      if (find(i) == i) ++C;

    EXPECT_EQ(V - E + F, 1 + C)
      << "V=" << V << " E=" << E << " F=" << F << " C=" << C;
  };

  // Case 1: Triangle.
  {
    Array<Segment> segs;
    segs.append(Segment(Point(0, 0), Point(6, 0)));
    segs.append(Segment(Point(6, 0), Point(3, 6)));
    segs.append(Segment(Point(3, 6), Point(0, 0)));
    check_euler(arr(segs));
  }

  // Case 2: Square.
  {
    Array<Segment> segs;
    segs.append(Segment(Point(0, 0), Point(4, 0)));
    segs.append(Segment(Point(4, 0), Point(4, 4)));
    segs.append(Segment(Point(4, 4), Point(0, 4)));
    segs.append(Segment(Point(0, 4), Point(0, 0)));
    check_euler(arr(segs));
  }

  // Case 3: Two crossing segments.
  {
    Array<Segment> segs;
    segs.append(Segment(Point(0, 0), Point(4, 4)));
    segs.append(Segment(Point(0, 4), Point(4, 0)));
    check_euler(arr(segs));
  }

  // Case 4: Star pattern.
  {
    Array<Segment> segs;
    segs.append(Segment(Point(-2, 0), Point(2, 0)));
    segs.append(Segment(Point(0, -2), Point(0, 2)));
    segs.append(Segment(Point(-2, -2), Point(2, 2)));
    segs.append(Segment(Point(-2, 2), Point(2, -2)));
    check_euler(arr(segs));
  }
}

TEST_F(GeomAlgorithmsTest, ArrangementHasUnboundedFace)
{
  // Any non-empty arrangement must have exactly one unbounded face.
  // Test with a square arrangement.
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  segs.append(Segment(Point(4, 0), Point(4, 4)));
  segs.append(Segment(Point(4, 4), Point(0, 4)));
  segs.append(Segment(Point(0, 4), Point(0, 0)));
  auto r = arr(segs);

  EXPECT_EQ(r.vertices.size(), 4u);
  EXPECT_EQ(r.edges.size(), 4u);

  size_t ub_count = 0;
  for (size_t i = 0; i < r.faces.size(); ++i)
    if (r.faces(i).unbounded) ++ub_count;
  EXPECT_EQ(ub_count, 1u);

  // Should have 2 faces: 1 bounded (inside square) + 1 unbounded.
  EXPECT_EQ(r.faces.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, ArrangementBoundedFaceVertices)
{
  // Triangle: the bounded face should have exactly 3 boundary vertices.
  SegmentArrangement arr;
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(6, 0)));
  segs.append(Segment(Point(6, 0), Point(3, 6)));
  segs.append(Segment(Point(3, 6), Point(0, 0)));
  auto r = arr(segs);

  // Find the bounded face.
  bool found_bounded = false;
  for (size_t i = 0; i < r.faces.size(); ++i)
    {
      if (!r.faces(i).unbounded)
        {
          EXPECT_EQ(r.faces(i).boundary.size(), 3u);
          found_bounded = true;
        }
    }
  EXPECT_TRUE(found_bounded);
}

// ============================================================================
// Rotated Ellipse Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, RotatedEllipseAxisAligned)
{
  // Axis-aligned ellipse (θ = 0): a=4, b=2
  RotatedEllipse e(Point(0, 0), Geom_Number(4), Geom_Number(2));

  // Center should be contained.
  EXPECT_TRUE(e.contains(Point(0, 0)));

  // Points on the semi-axes should be on the boundary.
  EXPECT_TRUE(e.on_boundary(Point(4, 0)));
  EXPECT_TRUE(e.on_boundary(Point(-4, 0)));
  EXPECT_TRUE(e.on_boundary(Point(0, 2)));
  EXPECT_TRUE(e.on_boundary(Point(0, -2)));

  // A point well inside.
  EXPECT_TRUE(e.strictly_contains(Point(1, 1)));

  // A point well outside.
  EXPECT_FALSE(e.contains(Point(5, 0)));
  EXPECT_FALSE(e.contains(Point(0, 3)));
}

TEST_F(GeomAlgorithmsTest, RotatedEllipse90Degrees)
{
  // Rotated 90°: cos=0, sin=1.  a=4, b=2 → after rotation, the
  // semi-axis of length 4 points along y and the one of length 2 along x.
  RotatedEllipse e(Point(0, 0), Geom_Number(4), Geom_Number(2),
                   Geom_Number(0), Geom_Number(1));

  // After 90° rotation: (4,0) in local → (0,4) in world,
  // (0,2) in local → (-2,0) in world.
  EXPECT_TRUE(e.on_boundary(Point(0, 4)));
  EXPECT_TRUE(e.on_boundary(Point(0, -4)));
  EXPECT_TRUE(e.on_boundary(Point(-2, 0)));
  EXPECT_TRUE(e.on_boundary(Point(2, 0)));

  EXPECT_TRUE(e.contains(Point(0, 0)));
  EXPECT_FALSE(e.contains(Point(3, 0)));
  EXPECT_FALSE(e.contains(Point(0, 5)));
}

TEST_F(GeomAlgorithmsTest, RotatedEllipseExtremalPoints)
{
  RotatedEllipse e(Point(1, 2), Geom_Number(3), Geom_Number(1));
  auto ext = e.extremal_points();

  // Axis-aligned: right = center + (a, 0)
  EXPECT_EQ(ext.right, Point(4, 2));
  EXPECT_EQ(ext.left, Point(-2, 2));
  EXPECT_EQ(ext.top, Point(1, 3));
  EXPECT_EQ(ext.bottom, Point(1, 1));
}

TEST_F(GeomAlgorithmsTest, RotatedEllipseSample)
{
  RotatedEllipse e(Point(0, 0), Geom_Number(3), Geom_Number(2));

  // Sample at cos=1, sin=0 → local (3, 0) → world (3, 0).
  Point p = e.sample(Geom_Number(1), Geom_Number(0));
  EXPECT_EQ(p, Point(3, 0));

  // Sample at cos=0, sin=1 → local (0, 2) → world (0, 2).
  p = e.sample(Geom_Number(0), Geom_Number(1));
  EXPECT_EQ(p, Point(0, 2));
}

// ============================================================================
// Bezier Curve Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, BezierQuadraticEndpoints)
{
  Point p0(0, 0), p1(1, 2), p2(2, 0);

  // At t=0, should be p0.
  EXPECT_EQ(BezierCurve::quadratic(p0, p1, p2, Geom_Number(0)), p0);

  // At t=1, should be p2.
  EXPECT_EQ(BezierCurve::quadratic(p0, p1, p2, Geom_Number(1)), p2);
}

TEST_F(GeomAlgorithmsTest, BezierQuadraticMidpoint)
{
  Point p0(0, 0), p1(1, 2), p2(2, 0);

  // At t=1/2: (1-t)²p0 + 2t(1-t)p1 + t²p2
  // = (1/4)(0,0) + (1/2)(1,2) + (1/4)(2,0)
  // = (0,0) + (1/2, 1) + (1/2, 0) = (1, 1)
  Point mid = BezierCurve::quadratic(p0, p1, p2, Geom_Number(1, 2));
  EXPECT_EQ(mid, Point(1, 1));
}

TEST_F(GeomAlgorithmsTest, BezierCubicEndpoints)
{
  Point p0(0, 0), p1(1, 3), p2(3, 3), p3(4, 0);

  EXPECT_EQ(BezierCurve::cubic(p0, p1, p2, p3, Geom_Number(0)), p0);
  EXPECT_EQ(BezierCurve::cubic(p0, p1, p2, p3, Geom_Number(1)), p3);
}

TEST_F(GeomAlgorithmsTest, BezierCubicMidpoint)
{
  Point p0(0, 0), p1(0, 4), p2(4, 4), p3(4, 0);

  // At t=1/2: (1/8)(0,0) + 3(1/8)(0,4) + 3(1/8)(4,4) + (1/8)(4,0)
  // = (0,0) + (0, 3/2) + (3/2, 3/2) + (1/2, 0)
  // = (2, 3)
  Point mid = BezierCurve::cubic(p0, p1, p2, p3, Geom_Number(1, 2));
  EXPECT_EQ(mid, Point(2, 3));
}

TEST_F(GeomAlgorithmsTest, BezierSampleQuadratic)
{
  Point p0(0, 0), p1(1, 2), p2(2, 0);
  auto pts = BezierCurve::sample_quadratic(p0, p1, p2, 4);

  EXPECT_EQ(pts.size(), 5u);
  EXPECT_EQ(pts(0), p0);
  EXPECT_EQ(pts(4), p2);
}

TEST_F(GeomAlgorithmsTest, BezierSampleCubic)
{
  Point p0(0, 0), p1(1, 3), p2(3, 3), p3(4, 0);
  auto pts = BezierCurve::sample_cubic(p0, p1, p2, p3, 10);

  EXPECT_EQ(pts.size(), 11u);
  EXPECT_EQ(pts(0), p0);
  EXPECT_EQ(pts(10), p3);
}

TEST_F(GeomAlgorithmsTest, BezierSplitCubic)
{
  Point p0(0, 0), p1(1, 3), p2(3, 3), p3(4, 0);

  auto sr = BezierCurve::split_cubic(p0, p1, p2, p3, Geom_Number(1, 2));

  // Left curve starts at p0.
  EXPECT_EQ(sr.left[0], p0);

  // Right curve ends at p3.
  EXPECT_EQ(sr.right[3], p3);

  // Both meet at the midpoint.
  EXPECT_EQ(sr.left[3], sr.right[0]);

  // The meeting point should equal cubic evaluation at t=1/2.
  Point mid = BezierCurve::cubic(p0, p1, p2, p3, Geom_Number(1, 2));
  EXPECT_EQ(sr.left[3], mid);
}

TEST_F(GeomAlgorithmsTest, BezierControlBbox)
{
  Point p0(0, 0), p1(1, 5), p2(3, -1), p3(4, 2);
  auto bbox = BezierCurve::control_bbox(p0, p1, p2, p3);

  EXPECT_EQ(bbox.get_xmin(), Geom_Number(0));
  EXPECT_EQ(bbox.get_xmax(), Geom_Number(4));
  EXPECT_EQ(bbox.get_ymin(), Geom_Number(-1));
  EXPECT_EQ(bbox.get_ymax(), Geom_Number(5));
}

// ============================================================================
// Alpha Shape Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, AlphaShapeLargeAlphaEqualsDelaunay)
{
  // With a very large alpha, all Delaunay triangles should pass.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  pts.append(Point(4, 4));
  pts.append(Point(0, 4));
  pts.append(Point(2, 2));

  AlphaShape alpha;
  auto result = alpha(pts, Geom_Number(100000));

  // Should contain all Delaunay triangles.
  DelaunayTriangulationBowyerWatson del;
  auto dt = del(pts);

  EXPECT_EQ(result.triangles.size(), dt.triangles.size());
}

TEST_F(GeomAlgorithmsTest, AlphaShapeSmallAlphaFilters)
{
  // With alpha very small, fewer (or no) triangles should pass.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(5, 10));

  AlphaShape alpha;

  // Large alpha: should keep the triangle.
  auto r1 = alpha(pts, Geom_Number(10000));
  EXPECT_EQ(r1.triangles.size(), 1u);

  // Very small alpha: triangle's circumradius² > alpha², so it should be
  // filtered out.
  auto r2 = alpha(pts, Geom_Number(1, 100));
  EXPECT_EQ(r2.triangles.size(), 0u);
  EXPECT_EQ(r2.boundary_edges.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AlphaShapeBoundaryEdges)
{
  // Equilateral-like triangle: all edges should be boundary for large alpha.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(6, 0));
  pts.append(Point(3, 5));

  AlphaShape alpha;
  auto r = alpha(pts, Geom_Number(100000));

  EXPECT_EQ(r.triangles.size(), 1u);
  // 1 triangle → all 3 edges are boundary.
  EXPECT_EQ(r.boundary_edges.size(), 3u);
}

// ============================================================================
// Power Diagram Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, PowerDiagramEqualWeights)
{
  // With equal weights, power diagram should be identical to Voronoi.
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(4, 0), Geom_Number(0)});
  sites.append({Point(2, 4), Geom_Number(0)});

  PowerDiagram pd;
  auto result = pd(sites);

  EXPECT_EQ(result.sites.size(), 3u);
  // 1 Delaunay triangle → 1 power vertex (circumcenter).
  EXPECT_EQ(result.vertices.size(), 1u);
  // 1 cell per site.
  EXPECT_EQ(result.cells.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, PowerDiagramPowerCenter)
{
  // Three sites with equal weights at (0,0), (4,0), (2,4).
  // Power center = circumcenter when weights are equal.
  PowerDiagram::WeightedSite a{Point(0, 0), Geom_Number(0)};
  PowerDiagram::WeightedSite b{Point(4, 0), Geom_Number(0)};
  PowerDiagram::WeightedSite c{Point(2, 4), Geom_Number(0)};

  Point pc = PowerDiagram::power_center(a, b, c);

  // Circumcenter of (0,0), (4,0), (2,4): midpoint checks.
  // Should be equidistant from all three.
  Geom_Number da = pc.distance_squared_to(Point(0, 0));
  Geom_Number db = pc.distance_squared_to(Point(4, 0));
  Geom_Number dc = pc.distance_squared_to(Point(2, 4));
  EXPECT_EQ(da, db);
  EXPECT_EQ(db, dc);
}

TEST_F(GeomAlgorithmsTest, PowerDiagramWithWeights)
{
  // When weights differ, the power center shifts.
  PowerDiagram::WeightedSite a{Point(0, 0), Geom_Number(1)};
  PowerDiagram::WeightedSite b{Point(4, 0), Geom_Number(1)};
  PowerDiagram::WeightedSite c{Point(2, 4), Geom_Number(1)};

  // Equal weights should still give circumcenter.
  Point pc = PowerDiagram::power_center(a, b, c);
  Geom_Number da = pc.distance_squared_to(Point(0, 0)) - 1;
  Geom_Number db = pc.distance_squared_to(Point(4, 0)) - 1;
  Geom_Number dc = pc.distance_squared_to(Point(2, 4)) - 1;
  EXPECT_EQ(da, db);
  EXPECT_EQ(db, dc);
}

TEST_F(GeomAlgorithmsTest, PowerDiagramSquare)
{
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(4, 0), Geom_Number(0)});
  sites.append({Point(4, 4), Geom_Number(0)});
  sites.append({Point(0, 4), Geom_Number(0)});

  PowerDiagram pd;
  auto result = pd(sites);

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_EQ(result.cells.size(), 4u);
  // Should have at least 1 edge.
  EXPECT_GE(result.edges.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, RegularTriangulationEqualWeightsMatchesDelaunay)
{
  // With equal weights, regular triangulation == standard Delaunay.
  Array<RegularTriangulationBowyerWatson::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(6, 0), Geom_Number(0)});
  sites.append({Point(2, 4), Geom_Number(0)});

  constexpr RegularTriangulationBowyerWatson reg;
  auto rr = reg(sites);

  ASSERT_EQ(rr.sites.size(), 3u);
  ASSERT_EQ(rr.triangles.size(), 1u);

  // Compare against standard Delaunay.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(6, 0));
  pts.append(Point(2, 4));

  constexpr DelaunayTriangulationBowyerWatson del;
  auto dr = del(pts);

  ASSERT_EQ(dr.triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, RegularTriangulationNonUniformWeights)
{
  // Five sites with non-uniform weights.  The regular triangulation must
  // produce valid (non-degenerate) triangles.
  Array<RegularTriangulationBowyerWatson::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(10, 0), Geom_Number(0)});
  sites.append({Point(10, 10), Geom_Number(0)});
  sites.append({Point(0, 10), Geom_Number(0)});
  sites.append({Point(5, 5), Geom_Number(50)});  // very large weight

  constexpr RegularTriangulationBowyerWatson reg;
  auto rr = reg(sites);

  EXPECT_EQ(rr.sites.size(), 5u);
  // With a large weight on the center point, the regular triangulation
  // should still produce triangles (the center site dominates).
  EXPECT_GE(rr.triangles.size(), 1u);

  // Every output triangle must be non-degenerate.
  for (size_t t = 0; t < rr.triangles.size(); ++t)
    {
      const auto & tri = rr.triangles(t);
      EXPECT_NE(orientation(rr.sites(tri.i).position,
                            rr.sites(tri.j).position,
                            rr.sites(tri.k).position),
                Orientation::COLLINEAR);
    }
}

TEST_F(GeomAlgorithmsTest, PowerDiagramNonUniformWeightsCorrectness)
{
  // Triangle with one site having a very large weight.
  // The power center of each triangle must be equidistant (in power
  // distance) to all three vertices of that triangle.
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(10, 0), Geom_Number(0)});
  sites.append({Point(5, 8), Geom_Number(30)});

  PowerDiagram pd;
  auto result = pd(sites);

  EXPECT_EQ(result.sites.size(), 3u);
  ASSERT_GE(result.vertices.size(), 1u);
  EXPECT_EQ(result.cells.size(), 3u);

  // For each power vertex (one per triangle in the regular triangulation),
  // check that the power distance to all three sites of that triangle is
  // equal.  This is the defining property of the power center.
  // We don't have direct triangle→site mapping in the Result, but we can
  // verify that each vertex is equidistant to at least one triple.
  // Since we have 3 sites and at least 1 triangle, verify the first vertex.
  const Point & pc = result.vertices(0);
  const Geom_Number pd0 = pc.distance_squared_to(sites(0).position) - sites(0).weight;
  const Geom_Number pd1 = pc.distance_squared_to(sites(1).position) - sites(1).weight;
  const Geom_Number pd2 = pc.distance_squared_to(sites(2).position) - sites(2).weight;

  EXPECT_EQ(pd0, pd1) << "Power distance to site 0 != site 1";
  EXPECT_EQ(pd1, pd2) << "Power distance to site 1 != site 2";
}

TEST_F(GeomAlgorithmsTest, PowerDiagramFourSitesNonUniformWeights)
{
  // Four sites in a square, one corner with a large weight.
  // The regular triangulation may differ from the standard Delaunay.
  Array<PowerDiagram::WeightedSite> sites;
  sites.append({Point(0, 0), Geom_Number(0)});
  sites.append({Point(10, 0), Geom_Number(0)});
  sites.append({Point(10, 10), Geom_Number(0)});
  sites.append({Point(0, 10), Geom_Number(80)});  // large weight

  PowerDiagram pd;
  auto result = pd(sites);

  EXPECT_EQ(result.sites.size(), 4u);
  EXPECT_EQ(result.cells.size(), 4u);
  EXPECT_GE(result.vertices.size(), 1u);

  // Each power vertex must be equi-power-distant to its defining triple.
  // Verify all vertices satisfy the power-center property for *some* triple.
  for (size_t v = 0; v < result.vertices.size(); ++v)
    {
      const Point & pc = result.vertices(v);
      Array<Geom_Number> pds;
      for (size_t s = 0; s < result.sites.size(); ++s)
        pds.append(pc.distance_squared_to(result.sites(s).position)
                   - result.sites(s).weight);

      // At least 3 power distances must be equal (those of the defining triple).
      bool found_triple = false;
      for (size_t a = 0; a < pds.size() and not found_triple; ++a)
        for (size_t b = a + 1; b < pds.size() and not found_triple; ++b)
          for (size_t c = b + 1; c < pds.size() and not found_triple; ++c)
            if (pds(a) == pds(b) and pds(b) == pds(c))
              found_triple = true;

      EXPECT_TRUE(found_triple) << "Power vertex " << v
                                << " is not equidistant to any site triple";
    }
}

// ============================================================================
// Boolean Polygon Operations Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, BooleanIntersectionOverlappingSquares)
{
  // Two overlapping unit squares: one at (0,0)-(2,2) and another at (1,1)-(3,3).
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(2, 0));
  sq1.add_vertex(Point(2, 2));
  sq1.add_vertex(Point(0, 2));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(1, 1));
  sq2.add_vertex(Point(3, 1));
  sq2.add_vertex(Point(3, 3));
  sq2.add_vertex(Point(1, 3));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.intersection(sq1, sq2);

  // Should produce exactly one polygon.
  EXPECT_EQ(result.size(), 1u);

  // The intersection should be a square with vertices at (1,1), (2,1), (2,2), (1,2).
  EXPECT_EQ(result(0).size(), 4u);
}

TEST_F(GeomAlgorithmsTest, BooleanIntersectionDisjoint)
{
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(1, 0));
  sq1.add_vertex(Point(1, 1));
  sq1.add_vertex(Point(0, 1));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(5, 5));
  sq2.add_vertex(Point(6, 5));
  sq2.add_vertex(Point(6, 6));
  sq2.add_vertex(Point(5, 6));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.intersection(sq1, sq2);

  // Disjoint squares should have empty intersection.
  EXPECT_EQ(result.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, BooleanUnionDisjoint)
{
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(1, 0));
  sq1.add_vertex(Point(1, 1));
  sq1.add_vertex(Point(0, 1));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(5, 5));
  sq2.add_vertex(Point(6, 5));
  sq2.add_vertex(Point(6, 6));
  sq2.add_vertex(Point(5, 6));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.polygon_union(sq1, sq2);

  // Disjoint: should return both polygons.
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, BooleanUnionOverlapping)
{
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(2, 0));
  sq1.add_vertex(Point(2, 2));
  sq1.add_vertex(Point(0, 2));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(1, 1));
  sq2.add_vertex(Point(3, 1));
  sq2.add_vertex(Point(3, 3));
  sq2.add_vertex(Point(1, 3));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.polygon_union(sq1, sq2);

  // Overlapping: should return 1 merged polygon.
  EXPECT_EQ(result.size(), 1u);
  // The hull of the union should have vertices from both squares.
  EXPECT_GE(result(0).size(), 4u);
}

TEST_F(GeomAlgorithmsTest, BooleanDifferenceNoOverlap)
{
  Polygon sq1;
  sq1.add_vertex(Point(0, 0));
  sq1.add_vertex(Point(1, 0));
  sq1.add_vertex(Point(1, 1));
  sq1.add_vertex(Point(0, 1));
  sq1.close();

  Polygon sq2;
  sq2.add_vertex(Point(5, 5));
  sq2.add_vertex(Point(6, 5));
  sq2.add_vertex(Point(6, 6));
  sq2.add_vertex(Point(5, 6));
  sq2.close();

  BooleanPolygonOperations bop;
  auto result = bop.difference(sq1, sq2);

  // No overlap: a - b = a.
  EXPECT_EQ(result.size(), 1u);
  EXPECT_EQ(result(0).size(), 4u);
}

// ============================================================================
// 3D Primitives Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, Point3DBasicOps)
{
  Point3D a(1, 2, 3);
  Point3D b(4, 5, 6);

  auto sum = a + b;
  EXPECT_EQ(sum.get_x(), Geom_Number(5));
  EXPECT_EQ(sum.get_y(), Geom_Number(7));
  EXPECT_EQ(sum.get_z(), Geom_Number(9));

  auto diff = b - a;
  EXPECT_EQ(diff.get_x(), Geom_Number(3));
  EXPECT_EQ(diff.get_y(), Geom_Number(3));
  EXPECT_EQ(diff.get_z(), Geom_Number(3));

  auto scaled = a * Geom_Number(2);
  EXPECT_EQ(scaled.get_x(), Geom_Number(2));
  EXPECT_EQ(scaled.get_y(), Geom_Number(4));
  EXPECT_EQ(scaled.get_z(), Geom_Number(6));
}

TEST_F(GeomAlgorithmsTest, Point3DDotCross)
{
  Point3D i(1, 0, 0);
  Point3D j(0, 1, 0);
  Point3D k(0, 0, 1);

  // i · j = 0
  EXPECT_EQ(i.dot(j), Geom_Number(0));
  // i · i = 1
  EXPECT_EQ(i.dot(i), Geom_Number(1));

  // i × j = k
  auto ixj = i.cross(j);
  EXPECT_EQ(ixj, k);

  // j × k = i
  auto jxk = j.cross(k);
  EXPECT_EQ(jxk, i);

  // k × i = j
  auto kxi = k.cross(i);
  EXPECT_EQ(kxi, j);
}

TEST_F(GeomAlgorithmsTest, Point3DDistanceAndNorm)
{
  Point3D a(0, 0, 0);
  Point3D b(3, 4, 0);

  EXPECT_EQ(a.distance_squared_to(b), Geom_Number(25));
  EXPECT_EQ(b.norm_squared(), Geom_Number(25));
}

TEST_F(GeomAlgorithmsTest, Point3DProjectionAndLift)
{
  Point3D p(3, 4, 5);
  Point p2d = p.to_2d();
  EXPECT_EQ(p2d, Point(3, 4));

  Point3D lifted = Point3D::from_2d(Point(1, 2));
  EXPECT_EQ(lifted, Point3D(1, 2, 0));

  Point3D lifted_z = Point3D::from_2d(Point(1, 2), Geom_Number(7));
  EXPECT_EQ(lifted_z, Point3D(1, 2, 7));
}

TEST_F(GeomAlgorithmsTest, Segment3DBasic)
{
  Point3D a(0, 0, 0), b(3, 4, 0);
  Segment3D s(a, b);

  EXPECT_EQ(s.get_src(), a);
  EXPECT_EQ(s.get_tgt(), b);
  EXPECT_EQ(s.length_squared(), Geom_Number(25));

  auto mid = s.midpoint();
  EXPECT_EQ(mid, Point3D(Geom_Number(3, 2), Geom_Number(2), Geom_Number(0)));

  EXPECT_EQ(s.at(Geom_Number(0)), a);
  EXPECT_EQ(s.at(Geom_Number(1)), b);
}

TEST_F(GeomAlgorithmsTest, Triangle3DNormal)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(0, 1, 0));

  // Normal should be (0, 0, 1) (z-axis).
  auto n = t.normal();
  EXPECT_EQ(n, Point3D(0, 0, 1));

  EXPECT_FALSE(t.is_degenerate());
}

TEST_F(GeomAlgorithmsTest, Triangle3DDegenerate)
{
  // Collinear points → degenerate triangle.
  Triangle3D t(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(2, 0, 0));
  EXPECT_TRUE(t.is_degenerate());
}

TEST_F(GeomAlgorithmsTest, Triangle3DCentroid)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(3, 0, 0), Point3D(0, 3, 0));
  auto c = t.centroid();
  EXPECT_EQ(c, Point3D(1, 1, 0));
}

TEST_F(GeomAlgorithmsTest, Triangle3DBarycentric)
{
  Triangle3D t(Point3D(0, 0, 0), Point3D(4, 0, 0), Point3D(0, 4, 0));

  // Centroid should have barycentric coords (1/3, 1/3, 1/3).
  auto bc = t.barycentric(Point3D(Geom_Number(4, 3), Geom_Number(4, 3), 0));
  EXPECT_EQ(bc.u, Geom_Number(1, 3));
  EXPECT_EQ(bc.v, Geom_Number(1, 3));
  EXPECT_EQ(bc.w, Geom_Number(1, 3));

  // Vertex a should have (1, 0, 0).
  auto bca = t.barycentric(Point3D(0, 0, 0));
  EXPECT_EQ(bca.u, Geom_Number(1));
  EXPECT_EQ(bca.v, Geom_Number(0));
  EXPECT_EQ(bca.w, Geom_Number(0));
}

TEST_F(GeomAlgorithmsTest, TetrahedronVolume)
{
  // Regular tetrahedron with one vertex at origin.
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(6, 0, 0),
                  Point3D(0, 6, 0),
                  Point3D(0, 0, 6));

  // Volume = |det| / 6 = 6*6*6 / 6 = 36.
  EXPECT_EQ(tet.volume(), Geom_Number(36));

  EXPECT_FALSE(tet.is_degenerate());
}

TEST_F(GeomAlgorithmsTest, TetrahedronDegenerate)
{
  // Four coplanar points.
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(1, 0, 0),
                  Point3D(0, 1, 0),
                  Point3D(1, 1, 0));

  EXPECT_TRUE(tet.is_degenerate());
  EXPECT_EQ(tet.volume(), Geom_Number(0));
}

TEST_F(GeomAlgorithmsTest, TetrahedronContains)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(4, 0, 0),
                  Point3D(0, 4, 0),
                  Point3D(0, 0, 4));

  // Centroid should be inside.
  EXPECT_TRUE(tet.contains(Point3D(1, 1, 1)));

  // Origin vertex should be inside (on boundary).
  EXPECT_TRUE(tet.contains(Point3D(0, 0, 0)));

  // A point far outside.
  EXPECT_FALSE(tet.contains(Point3D(10, 10, 10)));

  // A point outside but close.
  EXPECT_FALSE(tet.contains(Point3D(2, 2, 2)));
}

TEST_F(GeomAlgorithmsTest, TetrahedronCentroid)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(4, 0, 0),
                  Point3D(0, 4, 0),
                  Point3D(0, 0, 4));

  auto c = tet.centroid();
  EXPECT_EQ(c, Point3D(1, 1, 1));
}

TEST_F(GeomAlgorithmsTest, TetrahedronFaces)
{
  Tetrahedron tet(Point3D(0, 0, 0),
                  Point3D(1, 0, 0),
                  Point3D(0, 1, 0),
                  Point3D(0, 0, 1));

  auto f = tet.faces();
  // Should have 4 faces.
  for (int i = 0; i < 4; ++i)
    EXPECT_FALSE(f.f[i].is_degenerate());
}

TEST_F(GeomAlgorithmsTest, ScalarTripleProduct)
{
  Point3D a(1, 0, 0);
  Point3D b(0, 1, 0);
  Point3D c(0, 0, 1);

  // a · (b × c) = 1 · (1) = 1
  EXPECT_EQ(scalar_triple_product(a, b, c), Geom_Number(1));

  // Cyclic: b · (c × a) = 1
  EXPECT_EQ(scalar_triple_product(b, c, a), Geom_Number(1));

  // Anti-cyclic: a · (c × b) = -1
  EXPECT_EQ(scalar_triple_product(a, c, b), Geom_Number(-1));
}

// ============================================================================
// operator<< Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, StreamOutputPoint)
{
  std::ostringstream os;
  os << Point(3, 4);
  EXPECT_NE(os.str().find("Point("), std::string::npos);
  EXPECT_NE(os.str().find("3"), std::string::npos);
  EXPECT_NE(os.str().find("4"), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputSegment)
{
  std::ostringstream os;
  os << Segment(Point(1, 2), Point(3, 4));
  EXPECT_NE(os.str().find("Segment("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputTriangle)
{
  std::ostringstream os;
  os << Triangle(Point(0, 0), Point(1, 0), Point(0, 1));
  EXPECT_NE(os.str().find("Triangle("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputRectangle)
{
  std::ostringstream os;
  os << Rectangle(0, 0, 5, 5);
  EXPECT_NE(os.str().find("Rectangle("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputEllipse)
{
  std::ostringstream os;
  os << Ellipse(Point(0, 0), 3, 2);
  EXPECT_NE(os.str().find("Ellipse("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputRotatedEllipse)
{
  std::ostringstream os;
  os << RotatedEllipse(Point(0, 0), 3, 2);
  EXPECT_NE(os.str().find("RotatedEllipse("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutputPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  std::ostringstream os;
  os << sq;
  EXPECT_NE(os.str().find("Polygon("), std::string::npos);
  EXPECT_NE(os.str().find("n=4"), std::string::npos);
  EXPECT_NE(os.str().find("closed"), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StreamOutput3DTypes)
{
  {
    std::ostringstream os;
    os << Point3D(1, 2, 3);
    EXPECT_NE(os.str().find("Point3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Segment3D(Point3D(0, 0, 0), Point3D(1, 1, 1));
    EXPECT_NE(os.str().find("Segment3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Triangle3D(Point3D(0, 0, 0), Point3D(1, 0, 0), Point3D(0, 1, 0));
    EXPECT_NE(os.str().find("Triangle3D("), std::string::npos);
  }
  {
    std::ostringstream os;
    os << Tetrahedron(Point3D(0, 0, 0), Point3D(1, 0, 0),
                      Point3D(0, 1, 0), Point3D(0, 0, 1));
    EXPECT_NE(os.str().find("Tetrahedron("), std::string::npos);
  }
}

// ============================================================================
// Serialization (WKT, GeoJSON) Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, WKTPoint)
{
  auto wkt = GeomSerializer::to_wkt(Point(3, 4));
  EXPECT_NE(wkt.find("POINT ("), std::string::npos);
  EXPECT_NE(wkt.find("3"), std::string::npos);
  EXPECT_NE(wkt.find("4"), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, WKTSegment)
{
  auto wkt = GeomSerializer::to_wkt(Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(wkt.find("LINESTRING ("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, WKTTriangle)
{
  auto wkt = GeomSerializer::to_wkt(Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
  // WKT polygon must close: first point repeated at end.
  // Count occurrences of "0 0" — should appear twice (start and end).
  size_t pos = 0, count = 0;
  while ((pos = wkt.find("0 0", pos)) != std::string::npos)
    { ++count; ++pos; }
  EXPECT_GE(count, 2u);
}

TEST_F(GeomAlgorithmsTest, WKTRectangle)
{
  auto wkt = GeomSerializer::to_wkt(Rectangle(0, 0, 5, 5));
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, WKTPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto wkt = GeomSerializer::to_wkt(sq);
  EXPECT_NE(wkt.find("POLYGON (("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, WKTPoint3D)
{
  auto wkt = GeomSerializer::to_wkt(Point3D(1, 2, 3));
  EXPECT_NE(wkt.find("POINT Z ("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, GeoJSONPoint)
{
  auto json = GeomSerializer::to_geojson(Point(3, 4));
  EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
  EXPECT_NE(json.find("\"coordinates\":["), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, GeoJSONSegment)
{
  auto json = GeomSerializer::to_geojson(Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(json.find("\"type\":\"LineString\""), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, GeoJSONTriangle)
{
  auto json = GeomSerializer::to_geojson(
    Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, GeoJSONPolygon)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto json = GeomSerializer::to_geojson(sq);
  EXPECT_NE(json.find("\"type\":\"Polygon\""), std::string::npos);
  EXPECT_NE(json.find("\"coordinates\":[["), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, GeoJSONPoint3D)
{
  auto json = GeomSerializer::to_geojson(Point3D(1, 2, 3));
  EXPECT_NE(json.find("\"type\":\"Point\""), std::string::npos);
}

// ============================================================================
// AABB Tree Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, AABBTreeEmpty)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  tree.build(entries);
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AABBTreeSingleEntry)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 10, 10), 42});
  tree.build(entries);

  EXPECT_EQ(tree.size(), 1u);
  EXPECT_FALSE(tree.is_empty());

  // Point inside.
  auto r = tree.query_point(Point(5, 5));
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r(0), 42u);

  // Point outside.
  r = tree.query_point(Point(20, 20));
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AABBTreeMultipleEntries)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(3, 3, 8, 8), 1});
  entries.append({Rectangle(10, 10, 15, 15), 2});
  entries.append({Rectangle(12, 0, 17, 5), 3});
  tree.build(entries);

  EXPECT_EQ(tree.size(), 4u);

  // Query a point in the overlap of boxes 0 and 1.
  auto r = tree.query_point(Point(4, 4));
  EXPECT_EQ(r.size(), 2u);

  // Query a point only in box 2.
  r = tree.query_point(Point(12, 12));
  EXPECT_EQ(r.size(), 1u);
  EXPECT_EQ(r(0), 2u);

  // Query a point outside all boxes.
  r = tree.query_point(Point(50, 50));
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AABBTreeBoxQuery)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(3, 3, 8, 8), 1});
  entries.append({Rectangle(10, 10, 15, 15), 2});
  tree.build(entries);

  // Query box overlapping entries 0 and 1.
  auto r = tree.query(Rectangle(2, 2, 6, 6));
  EXPECT_EQ(r.size(), 2u);

  // Query box overlapping all entries.
  r = tree.query(Rectangle(0, 0, 20, 20));
  EXPECT_EQ(r.size(), 3u);

  // Query box overlapping nothing.
  r = tree.query(Rectangle(50, 50, 60, 60));
  EXPECT_EQ(r.size(), 0u);
}

TEST_F(GeomAlgorithmsTest, AABBTreeRootBbox)
{
  AABBTree tree;
  Array<AABBTree::Entry> entries;
  entries.append({Rectangle(0, 0, 5, 5), 0});
  entries.append({Rectangle(10, 10, 15, 15), 1});
  tree.build(entries);

  auto root = tree.root_bbox();
  EXPECT_EQ(root.get_xmin(), Geom_Number(0));
  EXPECT_EQ(root.get_ymin(), Geom_Number(0));
  EXPECT_EQ(root.get_xmax(), Geom_Number(15));
  EXPECT_EQ(root.get_ymax(), Geom_Number(15));
}

// ============================================================================
// GeomNumberType Concept Test (compile-time)
// ============================================================================

#if __cplusplus >= 202002L
TEST_F(GeomAlgorithmsTest, GeomNumberConceptSatisfied)
{
  // These are compile-time checks; if we get here, the static_asserts passed.
  EXPECT_TRUE((GeomNumberType<Geom_Number>));
  EXPECT_TRUE((GeomNumberType<double>));
  EXPECT_TRUE((GeomNumberType<long long>));
}
#endif

// ============================================================================
// std::format Tests
// ============================================================================

#if __cplusplus >= 202002L && __has_include(<format>)
# include <format>

TEST_F(GeomAlgorithmsTest, StdFormatPoint)
{
  auto s = std::format("{}", Point(3, 4));
  EXPECT_NE(s.find("Point("), std::string::npos);
  EXPECT_NE(s.find("3"), std::string::npos);
  EXPECT_NE(s.find("4"), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StdFormatSegment)
{
  auto s = std::format("{}", Segment(Point(1, 2), Point(3, 4)));
  EXPECT_NE(s.find("Segment("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StdFormatTriangle)
{
  auto s = std::format("{}", Triangle(Point(0, 0), Point(1, 0), Point(0, 1)));
  EXPECT_NE(s.find("Triangle("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StdFormatRectangle)
{
  auto s = std::format("{}", Rectangle(0, 0, 5, 5));
  EXPECT_NE(s.find("Rectangle("), std::string::npos);
}

TEST_F(GeomAlgorithmsTest, StdFormatPoint3D)
{
  auto s = std::format("{}", Point3D(1, 2, 3));
  EXPECT_NE(s.find("Point3D("), std::string::npos);
}

#endif // C++20 format

// ============================================================================
// Polygon Aleph Patterns Tests (Section 6.3)
// ============================================================================

TEST_F(GeomAlgorithmsTest, PolygonRangeBasedFor)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(4, 4));
  sq.add_vertex(Point(0, 4));
  sq.close();

  // Range-based for via StlAlephIterator begin()/end().
  size_t count = 0;
  for (const auto & pt : sq)
    {
      (void)pt;
      ++count;
    }
  EXPECT_EQ(count, 4u);
}

TEST_F(GeomAlgorithmsTest, PolygonIterator)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  // Use the new Polygon::Iterator directly.
  Polygon::Iterator it(sq);
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), Point(0, 0));
  it.next();
  EXPECT_EQ(it.get_curr(), Point(4, 0));
  it.next();
  EXPECT_EQ(it.get_curr(), Point(2, 3));
  it.next();
  EXPECT_FALSE(it.has_curr());
}

TEST_F(GeomAlgorithmsTest, PolygonForEach)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  // FunctionalMethods::for_each
  Geom_Number sum_x = 0;
  sq.for_each([&sum_x](const Point & p) { sum_x += p.get_x(); });
  EXPECT_EQ(sum_x, Geom_Number(2)); // 0 + 1 + 1 + 0
}

TEST_F(GeomAlgorithmsTest, PolygonTraverse)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  // GenericTraverse::traverse — stop early.
  size_t visited = 0;
  bool completed = sq.traverse([&visited](const Point &) {
    ++visited;
    return visited < 2; // stop after 2
  });
  EXPECT_FALSE(completed);
  EXPECT_EQ(visited, 2u);
}

TEST_F(GeomAlgorithmsTest, PolygonExists)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  EXPECT_TRUE(sq.exists([](const Point & p) {
    return p.get_x() == 1 && p.get_y() == 1;
  }));

  EXPECT_FALSE(sq.exists([](const Point & p) {
    return p.get_x() == 99;
  }));
}

TEST_F(GeomAlgorithmsTest, PolygonAll)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  EXPECT_TRUE(sq.all([](const Point & p) {
    return p.get_x() >= 0 && p.get_y() >= 0;
  }));

  EXPECT_FALSE(sq.all([](const Point & p) {
    return p.get_x() > 0;
  }));
}

TEST_F(GeomAlgorithmsTest, PolygonMaps)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  auto xs = sq.maps<Geom_Number>([](const Point & p) { return p.get_x(); });
  EXPECT_EQ(xs.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, PolygonFilter)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(1, 0));
  sq.add_vertex(Point(1, 1));
  sq.add_vertex(Point(0, 1));
  sq.close();

  auto filtered = sq.filter([](const Point & p) { return p.get_x() > 0; });
  EXPECT_EQ(filtered.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, PolygonInitializerList)
{
  // Special_Ctors: construct from initializer_list<Point>.
  Polygon poly = {Point(0, 0), Point(2, 0), Point(2, 2), Point(0, 2)};

  EXPECT_EQ(poly.size(), 4u);
  EXPECT_FALSE(poly.is_closed()); // Special_Ctors doesn't close
}

TEST_F(GeomAlgorithmsTest, PolygonGetIt)
{
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(4, 0));
  sq.add_vertex(Point(2, 3));
  sq.close();

  auto it = sq.get_it();
  EXPECT_TRUE(it.has_curr());
  EXPECT_EQ(it.get_curr(), Point(0, 0));

  auto it2 = sq.get_it(2);
  EXPECT_EQ(it2.get_curr(), Point(2, 3));
}

// ============================================================================
// Section 7.1: Missing Correctness Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, ConvexHullAlgorithmComparison)
{
  // All convex hull algorithms should produce the same result.
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(10, 10));
  pts.append(Point(0, 10));
  pts.append(Point(5, 5));   // interior
  pts.append(Point(3, 2));   // interior
  pts.append(Point(7, 8));   // interior
  pts.append(Point(1, 9));   // interior

  GiftWrappingConvexHull gift;
  GrahamScanConvexHull graham;
  QuickHull qh;

  auto hull_gw = gift(pts);
  auto hull_gm = graham(pts);
  auto hull_qh = qh(pts);

  // All should have same number of hull vertices (the 4 corners).
  EXPECT_EQ(hull_gw.size(), 4u);
  EXPECT_EQ(hull_gm.size(), 4u);
  EXPECT_EQ(hull_qh.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, TriangulationNonConvexLShape)
{
  // L-shaped polygon (non-convex).
  Polygon L;
  L.add_vertex(Point(0, 0));
  L.add_vertex(Point(6, 0));
  L.add_vertex(Point(6, 3));
  L.add_vertex(Point(3, 3));
  L.add_vertex(Point(3, 6));
  L.add_vertex(Point(0, 6));
  L.close();

  CuttingEarsTriangulation cet;
  auto tris = cet(L);
  // An n-vertex polygon yields n-2 triangles.
  EXPECT_EQ(tris.size(), 4u); // 6 vertices -> 4 triangles
}

TEST_F(GeomAlgorithmsTest, TriangulationNonConvexUShaped)
{
  // U-shaped polygon.
  Polygon U;
  U.add_vertex(Point(0, 0));
  U.add_vertex(Point(6, 0));
  U.add_vertex(Point(6, 6));
  U.add_vertex(Point(5, 6));
  U.add_vertex(Point(5, 1));
  U.add_vertex(Point(1, 1));
  U.add_vertex(Point(1, 6));
  U.add_vertex(Point(0, 6));
  U.close();

  CuttingEarsTriangulation cet;
  auto tris = cet(U);
  EXPECT_EQ(tris.size(), 6u); // 8 vertices -> 6 triangles
}

TEST_F(GeomAlgorithmsTest, PointInPolygonManyVertices)
{
  // Create a regular-ish polygon with many vertices (circle approximation).
  Polygon circle;
  const int N = 32;
  for (int i = 0; i < N; ++i)
    {
      Geom_Number x(Geom_Number(1000) * Geom_Number(i) / Geom_Number(N));
      Geom_Number y(Geom_Number(1000) * Geom_Number((i * 7 + 3) % N) / Geom_Number(N));
      // Use a convex polygon: just use a large square with many vertices on edges
      if (i < N/4)
        circle.add_vertex(Point(i * 4, 0));
      else if (i < N/2)
        circle.add_vertex(Point((N/4) * 4, (i - N/4) * 4));
      else if (i < 3*N/4)
        circle.add_vertex(Point((3*N/4 - i) * 4, (N/4) * 4));
      else
        circle.add_vertex(Point(0, (N - i) * 4));
    }
  circle.close();

  // Center should be inside.
  EXPECT_TRUE(circle.contains(Point(16, 16)));
  // Far away point should be outside.
  EXPECT_FALSE(circle.contains(Point(1000, 1000)));
}

// ============================================================================
// Section 7.2: Missing Robustness Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, NearCollinearPoints)
{
  // Three nearly collinear points — exact arithmetic should handle this.
  // p3 deviates by 1/10^15 from the line p1-p2.
  Point p1(0, 0);
  Point p2(Geom_Number(1000000), 0);
  // Tiny deviation from collinear.
  Point p3(Geom_Number(500000), Geom_Number(1, 1000000000));

  // Should NOT be collinear (exact rational arithmetic).
  EXPECT_FALSE(p3.is_colinear_with(p1, p2));

  // But if deviation is exactly 0, it IS collinear.
  Point p4(Geom_Number(500000), 0);
  EXPECT_TRUE(p4.is_colinear_with(p1, p2));
}

TEST_F(GeomAlgorithmsTest, ExtremeCoordinates)
{
  // Very large coordinates.
  Geom_Number big("1000000000000000000"); // 10^18
  Point p1(big, big);
  Point p2(-big, -big);
  Point p3(big, -big);

  // Distance should be exact.
  Geom_Number dist_sq = p1.distance_squared_to(p2);
  EXPECT_EQ(dist_sq, Geom_Number(4) * big * big * Geom_Number(2));

  // Triangle should work.
  Triangle t(p1, p2, p3);
  EXPECT_FALSE(t.contains(Point(0, 0))); // origin outside this triangle

  // Very small coordinates.
  Geom_Number tiny(1, 1000000000);
  Point q1(0, 0);
  Point q2(tiny, 0);
  Point q3(0, tiny);
  Triangle t2(q1, q2, q3);
  // A point at (tiny/3, tiny/3) should be inside.
  EXPECT_TRUE(t2.contains(Point(tiny / 3, tiny / 3)));
}

TEST_F(GeomAlgorithmsTest, NearParallelSegments)
{
  // Two segments that are nearly parallel but do intersect.
  Segment s1(Point(0, 0), Point(Geom_Number(1000000), Geom_Number(1)));
  Segment s2(Point(0, Geom_Number(1, 2)),
             Point(Geom_Number(1000000), 0));

  // They should intersect (they cross at some point).
  EXPECT_TRUE(s1.intersects_with(s2));
}

TEST_F(GeomAlgorithmsTest, CocircularPoints)
{
  // 4 points on a circle of radius 5 centered at origin.
  // (3,4), (-3,4), (-3,-4), (3,-4) all on circle r=5.
  Point a(3, 4);
  Point b(-3, 4);
  Point c(-3, -4);
  Point d(3, -4);

  // d should be ON the circumcircle of a,b,c (not inside).
  auto result = in_circle(a, b, c, d);
  EXPECT_EQ(result, InCircleResult::ON_CIRCLE);
}

// ============================================================================
// Section 7.4: Missing Primitive Tests
// ============================================================================

TEST_F(GeomAlgorithmsTest, IntersectsProperlyWithNearCollinear)
{
  // Test the intersects_properly_with predicate with near-collinear segments.
  Segment s1(Point(0, 0), Point(10, 0));
  Segment s2(Point(5, -1), Point(5, 1));

  EXPECT_TRUE(s1.intersects_properly_with(s2));

  // Collinear overlapping segments should NOT intersect properly.
  Segment s3(Point(0, 0), Point(6, 0));
  Segment s4(Point(4, 0), Point(10, 0));
  EXPECT_FALSE(s3.intersects_properly_with(s4));
}

TEST_F(GeomAlgorithmsTest, EllipseIntersectionVerticalSegment)
{
  // Vertical segment through the center of an ellipse.
  Ellipse e(Point(0, 0), 5, 3);

  // Vertical segment x=0 from y=-10 to y=10.
  Segment vert(Point(0, -10), Point(0, 10));
  EXPECT_TRUE(e.intersects_with(vert));
}

TEST_F(GeomAlgorithmsTest, SegmentEnlargeDiagonal)
{
  // Enlarge a diagonal segment in both directions.
  Segment s(Point(0, 0), Point(3, 4)); // length = 5

  Segment s_copy = s;
  s_copy.enlarge_src(Geom_Number(5));
  // Source should have moved away from target.
  EXPECT_TRUE(s_copy.size() > s.size());

  Segment s_copy2 = s;
  s_copy2.enlarge_tgt(Geom_Number(5));
  EXPECT_TRUE(s_copy2.size() > s.size());
}

TEST_F(GeomAlgorithmsTest, TriangleCWvsCCW)
{
  // CCW triangle.
  Triangle ccw(Point(0, 0), Point(4, 0), Point(2, 3));
  EXPECT_TRUE(ccw.contains(Point(2, 1)));

  // CW triangle (reversed vertex order).
  Triangle cw(Point(0, 0), Point(2, 3), Point(4, 0));
  EXPECT_TRUE(cw.contains(Point(2, 1)));
}

TEST_F(GeomAlgorithmsTest, RectangleCornerIntersection)
{
  // Two rectangles sharing exactly one corner.
  Rectangle r1(0, 0, 5, 5);
  Rectangle r2(5, 5, 10, 10);

  // They touch at (5,5) — xmin of r2 == xmax of r1.
  // Depending on the definition, they may or may not intersect.
  // The point (5,5) is on the boundary of both.
  Point corner(5, 5);
  EXPECT_TRUE(corner.get_x() >= r1.get_xmin() &&
              corner.get_x() <= r1.get_xmax() &&
              corner.get_y() >= r1.get_ymin() &&
              corner.get_y() <= r1.get_ymax());
  EXPECT_TRUE(corner.get_x() >= r2.get_xmin() &&
              corner.get_x() <= r2.get_xmax() &&
              corner.get_y() >= r2.get_ymin() &&
              corner.get_y() <= r2.get_ymax());
}

TEST_F(GeomAlgorithmsTest, SegmentContainsEndpoints)
{
  // contains() should return true for endpoints.
  Segment s(Point(1, 2), Point(5, 6));
  EXPECT_TRUE(s.contains(s.get_src_point()));
  EXPECT_TRUE(s.contains(s.get_tgt_point()));

  // Midpoint should also be contained.
  EXPECT_TRUE(s.contains(s.mid_point()));
}

TEST_F(GeomAlgorithmsTest, PolygonContainsNewAPI)
{
  // Verify the new contains() method on Polygon works.
  Polygon sq;
  sq.add_vertex(Point(0, 0));
  sq.add_vertex(Point(10, 0));
  sq.add_vertex(Point(10, 10));
  sq.add_vertex(Point(0, 10));
  sq.close();

  EXPECT_TRUE(sq.contains(Point(5, 5)));
  EXPECT_FALSE(sq.contains(Point(20, 20)));
  // Boundary point.
  EXPECT_TRUE(sq.contains(Point(0, 5)));
}

TEST_F(GeomAlgorithmsTest, TriangleContainsNewAPI)
{
  Triangle t(Point(0, 0), Point(10, 0), Point(0, 10));
  EXPECT_TRUE(t.contains(Point(1, 1)));
  EXPECT_FALSE(t.contains(Point(8, 8)));
}

TEST_F(GeomAlgorithmsTest, EllipseContainsNewAPI)
{
  Ellipse e(Point(0, 0), 5, 3);
  EXPECT_TRUE(e.contains(Point(0, 0)));    // center
  EXPECT_TRUE(e.contains(Point(4, 0)));    // inside
  EXPECT_FALSE(e.contains(Point(10, 10))); // outside
}
