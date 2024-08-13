//
// Created by lrleon on 8/13/24.
//

#ifndef AH_CHRONOS_UTILS_H
#define AH_CHRONOS_UTILS_H

# include <string>
# include <chrono>

using namespace std;
using namespace std::chrono;

namespace Aleph
{
// given a duration, return the number of seconds
inline double duration_to_seconds(const duration<double> & d)
{
  return d.count();
}

// given a duration, return the number of milliseconds
inline double duration_to_milliseconds(const duration<double> & d)
{
  return duration_cast<milliseconds>(d).count();
}

// given a duration, return the number of microseconds
inline double duration_to_microseconds(const duration<double> & d)
{
  return duration_cast<microseconds>(d).count();
}

// given a duration, return the number of nanoseconds
inline double duration_to_nanoseconds(const duration<double> & d)
{
  return duration_cast<nanoseconds>(d).count();
}

// given a duration, return the number of minutes
inline double duration_to_minutes(const duration<double> & d)
{
  return duration_cast<minutes>(d).count();
}

// given a duration, return the number of hours
inline double duration_to_hours(const duration<double> & d)
{
  return duration_cast<hours>(d).count();
}

// given a duration, return the number of days
inline double duration_to_days(const duration<double> & d)
{
  return duration_cast<days>(d).count();
}

// given a duration, return a string with the proper units
// acording to the magnitude of the duration
inline string duration_to_string(const auto & d)
{
  if (d < seconds(1))
    return to_string(duration_to_milliseconds(d)) + " ms";

  if (d < minutes(1))
    return to_string(duration_to_seconds(d)) + " s";
  if (d < hours(1))
    return to_string(duration_to_minutes(d)) + " min";
  if (d < days(1))
    return to_string(duration_to_hours(d)) + " h";

  return to_string(duration_to_days(d)) + " days";
}
}

#endif //AH_CHRONOS_UTILS_H
