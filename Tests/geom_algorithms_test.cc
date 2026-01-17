#include <gtest/gtest.h>
#include <geom_algorithms.H>

class GeomAlgorithmsTest : public ::testing::Test
{
protected:
  void SetUp() override {}
  void TearDown() override {}
};

// Basic triangulation tests
TEST_F(GeomAlgorithmsTest, TriangulateTriangle)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(2, 3));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 1u);
}

TEST_F(GeomAlgorithmsTest, TriangulateSquare)
{
  Polygon p;
  p.add_vertex(Point(0, 0));
  p.add_vertex(Point(4, 0));
  p.add_vertex(Point(4, 4));
  p.add_vertex(Point(0, 4));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 2u);
}

TEST_F(GeomAlgorithmsTest, TriangulatePentagon)
{
  Polygon p;
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(4, 1.5));
  p.add_vertex(Point(3, 4));
  p.add_vertex(Point(1, 4));
  p.add_vertex(Point(0, 1.5));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 3u);
}

// Convex hull tests
TEST_F(GeomAlgorithmsTest, QuickHullTriangle)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(6, 0));
  points.append(Point(3, 5));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, QuickHullSquare)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(5, 0));
  points.append(Point(5, 5));
  points.append(Point(0, 5));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, QuickHullWithInterior)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(10, 0));
  points.append(Point(10, 10));
  points.append(Point(0, 10));
  points.append(Point(5, 5));  // Interior
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, GiftWrappingSquare)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(4, 0));
  points.append(Point(4, 4));
  points.append(Point(0, 4));
  
  GiftWrappingConvexHull gw;
  Polygon hull = gw(points);
  
  EXPECT_EQ(hull.size(), 4u);
}

TEST_F(GeomAlgorithmsTest, BruteForceHullTriangle)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(4, 0));
  points.append(Point(2, 3));
  
  BruteForceConvexHull bf;
  Polygon hull = bf(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, ThreePointsHull)
{
  DynList<Point> points;
  points.append(Point(0, 0));
  points.append(Point(2, 0));
  points.append(Point(1, 2));
  
  QuickHull qh;
  Polygon hull = qh(points);
  
  EXPECT_EQ(hull.size(), 3u);
}

TEST_F(GeomAlgorithmsTest, TriangulateHexagon)
{
  Polygon p;
  p.add_vertex(Point(1, 0));
  p.add_vertex(Point(2, 0));
  p.add_vertex(Point(3, 1));
  p.add_vertex(Point(2, 2));
  p.add_vertex(Point(1, 2));
  p.add_vertex(Point(0, 1));
  p.close();
  
  CuttingEarsTriangulation triangulator;
  auto triangles = triangulator(p);
  
  EXPECT_EQ(triangles.size(), 4u);
}
