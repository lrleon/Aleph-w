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
