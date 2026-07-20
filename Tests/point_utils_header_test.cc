#include <point_utils.H>

#include <gtest/gtest.h>

TEST(PointUtilsHeaderTest, IsSelfContained)
{
  const Aleph::Point a(0, 0);
  const Aleph::Point b(1, 0);
  const Aleph::Point c(0, 1);

  EXPECT_GT(Aleph::area_of_triangle(a, b, c), 0);
  EXPECT_EQ(Aleph::orientation(a, b, c), Aleph::Orientation::CCW);
}
