
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
# ifndef AHNOW_H
# define AHNOW_H

# include <string>
# include <chrono>

# include <aleph.H>

using namespace std;
using namespace chrono;
using namespace Aleph;

namespace Aleph
{
  /** Class Now is a practical class for timing based in a high
   * resolution clock.
   *
   *  An instance of this class allows to measure elapsed time between
   *  two instants.
   *
   *  Usage example:
   *  \code{.cpp}
   *  Now now;
   *  now.start();
   *  // Any block of code
   *  cout << "Time elapsed: " << now.elapsed() << "ms\n";
   *  \endcode
   *  
   *  Furthermore you may use the static methods just like that:
   *  \code{.cpp}
   *  auto t = Now::current_time_point();
   *  // Any block of code
   *  cout << "Time elapsed: " << Now::elapsed(t) << "ms\n";
   *  \endcode
   *
   *  @ingroup utils
   *  @author Alejandro J. Mujica
   */
  class Now
  {
  public:
    /// Precision for timing.
    enum class Precision
      {
       Hours,
       Minutes,
       Seconds,
       Milliseconds,
       Microseconds,
       Nanoseconds
      };

    static int to_int(Precision p)
    {
      return static_cast<int>(p);
    }
    
    using ClockType = high_resolution_clock;
    using TimePointType = ClockType::time_point;
    using DurationType = ClockType::duration;

  private:
    TimePointType tp;

    Precision precision;

    static const double precision_values[];
    
  public:
    /// Gets the current time point.
    static TimePointType current_time_point()
    {
      return ClockType::now();
    }

    /** Calculates the time that has elapsed between two time points.
     * 
     * @param rtp Right time point.
     * @param ltp Left time point.
     * @param precision Type of precision for calculating the time.
     * @return The time calculated.
     * @see Precision
     */
    static double compute_time_diff(const TimePointType & rtp,
                                    const TimePointType & ltp,
                                    const Precision & precision)
    {
      DurationType et = std::chrono::duration_cast<nanoseconds>(rtp - ltp);
      return et.count() * precision_values[to_int(precision)];
    }

    /** \brief Builds a new object with default values.
     *
     *  Builds a new object with default time point and precision in
     *  MILLISECONDS. You may set the object to start counting time 
     *  immediately by setting the parameter start_now in true. This
     *  constructor can be used as parametric or default.
     *
     *  @param start_now If true, then the object starts counting the time 
     *  immediately. By default is false.
     *
     *  @see Precision
     */
    Now(bool start_now = false)
      : tp(), precision(Precision::Milliseconds)
    {
      if (start_now)
        start();
    }

    /** \brief Builds a new object with parametric precision time.
     *
     *  Builds a new object with default time point and precision given as a
     *  parameter. You may set the object to start counting time 
     *  immediately by setting the parameter start_now in true.
     *
     *  @param _precision The type of precision for timing.
     *  @param start_now If true, then the object starts counting the time 
     *  immediately. By default is false.
     *
     *  @see Precision
     */
    Now(const Precision & _precision, bool start_now = false)
      : tp(), precision(_precision)
    {
      if (start_now)
        start();
    }

    /** Gets the type of precision.
     * 
     *  @see Precision
     */
    const Precision & get_precision() const
    {
      return precision;
    }

    /** Sets the type of precision.
     * 
     *  @see Precision
     */
    void set_precision(const Precision & _precision)
    {
      precision = _precision;
    }

    /** Sets internally the current time point.
     *
     *  This method must be used before calling elapsed() or delta() the first
     *  time.
     *
     *  @return The current time point.
     *  @see current_time_point()
     */
    TimePointType start()
    {
      tp = this->current_time_point();
      return tp;
    }

    /** Calculates the time that has elapsed since the last time
     *  start(), elapsed() or delta() was called.
     *
     *  The value calculated depends of precision.
     *
     *  @return The time calculated.
     *  @see start(), delta(), Precision
     */
    double elapsed()
    {
      TimePointType ltp = tp;
      tp = this->current_time_point();
      return compute_time_diff(tp, ltp, precision);
    }

    /** Like elapsed().
     *
     * @see elapsed().
     */
    double delta()
    {
      return elapsed();
    }

    /** Calculates the time that has elapsed since a given time point.
     *
     *  This is a class method. It's useful when is required massive time
     *  calculations and is not desired massive object instances of Now.
     *
     *  @param tp Time point from which you want to calculate.
     *  @param precision The type of precision for timing, by default is
     *  MILLISECONDS.
     *  @return The time calculated.
     *  @see Precision
     */
    static double elapsed(const TimePointType & tp,
                          const Precision & precision = Precision::Milliseconds)
    {
      return compute_time_diff(current_time_point(), tp, precision);
    }

    /** Like elapsed(const TimePointType & tp,
     *               const Precision & precision = MILLISECONDS).
     *
     *  @see elapsed(const TimePointType & tp,
     *               const Precision & precision = MILLISECONDS)
     */
    static double delta(const TimePointType & tp,
                        const Precision & precision = Precision::Milliseconds)
    {
      return elapsed(tp, precision);
    }
  };  
} // End namespace Aleph
  
# endif // AHNOW_H
