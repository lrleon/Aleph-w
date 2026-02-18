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


/** @file point.H
 *  @brief 2D point and geometric utilities.
 *
 *  This file provides classes for 2D points and geometric operations including
 *  distance calculations, vector operations, and coordinate transformations.
 *  Used by graph visualization and geometric algorithms.
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */

# ifndef POINT_H
# define POINT_H

# include <cmath>

# include <cstddef>
# include <limits>
# include <array>
# include <functional>

# include <iomanip>
# include <string>

# include <ahAssert.H>
# include <ahUtils.H>
# include <ah-errors.H>
# include <utility>

# include <gmpfrxx.h>

namespace Aleph
{
  /** @brief Numeric type used by the geometry module.
   *
   * Currently, this is an arbitrary precision rational (@c mpq_class). All helper
   * routines convert to/from @c mpfr_class internally when transcendentals are
   * required.
   */
  using Geom_Number = mpq_class;

  inline double geom_number_to_double(const Geom_Number & n)
  {
    return n.get_d();
  }

  inline std::ostream &operator <<(std::ostream & o, const Geom_Number & n)
  {
    o << n.get_d();
    return o;
  }

  // TODO: provide helpers for rotating special figures (e.g., ellipses) by
  // rotating the Cartesian axes instead of recomputing points from scratch.


  // Legacy double-precision constants kept for backward compatibility.
  constexpr double PI = 3.1415926535897932384626433832795028841971693993751;
  constexpr double PI_2 = PI / 2.0;
  constexpr double PI_4 = PI / 4.0;

  // High-precision pi value for computations that require Geom_Number.
  inline const Geom_Number &geom_pi()
  {
    static const Geom_Number pi = acos(mpfr_class(-1));
    return pi;
  }


  class Point;
  class Polar_Point;
  class Segment;
  class Triangle;
  class Ellipse;


  /// Compute the area of parallelogram defined by vectors a->b and b->c
  inline Geom_Number
  area_of_parallelogram(const Point & a, const Point & b, const Point & c);


  /// \brief Euclidean distance (hypotenuse) of the vector (x, y).
  inline Geom_Number euclidean_distance(const Geom_Number & x,
                                        const Geom_Number & y)
  {
    return hypot(mpfr_class(x), mpfr_class(y));
  }

  /// @deprecated Use euclidean_distance() instead.
  [[deprecated("Use euclidean_distance() instead")]]
  inline Geom_Number pitag(const Geom_Number & x, const Geom_Number & y)
  {
    return euclidean_distance(x, y);
  }

  /// \brief Arc tangent of @p m (wrapper over mpfr).
  inline Geom_Number arctan(const Geom_Number & m)
  {
    return atan(mpfr_class(m));
  }

  /// \brief Two-argument arc tangent (wrapper over mpfr).
  inline Geom_Number arctan2(const Geom_Number & m, const Geom_Number & n)
  {
    return atan2(mpfr_class(m), mpfr_class(n));
  }

  /// \brief Sine of @p x (wrapper over mpfr).
  inline Geom_Number sinus(const Geom_Number & x)
  {
    return sin(mpfr_class(x));
  }

  /// \brief Cosine of @p x (wrapper over mpfr).
  inline Geom_Number cosinus(const Geom_Number & x)
  {
    return cos(mpfr_class(x));
  }

  /// \brief Square root of @p x (wrapper over mpfr).
  inline Geom_Number square_root(const Geom_Number & x)
  {
    return sqrt(mpfr_class(x));
  }

  struct Geom_Object
  {
    Geom_Object() = default;

    virtual ~Geom_Object() = default;
  };

  /**
  * @brief Represents a point with rectangular coordinates in a 2D plane.
  * @ingroup Geometry
  *
  * Fundamental class defining a point in a Cartesian coordinate plane.
  * It provides vector operations, transformations, and geometric predicates.
  * Uses the `Geom_Number` type for exact-precision arithmetic.
  */
  class Point : public Geom_Object
  {
    friend class Segment;
    friend class Triangle;
    friend class Polar_Point;

    Geom_Number x_;
    Geom_Number y_;

  public:
    /**
     * @brief Default constructor.
     * @note Initializes the point at the origin (0, 0).
     */
    Point() : Geom_Object(), x_(0), y_(0)
    { /* empty */
    }

    /**
     * @brief Constructs a point from Cartesian coordinates.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     */
    Point(const Geom_Number & x, const Geom_Number & y)
      : Geom_Object(), x_(x), y_(y)
    {
      // empty
    }

    /**
     * @brief Constructs a point from polar coordinates.
     * @param pp The point in polar coordinates (`Polar_Point`) to be converted.
     */
    inline Point(const Polar_Point & pp);

    /**
     * @brief Checks for exact equality between two points.
     * @param point The other point to compare against.
     * @return `true` if both x and y coordinates are identical, `false` otherwise.
     */
    [[nodiscard]] bool operator ==(const Point & point) const noexcept
    {
      return x_ == point.x_ and y_ == point.y_;
    }

    /**
     * @brief Checks for inequality between two points.
     * @param point The other point to compare against.
     * @return `true` if either the x or y coordinate is different.
     */
    [[nodiscard]] bool operator !=(const Point & point) const noexcept
    {
      return not (*this == point);
    }

    /**
     * @brief Defines a strict lexicographical ordering for points.
     * @param point The other point to compare against.
     * @return `true` if this point is less than `point` (first by x, then by y).
     */
    [[nodiscard]] bool operator <(const Point & point) const noexcept
    {
      return x_ < point.x_ or (x_ == point.x_ and y_ < point.y_);
    }

    /**
     * @brief Vector addition.
     * @param p The other point (vector) to add.
     * @return A new point representing the sum of the corresponding coordinates.
     */
    [[nodiscard]] Point operator +(const Point & p) const
    {
      return {x_ + p.x_, y_ + p.y_};
    }

    /**
     * @brief Vector addition and assignment.
     * @param p The other point (vector) to add.
     * @return A reference to this modified point.
     */
    Point &operator +=(const Point & p)
    {
      x_ += p.x_;
      y_ += p.y_;

      return *this;
    }

    /**
     * @brief Vector subtraction.
     * @param p The point (vector) to subtract.
     * @return A new point representing the difference of the coordinates.
     */
    Point operator -(const Point & p) const
    {
      return {x_ - p.x_, y_ - p.y_};
    }

    /**
     * @brief Vector subtraction and assignment.
     * @param p The point (vector) to subtract.
     * @return A reference to this modified point.
     */
    Point &operator -=(const Point & p)
    {
      x_ -= p.x_;
      y_ -= p.y_;

      return *this;
    }

    /**
     * @brief Unary negation (vector inversion).
     * @return A new point with both coordinates negated.
     */
    [[nodiscard]] Point operator -() const
    {
      return {-x_, -y_};
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar value to multiply by.
     * @return A new point with each coordinate multiplied by `s`.
     */
    [[nodiscard]] Point operator *(const Geom_Number & s) const
    {
      return {x_ * s, y_ * s};
    }

    /**
     * @brief Scalar division.
     * @param s The scalar value to divide by.
     * @return A new point with each coordinate divided by `s`.
     */
    [[nodiscard]] Point operator /(const Geom_Number & s) const
    {
      return {x_ / s, y_ / s};
    }

    /**
     * @brief Dot product.
     * @param p The other vector (point).
     * @return The scalar result of the dot product (`x1*x2 + y1*y2`).
     */
    [[nodiscard]] Geom_Number dot(const Point & p) const
    {
      return x_ * p.x_ + y_ * p.y_;
    }

    /**
     * @brief 2D cross-product (z-component of the 3D cross-product).
     * @param p The other vector (point).
     * @return The scalar `x1*y2 - y1*x2`. Its sign indicates orientation.
     */
    [[nodiscard]] Geom_Number cross(const Point & p) const
    {
      return x_ * p.y_ - y_ * p.x_;
    }

    /**
     * @brief Squared Euclidean norm.
     * @return The sum of the squares of the coordinates (`x*x + y*y`).
     * @note This is more efficient than `norm()` if only needed for magnitude comparisons.
     */
    [[nodiscard]] Geom_Number norm_squared() const
    {
      return x_ * x_ + y_ * y_;
    }

    /**
     * @brief Euclidean norm (vector magnitude).
     * @return The square root of the sum of the squares of the coordinates.
     */
    [[nodiscard]] Geom_Number norm() const
    {
      return euclidean_distance(x_, y_);
    }

    /**
     * @brief Returns a normalized copy of this vector (magnitude 1).
     * @return A new `Point` representing the unit vector.
     * @throws std::domain_error if normalizing the zero vector is attempted.
     */
    [[nodiscard]] Point normalize() const
    {
      const Geom_Number n = norm();
      ah_domain_error_if(n == 0) << "Cannot normalize the zero vector";
      return *this / n;
    }

    /**
     * @brief Rotates the point around the origin by a given angle.
     * @param angle The rotation angle in radians.
     * @return A new `Point` with the rotated coordinates.
     */
    [[nodiscard]] Point rotate(const Geom_Number & angle) const
    {
      const Geom_Number c = cosinus(angle);
      const Geom_Number s = sinus(angle);
      return {x_ * c - y_ * s, x_ * s + y_ * c};
    }

    /**
     * @brief Linear interpolation between this point and another.
     * @param other The other point for interpolation.
     * @param t The interpolation parameter, typically in [0, 1].
     * @return `this * (1-t) + other * t`.
     */
    [[nodiscard]] Point lerp(const Point & other, const Geom_Number & t) const
    {
      const Geom_Number one_minus_t = Geom_Number(1) - t;
      return *this * one_minus_t + other * t;
    }

    /**
     * @brief Calculates the midpoint between this point and another.
     * @param other The other point.
     * @return The midpoint, equivalent to `lerp(other, 0.5)`.
     */
    [[nodiscard]] Point midpoint(const Point & other) const
    {
      return lerp(other, Geom_Number(1, 2));
    }

    /**
     * @brief Gets the x-coordinate value.
     * @return A const reference to the x-coordinate.
     */
    [[nodiscard]] const Geom_Number &get_x() const noexcept
    {
      return x_;
    }

    /**
     * @brief Gets the y-coordinate value.
     * @return A const reference to the y-coordinate.
     */
    [[nodiscard]] const Geom_Number &get_y() const noexcept
    {
      return y_;
    }

    /**
     * @brief Checks if this point is collinear with two other points.
     * @param p1 The first point.
     * @param p2 The second point.
     * @return `true` if all three points lie on the same line.
     */
    [[nodiscard]] bool is_colinear_with(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(*this, p1, p2) == 0;
    }

    /**
     * @brief Checks if this point is collinear with a segment.
     * @param s The segment.
     * @return `true` if the point lies on the infinite line defined by the segment.
     */
    [[nodiscard]] inline bool is_colinear_with(const Segment & s) const;

    /**
     * @brief Checks if this point is to the left of the directed line from p1 to p2.
     * @param p1 The starting point of the line.
     * @param p2 The ending point of the line.
     * @return `true` if the point is to the left.
     */
    [[nodiscard]] bool is_left_of(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(p1, p2, *this) > 0;
    }

    /// @deprecated Use is_left_of() instead.
    [[deprecated("Use is_left_of() instead")]]
    [[nodiscard]] bool is_to_left_from(const Point & p1, const Point & p2) const
    {
      return is_left_of(p1, p2);
    }

    /**
     * @brief Checks if this point is to the right of the directed line from p1 to p2.
     * @param p1 The starting point of the line.
     * @param p2 The ending point of the line.
     * @return `true` if the point is to the right.
     */
    [[nodiscard]] bool is_to_right_from(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(p1, p2, *this) < 0;
    }

    /**
     * @brief Checks if this point is to the left of or on the line from p1 to p2.
     * @param p1 The starting point of the line.
     * @param p2 The ending point of the line.
     * @return `true` if the point is not to the right.
     */
    [[nodiscard]] bool is_to_left_on_from(const Point & p1, const Point & p2) const
    {
      return not is_to_right_from(p1, p2);
    }

    /**
     * @brief Checks if this point is to the right of or on the line from p1 to p2.
     * @param p1 The starting point of the line.
     * @param p2 The ending point of the line.
     * @return `true` if the point is not to the left.
     */
    [[nodiscard]] bool is_right_on_of(const Point & p1, const Point & p2) const
    {
      return not is_left_of(p1, p2);
    }

    /// @deprecated Use is_right_on_of() instead.
    [[deprecated("Use is_right_on_of() instead")]]
    [[nodiscard]] bool is_to_right_on_from(const Point & p1, const Point & p2) const
    {
      return is_right_on_of(p1, p2);
    }

    /**
     * @brief Determines if the sequence of three points (this, p1, p2) makes a clockwise turn.
     * @param p1 The second point in the sequence.
     * @param p2 The third point in the sequence.
     * @return `true` if the orientation is clockwise.
     */
    [[nodiscard]] bool is_clockwise_with(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(*this, p1, p2) < 0;
    }

    /**
     * @brief Checks if this point is to the left of a directed segment.
     * @param s The segment.
     * @return `true` if the point is to the left.
     */
    [[nodiscard]] inline bool is_left_of(const Segment & s) const;

    /**
     * @brief Checks if this point is to the right of a directed segment.
     * @param s The segment.
     * @return `true` if the point is to the right.
     */
    [[nodiscard]] inline bool is_right_of(const Segment & s) const;

    /// @deprecated Use is_left_of() instead.
    [[deprecated("Use is_left_of() instead")]]
    [[nodiscard]] bool is_to_left_from(const Segment & s) const
    {
      return is_left_of(s);
    }

    /// @deprecated Use is_right_of() instead.
    [[deprecated("Use is_right_of() instead")]]
    [[nodiscard]] bool is_to_right_from(const Segment & s) const
    {
      return is_right_of(s);
    }

    /**
     * @brief Determines if the sequence (this, s.src, s.tgt) makes a clockwise turn.
     * @param s The segment.
     * @return `true` if the orientation is clockwise.
     */
    [[nodiscard]] inline bool is_clockwise_with(const Segment & s) const;

    /**
     * @brief Checks if this point is on the bounding box of p1 and p2 and is collinear with them.
     * @param p1 First point of the segment.
     * @param p2 Second point of the segment.
     * @return `true` if this point lies on the segment joining p1 and p2.
     */
    [[nodiscard]] bool is_between(const Point & p1, const Point & p2) const
    {
      if (not this->is_colinear_with(p1, p2))
        return false;

      if (p1.get_x() == p2.get_x())
        return (p1.get_y() <= this->get_y() and this->get_y() <= p2.get_y())
               or (p1.get_y() >= this->get_y() and (this->get_y() >= p2.get_y()));

      return (p1.get_x() <= this->get_x() and (this->get_x() <= p2.get_x()))
             or (p1.get_x() >= this->get_x() and (this->get_x() >= p2.get_x()));
    }

    /**
     * @brief Returns which of the two points, p1 or p2, is nearer to this point.
     * @param p1 The first candidate point.
     * @param p2 The second candidate point.
     * @return A const reference to the nearest point (`p1` or `p2`).
     */
    [[nodiscard]] const Point &nearest_point(const Point & p1, const Point & p2) const
    {
      return this->distance_squared_to(p1) < this->distance_squared_to(p2) ? p1 : p2;
    }

    /**
     * @brief Checks if this point is contained within a segment.
     * @param s The segment to check against.
     * @return `true` if the point is on the segment.
     * @see Segment::contains()
     */
    [[nodiscard]] inline bool is_inside(const Segment & s) const;

    /**
     * @brief Checks if this point is contained within an ellipse.
     * @param e The ellipse to check against.
     * @return `true` if the point is inside or on the boundary of the ellipse.
     * @see Ellipse::contains()
     */
    [[nodiscard]] inline bool is_inside(const Ellipse & e) const;

    /**
     * @brief Checks if this point lies exactly on the boundary of an ellipse.
     * @param e The ellipse to check against.
     * @return `true` if the point is on the boundary.
     * @see Ellipse::intersects_with()
     */
    [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

    /**
     * @brief Returns a string representation of the point as "(x,y)".
     * @return A `std::string` representing the point.
     */
    [[nodiscard]] std::string to_string() const
    {
      return "(" + std::to_string(geom_number_to_double(x_)) + "," +
             std::to_string(geom_number_to_double(y_)) + ")";
    }

    /**
     * @brief Cast operator to `std::string`.
     * @return The result of `to_string()`.
     */
    operator std::string() const { return to_string(); }

    /**
     * @brief Calculates the squared Euclidean distance to another point.
     * @param that The other point.
     * @return The squared distance.
     */
    [[nodiscard]] inline Geom_Number distance_squared_to(const Point & that) const;

    /**
     * @brief Calculates the Euclidean distance to another point.
     * @param p The other point.
     * @return The Euclidean distance.
     */
    [[nodiscard]] inline Geom_Number distance_to(const Point & p) const;

    /// @deprecated Use distance_to() instead.
    [[deprecated("Use distance_to() instead")]]
    [[nodiscard]] Geom_Number distance_with(const Point & p) const
    {
      return distance_to(p);
    }

    /**
     * @brief Returns the highest point (largest y-coordinate). For a `Point`, it's itself.
     * @return A const reference to itself.
     */
    [[nodiscard]] const Point &highest_point() const { return *this; }

    /**
     * @brief Returns the lowest point (smallest y-coordinate). For a `Point`, it's itself.
     * @return A const reference to itself.
     */
    [[nodiscard]] const Point &lowest_point() const { return *this; }

    /**
     * @brief Returns the leftmost point (smallest x-coordinate). For a `Point`, it's itself.
     * @return A const reference to itself.
     */
    [[nodiscard]] const Point &leftmost_point() const { return *this; }

    /**
     * @brief Returns the rightmost point (largest x-coordinate). For a `Point`, it's itself.
     * @return A const reference to itself.
     */
    [[nodiscard]] const Point &rightmost_point() const { return *this; }
  };

  /// Scalar multiplication with commutative order.
  [[nodiscard]] inline Point operator*(const Geom_Number & s, const Point & p)
  {
    return p * s;
  }


  /** @brief Polar representation of a 2D point.
   *
   * @ingroup Geometry
   * @author Leandro Rabindranath León
   *
   * Polar coordinates are convenient for algorithms that rely on angles or
   * rotations. Instances can be constructed from Cartesian coordinates and are
   * convertible back into @ref Point objects.
   */
  class Polar_Point : public Geom_Object
  {
    friend class Point;

    Geom_Number r_ = 0; // distance from origin
    Geom_Number theta_ = 0; // angle in radians from positive x-axis

  public:
    /**
     * @brief Gets the radius (magnitude) of the polar point.
     * @return A const reference to the radius `r`.
     */
    [[nodiscard]] const Geom_Number &get_r() const { return r_; }

    /**
     * @brief Gets the angle (theta) of the polar point.
     * @return A const reference to the angle `theta` in radians.
     */
    [[nodiscard]] const Geom_Number &get_theta() const { return theta_; }

    /**
     * @brief Constructs a polar point from a radius and an angle.
     * @param r The distance from the origin (radius).
     * @param theta The angle in radians from the positive x-axis.
     */
    Polar_Point(const Geom_Number & r, const Geom_Number & theta)
      : r_(r), theta_(theta)
    {
      // empty
    }

    /**
     * @brief Constructs a polar point from a Cartesian point.
     * @param p The Cartesian point to convert. The origin is assumed to be (0,0).
     */
    explicit Polar_Point(const Point & p)
    {
      const Geom_Number & x = p.get_x();
      const Geom_Number & y = p.get_y();

      r_ = euclidean_distance(x, y);
      theta_ = arctan2(y, x);
    }

    /// @brief Enumerates polar quadrants in counterclockwise order.
    enum Quadrant { First, Second, Third, Fourth };

    /**
     * @brief Returns the quadrant where the point lies.
     * @note The origin (0,0) is considered to be in the First quadrant.
     * @return The `Quadrant` enum value.
     */
    [[nodiscard]] Quadrant get_quadrant() const
    {
      const Point cartesian(*this);
      const bool east = cartesian.get_x() >= 0;
      const bool north = cartesian.get_y() >= 0;

      if (east and north)
        return First;
      if (not east and north)
        return Second;
      if (not east and not north)
        return Third;
      return Fourth;
    }

    /**
     * @brief Converts the polar point to a string representation "[r,theta]".
     * @return A `std::string` representing the polar coordinates.
     */
    [[nodiscard]] std::string to_string() const
    {
      return "[" + std::to_string(geom_number_to_double(r_)) + "," +
             std::to_string(geom_number_to_double(theta_)) + "]";
    }

    /**
     * @brief Default constructor.
     * @note Initializes a polar point at the origin (r=0, theta=0).
     */
    Polar_Point() = default;
  };


  inline
  Point::Point(const Polar_Point & pp)
    : x_(pp.r_ * cosinus(pp.theta_)), y_(pp.r_ * sinus(pp.theta_))
  {
    // empty
  }


  /**
   * @brief Represents a line segment between two points.
   * @ingroup Geometry
   *
   * Represents an (undirected) segment in the plane, stored as source and
   * target endpoints. Provides methods for intersection, projection, distance
   * calculations, and other geometric queries.
   */
  class Segment : public Geom_Object
  {
    friend class Point;
    friend class Triangle;

    Point src_, tgt_;

    /**
     * @brief Internal helper to compute the slope as a double.
     * @return The slope of the segment. Returns +/- infinity for vertical segments.
     */
    [[nodiscard]] double compute_slope() const
    {
      if (tgt_.get_x() == src_.get_x())
        {
          if (src_.get_y() < tgt_.get_y())
            return std::numeric_limits<double>::max();
          return -std::numeric_limits<double>::max();
        }

      const Geom_Number slope_ = (tgt_.get_y() - src_.get_y()) / (tgt_.get_x() - src_.get_x());

      return slope_.get_d();
    }

  public:
    /**
     * @brief Checks for equality between two segments.
     * @note Segments are considered equal if their endpoints are the same,
     *       regardless of direction (src/tgt).
     * @param s The other segment to compare.
     * @return `true` if segments are equal, `false` otherwise.
     */
    [[nodiscard]] bool operator ==(const Segment & s) const noexcept
    {
      return (src_ == s.src_ and tgt_ == s.tgt_) or
             (src_ == s.tgt_ and tgt_ == s.src_);
    }

    /**
     * @brief Checks for inequality between two segments.
     * @param s The other segment to compare.
     * @return `true` if segments are not equal.
     */
    [[nodiscard]] bool operator !=(const Segment & s) const noexcept
    {
      return not (*this == s);
    }

    /**
     * @brief Gets the endpoint with the largest y-coordinate.
     * @return A const reference to the highest point.
     */
    [[nodiscard]] const Point &highest_point() const noexcept
    {
      return src_.get_y() > tgt_.get_y() ? src_ : tgt_;
    }

    /**
     * @brief Gets the endpoint with the smallest y-coordinate.
     * @return A const reference to the lowest point.
     */
    [[nodiscard]] const Point &lowest_point() const noexcept
    {
      return src_.get_y() < tgt_.get_y() ? src_ : tgt_;
    }

    /**
     * @brief Gets the endpoint with the smallest x-coordinate.
     * @return A const reference to the leftmost point.
     */
    [[nodiscard]] const Point &leftmost_point() const noexcept
    {
      return src_.get_x() < tgt_.get_x() ? src_ : tgt_;
    }

    /**
     * @brief Gets the endpoint with the largest x-coordinate.
     * @return A const reference to the rightmost point.
     */
    [[nodiscard]] const Point &rightmost_point() const noexcept
    {
      return src_.get_x() > tgt_.get_x() ? src_ : tgt_;
    }

    /**
     * @brief Gets the source point of the segment.
     * @return A const reference to the source point.
     */
    [[nodiscard]] const Point &get_src_point() const noexcept { return src_; }

    /**
     * @brief Gets the target point of the segment.
     * @return A const reference to the target point.
     */
    [[nodiscard]] const Point &get_tgt_point() const noexcept { return tgt_; }

    /**
     * @brief Default constructor. Initializes a zero-length segment at the origin.
     */
    Segment() = default;

    /**
     * @brief Constructs a segment from two endpoints.
     * @param src The source point.
     * @param tgt The target point.
     */
    Segment(Point src, Point tgt)
      : Geom_Object(), src_(std::move(src)), tgt_(std::move(tgt))
    {
      // empty
    }

  private:
    /**
     * @brief Computes the target point of a segment given a source, slope, and length.
     * @param src The origin point.
     * @param m The slope.
     * @param d The length of the segment.
     * @return The calculated target point, placed to the east (right).
     */
    static
    Point compute_tgt_point(const Point & src, // Point of origin
                            const Geom_Number & m, // slope
                            const Geom_Number & d) // segment length
    {
      const Geom_Number den2 = 1 + m * m;

      const Geom_Number den = square_root(den2);

      const Geom_Number x = src.get_x() + d / den;

      const Geom_Number y = src.get_y() + d * m / den;

      return {x, y};
    }

  public:
    /**
     * @brief Constructs a new segment from a source point, slope, and length.
     * @details It computes the target point by solving the system of equations
     *          for a line and distance.
     * @param src The source point.
     * @param m The slope (rise over run).
     * @param l The length of the segment.
     */
    Segment(Point src, const Geom_Number & m, const Geom_Number & l)
      : Geom_Object(), src_(std::move(src)), tgt_(compute_tgt_point(src_, m, l))
    {
      // empty
    }

    /**
     * @brief Constructs a new segment parallel to a given segment at a specified distance.
     * @param sg The reference segment.
     * @param dist The perpendicular distance to the new segment.
     */
    Segment(const Segment & sg, const Geom_Number & dist)
    {
      const Segment perp = sg.mid_perpendicular(dist);

      const Point mid_point = sg.mid_point();

      const Point diff_point = mid_point - perp.get_src_point();

      src_ = sg.get_src_point() + diff_point;
      tgt_ = sg.get_tgt_point() + diff_point;
    }

    /**
     * @brief Returns the slope of the segment.
     * @return The slope as a `double`. Returns +/- infinity for vertical lines.
     */
    [[nodiscard]] double slope() const
    {
      return compute_slope();
    }

    /**
     * @brief Returns the exact slope of the segment as a `Geom_Number`.
     * @return The exact rational slope.
     * @throws std::domain_error if the segment is vertical.
     */
    [[nodiscard]] Geom_Number slope_exact() const
    {
      ah_domain_error_if(tgt_.get_x() == src_.get_x()) << "Vertical segment has undefined slope";
      return (tgt_.get_y() - src_.get_y()) / (tgt_.get_x() - src_.get_x());
    }

    /**
     * @brief Computes the counter-clockwise rotation angle FROM this segment TO another.
     *
     * Returns the angle (in radians) you would rotate this segment counter-clockwise
     * to align it with segment @p s. For example, if this segment points along +X
     * and @p s points along +Y, the result is π/2 (90°).
     *
     * @param s The target segment.
     * @return The angle in radians, in the range [0, 2*PI).
     */
    [[nodiscard]] double counterclockwise_angle_with(const Segment & s) const
    {
      const Geom_Number x1 = tgt_.get_x() - src_.get_x();
      const Geom_Number x2 = s.tgt_.get_x() - s.src_.get_x();
      const Geom_Number y1 = tgt_.get_y() - src_.get_y();
      const Geom_Number y2 = s.tgt_.get_y() - s.src_.get_y();
      const Geom_Number dot = x1 * x2 + y1 * y2;
      const Geom_Number det = x1 * y2 - y1 * x2;

      const Geom_Number angle = arctan2(det, dot);

      if (angle < 0)
        return geom_number_to_double(angle + 2 * geom_pi());

      return angle.get_d();
    }

    /**
     * @brief Computes the counter-clockwise angle of this segment with respect to the x-axis.
     * @return The angle in radians, in the range [0, 2*PI).
     */
    [[nodiscard]] double counterclockwise_angle() const
    {
      const Segment x_axis(Point(0, 0), Point(1, 0));
      return x_axis.counterclockwise_angle_with(*this);
    }

    /**
     * @brief Returns the Euclidean length of the segment.
     * @return The distance between the segment's endpoints.
     */
    [[nodiscard]] Geom_Number length() const
    {
      return euclidean_distance(tgt_.get_x() - src_.get_x(), tgt_.get_y() - src_.get_y());
    }

    /// @deprecated Use length() instead.
    [[deprecated("Use length() instead")]]
    [[nodiscard]] Geom_Number size() const { return length(); }

    /**
     * @brief Checks if a point is collinear with this segment.
     * @param p The point to check.
     * @return `true` if the point lies on the infinite line defined by this segment.
     */
    [[nodiscard]] bool is_colinear_with(const Point & p) const
    {
      return p.is_colinear_with(src_, tgt_);
    }

    /**
     * @brief Checks if this segment is to the left of a given point.
     * @param p The point.
     * @return `true` if the point is to the right of this directed segment.
     * @see Point::is_right_of()
     */
    [[nodiscard]] bool is_left_of(const Point & p) const
    {
      return p.is_right_of(*this);
    }

    /**
     * @brief Checks if this segment is to the right of a given point.
     * @param p The point.
     * @return `true` if the point is to the left of this directed segment.
     * @see Point::is_left_of()
     */
    [[nodiscard]] bool is_right_of(const Point & p) const
    {
      return p.is_left_of(*this);
    }

    /// @deprecated Use is_left_of() instead.
    [[deprecated("Use is_left_of() instead")]]
    [[nodiscard]] bool is_to_left_from(const Point & p) const
    {
      return is_left_of(p);
    }

    /// @deprecated Use is_right_of() instead.
    [[deprecated("Use is_right_of() instead")]]
    [[nodiscard]] bool is_to_right_from(const Point & p) const
    {
      return is_right_of(p);
    }

    /**
     * @brief Returns the midpoint of this segment.
     * @return A `Point` representing the midpoint.
     */
    [[nodiscard]] Point mid_point() const
    {
      const Geom_Number x = (src_.get_x() + tgt_.get_x()) / 2;
      const Geom_Number y = (src_.get_y() + tgt_.get_y()) / 2;

      return {x, y};
    }

    /**
     * @brief Returns a new segment with the endpoints swapped.
     * @return A `Segment` with source and target points reversed.
     */
    [[nodiscard]] Segment reversed() const
    {
      return {tgt_, src_};
    }

    /**
     * @brief Evaluates a point on the segment via linear interpolation.
     * @param t The interpolation parameter. t=0 returns the source, t=1 returns the target.
     * @return The `Point` at parameter `t`.
     */
    [[nodiscard]] Point at(const Geom_Number & t) const
    {
      return src_.lerp(tgt_, t);
    }

    /**
     * @brief Orthogonal projection of a point onto this segment's infinite line, clamped to the segment's endpoints.
     * @param p The point to project.
     * @return The closest point on the segment to `p`.
     */
    [[nodiscard]] Point project(const Point & p) const
    {
      const Point dir = tgt_ - src_;
      const Geom_Number len2 = dir.dot(dir);

      if (len2 == 0)
        return src_;

      Geom_Number t = (p - src_).dot(dir) / len2;
      if (t < 0)
        t = 0;
      else if (t > 1)
        t = 1;

      return at(t);
    }

    /**
     * @brief Calculates the Euclidean distance from a point to this segment.
     * @param p The point.
     * @return The shortest distance from `p` to any point on the segment.
     */
    [[nodiscard]] Geom_Number distance_to(const Point & p) const
    {
      return p.distance_to(project(p));
    }


    /**
     * @brief Returns whichever endpoint of this segment is nearer to a given point.
     * @param p The reference point.
     * @return A const reference to the nearest endpoint (`src_` or `tgt_`).
     */
    [[nodiscard]] const Point &nearest_point(const Point & p) const
    {
      return p.nearest_point(get_src_point(), get_tgt_point());
    }

    /**
     * @brief Constructs a segment perpendicular to `this` that passes through point `p`.
     * @param p The point through which the perpendicular segment will pass.
     * @return A new `Segment` starting at the projected point on this segment's line and ending at `p`.
     */
    [[nodiscard]] Segment get_perpendicular(const Point & p) const
    {
      if (src_ == tgt_)
        return *this;

      const Point dir = tgt_ - src_;
      const Geom_Number len2 = dir.dot(dir);
      ah_domain_error_if(len2 == 0) << "Cannot get perpendicular of zero-length segment";

      const Geom_Number t = (p - src_).dot(dir) / len2;
      const Point foot = src_.lerp(tgt_, t);

      // The perpendicular segment goes from the foot to p.
      return {foot, p};
    }

    /**
     * @brief Returns the perpendicular chord of a given length passing through the midpoint.
     * @param dist The half-length of the perpendicular chord. The total length will be `2*dist`.
     * @return A `Segment` perpendicular to this one, centered on its midpoint.
     * @throws std::domain_error if this segment has zero length.
     */
    [[nodiscard]] Segment mid_perpendicular(const Geom_Number & dist) const
    {
      const Point mid = mid_point();
      const Point dir = (tgt_ - src_).normalize();
      const Point perp_dir(-dir.get_y(), dir.get_x()); // Rotated by +90 degrees

      return {mid - perp_dir * dist, mid + perp_dir * dist};
    }

    /**
     * @brief Checks if this segment properly intersects another segment.
     * @details A proper intersection means the segments cross at an interior point,
     *          and do not share any endpoints or overlap collinearly.
     * @param s The other segment.
     * @return `true` if they properly intersect.
     */
    [[nodiscard]] bool intersects_properly_with(const Segment & s) const
    {
      // Each pair of endpoints must lie strictly on opposite sides of the
      // other segment's supporting line.  Using the product of signed areas
      // guarantees that shared endpoints (area == 0) are never treated as a
      // proper crossing.
      const auto d1 = area_of_parallelogram(s.src_, s.tgt_, src_);
      const auto d2 = area_of_parallelogram(s.src_, s.tgt_, tgt_);
      const auto d3 = area_of_parallelogram(src_, tgt_, s.src_);
      const auto d4 = area_of_parallelogram(src_, tgt_, s.tgt_);
      return (d1 * d2 < 0) and (d3 * d4 < 0);
    }

    /**
     * @brief Checks if a point lies on this segment.
     * @param p The point to check.
     * @return `true` if `p` is collinear and lies between the endpoints.
     */
    [[nodiscard]] bool contains(const Point & p) const
    {
      return p.is_between(src_, tgt_);
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Point & p) const { return contains(p); }

    /**
     * @brief Checks if another segment `s` lies entirely inside this segment.
     * @param s The other segment.
     * @return `true` if both endpoints of `s` are on this segment.
     */
    [[nodiscard]] bool contains(const Segment & s) const
    {
      return contains(s.get_src_point()) and contains(s.get_tgt_point());
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Segment & s) const { return contains(s); }

    /**
     * @brief Checks if this segment intersects another one (including endpoints and collinear overlap).
     * @param s The other segment.
     * @return `true` if they share at least one point.
     */
    [[nodiscard]] bool intersects_with(const Segment & s) const
    {
      if (this->intersects_properly_with(s))
        return true;

      return this->contains(s.src_) or this->contains(s.tgt_) or
             s.contains(this->src_) or s.contains(this->tgt_);
    }

    /**
     * @brief Checks if this segment intersects a triangle.
     * @param t The triangle.
     * @return `true` if the segment and triangle share at least one point.
     */
    [[nodiscard]] inline bool intersects_with(const Triangle & t) const;

    /**
     * @brief Checks if this segment intersects an ellipse.
     * @param e The ellipse.
     * @return `true` if the segment and ellipse share at least one point.
     */
    [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

    /**
     * @brief Checks if this segment is parallel to another one.
     * @param s The other segment.
     * @return `true` if their direction vectors are parallel (cross product is zero).
     */
    [[nodiscard]] bool is_parallel_with(const Segment & s) const
    {
      return (tgt_ - src_).cross(s.tgt_ - s.src_) == 0;
    }

    /**
     * @brief Computes the intersection point of the infinite lines defined by two segments.
     * @param s The other segment.
     * @return The `Point` of intersection.
     * @throws std::domain_error if the segments are parallel.
     */
    [[nodiscard]] Point intersection_with(const Segment & s) const
    {
      const Point v1 = tgt_ - src_;
      const Point v2 = s.tgt_ - s.src_;
      const Geom_Number cross = v1.cross(v2);

      ah_domain_error_if(cross == 0) << "Segments are parallel";

      const Geom_Number t = (s.src_ - src_).cross(v2) / cross;

      return src_ + v1 * t;
    }

    /// @brief Cardinal and intercardinal directions associated with a segment's vector.
    enum Sense { E, NE, N, NW, W, SW, S, SE };

    /**
     * @brief Determines the cardinal/intercardinal direction of the segment.
     * @return The `Sense` enum value representing the direction from source to target.
     */
    [[nodiscard]] Sense sense() const
    {
      const Point dir = tgt_ - src_;
      if (dir.get_x() > 0) // headed east?
        {
          if (dir.get_y() > 0) // and north?
            return NE;
          if (dir.get_y() < 0) // and south?
            return SE;
          return E;
        }
      if (dir.get_x() < 0) // headed west?
        {
          if (dir.get_y() > 0) // and north?
            return NW;
          if (dir.get_y() < 0) // and south?
            return SW;
          return W;
        }

      // Vertical segment
      return dir.get_y() > 0 ? N : S;
    }

    /**
     * @brief Extends the segment by a given distance from the source endpoint.
     * @param dist The distance to extend.
     */
    void enlarge_src(const Geom_Number & dist)
    {
      if (src_ == tgt_) return;
      src_ -= (tgt_ - src_).normalize() * dist;
    }

    /**
     * @brief Extends the segment by a given distance from the target endpoint.
     * @param dist The distance to extend.
     */
    void enlarge_tgt(const Geom_Number & dist)
    {
      if (src_ == tgt_) return;
      tgt_ += (tgt_ - src_).normalize() * dist;
    }

    /**
     * @brief Returns a string representation of the segment, e.g., "(x1,y1)(x2,y2)".
     * @return A `std::string` representing the segment.
     */
    [[nodiscard]] std::string to_string() const
    {
      return src_.to_string() + tgt_.to_string();
    }

    /**
     * @brief Cast operator to `std::string`.
     */
    operator std::string() const
    {
      return to_string();
    }

    /**
     * @brief Rotates the segment by a given angle around its source point.
     * @param angle The angle of rotation in radians.
     */
    void rotate(const Geom_Number & angle)
    {
      if (angle == 0)
        return;
      tgt_ = src_ + (tgt_ - src_).rotate(angle);
    }

    /**
     * @brief Computes the intersection segment with a triangle.
     * @param t The triangle.
     * @return The `Segment` of intersection. Can be a point (degenerate segment).
     * @throws std::domain_error if there is no intersection.
     */
    [[nodiscard]] inline Segment intersection_with(const Triangle & t) const;

    /**
     * @brief Computes the intersection segment with an ellipse.
     * @param e The ellipse.
     * @return The `Segment` of intersection. Can be a point for tangency.
     * @throws std::domain_error if there is no intersection.
     */
    [[nodiscard]] inline Segment intersection_with(const Ellipse & e) const;
  };

  // Return true if this point lies inside segment s.
  inline bool Point::is_inside(const Segment & s) const
  {
    return s.contains(*this);
  }

  // Return true if this point is colinear with segment s.
  inline bool Point::is_colinear_with(const Segment & s) const
  {
    return this->is_colinear_with(s.src_, s.tgt_);
  }

  // Return true if this point is to the left of the segment s.
  inline bool Point::is_left_of(const Segment & s) const
  {
    return this->is_left_of(s.src_, s.tgt_);
  }

  // Return true if this point is to the right of the segment s.
  inline bool Point::is_right_of(const Segment & s) const
  {
    return area_of_parallelogram(s.src_, s.tgt_, *this) < 0;
  }

  // Return true if the sequence (this, segment) is clockwise.
  inline bool Point::is_clockwise_with(const Segment & s) const
  {
    return this->is_clockwise_with(s.src_, s.tgt_);
  }


  // Return the squared Euclidean distance to that.
  inline Geom_Number Point::distance_squared_to(const Point & that) const
  {
    const Geom_Number dx = this->x_ - that.x_;
    const Geom_Number dy = this->y_ - that.y_;
    return dx * dx + dy * dy;
  }


  // Return the Euclidean distance to p.
  inline Geom_Number Point::distance_to(const Point & p) const
  {
    return euclidean_distance(p.x_ - x_, p.y_ - y_);
  }


  /** @brief A non-degenerate triangle defined by three points.
   *
   * @ingroup Geometry
   *
   * Provides basic geometric properties and predicates for a triangle,
   * such as area, orientation, and special centers (centroid, circumcenter, incenter).
   * Used by polygon triangulation and other geometric algorithms.
   */
  class Triangle : public Geom_Object
  {
    friend class Point;
    friend class Segment;

    Point p1_, p2_, p3_;

    Geom_Number area_; // signed area * 2

  public:
    /**
     * @brief Constructs a triangle from three points.
     * @param p1 The first vertex.
     * @param p2 The second vertex.
     * @param p3 The third vertex.
     * @throws std::domain_error if the three points are collinear.
     */
    Triangle(Point p1, Point p2, Point p3)
      : p1_(std::move(p1)), p2_(std::move(p2)), p3_(std::move(p3))
    {
      // Note: area_ is twice the signed area
      area_ = area_of_parallelogram(p1_, p2_, p3_);
      ah_domain_error_if(area_ == 0) << "The three points of a triangle cannot be collinear";
    }

    /**
     * @brief Constructs a triangle from a point and a segment.
     * @param p The first vertex.
     * @param s The segment defining the other two vertices.
     * @throws std::domain_error if the three points are collinear.
     */
    Triangle(Point p, const Segment & s)
      : p1_(std::move(p)), p2_(s.src_), p3_(s.tgt_)
    {
      area_ = area_of_parallelogram(p1_, p2_, p3_);
      ah_domain_error_if(area_ == 0) << "The three points of a triangle cannot be collinear";
    }

    /**
     * @brief Constructs a triangle from a segment and a point.
     * @param s The segment defining the first two vertices.
     * @param p The third vertex.
     * @throws std::domain_error if the three points are collinear.
     */
    Triangle(const Segment & s, Point p)
      : p1_(s.src_), p2_(s.tgt_), p3_(std::move(p))
    {
      area_ = area_of_parallelogram(p1_, p2_, p3_);
      ah_domain_error_if(area_ == 0) << "The three points of a triangle cannot be collinear";
    }

    /**
     * @brief Calculates the unsigned area of the triangle.
     * @return The area of the triangle.
     */
    [[nodiscard]] Geom_Number area() const
    {
      return abs(area_) / 2;
    }

    /**
     * @brief Checks if the triangle vertices are ordered clockwise.
     * @return `true` if the vertex order (p1, p2, p3) is clockwise, `false` otherwise.
     */
    [[nodiscard]] bool is_clockwise() const
    {
      return area_ < 0;
    }

    /**
     * @brief Gets the vertex with the largest y-coordinate.
     * @return A const reference to the highest vertex.
     */
    [[nodiscard]] const Point &highest_point() const
    {
      const Point & max = p1_.get_y() > p2_.get_y() ? p1_ : p2_;
      return p3_.get_y() > max.get_y() ? p3_ : max;
    }

    /**
     * @brief Gets the vertex with the smallest y-coordinate.
     * @return A const reference to the lowest vertex.
     */
    [[nodiscard]] const Point &lowest_point() const
    {
      const Point & min = p1_.get_y() < p2_.get_y() ? p1_ : p2_;
      return p3_.get_y() < min.get_y() ? p3_ : min;
    }

    /**
     * @brief Gets the vertex with the smallest x-coordinate.
     * @return A const reference to the leftmost vertex.
     */
    [[nodiscard]] const Point &leftmost_point() const
    {
      const Point & min = p1_.get_x() < p2_.get_x() ? p1_ : p2_;
      return p3_.get_x() < min.get_x() ? p3_ : min;
    }

    /**
     * @brief Gets the vertex with the largest x-coordinate.
     * @return A const reference to the rightmost vertex.
     */
    [[nodiscard]] const Point &rightmost_point() const
    {
      const Point & max = p1_.get_x() > p2_.get_x() ? p1_ : p2_;
      return p3_.get_x() > max.get_x() ? p3_ : max;
    }

    /** @brief Gets the first vertex. */
    [[nodiscard]] const Point &get_p1() const { return p1_; }
    /** @brief Gets the second vertex. */
    [[nodiscard]] const Point &get_p2() const { return p2_; }
    /** @brief Gets the third vertex. */
    [[nodiscard]] const Point &get_p3() const { return p3_; }

    /**
     * @brief Checks for equality between two triangles.
     * @note Triangles are equal if they have the same set of three vertices,
     *       regardless of their order.
     * @param t The other triangle to compare.
     * @return `true` if the triangles are composed of the same three points.
     */
    [[nodiscard]] bool operator==(const Triangle & t) const noexcept
    {
      const bool p1_in = p1_ == t.p1_ or p1_ == t.p2_ or p1_ == t.p3_;
      const bool p2_in = p2_ == t.p1_ or p2_ == t.p2_ or p2_ == t.p3_;
      const bool p3_in = p3_ == t.p1_ or p3_ == t.p2_ or p3_ == t.p3_;
      return p1_in and p2_in and p3_in;
    }

    /**
     * @brief Checks for inequality between two triangles.
     * @param t The other triangle to compare.
     * @return `true` if the triangles do not share the same three vertices.
     */
    [[nodiscard]] bool operator!=(const Triangle & t) const noexcept
    {
      return not (*this == t);
    }

    /**
     * @brief Computes the centroid (center of mass) of the triangle.
     * @return A `Point` representing the centroid.
     */
    [[nodiscard]] Point centroid() const
    {
      return (p1_ + p2_ + p3_) / Geom_Number(3);
    }

    /**
     * @brief Computes the perimeter of the triangle.
     * @return The sum of the lengths of the three edges.
     */
    [[nodiscard]] Geom_Number perimeter() const
    {
      return p1_.distance_to(p2_) + p2_.distance_to(p3_) + p3_.distance_to(p1_);
    }

    /**
     * @brief Computes the circumcenter of the triangle.
     * @details The circumcenter is the intersection point of the perpendicular
     *          bisectors of the edges and is equidistant from the three vertices.
     * @return A `Point` representing the circumcenter.
     * @throws std::domain_error if the triangle is degenerate (collinear vertices).
     */
    [[nodiscard]] Point circumcenter() const
    {
      const Geom_Number & x1 = p1_.get_x();
      const Geom_Number & y1 = p1_.get_y();
      const Geom_Number & x2 = p2_.get_x();
      const Geom_Number & y2 = p2_.get_y();
      const Geom_Number & x3 = p3_.get_x();
      const Geom_Number & y3 = p3_.get_y();

      const Geom_Number d = Geom_Number(2) *
                            (x1 * (y2 - y3) + x2 * (y3 - y1) + x3 * (y1 - y2));
      ah_domain_error_if(d == 0) << "Cannot compute circumcenter of a degenerate triangle";

      const Geom_Number x1sq_y1sq = x1 * x1 + y1 * y1;
      const Geom_Number x2sq_y2sq = x2 * x2 + y2 * y2;
      const Geom_Number x3sq_y3sq = x3 * x3 + y3 * y3;

      const Geom_Number ux =
      (x1sq_y1sq * (y2 - y3) + x2sq_y2sq * (y3 - y1) +
       x3sq_y3sq * (y1 - y2)) / d;

      const Geom_Number uy =
      (x1sq_y1sq * (x3 - x2) + x2sq_y2sq * (x1 - x3) +
       x3sq_y3sq * (x2 - x1)) / d;

      return {ux, uy};
    }

    /**
     * @brief Computes the incenter of the triangle.
     * @details The incenter is the intersection point of the angle bisectors
     *          and is the center of the triangle's incircle.
     * @return A `Point` representing the incenter.
     * @throws std::domain_error if the triangle is degenerate.
     */
    [[nodiscard]] Point incenter() const
    {
      const Geom_Number a = p2_.distance_to(p3_);
      const Geom_Number b = p1_.distance_to(p3_);
      const Geom_Number c = p1_.distance_to(p2_);
      const Geom_Number sum = a + b + c;
      ah_domain_error_if(sum == 0) << "Cannot compute incenter of a degenerate triangle";

      return (p1_ * a + p2_ * b + p3_ * c) / sum;
    }

    /**
     * @brief Gets the three edges of the triangle.
     * @return A `std::array` of 3 `Segment`s in the order (p1,p2), (p2,p3), (p3,p1).
     */
    [[nodiscard]] std::array<Segment, 3> edges() const
    {
      return {Segment(p1_, p2_), Segment(p2_, p3_), Segment(p3_, p1_)};
    }

    /**
     * @brief Checks if a point lies strictly inside this triangle.
     * @details This is done by checking if the point is on the same side of all
     *          three edges. The result depends on the triangle's orientation.
     * @param p The point to check.
     * @return `true` if the point is strictly inside, `false` if it is on an
     *         edge or outside.
     */
    [[nodiscard]] bool contains(const Point & p) const
    {
      if (is_clockwise())
        return p.is_to_right_from(p1_, p2_) and
               p.is_to_right_from(p2_, p3_) and
               p.is_to_right_from(p3_, p1_);
      return p.is_left_of(p1_, p2_) and
             p.is_left_of(p2_, p3_) and
             p.is_left_of(p3_, p1_);
    }

    /**
     * @brief Computes the intersection segment between this triangle and a segment.
     * @param s The segment to intersect with.
     * @return The intersection `Segment`.
     * @see Segment::intersection_with(const Triangle &)
     */
    [[nodiscard]] Segment intersection_with(const Segment & s) const
    {
      return s.intersection_with(*this);
    }
  };

  /** @brief An axis-aligned rectangle.
   *
   * @ingroup Geometry
   *
   * Stored as (xmin, ymin, xmax, ymax) with the invariant that
   * xmin <= xmax and ymin <= ymax must hold.
   */
  class Rectangle
  {
    Geom_Number xmin_, ymin_;
    Geom_Number xmax_, ymax_;

  public:
    /**
     * @brief Checks for exact equality between two rectangles.
     * @param r The other rectangle to compare.
     * @return `true` if all four coordinates are identical.
     */
    [[nodiscard]] bool operator==(const Rectangle & r) const noexcept
    {
      return xmin_ == r.xmin_ and ymin_ == r.ymin_ and
             xmax_ == r.xmax_ and ymax_ == r.ymax_;
    }

    /**
     * @brief Checks for inequality between two rectangles.
     * @param r The other rectangle to compare.
     * @return `true` if any coordinate is different.
     */
    [[nodiscard]] bool operator!=(const Rectangle & r) const noexcept
    {
      return not (*this == r);
    }

    /** @brief Gets the minimum x-coordinate. */
    [[nodiscard]] const Geom_Number &get_xmin() const { return xmin_; }
    /** @brief Gets the minimum y-coordinate. */
    [[nodiscard]] const Geom_Number &get_ymin() const { return ymin_; }
    /** @brief Gets the maximum x-coordinate. */
    [[nodiscard]] const Geom_Number &get_xmax() const { return xmax_; }
    /** @brief Gets the maximum y-coordinate. */
    [[nodiscard]] const Geom_Number &get_ymax() const { return ymax_; }

    /**
     * @brief Default constructor. Initializes a zero-sized rectangle at the origin.
     */
    Rectangle() : xmin_(0), ymin_(0), xmax_(0), ymax_(0)
    {
      // empty
    }

    /**
     * @brief Constructs a rectangle from four coordinates.
     * @param xmin The minimum x-coordinate.
     * @param ymin The minimum y-coordinate.
     * @param xmax The maximum x-coordinate.
     * @param ymax The maximum y-coordinate.
     * @throws std::out_of_range if xmin > xmax or ymin > ymax.
     */
    Rectangle(const Geom_Number & xmin, const Geom_Number & ymin,
              const Geom_Number & xmax, const Geom_Number & ymax)
      : xmin_(xmin), ymin_(ymin), xmax_(xmax), ymax_(ymax)
    {
      ah_range_error_if(xmax_ < xmin_ or ymax_ < ymin_) << "Invalid rectangle";
    }

    /**
     * @brief Sets the coordinates of the rectangle.
     * @param xmin The minimum x-coordinate.
     * @param ymin The minimum y-coordinate.
     * @param xmax The maximum x-coordinate.
     * @param ymax The maximum y-coordinate.
     * @throws std::out_of_range if xmin > xmax or ymin > ymax.
     */
    void set_rect(const Geom_Number & xmin, const Geom_Number & ymin,
                  const Geom_Number & xmax, const Geom_Number & ymax)
    {
      ah_range_error_if(xmax < xmin or ymax < ymin) << "Invalid rectangle";

      xmin_ = xmin;
      ymin_ = ymin;
      xmax_ = xmax;
      ymax_ = ymax;
    }

    /** @brief Calculates the width of the rectangle. */
    [[nodiscard]] Geom_Number width() const noexcept { return xmax_ - xmin_; }
    /** @brief Calculates the height of the rectangle. */
    [[nodiscard]] Geom_Number height() const noexcept { return ymax_ - ymin_; }
    /** @brief Calculates the area of the rectangle. */
    [[nodiscard]] Geom_Number area() const noexcept { return width() * height(); }

    /**
     * @brief Calculates the perimeter of the rectangle.
     * @return The perimeter length.
     */
    [[nodiscard]] Geom_Number perimeter() const noexcept
    {
      return 2 * (width() + height());
    }

    /**
     * @brief Calculates the center point of the rectangle.
     * @return A `Point` at the center of the rectangle.
     */
    [[nodiscard]] Point center() const noexcept
    {
      return {(xmin_ + xmax_) / 2, (ymin_ + ymax_) / 2};
    }

    /**
     * @brief Gets the four corners of the rectangle.
     * @return A `std::array` of 4 `Point`s in counter-clockwise order,
     *         starting at the bottom-left.
     */
    [[nodiscard]] std::array<Point, 4> corners() const noexcept
    {
      return {
            Point(xmin_, ymin_),
            Point(xmax_, ymin_),
            Point(xmax_, ymax_),
            Point(xmin_, ymax_)
          };
    }

    /**
     * @brief Checks if this axis-aligned rectangle intersects another one.
     * @param that The other rectangle.
     * @return `true` if they overlap, `false` otherwise.
     */
    [[nodiscard]] bool intersects(const Rectangle & that) const noexcept
    {
      return this->xmax_ >= that.xmin_ and this->ymax_ >= that.ymin_ and
             that.xmax_ >= this->xmin_ and that.ymax_ >= this->ymin_;
    }

    /**
     * @brief Calculates the squared Euclidean distance from the rectangle to a point.
     * @param p The point.
     * @return The squared distance from `p` to the closest point on this rectangle.
     *         Returns 0 if the point is inside.
     */
    [[nodiscard]] Geom_Number distance_squared_to(const Point & p) const noexcept
    {
      Geom_Number dx = 0.0, dy = 0.0;
      if (p.get_x() < xmin_)
        dx = p.get_x() - xmin_;
      else if (p.get_x() > xmax_)
        dx = p.get_x() - xmax_;

      if (p.get_y() < ymin_)
        dy = p.get_y() - ymin_;
      else if (p.get_y() > ymax_)
        dy = p.get_y() - ymax_;

      return dx * dx + dy * dy;
    }

    /**
     * @brief Calculates the Euclidean distance from the rectangle to a point.
     * @param p The point.
     * @return The distance from `p` to the closest point on this rectangle.
     */
    [[nodiscard]] Geom_Number distance_to(const Point & p) const
    {
      return sqrt(mpfr_class(distance_squared_to(p)));
    }

    /**
     * @brief Checks if this axis-aligned rectangle contains a point.
     * @param p The point to check.
     * @return `true` if the point is inside or on the boundary of the rectangle.
     */
    [[nodiscard]] bool contains(const Point & p) const noexcept
    {
      return p.get_x() >= xmin_ and p.get_x() <= xmax_ and
             p.get_y() >= ymin_ and p.get_y() <= ymax_;
    }

    /**
     * @brief Returns a string representation of the rectangle.
     * @return A `std::string` in the format "(xmin,ymin)-(xmax,ymax)".
     */
    [[nodiscard]] std::string to_string() const
    {
      return "(" + std::to_string(geom_number_to_double(xmin_)) + "," +
             std::to_string(geom_number_to_double(ymin_)) + ")-(" +
             std::to_string(geom_number_to_double(xmax_)) + "," +
             std::to_string(geom_number_to_double(ymax_)) + ")";
    }
  };

  // Returns true if this segment intersects one or two edges of triangle @p t.
  inline bool Segment::intersects_with(const Triangle & t) const
  {
    return this->intersects_with(Segment(t.get_p1(), t.get_p2())) or
           this->intersects_with(Segment(t.get_p2(), t.get_p3())) or
           this->intersects_with(Segment(t.get_p3(), t.get_p1()));
  }


  // Returns the segment resulting from intersecting this segment with triangle @p t.
  // If an endpoint lies inside the triangle, the intersection degenerates to a
  // point, which can be used to test if a point belongs to @p t.
  inline Segment Segment::intersection_with(const Triangle & t) const
  {
    ah_domain_error_if(not this->intersects_with(t))
      << "segment does not intersects with triangle";

    std::array<Point, 6> points;
    size_t count = 0;

    auto append_unique = [&points, &count](const Point & pt)
      {
        for (size_t i = 0; i < count; ++i)
          if (points[i] == pt)
            return;
        points[count++] = pt;
      };

    auto collect_with_edge = [this, &append_unique](const Segment & edge)
      {
        if (not this->intersects_with(edge))
          return;

        if (this->is_parallel_with(edge))
          {
            if (this->contains(edge.get_src_point()))
              append_unique(edge.get_src_point());
            if (this->contains(edge.get_tgt_point()))
              append_unique(edge.get_tgt_point());
            if (edge.contains(this->get_src_point()))
              append_unique(this->get_src_point());
            if (edge.contains(this->get_tgt_point()))
              append_unique(this->get_tgt_point());
            return;
          }

        append_unique(this->intersection_with(edge));
      };

    collect_with_edge(Segment(t.get_p1(), t.get_p2()));
    collect_with_edge(Segment(t.get_p2(), t.get_p3()));
    collect_with_edge(Segment(t.get_p3(), t.get_p1()));

    ah_domain_error_if(count == 0)
      << "no intersection point found despite intersection confirmed";

    if (count == 1)
      return {points[0], points[0]};

    // Keep the farthest pair in case collinear overlaps produced >2 points.
    size_t i_best = 0;
    size_t j_best = 1;
    Geom_Number best_d2 = points[0].distance_squared_to(points[1]);
    for (size_t i = 0; i < count; ++i)
      for (size_t j = i + 1; j < count; ++j)
        if (const Geom_Number d2 = points[i].distance_squared_to(points[j]); d2 > best_d2)
          {
            best_d2 = d2;
            i_best = i;
            j_best = j;
          }

    return {points[i_best], points[j_best]};
  }


  /** @brief An axis-aligned ellipse.
   *
   * @ingroup Geometry
   *
   * Represents an axis-aligned ellipse centered at a point with a horizontal
   * and a vertical radius. Provides methods for calculating geometric properties
   * and handling intersections.
   */
  class Ellipse : public Geom_Object
  {
    friend class Point;

    /*
      The ellipse is defined relative to its center (xc, yc) by:

                                         2           2
                                 (y - yc)    (x - xc)
                                 --------- + --------- = 1
                                    2           2
                                  vr          hr
    */

    Point center_; // ellipse center

    Geom_Number hr_; // horizontal radius (parameter a)
    Geom_Number vr_; // vertical radius (parameter b)

  public:
    /**
     * @brief Constructs an ellipse.
     * @param center The center point of the ellipse.
     * @param hr The horizontal radius.
     * @param vr The vertical radius.
     * @throws std::domain_error if either radius is not positive.
     */
    Ellipse(Point center,
            const Geom_Number & hr,
            const Geom_Number & vr)
      : center_(std::move(center)), hr_(hr), vr_(vr)
    {
      validate_positive_radii();
    }

    /**
     * @brief Copy constructor.
     */
    Ellipse(const Ellipse & e) = default;


    /**
     * @brief Default constructor.
     * @note Creates a unit circle centered at the origin.
     */
    Ellipse() : center_(0, 0), hr_(1), vr_(1)
    {
      // empty
    }

    /**
     * @brief Checks for exact equality.
     * @param e The other ellipse.
     * @return `true` if center and radii are identical.
     */
    [[nodiscard]] bool operator==(const Ellipse & e) const noexcept
    {
      return center_ == e.center_ and hr_ == e.hr_ and vr_ == e.vr_;
    }

    /**
     * @brief Checks for inequality.
     * @param e The other ellipse.
     * @return `true` if center or radii are different.
     */
    [[nodiscard]] bool operator!=(const Ellipse & e) const noexcept
    {
      return not (*this == e);
    }

    /** @brief Gets the center point of the ellipse. */
    [[nodiscard]] const Point &get_center() const { return center_; }
    /** @brief Gets the horizontal radius. */
    [[nodiscard]] const Geom_Number &get_hradius() const { return hr_; }
    /** @brief Gets the vertical radius. */
    [[nodiscard]] const Geom_Number &get_vradius() const { return vr_; }

    /**
     * @brief Calculates the area of the ellipse.
     * @return The area (pi * hr * vr).
     */
    [[nodiscard]] Geom_Number area() const
    {
      validate_positive_radii();
      return geom_pi() * hr_ * vr_;
    }

    /**
     * @brief Approximates the perimeter of the ellipse.
     * @details Uses Ramanujan's second approximation formula for the perimeter.
     * @return The approximate perimeter length.
     */
    [[nodiscard]] Geom_Number perimeter() const
    {
      validate_positive_radii();
      const Geom_Number ab_sum = hr_ + vr_;
      const Geom_Number h = (hr_ - vr_) * (hr_ - vr_) / (ab_sum * ab_sum);
      const Geom_Number correction =
          Geom_Number(1) + Geom_Number(3) * h /
          (Geom_Number(10) + square_root(Geom_Number(4) - Geom_Number(3) * h));
      return geom_pi() * ab_sum * correction;
    }

    /**
     * @brief Samples a point on the ellipse's boundary at a given angle.
     * @param angle The parametric angle in radians.
     * @return A `Point` on the ellipse's boundary.
     */
    [[nodiscard]] Point sample(const Geom_Number & angle) const
    {
      validate_positive_radii();
      return {
            center_.get_x() + hr_ * cosinus(angle),
            center_.get_y() + vr_ * sinus(angle)
          };
    }

    /**
     * @brief Returns a string representation of the ellipse.
     * @return A `std::string` describing the ellipse's properties.
     */
    [[nodiscard]] std::string to_string() const
    {
      return "Ellipse(center=" + center_.to_string() +
             ", hr=" + std::to_string(geom_number_to_double(hr_)) +
             ", vr=" + std::to_string(geom_number_to_double(vr_)) + ")";
    }

  private:
    /**
     * @brief Validates that radii are positive.
     * @throws std::domain_error if either radius is not > 0.
     */
    void validate_positive_radii() const
    {
      ah_domain_error_if(hr_ <= 0 or vr_ <= 0) << "Ellipse radii must be > 0";
    }

    /**
     * @brief Helper to check if a parameter `t` is in the [0, 1] interval.
     */
    [[nodiscard]] static bool in_unit_interval(const Geom_Number & t)
    {
      return t >= 0 and t <= 1;
    }

  public:
    /**
     * @brief Returns the orientation of the ellipse. Always counter-clockwise.
     * @return `false`, indicating CCW.
     */
    static bool is_clockwise() { return false; }

    /** @brief Gets the highest point on the ellipse boundary. */
    [[nodiscard]] Point highest_point() const
    {
      return {center_.get_x(), center_.get_y() + vr_};
    }

    /** @brief Gets the lowest point on the ellipse boundary. */
    [[nodiscard]] Point lowest_point() const
    {
      return {center_.get_x(), center_.get_y() - vr_};
    }

    /** @brief Gets the leftmost point on the ellipse boundary. */
    [[nodiscard]] Point leftmost_point() const
    {
      return {center_.get_x() - hr_, center_.get_y()};
    }

    /** @brief Gets the rightmost point on the ellipse boundary. */
    [[nodiscard]] Point rightmost_point() const
    {
      return {center_.get_x() + hr_, center_.get_y()};
    }

    /**
     * @brief Computes the two tangent segments to this ellipse with a given slope.
     * @param[out] s1 The first tangent segment.
     * @param[out] s2 The second tangent segment.
     * @param[in] m The slope of the tangents.
     */
    void
    compute_tangents(Segment & s1, Segment & s2, const Geom_Number & m) const
    {
      validate_positive_radii();

      const Geom_Number m2 = m * m;
      const Geom_Number hr2 = hr_ * hr_;
      const Geom_Number vr2 = vr_ * vr_;
      const Geom_Number y_intercept_sq = hr2 * m2 + vr2;
      ah_domain_error_if(y_intercept_sq < 0) << "No tangent for this slope";

      const Geom_Number y_intercept = square_root(y_intercept_sq);

      // Tangent points on origin-centered ellipse
      const Point t1_local(-hr2*m/y_intercept,  vr2/y_intercept);
      const Point t2_local( hr2*m/y_intercept, -vr2/y_intercept);

      // Translate to ellipse's actual center
      const Point tangent_point1 = center_ + t1_local;
      const Point tangent_point2 = center_ + t2_local;

      // Create segments of arbitrary length through the tangent points
      const Geom_Number tangent_len = hr_ > vr_ ? hr_ : vr_;
      s1 = Segment(tangent_point1 - (Point(1, m).normalize() * tangent_len),
                   tangent_point1 + (Point(1, m).normalize() * tangent_len));
      s2 = Segment(tangent_point2 - (Point(1, m).normalize() * tangent_len),
                   tangent_point2 + (Point(1, m).normalize() * tangent_len));
    }

    /**
     * @brief Checks if a segment intersects the ellipse.
     * @details Solves the quadratic equation for the intersection of a line
     *          and an ellipse.
     * @param s The segment to check.
     * @return `true` if the segment and ellipse share at least one point.
     */
    [[nodiscard]] bool intersects_with(const Segment & s) const
    {
      validate_positive_radii();

      const Point p0 = s.get_src_point() - center_;
      const Point p1 = s.get_tgt_point() - center_;
      const Geom_Number dx = p1.get_x() - p0.get_x();
      const Geom_Number dy = p1.get_y() - p0.get_y();
      const Geom_Number a2 = hr_ * hr_;
      const Geom_Number b2 = vr_ * vr_;

      const Geom_Number A = (dx * dx) / a2 + (dy * dy) / b2;
      const Geom_Number B =
          Geom_Number(2) * (p0.get_x() * dx / a2 + p0.get_y() * dy / b2);
      const Geom_Number C =
          (p0.get_x() * p0.get_x()) / a2 + (p0.get_y() * p0.get_y()) / b2 -
          Geom_Number(1);

      // For a segment, A can be 0 if it's a point.
      if (A == 0)
        return C <= 0; // Point is inside or on the ellipse

      const Geom_Number discriminant = B * B - Geom_Number(4) * A * C;
      if (discriminant < 0)
        return false; // No real roots, no intersection

      // Check if intersection points lie on the segment (t in [0,1])
      const Geom_Number root = square_root(discriminant);
      const Geom_Number denom = Geom_Number(2) * A;
      const Geom_Number t1 = (-B - root) / denom;
      const Geom_Number t2 = (-B + root) / denom;
      return in_unit_interval(t1) or in_unit_interval(t2);
    }

  private:
    /**
     * @brief Computes the ellipse equation value for a point.
     * @details Calculates `(x-xc)^2/hr^2 + (y-yc)^2/vr^2`.
     * @param p The point to evaluate.
     * @return A value that is < 1 for interior points, = 1 for boundary points,
     *         and > 1 for exterior points.
     */
    [[nodiscard]] Geom_Number compute_radius(const Point & p) const
    {
      validate_positive_radii();

      Geom_Number x2 = (p.get_x() - center_.get_x());
      x2 = x2 * x2;

      Geom_Number y2 = (p.get_y() - center_.get_y());
      y2 = y2 * y2;

      return x2 / (hr_ * hr_) + y2 / (vr_ * vr_);
    }

  public:
    /**
     * @brief Checks if a point lies inside or on the boundary of this ellipse.
     * @param p The point to check.
     * @return `true` if the point is contained within the ellipse.
     */
    [[nodiscard]] bool contains(const Point & p) const
    {
      return compute_radius(p) <= 1;
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Point & p) const { return contains(p); }

    /**
     * @brief Checks if a point lies exactly on the ellipse boundary.
     * @param p The point to check.
     * @return `true` if the point is on the boundary.
     */
    [[nodiscard]] bool intersects_with(const Point & p) const
    {
      return compute_radius(p) == 1;
    }

    /**
     * @brief Computes the intersection segment between a line segment and this ellipse.
     * @param sg The segment to intersect with.
     * @return A `Segment` representing the portion of `sg` inside the ellipse.
     *         If tangent, the segment is degenerate (a point).
     * @throws std::domain_error if there is no intersection.
     */
    [[nodiscard]] Segment intersection_with(const Segment & sg) const
    {
      validate_positive_radii();

      const Point p0 = sg.get_src_point() - center_;
      const Point p1 = sg.get_tgt_point() - center_;
      const Geom_Number dx = p1.get_x() - p0.get_x();
      const Geom_Number dy = p1.get_y() - p0.get_y();
      const Geom_Number a2 = hr_ * hr_;
      const Geom_Number b2 = vr_ * vr_;

      const Geom_Number A = dx * dx / a2 + dy * dy / b2;
      const Geom_Number B =
          Geom_Number(2) * (p0.get_x() * dx / a2 + p0.get_y() * dy / b2);
      const Geom_Number C =
          p0.get_x() * p0.get_x() / a2 + p0.get_y() * p0.get_y() / b2 - Geom_Number(1);

      if (A == 0)
        {
          ah_domain_error_if(C > 0) << "No intersection (point outside)";
          return {sg.get_src_point(), sg.get_src_point()};
        }

      const Geom_Number discriminant = B * B - Geom_Number(4) * A * C;
      ah_domain_error_if(discriminant < 0) << "No intersection (no real roots)";

      const Geom_Number root = square_root(discriminant);
      const Geom_Number denom = Geom_Number(2) * A;
      const Geom_Number t1 = (-B - root) / denom;
      const Geom_Number t2 = (-B + root) / denom;

      const bool in1 = in_unit_interval(t1);
      const bool in2 = in_unit_interval(t2);
      ah_domain_error_if(not in1 and not in2) << "Intersection points are outside the segment";

      auto eval_at = [&](const Geom_Number & t)
        {
          return sg.at(t);
        };

      if (in1 and in2)
        return {eval_at(t1), eval_at(t2)};

      const Point touch = in1 ? eval_at(t1) : eval_at(t2);
      return {touch, touch};
    }
  };

  // Return true if this point lies inside ellipse e.
  inline bool Point::is_inside(const Ellipse & e) const
  {
    return e.contains(*this);
  }

  // Return true if this point lies exactly on ellipse e.
  inline bool Point::intersects_with(const Ellipse & e) const
  {
    return e.intersects_with(*this);
  }

  // Return true if this segment intersects ellipse e.
  inline bool Segment::intersects_with(const Ellipse & e) const
  {
    return e.intersects_with(*this);
  }

  // Return the segment defined by the two intersection points between this
  // segment and ellipse e.
  inline Segment Segment::intersection_with(const Ellipse & e) const
  {
    return e.intersection_with(*this);
  }

  // ============================================================================
  // Rotated Ellipse
  // ============================================================================

  /**
   * @brief An ellipse with arbitrary rotation.
   * @ingroup Geometry
   *
   * @details Extends the axis-aligned Ellipse by adding a rotation angle.
   * The implicit equation in the rotated frame is:
   *
   *     `((x' - cx)² / a²) + ((y' - cy)² / b²) = 1`
   *
   * where `(x', y')` are coordinates rotated by `-θ` around the center.
   *
   * Since exact trigonometry is often not possible with rational numbers, the
   * rotation is specified by its cosine and sine as exact `Geom_Number` values.
   * The user is responsible for providing these values.
   */
  class RotatedEllipse : public Geom_Object
  {
    Point center_;
    Geom_Number a_; // semi-axis along the local x (before rotation)
    Geom_Number b_; // semi-axis along the local y (before rotation)
    Geom_Number cos_th_; // cosine of the rotation angle
    Geom_Number sin_th_; // sine of the rotation angle

    /**
     * @brief Transforms a world point to the ellipse's local, unrotated frame.
     * @param p A point in world coordinates.
     * @return The corresponding point in the ellipse's local coordinate system.
     */
    [[nodiscard]] Point to_local(const Point & p) const
    {
      const Geom_Number dx = p.get_x() - center_.get_x();
      const Geom_Number dy = p.get_y() - center_.get_y();
      return {
            cos_th_ * dx + sin_th_ * dy,
            -sin_th_ * dx + cos_th_ * dy
          };
    }

    /**
     * @brief Transforms a point from the ellipse's local frame back to world coordinates.
     * @param p A point in the ellipse's local coordinate system.
     * @return The corresponding point in world coordinates.
     */
    [[nodiscard]] Point to_world(const Point & p) const
    {
      return {
            cos_th_ * p.get_x() - sin_th_ * p.get_y() + center_.get_x(),
            sin_th_ * p.get_x() + cos_th_ * p.get_y() + center_.get_y()
          };
    }

    /**
     * @brief Validates that semi-axes are positive.
     * @throws std::domain_error if either a_ or b_ is not > 0.
     */
    void validate_positive_radii() const
    {
      ah_domain_error_if(a_ <= 0 or b_ <= 0) << "RotatedEllipse radii must be > 0";
    }

    /**
     * @brief Normalizes the rotation (cos, sin) vector to ensure it's a unit vector.
     * @throws std::domain_error if the rotation vector is (0,0).
     */
    void normalize_rotation()
    {
      const Geom_Number norm2 = cos_th_ * cos_th_ + sin_th_ * sin_th_;
      ah_domain_error_if(norm2 == 0) << "Rotation vector (cos,sin) cannot be (0,0)";
      if (norm2 != 1)
        {
          const Geom_Number norm = square_root(norm2);
          cos_th_ /= norm;
          sin_th_ /= norm;
        }
    }

  public:
    /**
     * @brief Constructs a rotated ellipse.
     * @param center The center point.
     * @param a The semi-axis length along the ellipse's local x-axis.
     * @param b The semi-axis length along the ellipse's local y-axis.
     * @param cos_theta The cosine of the rotation angle.
     * @param sin_theta The sine of the rotation angle.
     */
    RotatedEllipse(Point center,
                   const Geom_Number & a, const Geom_Number & b,
                   const Geom_Number & cos_theta,
                   const Geom_Number & sin_theta)
      : center_(std::move(center)), a_(a), b_(b),
        cos_th_(cos_theta), sin_th_(sin_theta)
    {
      validate_positive_radii();
      normalize_rotation();
    }

    /**
     * @brief Constructs an axis-aligned ellipse (rotation angle is 0).
     * @param center The center point.
     * @param a The semi-axis length (horizontal radius).
     * @param b The semi-axis length (vertical radius).
     */
    RotatedEllipse(Point center,
                   const Geom_Number & a, const Geom_Number & b)
      : center_(std::move(center)), a_(a), b_(b), cos_th_(1), sin_th_(0)
    {
      validate_positive_radii();
    }

    /**
     * @brief Default constructor.
     * @note Creates a unit circle at the origin with no rotation.
     */
    RotatedEllipse()
      : center_(0, 0), a_(1), b_(1), cos_th_(1), sin_th_(0)
    {
      // empty
    }

    RotatedEllipse(const RotatedEllipse &) = default;

    RotatedEllipse &operator=(const RotatedEllipse &) = default;

    /**
     * @brief Checks for exact equality between two rotated ellipses.
     * @param e The other ellipse.
     * @return `true` if all parameters (center, axes, rotation) are identical.
     */
    [[nodiscard]] bool operator==(const RotatedEllipse & e) const noexcept
    {
      return center_ == e.center_ and a_ == e.a_ and b_ == e.b_ and
             cos_th_ == e.cos_th_ and sin_th_ == e.sin_th_;
    }

    /**
     * @brief Checks for inequality between two rotated ellipses.
     * @param e The other ellipse.
     * @return `true` if any parameter is different.
     */
    [[nodiscard]] bool operator!=(const RotatedEllipse & e) const noexcept
    {
      return not (*this == e);
    }

    /** @brief Gets the center point. */
    [[nodiscard]] const Point &get_center() const { return center_; }
    /** @brief Gets the semi-axis 'a' (local x-axis radius). */
    [[nodiscard]] const Geom_Number &get_a() const { return a_; }
    /** @brief Gets the semi-axis 'b' (local y-axis radius). */
    [[nodiscard]] const Geom_Number &get_b() const { return b_; }
    /** @brief Gets the cosine of the rotation angle. */
    [[nodiscard]] const Geom_Number &get_cos() const { return cos_th_; }
    /** @brief Gets the sine of the rotation angle. */
    [[nodiscard]] const Geom_Number &get_sin() const { return sin_th_; }

    /**
     * @brief Calculates the area of the ellipse.
     * @return The area (pi * a * b).
     */
    [[nodiscard]] Geom_Number area() const
    {
      validate_positive_radii();
      return geom_pi() * a_ * b_;
    }

    /**
     * @brief Evaluates the ellipse equation for a given point.
     * @param p The point in world coordinates.
     * @return A value that is `< 1` for interior points, `= 1` for boundary
     *         points, and `> 1` for exterior points.
     */
    [[nodiscard]] Geom_Number radius_value(const Point & p) const
    {
      validate_positive_radii();

      const Point lp = to_local(p);
      return (lp.get_x() * lp.get_x()) / (a_ * a_) +
             (lp.get_y() * lp.get_y()) / (b_ * b_);
    }

    /**
     * @brief Checks if a point lies inside or on the ellipse.
     * @param p The point to check.
     * @return `true` if the point is contained.
     */
    [[nodiscard]] bool contains(const Point & p) const
    {
      return radius_value(p) <= 1;
    }

    /**
     * @brief Checks if a point lies strictly inside the ellipse.
     * @param p The point to check.
     * @return `true` if the point is strictly inside.
     */
    [[nodiscard]] bool strictly_contains(const Point & p) const
    {
      return radius_value(p) < 1;
    }

    /**
     * @brief Checks if a point lies exactly on the ellipse boundary.
     * @param p The point to check.
     * @return `true` if the point is on the boundary.
     */
    [[nodiscard]] bool on_boundary(const Point & p) const
    {
      return radius_value(p) == 1;
    }

    /**
     * @brief Samples a point on the ellipse boundary from a parametric angle.
     * @param cos_t The cosine of the parameter angle `t`.
     * @param sin_t The sine of the parameter angle `t`.
     * @return The corresponding `Point` on the ellipse in world coordinates.
     */
    [[nodiscard]] Point sample(const Geom_Number & cos_t,
                               const Geom_Number & sin_t) const
    {
      validate_positive_radii();
      // Point on axis-aligned ellipse: (a cos t, b sin t).
      // Rotate and translate to the world.
      return to_world(Point(a_ * cos_t, b_ * sin_t));
    }

    /**
     * @brief Samples a point on the ellipse boundary for a parameter `t`.
     * @param t A parameter in [0, 1], which maps to an angle in [0, 2π).
     * @return The corresponding `Point` on the ellipse.
     */
    [[nodiscard]] Point sample(const Geom_Number & t) const
    {
      const Geom_Number angle = Geom_Number(2) * geom_pi() * t;
      return sample(cosinus(angle), sinus(angle));
    }

    /**
     * @brief Checks if a segment intersects this rotated ellipse.
     * @param s The segment to check.
     * @return `true` if they share at least one point.
     */
    [[nodiscard]] bool intersects_with(const Segment & s) const
    {
      validate_positive_radii();
      const Segment local_s(to_local(s.get_src_point()), to_local(s.get_tgt_point()));
      return Ellipse(Point(0, 0), a_, b_).intersects_with(local_s);
    }

    /**
     * @brief Computes the intersection segment between `s` and this ellipse.
     * @param s The segment to intersect with.
     * @return The intersection `Segment`. Can be a point for tangency.
     * @throws std::domain_error if there is no intersection.
     */
    [[nodiscard]] Segment intersection_with(const Segment & s) const
    {
      validate_positive_radii();
      const Segment local_s(to_local(s.get_src_point()), to_local(s.get_tgt_point()));
      const Segment local_i = Ellipse(Point(0, 0), a_, b_).intersection_with(local_s);
      return {to_world(local_i.get_src_point()), to_world(local_i.get_tgt_point())};
    }

    [[nodiscard]] std::string to_string() const
    {
      return "RotatedEllipse(center=" + center_.to_string() +
             ", a=" + std::to_string(geom_number_to_double(a_)) +
             ", b=" + std::to_string(geom_number_to_double(b_)) +
             ", cos=" + std::to_string(geom_number_to_double(cos_th_)) +
             ", sin=" + std::to_string(geom_number_to_double(sin_th_)) + ")";
    }

    /// @brief Holds the four axis-extremal points of a rotated ellipse.
    struct ExtremalPoints
    {
      Point right, top, left, bottom;
    };

    /**
     * @brief Computes the four axis-extremal points of the rotated ellipse.
     * @note These are the points on the ellipse that are most to the right,
     *       top, left, and bottom in the world coordinate system.
     * @return An `ExtremalPoints` struct.
     */
    [[nodiscard]] ExtremalPoints extremal_points() const
    {
      // TODO: This is incorrect. It returns the endpoints of the semi-axes,
      // which are only the extremal points if the ellipse is axis-aligned.
      // The correct calculation requires finding the points where the derivative
      // of the world coordinate is zero.
      return {
            to_world(Point(a_, Geom_Number(0))),
            to_world(Point(Geom_Number(0), b_)),
            to_world(Point(-a_, Geom_Number(0))),
            to_world(Point(Geom_Number(0), -b_))
          };
    }
  };

  /*****************************************************************

                         Fundamental text primitive

     Utility to draw text strings on the plane. Offsets are not stored because
     callers can shift the reference point directly.
  */

  /* Estimate the count of printable characters in a LaTeX string (skipping '\',
     '$', '{', '}', etc.).
  */
  inline size_t approximate_string_size(const std::string & str)
  {
    const char *ptr = str.c_str();

    size_t _len = 0;
    for (int i = 0; true; /* empty */)
      {
        switch (ptr[i])
          {
          case '\\':
            // Skip all characters that compose the LaTeX command.
            for (++i; isalnum(ptr[i]) and ptr[i] != '\0'; /* nothing */)
              ++i;
            ++_len;
            break;

          case '$':
          case '{':
          case '}':
          case '\n':
            ++i;
            break;

          case '\0':
            return _len;

          default:
            ++_len;
            ++i;
            break;
          }
      }
  }

  class Text : public Geom_Object
  {
    Point p_;

    std::string str_;

    size_t len_ = 0;

  public:
    static constexpr double font_width_in_points = 0.8;

    static constexpr double font_height_in_points = 1.2;

    Text(Point p, const std::string & str)
      : p_(std::move(p)), str_(str), len_(approximate_string_size(str))
    {
      // empty
    }

    Text() = default;

    [[nodiscard]] const size_t &len() const { return len_; }

    [[nodiscard]] const Point &get_point() const
    {
      return p_;
    }

    [[nodiscard]] const std::string &get_str() const { return str_; }

    [[nodiscard]] Point highest_point() const
    {
      return p_;
    }

    [[nodiscard]] Point lowest_point() const
    {
      return p_;
    }

    [[nodiscard]] Point leftmost_point() const
    {
      return p_;
    }

    [[nodiscard]] Point rightmost_point() const
    {
      return p_;
    }
  };

  inline Geom_Number
  area_of_parallelogram(const Point & a, const Point & b, const Point & c)
  {
    return ((b.get_x() - a.get_x()) * (c.get_y() - a.get_y()) -
            (c.get_x() - a.get_x()) * (b.get_y() - a.get_y()));
  }


  /// Classification of three-point orientation.
  enum class Orientation
  {
    CCW, // counter-clockwise
    CW, // clockwise
    COLLINEAR
  };

  /** Return the orientation of the triple (a, b, c).
      Based on the sign of the cross product (b-a)x(c-a). */
  [[nodiscard]] inline Orientation
  orientation(const Point & a, const Point & b, const Point & c)
  {
    const Geom_Number area = area_of_parallelogram(a, b, c);
    if (area > 0) return Orientation::CCW;
    if (area < 0) return Orientation::CW;
    return Orientation::COLLINEAR;
  }

  /// Classification of a point with respect to a triangle circumcircle.
  enum class InCircleResult { INSIDE, ON_CIRCLE, OUTSIDE, DEGENERATE };

  /** Return the exact in-circle determinant for (a,b,c,p).
      The sign depends on triangle orientation (a,b,c). */
  [[nodiscard]] inline Geom_Number
  in_circle_determinant(const Point & a, const Point & b,
                        const Point & c, const Point & p)
  {
    const Geom_Number adx = a.get_x() - p.get_x();
    const Geom_Number ady = a.get_y() - p.get_y();
    const Geom_Number bdx = b.get_x() - p.get_x();
    const Geom_Number bdy = b.get_y() - p.get_y();
    const Geom_Number cdx = c.get_x() - p.get_x();
    const Geom_Number cdy = c.get_y() - p.get_y();

    const Geom_Number ad2 = adx * adx + ady * ady;
    const Geom_Number bd2 = bdx * bdx + bdy * bdy;
    const Geom_Number cd2 = cdx * cdx + cdy * cdy;

    return ad2 * (bdx * cdy - bdy * cdx) -
           bd2 * (adx * cdy - ady * cdx) +
           cd2 * (adx * bdy - ady * bdx);
  }

  /** Classify point p against circumcircle of triangle (a,b,c), exactly.
      @return INSIDE/ON_CIRCLE/OUTSIDE. If (a,b,c) is collinear, returns DEGENERATE. */
  [[nodiscard]] inline InCircleResult
  in_circle(const Point & a, const Point & b,
            const Point & c, const Point & p)
  {
    const Orientation o = orientation(a, b, c);
    if (o == Orientation::COLLINEAR)
      return InCircleResult::DEGENERATE;

    const Geom_Number det = in_circle_determinant(a, b, c, p);
    if (det == 0)
      return InCircleResult::ON_CIRCLE;

    if (o == Orientation::CCW)
      return det > 0 ? InCircleResult::INSIDE : InCircleResult::OUTSIDE;

    return det < 0 ? InCircleResult::INSIDE : InCircleResult::OUTSIDE;
  }

  /// Return true if @p p lies on segment @p s (exact).
  [[nodiscard]] inline bool
  on_segment(const Segment & s, const Point & p)
  {
    return s.contains(p);
  }

  /// Return true if segments @p s1 and @p s2 intersect (including endpoints).
  [[nodiscard]] inline bool
  segments_intersect(const Segment & s1, const Segment & s2)
  {
    return s1.intersects_with(s2);
  }

  /// Return true if segments (a,b) and (c,d) intersect (including endpoints).
  [[nodiscard]] inline bool
  segments_intersect(const Point & a, const Point & b,
                     const Point & c, const Point & d)
  {
    return Segment(a, b).intersects_with(Segment(c, d));
  }

  /** Compute the exact intersection point of segments @p s1 and @p s2.
      @throws std::domain_error if segments do not intersect.
      @throws std::domain_error if the intersection set is not a unique point. */
  [[nodiscard]] inline Point
  segment_intersection_point(const Segment & s1, const Segment & s2)
  {
    ah_domain_error_if(not segments_intersect(s1, s2)) << "Segments do not intersect";

    const Point & a = s1.get_src_point();
    const Point & b = s1.get_tgt_point();
    const Point & c = s2.get_src_point();
    const Point & d = s2.get_tgt_point();

    // Degenerate-point cases may still have a unique intersection point.
    if (a == b and c == d)
      {
        ah_domain_error_if(a != c) << "Segments do not intersect";
        return a;
      }

    if (a == b)
      return a;

    if (c == d)
      return c;

    if (not s1.is_parallel_with(s2))
      return s1.intersection_with(s2);

    // Collinear/parallel intersections can still be unique (touching endpoint).
    Point unique_point;
    bool has_unique_point = false;
    auto accept_candidate = [&unique_point, &has_unique_point](const Point & p)
      {
        if (not has_unique_point)
          {
            unique_point = p;
            has_unique_point = true;
            return;
          }

        ah_domain_error_if(unique_point != p) << "No unique intersection point";
      };

    if (on_segment(s2, a))
      accept_candidate(a);
    if (on_segment(s2, b))
      accept_candidate(b);
    if (on_segment(s1, c))
      accept_candidate(c);
    if (on_segment(s1, d))
      accept_candidate(d);

    ah_domain_error_if(not has_unique_point) << "No unique intersection point";

    return unique_point;
  }

  /// Return the (unsigned) area of triangle (a, b, c) as an exact rational.
  [[nodiscard]] inline Geom_Number
  area_of_triangle(const Point & a, const Point & b, const Point & c)
  {
    return abs(area_of_parallelogram(a, b, c)) / 2;
  }

  // ============================================================================
  // 3D Primitives
  // ============================================================================

  /**
   * @brief Represents a point in 3D space with exact rational coordinates.
   * @ingroup Geometry
   *
   * Extends the 2D geometry module with a basic 3D point type.
   * All operations use exact `Geom_Number` arithmetic.
   */
  class Point3D
  {
    Geom_Number x_, y_, z_;

  public:
    /**
     * @brief Default constructor. Initializes a point at the origin (0,0,0).
     */
    Point3D() : x_(0), y_(0), z_(0) {}

    /**
     * @brief Constructs a 3D point from x, y, and z coordinates.
     * @param x The x-coordinate.
     * @param y The y-coordinate.
     * @param z The z-coordinate.
     */
    Point3D(const Geom_Number & x, const Geom_Number & y, const Geom_Number & z)
      : x_(x), y_(y), z_(z) {}

    Point3D(const Point3D &) = default;

    Point3D &operator=(const Point3D &) = default;

    /** @brief Gets the x-coordinate. */
    [[nodiscard]] const Geom_Number &get_x() const { return x_; }
    /** @brief Gets the y-coordinate. */
    [[nodiscard]] const Geom_Number &get_y() const { return y_; }
    /** @brief Gets the z-coordinate. */
    [[nodiscard]] const Geom_Number &get_z() const { return z_; }

    /**
     * @brief Checks for exact equality between two 3D points.
     * @param p The other point.
     * @return `true` if all three coordinates are identical.
     */
    [[nodiscard]] bool operator==(const Point3D & p) const
    {
      return x_ == p.x_ and y_ == p.y_ and z_ == p.z_;
    }

    /**
     * @brief Checks for inequality between two 3D points.
     * @param p The other point.
     * @return `true` if any coordinate is different.
     */
    [[nodiscard]] bool operator!=(const Point3D & p) const
    {
      return ! (*this == p);
    }

    /**
     * @brief Vector addition.
     * @param p The other point to add.
     * @return A new `Point3D` representing the component-wise sum.
     */
    [[nodiscard]] Point3D operator+(const Point3D & p) const
    {
      return {x_ + p.x_, y_ + p.y_, z_ + p.z_};
    }

    /**
     * @brief Vector subtraction.
     * @param p The other point to subtract.
     * @return A new `Point3D` representing the component-wise difference.
     */
    [[nodiscard]] Point3D operator-(const Point3D & p) const
    {
      return {x_ - p.x_, y_ - p.y_, z_ - p.z_};
    }

    /**
     * @brief Vector addition and assignment.
     * @param p The other point to add.
     * @return A reference to this modified point.
     */
    Point3D &operator+=(const Point3D & p)
    {
      x_ += p.x_;
      y_ += p.y_;
      z_ += p.z_;
      return *this;
    }

    /**
     * @brief Vector subtraction and assignment.
     * @param p The other point to subtract.
     * @return A reference to this modified point.
     */
    Point3D &operator-=(const Point3D & p)
    {
      x_ -= p.x_;
      y_ -= p.y_;
      z_ -= p.z_;
      return *this;
    }

    /**
     * @brief Unary negation.
     * @return A new `Point3D` with all coordinates negated.
     */
    [[nodiscard]] Point3D operator-() const
    {
      return {-x_, -y_, -z_};
    }

    /**
     * @brief Scalar multiplication.
     * @param s The scalar value.
     * @return A new `Point3D` with each coordinate multiplied by `s`.
     */
    [[nodiscard]] Point3D operator*(const Geom_Number & s) const
    {
      return {x_ * s, y_ * s, z_ * s};
    }

    /**
     * @brief Scalar division.
     * @param s The scalar value.
     * @return A new `Point3D` with each coordinate divided by `s`.
     */
    [[nodiscard]] Point3D operator/(const Geom_Number & s) const
    {
      return {x_ / s, y_ / s, z_ / s};
    }

    /**
     * @brief Dot product.
     * @param p The other vector.
     * @return The scalar result of the dot product.
     */
    [[nodiscard]] Geom_Number dot(const Point3D & p) const
    {
      return x_ * p.x_ + y_ * p.y_ + z_ * p.z_;
    }

    /**
     * @brief Cross product.
     * @param p The other vector.
     * @return A new `Point3D` representing the cross product vector.
     */
    [[nodiscard]] Point3D cross(const Point3D & p) const
    {
      return {
            y_ * p.z_ - z_ * p.y_,
            z_ * p.x_ - x_ * p.z_,
            x_ * p.y_ - y_ * p.x_
          };
    }

    /**
     * @brief Squared Euclidean distance to another point.
     * @param p The other point.
     * @return The squared distance.
     */
    [[nodiscard]] Geom_Number distance_squared_to(const Point3D & p) const
    {
      const Geom_Number dx = x_ - p.x_;
      const Geom_Number dy = y_ - p.y_;
      const Geom_Number dz = z_ - p.z_;
      return dx * dx + dy * dy + dz * dz;
    }

    /**
     * @brief Squared Euclidean norm (magnitude).
     * @return The squared length of the vector from the origin.
     */
    [[nodiscard]] Geom_Number norm_squared() const
    {
      return x_ * x_ + y_ * y_ + z_ * z_;
    }

    /**
     * @brief Euclidean norm (magnitude).
     * @return The length of the vector from the origin.
     */
    [[nodiscard]] Geom_Number norm() const
    {
      return square_root(norm_squared());
    }

    /**
     * @brief Euclidean distance to another point.
     * @param p The other point.
     * @return The distance.
     */
    [[nodiscard]] Geom_Number distance_to(const Point3D & p) const
    {
      return square_root(distance_squared_to(p));
    }

    /**
     * @brief Returns a normalized copy of this vector (unit vector).
     * @return A `Point3D` with a magnitude of 1.
     * @throws std::domain_error if normalizing the zero vector is attempted.
     */
    [[nodiscard]] Point3D normalize() const
    {
      const Geom_Number n = norm();
      ah_domain_error_if(n == 0) << "Cannot normalize zero Point3D";
      return *this / n;
    }

    /**
     * @brief Projects this 3D point to a 2D point by dropping the z-coordinate.
     * @return A `Point` with the x and y coordinates of this point.
     */
    [[nodiscard]] Point to_2d() const { return {x_, y_}; }

    /**
     * @brief Lifts a 2D point to 3D, setting its z-coordinate to 0.
     * @param p The 2D point.
     * @return A new `Point3D`.
     */
    [[nodiscard]] static Point3D from_2d(const Point & p)
    {
      return {p.get_x(), p.get_y(), Geom_Number(0)};
    }

    /**
     * @brief Lifts a 2D point to 3D with a specified z-coordinate.
     * @param p The 2D point.
     * @param z The value for the z-coordinate.
     * @return A new `Point3D`.
     */
    [[nodiscard]] static Point3D from_2d(const Point & p, const Geom_Number & z)
    {
      return {p.get_x(), p.get_y(), z};
    }
  };

  /// Scalar triple product: a · (b × c).
  [[nodiscard]] inline Geom_Number
  scalar_triple_product(const Point3D & a, const Point3D & b, const Point3D & c)
  {
    return a.dot(b.cross(c));
  }

  /**
   * @brief Represents a line segment in 3D space.
   * @ingroup Geometry
   */
  class Segment3D
  {
    Point3D src_, tgt_;

  public:
    /** @brief Default constructor. Initializes a zero-length segment at the origin. */
    Segment3D() = default;

    /**
     * @brief Constructs a 3D segment from two endpoints.
     * @param src The source point.
     * @param tgt The target point.
     */
    Segment3D(const Point3D & src, const Point3D & tgt) : src_(src), tgt_(tgt) {}

    /** @brief Gets the source point of the segment. */
    [[nodiscard]] const Point3D &get_src() const noexcept { return src_; }
    /** @brief Gets the target point of the segment. */
    [[nodiscard]] const Point3D &get_tgt() const noexcept { return tgt_; }
    /** @brief Gets the source point of the segment (alias for get_src). */
    [[nodiscard]] const Point3D &get_src_point() const noexcept { return src_; }
    /** @brief Gets the target point of the segment (alias for get_tgt). */
    [[nodiscard]] const Point3D &get_tgt_point() const noexcept { return tgt_; }

    /**
     * @brief Calculates the direction vector of the segment.
     * @return A `Point3D` representing the vector from source to target.
     */
    [[nodiscard]] Point3D direction() const { return tgt_ - src_; }

    /**
     * @brief Calculates the squared length of the segment.
     * @return The squared Euclidean distance between the endpoints.
     */
    [[nodiscard]] Geom_Number length_squared() const
    {
      return src_.distance_squared_to(tgt_);
    }

    /**
     * @brief Calculates the length of the segment.
     * @return The Euclidean distance between the endpoints.
     */
    [[nodiscard]] Geom_Number length() const
    {
      return src_.distance_to(tgt_);
    }

    /**
     * @brief Evaluates a point on the segment via linear interpolation.
     * @param t The interpolation parameter, clamped to [0, 1]. t=0 returns src, t=1 returns tgt.
     * @return The `Point3D` at parameter `t`.
     */
    [[nodiscard]] Point3D at(const Geom_Number & t) const
    {
      const Geom_Number s = Geom_Number(1) - t;
      return src_ * s + tgt_ * t;
    }

    /**
     * @brief Calculates the midpoint of the segment.
     * @return The `Point3D` at the midpoint.
     */
    [[nodiscard]] Point3D midpoint() const
    {
      return at(Geom_Number(1, 2));
    }

    /**
     * @brief Checks for equality between two segments.
     * @note Segments are considered equal if their endpoints are the same,
     *       regardless of direction (src/tgt).
     * @param s The other segment.
     * @return `true` if segments are equal, `false` otherwise.
     */
    [[nodiscard]] bool operator==(const Segment3D & s) const
    {
      return (src_ == s.src_ and tgt_ == s.tgt_) or
             (src_ == s.tgt_ and tgt_ == s.src_);
    }

    /**
     * @brief Checks for inequality.
     * @param s The other segment.
     * @return `true` if source or target points are different.
     */
    [[nodiscard]] bool operator!=(const Segment3D & s) const
    {
      return not (*this == s);
    }

    /**
     * @brief Checks if a point lies on the segment.
     * @param p The point to check.
     * @return `true` if the point is collinear and between the endpoints.
     */
    [[nodiscard]] bool contains(const Point3D & p) const
    {
      const Point3D d = direction();
      const Point3D w = p - src_;

      if (d == Point3D(0, 0, 0))
        return p == src_; // Segment is a point

      // Check for collinearity using cross product
      if (d.cross(w) != Point3D(0, 0, 0))
        return false;

      // Check if point is within the segment bounds using dot product
      const Geom_Number dot = w.dot(d);
      return dot >= 0 && dot <= d.dot(d);
    }

    /**
     * @brief Calculates the shortest distance from a point to this segment.
     * @param p The point.
     * @return The Euclidean distance to the closest point on the segment.
     */
    [[nodiscard]] Geom_Number distance_to(const Point3D & p) const
    {
      const Point3D d = direction();
      const Geom_Number len2 = d.dot(d);

      if (len2 == 0)
        return p.distance_to(src_); // Segment is a point

      // Project p onto the line defined by the segment
      // t = ((p - src) . d) / |d|^2
      Geom_Number t = (p - src_).dot(d) / len2;

      // Clamp t to the [0, 1] interval to stay on the segment
      if (t < 0)
        t = 0;
      else if (t > 1)
        t = 1;

      const Point3D proj = at(t);
      return p.distance_to(proj);
    }
  };

  /**
   * @brief Represents a triangle in 3D space defined by three points.
   * @ingroup Geometry
   */
  class Triangle3D
  {
    Point3D p1_, p2_, p3_;

  public:
    /** @brief Default constructor. Initializes a degenerate triangle at the origin. */
    Triangle3D() = default;

    /**
     * @brief Constructs a 3D triangle from three vertices.
     * @param p1 The first vertex.
     * @param p2 The second vertex.
     * @param p3 The third vertex.
     */
    Triangle3D(const Point3D & p1, const Point3D & p2, const Point3D & p3)
      : p1_(p1), p2_(p2), p3_(p3) {}

    /** @brief Gets the first vertex of the triangle. */
    [[nodiscard]] const Point3D &get_p1() const { return p1_; }
    /** @brief Gets the second vertex of the triangle. */
    [[nodiscard]] const Point3D &get_p2() const { return p2_; }
    /** @brief Gets the third vertex of the triangle. */
    [[nodiscard]] const Point3D &get_p3() const { return p3_; }

    /**
     * @brief Computes the normal vector of the triangle's plane.
     * @details The normal is calculated as `(p2-p1) x (p3-p1)`.
     *          Its magnitude is twice the triangle's area. The direction
     *          depends on the winding order of the vertices.
     * @return A `Point3D` representing the normal vector (not normalized).
     */
    [[nodiscard]] Point3D normal() const
    {
      return (p2_ - p1_).cross(p3_ - p1_);
    }

    /**
     * @brief Calculates twice the squared area of the triangle.
     * @return `2 * area^2`, which is equal to `normal().norm_squared() / 2`.
     */
    [[nodiscard]] Geom_Number double_area_squared() const
    {
      return normal().norm_squared() / Geom_Number(2);
    }

    /**
     * @brief Computes the centroid (center of mass) of the triangle.
     * @return A `Point3D` representing the centroid.
     */
    [[nodiscard]] Point3D centroid() const
    {
      return (p1_ + p2_ + p3_) / Geom_Number(3);
    }

    /**
     * @brief Checks if the triangle is degenerate (i.e., its vertices are collinear).
     * @return `true` if the area is zero, `false` otherwise.
     */
    [[nodiscard]] bool is_degenerate() const
    {
      return normal().norm_squared() == 0;
    }

    /**
     * @brief Structure to hold barycentric coordinates.
     * @details Barycentric coordinates `(u, v, w)` represent a point `p` as a
     *          weighted average of the triangle's vertices:
     *          `p = u*p1 + v*p2 + w*p3`, where `u+v+w = 1`.
     */
    struct BaryCoords
    {
      Geom_Number u, v, w;
    };

    /**
     * @brief Computes the barycentric coordinates of a point `p` with respect to this triangle.
     * @param p The point to find coordinates for.
     * @return A `BaryCoords` struct containing the coordinates (u, v, w).
     * @throws std::domain_error if the triangle is degenerate.
     */
    [[nodiscard]] BaryCoords barycentric(const Point3D & p) const
    {
      ah_domain_error_if(is_degenerate())
      << "Barycentric coordinates undefined for degenerate triangle";

      const Point3D v0 = p2_ - p1_, v1 = p3_ - p1_, v2 = p - p1_;
      const Geom_Number d00 = v0.dot(v0);
      const Geom_Number d01 = v0.dot(v1);
      const Geom_Number d11 = v1.dot(v1);
      const Geom_Number d20 = v2.dot(v0);
      const Geom_Number d21 = v2.dot(v1);
      const Geom_Number denom = d00 * d11 - d01 * d01;
      ah_domain_error_if(denom == 0)
      << "Barycentric coordinates undefined for degenerate triangle";
      const Geom_Number v = (d11 * d20 - d01 * d21) / denom;
      const Geom_Number w = (d00 * d21 - d01 * d20) / denom;
      return {Geom_Number(1) - v - w, v, w};
    }
  };

  /**
   * @brief Represents a tetrahedron in 3D space defined by four points.
   * @ingroup Geometry
   */
  class Tetrahedron
  {
    Point3D p1_, p2_, p3_, p4_;

  public:
    /** @brief Default constructor. Initializes a degenerate tetrahedron at the origin. */
    Tetrahedron() = default;

    /**
     * @brief Constructs a tetrahedron from four vertices.
     * @param p1 The first vertex.
     * @param p2 The second vertex.
     * @param p3 The third vertex.
     * @param p4 The fourth vertex.
     */
    Tetrahedron(const Point3D & p1, const Point3D & p2,
                const Point3D & p3, const Point3D & p4)
      : p1_(p1), p2_(p2), p3_(p3), p4_(p4) {}

    /** @brief Gets the first vertex. */
    [[nodiscard]] const Point3D &get_p1() const { return p1_; }
    /** @brief Gets the second vertex. */
    [[nodiscard]] const Point3D &get_p2() const { return p2_; }
    /** @brief Gets the third vertex. */
    [[nodiscard]] const Point3D &get_p3() const { return p3_; }
    /** @brief Gets the fourth vertex. */
    [[nodiscard]] const Point3D &get_p4() const { return p4_; }

    /**
     * @brief Calculates 6 times the signed volume of the tetrahedron.
     * @details The value is computed using the scalar triple product:
     *          `(p2-p1) . ((p3-p1) x (p4-p1))`.
     *          The sign indicates the orientation of the vertices.
     * @return 6 times the signed volume.
     */
    [[nodiscard]] Geom_Number signed_volume_x6() const
    {
      return scalar_triple_product(p2_ - p1_, p3_ - p1_, p4_ - p1_);
    }

    /**
     * @brief Calculates the unsigned volume of the tetrahedron.
     * @return The absolute volume.
     */
    [[nodiscard]] Geom_Number volume() const
    {
      Geom_Number v = signed_volume_x6();
      if (v < 0) v = -v;
      return v / Geom_Number(6);
    }

    /**
     * @brief Checks if the tetrahedron is degenerate (i.e., its vertices are coplanar).
     * @return `true` if the volume is zero, `false` otherwise.
     */
    [[nodiscard]] bool is_degenerate() const
    {
      return signed_volume_x6() == 0;
    }

    /**
     * @brief Computes the centroid (center of mass) of the tetrahedron.
     * @return A `Point3D` representing the centroid.
     */
    [[nodiscard]] Point3D centroid() const
    {
      return (p1_ + p2_ + p3_ + p4_) / Geom_Number(4);
    }

    /**
     * @brief Checks if a point lies inside the tetrahedron.
     * @details This test is based on checking the sign of the volumes of the
     *          four sub-tetrahedra formed by the point and each face.
     * @param p The point to check.
     * @return `true` if the point is inside or on the boundary of the tetrahedron.
     */
    [[nodiscard]] bool contains(const Point3D & p) const
    {
      auto signed_volume_x6_of = [](const Point3D & a, const Point3D & b,
                                    const Point3D & c, const Point3D & d)
        {
          return scalar_triple_product(b - a, c - a, d - a);
        };

      const Geom_Number d0 = signed_volume_x6();
      if (d0 == 0) return false; // Not contained in a degenerate tetrahedron

      // Keep vertex order consistent with d0 by replacing one vertex at a time.
      const Geom_Number d1 = signed_volume_x6_of(p, p2_, p3_, p4_);
      const Geom_Number d2 = signed_volume_x6_of(p1_, p, p3_, p4_);
      const Geom_Number d3 = signed_volume_x6_of(p1_, p2_, p, p4_);
      const Geom_Number d4 = signed_volume_x6_of(p1_, p2_, p3_, p);

      // Point is inside iff all sub-volumes have the same sign as d0.
      if (d0 > 0)
        return d1 >= 0 and d2 >= 0 and d3 >= 0 and d4 >= 0;
      return d1 <= 0 and d2 <= 0 and d3 <= 0 and d4 <= 0;
    }

    /**
     * @brief A struct holding the four faces of the tetrahedron.
     */
    struct Faces
    {
      Triangle3D f[4];
    };

    /**
     * @brief Gets the four faces of the tetrahedron.
     * @return A `Faces` struct containing four `Triangle3D` objects.
     */
    [[nodiscard]] Faces faces() const
    {
      return {
            {
              Triangle3D(p1_, p2_, p3_),
              Triangle3D(p1_, p2_, p4_),
              Triangle3D(p1_, p3_, p4_),
              Triangle3D(p2_, p3_, p4_)
            }
          };
    }
  };

  // ============================================================================
  // Stream output operators for geometry classes
  // ============================================================================

  inline std::ostream &operator<<(std::ostream & o, const Point & p)
  {
    o << "Point(" << p.get_x() << ", " << p.get_y() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Segment & s)
  {
    o << "Segment(" << s.get_src_point() << " -> " << s.get_tgt_point() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Triangle & t)
  {
    o << "Triangle(" << t.get_p1() << ", " << t.get_p2()
        << ", " << t.get_p3() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Rectangle & r)
  {
    o << "Rectangle" << r.to_string();
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Ellipse & e)
  {
    o << e.to_string();
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const RotatedEllipse & e)
  {
    o << e.to_string();
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Point3D & p)
  {
    o << "Point3D(" << p.get_x() << ", " << p.get_y()
        << ", " << p.get_z() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Segment3D & s)
  {
    o << "Segment3D(" << s.get_src() << " -> " << s.get_tgt() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Triangle3D & t)
  {
    o << "Triangle3D(" << t.get_p1() << ", " << t.get_p2()
        << ", " << t.get_p3() << ")";
    return o;
  }

  inline std::ostream &operator<<(std::ostream & o, const Tetrahedron & t)
  {
    o << "Tetrahedron(" << t.get_p1() << ", " << t.get_p2()
        << ", " << t.get_p3() << ", " << t.get_p4() << ")";
    return o;
  }
} // namespace Aleph

namespace std
{
  template <>
  struct hash<Aleph::Point>
  {
    std::size_t operator()(const Aleph::Point & p) const
    {
      const std::size_t hx = std::hash<std::string>{}(p.get_x().get_str());
      const std::size_t hy = std::hash<std::string>{}(p.get_y().get_str());
      return hx ^ (hy + 0x9e3779b97f4a7c15ULL + (hx << 6) + (hx >> 2));
    }
  };
} // namespace std

// ============================================================================
// std::format support (C++20)
// ============================================================================

#if __cplusplus >= 202002L && __has_include(<format>)
# include <format>

# if defined(__cpp_lib_format)

template <>
struct std::formatter<Aleph::Point> : std::formatter<std::string>
{
  auto format(const Aleph::Point & p, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Point({}, {})",
                                                           Aleph::geom_number_to_double(p.get_x()),
                                                           Aleph::geom_number_to_double(p.get_y())), ctx);
  }
};

template <>
struct std::formatter<Aleph::Segment> : std::formatter<std::string>
{
  auto format(const Aleph::Segment & s, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Segment({} -> {})",
                                                           s.get_src_point(), s.get_tgt_point()), ctx);
  }
};

template <>
struct std::formatter<Aleph::Triangle> : std::formatter<std::string>
{
  auto format(const Aleph::Triangle & t, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Triangle({}, {}, {})",
                                                           t.get_p1(), t.get_p2(), t.get_p3()), ctx);
  }
};

template <>
struct std::formatter<Aleph::Rectangle> : std::formatter<std::string>
{
  auto format(const Aleph::Rectangle & r, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Rectangle({}, {} -- {}, {})",
                                                           Aleph::geom_number_to_double(r.get_xmin()),
                                                           Aleph::geom_number_to_double(r.get_ymin()),
                                                           Aleph::geom_number_to_double(r.get_xmax()),
                                                           Aleph::geom_number_to_double(r.get_ymax())), ctx);
  }
};

template <>
struct std::formatter<Aleph::Polar_Point> : std::formatter<std::string>
{
  auto format(const Aleph::Polar_Point & p, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("PolarPoint(r={}, theta={})",
                                                           Aleph::geom_number_to_double(p.get_r()),
                                                           Aleph::geom_number_to_double(p.get_theta())), ctx);
  }
};

template <>
struct std::formatter<Aleph::Ellipse> : std::formatter<std::string>
{
  auto format(const Aleph::Ellipse & e, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("{}",
                                                           e.to_string()), ctx);
  }
};

template <>
struct std::formatter<Aleph::RotatedEllipse> : std::formatter<std::string>
{
  auto format(const Aleph::RotatedEllipse & e, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("{}",
                                                           e.to_string()), ctx);
  }
};

template <>
struct std::formatter<Aleph::Point3D> : std::formatter<std::string>
{
  auto format(const Aleph::Point3D & p, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Point3D({}, {}, {})",
                                                           Aleph::geom_number_to_double(p.get_x()),
                                                           Aleph::geom_number_to_double(p.get_y()),
                                                           Aleph::geom_number_to_double(p.get_z())), ctx);
  }
};

template <>
struct std::formatter<Aleph::Segment3D> : std::formatter<std::string>
{
  auto format(const Aleph::Segment3D & s, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Segment3D({} -> {})",
                                                           s.get_src(), s.get_tgt()), ctx);
  }
};

template <>
struct std::formatter<Aleph::Triangle3D> : std::formatter<std::string>
{
  auto format(const Aleph::Triangle3D & t, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Triangle3D({}, {}, {})",
                                                           t.get_p1(), t.get_p2(), t.get_p3()), ctx);
  }
};

template <>
struct std::formatter<Aleph::Tetrahedron> : std::formatter<std::string>
{
  auto format(const Aleph::Tetrahedron & t, std::format_context & ctx) const
  {
    return std::formatter<std::string>::format(
                                               std::format("Tetrahedron({}, {}, {}, {})",
                                                           t.get_p1(), t.get_p2(), t.get_p3(), t.get_p4()), ctx);
  }
};

# endif // __cpp_lib_format
#endif // C++20 format

# endif // POINT_H
