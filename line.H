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

/** @file line.H
 *  @brief 2D line representation using slope-intercept form.
 *
 *  This file provides the LineEq class for representing infinite lines
 *  in 2D space using the slope-intercept form: y = y0 + m*x
 *
 *  ## Line Equation Form
 *
 *  The line is stored as:
 *  - y0: y-intercept (where line crosses y-axis)
 *  - m: slope (rise over run)
 *
 *  This gives the equation: y = y0 + m*x
 *
 *  ## Usage Example
 *
 *  ```cpp
 *  // Line through origin with slope 2
 *  LineEq line1(0.0, 2.0);  // y = 2x
 *
 *  // Line through two points
 *  LineEq line2(0.0, 0.0, 1.0, 1.0);  // y = x
 *
 *  // Evaluate line at x = 5
 *  double y = line1(5.0);  // y = 10
 *
 *  // Find intersection
 *  auto [x, y] = line1.intersection(line2);
 *  ```
 *
 *  @see segment.H Finite line segments
 *  @see point.H Point representation
 *
 *  @ingroup Geometria
 *  @author Leandro Rabindranath León
 *  @author Leandro Rabindranath León
 */

#ifndef LINE_H
#define LINE_H

# include <aleph-exceptions.H>

/**
 * @brief 2D infinite line in slope-intercept form.
 *
 * Represents an infinite line using the equation y = y0 + m*x where:
 * - y0 is the y-intercept
 * - m is the slope
 *
 * @note This representation cannot handle vertical lines (infinite slope).
 *       For vertical lines, use parametric or implicit form.
 *
 * @ingroup Geometria
 *  @author Leandro Rabindranath León
 */
struct LineEq
{
  mutable double y0 = 0;  ///< Y-intercept (where line crosses y-axis)
  mutable double m = 1;   ///< Slope (dy/dx)

  /**
   * @brief Default constructor creates line y = x.
   *
   * The default line passes through origin with slope 1.
   */
  LineEq() {}

  /**
   * @brief Construct line from y-intercept and slope.
   *
   * Creates line: y = y0 + m*x
   *
   * @param __y0 Y-intercept (value of y when x = 0)
   * @param __m Slope of the line
   *
   * @par Example
   * @code
   * LineEq line(5.0, 2.0);  // y = 5 + 2x
   * @endcode
   */
  LineEq(const double __y0, const double __m) : y0(__y0), m(__m) {}

  /**
   * @brief Construct line through a point with given slope.
   *
   * Computes y-intercept from point and slope.
   *
   * @param x1 X-coordinate of point on the line
   * @param y1 Y-coordinate of point on the line
   * @param __m Slope of the line
   *
   * @par Example
   * @code
   * // Line through (2, 4) with slope 3
   * LineEq line(2.0, 4.0, 3.0);  // y = -2 + 3x
   * @endcode
   */
  LineEq(const double x1, const double y1, const double __m) : m(__m)
  {
    y0 = y1 - m*x1;
  }

  /**
   * @brief Construct line through two points.
   *
   * Computes slope and y-intercept from the two points.
   *
   * @param x1 X-coordinate of first point
   * @param y1 Y-coordinate of first point
   * @param x2 X-coordinate of second point
   * @param y2 Y-coordinate of second point
   *
   * @throw OutOfRange if x1 >= x2 (points must be left-to-right)
   * @throw OutOfRange if y1 == y2 (horizontal line - use slope form)
   *
   * @par Example
   * @code
   * LineEq line(0.0, 0.0, 2.0, 6.0);  // y = 3x
   * @endcode
   */
  LineEq(const double x1, const double y1, const double x2, const double y2)
  {
    if (x1 >= x2)
      ALEPHTHROW(OutOfRange, "value x1 = " + ::to_string(x1) +
	       " is greater or equal than x2 = " + ::to_string(x2));
    if (y1 == y2)
      ALEPHTHROW(OutOfRange, "value of y1 = " + ::to_string(y1) + " == y2 = " +
	       ::to_string(y2));
    m = (y2 - y1)/(x2 - x1);
    y0 = y1 - m*x1;
  }

  /**
   * @brief Evaluate line at given x-coordinate.
   *
   * @param x X-coordinate to evaluate
   * @return Y-coordinate on the line: y0 + m*x
   *
   * @par Example
   * @code
   * LineEq line(1.0, 2.0);  // y = 1 + 2x
   * double y = line(3.0);   // y = 7
   * @endcode
   */
  double operator() (const double x) const noexcept { return y0 + m*x; }

  /**
   * @brief Compute intersection point with another line.
   *
   * Solves the system of equations to find where lines cross.
   *
   * @param l The other line
   * @return Pair (x, y) of intersection coordinates
   *
   * @throw OutOfRange if lines are parallel (same slope)
   *
   * @par Example
   * @code
   * LineEq l1(0.0, 1.0);   // y = x
   * LineEq l2(2.0, -1.0);  // y = 2 - x
   * auto [x, y] = l1.intersection(l2);  // (1, 1)
   * @endcode
   */
  pair<double, double> intersection(const LineEq & l) const
  {
    if (m == l.m)
      ALEPHTHROW(OutOfRange, "parallel lines");

    double x = (y0 - l.y0)/(l.m - m);

    return pair<double, double>(x, (*this)(x));
  }

  /**
   * @brief Test equality of two lines.
   *
   * Lines are equal if they have the same slope and y-intercept.
   *
   * @param l Line to compare with
   * @return true if lines are identical
   */
  bool operator == (const LineEq & l) const noexcept
  {
    return y0 == l.y0 and m == l.m;
  }

  /**
   * @brief Test inequality of two lines.
   *
   * @param l Line to compare with
   * @return true if lines differ
   */
  bool operator != (const LineEq & l) const noexcept
  {
    return not (*this == l);
  }

  /**
   * @brief Convert line to string representation.
   *
   * @return String in format "y = y0 + m * x"
   */
  string to_string() const
  {
    ostringstream s;
    s << "y = " << y0 << " + " << m << " * x";
    return s.str();
  }

  /**
   * @brief Output stream operator.
   *
   * @param out Output stream
   * @param l Line to output
   * @return Reference to output stream
   */
  friend ostream & operator << (ostream & out, const LineEq & l)
  {
    return out << l.to_string();
  }
};

#endif
