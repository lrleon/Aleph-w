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

// ---- helpers ----

static Polygon make_square(Geom_Number cx, Geom_Number cy, Geom_Number half)
{
  Polygon p;
  p.add_vertex(Point(cx - half, cy - half));
  p.add_vertex(Point(cx + half, cy - half));
  p.add_vertex(Point(cx + half, cy + half));
  p.add_vertex(Point(cx - half, cy + half));
  p.close();
  return p;
}

static Polygon make_triangle()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(6, 0));
  p.add_vertex(Point(3, 6));
  p.close();
  return p;
}

/// L-shaped polygon (non-convex, one reflex vertex).
///  (0,4)---(2,4)
///    |       |
///    |  (2,2)--(4,2)
///    |           |
///  (0,0)------(4,0)
static Polygon make_L_shape()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 2));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(2, 4));
  p.add_vertex(Point(0, 4));
  p.close();
  return p;
}

/// Star-shaped polygon with 5 reflex vertices.
static Polygon make_star()
{
  Polygon p;
  // Outer tip → inner notch, alternating at 5 points.
  const double R = 5.0, r = 2.0;
  for (int i = 0; i < 5; ++i)
    {
      double angle_out = M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
      double angle_in = angle_out + M_PI / 5.0;
      p.add_vertex(Point(Geom_Number(R * std::cos(angle_out)),
                         Geom_Number(R * std::sin(angle_out))));
      p.add_vertex(Point(Geom_Number(r * std::cos(angle_in)),
                         Geom_Number(r * std::sin(angle_in))));
    }
  p.close();
  return p;
}

/// Regular pentagon (convex).
static Polygon make_pentagon()
{
  Polygon p;
  const double R = 4.0;
  for (int i = 0; i < 5; ++i)
    {
      double a = M_PI / 2.0 + i * 2.0 * M_PI / 5.0;
      p.add_vertex(Point(Geom_Number(R * std::cos(a)),
                         Geom_Number(R * std::sin(a))));
    }
  p.close();
  return p;
}

/// Narrow horizontal corridor: width 1, length 10.
static Polygon make_narrow_corridor()
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(10, 0));
  p.add_vertex(Point(10, 1));
  p.add_vertex(Point(0, 1));
  p.close();
  return p;
}

// ---- Tests ----

TEST_F(GeomAlgorithmsTest, Offset_SquareOutward)
{
  Polygon sq = make_square(0, 0, 2); // side=4, area=16
  PolygonOffset off;
  auto result = off(sq, Geom_Number(1));
  ASSERT_FALSE(result.is_empty());

  // Outward offset by 1 on a 4×4 square → 6×6 → area = 36.
  Geom_Number area = polygon_area(result.polygons(0));
  EXPECT_EQ(area, 36);
  EXPECT_EQ(result.polygons(0).size(), 4);
}

TEST_F(GeomAlgorithmsTest, Offset_SquareInward)
{
  Polygon sq = make_square(0, 0, 2); // side=4, area=16
  PolygonOffset off;
  auto result = off(sq, Geom_Number(-1));
  ASSERT_FALSE(result.is_empty());

  // Inward offset by 1 on a 4×4 square → 2×2 → area = 4.
  Geom_Number area = polygon_area(result.polygons(0));
  EXPECT_EQ(area, 4);
}

TEST_F(GeomAlgorithmsTest, Offset_TriangleOutward)
{
  Polygon tri = make_triangle();
  PolygonOffset off;
  auto result = off(tri, Geom_Number(1));
  ASSERT_FALSE(result.is_empty());

  Geom_Number orig_area = polygon_area(tri);
  Geom_Number new_area = polygon_area(result.polygons(0));
  EXPECT_GT(new_area, orig_area);
}

TEST_F(GeomAlgorithmsTest, Offset_TriangleInward)
{
  Polygon tri = make_triangle();
  PolygonOffset off;
  auto result = off(tri, Geom_Number(-1));
  ASSERT_FALSE(result.is_empty());

  Geom_Number orig_area = polygon_area(tri);
  Geom_Number new_area = polygon_area(result.polygons(0));
  EXPECT_LT(new_area, orig_area);
}

TEST_F(GeomAlgorithmsTest, Offset_ZeroDistance)
{
  Polygon sq = make_square(0, 0, 2);
  PolygonOffset off;
  auto result = off(sq, Geom_Number(0));
  ASSERT_EQ(result.size(), size_t(1));

  Geom_Number area = polygon_area(result.polygons(0));
  EXPECT_EQ(area, polygon_area(sq));
}

TEST_F(GeomAlgorithmsTest, Offset_ExcessiveInward)
{
  Polygon sq = make_square(0, 0, 2); // side=4
  PolygonOffset off;
  // Inward offset of 3 on a 4×4 square collapses it.
  auto result = off(sq, Geom_Number(-3));
  EXPECT_TRUE(result.is_empty());
}

TEST_F(GeomAlgorithmsTest, Offset_LShapeOutward)
{
  Polygon L = make_L_shape();
  PolygonOffset off;
  auto result = off(L, Geom_Number(Geom_Number(1) / 2));
  ASSERT_FALSE(result.is_empty());

  Geom_Number orig_area = polygon_area(L);
  Geom_Number new_area = polygon_area(result.polygons(0));
  EXPECT_GT(new_area, orig_area);
}

TEST_F(GeomAlgorithmsTest, Offset_LShapeInward)
{
  Polygon L = make_L_shape();
  PolygonOffset off;
  auto result = off(L, Geom_Number(Geom_Number(-1) / 4));
  ASSERT_FALSE(result.is_empty());

  Geom_Number orig_area = polygon_area(L);
  Geom_Number new_area = polygon_area(result.polygons(0));
  EXPECT_LT(new_area, orig_area);
}

TEST_F(GeomAlgorithmsTest, Offset_StarOutward)
{
  Polygon star = make_star();
  PolygonOffset off;
  auto result = off(star, Geom_Number(Geom_Number(1) / 2));
  ASSERT_FALSE(result.is_empty());

  Geom_Number orig_area = polygon_area(star);
  // Total offset area should be larger.
  Geom_Number total = 0;
  for (size_t i = 0; i < result.size(); ++i)
    total += polygon_area(result.polygons(i));
  EXPECT_GT(total, orig_area);
}

TEST_F(GeomAlgorithmsTest, Offset_OutwardIncreasesArea)
{
  Array<Polygon> shapes;
  shapes.append(make_square(0, 0, 2));
  shapes.append(make_triangle());
  shapes.append(make_pentagon());

  PolygonOffset off;
  for (size_t i = 0; i < shapes.size(); ++i)
    {
      auto result = off(shapes(i), Geom_Number(1));
      ASSERT_FALSE(result.is_empty()) << "shape " << i;
      EXPECT_GT(polygon_area(result.polygons(0)), polygon_area(shapes(i)))
          << "shape " << i;
    }
}

TEST_F(GeomAlgorithmsTest, Offset_InwardDecreasesArea)
{
  Array<Polygon> shapes;
  shapes.append(make_square(0, 0, 3));
  shapes.append(make_triangle());
  shapes.append(make_pentagon());

  PolygonOffset off;
  for (size_t i = 0; i < shapes.size(); ++i)
    {
      auto result = off(shapes(i), Geom_Number(-1));
      ASSERT_FALSE(result.is_empty()) << "shape " << i;
      EXPECT_LT(polygon_area(result.polygons(0)), polygon_area(shapes(i)))
          << "shape " << i;
    }
}

TEST_F(GeomAlgorithmsTest, Offset_CCWPreserved)
{
  Polygon sq = make_square(0, 0, 3);
  PolygonOffset off;

  auto out = off(sq, Geom_Number(1));
  ASSERT_FALSE(out.is_empty());
  EXPECT_TRUE(is_ccw(out.polygons(0)));

  auto in = off(sq, Geom_Number(-1));
  ASSERT_FALSE(in.is_empty());
  EXPECT_TRUE(is_ccw(in.polygons(0)));
}

TEST_F(GeomAlgorithmsTest, Offset_MiterVsBevel)
{
  Polygon sq = make_square(0, 0, 2);
  PolygonOffset off;

  auto miter = off(sq, Geom_Number(1), PolygonOffset::JoinType::Miter);
  auto bevel = off(sq, Geom_Number(1), PolygonOffset::JoinType::Bevel);
  ASSERT_FALSE(miter.is_empty());
  ASSERT_FALSE(bevel.is_empty());

  // Bevel produces more vertices (2 per corner vs 1).
  EXPECT_GT(bevel.polygons(0).size(), miter.polygons(0).size());
}

TEST_F(GeomAlgorithmsTest, Offset_BevelJoin)
{
  Polygon sq = make_square(0, 0, 2);
  PolygonOffset off;
  auto result = off(sq, Geom_Number(1), PolygonOffset::JoinType::Bevel);
  ASSERT_FALSE(result.is_empty());

  // 4 corners × 2 vertices = 8 vertices.
  EXPECT_EQ(result.polygons(0).size(), size_t(8));

  // Area should be less than miter (corners are cut).
  auto miter = off(sq, Geom_Number(1), PolygonOffset::JoinType::Miter);
  EXPECT_LT(polygon_area(result.polygons(0)),
             polygon_area(miter.polygons(0)));
}

TEST_F(GeomAlgorithmsTest, Offset_MiterLimit)
{
  // A very acute triangle where miter joins would be very long.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(10, 0));
  p.add_vertex(Point(5, 1)); // very flat triangle
  p.close();

  PolygonOffset off;
  // With miter_limit = 1, almost all corners get beveled.
  auto result = off(p, Geom_Number(Geom_Number(1) / 4),
                    PolygonOffset::JoinType::Miter, Geom_Number(1));
  ASSERT_FALSE(result.is_empty());

  // With very high miter limit, pure miter.
  auto result_high = off(p, Geom_Number(Geom_Number(1) / 4),
                         PolygonOffset::JoinType::Miter, Geom_Number(100));
  ASSERT_FALSE(result_high.is_empty());

  // Low miter limit should produce >= the vertices of high miter limit.
  EXPECT_GE(result.polygons(0).size(), result_high.polygons(0).size());
}

TEST_F(GeomAlgorithmsTest, Offset_NestedOffsets)
{
  Polygon sq = make_square(0, 0, 3);
  PolygonOffset off;

  Geom_Number prev_area = polygon_area(sq);
  for (int d = 1; d <= 3; ++d)
    {
      auto result = off(sq, Geom_Number(d));
      ASSERT_FALSE(result.is_empty()) << "d=" << d;
      Geom_Number a = polygon_area(result.polygons(0));
      EXPECT_GT(a, prev_area) << "d=" << d;
      prev_area = a;
    }
}

TEST_F(GeomAlgorithmsTest, Offset_ConvexEquivalence)
{
  Polygon sq = make_square(0, 0, 2);
  PolygonOffset gen_off;
  auto gen_result = gen_off(sq, Geom_Number(1));
  ASSERT_FALSE(gen_result.is_empty());

  Polygon conv_result = ConvexPolygonOffset::outward(sq, Geom_Number(1));

  // Both should have the same area.
  EXPECT_EQ(polygon_area(gen_result.polygons(0)), polygon_area(conv_result));
}

TEST_F(GeomAlgorithmsTest, Offset_SmallTriangle)
{
  // Small triangle: side length ~1.4.  Inward by 1 should collapse.
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(Geom_Number(1) / 2, 1));
  p.close();

  PolygonOffset off;
  auto result = off(p, Geom_Number(-1));
  EXPECT_TRUE(result.is_empty());
}

TEST_F(GeomAlgorithmsTest, Offset_Pentagon)
{
  Polygon pent = make_pentagon();
  PolygonOffset off;

  auto out = off(pent, Geom_Number(1));
  ASSERT_FALSE(out.is_empty());
  EXPECT_GT(polygon_area(out.polygons(0)), polygon_area(pent));

  auto in = off(pent, Geom_Number(-1));
  ASSERT_FALSE(in.is_empty());
  EXPECT_LT(polygon_area(in.polygons(0)), polygon_area(pent));
}

TEST_F(GeomAlgorithmsTest, Offset_NarrowCorridor)
{
  Polygon corridor = make_narrow_corridor(); // width=1
  PolygonOffset off;

  // Inward by 0.25 should still leave a valid corridor.
  auto result = off(corridor, Geom_Number(Geom_Number(-1) / 4));
  ASSERT_FALSE(result.is_empty());
  EXPECT_LT(polygon_area(result.polygons(0)), polygon_area(corridor));

  // Inward by 1 should collapse (width=1, offset=1 → zero width).
  auto collapsed = off(corridor, Geom_Number(-1));
  EXPECT_TRUE(collapsed.is_empty());
}
