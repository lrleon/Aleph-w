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
#include <cmath>

using namespace Aleph;

// Helper: check that result is a subsequence of original
static bool is_subsequence(const Array<Point> & original,
                           const Array<Point> & sub)
{
  size_t j = 0;
  for (size_t i = 0; i < original.size() and j < sub.size(); ++i)
    if (original(i) == sub(j))
      ++j;
  return j == sub.size();
}

// Helper: extract polygon vertices into Array
static Array<Point> poly_verts(const Polygon & poly)
{
  return GeomPolygonUtils::extract_vertices(poly);
}

// Helper: make a circle approximation polygon
static Polygon make_circle(size_t n, double radius = 10.0,
                           double cx = 0.0, double cy = 0.0)
{
  Polygon poly;
  for (size_t i = 0; i < n; ++i)
    {
      double angle = 2.0 * M_PI * i / n;
      poly.add_vertex(Point(cx + radius * cos(angle),
                            cy + radius * sin(angle)));
    }
  poly.close();
  return poly;
}

// ===================== Douglas-Peucker tests =====================

TEST_F(GeomAlgorithmsTest, DP_StraightLine)
{
  // Collinear points → only endpoints kept
  Array<Point> line;
  line.append(Point(0, 0));
  line.append(Point(1, 0));
  line.append(Point(2, 0));
  line.append(Point(3, 0));
  line.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  auto result = dp(line, Geom_Number(1, 10)); // epsilon = 0.1

  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result(0), Point(0, 0));
  EXPECT_EQ(result(1), Point(4, 0));
}

TEST_F(GeomAlgorithmsTest, DP_LShape)
{
  // L-shape: all 3 points must be kept
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 0));
  pts.append(Point(5, 5));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(1, 10));

  EXPECT_EQ(result.size(), 3u);
  EXPECT_EQ(result(0), Point(0, 0));
  EXPECT_EQ(result(1), Point(5, 0));
  EXPECT_EQ(result(2), Point(5, 5));
}

TEST_F(GeomAlgorithmsTest, DP_Zigzag)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 2));
  pts.append(Point(2, 0));
  pts.append(Point(3, 2));
  pts.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  // Small epsilon: keep all
  auto result = dp(pts, Geom_Number(1, 100));
  EXPECT_EQ(result.size(), 5u);

  // Large epsilon: only endpoints
  auto result2 = dp(pts, Geom_Number(10));
  EXPECT_EQ(result2.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, DP_Rectangle_Closed)
{
  Polygon rect;
  rect.add_vertex(Point(0, 0));
  rect.add_vertex(Point(10, 0));
  rect.add_vertex(Point(10, 5));
  rect.add_vertex(Point(0, 5));
  rect.close();

  DouglasPeuckerSimplification dp;
  Polygon result = dp.simplify_polygon(rect, Geom_Number(1, 10));

  auto rv = poly_verts(result);
  EXPECT_EQ(rv.size(), 4u); // Rectangle should be preserved
}

TEST_F(GeomAlgorithmsTest, DP_EpsilonZero_KeepsAll)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 1));
  pts.append(Point(2, 0));
  pts.append(Point(3, 1));
  pts.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(0));
  EXPECT_EQ(result.size(), pts.size());
}

TEST_F(GeomAlgorithmsTest, DP_LargeEpsilon_MinimalOutput)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 1));
  pts.append(Point(2, 0));
  pts.append(Point(3, 1));
  pts.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(100));
  EXPECT_EQ(result.size(), 2u); // Only endpoints
}

TEST_F(GeomAlgorithmsTest, DP_TwoPoints)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 5));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(1));
  EXPECT_EQ(result.size(), 2u);
  EXPECT_EQ(result(0), Point(0, 0));
  EXPECT_EQ(result(1), Point(5, 5));
}

TEST_F(GeomAlgorithmsTest, DP_CircleApprox_Progressive)
{
  Polygon circle = make_circle(40);

  DouglasPeuckerSimplification dp;
  Polygon mild = dp.simplify_polygon(circle, Geom_Number(1, 10));
  Polygon aggressive = dp.simplify_polygon(circle, Geom_Number(5));

  auto mv = poly_verts(mild);
  auto av = poly_verts(aggressive);

  EXPECT_LE(av.size(), mv.size());
  EXPECT_LE(mv.size(), 40u);
  EXPECT_GE(av.size(), 3u);
}

// ===================== Visvalingam-Whyatt tests =====================

TEST_F(GeomAlgorithmsTest, VW_SmallTriangleRemoval)
{
  // Nearly collinear interior point → removed
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, Geom_Number(1, 100))); // tiny deviation
  pts.append(Point(10, 0));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(1)); // area threshold = 1
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, VW_Staircase)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 0));
  pts.append(Point(1, 1));
  pts.append(Point(2, 1));
  pts.append(Point(2, 2));
  pts.append(Point(3, 2));

  VisvalingamWhyattSimplification vw;
  // Small threshold keeps all
  auto result = vw(pts, Geom_Number(1, 1000));
  EXPECT_EQ(result.size(), pts.size());

  // Large threshold: only endpoints
  auto result2 = vw(pts, Geom_Number(100));
  EXPECT_EQ(result2.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, VW_Rectangle_Closed)
{
  Polygon rect;
  rect.add_vertex(Point(0, 0));
  rect.add_vertex(Point(10, 0));
  rect.add_vertex(Point(10, 5));
  rect.add_vertex(Point(0, 5));
  rect.close();

  VisvalingamWhyattSimplification vw;
  Polygon result = vw.simplify_polygon(rect, Geom_Number(1));

  auto rv = poly_verts(result);
  EXPECT_EQ(rv.size(), 4u); // Rectangle corners have area=25, should stay
}

TEST_F(GeomAlgorithmsTest, VW_ThresholdZero_KeepsAll)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 1));
  pts.append(Point(2, 0));
  pts.append(Point(3, 1));
  pts.append(Point(4, 0));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(0));
  EXPECT_EQ(result.size(), pts.size());
}

TEST_F(GeomAlgorithmsTest, VW_LargeThreshold_MinimalOutput)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 1));
  pts.append(Point(2, 0));
  pts.append(Point(3, 1));
  pts.append(Point(4, 0));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(10000));
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, VW_TwoPoints)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(5, 5));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(1));
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, VW_CircleApprox_Closed)
{
  Polygon circle = make_circle(40);

  VisvalingamWhyattSimplification vw;
  Polygon mild = vw.simplify_polygon(circle, Geom_Number(1, 10));
  Polygon aggressive = vw.simplify_polygon(circle, Geom_Number(50));

  auto mv = poly_verts(mild);
  auto av = poly_verts(aggressive);

  EXPECT_LE(av.size(), mv.size());
  EXPECT_LE(mv.size(), 40u);
  EXPECT_GE(av.size(), 3u);
}

// ===================== Property checks (both) =====================

TEST_F(GeomAlgorithmsTest, Simplification_SubsequenceProperty_DP)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 2));
  pts.append(Point(2, -1));
  pts.append(Point(3, 3));
  pts.append(Point(4, 0));
  pts.append(Point(5, 2));
  pts.append(Point(6, -1));
  pts.append(Point(7, 0));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(1));

  EXPECT_TRUE(is_subsequence(pts, result));
  EXPECT_LE(result.size(), pts.size());
  EXPECT_EQ(result(0), pts(0));
  EXPECT_EQ(result(result.size() - 1), pts(pts.size() - 1));
}

TEST_F(GeomAlgorithmsTest, Simplification_SubsequenceProperty_VW)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 2));
  pts.append(Point(2, -1));
  pts.append(Point(3, 3));
  pts.append(Point(4, 0));
  pts.append(Point(5, 2));
  pts.append(Point(6, -1));
  pts.append(Point(7, 0));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(1));

  EXPECT_TRUE(is_subsequence(pts, result));
  EXPECT_LE(result.size(), pts.size());
  EXPECT_EQ(result(0), pts(0));
  EXPECT_EQ(result(result.size() - 1), pts(pts.size() - 1));
}

TEST_F(GeomAlgorithmsTest, Simplification_CountProperty)
{
  Polygon circle = make_circle(30);
  auto orig = poly_verts(circle);

  DouglasPeuckerSimplification dp;
  auto dp_result = dp.simplify_polygon(circle, Geom_Number(1));
  auto dpv = poly_verts(dp_result);
  EXPECT_LE(dpv.size(), orig.size());

  VisvalingamWhyattSimplification vw;
  auto vw_result = vw.simplify_polygon(circle, Geom_Number(1));
  auto vwv = poly_verts(vw_result);
  EXPECT_LE(vwv.size(), orig.size());
}

TEST_F(GeomAlgorithmsTest, DP_FirstLastPreserved)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 5));
  pts.append(Point(2, 0));
  pts.append(Point(3, 5));
  pts.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  for (int e = 1; e <= 10; ++e)
    {
      auto result = dp(pts, Geom_Number(e));
      EXPECT_GE(result.size(), 2u);
      EXPECT_EQ(result(0), pts(0));
      EXPECT_EQ(result(result.size() - 1), pts(pts.size() - 1));
    }
}

TEST_F(GeomAlgorithmsTest, VW_FirstLastPreserved)
{
  Array<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 5));
  pts.append(Point(2, 0));
  pts.append(Point(3, 5));
  pts.append(Point(4, 0));

  VisvalingamWhyattSimplification vw;
  for (int e = 1; e <= 10; ++e)
    {
      auto result = vw(pts, Geom_Number(e));
      EXPECT_GE(result.size(), 2u);
      EXPECT_EQ(result(0), pts(0));
      EXPECT_EQ(result(result.size() - 1), pts(pts.size() - 1));
    }
}

TEST_F(GeomAlgorithmsTest, DP_DynListOverload)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 2));
  pts.append(Point(2, 0));
  pts.append(Point(3, 2));
  pts.append(Point(4, 0));

  DouglasPeuckerSimplification dp;
  auto result = dp(pts, Geom_Number(100));
  EXPECT_EQ(result.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, VW_DynListOverload)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(1, 2));
  pts.append(Point(2, 0));
  pts.append(Point(3, 2));
  pts.append(Point(4, 0));

  VisvalingamWhyattSimplification vw;
  auto result = vw(pts, Geom_Number(100));
  EXPECT_EQ(result.size(), 2u);
}
