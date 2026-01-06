/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9c
  /_/   \_\_|\___| .__/|_| |_|      \_/\_/   https://github.com/lrleon/Aleph-w
                 |_|

  This file is part of Aleph-w library

  Copyright (c) 2002-2018 Leandro Rabindranath Leon 

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

/** @file ah-chronos-utils.H
 *  @brief Time duration conversion utilities.
 *
 *  Provides functions for converting std::chrono durations to various
 *  time units (seconds, milliseconds, microseconds, nanoseconds) and
 *  formatting durations as human-readable strings.
 *
 *  @ingroup Utilidades
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
