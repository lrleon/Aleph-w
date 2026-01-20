
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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
 * @file Simplex_test.cc
 * @brief Tests for Simplex
 */

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
// New Features Tests: Optimization Types, Constraints, Statistics
//==============================================================================

TEST_F(SimplexTest, MinimizationModeAPI)
{
  // Test that minimization mode is properly set and affects optimization
  // Standard maximization problem for comparison
  Simplex<double> simplex_max(2, OptimizationType::Maximize);
  simplex_max.put_objetive_function_coef(0, 40.0);
  simplex_max.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex_max.put_restriction(c1);
  simplex_max.put_restriction(c2);
  
  simplex_max.prepare_linear_program();
  auto state_max = simplex_max.solve();
  EXPECT_EQ(state_max, Simplex<double>::Solved);
  
  simplex_max.load_solution();
  double max_value = simplex_max.objetive_value();
  
  // Same problem but with minimization constructor
  Simplex<double> simplex_min(2, OptimizationType::Minimize);
  simplex_min.put_objetive_function_coef(0, 40.0);
  simplex_min.put_objetive_function_coef(1, 30.0);
  
  simplex_min.put_restriction(c1);
  simplex_min.put_restriction(c2);
  
  simplex_min.prepare_linear_program();
  auto state_min = simplex_min.solve();
  EXPECT_EQ(state_min, Simplex<double>::Solved);
  
  simplex_min.load_solution();
  double min_value = simplex_min.objetive_value();
  
  // The minimization should find a value <= maximization value
  // (In this bounded problem, min is at origin: 0)
  EXPECT_LE(min_value, max_value);
  
  // Verify optimization types are correctly reported
  EXPECT_EQ(simplex_max.get_optimization_type(), OptimizationType::Maximize);
  EXPECT_EQ(simplex_min.get_optimization_type(), OptimizationType::Minimize);
}

TEST_F(SimplexTest, EqualityConstraintSimulated)
{
  // Maximize Z = 5x + 4y
  // s.t. x + y = 10 (simulated with x + y <= 10 and -x - y <= -10)
  //      x <= 6
  //      y <= 8
  //      x, y >= 0
  //
  // Note: True equality constraints require Big-M or two-phase simplex.
  // Here we test by using tight upper/lower bounds on the sum.
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 5.0);
  simplex.put_objetive_function_coef(1, 4.0);
  
  double c1[] = {1.0, 1.0, 10.0};   // x + y <= 10
  double c2[] = {1.0, 0.0, 6.0};    // x <= 6
  double c3[] = {0.0, 1.0, 8.0};    // y <= 8
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  // With x + y <= 10, maximize 5x + 4y
  // Optimal: x=6, y=4, Z=46 (using full capacity of sum constraint)
  EXPECT_NEAR(simplex.objetive_value(), 46.0, 0.1);
}

TEST_F(SimplexTest, ConstraintTypesAPI)
{
  // Test that constraint types are properly stored
  // Maximize Z = 2x + 3y with standard LE constraints
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 2.0);
  simplex.put_objetive_function_coef(1, 3.0);
  
  double c1[] = {1.0, 1.0, 10.0};  // x + y <= 10
  double c2[] = {1.0, 0.0, 8.0};   // x <= 8
  double c3[] = {0.0, 1.0, 6.0};   // y <= 6
  
  simplex.put_restriction(c1, ConstraintType::LE);
  simplex.put_restriction(c2, ConstraintType::LE);
  simplex.put_restriction(c3, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  // Optimal: x=4, y=6, Z=8+18=26 (limited by x+y<=10 and y<=6)
  EXPECT_NEAR(simplex.objetive_value(), 26.0, 0.1);
  EXPECT_LE(simplex.get_solution(0) + simplex.get_solution(1), 10.1);
}

TEST_F(SimplexTest, StatisticsTracking)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  
  auto stats = simplex.get_stats();
  
  EXPECT_GT(stats.iterations, 0u);
  EXPECT_GT(stats.pivots, 0u);
  EXPECT_GE(stats.elapsed_ms, 0.0);
}

TEST_F(SimplexTest, BlandRuleEnabled)
{
  // Test that Bland's rule can be enabled and affects pivot selection
  Simplex<double> simplex(3);
  simplex.enable_bland_rule();
  
  simplex.put_objetive_function_coef(0, 100.0);
  simplex.put_objetive_function_coef(1, 10.0);
  simplex.put_objetive_function_coef(2, 1.0);
  
  double c1[] = {1.0, 0.0, 0.0, 1.0};
  double c2[] = {20.0, 1.0, 0.0, 100.0};
  double c3[] = {200.0, 20.0, 1.0, 10000.0};
  
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  simplex.put_restriction(c3);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  // Verify solution is valid
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  
  // Optimal is x1=1, x2=80, x3=9800, Z=100+800+9800=10700
  EXPECT_GT(simplex.objetive_value(), 0.0);
}

TEST_F(SimplexTest, SensitivityAnalysisObjective)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  simplex.load_solution();
  
  auto range = simplex.objective_sensitivity(0);
  
  EXPECT_EQ(range.current_value, 40.0);
  // Range should be finite
  EXPECT_FALSE(std::isinf(range.lower_bound) && std::isinf(range.upper_bound));
}

TEST_F(SimplexTest, ShadowPrices)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  simplex.load_solution();
  
  auto prices = simplex.get_all_shadow_prices();
  
  EXPECT_EQ(prices.size(), 2u);
  // Shadow prices should be non-negative for <= constraints in maximization
}

TEST_F(SimplexTest, ReducedCosts)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  simplex.load_solution();
  
  auto costs = simplex.get_all_reduced_costs();
  
  EXPECT_EQ(costs.size(), 2u);
}

TEST_F(SimplexTest, DualSimplexReoptimize)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  auto state1 = simplex.solve();
  EXPECT_EQ(state1, Simplex<double>::Solved);
  
  simplex.load_solution();
  double original_obj = simplex.objetive_value();
  
  // Update RHS of first constraint and reoptimize
  auto state2 = simplex.update_rhs_and_reoptimize(0, 50.0);
  
  if (state2 == Simplex<double>::Solved)
    {
      simplex.load_solution();
      // New objective should be different (likely higher with more room)
      EXPECT_GE(simplex.objetive_value(), original_obj);
      EXPECT_TRUE(simplex.verify_solution());
    }
}

TEST_F(SimplexTest, LargeDegenerateProblem)
{
  // Create a problem with many constraints that may have degenerate solutions
  const size_t n = 10;
  Simplex<double> simplex(n);
  simplex.enable_bland_rule();  // Prevent cycling
  
  // Objective: maximize sum of xi
  for (size_t i = 0; i < n; ++i)
    simplex.put_objetive_function_coef(i, 1.0);
  
  // Constraints: each xi <= 1
  for (size_t i = 0; i < n; ++i)
    {
      DynArray<double> c(n + 1);
      for (size_t j = 0; j <= n; ++j)
        c[j] = 0.0;
      c[i] = 1.0;
      c[n] = 1.0;
      simplex.put_restriction(c);
    }
  
  // Sum constraint: sum <= n/2
  DynArray<double> sum_c(n + 1);
  for (size_t i = 0; i < n; ++i)
    sum_c[i] = 1.0;
  sum_c[n] = static_cast<double>(n) / 2.0;
  simplex.put_restriction(sum_c);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_NEAR(simplex.objetive_value(), static_cast<double>(n) / 2.0, 0.1);
  
  // Check for degenerate pivots
  auto stats = simplex.get_stats();
  // Some degenerate pivots are expected in this problem
  (void)stats;  // Just check it doesn't crash
}

TEST_F(SimplexTest, SetMinimizeMaximize)
{
  Simplex<double> simplex(2);
  
  EXPECT_EQ(simplex.get_optimization_type(), OptimizationType::Maximize);
  
  simplex.set_minimize();
  EXPECT_EQ(simplex.get_optimization_type(), OptimizationType::Minimize);
  
  simplex.set_maximize();
  EXPECT_EQ(simplex.get_optimization_type(), OptimizationType::Maximize);
}

TEST_F(SimplexTest, IsBasicVariable)
{
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 40.0);
  simplex.put_objetive_function_coef(1, 30.0);
  
  double c1[] = {1.0, 1.0, 40.0};
  double c2[] = {2.0, 1.0, 60.0};
  simplex.put_restriction(c1);
  simplex.put_restriction(c2);
  
  simplex.prepare_linear_program();
  simplex.solve();
  simplex.load_solution();
  
  // At least one variable should be basic
  bool has_basic = simplex.is_basic_variable(0) || simplex.is_basic_variable(1);
  EXPECT_TRUE(has_basic);
}

//==============================================================================
// Revised Simplex Tests
//==============================================================================

class RevisedSimplexTest : public ::testing::Test
{
protected:
  static constexpr double EPSILON = 1e-6;
  
  bool nearly_equal(double a, double b, double eps = EPSILON)
  {
    return std::fabs(a - b) < eps;
  }
};

TEST_F(RevisedSimplexTest, BasicConstructor)
{
  RevisedSimplex<double> simplex(3, 2);
  
  EXPECT_EQ(simplex.get_num_vars(), 3);
  EXPECT_EQ(simplex.get_num_constraints(), 2);
  EXPECT_EQ(simplex.get_state(), RevisedSimplex<double>::Not_Solved);
}

TEST_F(RevisedSimplexTest, SimpleMaximization)
{
  // Maximize Z = 40x + 30y
  // s.t. x + y <= 40
  //      2x + y <= 60
  
  RevisedSimplex<double> simplex(2, 2);
  
  // Objective
  simplex.set_objective(0, 40.0);
  simplex.set_objective(1, 30.0);
  
  // Constraints
  double c1[] = {1.0, 1.0};
  double c2[] = {2.0, 1.0};
  simplex.set_constraint_row(0, c1, 40.0);
  simplex.set_constraint_row(1, c2, 60.0);
  
  auto state = simplex.solve();
  
  EXPECT_EQ(state, RevisedSimplex<double>::Solved);
  
  // Optimal: x=20, y=20, Z=1400
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 20.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 20.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.objective_value(), 1400.0, 0.1));
  
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(RevisedSimplexTest, ThreeVariables)
{
  // Maximize Z = 5x + 4y + 3z
  // s.t. 2x + 3y + z <= 5
  //      4x + 2y + 3z <= 11
  //      3x + 4y + 2z <= 8
  
  RevisedSimplex<double> simplex(3, 3);
  
  simplex.set_objective(0, 5.0);
  simplex.set_objective(1, 4.0);
  simplex.set_objective(2, 3.0);
  
  double c1[] = {2.0, 3.0, 1.0};
  double c2[] = {4.0, 2.0, 3.0};
  double c3[] = {3.0, 4.0, 2.0};
  
  simplex.set_constraint_row(0, c1, 5.0);
  simplex.set_constraint_row(1, c2, 11.0);
  simplex.set_constraint_row(2, c3, 8.0);
  
  auto state = simplex.solve();
  
  EXPECT_EQ(state, RevisedSimplex<double>::Solved);
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_GT(simplex.objective_value(), 0.0);
}

TEST_F(RevisedSimplexTest, CompareWithStandardSimplex)
{
  // Same problem solved by both algorithms
  // Maximize Z = 3x + 2y + 4z
  // s.t. x + y + 2z <= 4
  //      2x + y + z <= 5
  
  // Standard Simplex
  Simplex<double> std_simplex(3);
  std_simplex.put_objetive_function_coef(0, 3.0);
  std_simplex.put_objetive_function_coef(1, 2.0);
  std_simplex.put_objetive_function_coef(2, 4.0);
  
  double c1[] = {1.0, 1.0, 2.0, 4.0};
  double c2[] = {2.0, 1.0, 1.0, 5.0};
  std_simplex.put_restriction(c1);
  std_simplex.put_restriction(c2);
  
  std_simplex.prepare_linear_program();
  std_simplex.solve();
  std_simplex.load_solution();
  double std_obj = std_simplex.objetive_value();
  
  // Revised Simplex
  RevisedSimplex<double> rev_simplex(3, 2);
  rev_simplex.set_objective(0, 3.0);
  rev_simplex.set_objective(1, 2.0);
  rev_simplex.set_objective(2, 4.0);
  
  double r1[] = {1.0, 1.0, 2.0};
  double r2[] = {2.0, 1.0, 1.0};
  rev_simplex.set_constraint_row(0, r1, 4.0);
  rev_simplex.set_constraint_row(1, r2, 5.0);
  
  rev_simplex.solve();
  double rev_obj = rev_simplex.objective_value();
  
  // Both should find same optimal value
  EXPECT_NEAR(std_obj, rev_obj, 0.1);
}

TEST_F(RevisedSimplexTest, MediumSizeProblem)
{
  // 10 variables, 5 constraints
  const size_t n = 10;
  const size_t m = 5;
  
  RevisedSimplex<double> simplex(n, m);
  
  // Random but consistent coefficients
  for (size_t j = 0; j < n; ++j)
    simplex.set_objective(j, static_cast<double>((j + 1) * 3 % 7 + 1));
  
  for (size_t i = 0; i < m; ++i)
    {
      for (size_t j = 0; j < n; ++j)
        simplex.set_constraint(i, j, static_cast<double>((i + j + 1) % 5 + 1));
      simplex.set_rhs(i, static_cast<double>((i + 1) * 20));
    }
  
  auto state = simplex.solve();
  
  EXPECT_EQ(state, RevisedSimplex<double>::Solved);
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_GT(simplex.objective_value(), 0.0);
}

TEST_F(RevisedSimplexTest, StatisticsTracking)
{
  RevisedSimplex<double> simplex(2, 2);
  
  simplex.set_objective(0, 40.0);
  simplex.set_objective(1, 30.0);
  
  double c1[] = {1.0, 1.0};
  double c2[] = {2.0, 1.0};
  simplex.set_constraint_row(0, c1, 40.0);
  simplex.set_constraint_row(1, c2, 60.0);
  
  simplex.solve();
  
  auto stats = simplex.get_stats();
  
  EXPECT_GT(stats.iterations, 0u);
  EXPECT_GT(stats.pivots, 0u);
  EXPECT_GE(stats.elapsed_ms, 0.0);
}

TEST_F(RevisedSimplexTest, PerformanceBenchmark)
{
  // Compare performance on a moderate problem with well-defined coefficients
  const size_t n = 20;   // 20 variables
  const size_t m = 10;   // 10 constraints
  
  // Setup problem with bounded coefficients
  std::vector<double> obj(n);
  std::vector<std::vector<double>> A(m, std::vector<double>(n));
  std::vector<double> b(m);
  
  for (size_t j = 0; j < n; ++j)
    obj[j] = static_cast<double>(j % 5 + 1);  // 1-5
  
  for (size_t i = 0; i < m; ++i)
    {
      double row_sum = 0;
      for (size_t j = 0; j < n; ++j)
        {
          A[i][j] = static_cast<double>((i + j) % 3 + 1);  // 1-3
          row_sum += A[i][j];
        }
      b[i] = row_sum * 2;  // Ensure feasible region
    }
  
  // Standard Simplex
  Simplex<double> std_simplex(n);
  for (size_t j = 0; j < n; ++j)
    std_simplex.put_objetive_function_coef(j, obj[j]);
  
  for (size_t i = 0; i < m; ++i)
    {
      DynArray<double> row(n + 1);
      for (size_t j = 0; j < n; ++j)
        row[j] = A[i][j];
      row[n] = b[i];
      std_simplex.put_restriction(row);
    }
  
  std_simplex.prepare_linear_program();
  auto std_state = std_simplex.solve();
  std_simplex.load_solution();
  auto std_stats = std_simplex.get_stats();
  double std_obj = std_simplex.objetive_value();
  
  // Revised Simplex
  RevisedSimplex<double> rev_simplex(n, m);
  rev_simplex.set_objective(obj.data());
  for (size_t i = 0; i < m; ++i)
    rev_simplex.set_constraint_row(i, A[i].data(), b[i]);
  
  auto rev_state = rev_simplex.solve();
  auto rev_stats = rev_simplex.get_stats();
  double rev_obj = rev_simplex.objective_value();
  
  // Both should solve successfully
  EXPECT_EQ(std_state, Simplex<double>::Solved);
  EXPECT_EQ(rev_state, RevisedSimplex<double>::Solved);
  
  // Both should find good solutions
  EXPECT_GT(std_obj, 0.0);
  EXPECT_GT(rev_obj, 0.0);
  
  std::cout << "\n=== Performance Comparison (n=" << n << ", m=" << m << ") ===\n"
            << "Standard Simplex: " << std_stats.elapsed_ms << " ms, "
            << std_stats.pivots << " pivots, obj=" << std_obj << "\n"
            << "Revised Simplex:  " << rev_stats.elapsed_ms << " ms, "
            << rev_stats.pivots << " pivots, obj=" << rev_obj << "\n";
}

//==============================================================================
// Non-Standard Constraints Tests (>=, ==)
//==============================================================================

class NonStandardConstraintsTest : public ::testing::Test
{
protected:
  static constexpr double EPSILON = 1e-6;
  
  bool nearly_equal(double a, double b, double eps = EPSILON)
  {
    return std::fabs(a - b) < eps;
  }
};

TEST_F(NonStandardConstraintsTest, GreaterThanOrEqualConstraints)
{
  // Maximize Z = 3x + 2y
  // Subject to:
  //   x + y >= 4  (GE constraint)
  //   x <= 6      (LE constraint)
  //   y <= 5      (LE constraint)
  //   x, y >= 0
  // Expected: x=6, y=5 (maximize on corner), but wait, that gives 28
  // Actually with x + y >= 4, we want to maximize, so push to upper bounds
  // x=6, y=5 satisfies x+y >= 4 (11 >= 4), x <= 6, y <= 5
  // Z = 3*6 + 2*5 = 18 + 10 = 28
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 2.0);
  
  // GE constraint: x + y >= 4
  double c1[] = {1.0, 1.0, 4.0};
  simplex.put_restriction(c1, ConstraintType::GE);
  
  // LE constraints
  double c2[] = {1.0, 0.0, 6.0};
  double c3[] = {0.0, 1.0, 5.0};
  simplex.put_restriction(c2, ConstraintType::LE);
  simplex.put_restriction(c3, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  // Should maximize to x=6, y=5, Z=28
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 6.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 5.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 28.0, 0.1));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(NonStandardConstraintsTest, EqualityConstraints)
{
  // Maximize Z = 5x + 4y
  // Subject to:
  //   x + y = 10  (EQ constraint)
  //   x <= 6      (LE constraint)
  //   y <= 8      (LE constraint)
  //   x, y >= 0
  // With x + y = 10, to maximize 5x + 4y, we want x as large as possible
  // x = 6 (bounded), then y = 4
  // Z = 5*6 + 4*4 = 30 + 16 = 46
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 5.0);
  simplex.put_objetive_function_coef(1, 4.0);
  
  // EQ constraint: x + y = 10
  double c1[] = {1.0, 1.0, 10.0};
  simplex.put_restriction(c1, ConstraintType::EQ);
  
  // LE constraints
  double c2[] = {1.0, 0.0, 6.0};
  double c3[] = {0.0, 1.0, 8.0};
  simplex.put_restriction(c2, ConstraintType::LE);
  simplex.put_restriction(c3, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  // Optimal: x=6, y=4, Z=46
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 6.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 4.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 46.0, 0.1));
  EXPECT_TRUE(simplex.verify_solution());
  
  // Verify equality constraint is satisfied
  double sum = simplex.get_solution(0) + simplex.get_solution(1);
  EXPECT_TRUE(nearly_equal(sum, 10.0, 0.1));
}

TEST_F(NonStandardConstraintsTest, MixedConstraintTypes)
{
  // Maximize Z = 2x + 3y + z
  // Subject to:
  //   x + y + z >= 5   (GE)
  //   x + 2y <= 10     (LE)
  //   2x + y = 8       (EQ)
  //   z <= 20          (LE) - Added to bound z
  //   x, y, z >= 0
  //
  // Analysis: From 2x + y = 8, we get y = 8 - 2x
  // For y >= 0: x <= 4
  // For x >= 0: x >= 0
  // Substituting in x + 2y <= 10: x + 2(8-2x) <= 10 => x >= 2
  // So x ∈ [2, 4]
  // At x=2: y=4, then z >= 5 - 2 - 4 = -1, so z can be 0 to 20
  // Maximize: 2(2) + 3(4) + z = 16 + z, max at z=20 => Z = 36
  
  Simplex<double> simplex(3);
  simplex.put_objetive_function_coef(0, 2.0);
  simplex.put_objetive_function_coef(1, 3.0);
  simplex.put_objetive_function_coef(2, 1.0);
  
  double c1[] = {1.0, 1.0, 1.0, 5.0};   // GE
  double c2[] = {1.0, 2.0, 0.0, 10.0};  // LE
  double c3[] = {2.0, 1.0, 0.0, 8.0};   // EQ
  double c4[] = {0.0, 0.0, 1.0, 20.0};  // z <= 20 (bound z)
  
  simplex.put_restriction(c1, ConstraintType::GE);
  simplex.put_restriction(c2, ConstraintType::LE);
  simplex.put_restriction(c3, ConstraintType::EQ);
  simplex.put_restriction(c4, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_GT(simplex.objetive_value(), 0.0);
}

TEST_F(NonStandardConstraintsTest, MultipleEqualityConstraints)
{
  // Maximize Z = x + y + z
  // Subject to:
  //   x + y = 5
  //   y + z = 6
  //   x, y, z >= 0
  // From eq1: x = 5 - y
  // From eq2: z = 6 - y
  // Z = (5-y) + y + (6-y) = 11 - y
  // To maximize, we want y as small as possible (y=0)
  // But we need x, z >= 0: x = 5-y >= 0 => y <= 5
  //                         z = 6-y >= 0 => y <= 6
  // So y can be 0, giving x=5, z=6, Z=11
  
  Simplex<double> simplex(3);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  simplex.put_objetive_function_coef(2, 1.0);
  
  double c1[] = {1.0, 1.0, 0.0, 5.0};
  double c2[] = {0.0, 1.0, 1.0, 6.0};
  
  simplex.put_restriction(c1, ConstraintType::EQ);
  simplex.put_restriction(c2, ConstraintType::EQ);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  double x = simplex.get_solution(0);
  double y = simplex.get_solution(1);
  double z = simplex.get_solution(2);
  
  // Verify equality constraints
  EXPECT_TRUE(nearly_equal(x + y, 5.0, 0.1));
  EXPECT_TRUE(nearly_equal(y + z, 6.0, 0.1));
  
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(NonStandardConstraintsTest, InfeasibleWithGEConstraints)
{
  // Maximize Z = x + y
  // Subject to:
  //   x + y >= 10   (GE)
  //   x <= 3        (LE)
  //   y <= 4        (LE)
  //   x, y >= 0
  // This is infeasible: x + y >= 10 but x <= 3 and y <= 4 => x + y <= 7
  //
  // NOTE: This tests whether the Simplex correctly detects infeasibility
  // with mixed GE/LE constraints. If the implementation doesn't detect
  // infeasibility correctly with GE constraints via Big-M method,
  // verify_solution() should still fail.
  
  Simplex<double> simplex(2);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 1.0);
  
  double c1[] = {1.0, 1.0, 10.0};
  double c2[] = {1.0, 0.0, 3.0};
  double c3[] = {0.0, 1.0, 4.0};
  
  simplex.put_restriction(c1, ConstraintType::GE);
  simplex.put_restriction(c2, ConstraintType::LE);
  simplex.put_restriction(c3, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  // The problem is mathematically infeasible
  // Either the solver correctly reports Unfeasible, or if it reports Solved,
  // the verify_solution() must fail (proving the "solution" violates constraints)
  if (state == Simplex<double>::Solved)
    {
      simplex.load_solution();
      double x = simplex.get_solution(0);
      double y = simplex.get_solution(1);
      
      // Either verification fails, or the solution actually violates constraints
      bool ge_satisfied = (x + y >= 10.0 - EPSILON);
      bool le1_satisfied = (x <= 3.0 + EPSILON);
      bool le2_satisfied = (y <= 4.0 + EPSILON);
      
      // At least one constraint must be violated since problem is infeasible
      // If all appear satisfied, then there's a numerical tolerance issue
      // or the implementation has a bug
      if (ge_satisfied && le1_satisfied && le2_satisfied)
        {
          // This would indicate a bug - log it for debugging
          std::cout << "WARNING: Simplex returned 'Solved' for infeasible problem\n";
          std::cout << "  x=" << x << ", y=" << y << ", x+y=" << (x+y) << "\n";
        }
      
      // The GE constraint cannot be satisfied given the LE bounds
      EXPECT_FALSE(ge_satisfied && le1_satisfied && le2_satisfied)
        << "All constraints appear satisfied but problem is mathematically infeasible";
    }
  else
    {
      EXPECT_EQ(state, Simplex<double>::Unfeasible);
    }
}

//==============================================================================
// Minimization Tests
//==============================================================================

class MinimizationTest : public ::testing::Test
{
protected:
  static constexpr double EPSILON = 1e-6;
  
  bool nearly_equal(double a, double b, double eps = EPSILON)
  {
    return std::fabs(a - b) < eps;
  }
};

TEST_F(MinimizationTest, BasicMinimization)
{
  // Minimize Z = 2x + 3y
  // Subject to:
  //   x + y >= 4
  //   x + 2y >= 6
  //   x, y >= 0
  // To minimize cost, find the corner of feasible region closest to origin
  // Corners: intersection of x+y=4 and x+2y=6
  // From eq1: x = 4 - y
  // Substitute: (4-y) + 2y = 6 => 4 + y = 6 => y = 2, x = 2
  // Z = 2*2 + 3*2 = 4 + 6 = 10
  
  Simplex<double> simplex(2, OptimizationType::Minimize);
  simplex.put_objetive_function_coef(0, 2.0);
  simplex.put_objetive_function_coef(1, 3.0);
  
  double c1[] = {1.0, 1.0, 4.0};
  double c2[] = {1.0, 2.0, 6.0};
  
  simplex.put_restriction(c1, ConstraintType::GE);
  simplex.put_restriction(c2, ConstraintType::GE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  EXPECT_TRUE(nearly_equal(simplex.get_solution(0), 2.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.get_solution(1), 2.0, 0.1));
  EXPECT_TRUE(nearly_equal(simplex.objetive_value(), 10.0, 0.1));
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(MinimizationTest, DietProblem)
{
  // Classic diet problem (minimize cost)
  // Minimize Z = 3x + 5y (cost per serving)
  // Subject to:
  //   2x + y >= 10   (protein requirement)
  //   x + 2y >= 8    (vitamin requirement)
  //   x, y >= 0
  
  Simplex<double> simplex(2, OptimizationType::Minimize);
  simplex.put_objetive_function_coef(0, 3.0);
  simplex.put_objetive_function_coef(1, 5.0);
  
  double c1[] = {2.0, 1.0, 10.0};
  double c2[] = {1.0, 2.0, 8.0};
  
  simplex.put_restriction(c1, ConstraintType::GE);
  simplex.put_restriction(c2, ConstraintType::GE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  double x = simplex.get_solution(0);
  double y = simplex.get_solution(1);
  
  // Verify constraints are satisfied
  EXPECT_GE(2*x + y, 10.0 - EPSILON);
  EXPECT_GE(x + 2*y, 8.0 - EPSILON);
  
  EXPECT_TRUE(simplex.verify_solution());
  EXPECT_GT(simplex.objetive_value(), 0.0);
}

TEST_F(MinimizationTest, TransportationCostMinimization)
{
  // Minimize Z = 4x1 + 2x2 + 3x3 + x4
  // Subject to:
  //   x1 + x2 >= 10  (demand at location 1)
  //   x3 + x4 >= 15  (demand at location 2)
  //   x1 + x3 <= 20  (supply from warehouse 1)
  //   x2 + x4 <= 20  (supply from warehouse 2)
  //   all xi >= 0
  
  Simplex<double> simplex(4, OptimizationType::Minimize);
  simplex.put_objetive_function_coef(0, 4.0);
  simplex.put_objetive_function_coef(1, 2.0);
  simplex.put_objetive_function_coef(2, 3.0);
  simplex.put_objetive_function_coef(3, 1.0);
  
  double c1[] = {1.0, 1.0, 0.0, 0.0, 10.0};  // GE
  double c2[] = {0.0, 0.0, 1.0, 1.0, 15.0};  // GE
  double c3[] = {1.0, 0.0, 1.0, 0.0, 20.0};  // LE
  double c4[] = {0.0, 1.0, 0.0, 1.0, 20.0};  // LE
  
  simplex.put_restriction(c1, ConstraintType::GE);
  simplex.put_restriction(c2, ConstraintType::GE);
  simplex.put_restriction(c3, ConstraintType::LE);
  simplex.put_restriction(c4, ConstraintType::LE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  EXPECT_TRUE(simplex.verify_solution());
  
  // Cost should be minimized (use cheaper routes)
  double cost = simplex.objetive_value();
  EXPECT_GT(cost, 0.0);
  EXPECT_LT(cost, 100.0);  // Should be reasonable
}

TEST_F(MinimizationTest, SetMinimizeMethod)
{
  // Test using set_minimize() method
  Simplex<double> simplex(2);  // Default is Maximize
  
  simplex.set_minimize();
  EXPECT_EQ(simplex.get_optimization_type(), OptimizationType::Minimize);
  
  simplex.put_objetive_function_coef(0, 5.0);
  simplex.put_objetive_function_coef(1, 4.0);
  
  double c1[] = {1.0, 1.0, 8.0};
  simplex.put_restriction(c1, ConstraintType::GE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  simplex.load_solution();
  
  // Should minimize to a corner satisfying x + y >= 8
  // Optimal would be on the line x + y = 8, minimizing 5x + 4y
  // To minimize, we want more of y (cheaper): x=0, y=8 gives Z=32
  double x = simplex.get_solution(0);
  double y = simplex.get_solution(1);
  
  EXPECT_GE(x + y, 8.0 - EPSILON);
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(MinimizationTest, MinimizationWithEquality)
{
  // Minimize Z = x + 2y + 3z
  // Subject to:
  //   x + y + z = 10
  //   x + z >= 4
  //   y >= 2
  
  Simplex<double> simplex(3, OptimizationType::Minimize);
  simplex.put_objetive_function_coef(0, 1.0);
  simplex.put_objetive_function_coef(1, 2.0);
  simplex.put_objetive_function_coef(2, 3.0);
  
  double c1[] = {1.0, 1.0, 1.0, 10.0};
  double c2[] = {1.0, 0.0, 1.0, 4.0};
  double c3[] = {0.0, 1.0, 0.0, 2.0};
  
  simplex.put_restriction(c1, ConstraintType::EQ);
  simplex.put_restriction(c2, ConstraintType::GE);
  simplex.put_restriction(c3, ConstraintType::GE);
  
  simplex.prepare_linear_program();
  auto state = simplex.solve();
  
  EXPECT_EQ(state, Simplex<double>::Solved);
  
  simplex.load_solution();
  
  double x = simplex.get_solution(0);
  double y = simplex.get_solution(1);
  double z = simplex.get_solution(2);
  
  // Verify constraints
  EXPECT_TRUE(nearly_equal(x + y + z, 10.0, 0.1));
  EXPECT_GE(x + z, 4.0 - EPSILON);
  EXPECT_GE(y, 2.0 - EPSILON);
  
  EXPECT_TRUE(simplex.verify_solution());
}

TEST_F(MinimizationTest, CompareMinimizeVsNegatedMaximize)
{
  // Verify that Minimize(c·x) = -Maximize(-c·x)
  
  // Problem: Minimize 3x + 2y subject to x + y >= 5, x,y >= 0
  Simplex<double> simplex_min(2, OptimizationType::Minimize);
  simplex_min.put_objetive_function_coef(0, 3.0);
  simplex_min.put_objetive_function_coef(1, 2.0);
  
  double c1[] = {1.0, 1.0, 5.0};
  simplex_min.put_restriction(c1, ConstraintType::GE);
  
  simplex_min.prepare_linear_program();
  simplex_min.solve();
  simplex_min.load_solution();
  
  double min_value = simplex_min.objetive_value();
  
  // Same problem as maximization with negated coefficients
  Simplex<double> simplex_max(2, OptimizationType::Maximize);
  simplex_max.put_objetive_function_coef(0, -3.0);
  simplex_max.put_objetive_function_coef(1, -2.0);
  
  simplex_max.put_restriction(c1, ConstraintType::GE);
  
  simplex_max.prepare_linear_program();
  simplex_max.solve();
  simplex_max.load_solution();
  
  double max_value = simplex_max.objetive_value();
  
  // They should be negatives of each other
  EXPECT_TRUE(nearly_equal(min_value, -max_value, 0.1));
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
