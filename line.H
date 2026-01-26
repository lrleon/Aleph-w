
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


/** @file line.H
 *  @brief 2D infinite line representation using slope-intercept form.
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
 *  ## Features
 *
 *  - Construction from slope and intercept
 *  - Construction from a point and slope
 *  - Construction from two points
 *  - Line evaluation at any x-coordinate
 *  - Intersection computation between lines
 *  - Parallel line detection
 *  - Comparison operations with epsilon tolerance
 *
 *  ## Usage Example
 *
 *  @code{.cpp}
 *  #include <line.H>
 *  
 *  // Line through origin with slope 2
 *  LineEq line1(0.0, 2.0);  // y = 2x
 *
 *  // Line through two points
 *  LineEq line2(Point(0, 0), Point(1, 1));  // y = x
 *
 *  // Evaluate line at x = 5
 *  double y = line1(5.0);  // y = 10
 *
 *  // Find intersection
 *  auto [x, y] = line1.intersection(line2);
 *  
 *  // Check if parallel
 *  if (line1.is_parallel_to(line2))
 *    std::cout << "Lines are parallel\n";
 *  @endcode
 *
 *  ## Limitations
 *
 *  This representation cannot handle vertical lines (infinite slope).
 *  For vertical lines, consider using parametric or implicit form.
 *
 *  @see segment.H Finite line segments
 *  @see point.H Point representation
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */

#ifndef LINE_H
#define LINE_H

#include <cmath>
#include <utility>
#include <string>
#include <sstream>
#include <ah-errors.H>

namespace Aleph
{

/** @brief Default epsilon for floating-point comparisons.
 *  @ingroup Geometry
 */
inline constexpr double LINE_EPSILON = 1e-10;

/**
 * @brief 2D infinite line in slope-intercept form.
 *
 * Represents an infinite line using the equation y = y0 + m*x where:
 * - y0 is the y-intercept (value of y when x = 0)
 * - m is the slope (dy/dx)
 *
 * @note This representation cannot handle vertical lines (infinite slope).
 *       For vertical lines, use parametric or implicit form.
 *
 * @ingroup Geometry
 */
struct LineEq
{
  double y0 = 0;  ///< Y-intercept (where line crosses y-axis)
  double m = 1;   ///< Slope (dy/dx)

  /**
   * @brief Default constructor creates line y = x.
   *
   * The default line passes through origin with slope 1.
   */
  constexpr LineEq() noexcept = default;

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
  constexpr LineEq(double __y0, double __m) noexcept 
    : y0(__y0), m(__m) {}

  /**
   * @brief Construct line through a point with given slope.
   *
   * Computes y-intercept from point and slope using: y0 = y1 - m*x1
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
  constexpr LineEq(double x1, double y1, double __m) noexcept
    : y0(y1 - __m * x1), m(__m) {}

  /**
   * @brief Construct line through two points.
   *
   * Computes slope and y-intercept from the two points.
   * The slope is: m = (y2 - y1) / (x2 - x1)
   *
   * @param x1 X-coordinate of first point
   * @param y1 Y-coordinate of first point
   * @param x2 X-coordinate of second point
   * @param y2 Y-coordinate of second point
   *
   * @throws std::domain_error if x1 == x2 (vertical line, undefined slope)
   *
   * @note Horizontal lines (y1 == y2) are valid and result in slope = 0.
   *
   * @par Example
   * @code
   * LineEq line(0.0, 0.0, 2.0, 6.0);  // y = 3x
   * LineEq horizontal(0.0, 5.0, 10.0, 5.0);  // y = 5 (horizontal)
   * @endcode
   */
  LineEq(double x1, double y1, double x2, double y2)
  {
    ah_domain_error_if(std::abs(x1 - x2) < LINE_EPSILON)
      << "Cannot create line from points with same x-coordinate (vertical line): "
      << "x1 = " << x1 << ", x2 = " << x2;
    
    m = (y2 - y1) / (x2 - x1);
    y0 = y1 - m * x1;
  }

  /**
   * @brief Evaluate line at given x-coordinate.
   *
   * Computes y = y0 + m*x
   *
   * @param x X-coordinate to evaluate
   * @return Y-coordinate on the line
   *
   * @par Example
   * @code
   * LineEq line(1.0, 2.0);  // y = 1 + 2x
   * double y = line(3.0);   // y = 7
   * @endcode
   */
  [[nodiscard]] constexpr double operator()(double x) const noexcept 
  { 
    return y0 + m * x; 
  }

  /**
   * @brief Get the slope of the line.
   * @return The slope (m)
   */
  [[nodiscard]] constexpr double slope() const noexcept { return m; }

  /**
   * @brief Get the y-intercept of the line.
   * @return The y-intercept (y0)
   */
  [[nodiscard]] constexpr double y_intercept() const noexcept { return y0; }

  /**
   * @brief Check if the line is horizontal (slope = 0).
   * @param epsilon Tolerance for floating-point comparison
   * @return True if the line is horizontal
   */
  [[nodiscard]] constexpr bool is_horizontal(double epsilon = LINE_EPSILON) const noexcept
  {
    return std::abs(m) < epsilon;
  }

  /**
   * @brief Check if this line is parallel to another.
   *
   * Two lines are parallel if they have the same slope.
   *
   * @param l The other line
   * @param epsilon Tolerance for floating-point comparison
   * @return True if lines are parallel
   */
  [[nodiscard]] constexpr bool is_parallel_to(const LineEq & l, 
                                               double epsilon = LINE_EPSILON) const noexcept
  {
    return std::abs(m - l.m) < epsilon;
  }

  /**
   * @brief Check if this line is perpendicular to another.
   *
   * Two lines are perpendicular if m1 * m2 = -1
   *
   * @param l The other line
   * @param epsilon Tolerance for floating-point comparison
   * @return True if lines are perpendicular
   */
  [[nodiscard]] constexpr bool is_perpendicular_to(const LineEq & l,
                                                    double epsilon = LINE_EPSILON) const noexcept
  {
    return std::abs(m * l.m + 1.0) < epsilon;
  }

  /**
   * @brief Compute x-coordinate for a given y-coordinate.
   *
   * Solves x = (y - y0) / m
   *
   * @param y Y-coordinate
   * @return X-coordinate on the line
   * @throws std::domain_error if line is horizontal (undefined x)
   */
  [[nodiscard]] double x_at(double y) const
  {
    ah_domain_error_if(is_horizontal())
      << "Cannot compute x for horizontal line (slope = 0)";
    return (y - y0) / m;
  }

  /**
   * @brief Compute intersection point with another line.
   *
   * Solves the system of equations to find where lines cross:
   * - y = y0_1 + m_1 * x
   * - y = y0_2 + m_2 * x
   *
   * Solution: x = (y0_1 - y0_2) / (m_2 - m_1)
   *
   * @param l The other line
   * @param epsilon Tolerance for parallel line detection
   * @return Pair (x, y) of intersection coordinates
   * @throws std::domain_error if lines are parallel (no unique intersection)
   *
   * @par Example
   * @code
   * LineEq l1(0.0, 1.0);   // y = x
   * LineEq l2(2.0, -1.0);  // y = 2 - x
   * auto [x, y] = l1.intersection(l2);  // (1, 1)
   * @endcode
   */
  [[nodiscard]] std::pair<double, double> intersection(const LineEq & l,
                                                        double epsilon = LINE_EPSILON) const
  {
    ah_domain_error_if(is_parallel_to(l, epsilon))
      << "Cannot compute intersection of parallel lines: "
      << "slope1 = " << m << ", slope2 = " << l.m;

    double x = (y0 - l.y0) / (l.m - m);
    return {x, (*this)(x)};
  }

  /**
   * @brief Compute perpendicular line through a point.
   *
   * Returns the line perpendicular to this one that passes through
   * the given point. The perpendicular slope is -1/m.
   *
   * @param px X-coordinate of point
   * @param py Y-coordinate of point
   * @return Perpendicular line through (px, py)
   * @throws std::domain_error if this line is horizontal (perpendicular would be vertical)
   */
  [[nodiscard]] LineEq perpendicular_through(double px, double py) const
  {
    ah_domain_error_if(is_horizontal())
      << "Cannot compute perpendicular to horizontal line (would be vertical)";
    
    double perp_slope = -1.0 / m;
    return LineEq(px, py, perp_slope);
  }

  /**
   * @brief Compute distance from a point to this line.
   *
   * Uses the formula: d = |y_point - (y0 + m*x_point)| / sqrt(1 + m^2)
   *
   * @param px X-coordinate of point
   * @param py Y-coordinate of point
   * @return Perpendicular distance from point to line
   */
  [[nodiscard]] double distance_to(double px, double py) const noexcept
  {
    double diff = py - (y0 + m * px);
    return std::abs(diff) / std::sqrt(1.0 + m * m);
  }

  /**
   * @brief Check if a point lies on this line.
   *
   * @param px X-coordinate of point
   * @param py Y-coordinate of point
   * @param epsilon Tolerance for floating-point comparison
   * @return True if point is on the line
   */
  [[nodiscard]] bool contains_point(double px, double py, 
                                    double epsilon = LINE_EPSILON) const noexcept
  {
    return std::abs(py - (*this)(px)) < epsilon;
  }

  /**
   * @brief Test equality of two lines.
   *
   * Lines are equal if they have the same slope and y-intercept
   * (within epsilon tolerance).
   *
   * @param l Line to compare with
   * @return True if lines are identical
   */
  [[nodiscard]] bool operator==(const LineEq & l) const noexcept
  {
    return std::abs(y0 - l.y0) < LINE_EPSILON and 
           std::abs(m - l.m) < LINE_EPSILON;
  }

  /**
   * @brief Test inequality of two lines.
   *
   * @param l Line to compare with
   * @return True if lines differ
   */
  [[nodiscard]] bool operator!=(const LineEq & l) const noexcept
  {
    return not (*this == l);
  }

  /**
   * @brief Convert line to string representation.
   *
   * @return String in format "y = y0 + m * x"
   */
  [[nodiscard]] std::string to_string() const
  {
    std::ostringstream s;
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
  friend std::ostream & operator<<(std::ostream & out, const LineEq & l)
  {
    return out << l.to_string();
  }
};

} // namespace Aleph

// Global namespace compatibility
using Aleph::LineEq;
using Aleph::LINE_EPSILON;

#endif // LINE_H
