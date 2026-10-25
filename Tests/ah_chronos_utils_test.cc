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
 * @file ah_chronos_utils_test.cc
 * @brief Tests for the duration helpers of ah-chronos-utils.H.
 *
 * duration_to_string() exists only where the standard library has
 * std::format: libstdc++ since GCC 13 and libc++ since 17, which defines
 * __cpp_lib_format only since 19. Without it, the header still gives the
 * other helpers, which this program checks too.
 */

#include <gtest/gtest.h>

#include <ah-chronos-utils.H>

#include <chrono>

using namespace std::chrono_literals;

TEST(AhChronosUtils, ConvertsADurationToEachUnit)
{
  const std::chrono::duration<double> d = 1.5s;

  EXPECT_DOUBLE_EQ(Aleph::duration_to_seconds(d), 1.5);
  EXPECT_DOUBLE_EQ(Aleph::duration_to_milliseconds(d), 1500.0);
  EXPECT_DOUBLE_EQ(Aleph::duration_to_microseconds(d), 1.5e6);
  EXPECT_DOUBLE_EQ(Aleph::duration_to_nanoseconds(d), 1.5e9);
}

TEST(AhChronosUtils, CountsWholeMinutesHoursAndDays)
{
  EXPECT_DOUBLE_EQ(Aleph::duration_to_minutes(90s), 1.0);
  EXPECT_DOUBLE_EQ(Aleph::duration_to_hours(5400s), 1.0);
  EXPECT_DOUBLE_EQ(Aleph::duration_to_days(200000s), 2.0);
}

#if defined(__cpp_lib_format) || \
    (defined(_LIBCPP_VERSION) && _LIBCPP_VERSION >= 170000)
TEST(AhChronosUtils, DurationToStringPicksTheUnitByMagnitude)
{
  using seconds_d = std::chrono::duration<double>;

  EXPECT_EQ(Aleph::duration_to_string(seconds_d(0.25)), "250 ms");
  EXPECT_EQ(Aleph::duration_to_string(seconds_d(1.5)), "1.5 s");
  EXPECT_EQ(Aleph::duration_to_string(seconds_d(90)), "1 min");
  EXPECT_EQ(Aleph::duration_to_string(seconds_d(5400)), "1 h");
  EXPECT_EQ(Aleph::duration_to_string(seconds_d(200000)), "2 days");
}

TEST(AhChronosUtils, DurationToStringTakesAnyDurationType)
{
  EXPECT_EQ(Aleph::duration_to_string(250ms), "250 ms");
  EXPECT_EQ(Aleph::duration_to_string(2min), "2 min");
}
#endif
