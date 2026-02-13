#include "geom_algorithms_test_common.h"

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

