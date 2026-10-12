#include <point_utils.H>

#include <gtest/gtest.h>

TEST(PointUtilsHeaderTest, IsSelfContained)
{
  const Aleph::Point a(0, 0);
  const Aleph::Point b(1, 0);
  const Aleph::Point c(0, 1);

  // Checked as a bool: EXPECT_GT would print the Geom_Number (mpq_class)
  // on failure through the compiled operator<< of libgmpxx, which does not
  // link on Windows. The message prints it as a double instead.
  const Aleph::Geom_Number area = Aleph::area_of_triangle(a, b, c);
  EXPECT_TRUE(area > 0) << "area = " << area.get_d();
  EXPECT_EQ(Aleph::orientation(a, b, c), Aleph::Orientation::CCW);
}
