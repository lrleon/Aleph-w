
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


/**
 * @file robust_predicates_test.cc
 * @brief Tests for robust geometry predicates in point.H
 *
 * Tests orientation(), in_circle(), on_segment(), segments_intersect(),
 * segment_intersection_point(), is_parallel_with(), and area_of_triangle().
 */

#include <gtest/gtest.h>
#include <point.H>

// ===================== Orientation tests =====================

TEST(Orientation, CCW)
{
  Point a(0, 0), b(1, 0), c(0, 1);
  EXPECT_EQ(orientation(a, b, c), Orientation::CCW);
}

TEST(Orientation, CW)
{
  Point a(0, 0), b(0, 1), c(1, 0);
  EXPECT_EQ(orientation(a, b, c), Orientation::CW);
}

TEST(Orientation, Collinear)
{
  Point a(0, 0), b(1, 1), c(2, 2);
  EXPECT_EQ(orientation(a, b, c), Orientation::COLLINEAR);
}

TEST(Orientation, DegenerateCoincident)
{
  Point a(3, 5), b(3, 5), c(7, 2);
  EXPECT_EQ(orientation(a, b, c), Orientation::COLLINEAR);
}

// ===================== in_circle tests =====================

TEST(InCircle, InsideCCW)
{
  Point a(0, 0), b(4, 0), c(0, 4);
  Point p(1, 1);
  EXPECT_EQ(in_circle(a, b, c, p), InCircleResult::INSIDE);
}

TEST(InCircle, InsideCW)
{
  Point a(0, 0), b(0, 4), c(4, 0);
  Point p(1, 1);
  EXPECT_EQ(in_circle(a, b, c, p), InCircleResult::INSIDE);
}

TEST(InCircle, OnCircle)
{
  Point a(0, 0), b(4, 0), c(0, 4);
  Point p(4, 4);
  EXPECT_EQ(in_circle(a, b, c, p), InCircleResult::ON_CIRCLE);
}

TEST(InCircle, Outside)
{
  Point a(0, 0), b(4, 0), c(0, 4);
  Point p(5, 5);
  EXPECT_EQ(in_circle(a, b, c, p), InCircleResult::OUTSIDE);
}

TEST(InCircle, DegenerateTriangleCollinear)
{
  Point a(0, 0), b(1, 1), c(2, 2);
  Point p(0, 3);
  EXPECT_EQ(in_circle(a, b, c, p), InCircleResult::DEGENERATE);
}

TEST(InCircleDeterminant, SignDependsOnTriangleOrientation)
{
  Point a(0, 0), b(4, 0), c(0, 4);
  Point p(1, 1);

  const Geom_Number det_ccw = in_circle_determinant(a, b, c, p);
  const Geom_Number det_cw = in_circle_determinant(a, c, b, p);

  EXPECT_GT(det_ccw, 0);
  EXPECT_LT(det_cw, 0);
}

// ===================== on_segment tests =====================

TEST(OnSegment, Endpoint)
{
  Segment s(Point(0, 0), Point(4, 4));
  EXPECT_TRUE(on_segment(s, Point(0, 0)));
  EXPECT_TRUE(on_segment(s, Point(4, 4)));
}

TEST(OnSegment, Interior)
{
  Segment s(Point(0, 0), Point(4, 4));
  EXPECT_TRUE(on_segment(s, Point(2, 2)));
}

TEST(OnSegment, Outside)
{
  Segment s(Point(0, 0), Point(4, 4));
  EXPECT_FALSE(on_segment(s, Point(5, 5)));
}

TEST(OnSegment, NotCollinear)
{
  Segment s(Point(0, 0), Point(4, 4));
  EXPECT_FALSE(on_segment(s, Point(1, 2)));
}

// ============== segments_intersect tests =================

TEST(SegmentsIntersect, ProperCross)
{
  Segment s1(Point(0, 0), Point(2, 2));
  Segment s2(Point(0, 2), Point(2, 0));
  EXPECT_TRUE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, TShaped)
{
  // endpoint of s2 touches the interior of s1
  Segment s1(Point(0, 0), Point(4, 0));
  Segment s2(Point(2, 0), Point(2, 3));
  EXPECT_TRUE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, SharedEndpoint)
{
  Segment s1(Point(0, 0), Point(1, 1));
  Segment s2(Point(1, 1), Point(2, 0));
  EXPECT_TRUE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, CollinearOverlap)
{
  Segment s1(Point(0, 0), Point(3, 0));
  Segment s2(Point(2, 0), Point(5, 0));
  EXPECT_TRUE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, CollinearDisjoint)
{
  Segment s1(Point(0, 0), Point(1, 0));
  Segment s2(Point(2, 0), Point(3, 0));
  EXPECT_FALSE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, Parallel)
{
  Segment s1(Point(0, 0), Point(2, 0));
  Segment s2(Point(0, 1), Point(2, 1));
  EXPECT_FALSE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, Disjoint)
{
  Segment s1(Point(0, 0), Point(1, 1));
  Segment s2(Point(3, 3), Point(4, 5));
  EXPECT_FALSE(segments_intersect(s1, s2));
}

TEST(SegmentsIntersect, FourPointOverload)
{
  EXPECT_TRUE(segments_intersect(Point(0, 0), Point(2, 2),
                                 Point(0, 2), Point(2, 0)));
  EXPECT_FALSE(segments_intersect(Point(0, 0), Point(1, 0),
                                  Point(0, 1), Point(1, 1)));
}

// =========== segment_intersection_point tests ============

TEST(IntersectionPoint, SimpleXCross)
{
  Segment s1(Point(0, 0), Point(2, 2));
  Segment s2(Point(0, 2), Point(2, 0));
  Point p = segment_intersection_point(s1, s2);
  EXPECT_EQ(p.get_x(), 1);
  EXPECT_EQ(p.get_y(), 1);
}

TEST(IntersectionPoint, ExactRational)
{
  // Line 1: (0,0) -> (3,0)  (horizontal, y=0)
  // Line 2: (1,1) -> (2,-1) (slope = -2, y = -2(x-1)+1 = -2x+3)
  // Intersection: -2x+3=0 => x=3/2, y=0
  Segment s1(Point(0, 0), Point(3, 0));
  Segment s2(Point(1, 1), Point(2, -1));
  Point p = segment_intersection_point(s1, s2);
  EXPECT_EQ(p.get_x(), Geom_Number(3, 2));
  EXPECT_EQ(p.get_y(), 0);
}

TEST(IntersectionPoint, ExactRationalNonTrivial)
{
  // Line 1: (0,0) -> (7,2)    direction (7,2)
  // Line 2: (0,3) -> (3,0)    direction (3,-3)
  // Parametric: p = t*(7,2), q = (0,3)+s*(3,-3)
  // 7t = 3s => s = 7t/3
  // 2t = 3 - 3s = 3 - 7t => 9t = 3 => t = 1/3
  // intersection = (7/3, 2/3)
  Segment s1(Point(0, 0), Point(7, 2));
  Segment s2(Point(0, 3), Point(3, 0));
  Point p = segment_intersection_point(s1, s2);
  EXPECT_EQ(p.get_x(), Geom_Number(7, 3));
  EXPECT_EQ(p.get_y(), Geom_Number(2, 3));
}

TEST(IntersectionPoint, VerticalSegment)
{
  Segment vertical(Point(2, 0), Point(2, 4));
  Segment diagonal(Point(0, 0), Point(4, 4));
  Point p = segment_intersection_point(vertical, diagonal);
  EXPECT_EQ(p.get_x(), 2);
  EXPECT_EQ(p.get_y(), 2);
}

TEST(IntersectionPoint, HorizontalSegment)
{
  Segment horizontal(Point(0, 3), Point(6, 3));
  Segment diagonal(Point(0, 0), Point(6, 6));
  Point p = segment_intersection_point(horizontal, diagonal);
  EXPECT_EQ(p.get_x(), 3);
  EXPECT_EQ(p.get_y(), 3);
}

TEST(IntersectionPoint, ParallelThrows)
{
  Segment s1(Point(0, 0), Point(2, 2));
  Segment s2(Point(0, 1), Point(2, 3));
  EXPECT_THROW(segment_intersection_point(s1, s2), std::domain_error);
}

TEST(IntersectionPoint, DisjointNonParallelThrows)
{
  Segment s1(Point(0, 0), Point(1, 0));
  Segment s2(Point(2, -1), Point(2, 1));
  EXPECT_FALSE(segments_intersect(s1, s2));
  EXPECT_THROW(segment_intersection_point(s1, s2), std::domain_error);
}

TEST(IntersectionPoint, CollinearOverlapThrows)
{
  Segment s1(Point(0, 0), Point(3, 0));
  Segment s2(Point(2, 0), Point(5, 0));
  EXPECT_TRUE(segments_intersect(s1, s2));
  EXPECT_THROW(segment_intersection_point(s1, s2), std::domain_error);
}

TEST(IntersectionPoint, CollinearTouchingAtEndpointReturnsUniquePoint)
{
  Segment s1(Point(0, 0), Point(2, 0));
  Segment s2(Point(2, 0), Point(5, 0));
  Point p = segment_intersection_point(s1, s2);
  EXPECT_EQ(p.get_x(), 2);
  EXPECT_EQ(p.get_y(), 0);
}

TEST(IntersectionPoint, DegeneratePointOnSegmentReturnsThatPoint)
{
  Segment point_seg(Point(2, 2), Point(2, 2));
  Segment diag(Point(0, 0), Point(4, 4));
  Point p = segment_intersection_point(point_seg, diag);
  EXPECT_EQ(p.get_x(), 2);
  EXPECT_EQ(p.get_y(), 2);
}

TEST(IntersectionPoint, DegenerateIdenticalPointsReturnThatPoint)
{
  Segment s1(Point(3, -1), Point(3, -1));
  Segment s2(Point(3, -1), Point(3, -1));
  Point p = segment_intersection_point(s1, s2);
  EXPECT_EQ(p.get_x(), 3);
  EXPECT_EQ(p.get_y(), -1);
}

// ============== is_parallel_with tests ===================

TEST(IsParallel, ExactParallel)
{
  Segment s1(Point(0, 0), Point(3, 6));
  Segment s2(Point(1, 0), Point(4, 6));
  EXPECT_TRUE(s1.is_parallel_with(s2));
}

TEST(IsParallel, AntiParallel)
{
  Segment s1(Point(0, 0), Point(2, 4));
  Segment s2(Point(4, 8), Point(2, 4));
  EXPECT_TRUE(s1.is_parallel_with(s2));
}

TEST(IsParallel, NotParallel)
{
  Segment s1(Point(0, 0), Point(1, 1));
  Segment s2(Point(0, 0), Point(1, 2));
  EXPECT_FALSE(s1.is_parallel_with(s2));
}

TEST(IsParallel, VerticalPair)
{
  Segment s1(Point(0, 0), Point(0, 5));
  Segment s2(Point(3, 1), Point(3, 7));
  EXPECT_TRUE(s1.is_parallel_with(s2));
}

TEST(IsParallel, HorizontalPair)
{
  Segment s1(Point(0, 0), Point(5, 0));
  Segment s2(Point(0, 3), Point(5, 3));
  EXPECT_TRUE(s1.is_parallel_with(s2));
}

// ============== area_of_triangle tests ====================

TEST(TriangleArea, Known)
{
  // right triangle with legs 3 and 4 => area = 6
  Point a(0, 0), b(3, 0), c(0, 4);
  EXPECT_EQ(area_of_triangle(a, b, c), 6);
}

TEST(TriangleArea, Degenerate)
{
  Point a(0, 0), b(1, 1), c(2, 2);
  EXPECT_EQ(area_of_triangle(a, b, c), 0);
}

TEST(TriangleArea, ExactRational)
{
  // Triangle (0,0), (1,0), (0,1) => area = 1/2
  Point a(0, 0), b(1, 0), c(0, 1);
  EXPECT_EQ(area_of_triangle(a, b, c), Geom_Number(1, 2));
}
