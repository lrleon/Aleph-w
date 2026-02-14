#include <gtest/gtest.h>

#include <cctype>
#include <sstream>
#include <string>

#include <tikzgeom.H>

using namespace Aleph;

namespace
{

bool has_nan_or_inf(std::string s)
{
  for (char & c : s)
    c = static_cast<char>(::tolower(static_cast<unsigned char>(c)));

  return s.find("nan") != std::string::npos or
         s.find("inf") != std::string::npos;
}

} // namespace

TEST(TikzPlaneTest, ConstructionAndAccessors)
{
  Tikz_Plane plane(120.0, 80.0, 5.0, 10.0);

  EXPECT_DOUBLE_EQ(plane.get_wide(), 120.0);
  EXPECT_DOUBLE_EQ(plane.get_height(), 80.0);
  EXPECT_DOUBLE_EQ(plane.get_xoffset(), 5.0);
  EXPECT_DOUBLE_EQ(plane.get_yoffset(), 10.0);
  EXPECT_EQ(plane.size(), 0U);
}

TEST(TikzPlaneTest, DrawEmptyPlane)
{
  Tikz_Plane plane(100, 60);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("\\begin{tikzpicture}"), std::string::npos);
  EXPECT_NE(result.find("\\clip (0,0) rectangle (100.000000,60.000000);"), std::string::npos);
  EXPECT_NE(result.find("\\end{tikzpicture}"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzPlaneTest, SinglePointDoesNotCrash)
{
  Tikz_Plane plane(100, 60);
  put_in_plane(plane, Point(10, 20));

  std::ostringstream output;
  EXPECT_NO_THROW(plane.draw(output));

  const std::string result = output.str();
  EXPECT_NE(result.find("\\fill"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzPlaneTest, DegenerateRangesVerticalAndHorizontalSegments)
{
  {
    Tikz_Plane plane(100, 60);
    put_in_plane(plane, Segment(Point(10, 0), Point(10, 100))); // x-range = 0

    std::ostringstream output;
    EXPECT_NO_THROW(plane.draw(output));
    const std::string result = output.str();
    EXPECT_NE(result.find("\\draw"), std::string::npos);
    EXPECT_FALSE(has_nan_or_inf(result));
  }

  {
    Tikz_Plane plane(100, 60);
    put_in_plane(plane, Segment(Point(0, 20), Point(100, 20))); // y-range = 0

    std::ostringstream output;
    EXPECT_NO_THROW(plane.draw(output));
    const std::string result = output.str();
    EXPECT_NE(result.find("\\draw"), std::string::npos);
    EXPECT_FALSE(has_nan_or_inf(result));
  }
}

TEST(TikzPlaneTest, SupportsCoreGeometryObjects)
{
  Tikz_Plane plane(140, 90);

  put_in_plane(plane, Point(0, 0));
  put_in_plane(plane, Segment(Point(0, 0), Point(100, 50)));
  put_in_plane(plane, Triangle(Point(0, 0), Point(60, 0), Point(30, 45)));
  put_in_plane(plane, Ellipse(Point(20, 10), 12, 6));

  Polygon poly;
  poly.add_vertex(Point(0, 0));
  poly.add_vertex(Point(40, 0));
  poly.add_vertex(Point(20, 20));
  poly.close();
  put_in_plane(plane, poly);

  Regular_Polygon reg(Point(50, 30), 15.0, 5);
  put_in_plane(plane, reg);

  put_in_plane(plane, Text(Point(10, 10), "Hello"));

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("ellipse [x radius="), std::string::npos);
  EXPECT_NE(result.find("\\node"), std::string::npos);
  EXPECT_NE(result.find("-- cycle;"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzPlaneTest, SupportsPolarPointAndRotatedEllipse)
{
  Tikz_Plane plane(140, 90);

  const Geom_Number pi_over_four = 0.7853981633974483096;
  put_in_plane(plane, Polar_Point(15, pi_over_four));

  RotatedEllipse rotated(Point(20, 20), 12, 5, 0.7071067811865475244, 0.7071067811865475244);
  put_in_plane(plane, rotated);

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("\\fill"), std::string::npos);
  EXPECT_NE(result.find("-- cycle;"), std::string::npos);
  EXPECT_FALSE(has_nan_or_inf(result));
}

TEST(TikzPlaneTest, OptionalCartesianAxis)
{
  Tikz_Plane plane(120, 80);
  plane.put_cartesian_axis();
  put_in_plane(plane, Segment(Point(-10, -10), Point(10, 10)));

  std::ostringstream output;
  plane.draw(output);
  const std::string result = output.str();

  EXPECT_NE(result.find("\\draw["), std::string::npos);
  EXPECT_NE(result.find("draw=gray"), std::string::npos);
  EXPECT_NE(result.find("->"), std::string::npos);
}
