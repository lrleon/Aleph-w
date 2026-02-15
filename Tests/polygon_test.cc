
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
 * @file polygon_test.cc
 * @brief Comprehensive test suite for polygon.H - 2D polygon operations
 *
 * Tests cover:
 * - Vertex class: construction, assignment, navigation
 * - Polygon class: construction, vertex management, iterators, 
 *   self-intersection prevention, containment testing
 * - Regular_Polygon class: construction, vertex computation, iterators
 * - Edge cases: empty polygons, single vertices, colinearity
 * - Move semantics and copy operations
 */

#include <gtest/gtest.h>
#include <cmath>
#include <vector>
#include <polygon.H>

using namespace Aleph;

// Tolerance for floating-point comparisons
constexpr double EPSILON = 1e-6;

// Helper to compare Geom_Number with tolerance
bool approx_equal(const Geom_Number & a, const Geom_Number & b, 
                  double tol = EPSILON)
{
  return std::abs(a.get_d() - b.get_d()) < tol;
}

// Helper to compare Points
bool points_equal(const Point & a, const Point & b, double tol = EPSILON)
{
  return approx_equal(a.get_x(), b.get_x(), tol) && 
         approx_equal(a.get_y(), b.get_y(), tol);
}

//============================================================================
// Vertex Class Tests
//============================================================================

class VertexTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    p1 = Point(10, 20);
    p2 = Point(30, 40);
  }

  Point p1, p2;
};

TEST_F(VertexTest, DefaultConstruction)
{
  Vertex v;
  EXPECT_EQ(v.get_x(), 0);
  EXPECT_EQ(v.get_y(), 0);
}

TEST_F(VertexTest, ConstructFromPoint)
{
  Vertex v(p1);
  EXPECT_EQ(v.get_x(), p1.get_x());
  EXPECT_EQ(v.get_y(), p1.get_y());
}

TEST_F(VertexTest, CopyConstruction)
{
  Vertex v1(p1);
  Vertex v2(v1);
  
  EXPECT_EQ(v2.get_x(), v1.get_x());
  EXPECT_EQ(v2.get_y(), v1.get_y());
}

TEST_F(VertexTest, CopyAssignment)
{
  Vertex v1(p1);
  Vertex v2(p2);
  
  v2 = v1;
  
  EXPECT_EQ(v2.get_x(), v1.get_x());
  EXPECT_EQ(v2.get_y(), v1.get_y());
}

TEST_F(VertexTest, SelfAssignment)
{
  Vertex v(p1);
  v = v;
  
  EXPECT_EQ(v.get_x(), p1.get_x());
  EXPECT_EQ(v.get_y(), p1.get_y());
}

TEST_F(VertexTest, ToPointReturnsCorrectPointValue)
{
  Vertex v(p1);
  Point p = v.to_point();

  EXPECT_EQ(p.get_x(), p1.get_x());
  EXPECT_EQ(p.get_y(), p1.get_y());
}

TEST_F(VertexTest, DlinkToVertexConversion)
{
  Vertex v(p1);
  Dlink * link = &v;
  
  Vertex * recovered = Vertex::dlink_to_vertex(link);
  EXPECT_EQ(recovered, &v);
  EXPECT_EQ(recovered->get_x(), p1.get_x());
}

TEST_F(VertexTest, DlinkToVertexConstConversion)
{
  const Vertex v(p1);
  const Dlink * link = &v;
  
  const Vertex * recovered = Vertex::dlink_to_vertex(link);
  EXPECT_EQ(recovered, &v);
  EXPECT_EQ(recovered->get_x(), p1.get_x());
}

//============================================================================
// Polygon Construction Tests
//============================================================================

class PolygonConstructionTest : public ::testing::Test
{
protected:
  Polygon create_square()
  {
    Polygon poly;
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
    poly.close();
    return poly;
  }
  
  Polygon create_triangle()
  {
    Polygon poly;
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(50, 100));
    poly.close();
    return poly;
  }
};

TEST_F(PolygonConstructionTest, DefaultConstruction)
{
  Polygon poly;
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_FALSE(poly.is_closed());
}

TEST_F(PolygonConstructionTest, AddSingleVertex)
{
  Polygon poly;
  poly.add_vertex(Point(10, 20));
  
  EXPECT_EQ(poly.size(), 1u);
  EXPECT_FALSE(poly.is_closed());
}

TEST_F(PolygonConstructionTest, AddMultipleVertices)
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 0));
  poly.add_vertex(Point(100, 100));
  
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_FALSE(poly.is_closed());
}

TEST_F(PolygonConstructionTest, ClosePolygon)
{
  Polygon poly = create_triangle();
  
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_TRUE(poly.is_closed());
}

TEST_F(PolygonConstructionTest, CannotAddVertexToClosedPolygon)
{
  Polygon poly = create_triangle();
  
  EXPECT_THROW(poly.add_vertex(Point(50, 50)), std::domain_error);
}

TEST_F(PolygonConstructionTest, CannotCloseAlreadyClosedPolygon)
{
  Polygon poly = create_triangle();
  
  EXPECT_THROW(poly.close(), std::domain_error);
}

TEST_F(PolygonConstructionTest, CopyConstruction)
{
  Polygon original = create_square();
  Polygon copy(original);
  
  EXPECT_EQ(copy.size(), original.size());
  EXPECT_EQ(copy.is_closed(), original.is_closed());
  EXPECT_TRUE(points_equal(copy.lowest_point(), original.lowest_point()));
}

TEST_F(PolygonConstructionTest, MoveConstruction)
{
  Polygon original = create_square();
  size_t orig_size = original.size();
  
  Polygon moved(std::move(original));
  
  EXPECT_EQ(moved.size(), orig_size);
  EXPECT_TRUE(moved.is_closed());
  EXPECT_EQ(original.size(), 0u);  // NOLINT - testing moved-from state
}

TEST_F(PolygonConstructionTest, CopyAssignment)
{
  Polygon poly1 = create_square();
  Polygon poly2 = create_triangle();
  
  poly2 = poly1;
  
  EXPECT_EQ(poly2.size(), poly1.size());
  EXPECT_EQ(poly2.is_closed(), poly1.is_closed());
}

TEST_F(PolygonConstructionTest, MoveAssignment)
{
  Polygon poly1 = create_square();
  size_t orig_size = poly1.size();
  Polygon poly2;
  
  poly2 = std::move(poly1);
  
  EXPECT_EQ(poly2.size(), orig_size);
  EXPECT_TRUE(poly2.is_closed());
}

TEST_F(PolygonConstructionTest, SelfCopyAssignment)
{
  Polygon poly = create_square();
  poly = poly;
  
  EXPECT_EQ(poly.size(), 4u);
  EXPECT_TRUE(poly.is_closed());
}

TEST_F(PolygonConstructionTest, AddVertexWithCoordinates)
{
  Polygon poly;
  poly.add_vertex(Geom_Number(50), Geom_Number(75));
  
  EXPECT_EQ(poly.size(), 1u);
  EXPECT_EQ(poly.get_first_vertex().get_x(), 50);
  EXPECT_EQ(poly.get_first_vertex().get_y(), 75);
}

//============================================================================
// Polygon Extreme Points Tests
//============================================================================

class PolygonExtremePointsTest : public ::testing::Test {};

TEST_F(PolygonExtremePointsTest, ExtremePointsSingleVertex)
{
  Polygon poly;
  poly.add_vertex(Point(50, 75));
  
  EXPECT_EQ(poly.lowest_point().get_x(), 50);
  EXPECT_EQ(poly.lowest_point().get_y(), 75);
  EXPECT_EQ(poly.highest_point().get_y(), 75);
  EXPECT_EQ(poly.leftmost_point().get_x(), 50);
  EXPECT_EQ(poly.rightmost_point().get_x(), 50);
}

TEST_F(PolygonExtremePointsTest, ExtremePointsMultipleVertices)
{
  Polygon poly;
  poly.add_vertex(Point(10, 20));
  poly.add_vertex(Point(100, 5));
  poly.add_vertex(Point(50, 150));
  poly.add_vertex(Point(-20, 80));
  
  EXPECT_EQ(poly.lowest_point().get_y(), 5);
  EXPECT_EQ(poly.highest_point().get_y(), 150);
  EXPECT_EQ(poly.leftmost_point().get_x(), -20);
  EXPECT_EQ(poly.rightmost_point().get_x(), 100);
}

TEST_F(PolygonExtremePointsTest, NegativeCoordinates)
{
  Polygon poly;
  poly.add_vertex(Point(-100, -100));
  poly.add_vertex(Point(100, -100));
  poly.add_vertex(Point(100, 100));
  poly.add_vertex(Point(-100, 100));
  
  EXPECT_EQ(poly.lowest_point().get_y(), -100);
  EXPECT_EQ(poly.highest_point().get_y(), 100);
  EXPECT_EQ(poly.leftmost_point().get_x(), -100);
  EXPECT_EQ(poly.rightmost_point().get_x(), 100);
}

//============================================================================
// Polygon Vertex Access Tests
//============================================================================

class PolygonVertexAccessTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
  }
  
  Polygon poly;
};

TEST_F(PolygonVertexAccessTest, GetFirstVertex)
{
  const Vertex & first = poly.get_first_vertex();
  EXPECT_EQ(first.get_x(), 0);
  EXPECT_EQ(first.get_y(), 0);
}

TEST_F(PolygonVertexAccessTest, GetLastVertex)
{
  const Vertex & last = poly.get_last_vertex();
  EXPECT_EQ(last.get_x(), 0);
  EXPECT_EQ(last.get_y(), 100);
}

TEST_F(PolygonVertexAccessTest, GetFirstVertexEmptyPolygonThrows)
{
  Polygon empty;
  EXPECT_THROW(empty.get_first_vertex(), std::domain_error);
}

TEST_F(PolygonVertexAccessTest, GetLastVertexEmptyPolygonThrows)
{
  Polygon empty;
  EXPECT_THROW(empty.get_last_vertex(), std::domain_error);
}

TEST_F(PolygonVertexAccessTest, VertexBelongsToPolygon)
{
  const Vertex & first = poly.get_first_vertex();
  EXPECT_TRUE(poly.vertex_belong_polygon(first));
}

TEST_F(PolygonVertexAccessTest, VertexDoesNotBelongToPolygon)
{
  Vertex external(Point(999, 999));
  EXPECT_FALSE(poly.vertex_belong_polygon(external));
}

TEST_F(PolygonVertexAccessTest, GetNextVertex)
{
  const Vertex & first = poly.get_first_vertex();
  const Vertex & next = poly.get_next_vertex(first);
  
  EXPECT_EQ(next.get_x(), 100);
  EXPECT_EQ(next.get_y(), 0);
}

TEST_F(PolygonVertexAccessTest, GetPrevVertex)
{
  const Vertex & last = poly.get_last_vertex();
  const Vertex & prev = poly.get_prev_vertex(last);
  
  EXPECT_EQ(prev.get_x(), 100);
  EXPECT_EQ(prev.get_y(), 100);
}

//============================================================================
// Polygon Segment Access Tests
//============================================================================

class PolygonSegmentAccessTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
  }
  
  Polygon poly;
};

TEST_F(PolygonSegmentAccessTest, GetFirstSegment)
{
  Segment first = poly.get_first_segment();
  
  EXPECT_TRUE(points_equal(first.get_src_point(), Point(0, 0)));
  EXPECT_TRUE(points_equal(first.get_tgt_point(), Point(100, 0)));
}

TEST_F(PolygonSegmentAccessTest, GetLastSegment)
{
  Segment last = poly.get_last_segment();
  
  EXPECT_TRUE(points_equal(last.get_src_point(), Point(100, 0)));
  EXPECT_TRUE(points_equal(last.get_tgt_point(), Point(100, 100)));
}

TEST_F(PolygonSegmentAccessTest, GetFirstSegmentSingleVertexThrows)
{
  Polygon single;
  single.add_vertex(Point(0, 0));
  
  EXPECT_THROW(single.get_first_segment(), std::domain_error);
}

TEST_F(PolygonSegmentAccessTest, GetFirstSegmentEmptyPolygonThrows)
{
  Polygon empty;
  EXPECT_THROW(empty.get_first_segment(), std::domain_error);
}

//============================================================================
// Polygon Vertex Iterator Tests
//============================================================================

class PolygonVertexIteratorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
  }
  
  Polygon poly;
};

TEST_F(PolygonVertexIteratorTest, IterateAllVertices)
{
  size_t count = 0;
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
    ++count;
  
  EXPECT_EQ(count, 4u);
}

TEST_F(PolygonVertexIteratorTest, GetCurrentVertex)
{
  Polygon::Vertex_Iterator it(poly);
  Vertex & v = it.get_current_vertex();
  
  EXPECT_EQ(v.get_x(), 0);
  EXPECT_EQ(v.get_y(), 0);
}

TEST_F(PolygonVertexIteratorTest, EmptyPolygonThrows)
{
  Polygon empty;
  EXPECT_THROW(Polygon::Vertex_Iterator it(empty), std::domain_error);
}

//============================================================================
// Polygon Segment Iterator Tests
//============================================================================

class PolygonSegmentIteratorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
  }
  
  Polygon poly;
};

TEST_F(PolygonSegmentIteratorTest, OpenPolygonIteratesNMinus1Segments)
{
  // poly is not closed, so 4 vertices = 3 segments
  size_t count = 0;
  for (Polygon::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
    ++count;
  
  EXPECT_EQ(count, 3u);
}

TEST_F(PolygonSegmentIteratorTest, ClosedPolygonIteratesNSegments)
{
  poly.close();
  
  size_t count = 0;
  for (Polygon::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
    ++count;
  
  EXPECT_EQ(count, 4u);
}

TEST_F(PolygonSegmentIteratorTest, GetCurrentSegment)
{
  Polygon::Segment_Iterator it(poly);
  Segment s = it.get_current_segment();
  
  EXPECT_TRUE(points_equal(s.get_src_point(), Point(0, 0)));
  EXPECT_TRUE(points_equal(s.get_tgt_point(), Point(100, 0)));
}

TEST_F(PolygonSegmentIteratorTest, EmptyPolygonThrows)
{
  Polygon empty;
  EXPECT_THROW(Polygon::Segment_Iterator it(empty), std::domain_error);
}

TEST_F(PolygonSegmentIteratorTest, SingleVertexPolygonThrows)
{
  Polygon single;
  single.add_vertex(Point(0, 0));
  
  EXPECT_THROW(Polygon::Segment_Iterator it(single), std::domain_error);
}

//============================================================================
// Polygon Intersection Tests
//============================================================================

class PolygonIntersectionTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    square.add_vertex(Point(0, 0));
    square.add_vertex(Point(100, 0));
    square.add_vertex(Point(100, 100));
    square.add_vertex(Point(0, 100));
    square.close();
  }
  
  Polygon square;
};

TEST_F(PolygonIntersectionTest, SegmentIntersectsPolygon)
{
  Segment cross(Point(-50, 50), Point(150, 50));
  EXPECT_TRUE(square.intersects_with(cross));
}

TEST_F(PolygonIntersectionTest, SegmentDoesNotIntersectPolygon)
{
  Segment outside(Point(200, 0), Point(200, 100));
  EXPECT_FALSE(square.intersects_with(outside));
}

TEST_F(PolygonIntersectionTest, SegmentInsidePolygon)
{
  Segment inside(Point(25, 25), Point(75, 75));
  EXPECT_FALSE(square.intersects_with(inside));
}

//============================================================================
// Polygon Self-Intersection Prevention Tests
//============================================================================

class PolygonSelfIntersectionTest : public ::testing::Test {};

TEST_F(PolygonSelfIntersectionTest, ValidPolygonConstruction)
{
  Polygon poly;
  EXPECT_NO_THROW({
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
    poly.close();
  });
}

TEST_F(PolygonSelfIntersectionTest, SelfIntersectingVertexThrows)
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 0));
  poly.add_vertex(Point(100, 100));
  poly.add_vertex(Point(0, 100));
  
  // Adding a vertex that would create a crossing edge
  EXPECT_THROW(poly.add_vertex(Point(150, -50)), std::domain_error);
}

TEST_F(PolygonSelfIntersectionTest, SelfIntersectingAddVertexThrows)
{
  // Create a polygon where adding a vertex causes self-intersection
  // Shape: Start at (0,0), go right to (100,0), up to (100,100), 
  // then diagonally down-left toward (-50, 50)
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 0));
  poly.add_vertex(Point(100, 100));
  poly.add_vertex(Point(-50, 50));
  
  // Adding (50, -50) would create an edge from (-50, 50) to (50, -50)
  // which crosses the edge (0, 0) -> (100, 0)
  EXPECT_THROW(poly.add_vertex(Point(50, -50)), std::domain_error);
}

//============================================================================
// Polygon Colinearity Tests
//============================================================================

class PolygonColinearityTest : public ::testing::Test {};

TEST_F(PolygonColinearityTest, ColinearPointExtendsEdge)
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(50, 0));  // Will be replaced
  poly.add_vertex(Point(100, 0)); // Colinear, replaces previous
  
  // Only 2 vertices: colinear point replaced the previous one
  EXPECT_EQ(poly.size(), 2u);
}

TEST_F(PolygonColinearityTest, PointInsideLastSegmentThrows)
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 0));
  
  // Point inside the last segment
  EXPECT_THROW(poly.add_vertex(Point(50, 0)), std::domain_error);
}

//============================================================================
// Polygon Containment Tests
//============================================================================

class PolygonContainmentTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Create a square 0-100 x 0-100
    square.add_vertex(Point(0, 0));
    square.add_vertex(Point(100, 0));
    square.add_vertex(Point(100, 100));
    square.add_vertex(Point(0, 100));
    square.close();
  }
  
  Polygon square;
};

TEST_F(PolygonContainmentTest, PointInsidePolygon)
{
  Point inside(50, 50);
  EXPECT_TRUE(square.contains(inside));
}

TEST_F(PolygonContainmentTest, PointOutsidePolygon)
{
  Point outside(200, 200);
  EXPECT_FALSE(square.contains(outside));
}

TEST_F(PolygonContainmentTest, PointNearEdge)
{
  Point near_edge(1, 50);  // Just inside left edge
  EXPECT_TRUE(square.contains(near_edge));
}

TEST_F(PolygonContainmentTest, OpenPolygonContainmentThrows)
{
  Polygon open;
  open.add_vertex(Point(0, 0));
  open.add_vertex(Point(100, 0));
  open.add_vertex(Point(100, 100));
  
  EXPECT_THROW(open.contains(Point(50, 50)), std::domain_error);
}

//============================================================================
// Polygon Remove Vertex Tests
//============================================================================

class PolygonRemoveVertexTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    poly.add_vertex(Point(0, 0));
    poly.add_vertex(Point(100, 0));
    poly.add_vertex(Point(100, 100));
    poly.add_vertex(Point(0, 100));
  }
  
  Polygon poly;
};

TEST_F(PolygonRemoveVertexTest, RemoveExistingVertex)
{
  const Vertex & v = poly.get_first_vertex();
  poly.remove_vertex(v);
  
  EXPECT_EQ(poly.size(), 3u);
}

TEST_F(PolygonRemoveVertexTest, RemoveNonExistingVertexThrows)
{
  Vertex external(Point(999, 999));
  EXPECT_THROW(poly.remove_vertex(external), std::domain_error);
}

//============================================================================
// Polygon from Triangle Tests
//============================================================================

class PolygonFromTriangleTest : public ::testing::Test {};

TEST_F(PolygonFromTriangleTest, ConstructFromTriangle)
{
  Triangle tr(Point(0, 0), Point(100, 0), Point(50, 100));
  Polygon poly(tr);
  
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_TRUE(poly.is_closed());
}

TEST_F(PolygonFromTriangleTest, CorrectVerticesFromTriangle)
{
  // Use non-colinear points
  Triangle tr(Point(0, 0), Point(100, 0), Point(50, 87));
  Polygon poly(tr);
  
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_TRUE(poly.is_closed());
  
  // Verify that the polygon contains the centroid of the triangle
  Point centroid(50, 29);  // Approximately (0+100+50)/3, (0+0+87)/3
  EXPECT_TRUE(poly.contains(centroid));
}

//============================================================================
// Regular Polygon Construction Tests
//============================================================================

class RegularPolygonConstructionTest : public ::testing::Test {};

TEST_F(RegularPolygonConstructionTest, DefaultConstruction)
{
  Regular_Polygon poly;
  EXPECT_EQ(poly.size(), 0u);
  EXPECT_EQ(poly.get_side_size(), 0);
}

TEST_F(RegularPolygonConstructionTest, ConstructTriangle)
{
  Regular_Polygon tri(Point(0, 0), 100.0, 3);
  
  EXPECT_EQ(tri.size(), 3u);
  EXPECT_TRUE(tri.is_closed());
  EXPECT_NEAR(tri.get_side_size(), 100.0, EPSILON);
}

TEST_F(RegularPolygonConstructionTest, ConstructSquare)
{
  Regular_Polygon sq(Point(0, 0), 100.0, 4);
  
  EXPECT_EQ(sq.size(), 4u);
  EXPECT_NEAR(sq.get_side_size(), 100.0, EPSILON);
}

TEST_F(RegularPolygonConstructionTest, ConstructHexagon)
{
  Regular_Polygon hex(Point(100, 100), 50.0, 6);
  
  EXPECT_EQ(hex.size(), 6u);
  EXPECT_TRUE(points_equal(hex.get_center(), Point(100, 100)));
}

TEST_F(RegularPolygonConstructionTest, LessThanThreeSidesThrows)
{
  EXPECT_THROW(Regular_Polygon(Point(0, 0), 100.0, 2), std::domain_error);
  EXPECT_THROW(Regular_Polygon(Point(0, 0), 100.0, 1), std::domain_error);
  EXPECT_THROW(Regular_Polygon(Point(0, 0), 100.0, 0), std::domain_error);
}

TEST_F(RegularPolygonConstructionTest, RadiusCalculation)
{
  // For a regular hexagon with side s, the circumradius r = s
  Regular_Polygon hex(Point(0, 0), 100.0, 6);
  EXPECT_NEAR(hex.radius(), 100.0, EPSILON);
}

//============================================================================
// Regular Polygon Vertex Access Tests
//============================================================================

class RegularPolygonVertexTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Hexagon centered at origin with side 100
    hex = std::make_unique<Regular_Polygon>(Point(0, 0), 100.0, 6);
  }
  
  std::unique_ptr<Regular_Polygon> hex;
};

TEST_F(RegularPolygonVertexTest, GetVertexInRange)
{
  for (size_t i = 0; i < hex->size(); ++i)
    EXPECT_NO_THROW(hex->get_vertex(i));
}

TEST_F(RegularPolygonVertexTest, GetVertexOutOfRangeThrows)
{
  EXPECT_THROW(hex->get_vertex(6), std::out_of_range);
  EXPECT_THROW(hex->get_vertex(100), std::out_of_range);
}

TEST_F(RegularPolygonVertexTest, GetFirstVertex)
{
  Point first = hex->get_first_vertex();
  Point vertex0 = hex->get_vertex(0);
  
  EXPECT_TRUE(points_equal(first, vertex0));
}

TEST_F(RegularPolygonVertexTest, GetLastVertex)
{
  Point last = hex->get_last_vertex();
  Point vertex5 = hex->get_vertex(5);
  
  EXPECT_TRUE(points_equal(last, vertex5));
}

TEST_F(RegularPolygonVertexTest, VerticesAreEquidistantFromCenter)
{
  double r = hex->radius();
  
  for (size_t i = 0; i < hex->size(); ++i)
  {
    Point v = hex->get_vertex(i);
    Point center = hex->get_center();
    double dist = std::sqrt(
      std::pow(v.get_x().get_d() - center.get_x().get_d(), 2) +
      std::pow(v.get_y().get_d() - center.get_y().get_d(), 2)
    );
    EXPECT_NEAR(dist, r, EPSILON);
  }
}

//============================================================================
// Regular Polygon Segment Access Tests
//============================================================================

class RegularPolygonSegmentTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Square centered at origin
    sq = std::make_unique<Regular_Polygon>(Point(0, 0), 100.0, 4);
  }
  
  std::unique_ptr<Regular_Polygon> sq;
};

TEST_F(RegularPolygonSegmentTest, GetFirstSegment)
{
  Segment first = sq->get_first_segment();
  Point vertex0 = sq->get_vertex(0);
  Point vertex1 = sq->get_vertex(1);
  
  EXPECT_TRUE(points_equal(first.get_src_point(), vertex0));
  EXPECT_TRUE(points_equal(first.get_tgt_point(), vertex1));
}

TEST_F(RegularPolygonSegmentTest, GetLastSegment)
{
  Segment last = sq->get_last_segment();
  Point vertex2 = sq->get_vertex(2);
  Point vertex3 = sq->get_vertex(3);

  EXPECT_TRUE(points_equal(last.get_src_point(), vertex2));
  EXPECT_TRUE(points_equal(last.get_tgt_point(), vertex3));
}

TEST_F(RegularPolygonSegmentTest, AllSidesEqualLength)
{
  double expected = sq->get_side_size();
  
  for (size_t i = 0; i < sq->size(); ++i)
  {
    Point v1 = sq->get_vertex(i);
    Point v2 = sq->get_vertex((i + 1) % sq->size());
    
    Segment s(v1, v2);
    EXPECT_NEAR(s.length().get_d(), expected, 0.01);  // Larger tolerance for floating point
  }
}

//============================================================================
// Regular Polygon Vertex Iterator Tests
//============================================================================

class RegularPolygonVertexIteratorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    pentagon = std::make_unique<Regular_Polygon>(Point(0, 0), 50.0, 5);
  }
  
  std::unique_ptr<Regular_Polygon> pentagon;
};

TEST_F(RegularPolygonVertexIteratorTest, IterateAllVertices)
{
  size_t count = 0;
  for (Regular_Polygon::Vertex_Iterator it(*pentagon); it.has_curr(); it.next())
    ++count;
  
  EXPECT_EQ(count, 5u);
}

TEST_F(RegularPolygonVertexIteratorTest, GetCurrentVertex)
{
  Regular_Polygon::Vertex_Iterator it(*pentagon);
  Vertex & v = it.get_current_vertex();
  
  Point expected = pentagon->get_vertex(0);
  EXPECT_TRUE(points_equal(v, expected));
}

TEST_F(RegularPolygonVertexIteratorTest, NextWithoutCurrent)
{
  Regular_Polygon::Vertex_Iterator it(*pentagon);
  for (size_t i = 0; i < 5; ++i)
    it.next_ne();
  
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.next(), std::overflow_error);
}

TEST_F(RegularPolygonVertexIteratorTest, GetCurrentVertexWithoutCurrent)
{
  Regular_Polygon::Vertex_Iterator it(*pentagon);
  for (size_t i = 0; i < 5; ++i)
    it.next_ne();
  
  EXPECT_THROW(it.get_current_vertex(), std::overflow_error);
}

//============================================================================
// Regular Polygon Segment Iterator Tests
//============================================================================

class RegularPolygonSegmentIteratorTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    triangle = std::make_unique<Regular_Polygon>(Point(0, 0), 100.0, 3);
  }
  
  std::unique_ptr<Regular_Polygon> triangle;
};

TEST_F(RegularPolygonSegmentIteratorTest, IterateAllSegments)
{
  size_t count = 0;
  for (Regular_Polygon::Segment_Iterator it(*triangle); it.has_curr(); it.next())
    ++count;
  
  EXPECT_EQ(count, 3u);
}

TEST_F(RegularPolygonSegmentIteratorTest, GetCurrentSegment)
{
  Regular_Polygon::Segment_Iterator it(*triangle);
  Segment s = it.get_current_segment();
  
  Point v0 = triangle->get_vertex(0);
  Point v1 = triangle->get_vertex(1);
  
  EXPECT_TRUE(points_equal(s.get_src_point(), v0));
  EXPECT_TRUE(points_equal(s.get_tgt_point(), v1));
}

TEST_F(RegularPolygonSegmentIteratorTest, NextWithoutCurrent)
{
  Regular_Polygon::Segment_Iterator it(*triangle);
  for (size_t i = 0; i < 3; ++i)
    it.next_ne();
  
  EXPECT_FALSE(it.has_curr());
  EXPECT_THROW(it.next(), std::overflow_error);
}

TEST_F(RegularPolygonSegmentIteratorTest, GetCurrentSegmentWithoutCurrent)
{
  Regular_Polygon::Segment_Iterator it(*triangle);
  for (size_t i = 0; i < 3; ++i)
    it.next_ne();
  
  EXPECT_THROW(it.get_current_segment(), std::overflow_error);
}

//============================================================================
// Regular Polygon Extreme Points Tests
//============================================================================

class RegularPolygonExtremePointsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    // Hexagon centered at (100, 100) with radius ~100
    hex = std::make_unique<Regular_Polygon>(Point(100, 100), 100.0, 6);
  }
  
  std::unique_ptr<Regular_Polygon> hex;
};

TEST_F(RegularPolygonExtremePointsTest, LowestPoint)
{
  Point lowest = hex->lowest_point();
  Point center = hex->get_center();
  double r = hex->radius();
  
  EXPECT_TRUE(points_equal(lowest, center + Point(0, -r)));
}

TEST_F(RegularPolygonExtremePointsTest, HighestPoint)
{
  Point highest = hex->highest_point();
  Point center = hex->get_center();
  double r = hex->radius();
  
  EXPECT_TRUE(points_equal(highest, center + Point(0, r)));
}

TEST_F(RegularPolygonExtremePointsTest, LeftmostPoint)
{
  Point leftmost = hex->leftmost_point();
  Point center = hex->get_center();
  double r = hex->radius();
  
  EXPECT_TRUE(points_equal(leftmost, center + Point(-r, 0)));
}

TEST_F(RegularPolygonExtremePointsTest, RightmostPoint)
{
  Point rightmost = hex->rightmost_point();
  Point center = hex->get_center();
  double r = hex->radius();
  
  EXPECT_TRUE(points_equal(rightmost, center + Point(r, 0)));
}

//============================================================================
// Regular Polygon Rotation Tests
//============================================================================

class RegularPolygonRotationTest : public ::testing::Test {};

TEST_F(RegularPolygonRotationTest, NoRotation)
{
  Regular_Polygon sq(Point(0, 0), 100.0, 4, 0);
  
  // First vertex should be at the "south" position (negative y)
  Point v0 = sq.get_vertex(0);
  EXPECT_LT(v0.get_y().get_d(), 0);  // Below center
  EXPECT_NEAR(v0.get_x().get_d(), 0, 0.1);  // On vertical axis
}

TEST_F(RegularPolygonRotationTest, RotationChangesVertexPositions)
{
  Regular_Polygon sq1(Point(0, 0), 100.0, 4, 0);
  Regular_Polygon sq2(Point(0, 0), 100.0, 4, PI / 4);  // 45 degree rotation
  
  // First vertex of rotated polygon should be different from non-rotated
  Point v0_orig = sq1.get_vertex(0);
  Point v0_rot = sq2.get_vertex(0);
  
  // The x-coordinate should change after rotation
  EXPECT_FALSE(approx_equal(v0_orig.get_x(), v0_rot.get_x(), 0.1));
}

TEST_F(RegularPolygonRotationTest, AllVerticesAtSameRadiusAfterRotation)
{
  // Rotated hexagon should still have all vertices equidistant from center
  Regular_Polygon hex(Point(0, 0), 100.0, 6, PI / 6);  // 30 degree rotation
  double r = hex.radius();
  
  for (size_t i = 0; i < hex.size(); ++i)
  {
    Point v = hex.get_vertex(i);
    double dist = std::sqrt(
      std::pow(v.get_x().get_d(), 2) + std::pow(v.get_y().get_d(), 2)
    );
    EXPECT_NEAR(dist, r, 0.001);
  }
}

//============================================================================
// Polygon from Regular Polygon Tests
//============================================================================

class PolygonFromRegularTest : public ::testing::Test {};

TEST_F(PolygonFromRegularTest, ConstructFromRegularPolygon)
{
  Regular_Polygon hex(Point(100, 100), 50.0, 6);
  Polygon poly(hex);
  
  EXPECT_EQ(poly.size(), 6u);
  EXPECT_TRUE(poly.is_closed());
}

TEST_F(PolygonFromRegularTest, AssignFromRegularPolygon)
{
  Regular_Polygon sq(Point(0, 0), 100.0, 4);
  Polygon poly;
  
  poly = sq;
  
  EXPECT_EQ(poly.size(), 4u);
  EXPECT_TRUE(poly.is_closed());
}

TEST_F(PolygonFromRegularTest, VerticesMatch)
{
  Regular_Polygon tri(Point(0, 0), 100.0, 3);
  Polygon poly(tri);
  
  // Both should have same number of vertices
  EXPECT_EQ(poly.size(), tri.size());
  
  // Verify each vertex
  size_t i = 0;
  for (Polygon::Vertex_Iterator it(poly); it.has_curr(); it.next_ne(), ++i)
  {
    Point polyV = it.get_current_vertex();
    Point regV = tri.get_vertex(i);
    EXPECT_TRUE(points_equal(polyV, regV));
  }
}

//============================================================================
// Type Traits and Noexcept Tests
//============================================================================

TEST(PolygonTypeTraits, MoveConstructorIsNoexcept)
{
  EXPECT_TRUE(std::is_nothrow_move_constructible<Polygon>::value);
}

TEST(PolygonTypeTraits, MoveAssignmentIsNoexcept)
{
  EXPECT_TRUE(std::is_nothrow_move_assignable<Polygon>::value);
}

//============================================================================
// Edge Cases and Stress Tests
//============================================================================

TEST(PolygonEdgeCases, PolygonWithManyVertices)
{
  Polygon poly;
  const size_t N = 1000;
  
  // Create a circle-like polygon with N vertices
  for (size_t i = 0; i < N; ++i)
  {
    double angle = 2.0 * PI * i / N;
    double x = 1000.0 * cos(angle);
    double y = 1000.0 * sin(angle);
    poly.add_vertex(Point(x, y));
  }
  poly.close();
  
  EXPECT_EQ(poly.size(), N);
  EXPECT_TRUE(poly.is_closed());
  
  // Containment test should work
  EXPECT_TRUE(poly.contains(Point(0, 0)));
  EXPECT_FALSE(poly.contains(Point(2000, 2000)));
}

TEST(PolygonEdgeCases, RegularPolygonWithManyVertices)
{
  Regular_Polygon poly(Point(0, 0), 100.0, 100);
  
  EXPECT_EQ(poly.size(), 100u);
  
  // All vertices should be at the same distance from center
  double r = poly.radius();
  for (size_t i = 0; i < poly.size(); ++i)
  {
    Point v = poly.get_vertex(i);
    double dist = std::sqrt(
      std::pow(v.get_x().get_d(), 2) + std::pow(v.get_y().get_d(), 2)
    );
    EXPECT_NEAR(dist, r, 0.001);
  }
}

TEST(PolygonEdgeCases, VerySmallPolygon)
{
  Polygon poly;
  poly.add_vertex(Point(0.0001, 0.0001));
  poly.add_vertex(Point(0.0002, 0.0001));
  poly.add_vertex(Point(0.00015, 0.0002));
  poly.close();
  
  EXPECT_EQ(poly.size(), 3u);
  EXPECT_TRUE(poly.is_closed());
}

TEST(PolygonEdgeCases, NegativeCoordinatesPolygon)
{
  Polygon poly;
  poly.add_vertex(Point(-1000, -1000));
  poly.add_vertex(Point(-500, -1000));
  poly.add_vertex(Point(-500, -500));
  poly.add_vertex(Point(-1000, -500));
  poly.close();
  
  EXPECT_TRUE(poly.contains(Point(-750, -750)));
  EXPECT_FALSE(poly.contains(Point(0, 0)));
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
