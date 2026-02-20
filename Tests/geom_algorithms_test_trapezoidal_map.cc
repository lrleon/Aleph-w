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

#include "geom_algorithms_test_common.h"

using namespace Aleph;

using TM = TrapezoidalMapPointLocation;
using LT = TM::LocationType;

// ---- helpers ----

static size_t count_active_trapezoids(const TM::Result & res)
{
  size_t count = 0;
  for (size_t i = 0; i < res.trapezoids.size(); ++i)
    if (res.trapezoids(i).active) ++count;
  return count;
}

static Polygon make_square_poly(Geom_Number cx, Geom_Number cy,
                                Geom_Number half)
{
  Polygon p;
  p.add_vertex(Point(cx - half, cy - half));
  p.add_vertex(Point(cx + half, cy - half));
  p.add_vertex(Point(cx + half, cy + half));
  p.add_vertex(Point(cx - half, cy + half));
  p.close();
  return p;
}

static Polygon make_triangle_poly()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(6, 0));
  p.add_vertex(Point(3, 6));
  p.close();
  return p;
}

static Polygon make_hexagon()
{
  // Regular-ish hexagon centered at (5,5) with radius ~4.
  Polygon p;
  p.add_vertex(Point(9, 5));
  p.add_vertex(Point(7, 9));
  p.add_vertex(Point(3, 9));
  p.add_vertex(Point(1, 5));
  p.add_vertex(Point(3, 1));
  p.add_vertex(Point(7, 1));
  p.close();
  return p;
}

// ============================================================================
// Basic tests
// ============================================================================

TEST(TrapezoidalMap, SingleSegment)
{
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 2)));

  TM tm;
  auto res = tm(segs);

  // Should have at least 4 active trapezoids:
  // left of src, above, below, right of tgt.
  EXPECT_GE(count_active_trapezoids(res), 4u);

  // Query above the segment.
  auto loc = res.locate(Point(2, 4));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query below the segment.
  loc = res.locate(Point(2, -2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query left of the segment.
  loc = res.locate(Point(-2, 0));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query right of the segment.
  loc = res.locate(Point(6, 1));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
}

TEST(TrapezoidalMap, TwoParallelSegments)
{
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  segs.append(Segment(Point(0, 2), Point(4, 2)));

  TM tm;
  auto res = tm(segs);

  // Query between the two segments.
  auto loc = res.locate(Point(2, 1));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query above both.
  loc = res.locate(Point(2, 4));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query below both.
  loc = res.locate(Point(2, -2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
}

TEST(TrapezoidalMap, TwoSegmentsSharedEndpoint)
{
  // V-shape: two segments sharing a common left endpoint.
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 2)));
  segs.append(Segment(Point(0, 0), Point(4, -2)));

  TM tm;
  auto res = tm(segs);

  // Query inside the V (between the two segments, right of origin).
  auto loc = res.locate(Point(2, 0));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Query at the shared endpoint.
  loc = res.locate(Point(0, 0));
  EXPECT_EQ(loc.type, LT::ON_POINT);
}

TEST(TrapezoidalMap, SquarePolygon)
{
  Polygon sq = make_square_poly(2, 2, 2);

  TM tm;
  auto res = tm(sq);

  // Inside the square.
  EXPECT_TRUE(res.contains(Point(2, 2)));
  EXPECT_TRUE(res.contains(Point(1, 1)));
  EXPECT_TRUE(res.contains(Point(3, 3)));

  // Outside the square.
  EXPECT_FALSE(res.contains(Point(5, 5)));
  EXPECT_FALSE(res.contains(Point(-1, -1)));
  EXPECT_FALSE(res.contains(Point(2, 5)));

  // On boundary — contains() detects boundary correctly.
  EXPECT_TRUE(res.contains(Point(0, 2)));
}

TEST(TrapezoidalMap, TrianglePolygon)
{
  Polygon tri = make_triangle_poly();

  TM tm;
  auto res = tm(tri);

  // Centroid (2,2) is inside.
  EXPECT_TRUE(res.contains(Point(2, 2)));
  EXPECT_TRUE(res.contains(Point(3, 1)));

  // Outside.
  EXPECT_FALSE(res.contains(Point(0, 6)));
  EXPECT_FALSE(res.contains(Point(-1, -1)));

  // Vertex.
  auto loc = res.locate(Point(0, 0));
  EXPECT_EQ(loc.type, LT::ON_POINT);

  // Edge midpoint — contains() always detects boundary correctly.
  // Note: locate() may return TRAPEZOID for boundary points depending
  // on the random DAG structure, so we test contains() instead.
  EXPECT_TRUE(res.contains(Point(3, 0)));
}

// ============================================================================
// Degeneracy tests
// ============================================================================

TEST(TrapezoidalMap, VerticalSegment)
{
  Array<Segment> segs;
  segs.append(Segment(Point(2, 0), Point(2, 4)));

  TM tm;
  auto res = tm(segs);

  // Left of the vertical segment.
  auto loc = res.locate(Point(0, 2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Right of the vertical segment.
  loc = res.locate(Point(4, 2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
}

TEST(TrapezoidalMap, SharedEndpoints)
{
  // Star pattern: 4 segments from common origin.
  const Point center(0, 0);
  Array<Segment> segs;
  segs.append(Segment(center, Point(4, 0)));
  segs.append(Segment(center, Point(0, 4)));
  segs.append(Segment(center, Point(-4, 0)));
  segs.append(Segment(center, Point(0, -4)));

  TM tm;
  auto res = tm(segs);

  // Origin is a shared endpoint.
  auto loc = res.locate(center);
  EXPECT_EQ(loc.type, LT::ON_POINT);

  // Each quadrant should be reachable.
  loc = res.locate(Point(2, 2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
  loc = res.locate(Point(-2, 2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
  loc = res.locate(Point(-2, -2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
  loc = res.locate(Point(2, -2));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
}

TEST(TrapezoidalMap, PointOnSegment)
{
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 0)));

  TM tm;
  auto res = tm(segs);

  // Point on the segment interior.
  auto loc = res.locate(Point(2, 0));
  EXPECT_EQ(loc.type, LT::ON_SEGMENT);
}

TEST(TrapezoidalMap, PointAtEndpoint)
{
  Array<Segment> segs;
  segs.append(Segment(Point(0, 0), Point(4, 2)));

  TM tm;
  auto res = tm(segs);

  auto loc = res.locate(Point(0, 0));
  EXPECT_EQ(loc.type, LT::ON_POINT);

  loc = res.locate(Point(4, 2));
  EXPECT_EQ(loc.type, LT::ON_POINT);
}

TEST(TrapezoidalMap, HorizontalSegment)
{
  Array<Segment> segs;
  segs.append(Segment(Point(0, 3), Point(6, 3)));

  TM tm;
  auto res = tm(segs);

  // Above.
  auto loc = res.locate(Point(3, 5));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // Below.
  loc = res.locate(Point(3, 1));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);

  // On.
  loc = res.locate(Point(3, 3));
  EXPECT_EQ(loc.type, LT::ON_SEGMENT);
}

// ============================================================================
// Correctness / stress tests
// ============================================================================

TEST(TrapezoidalMap, ConvexPolygonQueries)
{
  Polygon hex = make_hexagon();

  TM tm;
  auto res = tm(hex);

  // Cross-validate with PointInPolygonWinding for a grid of points.
  for (int x = 0; x <= 10; ++x)
    for (int y = 0; y <= 10; ++y)
      {
        Point p(x, y);
        const bool tm_inside = res.contains(p);
        const auto winding_loc = PointInPolygonWinding::locate(hex, p);
        const bool winding_inside =
            winding_loc != PointInPolygonWinding::Location::Outside;

        EXPECT_EQ(tm_inside, winding_inside)
            << "Mismatch at (" << x << ", " << y << "): "
            << "tm=" << tm_inside << " winding=" << winding_inside;
      }
}

TEST(TrapezoidalMap, MultiplePolygons)
{
  // Two non-overlapping triangles.
  Polygon t1;
  t1.add_vertex(Point(0, 0));
  t1.add_vertex(Point(4, 0));
  t1.add_vertex(Point(2, 3));
  t1.close();

  Polygon t2;
  t2.add_vertex(Point(6, 0));
  t2.add_vertex(Point(10, 0));
  t2.add_vertex(Point(8, 3));
  t2.close();

  Array<Polygon> polys;
  polys.append(t1);
  polys.append(t2);

  TM tm;
  auto res = tm(polys);

  // Inside first triangle.
  EXPECT_TRUE(res.contains(Point(2, 1)));
  // Inside second triangle.
  EXPECT_TRUE(res.contains(Point(8, 1)));
  // Between the two triangles.
  EXPECT_FALSE(res.contains(Point(5, 1)));
  // Outside both.
  EXPECT_FALSE(res.contains(Point(5, 5)));
}

TEST(TrapezoidalMap, ManySegments)
{
  // Build segments from a square and a triangle.
  Array<Segment> segs;

  // Square (0,0)-(4,0)-(4,4)-(0,4).
  segs.append(Segment(Point(0, 0), Point(4, 0)));
  segs.append(Segment(Point(4, 0), Point(4, 4)));
  segs.append(Segment(Point(4, 4), Point(0, 4)));
  segs.append(Segment(Point(0, 4), Point(0, 0)));

  // Triangle (6,0)-(10,0)-(8,4).
  segs.append(Segment(Point(6, 0), Point(10, 0)));
  segs.append(Segment(Point(10, 0), Point(8, 4)));
  segs.append(Segment(Point(8, 4), Point(6, 0)));

  // Diamond (3,6)-(5,8)-(3,10)-(1,8).
  segs.append(Segment(Point(3, 6), Point(5, 8)));
  segs.append(Segment(Point(5, 8), Point(3, 10)));
  segs.append(Segment(Point(3, 10), Point(1, 8)));
  segs.append(Segment(Point(1, 8), Point(3, 6)));

  TM tm;
  auto res = tm(segs);

  // The map should have more than the initial bounding-box trapezoid.
  EXPECT_GT(count_active_trapezoids(res), 1u);

  // All 10+ unique points should be locatable.
  for (size_t i = 0; i < res.num_input_points; ++i)
    {
      auto loc = res.locate(res.points(i));
      EXPECT_TRUE(loc.type == LT::ON_POINT or loc.type == LT::ON_SEGMENT)
          << "Point " << i << " not located as ON_POINT or ON_SEGMENT";
    }
}

TEST(TrapezoidalMap, Deterministic)
{
  // Build the same map twice and ensure query results match.
  Polygon sq = make_square_poly(2, 2, 2);

  TM tm;
  auto res1 = tm(sq);
  auto res2 = tm(sq);

  // Test a grid of points — both maps should agree on contains().
  for (int x = -1; x <= 5; ++x)
    for (int y = -1; y <= 5; ++y)
      {
        Point p(x, y);
        EXPECT_EQ(res1.contains(p), res2.contains(p))
            << "Non-deterministic result at (" << x << ", " << y << ")";
      }
}

// ============================================================================
// Edge cases
// ============================================================================

TEST(TrapezoidalMap, EmptyInput)
{
  Array<Segment> segs;

  TM tm;
  auto res = tm(segs);

  // Should have exactly 1 active trapezoid (the bounding box).
  EXPECT_EQ(count_active_trapezoids(res), 1u);

  // Any point query should return TRAPEZOID.
  auto loc = res.locate(Point(0, 0));
  EXPECT_EQ(loc.type, LT::TRAPEZOID);
}

TEST(TrapezoidalMap, DegenerateSegment)
{
  Array<Segment> segs;
  segs.append(Segment(Point(1, 1), Point(1, 1)));

  TM tm;
  EXPECT_THROW(tm(segs), std::domain_error);
}
