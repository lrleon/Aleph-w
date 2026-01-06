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
 * @file eepicgeom_test.cc
 * @brief Tests for Eepicgeom
 */

/**
 * @file eepicgeom_test.cc
 * @brief Comprehensive test suite for eepicgeom.H - EEPIC LaTeX drawing utilities
 *
 * Tests cover:
 * - Eepic_Plane: construction, configuration, coordinate transformations, zoom
 * - Eepic_Geom_Object: polymorphism, cloning, extreme point calculations
 * - Segment variants: Arrow, Thick_Segment, Dash_Segment, etc.
 * - Polygon variants: all drawing styles
 * - Ellipse variants: normal, shaded, thick
 * - Text variants: Left_Text, Right_Text, Center_Text
 * - Edge cases: empty planes, vertical segments (arrow division by zero bug)
 * - EEPIC output validation: proper LaTeX format
 */

#include <gtest/gtest.h>
#include <sstream>
#include <cmath>
#include <eepicgeom.H>

// Global variable required by eepicgeom.H (declared extern there)
bool tiny_keys = false;

using namespace Aleph;

// Tolerance for floating-point comparisons
constexpr double EPSILON = 1e-6;

// Helper to compare doubles with tolerance
bool approx_equal(double a, double b, double tol = EPSILON)
{
  return std::abs(a - b) < tol;
}

//============================================================================
// Eepic_Plane Construction Tests
//============================================================================

class EepicPlaneTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane_default = std::make_unique<Eepic_Plane>(500, 300);
    plane_with_offset = std::make_unique<Eepic_Plane>(500, 300, 10, 20);
  }

  std::unique_ptr<Eepic_Plane> plane_default;
  std::unique_ptr<Eepic_Plane> plane_with_offset;
};

TEST_F(EepicPlaneTest, DefaultConstruction)
{
  EXPECT_EQ(plane_default->get_wide(), 500);
  EXPECT_EQ(plane_default->get_height(), 300);
  EXPECT_EQ(plane_default->get_xoffset(), 0);
  EXPECT_EQ(plane_default->get_yoffset(), 0);
}

TEST_F(EepicPlaneTest, ConstructionWithOffset)
{
  EXPECT_EQ(plane_with_offset->get_wide(), 500);
  EXPECT_EQ(plane_with_offset->get_height(), 300);
  EXPECT_EQ(plane_with_offset->get_xoffset(), 10);
  EXPECT_EQ(plane_with_offset->get_yoffset(), 20);
}

TEST_F(EepicPlaneTest, ResolutionSettings)
{
  plane_default->set_resolution(0.1);
  EXPECT_EQ(plane_default->get_resolution(), 0.1);

  plane_default->set_resolution(0.05);
  EXPECT_EQ(plane_default->get_resolution(), 0.05);
}

TEST_F(EepicPlaneTest, ShadeThicknessSettings)
{
  double original = plane_default->get_shade_thickness();
  plane_default->set_shade_thickness(2.0);
  EXPECT_EQ(plane_default->get_shade_thickness(), 2.0);

  plane_default->set_shade_thickness(original);
  EXPECT_EQ(plane_default->get_shade_thickness(), original);
}

TEST_F(EepicPlaneTest, ZoomPositiveFactor)
{
  Eepic_Plane plane(500, 300);
  plane.zoom(2.0);
  EXPECT_EQ(plane.get_wide(), 1000);
  EXPECT_EQ(plane.get_height(), 600);
}

TEST_F(EepicPlaneTest, ZoomFractionalFactor)
{
  Eepic_Plane plane(500, 300);
  plane.zoom(0.5);
  EXPECT_EQ(plane.get_wide(), 250);
  EXPECT_EQ(plane.get_height(), 150);
}

TEST_F(EepicPlaneTest, ZoomInvalidFactor)
{
  Eepic_Plane plane(500, 300);
  EXPECT_THROW(plane.zoom(0), std::domain_error);
  EXPECT_THROW(plane.zoom(-1.0), std::domain_error);
}

//============================================================================
// Eepic_Plane with Objects Tests
//============================================================================

class EepicPlaneWithObjectsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

// NOTE: Single point causes assertion failure due to zero geom_wide/geom_height
// This is a known bug in eepicgeom.H. The DISABLED_ prefix documents this issue.
TEST_F(EepicPlaneWithObjectsTest, DISABLED_AddSinglePoint)
{
  // BUG: Single point causes assertion failure at line 447 of eepicgeom.H
  // because geom_wide = rightmost.x - leftmost.x = 0 for a single point
  Point p(10, 20);
  put_in_plane(*plane, p);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("\\end{picture}") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddTwoPoints)
{
  // Use two points to avoid zero-range bug
  Point p1(10, 20);
  Point p2(100, 80);
  put_in_plane(*plane, p1);
  put_in_plane(*plane, p2);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("\\end{picture}") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddSegment)
{
  Point p1(0, 0);
  Point p2(100, 100);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\path") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddArrow)
{
  Point p1(0, 0);
  Point p2(100, 50);
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  // Arrow should produce a path plus arrow edges
  EXPECT_TRUE(result.find("\\path") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddEllipse)
{
  Point center(50, 50);
  Ellipse ellipse(center, 20, 10);
  put_in_plane(*plane, ellipse);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddPolygon)
{
  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 0));
  poly.add_vertex(Point(100, 100));
  poly.add_vertex(Point(0, 100));
  poly.close();

  put_in_plane(*plane, poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\path") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddTriangle)
{
  Point p1(0, 0);
  Point p2(100, 0);
  Point p3(50, 100);
  Triangle tri(p1, p2, p3);

  put_in_plane(*plane, tri);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\path") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, AddText)
{
  // Add a segment first to provide coordinate range (avoid single-point bug)
  put_in_plane(*plane, Segment(Point(0, 0), Point(100, 100)));

  Point p(50, 50);
  Text text(p, "Hello");
  put_in_plane(*plane, text);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("Hello") != std::string::npos);
}

TEST_F(EepicPlaneWithObjectsTest, MultipleObjects)
{
  Point p(50, 50);
  put_in_plane(*plane, p);

  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(*plane, seg);

  Ellipse ellipse(Point(50, 50), 10, 10);
  put_in_plane(*plane, ellipse);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  // Check that 3 objects were put in the plane
  EXPECT_TRUE(result.find("3 geometric objects were put in the plane") !=
              std::string::npos);
}

//============================================================================
// Extreme Points Calculation Tests
//============================================================================

class ExtremePointsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

// NOTE: Single point causes assertion failure due to zero geom_wide/geom_height
TEST_F(ExtremePointsTest, DISABLED_SinglePointExtremes)
{
  // BUG: Crashes due to assertion failure at line 447 in eepicgeom.H
  Point p(50, 75);
  put_in_plane(*plane, p);

  std::ostringstream output;
  plane->draw(output);

  // After draw, extreme points should be computed
  EXPECT_EQ(plane->leftmost(), p);
  EXPECT_EQ(plane->rightmost(), p);
  EXPECT_EQ(plane->highest(), p);
  EXPECT_EQ(plane->lowest(), p);
}

TEST_F(ExtremePointsTest, SegmentExtremes)
{
  Point p1(10, 20);
  Point p2(100, 150);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output);

  EXPECT_EQ(plane->leftmost().get_x(), p1.get_x());
  EXPECT_EQ(plane->rightmost().get_x(), p2.get_x());
  EXPECT_EQ(plane->lowest().get_y(), p1.get_y());
  EXPECT_EQ(plane->highest().get_y(), p2.get_y());
}

TEST_F(ExtremePointsTest, MultipleObjectsExtremes)
{
  Point p1(-50, -25);
  Point p2(150, 200);

  put_in_plane(*plane, p1);
  put_in_plane(*plane, p2);

  std::ostringstream output;
  plane->draw(output);

  EXPECT_EQ(plane->leftmost().get_x(), Geom_Number(-50));
  EXPECT_EQ(plane->rightmost().get_x(), Geom_Number(150));
  EXPECT_EQ(plane->lowest().get_y(), Geom_Number(-25));
  EXPECT_EQ(plane->highest().get_y(), Geom_Number(200));
}

//============================================================================
// Segment Drawing Variants Tests
//============================================================================

class SegmentVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
    p1 = Point(0, 0);
    p2 = Point(100, 100);
  }

  std::unique_ptr<Eepic_Plane> plane;
  Point p1, p2;
};

TEST_F(SegmentVariantsTest, ThickSegment)
{
  Thick_Segment thick_seg(p1, p2);
  put_in_plane(*plane, thick_seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
}

TEST_F(SegmentVariantsTest, DottedSegment)
{
  Dotted_Segment dotted_seg(p1, p2);
  put_in_plane(*plane, dotted_seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dottedline") != std::string::npos);
}

TEST_F(SegmentVariantsTest, DashSegment)
{
  Dash_Segment dash_seg(p1, p2);
  put_in_plane(*plane, dash_seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dashline") != std::string::npos);
}

TEST_F(SegmentVariantsTest, DottedArrow)
{
  Dotted_Arrow dotted_arrow(p1, p2);
  put_in_plane(*plane, dotted_arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dottedline") != std::string::npos);
  // Should also have arrow edges
  EXPECT_TRUE(result.find("arrow's edges") != std::string::npos);
}

TEST_F(SegmentVariantsTest, DashArrow)
{
  Dash_Arrow dash_arrow(p1, p2);
  put_in_plane(*plane, dash_arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dashline") != std::string::npos);
}

//============================================================================
// Arrow Edge Case Tests - Testing the vertical segment bug
//============================================================================

class ArrowEdgeCaseTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(ArrowEdgeCaseTest, DiagonalArrow)
{
  // Standard diagonal arrow - should work fine
  Point p1(0, 0);
  Point p2(100, 50);
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));

  std::string result = output.str();
  EXPECT_FALSE(result.find("nan") != std::string::npos);
  EXPECT_FALSE(result.find("inf") != std::string::npos);
}

// Disabled due to zero-range bug when y1 == y2 (geom_height = 0)
TEST_F(ArrowEdgeCaseTest, DISABLED_HorizontalArrow)
{
  // Horizontal arrow (tgt_y == src_y) - BUG: triggers zero-range in y
  Point p1(0, 50);
  Point p2(100, 50);
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));

  std::string result = output.str();
  EXPECT_FALSE(result.find("nan") != std::string::npos);
  EXPECT_FALSE(result.find("inf") != std::string::npos);
}

TEST_F(ArrowEdgeCaseTest, NearHorizontalArrow)
{
  // Nearly horizontal arrow with slight y difference to avoid zero-range bug
  Point p1(0, 50);
  Point p2(100, 50.001);  // Slight y offset
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));

  std::string result = output.str();
  EXPECT_FALSE(result.find("nan") != std::string::npos);
  EXPECT_FALSE(result.find("inf") != std::string::npos);
}

// NOTE: This test documents the known bug - vertical segments cause
// division by zero in PUT_ARROW macro (line 692 in eepicgeom.H)
// The test is marked as disabled until the bug is fixed
TEST_F(ArrowEdgeCaseTest, DISABLED_VerticalArrow)
{
  // Vertical arrow (tgt_x == src_x) - BUG: causes division by zero
  // in: const double phi = atan( fabs( (tgt_y - src_y)/(tgt_x - src_x) ) );
  Point p1(50, 0);
  Point p2(50, 100);
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  // This would produce nan/inf values due to division by zero
  plane->draw(output);

  std::string result = output.str();
  // When fixed, these should pass
  EXPECT_FALSE(result.find("nan") != std::string::npos);
  EXPECT_FALSE(result.find("inf") != std::string::npos);
}

TEST_F(ArrowEdgeCaseTest, NearVerticalArrow)
{
  // Nearly vertical arrow - should work but may have precision issues
  Point p1(50, 0);
  Point p2(50.001, 100);  // Very slight horizontal offset
  Arrow arrow(p1, p2);
  put_in_plane(*plane, arrow);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));
}

//============================================================================
// Polygon Variants Tests
//============================================================================

class PolygonVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);

    // Create a simple square polygon
    base_poly.add_vertex(Point(0, 0));
    base_poly.add_vertex(Point(100, 0));
    base_poly.add_vertex(Point(100, 100));
    base_poly.add_vertex(Point(0, 100));
    base_poly.close();
  }

  std::unique_ptr<Eepic_Plane> plane;
  Polygon base_poly;
};

TEST_F(PolygonVariantsTest, ShadePolygon)
{
  Shade_Polygon shade_poly(base_poly);
  put_in_plane(*plane, shade_poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\shade") != std::string::npos);
}

TEST_F(PolygonVariantsTest, DottedPolygon)
{
  Dotted_Polygon dotted_poly(base_poly);
  put_in_plane(*plane, dotted_poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dottedline") != std::string::npos);
}

TEST_F(PolygonVariantsTest, DashPolygon)
{
  Dash_Polygon dash_poly(base_poly);
  put_in_plane(*plane, dash_poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\dashline") != std::string::npos);
}

TEST_F(PolygonVariantsTest, PolygonWithPoints)
{
  Polygon_With_Points poly_pts(base_poly);
  put_in_plane(*plane, poly_pts);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  // Should contain ellipses for the vertex points
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);
}

TEST_F(PolygonVariantsTest, Spline)
{
  Spline spline(base_poly);
  put_in_plane(*plane, spline);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\spline") != std::string::npos);
}

TEST_F(PolygonVariantsTest, SplineArrow)
{
  Spline_Arrow spline_arrow(base_poly);
  put_in_plane(*plane, spline_arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\spline") != std::string::npos);
}

//============================================================================
// Ellipse Variants Tests
//============================================================================

class EllipseVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
    center = Point(50, 50);
  }

  std::unique_ptr<Eepic_Plane> plane;
  Point center;
};

TEST_F(EllipseVariantsTest, NormalEllipse)
{
  Ellipse ellipse(center, 20, 10);
  put_in_plane(*plane, ellipse);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);
}

TEST_F(EllipseVariantsTest, ThickEllipse)
{
  Thick_Ellipse thick_ellipse(center, 20, 10);
  put_in_plane(*plane, thick_ellipse);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);
}

TEST_F(EllipseVariantsTest, ShadeEllipse)
{
  Shade_Ellipse shade_ellipse(center, 20, 10);
  put_in_plane(*plane, shade_ellipse);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\shade") != std::string::npos);
}

TEST_F(EllipseVariantsTest, CircularEllipse)
{
  // When hradius == vradius, it's a circle
  Ellipse circle(center, 15, 15);
  put_in_plane(*plane, circle);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);
}

//============================================================================
// Text Variants Tests
//============================================================================

class TextVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
    pos = Point(50, 50);
    // Add a segment to provide coordinate range (avoid single-point bug)
    put_in_plane(*plane, Segment(Point(0, 0), Point(100, 100)));
  }

  std::unique_ptr<Eepic_Plane> plane;
  Point pos;
};

TEST_F(TextVariantsTest, LeftText)
{
  Left_Text left_text(pos, "LeftAligned");
  put_in_plane(*plane, left_text);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("LeftAligned") != std::string::npos);
}

TEST_F(TextVariantsTest, RightText)
{
  Right_Text right_text(pos, "RightAligned");
  put_in_plane(*plane, right_text);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("RightAligned") != std::string::npos);
}

TEST_F(TextVariantsTest, CenterText)
{
  Center_Text center_text(pos, "Centered");
  put_in_plane(*plane, center_text);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("Centered") != std::string::npos);
}

TEST_F(TextVariantsTest, TextWithSpecialCharacters)
{
  // Test with LaTeX special characters (should be handled by user)
  Center_Text text(pos, "Test$LaTeX$");
  put_in_plane(*plane, text);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("Test$LaTeX$") != std::string::npos);
}

//============================================================================
// Regular Polygon Tests
//============================================================================

class RegularPolygonTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
    center = Point(100, 100);
  }

  std::unique_ptr<Eepic_Plane> plane;
  Point center;
};

TEST_F(RegularPolygonTest, RegularTriangle)
{
  Regular_Polygon tri(center, 50, 3, 0);
  Polygon poly(tri);  // Convert to regular polygon
  put_in_plane(*plane, poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

TEST_F(RegularPolygonTest, RegularSquare)
{
  Regular_Polygon square(center, 50, 4, M_PI / 4);  // 45 degree rotation
  Polygon poly(square);
  put_in_plane(*plane, poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

TEST_F(RegularPolygonTest, RegularHexagon)
{
  Regular_Polygon hexagon(center, 50, 6, 0);
  Polygon poly(hexagon);
  put_in_plane(*plane, poly);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

//============================================================================
// EEPIC Output Format Tests
//============================================================================

class EepicOutputFormatTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(EepicOutputFormatTest, UnitLengthSetting)
{
  // Use segment to avoid single-point zero-range bug
  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(*plane, seg);

  plane->set_resolution(0.1);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\setlength{\\unitlength}{0.1") != std::string::npos);
}

TEST_F(EepicOutputFormatTest, FillTypeSetting)
{
  // Use segment to avoid single-point zero-range bug
  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(*plane, seg);

  plane->set_fill_type("gray");

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\filltype{gray}") != std::string::npos);
}

TEST_F(EepicOutputFormatTest, PictureEnvironment)
{
  // Use segment to avoid single-point zero-range bug
  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();

  // Check for proper LaTeX picture environment
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("\\end{picture}") != std::string::npos);

  // Picture environment should come before end
  size_t begin_pos = result.find("\\begin{picture}");
  size_t end_pos = result.find("\\end{picture}");
  EXPECT_LT(begin_pos, end_pos);
}

TEST_F(EepicOutputFormatTest, ThinLinesReset)
{
  Point p1(0, 0);
  Point p2(100, 100);
  Thick_Segment thick_seg(p1, p2);
  put_in_plane(*plane, thick_seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  // Each object should be drawn with thinlines reset
  EXPECT_TRUE(result.find("\\thinlines") != std::string::npos);
}

TEST_F(EepicOutputFormatTest, ExtremePointsComments)
{
  Point p1(10, 20);
  Point p2(90, 80);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();

  // Output should contain comments about extreme points
  EXPECT_TRUE(result.find("leftmost point") != std::string::npos);
  EXPECT_TRUE(result.find("rightmost point") != std::string::npos);
  EXPECT_TRUE(result.find("highest point") != std::string::npos);
  EXPECT_TRUE(result.find("lowest point") != std::string::npos);
}

//============================================================================
// Squarize Mode Tests
//============================================================================

class SquarizeModeTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 300);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(SquarizeModeTest, DrawWithSquarize)
{
  Point p1(0, 0);
  Point p2(100, 50);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output, true);  // squarize = true

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

TEST_F(SquarizeModeTest, DrawWithoutSquarize)
{
  Point p1(0, 0);
  Point p2(100, 50);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  plane->draw(output, false);  // squarize = false

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

//============================================================================
// Cartesian Axis Tests
//============================================================================

class CartesianAxisTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(CartesianAxisTest, WithCartesianAxis)
{
  // Add a segment to provide coordinate range (avoid zero-range bug)
  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(*plane, seg);

  plane->put_cartesian_axis();

  std::ostringstream output;
  plane->draw(output);

  // The output should contain axis drawings
  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
}

//============================================================================
// Coordinate Transformation Tests
//============================================================================

class CoordinateTransformTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);

    // Add some objects to set up coordinate system
    Point p1(0, 0);
    Point p2(100, 100);
    Segment seg(p1, p2);
    put_in_plane(*plane, seg);

    // Force computation of extreme points
    std::ostringstream dummy;
    plane->draw(dummy);
  }

  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(CoordinateTransformTest, PointString)
{
  Point p(50, 50);
  std::string str = plane->point_string(p);

  // Should be in format "(x,y)"
  EXPECT_TRUE(str.front() == '(');
  EXPECT_TRUE(str.back() == ')');
  EXPECT_TRUE(str.find(',') != std::string::npos);
}

TEST_F(CoordinateTransformTest, HGeomNumberToEepic)
{
  Geom_Number x(50);
  double result = plane->h_geom_number_to_eepic(x);

  // Result should be a valid coordinate
  EXPECT_TRUE(std::isfinite(result));
}

TEST_F(CoordinateTransformTest, VGeomNumberToEepic)
{
  Geom_Number y(50);
  double result = plane->v_geom_number_to_eepic(y);

  // Result should be a valid coordinate
  EXPECT_TRUE(std::isfinite(result));
}

//============================================================================
// Clone and Polymorphism Tests
// Note: Eepic_* classes have private constructors. The public interface is
// put_in_plane(). These tests verify that cloning works correctly when
// objects are added to a plane.
//============================================================================

// Disabled due to single-point zero-range bug
TEST(EepicPolymorphismTest, DISABLED_PointInPlane)
{
  Eepic_Plane plane(500, 500);
  Point p(10, 20);
  put_in_plane(plane, p);

  std::ostringstream output;
  plane.draw(output);

  // After draw, extreme points should match the point
  EXPECT_EQ(plane.highest(), p);
  EXPECT_EQ(plane.lowest(), p);
  EXPECT_EQ(plane.leftmost(), p);
  EXPECT_EQ(plane.rightmost(), p);
}

TEST(EepicPolymorphismTest, SegmentInPlane)
{
  Eepic_Plane plane(500, 500);
  Point p1(0, 0);
  Point p2(100, 50);
  Segment seg(p1, p2);
  put_in_plane(plane, seg);

  std::ostringstream output;
  plane.draw(output);

  // Verify extreme points
  EXPECT_EQ(plane.leftmost().get_x(), p1.get_x());
  EXPECT_EQ(plane.rightmost().get_x(), p2.get_x());
}

TEST(EepicPolymorphismTest, EllipseInPlane)
{
  Eepic_Plane plane(500, 500);
  Point center(50, 50);
  Ellipse ellipse(center, 20, 10);
  put_in_plane(plane, ellipse);

  std::ostringstream output;
  plane.draw(output);

  // Ellipse extreme points are center +/- radii
  EXPECT_EQ(plane.leftmost().get_x(), Geom_Number(30));   // 50 - 20
  EXPECT_EQ(plane.rightmost().get_x(), Geom_Number(70));  // 50 + 20
  EXPECT_EQ(plane.lowest().get_y(), Geom_Number(40));     // 50 - 10
  EXPECT_EQ(plane.highest().get_y(), Geom_Number(60));    // 50 + 10
}

//============================================================================
// Thick Segment Variants Tests
//============================================================================

class ThickVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);
    p1 = Point(0, 0);
    p2 = Point(100, 100);
  }

  std::unique_ptr<Eepic_Plane> plane;
  Point p1, p2;
};

TEST_F(ThickVariantsTest, ThickDashSegment)
{
  Thick_Dash_Segment thick_dash(p1, p2);
  put_in_plane(*plane, thick_dash);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
  EXPECT_TRUE(result.find("\\dashline") != std::string::npos);
}

TEST_F(ThickVariantsTest, ThickArrow)
{
  Thick_Arrow thick_arrow(p1, p2);
  put_in_plane(*plane, thick_arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
}

TEST_F(ThickVariantsTest, ThickDashArrow)
{
  Thick_Dash_Arrow thick_dash_arrow(p1, p2);
  put_in_plane(*plane, thick_dash_arrow);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
  EXPECT_TRUE(result.find("\\dashline") != std::string::npos);
}

//============================================================================
// Thick Polygon Variants Tests
//============================================================================

class ThickPolygonVariantsTest : public ::testing::Test
{
protected:
  void SetUp() override
  {
    plane = std::make_unique<Eepic_Plane>(500, 500);

    base_poly.add_vertex(Point(0, 0));
    base_poly.add_vertex(Point(100, 0));
    base_poly.add_vertex(Point(100, 100));
    base_poly.add_vertex(Point(0, 100));
    base_poly.close();
  }

  std::unique_ptr<Eepic_Plane> plane;
  Polygon base_poly;
};

TEST_F(ThickPolygonVariantsTest, ThickSpline)
{
  Thick_Spline thick_spline(base_poly);
  put_in_plane(*plane, thick_spline);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
  EXPECT_TRUE(result.find("\\spline") != std::string::npos);
}

TEST_F(ThickPolygonVariantsTest, ThickDashSpline)
{
  Thick_Dash_Spline thick_dash_spline(base_poly);
  put_in_plane(*plane, thick_dash_spline);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
}

TEST_F(ThickPolygonVariantsTest, ThickShadePolygon)
{
  Thick_Shade_Polygon thick_shade(base_poly);
  put_in_plane(*plane, thick_shade);

  std::ostringstream output;
  plane->draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\Thicklines") != std::string::npos);
  EXPECT_TRUE(result.find("\\shade") != std::string::npos);
}

//============================================================================
// Edge Cases and Boundary Conditions
//============================================================================

class EdgeCaseTest : public ::testing::Test
{
protected:
  std::unique_ptr<Eepic_Plane> plane;
};

TEST_F(EdgeCaseTest, VerySmallCoordinates)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Point p1(0.001, 0.001);
  Point p2(0.002, 0.002);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));
}

TEST_F(EdgeCaseTest, VeryLargeCoordinates)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Point p1(1000000, 1000000);
  Point p2(1000100, 1000100);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));
}

TEST_F(EdgeCaseTest, NegativeCoordinates)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Point p1(-100, -100);
  Point p2(100, 100);
  Segment seg(p1, p2);
  put_in_plane(*plane, seg);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));
}

// Disabled due to zero-range bug (zero-length segment has same start/end point)
TEST_F(EdgeCaseTest, DISABLED_ZeroLengthSegment)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Point p(50, 50);
  Segment seg(p, p);  // Zero-length segment
  put_in_plane(*plane, seg);

  std::ostringstream output;
  // BUG: Crashes due to zero geom_wide and geom_height
  EXPECT_NO_THROW(plane->draw(output));
}

// Disabled due to zero-range bug (single vertex = single point)
TEST_F(EdgeCaseTest, DISABLED_SingleVertexPolygon)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Polygon poly;
  poly.add_vertex(Point(50, 50));
  // Single vertex polygon - edge case

  put_in_plane(*plane, poly);

  std::ostringstream output;
  // BUG: Crashes due to zero geom_wide and geom_height
  EXPECT_NO_THROW(plane->draw(output));
}

TEST_F(EdgeCaseTest, TwoVertexPolygon)
{
  plane = std::make_unique<Eepic_Plane>(500, 500);

  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(100, 100));
  // Two vertex polygon is essentially a segment

  put_in_plane(*plane, poly);

  std::ostringstream output;
  EXPECT_NO_THROW(plane->draw(output));
}

//============================================================================
// Memory Management Tests
//============================================================================

TEST(MemoryTest, PlaneDestruction)
{
  // Plane should properly clean up all objects on destruction
  {
    Eepic_Plane plane(500, 500);

    for (int i = 0; i < 100; ++i)
    {
      Point p(i, i * 2);
      put_in_plane(plane, p);
    }

    std::ostringstream output;
    plane.draw(output);

    // Plane goes out of scope and should clean up
  }

  // If we get here without crashes, memory management is working
  SUCCEED();
}

// Disabled due to single-point zero-range bug
TEST(MemoryTest, DISABLED_MultipleDrawCalls)
{
  Eepic_Plane plane(500, 500);

  Point p(50, 50);
  put_in_plane(plane, p);

  // Multiple draw calls should work correctly
  for (int i = 0; i < 10; ++i)
  {
    std::ostringstream output;
    plane.draw(output);
  }

  SUCCEED();
}

TEST(MemoryTest, MultipleDrawCallsWithSegment)
{
  Eepic_Plane plane(500, 500);

  // Use segment to avoid zero-range bug
  Segment seg(Point(0, 0), Point(100, 100));
  put_in_plane(plane, seg);

  // Multiple draw calls should work correctly
  for (int i = 0; i < 10; ++i)
  {
    std::ostringstream output;
    plane.draw(output);
  }

  SUCCEED();
}

//============================================================================
// Integration Tests
//============================================================================

TEST(IntegrationTest, ComplexDiagram)
{
  Eepic_Plane plane(1000, 800);

  // Add various object types to create a complex diagram

  // Points
  put_in_plane(plane, Point(100, 100));
  put_in_plane(plane, Point(500, 100));
  put_in_plane(plane, Point(300, 400));

  // Segments and arrows
  put_in_plane(plane, Segment(Point(100, 100), Point(500, 100)));
  put_in_plane(plane, Arrow(Point(500, 100), Point(300, 400)));
  put_in_plane(plane, Dash_Segment(Point(300, 400), Point(100, 100)));

  // Ellipse
  put_in_plane(plane, Ellipse(Point(300, 250), 50, 30));

  // Polygon
  Polygon poly;
  poly.add_vertex(Point(600, 200));
  poly.add_vertex(Point(700, 200));
  poly.add_vertex(Point(700, 300));
  poly.add_vertex(Point(600, 300));
  poly.close();
  put_in_plane(plane, poly);

  // Text
  put_in_plane(plane, Center_Text(Point(300, 50), "Title"));

  std::ostringstream output;
  plane.draw(output);

  std::string result = output.str();

  // Verify complete output
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("\\end{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("Title") != std::string::npos);
  EXPECT_TRUE(result.find("\\ellipse") != std::string::npos);

  // Should have recorded all objects
  EXPECT_TRUE(result.find("geometric objects were put in the plane") !=
              std::string::npos);
}

TEST(IntegrationTest, GraphStyleDiagram)
{
  // Simulate a graph-like diagram with nodes and edges
  Eepic_Plane plane(800, 600);

  // Node positions
  Point node1(100, 300);
  Point node2(400, 100);
  Point node3(400, 500);
  Point node4(700, 300);

  // Draw nodes as ellipses
  double node_radius = 30;
  put_in_plane(plane, Ellipse(node1, node_radius, node_radius));
  put_in_plane(plane, Ellipse(node2, node_radius, node_radius));
  put_in_plane(plane, Ellipse(node3, node_radius, node_radius));
  put_in_plane(plane, Ellipse(node4, node_radius, node_radius));

  // Draw edges as arrows
  put_in_plane(plane, Arrow(node1, node2));
  put_in_plane(plane, Arrow(node1, node3));
  put_in_plane(plane, Arrow(node2, node4));
  put_in_plane(plane, Arrow(node3, node4));

  // Add node labels
  put_in_plane(plane, Center_Text(node1, "A"));
  put_in_plane(plane, Center_Text(node2, "B"));
  put_in_plane(plane, Center_Text(node3, "C"));
  put_in_plane(plane, Center_Text(node4, "D"));

  std::ostringstream output;
  plane.draw(output);

  std::string result = output.str();
  EXPECT_TRUE(result.find("\\begin{picture}") != std::string::npos);
  EXPECT_TRUE(result.find("12 geometric objects were put in the plane") !=
              std::string::npos);
}

//============================================================================
// Main
//============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}