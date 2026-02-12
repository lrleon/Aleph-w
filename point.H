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

# include <iomanip>
# include <string>

# include <ahAssert.H>
# include <ahUtils.H>
# include <ah-errors.H>
# include <utility>

# include <gmpfrxx.h>

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


constexpr double PI = 3.1415926535897932384626433832795028841971693993751;
constexpr double PI_2 = PI / 2.0;
constexpr double PI_4 = PI / 4.0;


class Point;
class Polar_Point;
class Segment;
class Triangle;
class Ellipse;


/// Compute the area of parallelogram defined by vectors a->b and b->c
inline Geom_Number
area_of_parallelogram(const Point & a, const Point & b, const Point & c);


/// Return the Euclidean distance.
/// \brief Euclidean distance of the vector (x, y).
inline Geom_Number pitag(const Geom_Number & x, const Geom_Number & y)
{
  return hypot(mpfr_class(x), mpfr_class(y));
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

  bool operator ==(const Point & point) const
  {
    return x_ == point.x_ and y_ == point.y_;
  }

  bool operator !=(const Point & point) const
  {
    return not (*this == point);
  }

  Point operator +(const Point & p) const
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

  [[nodiscard]] const Geom_Number &get_x() const /// Returns x value
  {
    return x_;
  }

  [[nodiscard]] const Geom_Number &get_y() const /// Returns y value
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

  /// Return true if this is to the left from points p1 and p2
  [[nodiscard]] bool is_to_left_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) > 0;
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

  /// Return true if this is to right from (or on) points p1 and p2
  [[nodiscard]] bool is_to_right_on_from(const Point & p1, const Point & p2) const
  {
    return not is_to_left_from(p1, p2);
  }

  /// Returns true if the sequence this-p1-p2 is clockwise.
  [[nodiscard]] bool is_clockwise_with(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(*this, p1, p2) < 0;
  }

  [[nodiscard]] inline bool is_to_left_from(const Segment & s) const;

  [[nodiscard]] inline bool is_to_right_from(const Segment & s) const;

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
    return this->distance_with(p1) < this->distance_with(p2) ? p1 : p2;
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
  [[nodiscard]] inline Geom_Number distance_with(const Point & p) const;

  [[nodiscard]] const Point &highest_point() const { return *this; }

  [[nodiscard]] const Point &lowest_point() const { return *this; }

  [[nodiscard]] const Point &leftmost_point() const { return *this; }

  [[nodiscard]] const Point &rightmost_point() const { return *this; }
};


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

  Geom_Number r_;
  Geom_Number theta_;

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
  Polar_Point(const Point & p)
  {
    const Geom_Number & x = p.x_;
    const Geom_Number & y = p.y_;

    r_ = pitag(x, y);
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

  Polar_Point() : r_(0), theta_(0)
  { /* empty */
  }
};


inline
Point::Point(const Polar_Point & pp)
  : x_(pp.r_ * cosinus(pp.theta_)), y_(pp.r_ * sinus(pp.theta_))
{
  // empty
}


/** Fundamental segment defined by two points.

    @ingroup Geometry
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
  bool operator ==(const Segment & s) const
  {
    return src_ == s.src_ and tgt_ == s.tgt_;
  }

  bool operator !=(const Segment & s) const
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

  Segment(Point src, Point  tgt)
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
    : Geom_Object(), src_(std::move(src)), tgt_(compute_tgt_point(src, m, l))
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

    if (angle == 0)
      return 0;

    if (angle < 0)
      return -angle.get_d();

    return 2 * PI - angle.get_d();
  }

  /// Compute the counterclockwise angle of this respect x-axis.
  [[nodiscard]] double counterclockwise_angle() const
  {
    const Segment x_axis(Point(0, 0), Point(1, 0));
    return counterclockwise_angle_with(x_axis);
  }

  /// Return the Euclidean length of the segment (distance between endpoints).
  [[nodiscard]] Geom_Number size() const
  {
    return pitag(tgt_.x_ - src_.x_, tgt_.y_ - src_.y_);
  }

  /// Return true if @p p is colinear with this segment.
  [[nodiscard]] bool is_colinear_with(const Point & p) const
  {
    return p.is_colinear_with(src_, tgt_);
  }

  /// Return true if this segment is to the left of point @p p.
  [[nodiscard]] bool is_to_left_from(const Point & p) const
  {
    return p.is_to_right_from(*this);
  }

  /// Return true if this segment is to the right of point @p p.
  [[nodiscard]] bool is_to_right_from(const Point & p) const
  {
    return p.is_to_left_from(*this);
  }

  /// Return the midpoint of this segment.
  [[nodiscard]] Point mid_point() const
  {
    const Geom_Number x = (src_.get_x() + tgt_.get_x()) / 2;
    const Geom_Number y = (src_.get_y() + tgt_.get_y()) / 2;

    return {x, y};
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

    const Geom_Number & arc_tgt_src = tgt_polar.get_theta();

    Geom_Number arc_perp_pt = arctan(dist / (tgt_polar.get_r() / 2));

    const Geom_Number mperp = dist / (tgt_polar.get_r() / 2);

    // Distance from src to the perpendicular point.
    Geom_Number perp_r = pitag(dist, tgt_polar.get_r() / 2);

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
    if (p1.is_to_right_from(*this))
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
    return (src_.is_to_left_from(s) xor tgt_.is_to_left_from(s)) and
           s.src_.is_to_left_from(*this) xor s.tgt_.is_to_left_from(*this);
  }

  /// Return true if @p p lies on this segment.
  [[nodiscard]] bool contains_to(const Point & p) const
  {
    return p.is_between(src_, tgt_);
  }

  /// Return true if @p s lies entirely inside this segment.
  [[nodiscard]] bool contains_to(const Segment & s) const
  {
    return (s.get_src_point().is_between(src_, tgt_) and
            s.get_tgt_point().is_between(src_, tgt_));
  }

  /// Return true if @p s intersects this segment (including endpoints).
  [[nodiscard]] bool intersects_with(const Segment & s) const
  {
    if (this->intersects_properly_with(s))
      return true;

    return (this->contains_to(s.src_) or this->contains_to(s.tgt_) or
            s.contains_to(this->src_) or s.contains_to(this->tgt_));
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

  /// Extend the segment by @p __dist from the source endpoint.
  void enlarge_src(const Geom_Number & dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(src_, m, -dist);

    if (const Segment s(p, src_); s.size() < dist) // fallback to extending forward
      p = compute_tgt_point(src_, m, dist);

    src_ = p;
  }

  /// Extend the segment by @p __dist from the target endpoint.
  void enlarge_tgt(const Geom_Number & dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(tgt_, m, dist);

    if (const Segment s(tgt_, p); s.size() < dist)
      p = compute_tgt_point(tgt_, m, -dist);

    tgt_ = p;
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
  void rotate(const double & angle)
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
  return s.contains_to(*this);
}

// Return true if this point is colinear with segment s.
inline bool Point::is_colinear_with(const Segment & s) const
{
  return this->is_colinear_with(s.src_, s.tgt_);
}

// Return true if this point is to the left of the segment s.
inline bool Point::is_to_left_from(const Segment & s) const
{
  return this->is_to_left_from(s.src_, s.tgt_);
}

// Return true if this point is to the right of the segment s.
inline bool Point::is_to_right_from(const Segment & s) const
{
  return this->is_to_right_from(s.src_, s.tgt_);
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
inline Geom_Number Point::distance_with(const Point & p) const
{
  const Segment seg(*this, p);

  return seg.size();
}


/*****************************************************************

                      Fundamental triangle class

  Basic helper for polygon triangulation algorithms.
 */
class Triangle : public Geom_Object
{
  friend class Point;
  friend class Segment;

  Point p1_, p2_, p3_;

  Geom_Number area_;

public:
  Triangle(Point  p1, Point  p2, Point  p3)
    : p1_(std::move(p1)), p2_(std::move(p2)), p3_(std::move(p3))
  {
    area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

    ah_domain_error_if(area_ == 0)
      << "The three points of triangle are collinear";
  }

  Triangle(Point p, const Segment & s)
    : p1_(std::move(p)), p2_(s.src_), p3_(s.tgt_)
  {
    area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

    ah_domain_error_if(area_ == 0)
      << "The three points of triangle are collinear";
  }

  Triangle(const Segment & s, Point  p)
    : p1_(s.src_), p2_(s.tgt_), p3_(std::move(p))
  {
    area_ = area_of_parallelogram(p1_, p2_, p3_) / 2;

    ah_domain_error_if(area_ == 0)
      << "The three points of triangle are collinear";
  }

  [[nodiscard]] Geom_Number area() const
  {
    return abs(area_);
  }

  /// Returns true if the triangle vertices are ordered clockwise.
  [[nodiscard]] bool is_clockwise() const
  {
    return area_ >= 0;
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

  /// Returns true if the point p lies inside this triangle.
  [[nodiscard]] bool contains_to(const Point & p) const
  {
    const bool s = p.is_to_left_from(p1_, p2_);

    if (p.is_to_left_from(p2_, p3_) != s)
      return false;

    if (p.is_to_left_from(p3_, p1_) != s)
      return false;

    return true;
  }

  /// Returns the intersection segment between this triangle and segment @p s.
  [[nodiscard]] Segment intersection_with(const Segment & s) const
  {
    return s.intersection_with(*this);
  }
};

class Rectangle
{
  Geom_Number xmin_, ymin_;
  Geom_Number xmax_, ymax_;

public:
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
    ah_range_error_if(xmax < xmin or ymax < ymin)
      << "Invalid rectangle";

    xmin_ = xmin;
    ymin_ = ymin;
    xmax_ = xmax;
    ymax_ = ymax;
  }

  [[nodiscard]] Geom_Number width() const { return xmax_ - xmin_; }

  [[nodiscard]] Geom_Number height() const { return ymax_ - ymin_; }

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
};

// Returns true if this segment intersects one or two edges of triangle @p t.
inline bool Segment::intersects_with(const Triangle & t) const
{
  return (this->intersects_with(Segment(t.get_p1(), t.get_p2())) or
          this->intersects_with(Segment(t.get_p2(), t.get_p3())) or
          this->intersects_with(Segment(t.get_p3(), t.get_p1())));
}


// Returns the segment resulting from intersecting this segment with triangle @p t.
// If an endpoint lies inside the triangle, the intersection degenerates to a
// point, which can be used to test if a point belongs to @p t.
inline Segment Segment::intersection_with(const Triangle & t) const
{
  ah_domain_error_if(not this->intersects_with(t))
    << "segment does not intersects with triangle";

  Point p[2];

  int i = 0;

  try
    { // check intersection against edge p1-p2
      p[i] = this->intersection_with(Segment(t.get_p1(), t.get_p2()));

      if (t.contains_to(p[i]))
        ++i; // intersection detected
    }
  catch (std::domain_error &)
    {
      // No intersection with the previous edge; continue.
    }

  try
    { // check intersection against edge p2-p3
      p[i] = this->intersection_with(Segment(t.get_p2(), t.get_p3()));

      if (t.contains_to(p[i]))
        ++i; // intersection detected
    }
  catch (std::domain_error &)
    {
      // No intersection with the previous edge; continue.
    }

  if (i == 2) // two intersection points already collected
    return {p[0], p[1]};

  try
    { // check intersection against edge p3-p1
      p[i] = this->intersection_with(Segment(t.get_p3(), t.get_p1()));
    }
  catch (std::domain_error &)
    {
      throw; // should be unreachable because intersection was confirmed
    }

  // Result depends on the number of intersection points captured.
  return i == 1 ? Segment(p[0], p[0]) : Segment(p[0], p[1]);
}


/*****************************************************************

                        Fundamental ellipse class

  Represents an axis-aligned ellipse centered at p center with horizontal
  radius @p hr and vertical radius @p vr.
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
    // empty
  }

  Ellipse(const Ellipse & e) = default;


  Ellipse() = default;

  [[nodiscard]] const Point &get_center() const { return center_; }

  [[nodiscard]] const Geom_Number &get_hradius() const { return hr_; }

  [[nodiscard]] const Geom_Number &get_vradius() const { return vr_; }

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
      const Geom_Number dsrc = center_.distance_with(t1.get_src_point());
      const Geom_Number dtgt = center_.distance_with(t1.get_tgt_point());

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
    }

    { // compute distances for the second tangent
      const Geom_Number dsrc = center_.distance_with(t2.get_src_point());
      const Geom_Number dtgt = center_.distance_with(t2.get_tgt_point());

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
    Segment tg1;
    Segment tg2;

    compute_tangents(tg1, tg2, s.slope()); // build both parallel tangents

    // intersection exists if @p s lies between both tangents
    return (s.is_to_left_from(tg1.get_src_point()) xor
            s.is_to_left_from(tg2.get_tgt_point()));
  }

private:
  // Computes ( (p.x - xc)^2 / a^2 + (p.y - yc)^2 / b^2 ) which helps to
  // determine whether @p p is inside the ellipse (<= 1), on the border (=1),
  // or outside (> 1).
  [[nodiscard]] Geom_Number compute_radius(const Point & p) const
  {
    Geom_Number x2 = (p.get_x() - center_.get_x());
    x2 = x2 * x2;

    Geom_Number y2 = (p.get_y() - center_.get_y());
    y2 = y2 * y2;

    return x2 / (hr_ * hr_) + y2 / (vr_ * vr_);
  }

public:
  // Returns true if point @p p lies inside this ellipse.
  [[nodiscard]] bool contains_to(const Point & p) const
  {
    return compute_radius(p) <= 1;
  }

  // Returns true if point @p p lies exactly on the ellipse curve.
  [[nodiscard]] bool intersects_with(const Point & p) const
  {
    return compute_radius(p) == 1;
  }

  /*
    Segment intersection routines. Solve the system composed of the ellipse
    equation

                             2    2
                            y    x
           eq1              -- + -- = 1
                             2    2
                            b    a

    and the segment line equation

                eq2              y - yr = m*(x - xr)

    where (xr, yr) is a point on the line once it is translated to the
    ellipse-centered coordinate system.

    The explicit solutions (obtained, for example, via
    maxima: solve([eq1, eq2], [x, y])) are:

                    2                2   2    2  2    2     2         2  2
       a b sqrt(- yr  + 2 m xr yr - m  xr  + a  m  + b ) + a  m yr - a  m  xr
x1 = - ----------------------------------------------------------------------
                               2  2    2
                              a  m  + b

                       2                2   2    2  2    2     2       2
        a b m sqrt(- yr  + 2 m xr yr - m  xr  + a  m  + b ) - b  yr + b  m xr
y1 = - ----------------------------------------------------------------------
                               2  2    2
                              a  m  + b

                  2                2   2    2  2    2     2         2  2
     a b sqrt(- yr  + 2 m xr yr - m  xr  + a  m  + b ) - a  m yr + a  m  xr
x2 = ----------------------------------------------------------------------
                               2  2    2
                              a  m  + b

                    2                2   2    2  2    2     2       2
     a b m sqrt(- yr  + 2 m xr yr - m  xr  + a  m  + b ) + b  yr - b  m xr
y2 = ----------------------------------------------------------------------
                               2  2    2
                              a  m  + b

     These formulas assume the ellipse is centered at (0, 0).

     To compute the actual intersection: translate the segment so the ellipse
     center lands at the origin, evaluate the expressions above, and translate
     the resulting segment back to the real center.

     BUG: there remains a numerical issue when the line passes through the
          ellipse center; graphpic-generated code still disagrees between the
          two solutions even though they should match.
  */
  [[nodiscard]] Segment intersection_with(const Segment & sg) const
  {
    ah_domain_error_if(not intersects_with(sg)) << "there is no intersection";

    const Geom_Number & a = hr_;
    const Geom_Number & b = vr_;

    const Geom_Number a2 = a * a;
    const Geom_Number b2 = b * b;

    const Geom_Number ab = a * b;

    // Segment translated to coordinate system with origin at (xc, yc).
    const Segment sg_new(sg.get_src_point() - center_,
                         sg.get_tgt_point() - center_);

    const Point & pr = sg_new.get_tgt_point();

    const Geom_Number & xr = pr.get_x();
    const Geom_Number & yr = pr.get_y();

    const Geom_Number m = sg_new.slope();

    assert(m == sg.slope());

    const Geom_Number m2 = m * m;

    const Geom_Number yr2 = yr * yr;

    const Geom_Number xr2 = xr * xr;

    assert(m2 >= 0 and yr2 >= 0 and xr2 >= 0);

    const Geom_Number a2m2_plus_b2 = a2 * m2 + b2;

    Geom_Number ab_root = -yr2 + 2 * m * xr * yr - m2 * xr2 + a2m2_plus_b2;
    ab_root = ab * square_root(ab_root);

    const Geom_Number ab_m_root = m * ab_root;

    const Geom_Number yr_minus_m_xr = yr - m * xr;

    const Geom_Number sumx = a2 * m * yr_minus_m_xr;

    const Geom_Number sumy = b2 * yr_minus_m_xr;

    // With the main calculations done, compute the final values.

    const Geom_Number x1 = -(ab_root + sumx) / a2m2_plus_b2;

    const Geom_Number y1 = -(ab_m_root - sumy) / a2m2_plus_b2;

    const Geom_Number x2 = (ab_root - sumx) / a2m2_plus_b2;

    const Geom_Number y2 = (ab_m_root + sumy) / a2m2_plus_b2;

    // Since results are for the ellipse at (0,0), translate the intersection
    // points back to the actual ellipse center.

    const Point src = Point(x1, y1) + center_;
    const Point tgt = Point(x2, y2) + center_;

    return {src, tgt};
  }
};

// Return true if this point lies inside ellipse e.
inline bool Point::is_inside(const Ellipse & e) const
{
  return e.contains_to(*this);
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

/*****************************************************************

                       Fundamental text primitive

   Utility to draw text strings on the plane. Offsets are not stored because
   callers can shift the reference point directly.
*/

/* Estimate the count of printable characters in a LaTeX string (skipping '\',
   '$', '{', '}', etc.).
*/
inline size_t aproximate_string_size(const std::string & str)
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

  size_t len_;

public:
  static constexpr double font_width_in_points = 0.8;

  static constexpr double font_height_in_points = 1.2;

  Text(Point p, const std::string & str)
    : p_(std::move(p)), str_(str), len_(aproximate_string_size(str))
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
enum class Orientation { CCW, CW, COLLINEAR };

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

/// Return true if @p p lies on segment @p s (exact).
[[nodiscard]] inline bool
on_segment(const Segment & s, const Point & p)
{
  return s.contains_to(p);
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
    @throws std::domain_error if segments are parallel/collinear (no unique point). */
[[nodiscard]] inline Point
segment_intersection_point(const Segment & s1, const Segment & s2)
{
  ah_domain_error_if(not segments_intersect(s1, s2))
    << "Segments do not intersect";

  return s1.intersection_with(s2);
}

/// Return the (unsigned) area of triangle (a, b, c) as an exact rational.
[[nodiscard]] inline Geom_Number
area_of_triangle(const Point & a, const Point & b, const Point & c)
{
  return abs(area_of_parallelogram(a, b, c)) / 2;
}


# endif // POINT_H
