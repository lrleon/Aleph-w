
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
#include <algorithm>
#include <cmath>
#include <filesystem>
#include <fstream>
#include <random>
#include <sstream>
#include <string>
#include <utility>
#include <vector>

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

  // x = y = 20 stays optimal while 40/30 lies between the slopes 1 and 2
  // of the constraints: c_0 in [30, 60], c_1 in [20, 40]
  EXPECT_NEAR(range.lower_bound, 30.0, 1e-9);
  EXPECT_NEAR(range.upper_bound, 60.0, 1e-9);
  range = simplex.objective_sensitivity(1);
  EXPECT_NEAR(range.lower_bound, 20.0, 1e-9);
  EXPECT_NEAR(range.upper_bound, 40.0, 1e-9);

  // b_0 in [30, 60] and b_1 in [40, 80] keep x and y basic and >= 0
  range = simplex.rhs_sensitivity(0);
  EXPECT_EQ(range.current_value, 40.0);
  EXPECT_NEAR(range.lower_bound, 30.0, 1e-9);
  EXPECT_NEAR(range.upper_bound, 60.0, 1e-9);
  range = simplex.rhs_sensitivity(1);
  EXPECT_NEAR(range.lower_bound, 40.0, 1e-9);
  EXPECT_NEAR(range.upper_bound, 80.0, 1e-9);
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

  // The optimal dual solution: y_0 + 2 y_1 = 40 and y_0 + y_1 = 30, so
  // that 40 y_0 + 60 y_1 = 1400, the optimal value; >= 0, since they are
  // of <= constraints in a maximization
  EXPECT_NEAR(prices[0], 20.0, 1e-9);
  EXPECT_NEAR(prices[1], 10.0, 1e-9);
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

  // Both variables are basic
  EXPECT_NEAR(costs[0], 0.0, 1e-9);
  EXPECT_NEAR(costs[1], 0.0, 1e-9);
  EXPECT_TRUE(simplex.is_basic_variable(0));
  EXPECT_TRUE(simplex.is_basic_variable(1));
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
  
  // 50 is within [30, 60], the range of b_0: x = 60 - 50, y = 2 * 50 - 60,
  // and the value grows by the shadow price 20 per unit
  ASSERT_EQ(state2, Simplex<double>::Solved);
  simplex.load_solution();
  EXPECT_NEAR(simplex.get_solution(0), 10.0, 1e-9);
  EXPECT_NEAR(simplex.get_solution(1), 40.0, 1e-9);
  EXPECT_NEAR(simplex.objetive_value(), original_obj + 20.0 * 10, 1e-9);
  EXPECT_TRUE(simplex.verify_solution());
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
  
  // The problem is infeasible: x + y >= 10, x <= 3 and y <= 4. Simplex
  // reported it as solved, with a point that broke a constraint.
  EXPECT_EQ(state, Simplex<double>::Unfeasible);
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
// The defects of the Big-M version, and the classic hard programs
//==============================================================================

namespace
{
  using Rows = std::initializer_list<std::pair<std::vector<double>, ConstraintType>>;

  // Adds the rows to lp and prepares it
  void add_rows(Simplex<double> & lp, Rows rows)
  {
    for (const auto & [row, type] : rows)
      lp.put_restriction(row.data(), type);
    lp.prepare_linear_program();
  }

  // A file name in the temporary directory, unused
  std::string temp_name(const std::string & stem)
  {
    return (std::filesystem::temp_directory_path()
            / (stem + "_" + std::to_string(std::random_device{}()))).string();
  }

  std::string read_file(const std::string & name)
  {
    std::ifstream in(name);
    std::ostringstream s;
    s << in.rdbuf();
    return s.str();
  }
}

TEST(SimplexDefects, AnInfeasibleProgramIsReported)
{
  // x <= 0 and x >= 1: Simplex reported it as solved
  Simplex<double> lp(1);
  lp.put_objetive_function_coef(0, 1);
  add_rows(lp, {{{1, 0}, ConstraintType::LE}, {{1, 1}, ConstraintType::GE}});

  EXPECT_EQ(lp.solve(), Simplex<double>::Unfeasible);
  EXPECT_EQ(lp.get_state(), Simplex<double>::Unfeasible);
  EXPECT_THROW((void) lp.shadow_price(0), std::logic_error);
  EXPECT_THROW((void) lp.update_rhs_and_reoptimize(0, 2), std::logic_error);
}

TEST(SimplexDefects, ColumnsWithTheSameUnitVectorAreNotBothBasic)
{
  // max x + y, x + y <= 1: at the optimum both columns are the same unit
  // vector of cost zero, and the solution was read as x = y = 1
  Simplex<double> lp(2);
  lp.put_objetive_function_coef(0, 1);
  lp.put_objetive_function_coef(1, 1);
  add_rows(lp, {{{1, 1, 1}, ConstraintType::LE}});

  ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
  lp.load_solution();
  EXPECT_NEAR(lp.get_solution(0) + lp.get_solution(1), 1.0, 1e-12);
  EXPECT_NEAR(lp.objetive_value(), 1.0, 1e-12);
  EXPECT_TRUE(lp.verify_solution());
  EXPECT_NE(lp.is_basic_variable(0), lp.is_basic_variable(1));
}

TEST(SimplexDefects, ANegativeRightHandSideIsNotIgnored)
{
  {
    // max -x, -x <= -1, x <= 3: the first row was skipped, which gave x = 0
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, -1);
    add_rows(lp, {{{-1, -1}, ConstraintType::LE}, {{1, 3}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 1.0, 1e-12);
    EXPECT_NEAR(lp.objetive_value(), -1.0, 1e-12);
    EXPECT_TRUE(lp.verify_solution());
    // the shadow price of -x <= -1 when maximizing -x: raising b by one
    // lowers x by one, and raises the objective by one
    EXPECT_NEAR(lp.shadow_price(0), 1.0, 1e-12);
  }
  {
    // min x + y, x - y = -1: (0, 1)
    Simplex<double> lp(2, OptimizationType::Minimize);
    lp.put_objetive_function_coef(0, 1);
    lp.put_objetive_function_coef(1, 1);
    add_rows(lp, {{{1, -1, -1}, ConstraintType::EQ}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 0.0, 1e-12);
    EXPECT_NEAR(lp.get_solution(1), 1.0, 1e-12);
  }
  {
    // max -x - y, x - y >= -2, which holds at 0: (0, 0)
    Simplex<double> lp(2);
    lp.put_objetive_function_coef(0, -1);
    lp.put_objetive_function_coef(1, -1);
    add_rows(lp, {{{1, -1, -2}, ConstraintType::GE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.objetive_value(), 0.0, 1e-12);
  }
  {
    // x + y <= 5 and x + y >= 10 written as -x - y <= -10: infeasible
    Simplex<double> lp(2);
    lp.put_objetive_function_coef(0, 1);
    lp.put_objetive_function_coef(1, 1);
    add_rows(lp, {{{1, 1, 5}, ConstraintType::LE}, {{-1, -1, -10}, ConstraintType::LE}});
    EXPECT_EQ(lp.solve(), Simplex<double>::Unfeasible);
  }
}

TEST(SimplexDefects, AnObjectiveAboveTheBigMOfBefore)
{
  // min 1e12 x, x >= 1: with an artificial variable of cost 1e9, keeping it
  // in the basis was cheaper than x, and that was the solution
  Simplex<double> lp(1, OptimizationType::Minimize);
  lp.put_objetive_function_coef(0, 1e12);
  add_rows(lp, {{{1, 1}, ConstraintType::GE}});
  ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
  lp.load_solution();
  EXPECT_NEAR(lp.get_solution(0), 1.0, 1e-12);
  EXPECT_NEAR(lp.objetive_value(), 1e12, 1e-3);
}

TEST(SimplexDefects, SensitivityOfEqualitiesAndOfGreaterOrEqual)
{
  {
    // max x, x = 2: the shadow price of an = was 0
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{1, 2}, ConstraintType::EQ}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    EXPECT_NEAR(lp.shadow_price(0), 1.0, 1e-12);
  }
  {
    // min x + y, x + y = 3, x - y = 1: (2, 1); the dual y_0 + y_1 = 1,
    // y_0 - y_1 = 1 gives (1, 0)
    Simplex<double> lp(2, OptimizationType::Minimize);
    lp.put_objetive_function_coef(0, 1);
    lp.put_objetive_function_coef(1, 1);
    add_rows(lp, {{{1, 1, 3}, ConstraintType::EQ}, {{1, -1, 1}, ConstraintType::EQ}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 2.0, 1e-12);
    EXPECT_NEAR(lp.get_solution(1), 1.0, 1e-12);
    EXPECT_NEAR(lp.shadow_price(0), 1.0, 1e-12);
    EXPECT_NEAR(lp.shadow_price(1), 0.0, 1e-12);
  }
  {
    // min 2x + 3y, x + y >= 4, x + 3y >= 6: (3, 1), z = 9; the duals 1.5
    // and 0.5; x = (3 b_0 - b_1) / 2 and y = (b_1 - b_0) / 2 give b_0 in
    // [2, 6] and b_1 in [4, 12]; c_x / c_y in [1/3, 1] gives c_x in [1, 3]
    // and c_y in [2, 6]
    Simplex<double> lp(2, OptimizationType::Minimize);
    lp.put_objetive_function_coef(0, 2);
    lp.put_objetive_function_coef(1, 3);
    add_rows(lp, {{{1, 1, 4}, ConstraintType::GE}, {{1, 3, 6}, ConstraintType::GE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.objetive_value(), 9.0, 1e-12);
    EXPECT_NEAR(lp.shadow_price(0), 1.5, 1e-12);
    EXPECT_NEAR(lp.shadow_price(1), 0.5, 1e-12);

    auto r = lp.rhs_sensitivity(0);
    EXPECT_NEAR(r.lower_bound, 2.0, 1e-12);
    EXPECT_NEAR(r.upper_bound, 6.0, 1e-12);
    r = lp.rhs_sensitivity(1);
    EXPECT_NEAR(r.lower_bound, 4.0, 1e-12);
    EXPECT_NEAR(r.upper_bound, 12.0, 1e-12);
    r = lp.objective_sensitivity(0);
    EXPECT_NEAR(r.lower_bound, 1.0, 1e-12);
    EXPECT_NEAR(r.upper_bound, 3.0, 1e-12);
    r = lp.objective_sensitivity(1);
    EXPECT_NEAR(r.lower_bound, 2.0, 1e-12);
    EXPECT_NEAR(r.upper_bound, 6.0, 1e-12);
  }
  {
    // A non-basic variable: max x, x <= 3, x + y <= 5 has y = 0, and it
    // stays so while its cost is <= 0
    Simplex<double> lp(2);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{1, 0, 3}, ConstraintType::LE}, {{1, 1, 5}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    if (not lp.is_basic_variable(1))
      {
        const auto r = lp.objective_sensitivity(1);
        EXPECT_TRUE(r.is_unbounded_below());
        EXPECT_NEAR(r.upper_bound, 0.0, 1e-12);
      }
  }
}

TEST(SimplexDefects, ReoptimizationOfEqualitiesAndOfGreaterOrEqual)
{
  {
    // max x, x = 2, x <= 10; x = 5: the change of an = was lost
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{1, 2}, ConstraintType::EQ}, {{1, 10}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    ASSERT_EQ(lp.update_rhs_and_reoptimize(0, 5), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 5.0, 1e-12);
    // beyond the <= : infeasible
    EXPECT_EQ(lp.update_rhs_and_reoptimize(0, 11), Simplex<double>::Unfeasible);
    EXPECT_THROW((void) lp.update_rhs_and_reoptimize(0, 1), std::logic_error);
  }
  {
    // min x + y, x + y >= 2, x - y = 0: (1, 1); x + y >= 4: (2, 2)
    Simplex<double> lp(2, OptimizationType::Minimize);
    lp.put_objetive_function_coef(0, 1);
    lp.put_objetive_function_coef(1, 1);
    add_rows(lp, {{{1, 1, 2}, ConstraintType::GE}, {{1, -1, 0}, ConstraintType::EQ}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    ASSERT_EQ(lp.update_rhs_and_reoptimize(0, 4), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 2.0, 1e-12);
    EXPECT_NEAR(lp.get_solution(1), 2.0, 1e-12);
    // x - y = 2 too: (3, 1)
    ASSERT_EQ(lp.update_rhs_and_reoptimize(1, 2), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 3.0, 1e-12);
    EXPECT_NEAR(lp.get_solution(1), 1.0, 1e-12);
    EXPECT_TRUE(lp.verify_solution());
  }
  {
    // max -x, -x <= -1, whose row was multiplied by -1, and x <= 3: x = 1;
    // -x <= 2 always holds: x = 0; x <= -1 cannot hold
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, -1);
    add_rows(lp, {{{-1, -1}, ConstraintType::LE}, {{1, 3}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    ASSERT_EQ(lp.update_rhs_and_reoptimize(0, 2), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 0.0, 1e-12);
    EXPECT_EQ(lp.update_rhs_and_reoptimize(1, -1), Simplex<double>::Unfeasible);
  }
  {
    // x + y = 1 twice: the second row is redundant; x + y = 2 in it makes
    // the program infeasible
    Simplex<double> lp(2);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{1, 1, 1}, ConstraintType::EQ}, {{1, 1, 1}, ConstraintType::EQ}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.objetive_value(), 1.0, 1e-12);
    // the duals are not unique, but they add up to the value
    EXPECT_NEAR(lp.shadow_price(0) + lp.shadow_price(1), 1.0, 1e-12);
    EXPECT_EQ(lp.update_rhs_and_reoptimize(1, 2), Simplex<double>::Unfeasible);
  }
}

TEST(SimplexDefects, PrintAndLatexOutput)
{
  // print_matrix(), latex_matrix() and so latex_solve() did not compile
  Simplex<double> lp(2, OptimizationType::Minimize);
  lp.put_objetive_function_coef(0, 2);
  lp.put_objetive_function_coef(1, 3);
  add_rows(lp, {{{1, 1, 4}, ConstraintType::GE}, {{1, 3, 6}, ConstraintType::EQ},
                {{1, 0, 9}, ConstraintType::LE}});

  testing::internal::CaptureStdout();
  lp.print_matrix();
  const std::string out = testing::internal::GetCapturedStdout();
  EXPECT_EQ(std::count(out.begin(), out.end(), '\n'), 4); // row 0 and three rows
  EXPECT_NE(out.find("4.00"), std::string::npos);         // a right-hand side

  const std::string program = temp_name("aleph_simplex_program") + ".tex";
  lp.latex_linear_program(program);
  const std::string text = read_file(program);
  EXPECT_NE(text.find("\\min"), std::string::npos);
  EXPECT_NE(text.find("\\geq"), std::string::npos);
  EXPECT_NE(text.find(" = "), std::string::npos);
  EXPECT_NE(text.find("\\leq"), std::string::npos);
  std::filesystem::remove(program);

  const std::string base = temp_name("aleph_simplex_steps");
  ASSERT_EQ(lp.latex_solve(base.c_str()), Simplex<double>::Solved);
  EXPECT_NE(read_file(base + "-1.tex").find("\\circled"), std::string::npos);
  for (size_t k = 0; k <= lp.get_pivot_count() + 1; ++k)
    {
      std::filesystem::remove(base + "-" + std::to_string(k) + ".tex");
      std::filesystem::remove(base + "-" + std::to_string(k) + "-after.tex");
    }
  lp.load_solution();
  EXPECT_NEAR(lp.objetive_value(), 9.0, 1e-12);

  Simplex<double> unprepared(1);
  EXPECT_THROW(unprepared.print_matrix(), std::logic_error);
}

TEST(SimplexClassic, BealesCyclingProgramEnds)
{
  // Beale (1955): a degenerate program on which the most negative cost
  // with ties broken by the least index cycles; the optimum is 1/20 at
  // x = (1/25, 0, 1, 0)
  for (bool bland : {false, true})
    {
      Simplex<double> lp(4);
      lp.put_objetive_function_coef(0, 0.75);
      lp.put_objetive_function_coef(1, -150);
      lp.put_objetive_function_coef(2, 0.02);
      lp.put_objetive_function_coef(3, -6);
      if (bland)
        lp.enable_bland_rule();
      add_rows(lp, {{{0.25, -60, -0.04, 9, 0}, ConstraintType::LE},
                    {{0.5, -90, -0.02, 3, 0}, ConstraintType::LE},
                    {{0, 0, 1, 0, 1}, ConstraintType::LE}});
      ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
      lp.load_solution();
      EXPECT_NEAR(lp.objetive_value(), 0.05, 1e-12);
      EXPECT_NEAR(lp.get_solution(0), 0.04, 1e-12);
      EXPECT_NEAR(lp.get_solution(2), 1.0, 1e-12);
      EXPECT_TRUE(lp.verify_solution());

      const auto & st = lp.get_stats();
      EXPECT_GE(st.iterations, st.pivots);
      EXPECT_GE(st.pivots, st.degenerate_pivots);
      EXPECT_GT(st.degenerate_pivots, 0u);
    }
}

TEST(SimplexClassic, KleeMintyCube)
{
  // max sum 2^(n-j) x_j, sum_{j<i} 2^(i-j+1) x_j + x_i <= 5^i: the most
  // negative cost visits every vertex; the optimum is x_n = 5^n
  const size_t n = 8;
  for (bool bland : {false, true})
    {
      Simplex<double> lp(n);
      for (size_t j = 0; j < n; ++j)
        lp.put_objetive_function_coef(j, std::pow(2.0, double(n - 1 - j)));
      if (bland)
        lp.enable_bland_rule();
      for (size_t i = 0; i < n; ++i)
        {
          std::vector<double> row(n + 1, 0.0);
          for (size_t j = 0; j < i; ++j)
            row[j] = std::pow(2.0, double(i - j + 1));
          row[i] = 1;
          row[n] = std::pow(5.0, double(i + 1));
          lp.put_restriction(row.data());
        }
      lp.prepare_linear_program();
      ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
      lp.load_solution();
      EXPECT_NEAR(lp.objetive_value(), std::pow(5.0, double(n)), 1e-6);
      EXPECT_NEAR(lp.get_solution(n - 1), std::pow(5.0, double(n)), 1e-6);
      EXPECT_GE(lp.get_pivot_count(), n);
    }
}

TEST(SimplexClassic, ZeroRowsAndZeroObjective)
{
  {
    // 0 <= 5 always holds, 0 >= -5 too
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{0, 5}, ConstraintType::LE}, {{0, -5}, ConstraintType::GE},
                  {{1, 3}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 3.0, 1e-12);
  }
  for (auto [rhs, type] : {std::pair{5.0, ConstraintType::GE}, std::pair{3.0, ConstraintType::EQ},
                           std::pair{-1.0, ConstraintType::LE}})
    {
      // 0 >= 5, 0 = 3 and 0 <= -1 never hold
      Simplex<double> lp(1);
      lp.put_objetive_function_coef(0, 1);
      double row[] = {0, rhs};
      lp.put_restriction(row, type);
      lp.prepare_linear_program();
      EXPECT_EQ(lp.solve(), Simplex<double>::Unfeasible);
    }
  {
    // 0 = 0 is redundant
    Simplex<double> lp(1);
    lp.put_objetive_function_coef(0, 1);
    add_rows(lp, {{{0, 0}, ConstraintType::EQ}, {{1, 2}, ConstraintType::LE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.get_solution(0), 2.0, 1e-12);
  }
  {
    // a zero objective: any feasible point, of value 0
    Simplex<double> lp(2);
    add_rows(lp, {{{1, 1, 4}, ConstraintType::LE}, {{1, 0, 1}, ConstraintType::GE}});
    ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
    lp.load_solution();
    EXPECT_NEAR(lp.objetive_value(), 0.0, 1e-12);
    EXPECT_TRUE(lp.verify_solution());
  }
}

TEST(SimplexClassic, InconsistentEqualities)
{
  Simplex<double> lp(2);
  lp.put_objetive_function_coef(0, 1);
  add_rows(lp, {{{1, 1, 1}, ConstraintType::EQ}, {{1, 1, 2}, ConstraintType::EQ}});
  EXPECT_EQ(lp.solve(), Simplex<double>::Unfeasible);
}

TEST(SimplexClassic, AMovedSolverSolves)
{
  Simplex<double> lp(2);
  lp.put_objetive_function_coef(0, 40);
  lp.put_objetive_function_coef(1, 30);
  add_rows(lp, {{{1, 1, 40}, ConstraintType::LE}, {{2, 1, 60}, ConstraintType::LE}});

  Simplex<double> moved(std::move(lp));
  ASSERT_EQ(moved.solve(), Simplex<double>::Solved);
  moved.load_solution();
  EXPECT_NEAR(moved.objetive_value(), 1400.0, 1e-9);

  Simplex<double> assigned(1);
  assigned = std::move(moved);
  EXPECT_NEAR(assigned.shadow_price(0), 20.0, 1e-9);
}

TEST(SimplexClassic, StateErrors)
{
  Simplex<double> lp(1);
  lp.put_objetive_function_coef(0, 1);
  EXPECT_THROW(lp.prepare_linear_program(), std::logic_error);
  add_rows(lp, {{{1, 1}, ConstraintType::LE}});
  EXPECT_THROW((void) lp.shadow_price(0), std::logic_error);
  EXPECT_THROW((void) lp.reduced_cost(0), std::logic_error);
  EXPECT_THROW((void) lp.rhs_sensitivity(0), std::logic_error);
  EXPECT_THROW((void) lp.objective_sensitivity(0), std::logic_error);
  EXPECT_THROW((void) lp.dual_solve(), std::logic_error);
  EXPECT_THROW((void) lp.update_rhs_and_reoptimize(0, 1), std::logic_error);
  ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
  EXPECT_THROW((void) lp.solve(), std::logic_error);
  EXPECT_THROW((void) lp.latex_solve(), std::logic_error);
  EXPECT_THROW((void) lp.shadow_price(1), std::out_of_range);
  EXPECT_THROW((void) lp.objective_sensitivity(1), std::out_of_range);
  EXPECT_NO_THROW(lp.print_sensitivity_analysis());
}

TEST(RevisedSimplexDefects, ANegativeRightHandSideNeedsTheFirstPhase)
{
  // max -x, -x <= -1, x <= 3: the slack of the first row started at -1
  RevisedSimplex<double> rs(1, 2);
  rs.set_objective(0, -1);
  rs.set_constraint(0, 0, -1);
  rs.set_rhs(0, -1);
  rs.set_constraint(1, 0, 1);
  rs.set_rhs(1, 3);
  ASSERT_EQ(rs.solve(), RevisedSimplex<double>::Solved);
  EXPECT_NEAR(rs.get_solution(0), 1.0, 1e-12);
  EXPECT_NEAR(rs.objective_value(), -1.0, 1e-12);
  EXPECT_TRUE(rs.verify_solution());
}

TEST(RevisedSimplexDefects, InfeasibleAndUnbounded)
{
  {
    // x <= -1 with x >= 0
    RevisedSimplex<double> rs(1, 1);
    rs.set_objective(0, 1);
    rs.set_constraint(0, 0, 1);
    rs.set_rhs(0, -1);
    EXPECT_EQ(rs.solve(), RevisedSimplex<double>::Unfeasible);
  }
  {
    // max x, x - y <= 1
    RevisedSimplex<double> rs(2, 1);
    rs.set_objective(0, 1);
    const double row[] = {1, -1};
    rs.set_constraint_row(0, row, 1);
    EXPECT_EQ(rs.solve(), RevisedSimplex<double>::Unbounded);
  }
}

TEST(RevisedSimplexDefects, RefactorizationEveryPivotGivesTheSame)
{
  // the Klee-Minty cube of 6 dimensions, with B^-1 computed again after
  // each pivot and never
  const size_t n = 6;
  double value[2];
  for (size_t f : {size_t(1), size_t(1000)})
    {
      RevisedSimplex<double> rs(n, n);
      for (size_t j = 0; j < n; ++j)
        rs.set_objective(j, std::pow(2.0, double(n - 1 - j)));
      for (size_t i = 0; i < n; ++i)
        {
          for (size_t j = 0; j < i; ++j)
            rs.set_constraint(i, j, std::pow(2.0, double(i - j + 1)));
          rs.set_constraint(i, i, 1);
          rs.set_rhs(i, std::pow(5.0, double(i + 1)));
        }
      rs.set_refactorization_frequency(f);
      ASSERT_EQ(rs.solve(), RevisedSimplex<double>::Solved);
      value[f == 1 ? 0 : 1] = rs.objective_value();
    }
  EXPECT_NEAR(value[0], std::pow(5.0, double(n)), 1e-6);
  EXPECT_NEAR(value[1], std::pow(5.0, double(n)), 1e-6);

  RevisedSimplex<double> rs(1, 1);
  EXPECT_THROW(rs.set_refactorization_frequency(0), std::invalid_argument);
}

TEST(SimplexClassic, TheRestOfTheInterface)
{
  // min 2x + 3y, x + y >= 4 by put_ge_restriction(), x + 3y = 6 by
  // put_eq_restriction(): (3, 1), z = 9
  Simplex<double> lp(2);
  lp.set_optimization_type(OptimizationType::Minimize);
  EXPECT_EQ(lp.get_optimization_type(), OptimizationType::Minimize);
  lp.set_pivot_rule(Simplex<double>::PivotRule::Bland);
  lp.set_pivot_rule(Simplex<double>::PivotRule::Dantzig);
  lp.set_epsilon(1e-12);
  lp.put_objetive_function_coef(0, 2);
  lp.put_objetive_function_coef(1, 3);
  const double ge[] = {1, 1, 4}, eq[] = {1, 3, 6};
  lp.put_ge_restriction(ge);
  lp.put_eq_restriction(eq);
  lp.prepare_linear_program();
  ASSERT_EQ(lp.solve(), Simplex<double>::Solved);
  lp.load_solution();
  EXPECT_NEAR(lp.get_solution(0), 3.0, 1e-12);
  EXPECT_NEAR(lp.get_solution(1), 1.0, 1e-12);

  EXPECT_GE(lp.get_iteration_count(), lp.get_pivot_count());
  EXPECT_LE(lp.get_degenerate_pivot_count(), lp.get_pivot_count());
  EXPECT_GE(lp.get_elapsed_time_ms(), 0.0);
  EXPECT_FALSE(lp.get_stats().bland_rule_used);

  // nothing to restore: no pivot
  ASSERT_EQ(lp.dual_solve(), Simplex<double>::Solved);
  EXPECT_EQ(lp.get_pivot_count(), 0u);

  testing::internal::CaptureStdout();
  lp.print_stats();
  lp.print_sensitivity_analysis();
  const std::string out = testing::internal::GetCapturedStdout();
  EXPECT_NE(out.find("Pivots:"), std::string::npos);
  EXPECT_NE(out.find("shadow price"), std::string::npos);

  const std::string name = temp_name("aleph_simplex_matrix") + ".tex";
  lp.latex_matrix(name, 3, 1, 0);
  EXPECT_NE(read_file(name).find("\\circled{"), std::string::npos);
  std::filesystem::remove(name);

  RevisedSimplex<double> rs(1, 1);
  rs.set_objective(0, 1);
  rs.set_constraint(0, 0, 1);
  rs.set_rhs(0, 2);
  ASSERT_EQ(rs.solve(), RevisedSimplex<double>::Solved);
  EXPECT_EQ(rs.get_num_vars(), 1u);
  EXPECT_EQ(rs.get_num_constraints(), 1u);
  EXPECT_EQ(rs.get_stats().pivots, 1u);
  testing::internal::CaptureStdout();
  rs.print_stats();
  EXPECT_NE(testing::internal::GetCapturedStdout().find("Refactorizations: 0"),
            std::string::npos);
}

//==============================================================================
// Main
//==============================================================================

int main(int argc, char **argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
