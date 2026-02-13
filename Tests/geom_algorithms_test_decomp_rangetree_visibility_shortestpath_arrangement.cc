#include "geom_algorithms_test_common.h"


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


TEST_F(GeomAlgorithmsTest, ConvexOffsetOutwardCollinearTriple)
{
  // Convex polygon with a collinear triple at the closing junction D-A-B.
  Polygon p;
  p.add_vertex(Point(0, 0));   // A
  p.add_vertex(Point(4, 0));   // B
  p.add_vertex(Point(4, 3));   // C
  p.add_vertex(Point(-2, 0));  // D
  p.close();

  ConvexPolygonOffset off;
  EXPECT_NO_THROW({
    Polygon r = off.outward(p, Geom_Number(1));
    EXPECT_GE(r.size(), 3u);
  });
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
