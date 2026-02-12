
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
 * @file k2tree_test.cc
 * @brief Comprehensive test suite for K2Tree (2D k-d tree) spatial data structure
 * @author Test Suite Generator
 */

#include <gtest/gtest.h>
#include <tpl_2dtree.H>
#include <tpl_array.H>
#include <random>
#include <algorithm>
#include <vector>
#include <set>
#include <cmath>

using K2TreeInt = K2Tree<Empty_Class>;

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(K2TreeBasic, ConstructorAndEmpty)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
}

TEST(K2TreeBasic, InsertSinglePoint)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  bool inserted = tree.insert(Point(50, 50));
  
  EXPECT_TRUE(inserted);
  EXPECT_EQ(tree.size(), 1);
  EXPECT_FALSE(tree.is_empty());
}

TEST(K2TreeBasic, InsertMultiplePoints)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(25, 25));
  tree.insert(Point(75, 75));
  tree.insert(Point(50, 50));
  
  EXPECT_EQ(tree.size(), 3);
}

TEST(K2TreeBasic, InsertDuplicatePoint)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  bool first = tree.insert(Point(50, 50));
  bool second = tree.insert(Point(50, 50));
  
  EXPECT_TRUE(first);
  EXPECT_FALSE(second); // Duplicates not allowed
  EXPECT_EQ(tree.size(), 1);
}

TEST(K2TreeBasic, ContainsExisting)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(30, 40));
  tree.insert(Point(70, 60));
  
  EXPECT_TRUE(tree.contains(Point(30, 40)));
  EXPECT_TRUE(tree.contains(Point(70, 60)));
}

TEST(K2TreeBasic, ContainsNonExisting)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(30, 40));
  
  EXPECT_FALSE(tree.contains(Point(50, 50)));
  EXPECT_FALSE(tree.contains(Point(70, 60)));
}

TEST(K2TreeBasic, EmptyTreeContains)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  EXPECT_FALSE(tree.contains(Point(50, 50)));
}

// ============================================================================
// Insertion Patterns Tests
// ============================================================================

TEST(K2TreeInsertion, AscendingOrder)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  for (int i = 0; i < 10; ++i)
    {
      for (int j = 0; j < 10; ++j)
        {
          tree.insert(Point(i * 10, j * 10));
        }
    }
  
  EXPECT_EQ(tree.size(), 100);
  
  // Verify all points
  for (int i = 0; i < 10; ++i)
    {
      for (int j = 0; j < 10; ++j)
        {
          EXPECT_TRUE(tree.contains(Point(i * 10, j * 10)));
        }
    }
}

TEST(K2TreeInsertion, DescendingOrder)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  for (int i = 9; i >= 0; --i)
    {
      for (int j = 9; j >= 0; --j)
        {
          tree.insert(Point(i * 10, j * 10));
        }
    }
  
  EXPECT_EQ(tree.size(), 100);
}

TEST(K2TreeInsertion, RandomOrder)
{
  K2TreeInt tree(0, 0, 1000, 1000);
  
  std::mt19937 gen(12345);
  std::uniform_real_distribution<> dis(0, 1000);
  
  std::vector<Point> points;
  for (int i = 0; i < 100; ++i)
    {
      Point p(dis(gen), dis(gen));
      points.push_back(p);
      tree.insert(p);
    }
  
  EXPECT_EQ(tree.size(), points.size());
  
  for (const auto & p : points)
    {
      EXPECT_TRUE(tree.contains(p));
    }
}

TEST(K2TreeInsertion, ClusteredPoints)
{
  K2TreeInt tree(0, 0, 1000, 1000);
  
  // Insert cluster in one region
  for (int i = 0; i < 20; ++i)
    {
      tree.insert(Point(10 + i, 10 + i));
    }
  
  // Insert cluster in another region
  for (int i = 0; i < 20; ++i)
    {
      tree.insert(Point(900 + i, 900 + i));
    }
  
  EXPECT_EQ(tree.size(), 40);
}

// ============================================================================
// Nearest Neighbor Tests
// ============================================================================

TEST(K2TreeNearest, BasicQueries)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(50, 50));
  
  auto nearest = tree.nearest(Point(55, 55));
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ(nearest->get_x(), 50);
  EXPECT_EQ(nearest->get_y(), 50);
}

TEST(K2TreeNearest, MultiplePoints)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(50, 50));
  tree.insert(Point(10, 10));
  tree.insert(Point(90, 90));
  
  // Query near (50, 50)
  auto nearest = tree.nearest(Point(52, 48));
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ(nearest->get_x(), 50);
  EXPECT_EQ(nearest->get_y(), 50);
  
  // Query near (10, 10)
  nearest = tree.nearest(Point(12, 12));
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ(nearest->get_x(), 10);
  EXPECT_EQ(nearest->get_y(), 10);
  
  // Query near (90, 90)
  nearest = tree.nearest(Point(88, 92));
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ(nearest->get_x(), 90);
  EXPECT_EQ(nearest->get_y(), 90);
}

TEST(K2TreeNearest, EmptyTree)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  auto nearest = tree.nearest(Point(50, 50));
  EXPECT_FALSE(nearest.has_value());
}

TEST(K2TreeNearest, ExactMatch)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(50, 50));
  tree.insert(Point(75, 75));
  
  auto nearest = tree.nearest(Point(50, 50));
  ASSERT_TRUE(nearest.has_value());
  EXPECT_EQ(nearest->get_x(), 50);
  EXPECT_EQ(nearest->get_y(), 50);
}

TEST(K2TreeNearest, GridOfPoints)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  // Insert grid
  for (int i = 0; i <= 100; i += 10)
    {
      for (int j = 0; j <= 100; j += 10)
        {
          tree.insert(Point(i, j));
        }
    }
  
  // Query between grid points
  auto nearest = tree.nearest(Point(43, 57));
  ASSERT_TRUE(nearest.has_value());
  
  // Should find (40, 60) or (40, 50) or (50, 60) or (50, 50)
  Geom_Number dist = nearest->distance_with(Point(43, 57));
  EXPECT_LT(dist, 10); // Should be within one grid cell
}

// ============================================================================
// Range Query Tests
// ============================================================================

TEST(K2TreeRange, EmptyTree)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  Rectangle rect(20, 20, 80, 80);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_TRUE(result.is_empty());
}

TEST(K2TreeRange, AllPointsInRange)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(30, 30));
  tree.insert(Point(50, 50));
  tree.insert(Point(70, 70));
  
  Rectangle rect(0, 0, 100, 100);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_EQ(result.size(), 3);
}

TEST(K2TreeRange, NoPointsInRange)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(10, 10));
  tree.insert(Point(90, 90));
  
  Rectangle rect(40, 40, 60, 60);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_TRUE(result.is_empty());
}

TEST(K2TreeRange, SomePointsInRange)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(25, 25)); // Inside
  tree.insert(Point(50, 50)); // Inside
  tree.insert(Point(75, 75)); // Inside
  tree.insert(Point(10, 10)); // Outside
  tree.insert(Point(90, 90)); // Outside
  
  Rectangle rect(20, 20, 80, 80);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_EQ(result.size(), 3);
  
  // Verify the right points are in the result
  bool found_25 = false, found_50 = false, found_75 = false;
  for (const auto & p : result)
    {
      if (p == Point(25, 25)) found_25 = true;
      if (p == Point(50, 50)) found_50 = true;
      if (p == Point(75, 75)) found_75 = true;
    }
  
  EXPECT_TRUE(found_25);
  EXPECT_TRUE(found_50);
  EXPECT_TRUE(found_75);
}

TEST(K2TreeRange, BoundaryPoints)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  tree.insert(Point(20, 20)); // On boundary
  tree.insert(Point(80, 80)); // On boundary
  tree.insert(Point(50, 50)); // Inside
  
  Rectangle rect(20, 20, 80, 80);
  DynList<Point> result;
  tree.range(rect, &result);
  
  // Boundary behavior depends on Rectangle::contains implementation
  EXPECT_GE(result.size(), 1); // At least the inside point
  EXPECT_LE(result.size(), 3);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(K2TreeStress, LargeNumberOfPoints)
{
  K2TreeInt tree(0, 0, 10000, 10000);
  
  std::mt19937 gen(54321);
  std::uniform_real_distribution<> dis(0, 10000);
  
  const size_t num_points = 10000;
  std::vector<Point> points;
  
  for (size_t i = 0; i < num_points; ++i)
    {
      Point p(dis(gen), dis(gen));
      points.push_back(p);
      tree.insert(p);
      // May return nullptr for duplicates
    }
  
  // Size should be <= num_points due to possible duplicates
  EXPECT_LE(tree.size(), num_points);
  EXPECT_GT(tree.size(), num_points * 0.9); // Expect most to be unique
}

TEST(K2TreeStress, ManyNearestQueries)
{
  K2TreeInt tree(0, 0, 1000, 1000);
  
  std::mt19937 gen(99999);
  std::uniform_real_distribution<> dis(0, 1000);
  
  // Insert points
  for (int i = 0; i < 1000; ++i)
    {
      tree.insert(Point(dis(gen), dis(gen)));
    }
  
  // Perform many nearest queries
  for (int i = 0; i < 1000; ++i)
    {
      Point query(dis(gen), dis(gen));
      auto nearest = tree.nearest(query);
      EXPECT_TRUE(nearest.has_value());
    }
}

TEST(K2TreeStress, ManyRangeQueries)
{
  K2TreeInt tree(0, 0, 1000, 1000);
  
  std::mt19937 gen(11111);
  std::uniform_real_distribution<> dis(0, 1000);
  
  // Insert points
  for (int i = 0; i < 1000; ++i)
    {
      tree.insert(Point(dis(gen), dis(gen)));
    }
  
  // Perform many range queries
  for (int i = 0; i < 100; ++i)
    {
      double x1 = dis(gen), y1 = dis(gen);
      double x2 = dis(gen), y2 = dis(gen);
      
      if (x1 > x2) std::swap(x1, x2);
      if (y1 > y2) std::swap(y1, y2);
      
      Rectangle rect(x1, y1, x2, y2);
      DynList<Point> result;
      tree.range(rect, &result);
      
      // Just verify it doesn't crash
      EXPECT_GE(result.size(), 0);
    }
}

TEST(K2TreeStress, DenseAndSparseRegions)
{
  K2TreeInt tree(0, 0, 1000, 1000);
  
  // Dense region: [100, 200] × [100, 200]
  for (int i = 100; i <= 200; i += 2)
    {
      for (int j = 100; j <= 200; j += 2)
        {
          tree.insert(Point(i, j));
        }
    }
  
  // Sparse region: [700, 900] × [700, 900]
  for (int i = 700; i <= 900; i += 50)
    {
      for (int j = 700; j <= 900; j += 50)
        {
          tree.insert(Point(i, j));
        }
    }
  
  // Query dense region
  Rectangle dense_rect(100, 100, 200, 200);
  DynList<Point> dense_result;
  tree.range(dense_rect, &dense_result);
  EXPECT_GT(dense_result.size(), 0);
  
  // Query sparse region
  Rectangle sparse_rect(700, 700, 900, 900);
  DynList<Point> sparse_result;
  tree.range(sparse_rect, &sparse_result);
  EXPECT_GT(sparse_result.size(), 0);
  EXPECT_LT(sparse_result.size(), dense_result.size());
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(K2TreeEdgeCases, ColinearPoints)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  // Insert points along a line
  for (int i = 0; i <= 100; i += 10)
    {
      tree.insert(Point(i, i)); // Diagonal line
    }
  
  EXPECT_EQ(tree.size(), 11);
  
  // Verify all can be found
  for (int i = 0; i <= 100; i += 10)
    {
      EXPECT_TRUE(tree.contains(Point(i, i)));
    }
}

TEST(K2TreeEdgeCases, VerticalLine)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  for (int i = 0; i <= 100; i += 10)
    {
      tree.insert(Point(50, i));
    }
  
  EXPECT_EQ(tree.size(), 11);
}

TEST(K2TreeEdgeCases, HorizontalLine)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  for (int i = 0; i <= 100; i += 10)
    {
      tree.insert(Point(i, 50));
    }
  
  EXPECT_EQ(tree.size(), 11);
}

TEST(K2TreeEdgeCases, VeryClosePoints)
{
  K2TreeInt tree(0, 0, 1, 1);
  
  tree.insert(Point(0.1, 0.1));
  tree.insert(Point(0.100001, 0.100001));
  tree.insert(Point(0.9, 0.9));
  
  EXPECT_EQ(tree.size(), 3);
}

TEST(K2TreeEdgeCases, NegativeCoordinates)
{
  K2TreeInt tree(-100, -100, 100, 100);
  
  tree.insert(Point(-50, -50));
  tree.insert(Point(0, 0));
  tree.insert(Point(50, 50));
  
  EXPECT_EQ(tree.size(), 3);
  EXPECT_TRUE(tree.contains(Point(-50, -50)));
  EXPECT_TRUE(tree.contains(Point(0, 0)));
  EXPECT_TRUE(tree.contains(Point(50, 50)));
}

// ============================================================================
// Correctness Verification Tests
// ============================================================================

TEST(K2TreeCorrectness, NearestIsActuallyNearest)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  std::vector<Point> points = {
    Point(10, 10),
    Point(50, 50),
    Point(90, 90),
    Point(30, 70),
    Point(70, 30)
  };
  
  for (const auto & p : points)
    tree.insert(p);
  
  Point query(45, 45);
  auto nearest = tree.nearest(query);
  ASSERT_TRUE(nearest.has_value());
  
  // Verify it's actually the nearest
  Geom_Number min_dist = nearest->distance_with(query);
  for (const auto & p : points)
    {
      Geom_Number dist = p.distance_with(query);
      EXPECT_LE(min_dist, dist); // Exact arithmetic — no tolerance needed
    }
}

TEST(K2TreeCorrectness, RangeIncludesAllInside)
{
  K2TreeInt tree(0, 0, 100, 100);
  
  std::mt19937 gen(77777);
  std::uniform_real_distribution<> dis(0, 100);
  
  std::vector<Point> all_points;
  for (int i = 0; i < 100; ++i)
    {
      Point p(dis(gen), dis(gen));
      all_points.push_back(p);
      tree.insert(p);
    }
  
  Rectangle rect(25, 25, 75, 75);
  DynList<Point> result;
  tree.range(rect, &result);
  
  // Verify all points in result are actually in rectangle
  for (const auto & p : result)
    {
      EXPECT_TRUE(rect.contains(p));
    }
  
  // Verify all points in rectangle are in result
  for (const auto & p : all_points)
    {
      if (rect.contains(p))
        {
          bool found = false;
          for (const auto & r : result)
            {
              if (r == p)
                {
                  found = true;
                  break;
                }
            }
          EXPECT_TRUE(found) << "Point in rectangle not found in result";
        }
    }
}

// ============================================================================
// Fuzz Testing
// ============================================================================

TEST(K2TreeFuzz, RandomInsertionsAndQueries)
{
  K2TreeInt tree(0, 0, 10000, 10000);
  
  std::mt19937 gen(31415);
  std::uniform_real_distribution<> coord_dis(0, 10000);
  std::uniform_int_distribution<> op_dis(0, 2);
  
  std::set<std::pair<double, double>> inserted;
  
  for (int i = 0; i < 1000; ++i)
    {
      int op = op_dis(gen);
      
      if (op == 0) // Insert
        {
          Point p(coord_dis(gen), coord_dis(gen));
          bool ok = tree.insert(p);
          if (ok)
            {
              inserted.insert(std::make_pair(static_cast<double>(p.get_x().get_d()), 
                                             static_cast<double>(p.get_y().get_d())));
            }
        }
      else if (op == 1) // Nearest
        {
          Point query(coord_dis(gen), coord_dis(gen));
          auto nearest = tree.nearest(query);
          
          if (not tree.is_empty())
            EXPECT_TRUE(nearest.has_value());
        }
      else // Range
        {
          double x1 = coord_dis(gen), y1 = coord_dis(gen);
          double x2 = coord_dis(gen), y2 = coord_dis(gen);
          if (x1 > x2) std::swap(x1, x2);
          if (y1 > y2) std::swap(y1, y2);
          
          Rectangle rect(x1, y1, x2, y2);
          DynList<Point> result;
          tree.range(rect, &result);
        }
    }
  
  // Verify all inserted points can be found
  for (const auto & [x, y] : inserted)
    {
      EXPECT_TRUE(tree.contains(Point(x, y)));
    }
}

// ============================================================================
// Insert Correctness Tests
// ============================================================================

TEST(K2TreeInsertCorrectness, ReturnsTrueOnSuccess)
{
  K2TreeInt tree(0, 0, 100, 100);

  EXPECT_TRUE(tree.insert(Point(10, 20)));
  EXPECT_TRUE(tree.insert(Point(30, 40)));
  EXPECT_TRUE(tree.insert(Point(50, 60)));
  EXPECT_EQ(tree.size(), 3);
}

TEST(K2TreeInsertCorrectness, ReturnsFalseOnDuplicate)
{
  K2TreeInt tree(0, 0, 100, 100);

  EXPECT_TRUE(tree.insert(Point(42, 17)));
  EXPECT_FALSE(tree.insert(Point(42, 17)));
  EXPECT_EQ(tree.size(), 1);
}

TEST(K2TreeInsertCorrectness, AllInsertedPointsAreContained)
{
  K2TreeInt tree(0, 0, 1000, 1000);

  std::mt19937 gen(55555);
  std::uniform_real_distribution<> dis(0, 1000);

  std::vector<Point> points;
  for (int i = 0; i < 200; ++i)
    {
      Point p(dis(gen), dis(gen));
      if (tree.insert(p))
        points.push_back(p);
    }

  for (const auto & p : points)
    EXPECT_TRUE(tree.contains(p));

  EXPECT_EQ(tree.size(), points.size());
}

// ============================================================================
// Move Semantics Tests
// ============================================================================

TEST(K2TreeMove, MoveConstructor)
{
  K2TreeInt src(0, 0, 100, 100);
  src.insert(Point(10, 20));
  src.insert(Point(30, 40));
  src.insert(Point(50, 60));
  ASSERT_EQ(src.size(), 3);

  K2TreeInt dst(std::move(src));

  // dst owns the nodes now
  EXPECT_EQ(dst.size(), 3);
  EXPECT_TRUE(dst.contains(Point(10, 20)));
  EXPECT_TRUE(dst.contains(Point(30, 40)));
  EXPECT_TRUE(dst.contains(Point(50, 60)));

  // src is empty after move
  EXPECT_EQ(src.size(), 0);
  EXPECT_TRUE(src.is_empty());
}

TEST(K2TreeMove, MoveAssignment)
{
  K2TreeInt src(0, 0, 100, 100);
  src.insert(Point(10, 20));
  src.insert(Point(30, 40));

  K2TreeInt dst(0, 0, 200, 200);
  dst.insert(Point(99, 99));
  ASSERT_EQ(dst.size(), 1);

  dst = std::move(src);

  EXPECT_EQ(dst.size(), 2);
  EXPECT_TRUE(dst.contains(Point(10, 20)));
  EXPECT_TRUE(dst.contains(Point(30, 40)));
  EXPECT_FALSE(dst.contains(Point(99, 99))); // old node freed

  EXPECT_EQ(src.size(), 0);
  EXPECT_TRUE(src.is_empty());
}

TEST(K2TreeMove, MoveToSelf)
{
  K2TreeInt tree(0, 0, 100, 100);
  tree.insert(Point(5, 5));

  tree = std::move(tree);

  // Self-move should be a no-op
  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.contains(Point(5, 5)));
}

// ============================================================================
// Balanced Build Tests
// ============================================================================

TEST(K2TreeBuild, EmptyArray)
{
  Array<Point> pts;
  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(100, 100));

  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
}

TEST(K2TreeBuild, SinglePoint)
{
  Array<Point> pts;
  pts.append(Point(42, 17));

  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(100, 100));

  EXPECT_EQ(tree.size(), 1);
  EXPECT_TRUE(tree.contains(Point(42, 17)));
}

TEST(K2TreeBuild, MultiplePoints)
{
  Array<Point> pts;
  pts.append(Point(10, 10));
  pts.append(Point(20, 20));
  pts.append(Point(30, 30));
  pts.append(Point(40, 40));
  pts.append(Point(50, 50));

  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(100, 100));

  EXPECT_EQ(tree.size(), 5);
  for (size_t i = 0; i < pts.size(); ++i)
    EXPECT_TRUE(tree.contains(pts(i)));
}

TEST(K2TreeBuild, DuplicatesRemoved)
{
  Array<Point> pts;
  pts.append(Point(10, 10));
  pts.append(Point(10, 10));
  pts.append(Point(20, 20));
  pts.append(Point(20, 20));
  pts.append(Point(30, 30));

  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(100, 100));

  EXPECT_EQ(tree.size(), 3);
  EXPECT_TRUE(tree.contains(Point(10, 10)));
  EXPECT_TRUE(tree.contains(Point(20, 20)));
  EXPECT_TRUE(tree.contains(Point(30, 30)));
}

TEST(K2TreeBuild, NearestWorksOnBalancedTree)
{
  Array<Point> pts;
  for (int i = 0; i <= 100; i += 10)
    for (int j = 0; j <= 100; j += 10)
      pts.append(Point(i, j));

  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(100, 100));

  EXPECT_EQ(tree.size(), 121);

  auto nearest = tree.nearest(Point(43, 57));
  ASSERT_TRUE(nearest.has_value());

  // Should find a grid point within one cell (distance < 10)
  Geom_Number dist = nearest->distance_with(Point(43, 57));
  EXPECT_LT(dist, 10);
}

TEST(K2TreeBuild, StressBuildVsInsert)
{
  std::mt19937 gen(12321);
  std::uniform_real_distribution<> dis(0, 10000);

  Array<Point> pts;
  for (int i = 0; i < 5000; ++i)
    pts.append(Point(dis(gen), dis(gen)));

  auto balanced = K2TreeInt::build(pts, Point(0, 0), Point(10000, 10000));

  // All unique points should be contained
  for (size_t i = 0; i < pts.size(); ++i)
    EXPECT_TRUE(balanced.contains(pts(i)));

  // Nearest queries should give correct results
  for (int i = 0; i < 100; ++i)
    {
      Point query(dis(gen), dis(gen));
      auto near_bal = balanced.nearest(query);
      ASSERT_TRUE(near_bal.has_value());

      // Brute-force verification
      Geom_Number best_d2 = near_bal->distance_squared_to(query);
      for (size_t j = 0; j < pts.size(); ++j)
        {
          Geom_Number d2 = pts(j).distance_squared_to(query);
          EXPECT_LE(best_d2, d2);
        }
    }
}

// ============================================================================
// for_each Tests
// ============================================================================

TEST(K2TreeForEach, EmptyTree)
{
  K2TreeInt tree(0, 0, 100, 100);

  size_t count = 0;
  tree.for_each([&count](const Point &) { ++count; });

  EXPECT_EQ(count, 0);
}

TEST(K2TreeForEach, VisitsAllPoints)
{
  K2TreeInt tree(0, 0, 100, 100);

  std::set<std::pair<double, double>> expected;
  for (int i = 0; i <= 50; i += 10)
    for (int j = 0; j <= 50; j += 10)
      {
        tree.insert(Point(i, j));
        expected.insert({static_cast<double>(i), static_cast<double>(j)});
      }

  std::set<std::pair<double, double>> visited;
  tree.for_each([&visited](const Point & p)
    {
      visited.insert({p.get_x().get_d(), p.get_y().get_d()});
    });

  EXPECT_EQ(visited.size(), expected.size());
  EXPECT_EQ(visited, expected);
}

TEST(K2TreeForEach, BalancedTreeVisitsAll)
{
  Array<Point> pts;
  for (int i = 1; i <= 100; ++i)
    pts.append(Point(i, i * 2));

  auto tree = K2TreeInt::build(pts, Point(0, 0), Point(200, 200));

  size_t count = 0;
  tree.for_each([&count](const Point &) { ++count; });

  EXPECT_EQ(count, tree.size());
  EXPECT_EQ(count, 100);
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

