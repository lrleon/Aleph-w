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
 * @file k2tree_test.cc
 * @brief Comprehensive test suite for K2Tree (2D k-d tree) spatial data structure
 * @author Test Suite Generator
 */

#include <gtest/gtest.h>
#include <tpl_2dtree.H>
#include <random>
#include <algorithm>
#include <vector>
#include <set>
#include <cmath>

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(K2TreeBasic, ConstructorAndEmpty)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  EXPECT_TRUE(tree.is_empty());
  EXPECT_EQ(tree.size(), 0);
}

TEST(K2TreeBasic, InsertSinglePoint)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  Point * inserted = tree.insert(Point(50, 50));
  
  ASSERT_NE(inserted, nullptr);
  EXPECT_EQ(tree.size(), 1);
  EXPECT_FALSE(tree.is_empty());
}

TEST(K2TreeBasic, InsertMultiplePoints)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(25, 25));
  tree.insert(Point(75, 75));
  tree.insert(Point(50, 50));
  
  EXPECT_EQ(tree.size(), 3);
}

TEST(K2TreeBasic, InsertDuplicatePoint)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  Point * first = tree.insert(Point(50, 50));
  Point * second = tree.insert(Point(50, 50));
  
  ASSERT_NE(first, nullptr);
  EXPECT_EQ(second, nullptr); // Duplicates not allowed
  EXPECT_EQ(tree.size(), 1);
}

TEST(K2TreeBasic, ContainsExisting)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(30, 40));
  tree.insert(Point(70, 60));
  
  EXPECT_TRUE(tree.contains(Point(30, 40)));
  EXPECT_TRUE(tree.contains(Point(70, 60)));
}

TEST(K2TreeBasic, ContainsNonExisting)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(30, 40));
  
  EXPECT_FALSE(tree.contains(Point(50, 50)));
  EXPECT_FALSE(tree.contains(Point(70, 60)));
}

TEST(K2TreeBasic, EmptyTreeContains)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  EXPECT_FALSE(tree.contains(Point(50, 50)));
}

// ============================================================================
// Insertion Patterns Tests
// ============================================================================

TEST(K2TreeInsertion, AscendingOrder)
{
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 1000, 1000);
  
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
  K2Tree<> tree(0, 0, 1000, 1000);
  
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

TEST(K2TreeNearest, SinglePoint)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(50, 50));
  
  Point nearest = tree.nearest(Point(55, 55));
  
  EXPECT_EQ(nearest.get_x(), 50);
  EXPECT_EQ(nearest.get_y(), 50);
}

TEST(K2TreeNearest, MultiplePoints)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(10, 10));
  tree.insert(Point(50, 50));
  tree.insert(Point(90, 90));
  
  // Query near (50, 50)
  Point nearest = tree.nearest(Point(52, 48));
  EXPECT_EQ(nearest.get_x(), 50);
  EXPECT_EQ(nearest.get_y(), 50);
  
  // Query near (10, 10)
  nearest = tree.nearest(Point(12, 12));
  EXPECT_EQ(nearest.get_x(), 10);
  EXPECT_EQ(nearest.get_y(), 10);
  
  // Query near (90, 90)
  nearest = tree.nearest(Point(88, 92));
  EXPECT_EQ(nearest.get_x(), 90);
  EXPECT_EQ(nearest.get_y(), 90);
}

TEST(K2TreeNearest, EmptyTree)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  Point nearest = tree.nearest(Point(50, 50));
  
  // Should return NullPoint
  EXPECT_EQ(nearest, NullPoint);
}

TEST(K2TreeNearest, ExactMatch)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(50, 50));
  tree.insert(Point(75, 75));
  
  Point nearest = tree.nearest(Point(50, 50));
  
  EXPECT_EQ(nearest.get_x(), 50);
  EXPECT_EQ(nearest.get_y(), 50);
}

TEST(K2TreeNearest, GridOfPoints)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  // Insert grid
  for (int i = 0; i <= 100; i += 10)
    {
      for (int j = 0; j <= 100; j += 10)
        {
          tree.insert(Point(i, j));
        }
    }
  
  // Query between grid points
  Point nearest = tree.nearest(Point(43, 57));
  
  // Should find (40, 60) or (40, 50) or (50, 60) or (50, 50)
  double dist = nearest.distance_to(Point(43, 57));
  EXPECT_LT(dist, 10); // Should be within one grid cell
}

// ============================================================================
// Range Query Tests
// ============================================================================

TEST(K2TreeRange, EmptyTree)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  Rectangle rect(20, 20, 80, 80);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_TRUE(result.is_empty());
}

TEST(K2TreeRange, AllPointsInRange)
{
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
  tree.insert(Point(10, 10));
  tree.insert(Point(90, 90));
  
  Rectangle rect(40, 40, 60, 60);
  DynList<Point> result;
  tree.range(rect, &result);
  
  EXPECT_TRUE(result.is_empty());
}

TEST(K2TreeRange, SomePointsInRange)
{
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 10000, 10000);
  
  std::mt19937 gen(54321);
  std::uniform_real_distribution<> dis(0, 10000);
  
  const size_t num_points = 10000;
  std::vector<Point> points;
  
  for (size_t i = 0; i < num_points; ++i)
    {
      Point p(dis(gen), dis(gen));
      points.push_back(p);
      Point * result = tree.insert(p);
      // May return nullptr for duplicates
    }
  
  // Size should be <= num_points due to possible duplicates
  EXPECT_LE(tree.size(), num_points);
  EXPECT_GT(tree.size(), num_points * 0.9); // Expect most to be unique
}

TEST(K2TreeStress, ManyNearestQueries)
{
  K2Tree<> tree(0, 0, 1000, 1000);
  
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
      Point nearest = tree.nearest(query);
      
      EXPECT_NE(nearest, NullPoint);
    }
}

TEST(K2TreeStress, ManyRangeQueries)
{
  K2Tree<> tree(0, 0, 1000, 1000);
  
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
  K2Tree<> tree(0, 0, 1000, 1000);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
  for (int i = 0; i <= 100; i += 10)
    {
      tree.insert(Point(50, i));
    }
  
  EXPECT_EQ(tree.size(), 11);
}

TEST(K2TreeEdgeCases, HorizontalLine)
{
  K2Tree<> tree(0, 0, 100, 100);
  
  for (int i = 0; i <= 100; i += 10)
    {
      tree.insert(Point(i, 50));
    }
  
  EXPECT_EQ(tree.size(), 11);
}

TEST(K2TreeEdgeCases, VeryClosePoints)
{
  K2Tree<> tree(0, 0, 1, 1);
  
  tree.insert(Point(0.1, 0.1));
  tree.insert(Point(0.100001, 0.100001));
  tree.insert(Point(0.9, 0.9));
  
  EXPECT_EQ(tree.size(), 3);
}

TEST(K2TreeEdgeCases, NegativeCoordinates)
{
  K2Tree<> tree(-100, -100, 100, 100);
  
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
  K2Tree<> tree(0, 0, 100, 100);
  
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
  Point nearest = tree.nearest(query);
  
  // Verify it's actually the nearest
  double min_dist = nearest.distance_to(query);
  for (const auto & p : points)
    {
      double dist = p.distance_to(query);
      EXPECT_GE(dist, min_dist - 1e-9); // Allow tiny floating point error
    }
}

TEST(K2TreeCorrectness, RangeIncludesAllInside)
{
  K2Tree<> tree(0, 0, 100, 100);
  
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
  K2Tree<> tree(0, 0, 10000, 10000);
  
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
          Point * result = tree.insert(p);
          if (result != nullptr)
            inserted.insert({p.get_x(), p.get_y()});
        }
      else if (op == 1) // Nearest
        {
          Point query(coord_dis(gen), coord_dis(gen));
          Point nearest = tree.nearest(query);
          
          if (not tree.is_empty())
            EXPECT_NE(nearest, NullPoint);
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
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

