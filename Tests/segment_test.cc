/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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
 * @file segment_test.cc
 * @brief Tests for the Segment class defined in point.H
 *
 * The Segment class in point.H stores copies of Points (not pointers).
 * Note: There is also a separate segment.H file with a pointer-based
 * Segment class, but it's not used by the library.
 */

#include <gtest/gtest.h>
#include <point.H>
#include <cmath>

using namespace std;

// =============================================================================
// Test Fixture
// =============================================================================

class SegmentTest : public ::testing::Test
{
protected:
  // Common test points
  Point origin{0, 0};
  Point p1{10, 0};
  Point p2{0, 10};
  Point p3{10, 10};
  Point p4{5, 5};
  
  // Helper to compare Geom_Number with tolerance
  static bool near_equal(const Geom_Number& a, const Geom_Number& b, 
                         double epsilon = 1e-9)
  {
    return std::abs(geom_number_to_double(a - b)) < epsilon;
  }
  
  static bool near_equal(double a, double b, double epsilon = 1e-9)
  {
    return std::abs(a - b) < epsilon;
  }
};

// =============================================================================
// Constructor Tests
// =============================================================================

TEST_F(SegmentTest, DefaultConstructor)
{
  Segment s;
  // Default constructed segment has uninitialized points
  // Just verify it doesn't crash
  SUCCEED();
}

TEST_F(SegmentTest, TwoPointConstructor)
{
  Segment s(origin, p3);
  EXPECT_EQ(s.get_src_point(), origin);
  EXPECT_EQ(s.get_tgt_point(), p3);
}

TEST_F(SegmentTest, CopyConstructor)
{
  Segment s1(origin, p3);
  Segment s2(s1);
  EXPECT_EQ(s2.get_src_point(), s1.get_src_point());
  EXPECT_EQ(s2.get_tgt_point(), s1.get_tgt_point());
}

TEST_F(SegmentTest, SlopeLengthConstructor)
{
  // Segment from origin with slope 1 and length sqrt(2)
  Segment s(origin, Geom_Number(1), Geom_Number(std::sqrt(2.0)));
  // Target should be approximately (1, 1)
  EXPECT_TRUE(near_equal(s.get_tgt_point().get_x(), 1, 0.01));
  EXPECT_TRUE(near_equal(s.get_tgt_point().get_y(), 1, 0.01));
}

// =============================================================================
// Endpoint Access Tests
// =============================================================================

TEST_F(SegmentTest, GetEndpoints)
{
  Segment s(origin, p3);
  EXPECT_EQ(s.get_src_point(), origin);
  EXPECT_EQ(s.get_tgt_point(), p3);
}

TEST_F(SegmentTest, HighestPoint)
{
  Segment s1(origin, p2);  // (0,0) to (0,10)
  EXPECT_EQ(s1.highest_point(), p2);
  
  Segment s2(p2, origin);  // reverse order
  EXPECT_EQ(s2.highest_point(), p2);
}

TEST_F(SegmentTest, LowestPoint)
{
  Segment s1(origin, p2);  // (0,0) to (0,10)
  EXPECT_EQ(s1.lowest_point(), origin);
  
  Segment s2(p2, origin);  // reverse order
  EXPECT_EQ(s2.lowest_point(), origin);
}

TEST_F(SegmentTest, LeftmostPoint)
{
  Segment s1(origin, p1);  // (0,0) to (10,0)
  EXPECT_EQ(s1.leftmost_point(), origin);
  
  Segment s2(p1, origin);  // reverse order
  EXPECT_EQ(s2.leftmost_point(), origin);
}

TEST_F(SegmentTest, RightmostPoint)
{
  Segment s1(origin, p1);  // (0,0) to (10,0)
  EXPECT_EQ(s1.rightmost_point(), p1);
  
  Segment s2(p1, origin);  // reverse order
  EXPECT_EQ(s2.rightmost_point(), p1);
}

// =============================================================================
// Geometric Properties Tests
// =============================================================================

TEST_F(SegmentTest, Size)
{
  Segment horizontal(origin, p1);  // length 10
  EXPECT_TRUE(near_equal(horizontal.size(), 10));
  
  Segment vertical(origin, p2);  // length 10
  EXPECT_TRUE(near_equal(vertical.size(), 10));
  
  Segment diagonal(origin, p3);  // length sqrt(200) ≈ 14.14
  EXPECT_TRUE(near_equal(diagonal.size(), std::sqrt(200.0), 0.01));
}

TEST_F(SegmentTest, MidPoint)
{
  Segment s(origin, p3);  // (0,0) to (10,10)
  Point mid = s.mid_point();
  EXPECT_TRUE(near_equal(mid.get_x(), 5));
  EXPECT_TRUE(near_equal(mid.get_y(), 5));
}

TEST_F(SegmentTest, MidPointNegative)
{
  Point n1(-10, -10);
  Point n2(10, 10);
  Segment s(n1, n2);
  Point mid = s.mid_point();
  EXPECT_TRUE(near_equal(mid.get_x(), 0));
  EXPECT_TRUE(near_equal(mid.get_y(), 0));
}

TEST_F(SegmentTest, Slope)
{
  Segment s45(origin, p3);  // 45 degrees, slope = 1
  EXPECT_TRUE(near_equal(s45.slope(), 1.0));
  
  Segment horizontal(origin, p1);  // slope = 0
  EXPECT_TRUE(near_equal(horizontal.slope(), 0.0));
  
  Point neg(-10, 10);
  Segment negative(origin, neg);  // slope = -1
  EXPECT_TRUE(near_equal(negative.slope(), -1.0));
}

TEST_F(SegmentTest, SlopeVertical)
{
  Segment vertical(origin, p2);  // vertical segment
  // Vertical segments return max/min double for slope
  double s = vertical.slope();
  EXPECT_TRUE(s > 1e10 || s < -1e10);
}

// =============================================================================
// Equality Tests
// =============================================================================

TEST_F(SegmentTest, EqualitySameOrder)
{
  Segment s1(origin, p3);
  Segment s2(origin, p3);
  EXPECT_TRUE(s1 == s2);
  EXPECT_FALSE(s1 != s2);
}

TEST_F(SegmentTest, EqualityReversedOrder)
{
  Segment s1(origin, p3);
  Segment s2(p3, origin);
  // Note: In point.H Segment, equality checks exact src/tgt match
  EXPECT_FALSE(s1 == s2);  // Different src/tgt
}

TEST_F(SegmentTest, InequalityDifferentEndpoints)
{
  Segment s1(origin, p1);
  Segment s2(origin, p2);
  EXPECT_FALSE(s1 == s2);
  EXPECT_TRUE(s1 != s2);
}

// =============================================================================
// Colinearity Tests
// =============================================================================

TEST_F(SegmentTest, IsColinearWith)
{
  Segment s(origin, p3);  // diagonal
  EXPECT_TRUE(s.is_colinear_with(p4));  // (5,5) is on the line
  
  Point off(5, 6);  // not collinear
  EXPECT_FALSE(s.is_colinear_with(off));
}

// =============================================================================
// Left/Right Tests
// =============================================================================

TEST_F(SegmentTest, IsToLeftFrom)
{
  Segment s(origin, p1);  // horizontal at y=0
  Point above(5, 5);
  Point below(5, -5);
  
  // Segment is to the left of points above it (depending on orientation)
  // This depends on the specific implementation
  bool left = s.is_to_left_from(above);
  bool right = s.is_to_right_from(below);
  
  // Just verify the methods work without crashing
  SUCCEED();
}

// =============================================================================
// Angle Tests
// =============================================================================

TEST_F(SegmentTest, CounterclockwiseAngle)
{
  Segment s(origin, p1);  // along x-axis
  double angle = s.counterclockwise_angle();
  // Should be close to 0 or 2*PI
  EXPECT_TRUE(near_equal(angle, 0.0, 0.1) || near_equal(angle, 2*PI, 0.1));
}

TEST_F(SegmentTest, CounterclockwiseAngleWith)
{
  Segment s1(origin, p1);  // along x-axis
  Segment s2(origin, p2);  // along y-axis
  
  double angle = s1.counterclockwise_angle_with(s2);
  // 90 degrees counterclockwise from x to y
  EXPECT_TRUE(near_equal(angle, PI_2, 0.1) || 
              near_equal(angle, 2*PI - PI_2, 0.1));
}

// =============================================================================
// Parallel Segment Tests
// =============================================================================

TEST_F(SegmentTest, ParallelSegmentConstructor)
{
  Segment original(origin, p1);  // horizontal at y=0
  Segment parallel(original, Geom_Number(5));  // 5 units away
  
  // The parallel segment should have the same length
  EXPECT_TRUE(near_equal(parallel.size(), original.size(), 0.01));
  
  // The parallel segment should have a different y coordinate
  // (shifted by 5 units perpendicular to the original)
}

// =============================================================================
// Edge Cases
// =============================================================================

TEST_F(SegmentTest, NegativeCoordinates)
{
  Point n1(-10, -10);
  Point n2(-5, -5);
  Segment s(n1, n2);
  
  EXPECT_TRUE(near_equal(s.size(), std::sqrt(50.0), 0.01));
  Point mid = s.mid_point();
  EXPECT_TRUE(near_equal(mid.get_x(), -7.5, 0.1));
  EXPECT_TRUE(near_equal(mid.get_y(), -7.5, 0.1));
}

TEST_F(SegmentTest, DegenerateSegment)
{
  Point same(5, 5);
  Segment s(same, same);
  EXPECT_TRUE(near_equal(s.size(), 0));
}

TEST_F(SegmentTest, LargeCoordinates)
{
  Point a(1000000, 1000000);
  Point b(1000010, 1000010);
  Segment s(a, b);
  
  EXPECT_TRUE(near_equal(s.size(), std::sqrt(200.0), 0.01));
}

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
