
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


/** @file ah-chronos-utils.H
 *  @brief Time duration conversion utilities.
 *
 *  Provides functions for converting std::chrono durations to various
 *  time units (seconds, milliseconds, microseconds, nanoseconds) and
 *  formatting durations as human-readable strings.
 *
 *  @ingroup Utilities
 *  @author Leandro Rabindranath León
 */

#ifndef AH_CHRONOS_UTILS_H
#define AH_CHRONOS_UTILS_H

# include <string>
# include <chrono>

namespace Aleph
{
// given a duration, return the number of seconds
inline double duration_to_seconds(const std::chrono::duration<double> & d)
{
  return d.count();
}

// given a duration, return the number of milliseconds
inline double duration_to_milliseconds(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::milliseconds>(d).count();
}

// given a duration, return the number of microseconds
inline double duration_to_microseconds(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::microseconds>(d).count();
}

// given a duration, return the number of nanoseconds
inline double duration_to_nanoseconds(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::nanoseconds>(d).count();
}

// given a duration, return the number of minutes
inline double duration_to_minutes(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::minutes>(d).count();
}

// given a duration, return the number of hours
inline double duration_to_hours(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::hours>(d).count();
}

// given a duration, return the number of days
inline double duration_to_days(const std::chrono::duration<double> & d)
{
  return std::chrono::duration_cast<std::chrono::days>(d).count();
}

// given a duration, return a string with the proper units
// acording to the magnitude of the duration
inline std::string duration_to_string(const auto & d)
{
  using namespace std::chrono;
  if (d < seconds(1))
    return std::to_string(duration_to_milliseconds(d)) + " ms";

  if (d < minutes(1))
    return std::to_string(duration_to_seconds(d)) + " s";
  if (d < hours(1))
    return std::to_string(duration_to_minutes(d)) + " min";
  if (d < days(1))
    return std::to_string(duration_to_hours(d)) + " h";

  return std::to_string(duration_to_days(d)) + " days";
}
}

#endif //AH_CHRONOS_UTILS_H
