#include "geom_algorithms_test_common.h"


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

