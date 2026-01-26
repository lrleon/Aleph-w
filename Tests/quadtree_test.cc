
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
 * @file quadtree_test.cc
 * @brief Comprehensive test suite for QuadTree spatial data structure
 * @author Test Suite Generator
 */

#include <gtest/gtest.h>
#include <quadtree.H>
#include <random>
#include <algorithm>
#include <unordered_set>
#include <sstream>

// ============================================================================
// Basic Functionality Tests
// ============================================================================

TEST(QuadTreeBasic, ConstructorAndEmpty)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  EXPECT_NE(tree.get_root(), nullptr);
  EXPECT_EQ(tree.get_max_num_points_per_node(), 4);
}

TEST(QuadTreeBasic, InsertSinglePoint)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  Point * inserted = tree.insert(Point(50, 50));
  
  ASSERT_NE(inserted, nullptr);
  EXPECT_EQ(inserted->get_x(), 50);
  EXPECT_EQ(inserted->get_y(), 50);
}

TEST(QuadTreeBasic, InsertOutsideBounds)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  Point * result1 = tree.insert(Point(-10, 50));
  Point * result2 = tree.insert(Point(50, 150));
  Point * result3 = tree.insert(Point(150, 50));
  Point * result4 = tree.insert(Point(50, -10));
  
  EXPECT_EQ(result1, nullptr);
  EXPECT_EQ(result2, nullptr);
  EXPECT_EQ(result3, nullptr);
  EXPECT_EQ(result4, nullptr);
}

TEST(QuadTreeBasic, ContainsCheck)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(25, 25));
  tree.insert(Point(75, 75));
  
  EXPECT_TRUE(tree.contains(Point(25, 25)));
  EXPECT_TRUE(tree.contains(Point(75, 75)));
  EXPECT_TRUE(tree.contains(Point(50, 50))); // In bounds but not inserted
  EXPECT_FALSE(tree.contains(Point(-10, 50))); // Out of bounds
  EXPECT_FALSE(tree.contains(Point(150, 50))); // Out of bounds
}

TEST(QuadTreeBasic, SearchExistingPoint)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(30, 40));
  tree.insert(Point(70, 60));
  
  Point * found1 = tree.search(Point(30, 40));
  Point * found2 = tree.search(Point(70, 60));
  
  ASSERT_NE(found1, nullptr);
  ASSERT_NE(found2, nullptr);
  EXPECT_EQ(found1->get_x(), 30);
  EXPECT_EQ(found1->get_y(), 40);
  EXPECT_EQ(found2->get_x(), 70);
  EXPECT_EQ(found2->get_y(), 60);
}

TEST(QuadTreeBasic, SearchNonExistingPoint)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(30, 40));
  
  Point * found = tree.search(Point(50, 50));
  
  EXPECT_EQ(found, nullptr);
}

TEST(QuadTreeBasic, SearchContainerNode)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(25, 25));
  
  QuadNode * node = tree.search_container_node(Point(25, 25));
  
  ASSERT_NE(node, nullptr);
  EXPECT_TRUE(node->is_leaf());
  EXPECT_NE(node->search_point(Point(25, 25)), nullptr);
}

TEST(QuadTreeBasic, RemoveSinglePoint)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(50, 50));
  EXPECT_NE(tree.search(Point(50, 50)), nullptr);
  
  tree.remove(Point(50, 50));
  EXPECT_EQ(tree.search(Point(50, 50)), nullptr);
}

TEST(QuadTreeBasic, RemoveNonExistingPoint)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(50, 50));
  
  // Should not crash or cause errors
  tree.remove(Point(30, 30));
  
  EXPECT_NE(tree.search(Point(50, 50)), nullptr);
}

TEST(QuadTreeBasic, EmptyTree)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  tree.insert(Point(25, 25));
  tree.insert(Point(75, 75));
  tree.insert(Point(50, 50));
  
  tree.empty();
  
  EXPECT_EQ(tree.search(Point(25, 25)), nullptr);
  EXPECT_EQ(tree.search(Point(75, 75)), nullptr);
  EXPECT_EQ(tree.search(Point(50, 50)), nullptr);
}

// ============================================================================
// Subdivision and Merging Tests
// ============================================================================

TEST(QuadTreeSubdivision, SingleLevelSplit)
{
  QuadTree tree(0, 100, 0, 100, 2); // Max 2 points per node
  
  tree.insert(Point(25, 25));
  tree.insert(Point(30, 30));
  
  // Root should still be a leaf
  EXPECT_TRUE(tree.get_root()->is_leaf());
  
  // Third point should trigger split
  tree.insert(Point(35, 35));
  
  // Root should now be internal (Gray)
  EXPECT_FALSE(tree.get_root()->is_leaf());
  EXPECT_EQ(COLOR(tree.get_root()), QuadNode::Color::Gray);
}

TEST(QuadTreeSubdivision, MultiLevelSplit)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  // Insert many points in one quadrant to force deep splits
  for (int i = 1; i <= 10; ++i)
    {
      tree.insert(Point(10 + i, 10 + i));
    }
  
  // Verify root is internal
  EXPECT_FALSE(tree.get_root()->is_leaf());
  
  // Verify tree has multiple levels
  QuadNode * nw = NW_CHILD(tree.get_root());
  ASSERT_NE(nw, nullptr);
  
  // At least one child should be further subdivided
  bool has_deep_child = false;
  if (not nw->is_leaf() || (NE_CHILD(tree.get_root()) != nullptr and not NE_CHILD(tree.get_root())->is_leaf()))
    has_deep_child = true;
  
  EXPECT_TRUE(has_deep_child);
}

TEST(QuadTreeSubdivision, AllQuadrantsPopulated)
{
  QuadTree tree(0, 100, 0, 100, 1);
  
  // Insert one point in each quadrant
  tree.insert(Point(25, 25)); // SW
  tree.insert(Point(75, 25)); // SE
  tree.insert(Point(25, 75)); // NW
  tree.insert(Point(75, 75)); // NE
  
  QuadNode * root = tree.get_root();
  EXPECT_FALSE(root->is_leaf());
  
  // All four children should exist and be leaves
  ASSERT_NE(NW_CHILD(root), nullptr);
  ASSERT_NE(NE_CHILD(root), nullptr);
  ASSERT_NE(SW_CHILD(root), nullptr);
  ASSERT_NE(SE_CHILD(root), nullptr);
  
  EXPECT_TRUE(NW_CHILD(root)->is_leaf());
  EXPECT_TRUE(NE_CHILD(root)->is_leaf());
  EXPECT_TRUE(SW_CHILD(root)->is_leaf());
  EXPECT_TRUE(SE_CHILD(root)->is_leaf());
}

TEST(QuadTreeMerging, RemovalTriggersJoin)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  // Insert 3 points in DIFFERENT quadrants to trigger split
  // Points must be in different quadrants to avoid nested splits
  tree.insert(Point(25, 25));  // SW quadrant
  tree.insert(Point(75, 25));  // SE quadrant
  tree.insert(Point(25, 75));  // NW quadrant
  
  EXPECT_FALSE(tree.get_root()->is_leaf());
  
  // Remove one point to go below threshold
  tree.remove(Point(25, 75));
  
  // Root should merge back to leaf (only 2 points left, threshold is 2)
  EXPECT_TRUE(tree.get_root()->is_leaf());
}

TEST(QuadTreeMerging, MultipleRemovalsGradualJoin)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  // Insert many points
  std::vector<Point> points = {
    Point(10, 10), Point(15, 15), Point(20, 20),
    Point(80, 80), Point(85, 85), Point(90, 90)
  };
  
  for (const auto & p : points)
    tree.insert(p);
  
  // Tree should be subdivided
  EXPECT_FALSE(tree.get_root()->is_leaf());
  
  // Remove points one by one
  for (const auto & p : points)
    {
      tree.remove(p);
    }
  
  // After all removals, root should be a leaf again
  EXPECT_TRUE(tree.get_root()->is_leaf());
  EXPECT_EQ(COLOR(tree.get_root()), QuadNode::Color::White);
}

// ============================================================================
// Copy Constructor and Assignment Tests
// ============================================================================

TEST(QuadTreeCopy, CopyConstructor)
{
  QuadTree tree1(0, 100, 0, 100, 3);
  
  tree1.insert(Point(25, 25));
  tree1.insert(Point(75, 75));
  tree1.insert(Point(50, 50));
  
  QuadTree tree2(tree1);
  
  // Verify all points are in the copy
  EXPECT_NE(tree2.search(Point(25, 25)), nullptr);
  EXPECT_NE(tree2.search(Point(75, 75)), nullptr);
  EXPECT_NE(tree2.search(Point(50, 50)), nullptr);
  
  // Verify they are independent
  tree1.insert(Point(10, 10));
  EXPECT_EQ(tree2.search(Point(10, 10)), nullptr);
}

TEST(QuadTreeCopy, AssignmentOperator)
{
  QuadTree tree1(0, 100, 0, 100, 3);
  tree1.insert(Point(25, 25));
  tree1.insert(Point(75, 75));
  
  QuadTree tree2(0, 200, 0, 200, 5);
  tree2.insert(Point(150, 150));
  
  tree2 = tree1;
  
  // tree2 should now have tree1's data
  EXPECT_NE(tree2.search(Point(25, 25)), nullptr);
  EXPECT_NE(tree2.search(Point(75, 75)), nullptr);
  EXPECT_EQ(tree2.search(Point(150, 150)), nullptr);
  
  // Configuration should also be copied
  EXPECT_EQ(tree2.get_max_num_points_per_node(), 3);
}

TEST(QuadTreeCopy, SelfAssignment)
{
  QuadTree tree(0, 100, 0, 100, 3);
  tree.insert(Point(50, 50));
  
  tree = tree;
  
  // Should still work
  EXPECT_NE(tree.search(Point(50, 50)), nullptr);
}

// ============================================================================
// Stress Tests
// ============================================================================

TEST(QuadTreeStress, InsertManyPoints)
{
  QuadTree tree(0, 1000, 0, 1000, 4);
  
  std::mt19937 gen(12345);
  std::uniform_real_distribution<> dis(0, 1000);
  
  const size_t num_points = 10000;
  std::vector<Point> points;
  
  for (size_t i = 0; i < num_points; ++i)
    {
      Point p(dis(gen), dis(gen));
      points.push_back(p);
      Point * inserted = tree.insert(p);
      ASSERT_NE(inserted, nullptr);
    }
  
  // Verify all points can be found
  for (const auto & p : points)
    {
      EXPECT_NE(tree.search(p), nullptr);
    }
}

TEST(QuadTreeStress, InsertRemoveCycles)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  std::mt19937 gen(54321);
  std::uniform_real_distribution<> dis(0, 100);
  
  const size_t cycles = 100;
  const size_t points_per_cycle = 50;
  
  for (size_t cycle = 0; cycle < cycles; ++cycle)
    {
      std::vector<Point> points;
      
      // Insert points
      for (size_t i = 0; i < points_per_cycle; ++i)
        {
          Point p(dis(gen), dis(gen));
          points.push_back(p);
          tree.insert(p);
        }
      
      // Remove half of them
      for (size_t i = 0; i < points_per_cycle / 2; ++i)
        {
          tree.remove(points[i]);
        }
    }
  
  // Tree should still be functional
  Point * test = tree.insert(Point(50, 50));
  EXPECT_NE(test, nullptr);
}

TEST(QuadTreeStress, DenseRegion)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  // Insert many points in a small region
  for (int x = 40; x <= 60; ++x)
    {
      for (int y = 40; y <= 60; ++y)
        {
          tree.insert(Point(x, y));
        }
    }
  
  // Verify all can be found
  for (int x = 40; x <= 60; ++x)
    {
      for (int y = 40; y <= 60; ++y)
        {
          EXPECT_NE(tree.search(Point(x, y)), nullptr);
        }
    }
}

// ============================================================================
// Edge Cases
// ============================================================================

TEST(QuadTreeEdgeCases, BoundaryPoints)
{
  QuadTree tree(0, 100, 0, 100, 4);
  
  // Test exact boundary points
  Point * p1 = tree.insert(Point(0, 0));
  Point * p2 = tree.insert(Point(0, 100)); // Upper bound, should fail
  Point * p3 = tree.insert(Point(100, 0)); // Upper bound, should fail
  Point * p4 = tree.insert(Point(100, 100)); // Upper bounds, should fail
  
  EXPECT_NE(p1, nullptr);
  EXPECT_EQ(p2, nullptr); // Upper bounds are exclusive
  EXPECT_EQ(p3, nullptr);
  EXPECT_EQ(p4, nullptr);
}

TEST(QuadTreeEdgeCases, MidpointCoordinates)
{
  QuadTree tree(0, 100, 0, 100, 1);
  
  // Insert point exactly at midpoint
  tree.insert(Point(50, 50));
  
  // Insert more to trigger split
  tree.insert(Point(25, 25));
  
  // Midpoint should be in one of the quadrants
  EXPECT_NE(tree.search(Point(50, 50)), nullptr);
}

TEST(QuadTreeEdgeCases, SinglePointCapacity)
{
  QuadTree tree(0, 100, 0, 100, 1);
  
  tree.insert(Point(25, 25));
  tree.insert(Point(30, 30));
  
  // Should trigger immediate split
  EXPECT_FALSE(tree.get_root()->is_leaf());
}

TEST(QuadTreeEdgeCases, VerySmallRegion)
{
  QuadTree tree(0, 1, 0, 1, 2);
  
  tree.insert(Point(0.1, 0.1));
  tree.insert(Point(0.9, 0.9));
  
  EXPECT_NE(tree.search(Point(0.1, 0.1)), nullptr);
  EXPECT_NE(tree.search(Point(0.9, 0.9)), nullptr);
}

TEST(QuadTreeEdgeCases, VeryLargeRegion)
{
  QuadTree tree(-1e9, 1e9, -1e9, 1e9, 4);
  
  tree.insert(Point(0, 0));
  tree.insert(Point(1e8, 1e8));
  tree.insert(Point(-5e8, -5e8));
  
  EXPECT_NE(tree.search(Point(0, 0)), nullptr);
  EXPECT_NE(tree.search(Point(1e8, 1e8)), nullptr);
  EXPECT_NE(tree.search(Point(-5e8, -5e8)), nullptr);
}

// ============================================================================
// Traversal Tests
// ============================================================================

TEST(QuadTreeTraversal, ForEachNode)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  for (int i = 0; i < 10; ++i)
    {
      tree.insert(Point(10 * i, 10 * i));
    }
  
  size_t node_count = 0;
  tree.for_each([&node_count](QuadNode * node) {
    ++node_count;
    EXPECT_NE(node, nullptr);
  });
  
  EXPECT_GT(node_count, 0);
}

TEST(QuadTreeTraversal, CountLeaves)
{
  QuadTree tree(0, 100, 0, 100, 2);
  
  tree.insert(Point(10, 10));
  tree.insert(Point(20, 20));
  tree.insert(Point(80, 80));
  tree.insert(Point(90, 90));
  
  size_t leaf_count = 0;
  tree.for_each([&leaf_count](QuadNode * node) {
    if (node->is_leaf())
      ++leaf_count;
  });
  
  EXPECT_GT(leaf_count, 0);
}

// ============================================================================
// Fuzz Tests
// ============================================================================

TEST(QuadTreeFuzz, RandomOperations)
{
  QuadTree tree(0, 1000, 0, 1000, 4);
  
  std::mt19937 gen(99999);
  // Use integer coordinates to avoid mpq_class precision issues with double conversion
  std::uniform_int_distribution<int> coord_dis(0, 999);
  std::uniform_int_distribution<> op_dis(0, 2);
  
  // Store points directly to avoid string conversion precision issues
  std::vector<Point> inserted_points;
  
  for (int i = 0; i < 1000; ++i)
    {
      int op = op_dis(gen);
      
      if (op == 0 || op == 1) // Insert
        {
          Point p(coord_dis(gen), coord_dis(gen));
          Point * result = tree.insert(p);
          if (result != nullptr)
            inserted_points.push_back(p);
        }
      else if (op == 2 && not inserted_points.empty()) // Remove
        {
          // Pick a random inserted point
          size_t idx = gen() % inserted_points.size();
          Point to_remove = inserted_points[idx];
          
          tree.remove(to_remove);
          inserted_points.erase(inserted_points.begin() + idx);
        }
    }
  
  // Verify consistency
  for (const auto & p : inserted_points)
    EXPECT_NE(tree.search(p), nullptr) 
      << "Point (" << p.get_x() << ", " << p.get_y() << ") should be in tree";
}

// ============================================================================
// Main
// ============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

