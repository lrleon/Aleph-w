
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
 * @file simplex_allocation_test.cc
 * @brief Simplex when an allocation of prepare_linear_program() fails: the
 *        program builds no tableau, still takes restrictions, and a new
 *        preparation solves it.
 *
 * This program replaces the global operator new, so that a test can make
 * an allocation fail.
 */

# include <gtest/gtest.h>

# include "allocation_failure.H"

# include <cstddef>
# include <new>
# include <string>

# include <Simplex.H>

using namespace Aleph;

// The tableau was kept as soon as it was allocated: after a preparation
// that failed later, as with std::bad_alloc, every restriction was refused
// as if the program had been prepared
TEST(SimplexAllocation, AFailedPreparationStillTakesRestrictions)
{
  if (ALEPH_ALLOCATION_FAILURE_UNDER_TSAN)
    GTEST_SKIP() << "ThreadSanitizer replaces the global operator new";

  size_t failures = 0;
  for (size_t n = 1; n < 10000; ++n)
    {
      SCOPED_TRACE("the allocation " + std::to_string(n) + " fails");
      Simplex<double> simplex(2);  // maximize x + 2y
      simplex.put_objetive_function_coef(0, 1.0);
      simplex.put_objetive_function_coef(1, 2.0);
      double sum[] = {1.0, 1.0, 4.0};  // x + y <= 4
      simplex.put_restriction(sum);

      bool threw = false;
      {
        const Fail_At failing(n);
        try
          {
            simplex.prepare_linear_program();
          }
        catch (const std::bad_alloc &)
          {
            threw = true;
          }
      }
      if (not threw)
        {
          // completed: it must not have caught a failed allocation
          EXPECT_EQ(refused_allocations, 0u) << "no std::bad_alloc reached the caller";
          break;
        }
      ++failures;

      double y[] = {0.0, 1.0, 3.0};  // y <= 3
      ASSERT_NO_THROW(simplex.put_restriction(y));
      ASSERT_EQ(simplex.get_num_restrictions(), 2u);
      simplex.prepare_linear_program();
      ASSERT_EQ(simplex.solve(), Simplex<double>::Solved);
      simplex.load_solution();  // x = 1, y = 3
      EXPECT_NEAR(simplex.get_solution(0), 1.0, 1e-9);
      EXPECT_NEAR(simplex.get_solution(1), 3.0, 1e-9);
      EXPECT_NEAR(simplex.objetive_value(), 7.0, 1e-9);
    }
  EXPECT_GT(failures, 0u);
}
