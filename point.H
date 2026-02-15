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
  typedef mpq_class Geom_Number;

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

  /** Rectangular point in the plane.

  Fundamental class which defines a point in a Cartesian coordinates
  plane.

  @ingroup Geometry
  */
  class Point : public Geom_Object
  {
    friend class Segment;
    friend class Triangle;
    friend class Polar_Point;

    Geom_Number x_;
    Geom_Number y_;

  public:
    Point() : Geom_Object(), x_(0), y_(0)
    { /* empty */
    }

    /// Builds a new point in coordinates (__x, __y)
    Point(const Geom_Number & x, const Geom_Number & y)
      : Geom_Object(), x_(x), y_(y)
    {
      // empty
    }

    /// Builds a copy of p.
    // Point(const Point & p) : Geom_Object(*this), x(p.x), y(p.y)
    // {
    //   // empty
    // }

    /// Builds a new point from polar coordinates.
    inline Point(const Polar_Point & pp);

    [[nodiscard]] bool operator ==(const Point & point) const noexcept
    {
      return x_ == point.x_ and y_ == point.y_;
    }

    [[nodiscard]] bool operator !=(const Point & point) const noexcept
    {
      return not (*this == point);
    }

    [[nodiscard]] bool operator <(const Point & point) const noexcept
    {
      return x_ < point.x_ or (x_ == point.x_ and y_ < point.y_);
    }

    [[nodiscard]] Point operator +(const Point & p) const
    {
      return {x_ + p.x_, y_ + p.y_};
    }

    Point &operator +=(const Point & p)
    {
      x_ += p.x_;
      y_ += p.y_;

      return *this;
    }

    Point operator -(const Point & p) const
    {
      return {x_ - p.x_, y_ - p.y_};
    }

    Point &operator -=(const Point & p)
    {
      x_ -= p.x_;
      y_ -= p.y_;

      return *this;
    }

    /// Unary negation.
    [[nodiscard]] Point operator -() const
    {
      return {-x_, -y_};
    }

    /// Scalar multiplication.
    [[nodiscard]] Point operator *(const Geom_Number & s) const
    {
      return {x_ * s, y_ * s};
    }

    /// Scalar division.
    [[nodiscard]] Point operator /(const Geom_Number & s) const
    {
      return {x_ / s, y_ / s};
    }

    /// Dot product.
    [[nodiscard]] Geom_Number dot(const Point & p) const
    {
      return x_ * p.x_ + y_ * p.y_;
    }

    /// 2D cross product (z-component of the 3D cross product).
    [[nodiscard]] Geom_Number cross(const Point & p) const
    {
      return x_ * p.y_ - y_ * p.x_;
    }

    /// Squared Euclidean norm.
    [[nodiscard]] Geom_Number norm_squared() const
    {
      return x_ * x_ + y_ * y_;
    }

    /// Euclidean norm.
    [[nodiscard]] Geom_Number norm() const
    {
      return euclidean_distance(x_, y_);
    }

    /// Return a normalized copy of this vector.
    [[nodiscard]] Point normalize() const
    {
      const Geom_Number n = norm();
      ah_domain_error_if(n == 0) << "Cannot normalize the zero vector";
      return *this / n;
    }

    /// Rotate this point around the origin by @p angle (radians).
    [[nodiscard]] Point rotate(const Geom_Number & angle) const
    {
      const Geom_Number c = cosinus(angle);
      const Geom_Number s = sinus(angle);
      return {x_ * c - y_ * s, x_ * s + y_ * c};
    }

    /// Linear interpolation between this point and @p other.
    [[nodiscard]] Point lerp(const Point & other, const Geom_Number & t) const
    {
      const Geom_Number one_minus_t = Geom_Number(1) - t;
      return *this * one_minus_t + other * t;
    }

    /// Midpoint between this point and @p other.
    [[nodiscard]] Point midpoint(const Point & other) const
    {
      return lerp(other, Geom_Number(1, 2));
    }

    [[nodiscard]] const Geom_Number &get_x() const noexcept /// Returns x value
    {
      return x_;
    }

    [[nodiscard]] const Geom_Number &get_y() const noexcept /// Returns y value
    {
      return y_;
    }

    /// Returns true if this is colinear with p1 and p2.
    [[nodiscard]] bool is_colinear_with(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(*this, p1, p2) == 0;
    }

    /// Returns true if this is colinear with a segment s.
    [[nodiscard]] inline bool is_colinear_with(const Segment & s) const;

    /// Return true if this is to the left of the line from p1 to p2.
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

    /// Return true if this is to right from points p1 and p2
    [[nodiscard]] bool is_to_right_from(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(p1, p2, *this) < 0;
    }

    /// Return true if this is to the left from (or on) points p1 and p2
    [[nodiscard]] bool is_to_left_on_from(const Point & p1, const Point & p2) const
    {
      return not is_to_right_from(p1, p2);
    }

    /// Return true if this is to the right of (or on) the line from p1 to p2.
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

    /// Returns true if the sequence this-p1-p2 is clockwise.
    [[nodiscard]] bool is_clockwise_with(const Point & p1, const Point & p2) const
    {
      return area_of_parallelogram(*this, p1, p2) < 0;
    }

    [[nodiscard]] inline bool is_left_of(const Segment & s) const;

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

    [[nodiscard]] inline bool is_clockwise_with(const Segment & s) const;

    /// Returns true if this is between p1 and p2.
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

    /// Return the nearest point (to this) between p1 and p2.
    [[nodiscard]] const Point &nearest_point(const Point & p1, const Point & p2) const
    {
      return this->distance_squared_to(p1) < this->distance_squared_to(p2) ? p1 : p2;
    }

    /// Returns true if this is inside a segment s.
    [[nodiscard]] inline bool is_inside(const Segment & s) const;

    /// Returns true if this is inside the ellipse e.
    [[nodiscard]] inline bool is_inside(const Ellipse & e) const;

    /// Returns true if this intersects ellipse @p e.
    [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

    /// Return a string representation of this.
    [[nodiscard]] std::string to_string() const
    {
      return "(" + std::to_string(geom_number_to_double(x_)) + "," +
             std::to_string(geom_number_to_double(y_)) + ")";
    }

    /// String cast operator.
    operator std::string() const { return to_string(); }

    /// Returns the square distance between this y that
    [[nodiscard]] inline Geom_Number distance_squared_to(const Point & that) const;

    /// Returns the Euclidean distance between this and p.
    [[nodiscard]] inline Geom_Number distance_to(const Point & p) const;

    /// @deprecated Use distance_to() instead.
    [[deprecated("Use distance_to() instead")]]
    [[nodiscard]] Geom_Number distance_with(const Point & p) const
    {
      return distance_to(p);
    }

    [[nodiscard]] const Point &highest_point() const { return *this; }

    [[nodiscard]] const Point &lowest_point() const { return *this; }

    [[nodiscard]] const Point &leftmost_point() const { return *this; }

    [[nodiscard]] const Point &rightmost_point() const { return *this; }
  };

  /// Scalar multiplication with commutative order.
  [[nodiscard]] inline Point operator*(const Geom_Number & s, const Point & p)
  {
    return p * s;
  }


  /** @brief Polar representation of a 2D point.
   *
   * Polar coordinates are convenient for algorithms that rely on angles or
   * rotations. Instances can be constructed from rectangular coordinates and are
   * convertible back into @ref Point objects.
   *
   * @ingroup Geometry
   *  @author Leandro Rabindranath León
   */
  class Polar_Point : public Geom_Object
  {
    friend class Point;

    Geom_Number r_ = 0; // distance from origin
    Geom_Number theta_ = 0; // angle in radians from positive x-axis

  public:
    /// Returns the magnitude
    [[nodiscard]] const Geom_Number &get_r() const { return r_; }

    /// Returns the angle.
    [[nodiscard]] const Geom_Number &get_theta() const { return theta_; }

    Polar_Point(const Geom_Number & r, const Geom_Number & theta)
      : r_(r), theta_(theta)
    {
      // empty
    }

    /// Construct from Cartesian coordinates (origin at (0,0)).
    explicit Polar_Point(const Point & p)
    {
      const Geom_Number & x = p.x_;
      const Geom_Number & y = p.y_;

      r_ = euclidean_distance(x, y);
      theta_ = arctan2(y, x);
    }

    /// Enumerate polar quadrants in counterclockwise order.
    enum Quadrant { First, Second, Third, Fourth };

    /// Return the quadrant where the point lies (origin defaults to First).
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

    /// Convert to string representation "[r,theta]".
    [[nodiscard]] std::string to_string() const
    {
      return "[" + std::to_string(geom_number_to_double(r_)) + "," +
             std::to_string(geom_number_to_double(theta_)) + "]";
    }

    Polar_Point() = default;
  };


  inline
  Point::Point(const Polar_Point & pp)
    : x_(pp.r_ * cosinus(pp.theta_)), y_(pp.r_ * sinus(pp.theta_))
  {
    // empty
  }


  /** @brief Line segment between two points.
   *
   * Represents an (undirected) segment in the plane, stored as source/target
   * endpoints.
   *
   * @ingroup Geometry
   */
  class Segment : public Geom_Object
  {
    friend class Point;
    friend class Triangle;

    Point src_, tgt_;

    [[nodiscard]] double compute_slope() const
    {
      if (tgt_.x_ == src_.x_)
        {
          if (src_.y_ < tgt_.y_)
            return std::numeric_limits<double>::max();
          return -std::numeric_limits<double>::max();
        }

      const Geom_Number slope_ = (tgt_.y_ - src_.y_) / (tgt_.x_ - src_.x_);

      return slope_.get_d();
    }

  public:
    [[nodiscard]] bool operator ==(const Segment & s) const noexcept
    {
      return (src_ == s.src_ and tgt_ == s.tgt_) or
             (src_ == s.tgt_ and tgt_ == s.src_);
    }

    [[nodiscard]] bool operator !=(const Segment & s) const noexcept
    {
      return not (*this == s);
    }

    [[nodiscard]] const Point &highest_point() const
    {
      return src_.y_ > tgt_.y_ ? src_ : tgt_;
    }

    [[nodiscard]] const Point &lowest_point() const
    {
      return src_.y_ < tgt_.y_ ? src_ : tgt_;
    }

    [[nodiscard]] const Point &leftmost_point() const
    {
      return src_.x_ < tgt_.x_ ? src_ : tgt_;
    }

    [[nodiscard]] const Point &rightmost_point() const
    {
      return src_.x_ > tgt_.x_ ? src_ : tgt_;
    }

    [[nodiscard]] const Point &get_src_point() const { return src_; }

    [[nodiscard]] const Point &get_tgt_point() const { return tgt_; }

    Segment() = default;

    Segment(Point src, Point tgt)
      : Geom_Object(), src_(std::move(src)), tgt_(std::move(tgt))
    {
      // empty
    }

  private:
    /** Computes the target point of a segment given the source point, slope and
        length.

        It uses the solution of these equations:
        - d^2 = (tx-sx)^2 + (ty-sy)^2  (Pythagorean theorem)
        - ty - sy = m(tx-sx)           (line equation)

        @return The point placed to the east (right).
    */
    static
    Point compute_tgt_point(const Point & src, // Point of origin
                            const Geom_Number & m, // slope
                            const Geom_Number & d) // segment length
    {
      const Geom_Number den2 = 1 + m * m;

      const Geom_Number den = square_root(den2);

      const Geom_Number x = src.x_ + d / den;

      const Geom_Number y = src.y_ + d * m / den;

      return {x, y};
    }

  public:
    /** Builds a new segment given source point, slope, and length.

        It computes the source point by using these equations:
       -#\f$ d^2 = (x - x_1)^2 + (y - y_1)^2\f$ (Pythagoras)
       -#\f$ y - y_1 = m(x - x_1)\f$ (line equation given point and slope)

       @param[in] src Source point
       @param[in] m slope (rise over run, dimensionless)
       @param[in] l length
    */
    Segment(Point src, // source point
            const Geom_Number & m, // slope
            const Geom_Number & l) // segment length
      : Geom_Object(), src_(std::move(src)), tgt_(compute_tgt_point(src_, m, l))
    {
      // empty
    }

    /// Builds a new segment parallel to sg and with a distance dist.
    Segment(const Segment & sg, const Geom_Number & dist)
    {
      const Segment perp = sg.mid_perpendicular(dist);

      const Point mid_point = sg.mid_point();

      const Point diff_point = mid_point - perp.get_src_point();

      src_ = sg.get_src_point() + diff_point;
      tgt_ = sg.get_tgt_point() + diff_point;
    }

    [[nodiscard]] double slope() const
    {
      return compute_slope();
    }

    /// Exact slope as Geom_Number. Undefined for vertical segments.
    [[nodiscard]] Geom_Number slope_exact() const
    {
      ah_domain_error_if(tgt_.x_ == src_.x_) << "Vertical segment has undefined slope";
      return (tgt_.y_ - src_.y_) / (tgt_.x_ - src_.x_);
    }

    /// Compute the counterclockwise wise angle respect to another segment s.
    [[nodiscard]] double counterclockwise_angle_with(const Segment & s) const
    {
      const Geom_Number x1 = tgt_.x_ - src_.x_;
      const Geom_Number x2 = s.tgt_.x_ - s.src_.x_;
      const Geom_Number y1 = tgt_.y_ - src_.y_;
      const Geom_Number y2 = s.tgt_.y_ - s.src_.y_;
      const Geom_Number dot = x1 * x2 + y1 * y2;
      const Geom_Number det = x1 * y2 - y1 * x2;

      const Geom_Number angle = arctan2(det, dot);

      if (angle < 0)
        return geom_number_to_double(angle + 2 * geom_pi());

      return angle.get_d();
    }

    /// Compute the counterclockwise angle of this respect x-axis.
    [[nodiscard]] double counterclockwise_angle() const
    {
      const Segment x_axis(Point(0, 0), Point(1, 0));
      return x_axis.counterclockwise_angle_with(*this);
    }

    /// Return the Euclidean length of the segment (distance between endpoints).
    [[nodiscard]] Geom_Number length() const
    {
      return euclidean_distance(tgt_.x_ - src_.x_, tgt_.y_ - src_.y_);
    }

    /// @deprecated Use length() instead.
    [[deprecated("Use length() instead")]]
    [[nodiscard]] Geom_Number size() const { return length(); }

    /// Return true if @p p is colinear with this segment.
    [[nodiscard]] bool is_colinear_with(const Point & p) const
    {
      return p.is_colinear_with(src_, tgt_);
    }

    /// Return true if this segment is to the left of point @p p.
    [[nodiscard]] bool is_left_of(const Point & p) const
    {
      return p.is_right_of(*this);
    }

    /// Return true if this segment is to the right of point @p p.
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

    /// Return the midpoint of this segment.
    [[nodiscard]] Point mid_point() const
    {
      const Geom_Number x = (src_.get_x() + tgt_.get_x()) / 2;
      const Geom_Number y = (src_.get_y() + tgt_.get_y()) / 2;

      return {x, y};
    }

    /// Return the same segment with endpoints swapped.
    [[nodiscard]] Segment reversed() const
    {
      return {tgt_, src_};
    }

    /// Evaluate this segment at parameter @p t in [0, 1].
    [[nodiscard]] Point at(const Geom_Number & t) const
    {
      const Geom_Number one_minus_t = Geom_Number(1) - t;
      return src_ * one_minus_t + tgt_ * t;
    }

    /// Orthogonal projection of @p p onto the segment (clamped to endpoints).
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

    /// Euclidean distance from @p p to this segment.
    [[nodiscard]] Geom_Number distance_to(const Point & p) const
    {
      return p.distance_to(project(p));
    }


    /// Return whichever endpoint is nearer to @p p.
    [[nodiscard]] const Point &nearest_point(const Point & p) const
    {
      return p.nearest_point(get_src_point(), get_tgt_point());
    }

    /// Construct the segment perpendicular to @c this passing through @p p.
    [[nodiscard]] Segment get_perpendicular(const Point & p) const
    {
      if (src_ == tgt_)
        return *this;

      const Geom_Number dx = tgt_.x_ - src_.x_;
      const Geom_Number dy = tgt_.y_ - src_.y_;

      // Special-case horizontal segments so we can return a vertical segment.
      if (dy == 0)
        return {Point(p.get_x(), src_.get_y()), p};

      // Likewise for vertical segments.
      if (dx == 0)
        return {Point(src_.get_x(), p.get_y()), p};

      // Project p onto the line through src-tgt using dot product.
      // t = dot(p - src, tgt - src) / dot(tgt - src, tgt - src)
      const Geom_Number dot_num =
          (p.x_ - src_.x_) * dx + (p.y_ - src_.y_) * dy;
      const Geom_Number dot_den = dx * dx + dy * dy;
      const Geom_Number t = dot_num / dot_den;

      const Geom_Number fx = src_.x_ + t * dx;
      const Geom_Number fy = src_.y_ + t * dy;

      // The perpendicular segment goes from the foot to p.
      return {Point(fx, fy), p};
    }

    /// Return the perpendicular chord of length @c 2*dist through the midpoint.
    [[nodiscard]] Segment mid_perpendicular(const Geom_Number & dist) const
    {
      // Translate target to the origin and convert to polar coordinates.
      const Polar_Point tgt_polar(tgt_ - src_);
      ah_domain_error_if(tgt_polar.get_r() == 0)
      << "Cannot build a midpoint perpendicular of a zero-length segment";

      const Geom_Number & arc_tgt_src = tgt_polar.get_theta();

      Geom_Number arc_perp_pt = arctan(dist / (tgt_polar.get_r() / 2));

      // Distance from src to the perpendicular point.
      Geom_Number perp_r = euclidean_distance(dist, tgt_polar.get_r() / 2);

      if (tgt_polar.get_r() < 0)
        perp_r = -perp_r;

      if (tgt_polar.get_theta() < 0)
        arc_perp_pt = -arc_perp_pt;

      // Perpendicular point to the left of the segment (polar coordinates).
      const Polar_Point polar_perp_pt_l(perp_r, arc_tgt_src + arc_perp_pt);

      // Perpendicular point to the right of the segment (polar coordinates).
      const Polar_Point polar_perp_pt_r(perp_r, arc_tgt_src - arc_perp_pt);

      // Convert back to rectangular coordinates.
      const Point p1(Point(polar_perp_pt_l) + src_);
      const Point p2(Point(polar_perp_pt_r) + src_);

      // Return the segment in counterclockwise order with respect to @c this.
      if (p1.is_right_of(*this))
        return {p1, p2};
      return {p2, p1};
    }

    /// Return true if the segments intersect at interior points only.
    [[nodiscard]] bool intersects_properly_with(const Segment & s) const
    {
      // Reject degenerate colinear configurations first.
      if (src_.is_colinear_with(s) or tgt_.is_colinear_with(s) or
          s.src_.is_colinear_with(*this) or s.tgt_.is_colinear_with(*this))
        return false;

      // There is a proper intersection when each segment straddles the other.
      return (src_.is_left_of(s) xor tgt_.is_left_of(s)) and
             (s.src_.is_left_of(*this) xor s.tgt_.is_left_of(*this));
    }

    /// Return true if @p p lies on this segment.
    [[nodiscard]] bool contains(const Point & p) const
    {
      return p.is_between(src_, tgt_);
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Point & p) const { return contains(p); }

    /// Return true if @p s lies entirely inside this segment.
    [[nodiscard]] bool contains(const Segment & s) const
    {
      return (s.get_src_point().is_between(src_, tgt_) and
              s.get_tgt_point().is_between(src_, tgt_));
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Segment & s) const { return contains(s); }

    /// Return true if @p s intersects this segment (including endpoints).
    [[nodiscard]] bool intersects_with(const Segment & s) const
    {
      if (this->intersects_properly_with(s))
        return true;

      return this->contains(s.src_) or this->contains(s.tgt_) or
             s.contains(this->src_) or s.contains(this->tgt_);
    }

    [[nodiscard]] inline bool intersects_with(const Triangle & t) const;

    [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

    /// Return true if @p s has the same direction (exact cross-product test).
    [[nodiscard]] bool is_parallel_with(const Segment & s) const
    {
      return (tgt_.x_ - src_.x_) * (s.tgt_.y_ - s.src_.y_) ==
             (tgt_.y_ - src_.y_) * (s.tgt_.x_ - s.src_.x_);
    }

    /// Compute intersection point using exact determinant formula.
    [[nodiscard]] Point intersection_with(const Segment & s) const
    {
      const Geom_Number denom = (tgt_.x_ - src_.x_) * (s.tgt_.y_ - s.src_.y_) -
                                (tgt_.y_ - src_.y_) * (s.tgt_.x_ - s.src_.x_);

      ah_domain_error_if(denom == 0) << "Segments are parallel";

      const Geom_Number t = ((s.src_.x_ - src_.x_) * (s.tgt_.y_ - s.src_.y_) -
                             (s.src_.y_ - src_.y_) * (s.tgt_.x_ - s.src_.x_)) / denom;

      return {
            src_.x_ + t * (tgt_.x_ - src_.x_),
            src_.y_ + t * (tgt_.y_ - src_.y_)
          };
    }

    // TODO: consider polar-based sense calculation to avoid branching.

    /// Cardinal directions associated with a segment.
    enum Sense { E, NE, N, NW, W, S, SW, SE };

    [[nodiscard]] Sense sense() const
    {
      if (src_.x_ < tgt_.x_) // headed east?
        {
          if (src_.y_ < tgt_.y_) // and north?
            return NE;
          if (src_.y_ > tgt_.y_) // and south?
            return SE;
          return E;
        }

      if (src_.x_ > tgt_.x_) // headed west?
        {
          if (src_.y_ < tgt_.y_) // and north?
            return NW;
          if (src_.y_ > tgt_.y_) // and south?
            return SW;
          return W;
        }

      // Vertical segment: if the target is above the source, heading north; else south.
      return src_.y_ < tgt_.y_ ? N : S;
    }

    /// Extend the segment by @p dist from the source endpoint (away from target).
    void enlarge_src(const Geom_Number & dist)
    {
      const Geom_Number dx = tgt_.x_ - src_.x_;
      const Geom_Number dy = tgt_.y_ - src_.y_;
      const Geom_Number len = euclidean_distance(dx, dy);
      if (len == 0)
        return;
      src_.x_ -= dist * dx / len;
      src_.y_ -= dist * dy / len;
    }

    /// Extend the segment by @p dist from the target endpoint (away from source).
    void enlarge_tgt(const Geom_Number & dist)
    {
      const Geom_Number dx = tgt_.x_ - src_.x_;
      const Geom_Number dy = tgt_.y_ - src_.y_;
      const Geom_Number len = euclidean_distance(dx, dy);
      if (len == 0)
        return;
      tgt_.x_ += dist * dx / len;
      tgt_.y_ += dist * dy / len;
    }

    /// Return "(src)(tgt)" textual form.
    [[nodiscard]] std::string to_string() const
    {
      return src_.to_string() + tgt_.to_string();
    }

    operator std::string() const
    {
      return to_string();
    }

    /// Rotate the segment by @p angle (radians) around the source point.
    void rotate(const Geom_Number & angle)
    {
      if (angle == 0)
        return;

      const Polar_Point ptgt(tgt_ - src_); // target in polar coordinates

      // move by angle radians, convert back, and translate to source frame
      tgt_ = Polar_Point(ptgt.get_r(), ptgt.get_theta() + angle);
      tgt_ = tgt_ + src_;
    }

    // Return the intersection segment with triangle t (if any).
    [[nodiscard]] inline Segment intersection_with(const Triangle & t) const;

    // Return the intersection segment with ellipse e (if any).
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


  /** @brief Non-degenerate triangle defined by three points.
   *
   * Basic helper used by polygon triangulation and geometric predicates.
   *
   * @ingroup Geometry
   */
  class Triangle : public Geom_Object
  {
    friend class Point;
    friend class Segment;

    Point p1_, p2_, p3_;

    Geom_Number area_;

  public:
    Triangle(Point p1, Point p2, Point p3)
      : p1_(std::move(p1)), p2_(std::move(p2)), p3_(std::move(p3))
    {
      area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

      ah_domain_error_if(area_ == 0) << "The three points of triangle are collinear";
    }

    Triangle(Point p, const Segment & s)
      : p1_(std::move(p)), p2_(s.src_), p3_(s.tgt_)
    {
      area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

      ah_domain_error_if(area_ == 0) << "The three points of triangle are collinear";
    }

    Triangle(const Segment & s, Point p)
      : p1_(s.src_), p2_(s.tgt_), p3_(std::move(p))
    {
      area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

      ah_domain_error_if(area_ == 0) << "The three points of triangle are collinear";
    }

    [[nodiscard]] Geom_Number area() const
    {
      return abs(area_);
    }

    /// Returns true if the triangle vertices are ordered clockwise.
    [[nodiscard]] bool is_clockwise() const
    {
      return area_ < 0;
    }

    [[nodiscard]] const Point &highest_point() const
    {
      const Point & max = p1_.y_ > p2_.y_ ? p1_ : p2_;

      return p3_.y_ > max.y_ ? p3_ : max;
    }

    [[nodiscard]] const Point &lowest_point() const
    {
      const Point & min = p1_.y_ < p2_.y_ ? p1_ : p2_;

      return p3_.y_ < min.y_ ? p3_ : min;
    }

    [[nodiscard]] const Point &leftmost_point() const
    {
      const Point & min = p1_.x_ < p2_.x_ ? p1_ : p2_;

      return p3_.x_ < min.x_ ? p3_ : min;
    }

    [[nodiscard]] const Point &rightmost_point() const
    {
      const Point & max = p1_.x_ > p2_.x_ ? p1_ : p2_;

      return p3_.x_ > max.x_ ? p3_ : max;
    }

    [[nodiscard]] const Point &get_p1() const { return p1_; }

    [[nodiscard]] const Point &get_p2() const { return p2_; }

    [[nodiscard]] const Point &get_p3() const { return p3_; }

    [[nodiscard]] bool operator==(const Triangle & t) const noexcept
    {
      const bool p1_in = p1_ == t.p1_ or p1_ == t.p2_ or p1_ == t.p3_;
      const bool p2_in = p2_ == t.p1_ or p2_ == t.p2_ or p2_ == t.p3_;
      const bool p3_in = p3_ == t.p1_ or p3_ == t.p2_ or p3_ == t.p3_;
      return p1_in and p2_in and p3_in;
    }

    [[nodiscard]] bool operator!=(const Triangle & t) const noexcept
    {
      return not (*this == t);
    }

    /// Centroid of the triangle.
    [[nodiscard]] Point centroid() const
    {
      return (p1_ + p2_ + p3_) / Geom_Number(3);
    }

    /// Perimeter of the triangle.
    [[nodiscard]] Geom_Number perimeter() const
    {
      return p1_.distance_to(p2_) + p2_.distance_to(p3_) + p3_.distance_to(p1_);
    }

    /// Circumcenter of the triangle.
    [[nodiscard]] Point circumcenter() const
    {
      const Geom_Number & x1 = p1_.x_;
      const Geom_Number & y1 = p1_.y_;
      const Geom_Number & x2 = p2_.x_;
      const Geom_Number & y2 = p2_.y_;
      const Geom_Number & x3 = p3_.x_;
      const Geom_Number & y3 = p3_.y_;

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

    /// Incenter of the triangle.
    [[nodiscard]] Point incenter() const
    {
      const Geom_Number a = p2_.distance_to(p3_);
      const Geom_Number b = p1_.distance_to(p3_);
      const Geom_Number c = p1_.distance_to(p2_);
      const Geom_Number sum = a + b + c;
      ah_domain_error_if(sum == 0) << "Cannot compute incenter of a degenerate triangle";

      return (p1_ * a + p2_ * b + p3_ * c) / sum;
    }

    /// Three edges in counterclockwise vertex order.
    [[nodiscard]] std::array<Segment, 3> edges() const
    {
      return {Segment(p1_, p2_), Segment(p2_, p3_), Segment(p3_, p1_)};
    }

    /// Returns true if the point p lies strictly inside this triangle.
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

    /// Returns the intersection segment between this triangle and segment @p s.
    [[nodiscard]] Segment intersection_with(const Segment & s) const
    {
      return s.intersection_with(*this);
    }
  };

  /** @brief Axis-aligned rectangle.
   *
   * Stored as (xmin, ymin, xmax, ymax) with the invariant xmin <= xmax and
   * ymin <= ymax.
   *
   * @ingroup Geometry
   */
  class Rectangle
  {
    Geom_Number xmin_, ymin_;
    Geom_Number xmax_, ymax_;

  public:
    [[nodiscard]] bool operator==(const Rectangle & r) const noexcept
    {
      return xmin_ == r.xmin_ and ymin_ == r.ymin_ and
             xmax_ == r.xmax_ and ymax_ == r.ymax_;
    }

    [[nodiscard]] bool operator!=(const Rectangle & r) const noexcept
    {
      return not (*this == r);
    }

    [[nodiscard]] const Geom_Number &get_xmin() const { return xmin_; }

    [[nodiscard]] const Geom_Number &get_ymin() const { return ymin_; }

    [[nodiscard]] const Geom_Number &get_xmax() const { return xmax_; }

    [[nodiscard]] const Geom_Number &get_ymax() const { return ymax_; }

    Rectangle() : xmin_(0), ymin_(0), xmax_(0), ymax_(0)
    {
      // empty
    }

    Rectangle(const Geom_Number & xmin, const Geom_Number & ymin,
              const Geom_Number & xmax, const Geom_Number & ymax)
      : xmin_(xmin), ymin_(ymin), xmax_(xmax), ymax_(ymax)
    {
      ah_range_error_if(xmax_ < xmin_ or ymax_ < ymin_) << "Invalid rectangle";
    }

    void set_rect(const Geom_Number & xmin, const Geom_Number & ymin,
                  const Geom_Number & xmax, const Geom_Number & ymax)
    {
      ah_range_error_if(xmax < xmin or ymax < ymin) << "Invalid rectangle";

      xmin_ = xmin;
      ymin_ = ymin;
      xmax_ = xmax;
      ymax_ = ymax;
    }

    [[nodiscard]] Geom_Number width() const { return xmax_ - xmin_; }

    [[nodiscard]] Geom_Number height() const { return ymax_ - ymin_; }

    [[nodiscard]] Geom_Number area() const { return width() * height(); }

    [[nodiscard]] Geom_Number perimeter() const
    {
      return 2 * (width() + height());
    }

    [[nodiscard]] Point center() const
    {
      return {(xmin_ + xmax_) / 2, (ymin_ + ymax_) / 2};
    }

    /// Rectangle corners in CCW order starting at bottom-left.
    [[nodiscard]] std::array<Point, 4> corners() const
    {
      return {
            Point(xmin_, ymin_),
            Point(xmax_, ymin_),
            Point(xmax_, ymax_),
            Point(xmin_, ymax_)
          };
    }

    // does this axis-aligned rectangle intersect that one?
    [[nodiscard]] bool intersects(const Rectangle & that) const
    {
      return this->xmax_ >= that.xmin_ and this->ymax_ >= that.ymin_ and
             that.xmax_ >= this->xmin_ and that.ymax_ >= this->ymin_;
    }

    [[nodiscard]] Geom_Number distance_squared_to(const Point & p) const
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

    // distance from p to the closest point on this axis-aligned rectangle
    [[nodiscard]] Geom_Number distance_to(const Point & p) const
    {
      return sqrt(mpfr_class(distance_squared_to(p)));
    }

    // does this axis-aligned rectangle contain p?
    [[nodiscard]] bool contains(const Point & p) const
    {
      return p.get_x() >= xmin_ and p.get_x() <= xmax_ and
             p.get_y() >= ymin_ and p.get_y() <= ymax_;
    }

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
        {
          const Geom_Number d2 = points[i].distance_squared_to(points[j]);
          if (d2 > best_d2)
            {
              best_d2 = d2;
              i_best = i;
              j_best = j;
            }
        }

    return {points[i_best], points[j_best]};
  }


  /** @brief Axis-aligned ellipse.
   *
   * Represents an axis-aligned ellipse centered at @p center with horizontal
   * radius @p hr and vertical radius @p vr.
   *
   * @ingroup Geometry
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
    Ellipse(Point center,
            const Geom_Number & hr,
            const Geom_Number & vr)
      : center_(std::move(center)), hr_(hr), vr_(vr)
    {
      validate_positive_radii();
    }

    Ellipse(const Ellipse & e) = default;


    Ellipse() : center_(0, 0), hr_(1), vr_(1)
    {
      // empty
    }

    [[nodiscard]] bool operator==(const Ellipse & e) const noexcept
    {
      return center_ == e.center_ and hr_ == e.hr_ and vr_ == e.vr_;
    }

    [[nodiscard]] bool operator!=(const Ellipse & e) const noexcept
    {
      return not (*this == e);
    }

    [[nodiscard]] const Point &get_center() const { return center_; }

    [[nodiscard]] const Geom_Number &get_hradius() const { return hr_; }

    [[nodiscard]] const Geom_Number &get_vradius() const { return vr_; }

    [[nodiscard]] Geom_Number area() const
    {
      validate_positive_radii();
      return geom_pi() * hr_ * vr_;
    }

    /// Perimeter approximation (Ramanujan's second formula).
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

    /// Sample a point on the ellipse at parameter angle (radians).
    [[nodiscard]] Point sample(const Geom_Number & angle) const
    {
      validate_positive_radii();
      return {
            center_.get_x() + hr_ * cosinus(angle),
            center_.get_y() + vr_ * sinus(angle)
          };
    }

    [[nodiscard]] std::string to_string() const
    {
      return "Ellipse(center=" + center_.to_string() +
             ", hr=" + std::to_string(geom_number_to_double(hr_)) +
             ", vr=" + std::to_string(geom_number_to_double(vr_)) + ")";
    }

  private:
    void validate_positive_radii() const
    {
      ah_domain_error_if(hr_ <= 0 or vr_ <= 0)
      << "Ellipse radii must be > 0";
    }

    [[nodiscard]] static bool in_unit_interval(const Geom_Number & t)
    {
      return t >= 0 and t <= 1;
    }

  public:
    static bool is_clockwise() { return false; }

    [[nodiscard]] Point highest_point() const
    {
      return {center_.get_x(), center_.get_y() + vr_};
    }

    [[nodiscard]] Point lowest_point() const
    {
      return {center_.get_x(), center_.get_y() - vr_};
    }

    [[nodiscard]] Point leftmost_point() const
    {
      return {center_.get_x() - hr_, center_.get_y()};
    }

    [[nodiscard]] Point rightmost_point() const
    {
      return {center_.get_x() + hr_, center_.get_y()};
    }

    /* Computes the tangents to this ellipse with slope @p m.

       Derived from the line equation:

           y = m x + sqrt(a^2 m^2 + b^2)

       which corresponds to tangents to an ellipse centered at the origin.

       This comes from substituting y = mx + y0 into the simplified ellipse
       equation at (0,0):

                                 2     2
                                y     x
                               --- + --- = 1
                                 2      2
                               vr     hr

       WARNING: floating-point precision loss may appear due to irrational values.

       @p s1 and @p s2 are the tangent segments; @p m is the slope.
     */
    void
    compute_tangents(Segment & s1, Segment & s2, const Geom_Number & m) const
    {
      validate_positive_radii();

      if (m == 0)
        {
          s1 = Segment(center_ + Point(-hr_, vr_), center_ + Point(hr_, vr_));
          s2 = Segment(center_ + Point(-hr_, -vr_), center_ + Point(hr_, -vr_));

          return;
        }

      const Geom_Number product = hr_ * hr_ * m * m + vr_ * vr_;

      // intersection with the axis if the ellipse were centered at the origin
      const Geom_Number y1 = square_root(product);

      const Geom_Number x1 = -y1 / m;

      // when the ellipse is centered at (0,0), the tangent points are
      // (0, y1) and (0, -y1). For an ellipse centered at @p center we
      // translate them along the line (0,0)--center.

      const Segment t1 = Segment(center_ + Point(x1, 0), center_ + Point(0, y1));

      const Segment t2 = Segment(center_ + Point(-x1, 0), center_ + Point(0, -y1));

      // tangent segments may be longer than required. For each one pick the
      // endpoint closest to the center.

      // decide tangent length using the largest radius.
      const Geom_Number tangent_size = hr_ > vr_ ? hr_ : vr_; { // compute distances from center to tangent endpoints
        const Geom_Number dsrc = center_.distance_to(t1.get_src_point());
        const Geom_Number dtgt = center_.distance_to(t1.get_tgt_point());

        if (dsrc < dtgt) // choose the point nearest to center
          {
            s1 = Segment(t1.get_src_point(), m, tangent_size);
            s1.enlarge_src(tangent_size);
          }
        else
          {
            s1 = Segment(t1.get_tgt_point(), m, tangent_size);
            s1.enlarge_tgt(tangent_size);
          }
      } { // compute distances for the second tangent
        const Geom_Number dsrc = center_.distance_to(t2.get_src_point());
        const Geom_Number dtgt = center_.distance_to(t2.get_tgt_point());

        if (dsrc < dtgt) // choose the point nearest to center
          {
            s2 = Segment(t2.get_src_point(), m, tangent_size);
            s2.enlarge_src(tangent_size);
          }
        else
          {
            s2 = Segment(t2.get_tgt_point(), m, tangent_size);
            s2.enlarge_tgt(tangent_size);
          }
      }
    }

    // Returns true if segment @p s intersects the ellipse.
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

      // Degenerate segment: it intersects the boundary iff the point is on it.
      if (A == 0)
        return C == 0;

      const Geom_Number discriminant = B * B - Geom_Number(4) * A * C;
      if (discriminant < 0)
        return false;

      const Geom_Number root = square_root(discriminant);
      const Geom_Number denom = Geom_Number(2) * A;
      const Geom_Number t1 = (-B - root) / denom;
      const Geom_Number t2 = (-B + root) / denom;
      return in_unit_interval(t1) or in_unit_interval(t2);
    }

  private:
    // Computes ( (p.x - xc)^2 / a^2 + (p.y - yc)^2 / b^2 ) which helps to
    // determine whether @p p is inside the ellipse (<= 1), on the border (=1),
    // or outside (> 1).
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
    // Returns true if point @p p lies inside this ellipse.
    [[nodiscard]] bool contains(const Point & p) const
    {
      return compute_radius(p) <= 1;
    }

    /// @deprecated Use contains() instead.
    [[deprecated("Use contains() instead")]]
    [[nodiscard]] bool contains_to(const Point & p) const { return contains(p); }

    // Returns true if point @p p lies exactly on the ellipse curve.
    [[nodiscard]] bool intersects_with(const Point & p) const
    {
      return compute_radius(p) == 1;
    }

    // Returns the boundary intersection points between @p sg and this ellipse.
    // For tangency, returns a degenerate segment (same endpoint twice).
    [[nodiscard]] Segment intersection_with(const Segment & sg) const
    {
      validate_positive_radii();

      const Point p0 = sg.get_src_point() - center_;
      const Point p1 = sg.get_tgt_point() - center_;
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

      if (A == 0)
        {
          ah_domain_error_if(C != 0) << "there is no intersection";
          return {sg.get_src_point(), sg.get_src_point()};
        }

      const Geom_Number discriminant = B * B - Geom_Number(4) * A * C;
      ah_domain_error_if(discriminant < 0) << "there is no intersection";

      const Geom_Number root = square_root(discriminant);
      const Geom_Number denom = Geom_Number(2) * A;
      const Geom_Number t1 = (-B - root) / denom;
      const Geom_Number t2 = (-B + root) / denom;

      const bool in1 = in_unit_interval(t1);
      const bool in2 = in_unit_interval(t2);
      ah_domain_error_if(not in1 and not in2) << "there is no intersection";

      auto eval = [&](const Geom_Number & t)
      {
        return Point(p0.get_x() + dx * t, p0.get_y() + dy * t) + center_;
      };

      if (in1 and in2)
        return {eval(t1), eval(t2)};

      const Point touch = in1 ? eval(t1) : eval(t2);
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
   * @brief Ellipse with arbitrary rotation.
   *
   * Extends the axis-aligned Ellipse by adding a rotation angle θ.
   * The implicit equation in the rotated frame is:
   *
   *     ((x' - cx)² / a²) + ((y' - cy)² / b²) = 1
   *
   * where (x', y') are coordinates rotated by -θ around the center.
   *
   * Since exact trigonometry is not possible with rationals, the rotation
   * is specified by its cosine and sine as exact Geom_Number values.
   * For common angles (0, π/2, π, 3π/2) exact values are trivial.
   * For other angles, the caller provides rational approximations.
   */
  class RotatedEllipse : public Geom_Object
  {
    Point center_;
    Geom_Number a_; // semi-axis along the local x (before rotation)
    Geom_Number b_; // semi-axis along the local y (before rotation)
    Geom_Number cos_th_; // cosine of the rotation angle
    Geom_Number sin_th_; // sine of the rotation angle

    // Transform a world point to the ellipse's local frame.
    [[nodiscard]] Point to_local(const Point & p) const
    {
      const Geom_Number dx = p.get_x() - center_.get_x();
      const Geom_Number dy = p.get_y() - center_.get_y();
      return {
            cos_th_ * dx + sin_th_ * dy,
            -sin_th_ * dx + cos_th_ * dy
          };
    }

    // Transform a local-frame point back to world coordinates.
    [[nodiscard]] Point to_world(const Point & p) const
    {
      return {
            cos_th_ * p.get_x() - sin_th_ * p.get_y() + center_.get_x(),
            sin_th_ * p.get_x() + cos_th_ * p.get_y() + center_.get_y()
          };
    }

    void validate_positive_radii() const
    {
      ah_domain_error_if(a_ <= 0 or b_ <= 0)
      << "RotatedEllipse radii must be > 0";
    }

    void normalize_rotation()
    {
      const Geom_Number norm2 = cos_th_ * cos_th_ + sin_th_ * sin_th_;
      ah_domain_error_if(norm2 == 0)
      << "Rotation vector (cos,sin) cannot be (0,0)";
      if (norm2 != 1)
        {
          const Geom_Number norm = square_root(norm2);
          cos_th_ /= norm;
          sin_th_ /= norm;
        }
    }

  public:
    /// Construct a rotated ellipse from center, semi-axes, and cos/sin of angle.
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

    /// Construct an axis-aligned ellipse (θ = 0).
    RotatedEllipse(Point center,
                   const Geom_Number & a, const Geom_Number & b)
      : center_(std::move(center)), a_(a), b_(b), cos_th_(1), sin_th_(0)
    {
      validate_positive_radii();
    }

    RotatedEllipse()
      : center_(0, 0), a_(1), b_(1), cos_th_(1), sin_th_(0)
    {
      // empty
    }

    RotatedEllipse(const RotatedEllipse &) = default;

    RotatedEllipse &operator=(const RotatedEllipse &) = default;

    [[nodiscard]] bool operator==(const RotatedEllipse & e) const noexcept
    {
      return center_ == e.center_ and a_ == e.a_ and b_ == e.b_ and
             cos_th_ == e.cos_th_ and sin_th_ == e.sin_th_;
    }

    [[nodiscard]] bool operator!=(const RotatedEllipse & e) const noexcept
    {
      return not (*this == e);
    }

    [[nodiscard]] const Point &get_center() const { return center_; }
    [[nodiscard]] const Geom_Number &get_a() const { return a_; }
    [[nodiscard]] const Geom_Number &get_b() const { return b_; }
    [[nodiscard]] const Geom_Number &get_cos() const { return cos_th_; }
    [[nodiscard]] const Geom_Number &get_sin() const { return sin_th_; }

    [[nodiscard]] Geom_Number area() const
    {
      validate_positive_radii();
      return geom_pi() * a_ * b_;
    }

    /// Evaluate (x'/a)² + (y'/b)² for a point in local coords.
    [[nodiscard]] Geom_Number radius_value(const Point & p) const
    {
      validate_positive_radii();

      const Point lp = to_local(p);
      return (lp.get_x() * lp.get_x()) / (a_ * a_) +
             (lp.get_y() * lp.get_y()) / (b_ * b_);
    }

    /// Returns true if point p lies inside or on the ellipse.
    [[nodiscard]] bool contains(const Point & p) const
    {
      return radius_value(p) <= 1;
    }

    /// Returns true if point p lies strictly inside the ellipse.
    [[nodiscard]] bool strictly_contains(const Point & p) const
    {
      return radius_value(p) < 1;
    }

    /// Returns true if point p lies exactly on the ellipse boundary.
    [[nodiscard]] bool on_boundary(const Point & p) const
    {
      return radius_value(p) == 1;
    }

    /// Sample a point on the ellipse from caller-provided cos(t), sin(t).
    [[nodiscard]] Point sample(const Geom_Number & cos_t,
                               const Geom_Number & sin_t) const
    {
      validate_positive_radii();
      // Point on axis-aligned ellipse: (a cos t, b sin t).
      // Rotate and translate to world.
      return to_world(Point(a_ * cos_t, b_ * sin_t));
    }

    /// Sample a point on the ellipse for parameter t in [0, 1] -> angle [0, 2π).
    [[nodiscard]] Point sample(const Geom_Number & t) const
    {
      const Geom_Number angle = Geom_Number(2) * geom_pi() * t;
      return sample(cosinus(angle), sinus(angle));
    }

    /// Return true if segment @p s intersects this rotated ellipse.
    [[nodiscard]] bool intersects_with(const Segment & s) const
    {
      validate_positive_radii();
      const Segment local_s(to_local(s.get_src_point()), to_local(s.get_tgt_point()));
      return Ellipse(Point(0, 0), a_, b_).intersects_with(local_s);
    }

    /// Return boundary intersection segment between @p s and this rotated ellipse.
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

    /// Extremal points of the rotated ellipse.
    struct ExtremalPoints
    {
      Point right, top, left, bottom;
    };

    /// Compute the four axis-extremal points of the rotated ellipse.
    [[nodiscard]] ExtremalPoints extremal_points() const
    {
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
   * @brief 3D point with exact rational coordinates.
   *
   * Extends the 2D geometry module with a basic 3D point type.
   * All operations use exact `Geom_Number` arithmetic.
   */
  class Point3D
  {
    Geom_Number x_, y_, z_;

  public:
    Point3D() : x_(0), y_(0), z_(0) {}

    Point3D(const Geom_Number & x, const Geom_Number & y, const Geom_Number & z)
      : x_(x), y_(y), z_(z) {}

    Point3D(const Point3D &) = default;

    Point3D &operator=(const Point3D &) = default;

    [[nodiscard]] const Geom_Number &get_x() const { return x_; }
    [[nodiscard]] const Geom_Number &get_y() const { return y_; }
    [[nodiscard]] const Geom_Number &get_z() const { return z_; }

    [[nodiscard]] bool operator==(const Point3D & p) const
    {
      return x_ == p.x_ and y_ == p.y_ and z_ == p.z_;
    }

    [[nodiscard]] bool operator!=(const Point3D & p) const
    {
      return ! (*this == p);
    }

    [[nodiscard]] Point3D operator+(const Point3D & p) const
    {
      return {x_ + p.x_, y_ + p.y_, z_ + p.z_};
    }

    [[nodiscard]] Point3D operator-(const Point3D & p) const
    {
      return {x_ - p.x_, y_ - p.y_, z_ - p.z_};
    }

    Point3D &operator+=(const Point3D & p)
    {
      x_ += p.x_;
      y_ += p.y_;
      z_ += p.z_;
      return *this;
    }

    Point3D &operator-=(const Point3D & p)
    {
      x_ -= p.x_;
      y_ -= p.y_;
      z_ -= p.z_;
      return *this;
    }

    /// Unary negation.
    [[nodiscard]] Point3D operator-() const
    {
      return {-x_, -y_, -z_};
    }

    [[nodiscard]] Point3D operator*(const Geom_Number & s) const
    {
      return {x_ * s, y_ * s, z_ * s};
    }

    [[nodiscard]] Point3D operator/(const Geom_Number & s) const
    {
      return {x_ / s, y_ / s, z_ / s};
    }

    /// Dot product.
    [[nodiscard]] Geom_Number dot(const Point3D & p) const
    {
      return x_ * p.x_ + y_ * p.y_ + z_ * p.z_;
    }

    /// Cross product.
    [[nodiscard]] Point3D cross(const Point3D & p) const
    {
      return {
            y_ * p.z_ - z_ * p.y_,
            z_ * p.x_ - x_ * p.z_,
            x_ * p.y_ - y_ * p.x_
          };
    }

    /// Squared Euclidean distance.
    [[nodiscard]] Geom_Number distance_squared_to(const Point3D & p) const
    {
      const Geom_Number dx = x_ - p.x_;
      const Geom_Number dy = y_ - p.y_;
      const Geom_Number dz = z_ - p.z_;
      return dx * dx + dy * dy + dz * dz;
    }

    /// Squared norm.
    [[nodiscard]] Geom_Number norm_squared() const
    {
      return x_ * x_ + y_ * y_ + z_ * z_;
    }

    /// Euclidean norm.
    [[nodiscard]] Geom_Number norm() const
    {
      return square_root(norm_squared());
    }

    /// Euclidean distance.
    [[nodiscard]] Geom_Number distance_to(const Point3D & p) const
    {
      return square_root(distance_squared_to(p));
    }

    /// Return a normalized vector.
    [[nodiscard]] Point3D normalize() const
    {
      const Geom_Number n = norm();
      ah_domain_error_if(n == 0) << "Cannot normalize zero Point3D";
      return *this / n;
    }

    /// Project to 2D (drop z).
    [[nodiscard]] Point to_2d() const { return {x_, y_}; }

    /// Lift a 2D point to 3D with z = 0.
    [[nodiscard]] static Point3D from_2d(const Point & p)
    {
      return {p.get_x(), p.get_y(), Geom_Number(0)};
    }

    /// Lift a 2D point to 3D with specified z.
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
   * @brief 3D line segment.
   */
  class Segment3D
  {
    Point3D src_, tgt_;

  public:
    Segment3D() = default;

    Segment3D(const Point3D & src, const Point3D & tgt) : src_(src), tgt_(tgt) {}

    [[nodiscard]] const Point3D &get_src() const { return src_; }
    [[nodiscard]] const Point3D &get_tgt() const { return tgt_; }
    [[nodiscard]] const Point3D &get_src_point() const { return src_; }
    [[nodiscard]] const Point3D &get_tgt_point() const { return tgt_; }

    [[nodiscard]] Point3D direction() const { return tgt_ - src_; }

    [[nodiscard]] Geom_Number length_squared() const
    {
      return src_.distance_squared_to(tgt_);
    }

    [[nodiscard]] Geom_Number length() const
    {
      return src_.distance_to(tgt_);
    }

    /// Evaluate the segment at parameter t ∈ [0,1].
    [[nodiscard]] Point3D at(const Geom_Number & t) const
    {
      const Geom_Number s = Geom_Number(1) - t;
      return src_ * s + tgt_ * t;
    }

    /// Midpoint.
    [[nodiscard]] Point3D midpoint() const
    {
      return at(Geom_Number(1, 2));
    }

    [[nodiscard]] bool operator==(const Segment3D & s) const
    {
      return src_ == s.src_ and tgt_ == s.tgt_;
    }

    [[nodiscard]] bool operator!=(const Segment3D & s) const
    {
      return not (*this == s);
    }

    [[nodiscard]] bool contains(const Point3D & p) const
    {
      const Point3D d = direction();
      const Point3D w = p - src_;

      if (d == Point3D(0, 0, 0))
        return p == src_;

      if (d.cross(w) != Point3D(0, 0, 0))
        return false;

      Geom_Number t;
      if (d.get_x() != 0)
        t = w.get_x() / d.get_x();
      else if (d.get_y() != 0)
        t = w.get_y() / d.get_y();
      else
        t = w.get_z() / d.get_z();

      return t >= 0 and t <= 1;
    }

    [[nodiscard]] Geom_Number distance_to(const Point3D & p) const
    {
      const Point3D d = direction();
      const Geom_Number len2 = d.dot(d);

      if (len2 == 0)
        return p.distance_to(src_);

      Geom_Number t = (p - src_).dot(d) / len2;
      if (t < 0)
        t = 0;
      else if (t > 1)
        t = 1;

      const Point3D proj = at(t);
      return p.distance_to(proj);
    }
  };

  /**
   * @brief 3D triangle defined by three points.
   */
  class Triangle3D
  {
    Point3D p1_, p2_, p3_;

  public:
    Triangle3D() = default;

    Triangle3D(const Point3D & p1, const Point3D & p2, const Point3D & p3)
      : p1_(p1), p2_(p2), p3_(p3) {}

    [[nodiscard]] const Point3D &get_p1() const { return p1_; }
    [[nodiscard]] const Point3D &get_p2() const { return p2_; }
    [[nodiscard]] const Point3D &get_p3() const { return p3_; }

    /// Normal vector (not normalized): (p2-p1) × (p3-p1).
    [[nodiscard]] Point3D normal() const
    {
      return (p2_ - p1_).cross(p3_ - p1_);
    }

    /// 2 * area^2.
    [[nodiscard]] Geom_Number double_area_squared() const
    {
      return normal().norm_squared() / Geom_Number(2);
    }

    /// Centroid.
    [[nodiscard]] Point3D centroid() const
    {
      return (p1_ + p2_ + p3_) / Geom_Number(3);
    }

    /// Check if three points are collinear (zero-area triangle).
    [[nodiscard]] bool is_degenerate() const
    {
      return normal().norm_squared() == 0;
    }

    /// Barycentric coordinates of point p.
    /// Returns (u, v, w) such that p = u*p1 + v*p2 + w*p3, u+v+w = 1.
    struct BaryCoords
    {
      Geom_Number u, v, w;
    };

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
   * @brief Tetrahedron defined by four points in 3D.
   */
  class Tetrahedron
  {
    Point3D p1_, p2_, p3_, p4_;

  public:
    Tetrahedron() = default;

    Tetrahedron(const Point3D & p1, const Point3D & p2,
                const Point3D & p3, const Point3D & p4)
      : p1_(p1), p2_(p2), p3_(p3), p4_(p4) {}

    [[nodiscard]] const Point3D &get_p1() const { return p1_; }
    [[nodiscard]] const Point3D &get_p2() const { return p2_; }
    [[nodiscard]] const Point3D &get_p3() const { return p3_; }
    [[nodiscard]] const Point3D &get_p4() const { return p4_; }

    /// Signed volume × 6 = (p2-p1) · ((p3-p1) × (p4-p1)).
    [[nodiscard]] Geom_Number signed_volume_x6() const
    {
      return scalar_triple_product(p2_ - p1_, p3_ - p1_, p4_ - p1_);
    }

    /// Unsigned volume = |signed_volume_x6| / 6.
    [[nodiscard]] Geom_Number volume() const
    {
      Geom_Number v = signed_volume_x6();
      if (v < 0) v = -v;
      return v / Geom_Number(6);
    }

    /// Returns true if the four points are coplanar (zero volume).
    [[nodiscard]] bool is_degenerate() const
    {
      return signed_volume_x6() == 0;
    }

    /// Centroid of the tetrahedron.
    [[nodiscard]] Point3D centroid() const
    {
      return (p1_ + p2_ + p3_ + p4_) / Geom_Number(4);
    }

    /// Check if a point lies inside the tetrahedron (using sign of
    /// sub-tetrahedra volumes).
    [[nodiscard]] bool contains(const Point3D & p) const
    {
      auto signed_volume_x6_of = [](const Point3D & a, const Point3D & b,
                                    const Point3D & c, const Point3D & d)
      {
        return scalar_triple_product(b - a, c - a, d - a);
      };

      const Geom_Number d0 = signed_volume_x6();
      if (d0 == 0) return false; // degenerate

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

    /// Faces of the tetrahedron.
    struct Faces
    {
      Triangle3D f[4];
    };

    /// Get the four faces as Triangle3D objects.
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

#endif // C++20 format

# endif // POINT_H
