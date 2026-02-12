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
