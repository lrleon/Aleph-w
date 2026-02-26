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

/** @file test_helpers.H
 *  @brief Shared helper functions for Aleph-w unit tests.
 */

#ifndef TESTS_TEST_HELPERS_H
#define TESTS_TEST_HELPERS_H

#include <gtest/gtest.h>
#include <initializer_list>
#include <tpl_array.H>

namespace Aleph::test_helpers
{
  /** @brief Assert that an Aleph::Array contains exactly the elements in an initializer_list.
   *
   *  Performs equality checks between each element of the array and the corresponding
   *  element in the expected list.
   *
   *  @param[in] a The Aleph::Array to check.
   *  @param[in] expected Initializer list of expected values.
   */
  inline void expect_array_eq(const Array<size_t> & a,
                              std::initializer_list<size_t> expected)
  {
    ASSERT_EQ(a.size(), expected.size());
    size_t i = 0;
    for (const size_t value : expected)
      EXPECT_EQ(a[i++], value);
  }
}

#endif // TESTS_TEST_HELPERS_H
