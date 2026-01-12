/* Tests for tpl_mincost.H - Advanced minimum cost flow algorithms
 *
 * Tests cover:
 * - Assignment problem solver
 * - Transportation problem solver
 */

#include <gtest/gtest.h>
#include <tpl_mincost.H>

using namespace Aleph;


//==============================================================================
// Assignment Problem Tests
//==============================================================================

TEST(AssignmentTest, Basic3x3)
{
  std::vector<std::vector<double>> costs = {
    {10, 5, 13},
    {3, 9, 18},
    {10, 6, 12}
  };

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 3);
  EXPECT_DOUBLE_EQ(result.total_cost, 20.0);
}

TEST(AssignmentTest, Single1x1)
{
  std::vector<std::vector<double>> costs = {{42}};

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 1);
  EXPECT_DOUBLE_EQ(result.total_cost, 42.0);
}

TEST(AssignmentTest, Empty)
{
  std::vector<std::vector<double>> costs;

  auto result = solve_assignment(costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.assignments.size(), 0);
}


//==============================================================================
// Transportation Problem Tests
//==============================================================================

TEST(TransportationTest, Balanced)
{
  std::vector<double> supplies = {100, 100};
  std::vector<double> demands = {100, 100};
  std::vector<std::vector<double>> costs = {
    {1, 2},
    {3, 4}
  };

  auto result = solve_transportation(supplies, demands, costs);

  EXPECT_TRUE(result.feasible);
  EXPECT_EQ(result.shipments.size(), 2);
}

TEST(TransportationTest, Imbalanced)
{
  std::vector<double> supplies = {100, 100};
  std::vector<double> demands = {50, 50};

  std::vector<std::vector<double>> costs = {
    {1, 2},
    {3, 4}
  };

  auto result = solve_transportation(supplies, demands, costs);

  EXPECT_FALSE(result.feasible);
}


int main(int argc, char** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
