#include <gtest/gtest.h>
#include <geom_algorithms.H>

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
