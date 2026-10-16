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
 * @file cxx_standard_test.cc
 * @brief Checks that the tests are compiled with the C++ standard of the build.
 *
 * Tests/CMakeLists.txt used to force C++20, so that a build configured with
 * ALEPH_CXX_STANDARD=23 compiled the library as C++23 and every test as C++20.
 * ALEPH_EXPECTED_CXX_STANDARD is the standard of the library target, which
 * Tests/CMakeLists.txt passes to this program.
 */

#include <gtest/gtest.h>

#ifndef ALEPH_EXPECTED_CXX_STANDARD
#  error "Tests/CMakeLists.txt defines ALEPH_EXPECTED_CXX_STANDARD"
#endif

namespace
{
  // MSVC keeps __cplusplus at 199711L without /Zc:__cplusplus; _MSVC_LANG
  // holds the standard in use.
#if defined(_MSVC_LANG)
  constexpr long compiled_standard = _MSVC_LANG;
#else
  constexpr long compiled_standard = __cplusplus;
#endif
}

TEST(CxxStandard, TestsAreCompiledWithTheStandardOfTheBuild)
{
#if ALEPH_EXPECTED_CXX_STANDARD == 20
  EXPECT_EQ(compiled_standard, 202002L);
#elif ALEPH_EXPECTED_CXX_STANDARD == 23
  // 202100L before GCC 14, 202302L after
  EXPECT_GT(compiled_standard, 202002L);
#else
  FAIL() << "unexpected standard of the build: " << ALEPH_EXPECTED_CXX_STANDARD;
#endif
}
