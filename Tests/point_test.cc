
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

  This program is free software: you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  (at your option) any later version.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program. If not, see <https://www.gnu.org/licenses/>.
*/


/**
 * @file point_test.cc
 * @brief Tests for Point
 */

/**
 * @file point_test.cc
 * @brief Comprehensive test suite for point.H geometry module
 *
 * Tests cover:
 * - Point class: construction, arithmetic, comparisons, geometric predicates
 * - Polar_Point class: conversion, quadrants
 * - Segment class: construction, intersections, geometric operations
 * - Triangle class: construction, containment, area
 * - Rectangle class: construction, containment, distance
 * - Ellipse class: construction, containment, intersections
 * - Helper functions: area_of_parallelogram, pitag, trig wrappers
 */

#include <gtest/gtest.h>
#include <cmath>
#include <point.H>

using namespace Aleph;

// Tolerance for floating-point comparisons
constexpr double EPSILON = 1e-9;

// Helper to compare Geom_Number with tolerance
bool approx_equal(const Geom_Number & a, const Geom_Number & b,
                  double tol = EPSILON)
{
  return std::abs(a.get_d() - b.get_d()) < tol;
}

bool approx_equal(double a, double b, double tol = EPSILON)
{
  return std::abs(a - b) < tol;
}

//============================================================================
// Point Tests
//============================================================================

class PointTest : public ::testing::Test
{
protected:
  Point origin;
  Point p1{1, 2};
  Point p2{3, 4};
  Point p3{-1, -1};
};

TEST_F(PointTest, DefaultConstruction)
{
  EXPECT_EQ(origin.get_x(), 0);
  EXPECT_EQ(origin.get_y(), 0);
}

TEST_F(PointTest, ParameterizedConstruction)
{
  EXPECT_EQ(p1.get_x(), 1);
  EXPECT_EQ(p1.get_y(), 2);
}

TEST_F(PointTest, Equality)
{
  Point p1_copy{1, 2};
  EXPECT_EQ(p1, p1_copy);
  EXPECT_NE(p1, p2);
}

TEST_F(PointTest, Addition)
{
  Point sum = p1 + p2;
  EXPECT_EQ(sum.get_x(), 4);
  EXPECT_EQ(sum.get_y(), 6);
}

TEST_F(PointTest, AdditionAssignment)
{
  Point p = p1;
  p += p2;
  EXPECT_EQ(p.get_x(), 4);
  EXPECT_EQ(p.get_y(), 6);
}

TEST_F(PointTest, Subtraction)
{
  Point diff = p2 - p1;
  EXPECT_EQ(diff.get_x(), 2);
  EXPECT_EQ(diff.get_y(), 2);
}

TEST_F(PointTest, SubtractionAssignment)
{
  Point p = p2;
  p -= p1;
  EXPECT_EQ(p.get_x(), 2);
  EXPECT_EQ(p.get_y(), 2);
}

TEST_F(PointTest, ToString)
{
  std::string str = origin.to_string();
  EXPECT_FALSE(str.empty());
  EXPECT_NE(str.find("0"), std::string::npos);
}

TEST_F(PointTest, StringCastOperator)
{
  std::string str = static_cast<std::string>(p1);
  EXPECT_FALSE(str.empty());
}

TEST_F(PointTest, DistanceWith)
{
  Point a{0, 0};
  Point b{3, 4};
  Geom_Number dist = a.distance_with(b);
  EXPECT_TRUE(approx_equal(dist, 5.0));
}

TEST_F(PointTest, DistanceSquaredTo)
{
  Point a{0, 0};
  Point b{3, 4};
  Geom_Number dist_sq = a.distance_squared_to(b);
  EXPECT_EQ(dist_sq, 25);
}

TEST_F(PointTest, IsColinearWithPoints)
{
  Point a{0, 0};
  Point b{1, 1};
  Point c{2, 2};
  Point d{1, 0};

  EXPECT_TRUE(a.is_colinear_with(b, c));
  EXPECT_FALSE(a.is_colinear_with(b, d));
}

TEST_F(PointTest, IsToLeftFrom)
{
  Point a{0, 0};
  Point b{1, 0};
  Point left{0.5, 1};
  Point right{0.5, -1};

  EXPECT_TRUE(left.is_to_left_from(a, b));
  EXPECT_FALSE(right.is_to_left_from(a, b));
}

TEST_F(PointTest, IsToRightFrom)
{
  Point a{0, 0};
  Point b{1, 0};
  Point left{0.5, 1};
  Point right{0.5, -1};

  EXPECT_TRUE(right.is_to_right_from(a, b));
  EXPECT_FALSE(left.is_to_right_from(a, b));
}

TEST_F(PointTest, IsToLeftOnFrom)
{
  Point a{0, 0};
  Point b{1, 0};
  Point on_line{0.5, 0};
  Point left{0.5, 1};

  EXPECT_TRUE(on_line.is_to_left_on_from(a, b));
  EXPECT_TRUE(left.is_to_left_on_from(a, b));
}

TEST_F(PointTest, IsClockwiseWith)
{
  Point a{0, 0};
  Point b{1, 0};
  Point c{0.5, -1};  // Below line a-b -> clockwise

  EXPECT_TRUE(a.is_clockwise_with(b, c));
}

TEST_F(PointTest, IsBetween)
{
  Point a{0, 0};
  Point b{2, 2};
  Point between{1, 1};
  Point outside{3, 3};

  EXPECT_TRUE(between.is_between(a, b));
  EXPECT_FALSE(outside.is_between(a, b));
}

TEST_F(PointTest, NearestPoint)
{
  Point ref{0, 0};
  Point near{1, 1};
  Point far{10, 10};

  const Point & nearest = ref.nearest_point(near, far);
  EXPECT_EQ(nearest, near);
}

TEST_F(PointTest, HighestLowestLeftmostRightmost)
{
  EXPECT_EQ(p1.highest_point(), p1);
  EXPECT_EQ(p1.lowest_point(), p1);
  EXPECT_EQ(p1.leftmost_point(), p1);
  EXPECT_EQ(p1.rightmost_point(), p1);
}

//============================================================================
// Polar_Point Tests
//============================================================================

class PolarPointTest : public ::testing::Test
{
protected:
  Point cartesian{3, 4};
  Polar_Point polar{cartesian};
};

TEST_F(PolarPointTest, ConversionFromCartesian)
{
  EXPECT_TRUE(approx_equal(polar.get_r(), 5.0));
}

TEST_F(PolarPointTest, ConversionToCartesian)
{
  Point back(polar);
  EXPECT_TRUE(approx_equal(back.get_x(), 3.0));
  EXPECT_TRUE(approx_equal(back.get_y(), 4.0));
}

TEST_F(PolarPointTest, RoundTripConversion)
{
  Point original{7, 11};
  Polar_Point polar_form{original};
  Point back{polar_form};

  EXPECT_TRUE(approx_equal(back.get_x(), original.get_x()));
  EXPECT_TRUE(approx_equal(back.get_y(), original.get_y()));
}

TEST_F(PolarPointTest, QuadrantFirst)
{
  Point p{1, 1};
  Polar_Point pp{p};
  EXPECT_EQ(pp.get_quadrant(), Polar_Point::First);
}

TEST_F(PolarPointTest, QuadrantSecond)
{
  Point p{-1, 1};
  Polar_Point pp{p};
  EXPECT_EQ(pp.get_quadrant(), Polar_Point::Second);
}

TEST_F(PolarPointTest, QuadrantThird)
{
  Point p{-1, -1};
  Polar_Point pp{p};
  EXPECT_EQ(pp.get_quadrant(), Polar_Point::Third);
}

TEST_F(PolarPointTest, QuadrantFourth)
{
  Point p{1, -1};
  Polar_Point pp{p};
  EXPECT_EQ(pp.get_quadrant(), Polar_Point::Fourth);
}

TEST_F(PolarPointTest, ToString)
{
  std::string str = polar.to_string();
  EXPECT_FALSE(str.empty());
  EXPECT_EQ(str[0], '[');
}

TEST_F(PolarPointTest, DefaultConstruction)
{
  Polar_Point pp;
  EXPECT_EQ(pp.get_r(), 0);
  EXPECT_EQ(pp.get_theta(), 0);
}

//============================================================================
// Segment Tests
//============================================================================

class SegmentTest : public ::testing::Test
{
protected:
  Point origin{0, 0};
  Point p1{1, 0};
  Point p2{0, 1};
  Point p3{1, 1};

  Segment horizontal{origin, p1};
  Segment vertical{origin, p2};
  Segment diagonal{origin, p3};
};

TEST_F(SegmentTest, DefaultConstruction)
{
  Segment s;
  // Should not crash
}

TEST_F(SegmentTest, TwoPointConstruction)
{
  EXPECT_EQ(horizontal.get_src_point(), origin);
  EXPECT_EQ(horizontal.get_tgt_point(), p1);
}

TEST_F(SegmentTest, Equality)
{
  Segment s1{origin, p1};
  Segment s2{origin, p1};
  Segment s3{origin, p2};

  EXPECT_EQ(s1, s2);
  EXPECT_NE(s1, s3);
}

TEST_F(SegmentTest, Size)
{
  Geom_Number len = horizontal.size();
  EXPECT_TRUE(approx_equal(len, 1.0));

  Segment longer{Point{0, 0}, Point{3, 4}};
  EXPECT_TRUE(approx_equal(longer.size(), 5.0));
}

TEST_F(SegmentTest, Slope)
{
  EXPECT_TRUE(approx_equal(horizontal.slope(), 0.0));
  EXPECT_TRUE(approx_equal(diagonal.slope(), 1.0));
}

TEST_F(SegmentTest, MidPoint)
{
  Point mid = diagonal.mid_point();
  EXPECT_TRUE(approx_equal(mid.get_x(), 0.5));
  EXPECT_TRUE(approx_equal(mid.get_y(), 0.5));
}

TEST_F(SegmentTest, HighestPoint)
{
  Segment s{Point{0, 0}, Point{1, 2}};
  const Point & h = s.highest_point();
  EXPECT_EQ(h.get_y(), 2);
}

TEST_F(SegmentTest, LowestPoint)
{
  Segment s{Point{0, 0}, Point{1, 2}};
  const Point & l = s.lowest_point();
  EXPECT_EQ(l.get_y(), 0);
}

TEST_F(SegmentTest, LeftmostPoint)
{
  Segment s{Point{2, 0}, Point{0, 1}};
  const Point & l = s.leftmost_point();
  EXPECT_EQ(l.get_x(), 0);
}

TEST_F(SegmentTest, RightmostPoint)
{
  Segment s{Point{2, 0}, Point{0, 1}};
  const Point & r = s.rightmost_point();
  EXPECT_EQ(r.get_x(), 2);
}

TEST_F(SegmentTest, ContainsToPoint)
{
  Point inside{0.5, 0.5};
  Point outside{2, 2};

  EXPECT_TRUE(diagonal.contains_to(inside));
  EXPECT_FALSE(diagonal.contains_to(outside));
}

TEST_F(SegmentTest, IsColinearWith)
{
  Point colinear{0.5, 0.5};
  Point not_colinear{0.5, 0.6};

  EXPECT_TRUE(diagonal.is_colinear_with(colinear));
  EXPECT_FALSE(diagonal.is_colinear_with(not_colinear));
}

TEST_F(SegmentTest, IsParallelWith)
{
  Segment s1{Point{0, 0}, Point{1, 1}};
  Segment s2{Point{1, 0}, Point{2, 1}};  // Parallel
  Segment s3{Point{0, 0}, Point{1, 0}};  // Not parallel

  EXPECT_TRUE(s1.is_parallel_with(s2));
  EXPECT_FALSE(s1.is_parallel_with(s3));
}

TEST_F(SegmentTest, IntersectsWith)
{
  Segment s1{Point{0, 0}, Point{2, 2}};
  Segment s2{Point{0, 2}, Point{2, 0}};  // Cross
  Segment s3{Point{3, 0}, Point{4, 0}};  // No intersection

  EXPECT_TRUE(s1.intersects_with(s2));
  EXPECT_FALSE(s1.intersects_with(s3));
}

TEST_F(SegmentTest, IntersectsProperlyWith)
{
  Segment s1{Point{0, 0}, Point{2, 2}};
  Segment s2{Point{0, 2}, Point{2, 0}};  // Proper cross

  EXPECT_TRUE(s1.intersects_properly_with(s2));
}

TEST_F(SegmentTest, IntersectionWith)
{
  Segment s1{Point{0, 0}, Point{2, 2}};
  Segment s2{Point{0, 2}, Point{2, 0}};

  Point inter = s1.intersection_with(s2);
  EXPECT_TRUE(approx_equal(inter.get_x(), 1.0));
  EXPECT_TRUE(approx_equal(inter.get_y(), 1.0));
}

TEST_F(SegmentTest, IntersectionWithParallelThrows)
{
  Segment s1{Point{0, 0}, Point{1, 1}};
  Segment s2{Point{0, 1}, Point{1, 2}};  // Parallel

  EXPECT_THROW(s1.intersection_with(s2), std::domain_error);
}

TEST_F(SegmentTest, SenseEast)
{
  Segment s{Point{0, 0}, Point{1, 0}};
  EXPECT_EQ(s.sense(), Segment::E);
}

TEST_F(SegmentTest, SenseWest)
{
  Segment s{Point{1, 0}, Point{0, 0}};
  EXPECT_EQ(s.sense(), Segment::W);
}

TEST_F(SegmentTest, SenseNorth)
{
  Segment s{Point{0, 0}, Point{0, 1}};
  EXPECT_EQ(s.sense(), Segment::N);
}

TEST_F(SegmentTest, SenseSouth)
{
  Segment s{Point{0, 1}, Point{0, 0}};
  EXPECT_EQ(s.sense(), Segment::S);
}

TEST_F(SegmentTest, SenseNorthEast)
{
  Segment s{Point{0, 0}, Point{1, 1}};
  EXPECT_EQ(s.sense(), Segment::NE);
}

TEST_F(SegmentTest, SenseSouthWest)
{
  Segment s{Point{1, 1}, Point{0, 0}};
  EXPECT_EQ(s.sense(), Segment::SW);
}

TEST_F(SegmentTest, SenseNorthWest)
{
  Segment s{Point{1, 0}, Point{0, 1}};
  EXPECT_EQ(s.sense(), Segment::NW);
}

TEST_F(SegmentTest, SenseSouthEast)
{
  Segment s{Point{0, 1}, Point{1, 0}};
  EXPECT_EQ(s.sense(), Segment::SE);
}

TEST_F(SegmentTest, ToString)
{
  std::string str = diagonal.to_string();
  EXPECT_FALSE(str.empty());
}

TEST_F(SegmentTest, Rotate)
{
  Segment s{Point{0, 0}, Point{1, 0}};
  s.rotate(PI / 2);  // Rotate 90 degrees

  EXPECT_TRUE(approx_equal(s.get_tgt_point().get_x(), 0.0, 1e-6));
  EXPECT_TRUE(approx_equal(s.get_tgt_point().get_y(), 1.0, 1e-6));
}

TEST_F(SegmentTest, GetPerpendicular)
{
  Segment s{Point{0, 0}, Point{2, 0}};
  Point p{1, 1};

  Segment perp = s.get_perpendicular(p);
  // Perpendicular from (1,1) to horizontal line should hit (1,0)
  EXPECT_TRUE(approx_equal(perp.get_src_point().get_x(), 1.0, 1e-6));
  EXPECT_TRUE(approx_equal(perp.get_src_point().get_y(), 0.0, 1e-6));
}

TEST_F(SegmentTest, CounterclockwiseAngle)
{
  Segment s{Point{0, 0}, Point{1, 0}};
  double angle = s.counterclockwise_angle();
  EXPECT_TRUE(approx_equal(angle, 0.0, 1e-6));
}

//============================================================================
// Triangle Tests
//============================================================================

class TriangleTest : public ::testing::Test
{
protected:
  Point p1{0, 0};
  Point p2{4, 0};
  Point p3{0, 3};
  Triangle t{p1, p2, p3};
};

TEST_F(TriangleTest, ThreePointConstruction)
{
  EXPECT_EQ(t.get_p1(), p1);
  EXPECT_EQ(t.get_p2(), p2);
  EXPECT_EQ(t.get_p3(), p3);
}

TEST_F(TriangleTest, ConstructionFromPointAndSegment)
{
  Segment s{p2, p3};
  Triangle t2{p1, s};

  EXPECT_EQ(t2.get_p1(), p1);
}

TEST_F(TriangleTest, ConstructionFromSegmentAndPoint)
{
  Segment s{p1, p2};
  Triangle t2{s, p3};

  EXPECT_EQ(t2.get_p3(), p3);
}

TEST_F(TriangleTest, CollinearPointsThrow)
{
  Point a{0, 0};
  Point b{1, 1};
  Point c{2, 2};

  EXPECT_THROW(Triangle(a, b, c), std::domain_error);
}

TEST_F(TriangleTest, Area)
{
  // 3-4-5 right triangle has area = (1/2)*3*4 = 6
  Geom_Number area = t.area();
  EXPECT_EQ(area, 6);
}

TEST_F(TriangleTest, ContainsTo)
{
  Point inside{1, 1};
  Point outside{5, 5};

  EXPECT_TRUE(t.contains_to(inside));
  EXPECT_FALSE(t.contains_to(outside));
}

TEST_F(TriangleTest, HighestPoint)
{
  const Point & h = t.highest_point();
  EXPECT_EQ(h.get_y(), 3);
}

TEST_F(TriangleTest, LowestPoint)
{
  const Point & l = t.lowest_point();
  EXPECT_EQ(l.get_y(), 0);
}

TEST_F(TriangleTest, LeftmostPoint)
{
  const Point & l = t.leftmost_point();
  EXPECT_EQ(l.get_x(), 0);
}

TEST_F(TriangleTest, RightmostPoint)
{
  const Point & r = t.rightmost_point();
  EXPECT_EQ(r.get_x(), 4);
}

TEST_F(TriangleTest, IsClockwise)
{
  // Counter-clockwise triangle
  Triangle ccw{Point{0,0}, Point{1,0}, Point{0,1}};

  // Clockwise triangle
  Triangle cw{Point{0,0}, Point{0,1}, Point{1,0}};

  // Note: the is_clockwise check depends on sign of area
  EXPECT_NE(ccw.is_clockwise(), cw.is_clockwise());
}

//============================================================================
// Rectangle Tests
//============================================================================

class RectangleTest : public ::testing::Test
{
protected:
  Rectangle r{0, 0, 4, 3};
};

TEST_F(RectangleTest, DefaultConstruction)
{
  Rectangle r_default;
  EXPECT_EQ(r_default.get_xmin(), 0);
  EXPECT_EQ(r_default.get_ymin(), 0);
  EXPECT_EQ(r_default.get_xmax(), 0);
  EXPECT_EQ(r_default.get_ymax(), 0);
}

TEST_F(RectangleTest, ParameterizedConstruction)
{
  EXPECT_EQ(r.get_xmin(), 0);
  EXPECT_EQ(r.get_ymin(), 0);
  EXPECT_EQ(r.get_xmax(), 4);
  EXPECT_EQ(r.get_ymax(), 3);
}

TEST_F(RectangleTest, InvalidRectangleThrows)
{
  EXPECT_THROW(Rectangle(4, 0, 0, 3), std::range_error);  // xmax < xmin
  EXPECT_THROW(Rectangle(0, 3, 4, 0), std::range_error);  // ymax < ymin
}

TEST_F(RectangleTest, Width)
{
  EXPECT_EQ(r.width(), 4);
}

TEST_F(RectangleTest, Height)
{
  EXPECT_EQ(r.height(), 3);
}

TEST_F(RectangleTest, Contains)
{
  Point inside{2, 1};
  Point outside{5, 5};
  Point on_edge{0, 0};

  EXPECT_TRUE(r.contains(inside));
  EXPECT_FALSE(r.contains(outside));
  EXPECT_TRUE(r.contains(on_edge));
}

TEST_F(RectangleTest, Intersects)
{
  Rectangle r2{2, 1, 6, 4};  // Overlaps
  Rectangle r3{5, 5, 6, 6};  // No overlap

  EXPECT_TRUE(r.intersects(r2));
  EXPECT_FALSE(r.intersects(r3));
}

TEST_F(RectangleTest, DistanceSquaredTo)
{
  Point inside{2, 1};
  Point outside{5, 0};  // Distance 1 from right edge

  EXPECT_EQ(r.distance_squared_to(inside), 0);
  EXPECT_EQ(r.distance_squared_to(outside), 1);
}

TEST_F(RectangleTest, DistanceTo)
{
  Point outside{5, 0};
  Geom_Number dist = r.distance_to(outside);
  EXPECT_TRUE(approx_equal(dist, 1.0));
}

TEST_F(RectangleTest, SetRect)
{
  Rectangle r_mod;
  r_mod.set_rect(1, 2, 3, 4);

  EXPECT_EQ(r_mod.get_xmin(), 1);
  EXPECT_EQ(r_mod.get_ymin(), 2);
  EXPECT_EQ(r_mod.get_xmax(), 3);
  EXPECT_EQ(r_mod.get_ymax(), 4);
}

TEST_F(RectangleTest, SetRectInvalidThrows)
{
  Rectangle r_mod;
  EXPECT_THROW(r_mod.set_rect(3, 2, 1, 4), std::range_error);
}

//============================================================================
// Ellipse Tests
//============================================================================

class EllipseTest : public ::testing::Test
{
protected:
  Point center{0, 0};
  Ellipse circle{center, 1, 1};  // Unit circle
  Ellipse ellipse{center, 2, 1}; // Horizontal ellipse
};

TEST_F(EllipseTest, DefaultConstruction)
{
  Ellipse e;
  // Should not crash
}

TEST_F(EllipseTest, ParameterizedConstruction)
{
  EXPECT_EQ(ellipse.get_center(), center);
  EXPECT_EQ(ellipse.get_hradius(), 2);
  EXPECT_EQ(ellipse.get_vradius(), 1);
}

TEST_F(EllipseTest, CopyConstruction)
{
  Ellipse e2{ellipse};
  EXPECT_EQ(e2.get_center(), ellipse.get_center());
  EXPECT_EQ(e2.get_hradius(), ellipse.get_hradius());
  EXPECT_EQ(e2.get_vradius(), ellipse.get_vradius());
}

TEST_F(EllipseTest, HighestPoint)
{
  Point h = ellipse.highest_point();
  EXPECT_EQ(h.get_x(), 0);
  EXPECT_EQ(h.get_y(), 1);
}

TEST_F(EllipseTest, LowestPoint)
{
  Point l = ellipse.lowest_point();
  EXPECT_EQ(l.get_x(), 0);
  EXPECT_EQ(l.get_y(), -1);
}

TEST_F(EllipseTest, LeftmostPoint)
{
  Point l = ellipse.leftmost_point();
  EXPECT_EQ(l.get_x(), -2);
  EXPECT_EQ(l.get_y(), 0);
}

TEST_F(EllipseTest, RightmostPoint)
{
  Point r = ellipse.rightmost_point();
  EXPECT_EQ(r.get_x(), 2);
  EXPECT_EQ(r.get_y(), 0);
}

TEST_F(EllipseTest, ContainsToCenter)
{
  EXPECT_TRUE(circle.contains_to(center));
}

TEST_F(EllipseTest, ContainsToInside)
{
  Point inside{0.5, 0.5};
  EXPECT_TRUE(circle.contains_to(inside));
}

TEST_F(EllipseTest, ContainsToOutside)
{
  Point outside{2, 2};
  EXPECT_FALSE(circle.contains_to(outside));
}

TEST_F(EllipseTest, ContainsToOnBorder)
{
  Point on_border{1, 0};
  EXPECT_TRUE(circle.contains_to(on_border));
}

TEST_F(EllipseTest, IntersectsWithPointOnBorder)
{
  Point on_border{1, 0};
  EXPECT_TRUE(circle.intersects_with(on_border));
}

TEST_F(EllipseTest, IsClockwise)
{
  EXPECT_FALSE(Ellipse::is_clockwise());
}

TEST_F(EllipseTest, PointIsInsideEllipse)
{
  Point inside{0.5, 0};
  EXPECT_TRUE(inside.is_inside(circle));
}

//============================================================================
// Helper Function Tests
//============================================================================

TEST(HelperFunctionTest, AreaOfParallelogram)
{
  Point a{0, 0};
  Point b{1, 0};
  Point c{0, 1};

  Geom_Number area = area_of_parallelogram(a, b, c);
  EXPECT_EQ(area, 1);
}

TEST(HelperFunctionTest, AreaOfParallelogramNegative)
{
  Point a{0, 0};
  Point b{0, 1};
  Point c{1, 0};

  Geom_Number area = area_of_parallelogram(a, b, c);
  EXPECT_EQ(area, -1);
}

TEST(HelperFunctionTest, Pitag)
{
  Geom_Number result = pitag(3, 4);
  EXPECT_TRUE(approx_equal(result, 5.0));
}

TEST(HelperFunctionTest, PitagZero)
{
  Geom_Number result = pitag(0, 0);
  EXPECT_TRUE(approx_equal(result, 0.0));
}

TEST(HelperFunctionTest, Arctan)
{
  Geom_Number result = arctan(1);
  EXPECT_TRUE(approx_equal(result.get_d(), PI / 4, 1e-6));
}

TEST(HelperFunctionTest, Arctan2)
{
  Geom_Number result = arctan2(1, 1);
  EXPECT_TRUE(approx_equal(result.get_d(), PI / 4, 1e-6));
}

TEST(HelperFunctionTest, Sinus)
{
  Geom_Number result = sinus(Geom_Number(PI / 2));
  EXPECT_TRUE(approx_equal(result.get_d(), 1.0, 1e-6));
}

TEST(HelperFunctionTest, Cosinus)
{
  Geom_Number result = cosinus(0);
  EXPECT_TRUE(approx_equal(result.get_d(), 1.0, 1e-6));
}

TEST(HelperFunctionTest, SquareRoot)
{
  Geom_Number result = square_root(4);
  EXPECT_TRUE(approx_equal(result.get_d(), 2.0, 1e-6));
}

TEST(HelperFunctionTest, GeomNumberToDouble)
{
  Geom_Number n{3.14159};
  double d = geom_number_to_double(n);
  EXPECT_TRUE(approx_equal(d, 3.14159, 1e-4));
}

//============================================================================
// Text and String Utility Tests
//============================================================================

TEST(TextTest, AproximateStringSizeSimple)
{
  std::string str = "hello";
  size_t len = aproximate_string_size(str);
  EXPECT_EQ(len, 5);
}

TEST(TextTest, AproximateStringSizeWithLatex)
{
  std::string str = "\\alpha";  // Latex command counts as 1
  size_t len = aproximate_string_size(str);
  EXPECT_EQ(len, 1);
}

TEST(TextTest, AproximateStringSizeWithDollarSigns)
{
  std::string str = "$x$";  // $ signs are skipped
  size_t len = aproximate_string_size(str);
  EXPECT_EQ(len, 1);  // Only 'x' counts
}

TEST(TextTest, AproximateStringSizeWithBraces)
{
  std::string str = "{ab}";
  size_t len = aproximate_string_size(str);
  EXPECT_EQ(len, 2);  // Only 'a' and 'b' count
}

TEST(TextTest, TextConstruction)
{
  Point p{1, 2};
  Text t{p, "Hello"};

  EXPECT_EQ(t.get_point(), p);
  EXPECT_EQ(t.get_str(), "Hello");
  EXPECT_EQ(t.len(), 5);
}

TEST(TextTest, TextDefaultConstruction)
{
  Text t;
  // Should not crash
}

TEST(TextTest, TextBoundingPoints)
{
  Point p{1, 2};
  Text t{p, "test"};

  EXPECT_EQ(t.highest_point(), p);
  EXPECT_EQ(t.lowest_point(), p);
  EXPECT_EQ(t.leftmost_point(), p);
  EXPECT_EQ(t.rightmost_point(), p);
}

//============================================================================
// Geom_Object Tests
//============================================================================

TEST(GeomObjectTest, VirtualDestructor)
{
  // Test that virtual destructor works properly
  Geom_Object *obj = new Point{1, 2};
  delete obj;  // Should not crash
}

//============================================================================
// NullPoint Tests
//============================================================================

TEST(NullPointTest, Exists)
{
  // NullPoint should be defined and accessible
  EXPECT_EQ(NullPoint.get_x(), 0);
  EXPECT_EQ(NullPoint.get_y(), 0);
}

//============================================================================
// Edge Cases and Regression Tests
//============================================================================

TEST(EdgeCaseTest, VerticalSegmentSlope)
{
  Segment vertical{Point{0, 0}, Point{0, 1}};
  double slope = vertical.slope();
  EXPECT_GT(slope, 1e10);  // Should be very large positive
}

TEST(EdgeCaseTest, VerticalSegmentSlopeNegative)
{
  Segment vertical{Point{0, 1}, Point{0, 0}};
  double slope = vertical.slope();
  EXPECT_LT(slope, -1e10);  // Should be very large negative
}

TEST(EdgeCaseTest, SegmentConstructionWithSlopeAndLength)
{
  Point src{0, 0};
  Geom_Number slope{1};  // 45 degrees
  Geom_Number length{std::sqrt(2)};

  Segment s{src, slope, length};

  EXPECT_TRUE(approx_equal(s.size(), length, 1e-6));
}

TEST(EdgeCaseTest, ParallelSegmentOffset)
{
  Segment original{Point{0, 0}, Point{2, 0}};
  Geom_Number dist{1};

  Segment parallel{original, dist};

  // Parallel segment should have same length
  EXPECT_TRUE(approx_equal(parallel.size(), original.size(), 1e-6));
}

TEST(EdgeCaseTest, SegmentEnlargeSrc)
{
  Segment s{Point{1, 0}, Point{2, 0}};
  Geom_Number original_size = s.size();

  s.enlarge_src(1);

  // Segment should be longer now
  EXPECT_GT(s.size(), original_size);
}

TEST(EdgeCaseTest, SegmentEnlargeTgt)
{
  Segment s{Point{0, 0}, Point{1, 0}};
  Geom_Number original_size = s.size();

  s.enlarge_tgt(1);

  // Segment should be longer now
  EXPECT_GT(s.size(), original_size);
}

TEST(EdgeCaseTest, ZeroAreaTriangleFails)
{
  // Points on a line should fail to create a triangle
  EXPECT_THROW(Triangle(Point{0,0}, Point{1,1}, Point{2,2}), std::domain_error);
}

TEST(EdgeCaseTest, PointOnSegment)
{
  Segment s{Point{0, 0}, Point{2, 2}};
  Point on_segment{1, 1};

  EXPECT_TRUE(on_segment.is_inside(s));
}

TEST(EdgeCaseTest, MidPerpendicular)
{
  Segment s{Point{0, 0}, Point{2, 0}};
  Segment perp = s.mid_perpendicular(1);

  // Midpoint of perpendicular should be near midpoint of original
  Point mid = s.mid_point();
  Point perp_mid = perp.mid_point();

  EXPECT_TRUE(approx_equal(mid.distance_with(perp_mid), 0.0, 1e-6));
}

//============================================================================
// Segment-Triangle Intersection Tests
//============================================================================

TEST(SegmentTriangleTest, SegmentIntersectsTriangle)
{
  Triangle t{Point{0, 0}, Point{4, 0}, Point{2, 4}};
  Segment s{Point{2, -1}, Point{2, 5}};  // Vertical through triangle

  EXPECT_TRUE(s.intersects_with(t));
}

TEST(SegmentTriangleTest, SegmentDoesNotIntersectTriangle)
{
  Triangle t{Point{0, 0}, Point{4, 0}, Point{2, 4}};
  Segment s{Point{10, 10}, Point{11, 11}};  // Far away

  EXPECT_FALSE(s.intersects_with(t));
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}