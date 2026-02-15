
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

# ifndef LINE_H
# define LINE_H

#include <cmath>
#include <utility>
#include <string>
#include <sstream>
#include <ah-errors.H>
#include <point.H>

namespace Aleph
{

/** @brief Default epsilon for geometric comparisons.
 *
 *  Zero for exact rational arithmetic. Can be overridden in method calls
 *  for approximate comparisons.
 *
 *  @ingroup Geometry
 */
inline const Geom_Number LINE_EPSILON(0);

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
  Geom_Number y0{0};  ///< Y-intercept (where line crosses y-axis)
  Geom_Number m{1};   ///< Slope (dy/dx)

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
   * LineEq line(Geom_Number(5), Geom_Number(2));  // y = 5 + 2x
   * @endcode
   */
  LineEq(const Geom_Number & __y0, const Geom_Number & __m) noexcept
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
   * LineEq line(Geom_Number(2), Geom_Number(4), Geom_Number(3));  // y = -2 + 3x
   * @endcode
   */
  LineEq(const Geom_Number & x1, const Geom_Number & y1, const Geom_Number & __m) noexcept
    : y0(y1 - __m * x1), m(__m) {}

  /**
   * @brief Construct a line through two points.
   *
   * Computes slope and y-intercept from the two points.
   * The slope is: m = (y2 - y1) / (x2 - x1)
   *
   * @param x1 X-coordinate of first point
   * @param y1 Y-coordinate of first point
   * @param x2 X-coordinate of second point
   * @param y2 Y-coordinate of second point
   *
   * @throws domain_error if x1 == x2 (vertical line, undefined slope)
   *
   * @note Horizontal lines (y1 == y2) are valid and result in slope = 0.
   *
   * @par Example
   * @code
   * LineEq line(Geom_Number(0), Geom_Number(0), Geom_Number(2), Geom_Number(6));  // y = 3x
   * LineEq horizontal(Geom_Number(0), Geom_Number(5), Geom_Number(10), Geom_Number(5));  // y = 5 (horizontal)
   * @endcode
   */
  LineEq(const Geom_Number & x1, const Geom_Number & y1,
         const Geom_Number & x2, const Geom_Number & y2)
  {
    ah_domain_error_if(x1 == x2)
      << "Cannot create line from points with same x-coordinate (vertical line): "
      << "x1 = " << x1 << ", x2 = " << x2;

    m = (y2 - y1) / (x2 - x1);
    y0 = y1 - m * x1;
  }

  /**
   * @brief Construct line through a Point with given slope.
   *
   * @param p Point on the line
   * @param __m Slope of the line
   *
   * @par Example
   * @code
   * Point p(Geom_Number(2), Geom_Number(4));
   * LineEq line(p, Geom_Number(3));  // Line through (2, 4) with slope 3
   * @endcode
   */
  LineEq(const Point & p, const Geom_Number & __m) noexcept
    : LineEq(p.get_x(), p.get_y(), __m) {}

  /**
   * @brief Construct line through two Points.
   *
   * @param p1 First point
   * @param p2 Second point
   * @throws domain_error if points have the same x-coordinate (vertical line)
   *
   * @par Example
   * @code
   * Point a(Geom_Number(0), Geom_Number(0));
   * Point b(Geom_Number(2), Geom_Number(6));
   * LineEq line(a, b);  // y = 3x
   * @endcode
   */
  LineEq(const Point & p1, const Point & p2)
    : LineEq(p1.get_x(), p1.get_y(), p2.get_x(), p2.get_y()) {}

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
   * LineEq line(Geom_Number(1), Geom_Number(2));  // y = 1 + 2x
   * Geom_Number y = line(Geom_Number(3));   // y = 7
   * @endcode
   */
  [[nodiscard]] Geom_Number operator()(const Geom_Number & x) const noexcept
  {
    return y0 + m * x;
  }

  /**
   * @brief Get the slope of the line.
   * @return The slope (m)
   */
  [[nodiscard]] const Geom_Number & slope() const noexcept { return m; }

  /**
   * @brief Get the y-intercept of the line.
   * @return The y-intercept (y0)
   */
  [[nodiscard]] const Geom_Number & y_intercept() const noexcept { return y0; }

  /**
   * @brief Check if the line is horizontal (slope = 0).
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if the line is horizontal
   */
  [[nodiscard]] bool is_horizontal(const Geom_Number & epsilon = LINE_EPSILON) const noexcept
  {
    return abs(m) <= epsilon;
  }

  /**
   * @brief Check if this line is parallel to another.
   *
   * Two lines are parallel if they have the same slope.
   *
   * @param l The other line
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if lines are parallel
   */
  [[nodiscard]] bool is_parallel_to(const LineEq & l,
                                     const Geom_Number & epsilon = LINE_EPSILON) const noexcept
  {
    return abs(m - l.m) <= epsilon;
  }

  /**
   * @brief Check if this line is perpendicular to another.
   *
   * Two lines are perpendicular if m1 * m2 = -1
   *
   * @param l The other line
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if lines are perpendicular
   */
  [[nodiscard]] bool is_perpendicular_to(const LineEq & l,
                                          const Geom_Number & epsilon = LINE_EPSILON) const noexcept
  {
    return abs(m * l.m + Geom_Number(1)) <= epsilon;
  }

  /**
   * @brief Compute x-coordinate for a given y-coordinate.
   *
   * Solves x = (y - y0) / m
   *
   * @param y Y-coordinate
   * @return X-coordinate on the line
   * @throws domain_error if line is horizontal (undefined x)
   */
  [[nodiscard]] Geom_Number x_at(const Geom_Number & y) const
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
   * @throws domain_error if lines are parallel (no unique intersection)
   *
   * @par Example
   * @code
   * LineEq l1(Geom_Number(0), Geom_Number(1));   // y = x
   * LineEq l2(Geom_Number(2), Geom_Number(-1));  // y = 2 - x
   * auto [x, y] = l1.intersection(l2);  // (1, 1)
   * @endcode
   */
  [[nodiscard]] std::pair<Geom_Number, Geom_Number> intersection(const LineEq & l,
                                                                  const Geom_Number & epsilon = LINE_EPSILON) const
  {
    ah_domain_error_if(is_parallel_to(l, epsilon))
      << "Cannot compute intersection of parallel lines: "
      << "slope1 = " << m << ", slope2 = " << l.m;

    Geom_Number x = (y0 - l.y0) / (l.m - m);
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
   * @throws domain_error if this line is horizontal (perpendicular would be vertical)
   */
  [[nodiscard]] LineEq perpendicular_through(const Geom_Number & px, const Geom_Number & py) const
  {
    ah_domain_error_if(is_horizontal())
      << "Cannot compute perpendicular to horizontal line (would be vertical)";

    const Geom_Number perp_slope = Geom_Number(-1) / m;
    return {px, py, perp_slope};
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
  [[nodiscard]] Geom_Number distance_to(const Geom_Number & px, const Geom_Number & py) const noexcept
  {
    const Geom_Number diff = py - (y0 + m * px);
    // Use mpfr for sqrt, then convert back to rational
    const mpfr_class denominator = sqrt(mpfr_class(1) + mpfr_class(m) * mpfr_class(m));
    return abs(diff) / Geom_Number(denominator);
  }

  /**
   * @brief Check if a point lies on this line.
   *
   * @param px X-coordinate of point
   * @param py Y-coordinate of point
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if point is on the line
   */
  [[nodiscard]] bool contains_point(const Geom_Number & px, const Geom_Number & py,
                                     const Geom_Number & epsilon = LINE_EPSILON) const noexcept
  {
    return abs(py - (*this)(px)) <= epsilon;
  }

  /**
   * @brief Compute distance from a Point to this line.
   *
   * @param p The point
   * @return Perpendicular distance from point to line
   */
  [[nodiscard]] Geom_Number distance_to(const Point & p) const noexcept
  {
    return distance_to(p.get_x(), p.get_y());
  }

  /**
   * @brief Compute a perpendicular line through a Point.
   *
   * @param p Point on the perpendicular line
   * @return Perpendicular line through p
   * @throws domain_error if this line is horizontal (perpendicular would be vertical)
   */
  [[nodiscard]] LineEq perpendicular_through(const Point & p) const
  {
    return perpendicular_through(p.get_x(), p.get_y());
  }

  /**
   * @brief Check if a Point lies on this line.
   *
   * @param p The point to test
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if point is on the line
   */
  [[nodiscard]] bool contains_point(const Point & p,
                                     const Geom_Number & epsilon = LINE_EPSILON) const noexcept
  {
    return contains_point(p.get_x(), p.get_y(), epsilon);
  }

  /**
   * @brief Compute the intersection Point with another line.
   *
   * @param l The other line
   * @param epsilon Tolerance for parallel line detection
   * @return Intersection point
   * @throws domain_error if lines are parallel (no unique intersection)
   *
   * @par Example
   * @code
   * LineEq l1(Geom_Number(0), Geom_Number(1));   // y = x
   * LineEq l2(Geom_Number(2), Geom_Number(-1));  // y = 2 - x
   * Point p = l1.intersection_point(l2);  // Point(1, 1)
   * @endcode
   */
  [[nodiscard]] Point intersection_point(const LineEq & l,
                                          const Geom_Number & epsilon = LINE_EPSILON) const
  {
    auto [x, y] = intersection(l, epsilon);
    return {x, y};
  }

  /**
   * @brief Test equality of two lines.
   *
   * Lines are equal if they have the same slope and y-intercept
   * (within epsilon tolerance).
   *
   * @param l Line to compare with
   * @param epsilon Tolerance for comparison (default: exact equality)
   * @return True if lines are identical
   */
  [[nodiscard]] bool operator==(const LineEq & l) const noexcept
  {
    return y0 == l.y0 and m == l.m;
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
   * @brief Test approximate equality of two lines.
   *
   * Lines are approximately equal if they have the same slope and y-intercept
   * within epsilon tolerance.
   *
   * @param l Line to compare with
   * @param epsilon Tolerance for comparison
   * @return True if lines are approximately equal
   */
  [[nodiscard]] bool approx_equal(const LineEq & l, const Geom_Number & epsilon) const noexcept
  {
    return abs(y0 - l.y0) <= epsilon and abs(m - l.m) <= epsilon;
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

# endif // LINE_H
