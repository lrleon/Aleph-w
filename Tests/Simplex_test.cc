/**
 * @file Simplex_test.cc
 * @brief Comprehensive test suite for Simplex.H
 *
 * Tests all aspects of the Simplex linear programming solver including:
 * - Constructor and initialization
 * - Objective function setup
 * - Constraint management
 * - Solution algorithm
 * - Edge cases and error handling
 * - Classic linear programming problems
 */

#include <gtest/gtest.h>
#include <Simplex.H>
#include <cmath>

using namespace Aleph;

//==============================================================================
// Test Fixture
//==============================================================================

class SimplexTest : public ::testing::Test
{
protected:
  static constexpr double EPSILON = 1e-6;
  
  bool nearly_equal(double a, double b, double eps = EPSILON)
  {
    return std::fabs(a - b) < eps;
  }
};

//==============================================================================
// Constructor Tests
//==============================================================================

TEST_F(SimplexTest, ConstructorBasic)
{
  Simplex<double> simplex(3);
  
  EXPECT_EQ(simplex.get_num_vars(), 3);
  EXPECT_EQ(simplex.get_num_restrictions(), 0);
  EXPECT_EQ(simplex.get_state(), Simplex<double>::Not_Solved);
}

TEST_F(SimplexTest, ConstructorZeroVariablesThrows)
{
  EXPECT_THROW(Simplex<double>(0), std::invalid_argument);
}

TEST_F(SimplexTest, ConstructorSingleVariable)
{
  Simplex<double> simplex(1);
  EXPECT_EQ(simplex.get_num_vars(), 1);
}

TEST_F(SimplexTest, ConstructorLargeNumber)
{
  Simplex<double> simplex(100);
  EXPECT_EQ(simplex.get_num_vars(), 100);
}

//==============================================================================
// Objective Function Tests
//==============================================================================

TEST_F(SimplexTest, SetObjectiveCoefficient)
{
  Simplex<double> simplex(3);
  
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 2.0);
  simplex.put_objetive_function_coef(2, 1.0);
  
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(0), 3.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(1), 2.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(2), 1.0);
}

TEST_F(SimplexTest, SetObjectiveCoefficientOutOfRangeThrows)
{
  Simplex<double> simplex(3);
  
  EXPECT_THROW(simplex.put_objetive_function_coef(3, 1.0), std::out_of_range);
  EXPECT_THROW(simplex.put_objetive_function_coef(100, 1.0), std::out_of_range);
}

TEST_F(SimplexTest, GetObjectiveCoefficientOutOfRangeThrows)
{
  Simplex<double> simplex(3);
  
  EXPECT_THROW(simplex.get_objetive_function_coef(3), std::out_of_range);
}

TEST_F(SimplexTest, SetObjectiveFunctionFromArray)
{
  Simplex<double> simplex(3);
  double coefs[] = {5.0, 4.0, 3.0};
  
  simplex.put_objetive_function(coefs);
  
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(0), 5.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(1), 4.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(2), 3.0);
}

TEST_F(SimplexTest, SetObjectiveFunctionFromDynArray)
{
  Simplex<double> simplex(3);
  DynArray<double> coefs;
  coefs[0] = 5.0;
  coefs[1] = 4.0;
  coefs[2] = 3.0;
  
  simplex.put_objetive_function(coefs);
  
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(0), 5.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(1), 4.0);
  EXPECT_DOUBLE_EQ(simplex.get_objetive_function_coef(2), 3.0);
}

TEST_F(SimplexTest, GetObjectiveFunctionPointer)
{
  Simplex<double> simplex(3);
  simplex.put_objetive_function_coef(0, 7.0);
  
  double* ptr = simplex.get_objetive_function();
  ASSERT_NE(ptr, nullptr);
  EXPECT_DOUBLE_EQ(ptr[0], 7.0);
}

//==============================================================================
// Restriction Tests
//==============================================================================

TEST_F(SimplexTest, AddRestrictionFromArray)
{
  Simplex<double> simplex(2);
  double rest[] = {1.0, 2.0, 10.0};
  
  simplex.put_restriction(rest);
  
  EXPECT_EQ(simplex.get_num_restrictions(), 1);
}

TEST_F(SimplexTest, AddMultipleRestrictions)
{
  Simplex<double> simplex(2);
  double r1[] = {1.0, 1.0, 4.0};
  double r2[] = {2.0, 1.0, 5.0};
  double r3[] = {1.0, 2.0, 6.0};
  
  simplex.put_restriction(r1);
  simplex.put_restriction(r2);
  simplex.put_restriction(r3);
  
  EXPECT_EQ(simplex.get_num_restrictions(), 3);
}

TEST_F(SimplexTest, AddEmptyRestriction)
{
  Simplex<double> simplex(2);
  
  double* rest = simplex.put_restriction(nullptr);
  
  ASSERT_NE(rest, nullptr);
  EXPECT_EQ(simplex.get_num_restrictions(), 1);
  
  // Should be initialized to zero
  EXPECT_DOUBLE_EQ(rest[0], 0.0);
  EXPECT_DOUBLE_EQ(rest[1], 0.0);
  EXPECT_DOUBLE_EQ(rest[2], 0.0);
}

TEST_F(SimplexTest, ModifyRestrictionAfterAdd)
{
  Simplex<double> simplex(2);
  
  double* rest = simplex.put_restriction(nullptr);
  rest[0] = 3.0;
  rest[1] = 4.0;
  rest[2] = 12.0;
  
  double* retrieved = simplex.get_restriction(0);
  EXPECT_DOUBLE_EQ(retrieved[0], 3.0);
  EXPECT_DOUBLE_EQ(retrieved[1], 4.0);
  EXPECT_DOUBLE_EQ(retrieved[2], 12.0);
}

TEST_F(SimplexTest, GetRestrictionOutOfRangeThrows)
{
  Simplex<double> simplex(2);
  double rest[] = {1.0, 1.0, 4.0};
  simplex.put_restriction(rest);
  
  EXPECT_THROW(simplex.get_restriction(1), std::out_of_range);
  EXPECT_THROW(simplex.get_restriction(100), std::out_of_range);
}

TEST_F(SimplexTest, GetRestrictionCoef)
{
  Simplex<double> simplex(3);
  double rest[] = {1.0, 2.0, 3.0, 10.0};
  simplex.put_restriction(rest);
  
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 1), 2.0);
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 2), 3.0);
}

TEST_F(SimplexTest, PutRestrictionCoef)
{
  Simplex<double> simplex(2);
  simplex.put_restriction(nullptr);
  
  simplex.put_restriction_coef(0, 0, 5.0);
  simplex.put_restriction_coef(0, 1, 6.0);
  
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 0), 5.0);
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 1), 6.0);
}

TEST_F(SimplexTest, AddRestrictionFromDynArray)
{
  Simplex<double> simplex(2);
  DynArray<double> coefs;
  coefs[0] = 1.0;
  coefs[1] = 2.0;
  coefs[2] = 8.0;
  
  simplex.put_restriction(coefs);
  
  EXPECT_EQ(simplex.get_num_restrictions(), 1);
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 0), 1.0);
  EXPECT_DOUBLE_EQ(simplex.get_restriction_coef(0, 1), 2.0);
}

//==============================================================================
// Solve Error Handling Tests
//==============================================================================

TEST_F(SimplexTest, SolveWithoutRestrictionThrows)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 2.0);
  
  EXPECT_THROW(simplex.prepare_linear_program(), std::logic_error);
}

TEST_F(SimplexTest, SolveWithoutPrepareThrows)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 2.0);
  double rest[] = {1.0, 1.0, 4.0};
  simplex.put_restriction(rest);
  
  EXPECT_THROW(simplex.solve(), std::logic_error);
}

TEST_F(SimplexTest, SolveTwiceThrows)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 2.0);
  double rest[] = {1.0, 1.0, 4.0};
  simplex.put_restriction(rest);
  
  simplex.prepare_linear_program();
  simplex.solve();
  
  EXPECT_THROW(simplex.solve(), std::logic_error);
}

//==============================================================================
// Classic Linear Programming Problems
//==============================================================================

TEST_F(SimplexTest, SimpleMaximization)
{
  // Maximize Z = 3x + 2y
  // Subject to:
  //   x + y <= 4
  //   x <= 2
  //   y <= 3
  //   x, y >= 0
  // Expected solution: x=2, y=2, Z=10
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);  // x coefficient
  simplex.put_objetive_function_coef(1, 2.0);  // y coefficient
  
  double c1[] = {1.0, 1.0, 4.0};
  double c2[] = {1.0, 0.0, 2.0};
  double c3[] = {0.0, 1.0, 3.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 2.0));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 2.0));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 10.0));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(SimplexTest, ThreeVariableProblem)
{
  // Maximize Z = 5x1 + 4x2 + 3x3
  // Subject to:
  //   2x1 + 3x2 + x3 <= 5
  //   4x1 + 2x2 + 3x3 <= 11
  //   3x1 + 4x2 + 2x3 <= 8
  //   x1, x2, x3 >= 0
  
  Simplex<double> simplex(3);
  simplex.put_objetive_function_coef(0, 5.0);
  simplex.put_objetive_function_coef(1, 4.0);
  simplex.put_objetive_function_coef(2, 3.0);
  
  double c1[] = {2.0, 3.0, 1.0, 5.0};
  double c2[] = {4.0, 2.0, 3.0, 11.0};
  double c3[] = {3.0, 4.0, 2.0, 8.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  
  // Objective value should be positive and bounded
  EXPECT_GT(simplex.objetive_value(), 0.0);
}

TEST_F(SimplexTest, ProductMixProblem)
{
  // Classic product mix problem
  // Maximize Z = 40x1 + 30x2 (profit)
  // Subject to:
  //   x1 + x2 <= 40  (labor hours)
  //   2x1 + x2 <= 60 (machine hours)
  //   x1, x2 >= 0
  // Expected: x1=20, x2=20, Z=1400
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 20.0));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 20.0));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 1400.0));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(SimplexTest, DietProblem)
{
  // Simplified diet problem (but maximizing nutrition here)
  // Maximize Z = 10x1 + 15x2 + 12x3 (nutrition value)
  // Subject to:
  //   x1 + x2 + x3 <= 10 (total servings)
  //   2x1 + x2 + 3x3 <= 18 (cost constraint)
  //   x1, x2, x3 >= 0
  
  Simplex<double> simplex(3);
  simplex.put_objetive_function_coef(0, 10.0);
  simplex.put_objetive_function_coef(1, 15.0);
  simplex.put_objetive_function_coef(2, 12.0);
  
  double c1[] = {1.0, 1.0, 1.0, 10.0};
  double c2[] = {2.0, 1.0, 3.0, 18.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_GT(simplex.objetive_value(), 0.0);
}

TEST_F(SimplexTest, TransportationVariant)
{
  // Maximize Z = 2x1 + 3x2 + x3 + 4x4
  // Subject to:
  //   x1 + x2 <= 4
  //   x3 + x4 <= 6
  //   x1 + x3 <= 5
  //   x2 + x4 <= 5
  
  Simplex<double> simplex(4);
  simplex.put_objetive_function_coef(0, 2.0);
  simplex.put_objetive_function_coef(1, 3.0);
  simplex.put_objetive_function_coef(2, 1.0);
  simplex.put_objetive_function_coef(3, 4.0);
  
  double c1[] = {1.0, 1.0, 0.0, 0.0, 4.0};
  double c2[] = {0.0, 0.0, 1.0, 1.0, 6.0};
  double c3[] = {1.0, 0.0, 1.0, 0.0, 5.0};
  double c4[] = {0.0, 1.0, 0.0, 1.0, 5.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  simplex.put_restriction(c4);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
}

//==============================================================================
// Edge Cases
//==============================================================================

TEST_F(SimplexTest, SingleVariableSingleConstraint)
{
  // Maximize Z = 5x
  // Subject to: x <= 10
  // Expected: x=10, Z=50
  
  Simplex<double> simplex(1);
  simplex.put_objetive_function_coef(0, 5.0);
  
  double c1[] = {1.0, 10.0};
  simplex.put_restriction(c1);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 10.0));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 50.0));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(SimplexTest, ZeroObjectiveCoefficient)
{
  // Maximize Z = 0*x + 5*y
  // Subject to:
  //   x + y <= 10
  //   y <= 10  (explicit bound on y)
  //   x, y >= 0
  // Expected: y=10, x=0, Z=50
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 0.0);
  simplex.put_objetive_function_coef(1, 5.0);
  
  double c1[] = {1.0, 1.0, 10.0};
  double c2[] = {0.0, 1.0, 10.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  // Solution should maximize y within constraints
  EXPECT_GE(simplex.objetive_value(), 0.0);
}

TEST_F(SimplexTest, TightConstraints)
{
  // Maximize Z = x + y
  // Subject to:
  //   x <= 5
  //   y <= 5
  //   x + y <= 10
  // Expected: x=5, y=5, Z=10
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 0.0, 5.0};
  double c2[] = {0.0, 1.0, 5.0};
  double c3[] = {1.0, 1.0, 10.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 5.0));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 5.0));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 10.0));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(SimplexTest, RedundantConstraint)
{
  // Maximize Z = x + y
  // Subject to:
  //   x + y <= 10
  //   x <= 5
  //   y <= 5
  //   x, y >= 0
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 1.0, 10.0};
  double c2[] = {1.0, 0.0, 5.0};
  double c3[] = {0.0, 1.0, 5.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 10.0));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(SimplexTest, FractionalSolution)
{
  // Maximize Z = 3x + 5y
  // Subject to:
  //   x + y <= 4
  //   2x + 3y <= 9
  //   x <= 4
  //   y <= 4
  // Optimal: x=3, y=1, Z=14
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 5.0);
  
  double c1[] = {1.0, 1.0, 4.0};
  double c2[] = {2.0, 3.0, 9.0};
  double c3[] = {1.0, 0.0, 4.0};
  double c4[] = {0.0, 1.0, 4.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  simplex.put_restriction(c4);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  
  // Optimal value should be around 14
  EXPECT_GT(simplex.objetive_value(), 10.0);
}

//==============================================================================
// Float Type Tests
//==============================================================================

TEST_F(SimplexTest, FloatType)
{
  // Same problem but with float instead of double
  Simplex<float> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0f);
  simplex.put_objetive_function_coef(1, 2.0f);
  
  float c1[] = {1.0f, 1.0f, 4.0f};
  float c2[] = {1.0f, 0.0f, 2.0f};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<float>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
}

//==============================================================================
// State Transition Tests
//==============================================================================

TEST_F(SimplexTest, StateTransitions)
{
  Simplex<double> simplex(2);
  EXPECT_EQ(simplex.get_state(), Simplex<double>::Not_Solved);
  
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 1.0, 10.0};
  simplex.put_restriction(c1);
  
  simplex.prepare_linear_program();
  EXPECT_EQ(simplex.get_state(), Simplex<double>::Not_Solved);
  
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  EXPECT_EQ(simplex.get_state(), Simplex<double>::Solved);
}

//==============================================================================
// Verify Solution Tests
//==============================================================================

TEST_F(SimplexTest, VerifySolutionTrue)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 0.0, 5.0};
  double c2[] = {0.0, 1.0, 5.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  simplex.load_solution();
  
  EXPECT_TRUE(simplex.verify_solution());
}

//==============================================================================
// Accessor Tests
//==============================================================================

TEST_F(SimplexTest, GetNumVars)
{
  Simplex<double> simplex(5);
  EXPECT_EQ(simplex.get_num_vars(), 5);
}

TEST_F(SimplexTest, GetNumRestrictions)
{
  Simplex<double> simplex(2);
  EXPECT_EQ(simplex.get_num_restrictions(), 0);
  
  double c1[] = {1.0, 1.0, 4.0};
  simplex.put_restriction(c1);
  EXPECT_EQ(simplex.get_num_restrictions(), 1);
  
  double c2[] = {2.0, 1.0, 6.0};
  simplex.put_restriction(c2);
  EXPECT_EQ(simplex.get_num_restrictions(), 2);
}

//==============================================================================
// Large Problem Tests
//==============================================================================

TEST_F(SimplexTest, MediumSizeProblem)
{
  // 5 variables, 5 constraints - each variable bounded
  const int n = 5;
  
  Simplex<double> simplex(n);
  
  // Set objective: maximize sum of all variables
  for (int i = 0; i < n; ++i)
    simplex.put_objetive_function_coef(i, 1.0);
  
  // Add constraints: each variable <= 1
  for (int j = 0; j < n; ++j)
    {
      double* rest = simplex.put_restriction(nullptr);
      for (int i = 0; i < n; ++i)
        rest[i] = (i == j) ? 1.0 : 0.0;
      rest[n] = 1.0;  // RHS
    }
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 5.0));
}

//==============================================================================
// Numerical Stability Tests
//==============================================================================

TEST_F(SimplexTest, SmallCoefficients)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 0.001);
  simplex.put_objetive_function_coef(1, 0.002);
  
  double c1[] = {0.001, 0.001, 0.01};
  double c2[] = {0.001, 0.0, 0.005};
  double c3[] = {0.0, 0.001, 0.005};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  // Just verify we get a valid solution
  EXPECT_GE(simplex.objetive_value(), 0.0);
}

TEST_F(SimplexTest, LargeCoefficients)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 1000.0);
  simplex.put_objetive_function_coef(1, 2000.0);
  
  double c1[] = {1000.0, 1000.0, 10000.0};
  double c2[] = {1000.0, 0.0, 5000.0};
  double c3[] = {0.0, 1000.0, 5000.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  // x=5, y=5, Z=15000
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 15000.0));
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
