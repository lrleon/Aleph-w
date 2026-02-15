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
#include <random>

using namespace Aleph;

// ---------- Basic tests ----------

TEST_F(GeomAlgorithmsTest, MEC_SinglePoint)
{
  DynList<Point> pts;
  pts.append(Point(3, 7));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  EXPECT_EQ(c.center, Point(3, 7));
  EXPECT_EQ(c.radius_squared, 0);
}

TEST_F(GeomAlgorithmsTest, MEC_TwoPoints)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  EXPECT_EQ(c.center, Point(2, 0));
  EXPECT_EQ(c.radius_squared, 4);
  EXPECT_TRUE(c.contains(Point(0, 0)));
  EXPECT_TRUE(c.contains(Point(4, 0)));
}

TEST_F(GeomAlgorithmsTest, MEC_EquilateralTriangle)
{
  // Equilateral triangle with vertices at (0,0), (4,0), (2, 2*sqrt(3))
  // Circumradius = 2*sqrt(3)/sqrt(3)*2/sqrt(3) ... let's just check containment
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  const Geom_Number h = Geom_Number(2) * square_root(Geom_Number(3));
  pts.append(Point(2, h));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // All points must be contained
  EXPECT_TRUE(c.contains(Point(0, 0)));
  EXPECT_TRUE(c.contains(Point(4, 0)));
  EXPECT_TRUE(c.contains(Point(2, h)));

  // Radius should equal circumradius = 4/sqrt(3)
  const Geom_Number expected_r = Geom_Number(4) / square_root(Geom_Number(3));
  const Geom_Number tol = Geom_Number(1, 1000000);
  const Geom_Number actual_r = c.radius();
  const Geom_Number diff = actual_r > expected_r ?
      actual_r - expected_r : expected_r - actual_r;
  EXPECT_TRUE(diff < tol)
      << "radius " << actual_r << " != expected " << expected_r;
}

TEST_F(GeomAlgorithmsTest, MEC_RightTriangle)
{
  // Right triangle: hypotenuse is diameter of circumscribed circle
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(6, 0));
  pts.append(Point(0, 8));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // Center should be midpoint of hypotenuse = (3, 4)
  EXPECT_EQ(c.center, Point(3, 4));
  // Radius^2 = 9 + 16 = 25
  EXPECT_EQ(c.radius_squared, 25);

  EXPECT_TRUE(c.contains(Point(0, 0)));
  EXPECT_TRUE(c.contains(Point(6, 0)));
  EXPECT_TRUE(c.contains(Point(0, 8)));
}

TEST_F(GeomAlgorithmsTest, MEC_Square)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  pts.append(Point(4, 4));
  pts.append(Point(0, 4));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // Center at (2, 2), radius^2 = 8
  EXPECT_EQ(c.center, Point(2, 2));
  EXPECT_EQ(c.radius_squared, 8);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}

TEST_F(GeomAlgorithmsTest, MEC_RegularPentagon)
{
  // Regular pentagon centered at origin, circumradius = 5
  DynList<Point> pts;
  const double r = 5.0;
  for (int i = 0; i < 5; ++i)
    {
      const double angle = 2.0 * M_PI * i / 5.0;
      pts.append(Point(Geom_Number(r * std::cos(angle)),
                        Geom_Number(r * std::sin(angle))));
    }

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // All points should be contained
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));

  // Radius should be close to 5
  const Geom_Number tol = Geom_Number(1, 100000);
  const Geom_Number diff = c.radius() > Geom_Number(5) ?
      c.radius() - Geom_Number(5) : Geom_Number(5) - c.radius();
  EXPECT_TRUE(diff < tol)
      << "radius " << c.radius() << " expected ~5";
}

// ---------- Edge cases ----------

TEST_F(GeomAlgorithmsTest, MEC_EmptyInput)
{
  DynList<Point> pts;
  MinimumEnclosingCircle mec;
  EXPECT_THROW(mec(pts), std::domain_error);
}

TEST_F(GeomAlgorithmsTest, MEC_AllIdenticalPoints)
{
  DynList<Point> pts;
  for (int i = 0; i < 10; ++i)
    pts.append(Point(5, 5));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  EXPECT_EQ(c.center, Point(5, 5));
  EXPECT_EQ(c.radius_squared, 0);
}

TEST_F(GeomAlgorithmsTest, MEC_CollinearPoints)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(2, 0));
  pts.append(Point(4, 0));
  pts.append(Point(6, 0));
  pts.append(Point(10, 0));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // Diameter from (0,0) to (10,0): center = (5,0), r^2 = 25
  EXPECT_EQ(c.center, Point(5, 0));
  EXPECT_EQ(c.radius_squared, 25);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}

TEST_F(GeomAlgorithmsTest, MEC_DuplicatePoints)
{
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(4, 0));
  pts.append(Point(4, 0));
  pts.append(Point(0, 0));
  pts.append(Point(2, 3));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}

TEST_F(GeomAlgorithmsTest, MEC_PointAlreadyInside)
{
  // Triangle with an interior point
  DynList<Point> pts;
  pts.append(Point(0, 0));
  pts.append(Point(10, 0));
  pts.append(Point(5, 10));
  pts.append(Point(5, 3)); // interior

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));

  // Interior point should not affect the circle (boundary should have 3 pts)
  DynList<Point> pts_no_interior;
  pts_no_interior.append(Point(0, 0));
  pts_no_interior.append(Point(10, 0));
  pts_no_interior.append(Point(5, 10));

  auto c2 = mec(pts_no_interior);
  EXPECT_EQ(c.center, c2.center);
  EXPECT_EQ(c.radius_squared, c2.radius_squared);
}

// ---------- Property tests ----------

TEST_F(GeomAlgorithmsTest, MEC_AllPointsContained)
{
  DynList<Point> pts;
  pts.append(Point(1, 1));
  pts.append(Point(3, 7));
  pts.append(Point(8, 2));
  pts.append(Point(5, 9));
  pts.append(Point(0, 4));
  pts.append(Point(7, 6));
  pts.append(Point(2, 8));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  EXPECT_TRUE(c.radius_squared >= 0);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}

TEST_F(GeomAlgorithmsTest, MEC_InitializerList)
{
  MinimumEnclosingCircle mec;
  auto c = mec({Point(0, 0), Point(4, 0), Point(4, 4), Point(0, 4)});

  EXPECT_EQ(c.center, Point(2, 2));
  EXPECT_EQ(c.radius_squared, 8);
}

// ---------- Static helpers ----------

TEST_F(GeomAlgorithmsTest, MEC_FromTwoPoints)
{
  auto c = MinimumEnclosingCircle::from_two_points(Point(0, 0), Point(6, 8));
  EXPECT_EQ(c.center, Point(3, 4));
  // r^2 = 9 + 16 = 25
  EXPECT_EQ(c.radius_squared, 25);
}

TEST_F(GeomAlgorithmsTest, MEC_FromThreePointsCollinear)
{
  auto c = MinimumEnclosingCircle::from_three_points(
      Point(0, 0), Point(5, 0), Point(10, 0));
  EXPECT_EQ(c.center, Point(5, 0));
  EXPECT_EQ(c.radius_squared, 25);
}

// ---------- Stress tests ----------

TEST_F(GeomAlgorithmsTest, MEC_PointsOnKnownCircle)
{
  // Place 20 points on a circle of radius 10 centered at (5, 5)
  DynList<Point> pts;
  const double cx = 5.0, cy = 5.0, r = 10.0;
  for (int i = 0; i < 20; ++i)
    {
      const double angle = 2.0 * M_PI * i / 20.0;
      pts.append(Point(Geom_Number(cx + r * std::cos(angle)),
                        Geom_Number(cy + r * std::sin(angle))));
    }

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  // All points should be contained
  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));

  // Radius should be close to 10
  const Geom_Number tol = Geom_Number(1, 100000);
  const Geom_Number diff = c.radius() > Geom_Number(10) ?
      c.radius() - Geom_Number(10) : Geom_Number(10) - c.radius();
  EXPECT_TRUE(diff < tol)
      << "radius " << c.radius() << " expected ~10";
}

TEST_F(GeomAlgorithmsTest, MEC_GridPoints)
{
  // 10x10 grid
  DynList<Point> pts;
  for (int x = 0; x < 10; ++x)
    for (int y = 0; y < 10; ++y)
      pts.append(Point(x, y));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}

TEST_F(GeomAlgorithmsTest, MEC_RandomPoints)
{
  std::mt19937 gen(42);
  std::uniform_real_distribution<double> dist(-100.0, 100.0);

  DynList<Point> pts;
  for (int i = 0; i < 200; ++i)
    pts.append(Point(Geom_Number(dist(gen)), Geom_Number(dist(gen))));

  MinimumEnclosingCircle mec;
  auto c = mec(pts);

  EXPECT_TRUE(c.radius_squared >= 0);

  for (DynList<Point>::Iterator it(pts); it.has_curr(); it.next_ne())
    EXPECT_TRUE(c.contains(it.get_curr()));
}
