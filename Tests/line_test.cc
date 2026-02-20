
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


/** @file line_test.cc
 *  @brief Comprehensive unit tests for line.H
 */

#include <gtest/gtest.h>
#include <cmath>
#include <line.H>

using namespace Aleph;

//============================================================================
// Construction Tests
//============================================================================

class LineConstructorTest : public ::testing::Test {};

TEST_F(LineConstructorTest, DefaultConstructor)
{
  LineEq line;

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y0), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.m), 1.0);

  // Default line is y = x
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(1))), 1.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(5))), 5.0);
}

TEST_F(LineConstructorTest, SlopeInterceptConstructor)
{
  LineEq line(Geom_Number(5), Geom_Number(2));  // y = 5 + 2x

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 5.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 2.0);

  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 5.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(1))), 7.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(3))), 11.0);
}

TEST_F(LineConstructorTest, PointSlopeConstructor)
{
  // Line through (2, 4) with slope 3
  // y = y0 + 3x, where y0 = 4 - 3*2 = -2
  LineEq line(Geom_Number(2), Geom_Number(4), Geom_Number(3));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 3.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), -2.0);

  // Verify it passes through (2, 4)
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(2))), 4.0);
}

TEST_F(LineConstructorTest, TwoPointConstructor)
{
  // Line through (0, 0) and (2, 6) -> slope = 3
  LineEq line(Geom_Number(0), Geom_Number(0), Geom_Number(2), Geom_Number(6));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 3.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 0.0);

  // Verify it passes through both points
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(2))), 6.0);
}

TEST_F(LineConstructorTest, TwoPointConstructorNegativeSlope)
{
  // Line through (0, 4) and (2, 0) -> slope = -2
  LineEq line(Geom_Number(0), Geom_Number(4), Geom_Number(2), Geom_Number(0));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), -2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 4.0);
}

TEST_F(LineConstructorTest, TwoPointConstructorHorizontal)
{
  // Horizontal line through (0, 5) and (10, 5)
  LineEq line(Geom_Number(0), Geom_Number(5), Geom_Number(10), Geom_Number(5));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 5.0);
  EXPECT_TRUE(line.is_horizontal());
}

TEST_F(LineConstructorTest, TwoPointConstructorVerticalThrows)
{
  // Vertical line (same x) should throw
  EXPECT_THROW(LineEq(Geom_Number(3), Geom_Number(0), Geom_Number(3), Geom_Number(10)), std::domain_error);
}

TEST_F(LineConstructorTest, TwoPointConstructorSamePointThrows)
{
  // Same point should throw
  EXPECT_THROW(LineEq(Geom_Number(3), Geom_Number(5), Geom_Number(3), Geom_Number(5)), std::domain_error);
}

//============================================================================
// Evaluation Tests
//============================================================================

class LineEvaluationTest : public ::testing::Test {};

TEST_F(LineEvaluationTest, EvaluateAtZero)
{
  LineEq line(Geom_Number(5), Geom_Number(2));  // y = 5 + 2x
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 5.0);
}

TEST_F(LineEvaluationTest, EvaluateAtPositive)
{
  LineEq line(Geom_Number(1), Geom_Number(3));  // y = 1 + 3x
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(2))), 7.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(10))), 31.0);
}

TEST_F(LineEvaluationTest, EvaluateAtNegative)
{
  LineEq line(Geom_Number(1), Geom_Number(3));  // y = 1 + 3x
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(-2))), -5.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(-1))), -2.0);
}

TEST_F(LineEvaluationTest, EvaluateHorizontalLine)
{
  LineEq line(Geom_Number(7), Geom_Number(0));  // y = 7
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(-100))), 7.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 7.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(100))), 7.0);
}

TEST_F(LineEvaluationTest, XAtY)
{
  LineEq line(Geom_Number(0), Geom_Number(2));  // y = 2x -> x = y/2
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.x_at(Geom_Number(4))), 2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.x_at(Geom_Number(0))), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.x_at(Geom_Number(-6))), -3.0);
}

TEST_F(LineEvaluationTest, XAtYHorizontalThrows)
{
  LineEq line(Geom_Number(5), Geom_Number(0));  // y = 5 (horizontal)
  EXPECT_THROW((void)line.x_at(Geom_Number(5)), std::domain_error);
}

//============================================================================
// Property Tests
//============================================================================

class LinePropertyTest : public ::testing::Test {};

TEST_F(LinePropertyTest, SlopeGetter)
{
  LineEq line(Geom_Number(3), Geom_Number(5, 2));  // 2.5 = 5/2
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 2.5);
}

TEST_F(LinePropertyTest, YInterceptGetter)
{
  LineEq line(Geom_Number(3), Geom_Number(5, 2));  // 2.5 = 5/2
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 3.0);
}

TEST_F(LinePropertyTest, IsHorizontal)
{
  LineEq horizontal(Geom_Number(5), Geom_Number(0));
  LineEq nearly_horizontal(Geom_Number(5), Geom_Number(1e-15));  // 1e-15
  LineEq not_horizontal(Geom_Number(5), Geom_Number(1, 10));  // 0.1

  EXPECT_TRUE(horizontal.is_horizontal());
  EXPECT_TRUE(nearly_horizontal.is_horizontal(Geom_Number(1e-14)));  // 1e-14
  EXPECT_FALSE(not_horizontal.is_horizontal());
}

TEST_F(LinePropertyTest, IsParallelTo)
{
  LineEq l1(Geom_Number(0), Geom_Number(2));  // y = 2x
  LineEq l2(Geom_Number(5), Geom_Number(2));  // y = 5 + 2x
  LineEq l3(Geom_Number(0), Geom_Number(3));  // y = 3x

  EXPECT_TRUE(l1.is_parallel_to(l2));
  EXPECT_TRUE(l2.is_parallel_to(l1));
  EXPECT_FALSE(l1.is_parallel_to(l3));
}

TEST_F(LinePropertyTest, IsPerpendicularTo)
{
  LineEq l1(Geom_Number(0), Geom_Number(2));   // y = 2x
  LineEq l2(Geom_Number(0), Geom_Number(-1, 2));  // y = -0.5x (perpendicular: 2 * -0.5 = -1)
  LineEq l3(Geom_Number(0), Geom_Number(1));   // y = x (not perpendicular)

  EXPECT_TRUE(l1.is_perpendicular_to(l2));
  EXPECT_TRUE(l2.is_perpendicular_to(l1));
  EXPECT_FALSE(l1.is_perpendicular_to(l3));
}

//============================================================================
// Intersection Tests
//============================================================================

class LineIntersectionTest : public ::testing::Test {};

TEST_F(LineIntersectionTest, SimpleIntersection)
{
  LineEq l1(Geom_Number(0), Geom_Number(1));   // y = x
  LineEq l2(Geom_Number(2), Geom_Number(-1));  // y = 2 - x

  // Intersection: x = 2 - x -> 2x = 2 -> x = 1, y = 1
  auto [x, y] = l1.intersection(l2);

  EXPECT_DOUBLE_EQ(geom_number_to_double(x), 1.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y), 1.0);
}

TEST_F(LineIntersectionTest, IntersectionAtOrigin)
{
  LineEq l1(Geom_Number(0), Geom_Number(1));   // y = x
  LineEq l2(Geom_Number(0), Geom_Number(-1));  // y = -x

  auto [x, y] = l1.intersection(l2);

  EXPECT_DOUBLE_EQ(geom_number_to_double(x), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y), 0.0);
}

TEST_F(LineIntersectionTest, IntersectionNegativeCoordinates)
{
  LineEq l1(Geom_Number(2), Geom_Number(1));   // y = 2 + x
  LineEq l2(Geom_Number(4), Geom_Number(-1));  // y = 4 - x

  // 2 + x = 4 - x -> 2x = 2 -> x = 1, y = 3
  auto [x, y] = l1.intersection(l2);

  EXPECT_DOUBLE_EQ(geom_number_to_double(x), 1.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y), 3.0);
}

TEST_F(LineIntersectionTest, IntersectionWithHorizontal)
{
  LineEq diagonal(Geom_Number(0), Geom_Number(2));  // y = 2x
  LineEq horizontal(Geom_Number(4), Geom_Number(0));  // y = 4

  // 4 = 2x -> x = 2
  auto [x, y] = diagonal.intersection(horizontal);

  EXPECT_DOUBLE_EQ(geom_number_to_double(x), 2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y), 4.0);
}

TEST_F(LineIntersectionTest, ParallelLinesThrow)
{
  LineEq l1(Geom_Number(0), Geom_Number(2));  // y = 2x
  LineEq l2(Geom_Number(5), Geom_Number(2));  // y = 5 + 2x (parallel)

  EXPECT_THROW((void)l1.intersection(l2), std::domain_error);
}

TEST_F(LineIntersectionTest, IdenticalLinesThrow)
{
  LineEq l1(Geom_Number(3), Geom_Number(2));
  LineEq l2(Geom_Number(3), Geom_Number(2));  // Same line

  EXPECT_THROW((void)l1.intersection(l2), std::domain_error);
}

//============================================================================
// Perpendicular Through Point Tests
//============================================================================

class LinePerpendicularTest : public ::testing::Test {};

TEST_F(LinePerpendicularTest, PerpendicularThrough)
{
  LineEq l1(Geom_Number(0), Geom_Number(2));  // y = 2x

  // Perpendicular through (1, 2)
  LineEq perp = l1.perpendicular_through(Geom_Number(1), Geom_Number(2));

  EXPECT_DOUBLE_EQ(geom_number_to_double(perp.slope()), -0.5);  // -1/2
  EXPECT_TRUE(l1.is_perpendicular_to(perp));
  EXPECT_DOUBLE_EQ(geom_number_to_double(perp(Geom_Number(1))), 2.0);  // Passes through (1, 2)
}

TEST_F(LinePerpendicularTest, PerpendicularThroughOrigin)
{
  LineEq l1(Geom_Number(0), Geom_Number(1));  // y = x

  LineEq perp = l1.perpendicular_through(Geom_Number(0), Geom_Number(0));

  EXPECT_DOUBLE_EQ(geom_number_to_double(perp.slope()), -1.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(perp.y_intercept()), 0.0);
}

TEST_F(LinePerpendicularTest, PerpendicularToHorizontalThrows)
{
  LineEq horizontal(Geom_Number(5), Geom_Number(0));  // y = 5

  // Perpendicular would be vertical (undefined)
  EXPECT_THROW((void)horizontal.perpendicular_through(Geom_Number(1), Geom_Number(5)), std::domain_error);
}

//============================================================================
// Distance Tests
//============================================================================

class LineDistanceTest : public ::testing::Test {};

TEST_F(LineDistanceTest, DistanceToPointOnLine)
{
  LineEq line(Geom_Number(0), Geom_Number(1));  // y = x

  // Point (3, 3) is on the line
  EXPECT_NEAR(geom_number_to_double(line.distance_to(Geom_Number(3), Geom_Number(3))), 0.0, 1e-10);
}

TEST_F(LineDistanceTest, DistanceToHorizontalLine)
{
  LineEq line(Geom_Number(5), Geom_Number(0));  // y = 5

  // Distance from (0, 10) to y = 5 is 5
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.distance_to(Geom_Number(0), Geom_Number(10))), 5.0);

  // Distance from (100, 0) to y = 5 is 5
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.distance_to(Geom_Number(100), Geom_Number(0))), 5.0);
}

TEST_F(LineDistanceTest, DistanceToSlantedLine)
{
  LineEq line(Geom_Number(0), Geom_Number(1));  // y = x

  // Distance from (0, 1) to y = x
  // Formula: |y - mx - b| / sqrt(1 + m^2) = |1 - 0 - 0| / sqrt(2) = 1/sqrt(2)
  double expected = 1.0 / std::sqrt(2.0);
  EXPECT_NEAR(geom_number_to_double(line.distance_to(Geom_Number(0), Geom_Number(1))), expected, 1e-10);
}

//============================================================================
// Contains Point Tests
//============================================================================

class LineContainsTest : public ::testing::Test {};

TEST_F(LineContainsTest, ContainsPointOnLine)
{
  LineEq line(Geom_Number(0), Geom_Number(2));  // y = 2x

  EXPECT_TRUE(line.contains_point(Geom_Number(0), Geom_Number(0)));
  EXPECT_TRUE(line.contains_point(Geom_Number(1), Geom_Number(2)));
  EXPECT_TRUE(line.contains_point(Geom_Number(-3), Geom_Number(-6)));
}

TEST_F(LineContainsTest, DoesNotContainPointOffLine)
{
  LineEq line(Geom_Number(0), Geom_Number(2));  // y = 2x

  EXPECT_FALSE(line.contains_point(Geom_Number(1), Geom_Number(1)));  // y should be 2
  EXPECT_FALSE(line.contains_point(Geom_Number(0), Geom_Number(1)));  // y should be 0
}

TEST_F(LineContainsTest, ContainsPointWithTolerance)
{
  LineEq line(Geom_Number(0), Geom_Number(1));  // y = x

  // Point (1, 1.0000001) is almost on the line
  EXPECT_TRUE(line.contains_point(Geom_Number(1), Geom_Number(1.00000000001), Geom_Number(1e-10)));
  EXPECT_FALSE(line.contains_point(Geom_Number(1), Geom_Number(100001, 100000), Geom_Number(1e-10)));  // Too far
}

//============================================================================
// Equality Tests
//============================================================================

class LineEqualityTest : public ::testing::Test {};

TEST_F(LineEqualityTest, EqualLines)
{
  LineEq l1(Geom_Number(3), Geom_Number(2));
  LineEq l2(Geom_Number(3), Geom_Number(2));

  EXPECT_TRUE(l1 == l2);
  EXPECT_FALSE(l1 != l2);
}

TEST_F(LineEqualityTest, NearlyEqualLines)
{
  LineEq l1(Geom_Number(3), Geom_Number(2));
  LineEq l2(Geom_Number(3.000000000001), Geom_Number(2.000000000001));

  EXPECT_TRUE(l1.approx_equal(l2, Geom_Number(1e-10)));  // Within epsilon
}

TEST_F(LineEqualityTest, DifferentSlope)
{
  LineEq l1(Geom_Number(3), Geom_Number(2));
  LineEq l2(Geom_Number(3), Geom_Number(5, 2));  // 2.5 = 5/2

  EXPECT_FALSE(l1 == l2);
  EXPECT_TRUE(l1 != l2);
}

TEST_F(LineEqualityTest, DifferentIntercept)
{
  LineEq l1(Geom_Number(3), Geom_Number(2));
  LineEq l2(Geom_Number(4), Geom_Number(2));

  EXPECT_FALSE(l1 == l2);
  EXPECT_TRUE(l1 != l2);
}

//============================================================================
// String Conversion Tests
//============================================================================

class LineStringTest : public ::testing::Test {};

TEST_F(LineStringTest, ToString)
{
  LineEq line(Geom_Number(3), Geom_Number(2));
  std::string str = line.to_string();

  EXPECT_TRUE(str.find("y = ") != std::string::npos);
  EXPECT_TRUE(str.find("3") != std::string::npos);
  EXPECT_TRUE(str.find("2") != std::string::npos);
}

TEST_F(LineStringTest, StreamOutput)
{
  LineEq line(Geom_Number(3, 2), Geom_Number(5, 2));  // 1.5 = 3/2, 2.5 = 5/2
  std::ostringstream ss;
  ss << line;

  EXPECT_TRUE(ss.str().find("1.5") != std::string::npos || ss.str().find("3/2") != std::string::npos);
  EXPECT_TRUE(ss.str().find("2.5") != std::string::npos || ss.str().find("5/2") != std::string::npos);
}

//============================================================================
// Edge Cases and Stress Tests
//============================================================================

class LineEdgeCaseTest : public ::testing::Test {};

TEST_F(LineEdgeCaseTest, VeryLargeSlope)
{
  LineEq line(Geom_Number(0), Geom_Number(1e10));  // Very steep line

  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(1))), 1e10);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(-1))), -1e10);
}

TEST_F(LineEdgeCaseTest, VerySmallSlope)
{
  LineEq line(Geom_Number(5), Geom_Number(1e-10));  // Nearly horizontal (1e-10)

  EXPECT_NEAR(geom_number_to_double(line(Geom_Number(1))), 5.0, 1e-9);
  EXPECT_TRUE(line.is_horizontal(Geom_Number(1e-8)));  // 1e-8
}

TEST_F(LineEdgeCaseTest, NegativeYIntercept)
{
  LineEq line(Geom_Number(-100), Geom_Number(1));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), -100.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(100))), 0.0);
}

TEST_F(LineEdgeCaseTest, NegativeSlope)
{
  LineEq line(Geom_Number(10), Geom_Number(-2));  // y = 10 - 2x

  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(0))), 10.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(5))), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line(Geom_Number(10))), -10.0);
}

TEST_F(LineEdgeCaseTest, PointsInReverseOrder)
{
  // Points given with x2 < x1: (5, 10) and (0, 0)
  // Slope = (0 - 10) / (0 - 5) = -10 / -5 = 2
  LineEq line(Geom_Number(5), Geom_Number(10), Geom_Number(0), Geom_Number(0));

  EXPECT_DOUBLE_EQ(geom_number_to_double(line.slope()), 2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y_intercept()), 0.0);  // Passes through origin
}

//============================================================================
// Constexpr Tests - Note: Geom_Number (mpq_class) is not constexpr
//============================================================================

class LineConstexprTest : public ::testing::Test {};

TEST_F(LineConstexprTest, DefaultConstructorValues)
{
  LineEq line;
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y0), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.m), 1.0);
}

TEST_F(LineConstexprTest, SlopeInterceptConstructorValues)
{
  LineEq line(Geom_Number(5), Geom_Number(2));
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.y0), 5.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(line.m), 2.0);
}

TEST_F(LineConstexprTest, EvaluationValues)
{
  LineEq line(Geom_Number(1), Geom_Number(2));
  Geom_Number y = line(Geom_Number(3));
  EXPECT_DOUBLE_EQ(geom_number_to_double(y), 7.0);
}

//============================================================================
// Type Traits Tests
//============================================================================

TEST(LineTypeTraits, NoDiscardAttribute)
{
  // Verify [[nodiscard]] functions return expected types
  LineEq line(Geom_Number(0), Geom_Number(1));

  EXPECT_TRUE((std::is_same<decltype(line.slope()), const Geom_Number &>::value));
  EXPECT_TRUE((std::is_same<decltype(line.y_intercept()), const Geom_Number &>::value));
  EXPECT_TRUE((std::is_same<decltype(line.is_horizontal()), bool>::value));
  EXPECT_TRUE((std::is_same<decltype(line.to_string()), std::string>::value));
}

//============================================================================
// Integration Tests
//============================================================================

class LineIntegrationTest : public ::testing::Test {};

TEST_F(LineIntegrationTest, TriangleFromThreeLines)
{
  // Create three lines forming a triangle
  LineEq l1(Geom_Number(0), Geom_Number(1));   // y = x
  LineEq l2(Geom_Number(0), Geom_Number(-1));  // y = -x
  LineEq l3(Geom_Number(2), Geom_Number(0));   // y = 2

  auto [x1, y1] = l1.intersection(l2);  // (0, 0)
  auto [x2, y2] = l1.intersection(l3);  // (2, 2)
  auto [x3, y3] = l2.intersection(l3);  // (-2, 2)

  EXPECT_DOUBLE_EQ(geom_number_to_double(x1), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y1), 0.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(x2), 2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y2), 2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(x3), -2.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(y3), 2.0);
}

TEST_F(LineIntegrationTest, LineFromPointsAndIntersection)
{
  // Create line through (0, 0) and (4, 8) -> y = 2x
  LineEq l1(Geom_Number(0), Geom_Number(0), Geom_Number(4), Geom_Number(8));

  // Create line through (0, 4) and (4, 0) -> y = 4 - x
  LineEq l2(Geom_Number(0), Geom_Number(4), Geom_Number(4), Geom_Number(0));

  // Find intersection
  auto [x, y] = l1.intersection(l2);

  // 2x = 4 - x -> 3x = 4 -> x = 4/3
  EXPECT_NEAR(geom_number_to_double(x), 4.0/3.0, 1e-10);
  EXPECT_NEAR(geom_number_to_double(y), 8.0/3.0, 1e-10);
}

TEST_F(LineIntegrationTest, PerpendicularBisector)
{
  // Find perpendicular bisector of segment from (0, 0) to (4, 4)
  // Midpoint: (2, 2)
  // Original line: y = x, slope = 1
  // Perpendicular slope: -1

  LineEq original(Geom_Number(0), Geom_Number(0), Geom_Number(4), Geom_Number(4));  // y = x
  LineEq perp = original.perpendicular_through(Geom_Number(2), Geom_Number(2));

  EXPECT_DOUBLE_EQ(geom_number_to_double(perp.slope()), -1.0);
  EXPECT_DOUBLE_EQ(geom_number_to_double(perp(Geom_Number(2))), 2.0);  // Passes through midpoint

  // Verify it's equidistant from both endpoints
  EXPECT_NEAR(geom_number_to_double(perp.distance_to(Geom_Number(0), Geom_Number(0))),
              geom_number_to_double(perp.distance_to(Geom_Number(4), Geom_Number(4))), 1e-10);
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
