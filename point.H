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

/** @file point.H
 *  @brief 2D point and geometric utilities.
 *
 *  This file provides classes for 2D points and geometric operations including
 *  distance calculations, vector operations, and coordinate transformations.
 *  Used by graph visualization and geometric algorithms.
 *
 *  @ingroup Geometry
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
#include <utility>

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
  //  Geom_Object(const Geom_Object & ) { /* empty */ }

  Geom_Object() = default;

  virtual ~Geom_Object() = default;
};

/** Rectangular point in the plane.

Fundamental class which defines a point in a cartesians coordinates
plane.

@ingroup Geometry
*/
class Point : public Geom_Object
{
  friend class Segment;
  friend class Triangle;
  friend class Polar_Point;

  Geom_Number x;
  Geom_Number y;

public:
  Point() : Geom_Object(), x(0), y(0)
  { /* empty */
  }

  /// Builds a new point in coordinates (__x, __y)
  Point(const Geom_Number & __x, const Geom_Number & __y)
    : Geom_Object(), x(__x), y(__y)
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
    return x == point.x and y == point.y;
  }

  bool operator !=(const Point & point) const
  {
    return not (*this == point);
  }

  Point operator +(const Point & p) const
  {
    return {x + p.x, y + p.y};
  }

  Point &operator +=(const Point & p)
  {
    x += p.x;
    y += p.y;

    return *this;
  }

  Point operator -(const Point & p) const
  {
    return {x - p.x, y - p.y};
  }

  Point &operator -=(const Point & p)
  {
    x -= p.x;
    y -= p.y;

    return *this;
  }

  [[nodiscard]] const Geom_Number &get_x() const /// Returns x value
  {
    return x;
  }

  [[nodiscard]] const Geom_Number &get_y() const /// Returns y value
  {
    return y;
  }

  /// Returns true if this is colinear with p1 and p2.
  [[nodiscard]] bool is_colinear_with(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(*this, p1, p2) == 0;
  }

  /// Returns true if this is colinear with segment s.
  [[nodiscard]] inline bool is_colinear_with(const Segment & s) const;

  /// Return true if this is to left from points p1 and p2
  [[nodiscard]] bool is_to_left_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) > 0;
  }

  /// Return true if this is to right from points p1 and p2
  [[nodiscard]] bool is_to_right_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) < 0;
  }

  /// Return true if this is to left from (or on) points p1 and p2
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

  /// Returns true if this is inside of segment s.
  [[nodiscard]] inline bool is_inside(const Segment & s) const;

  /// Returns true if this is inside of the ellipse e.
  [[nodiscard]] inline bool is_inside(const Ellipse & e) const;

  /// Returns true if this intersects ellipse @p e.
  [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

  /// Return a string representation of this.
  [[nodiscard]] std::string to_string() const
  {
    return "(" + std::to_string(geom_number_to_double(x)) + "," +
           std::to_string(geom_number_to_double(y)) + ")";
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

extern const Point NullPoint;


/** @brief Polar representation of a 2D point.
 *
 * Polar coordinates are convenient for algorithms that rely on angles or
 * rotations. Instances can be constructed from rectangular coordinates and are
 * convertible back into @ref Point objects.
 *
 * @ingroup Geometry
 */
class Polar_Point : public Geom_Object
{
  friend class Point;

  Geom_Number r;
  Geom_Number theta;

public:
  /// Returns the magnitude
  [[nodiscard]] const Geom_Number &get_r() const { return r; }

  /// Returns the angle.
  [[nodiscard]] const Geom_Number &get_theta() const { return theta; }

  Polar_Point(const Geom_Number & __r, const Geom_Number & __theta)
    : r(__r), theta(__theta)
  {
    // empty
  }

  /// Construct from Cartesian coordinates (origin at (0,0)).
  Polar_Point(const Point & p) : Geom_Object(p)
  {
    const Geom_Number & x = p.x;
    const Geom_Number & y = p.y;

    r = pitag(x, y);
    theta = arctan2(y, x);
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
    return "[" + std::to_string(geom_number_to_double(r)) + "," +
           std::to_string(geom_number_to_double(theta)) + "]";
  }

  Polar_Point() : r(0), theta(0)
  { /* empty */
  }
};


inline
Point::Point(const Polar_Point & pp)
  : x(pp.r * cosinus(pp.theta)), y(pp.r * sinus(pp.theta))
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

  Point src, tgt;

  [[nodiscard]] double compute_slope() const
  {
    if (tgt.x == src.x)
      {
        if (src.y < tgt.y)
          return std::numeric_limits<double>::max();
        return -std::numeric_limits<double>::max();
      }

    const Geom_Number __slope = (tgt.y - src.y) / (tgt.x - src.x);

    return __slope.get_d();
  }

public:
  bool operator ==(const Segment & s) const
  {
    return src == s.src and tgt == s.tgt;
  }

  bool operator !=(const Segment & s) const
  {
    return not (*this == s);
  }

  [[nodiscard]] const Point &highest_point() const
  {
    return src.y > tgt.y ? src : tgt;
  }

  [[nodiscard]] const Point &lowest_point() const
  {
    return src.y < tgt.y ? src : tgt;
  }

  [[nodiscard]] const Point &leftmost_point() const
  {
    return src.x < tgt.x ? src : tgt;
  }

  [[nodiscard]] const Point &rightmost_point() const
  {
    return src.x > tgt.x ? src : tgt;
  }

  [[nodiscard]] const Point &get_src_point() const { return src; }

  [[nodiscard]] const Point &get_tgt_point() const { return tgt; }

  Segment() = default;

  // Segment(const Segment & s)
  //   : Geom_Object(), src(s.src), tgt(s.tgt)
  // {
  //   // empty
  // }

  Segment(Point  __src, const Point & __tgt)
    : Geom_Object(), src(std::move(__src)), tgt(__tgt)
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
  Point compute_tgt_point(const Point & __src, // Point of origin
                          const Geom_Number & m, // slope
                          const Geom_Number & d) // segment length
  {
    const Geom_Number den2 = 1 + m * m;

    const Geom_Number den = square_root(den2);

    const Geom_Number x = __src.x + d / den;

    const Geom_Number y = __src.y + d * m / den;

    return {x, y};
  }

public:
  /** Builds a new segment given source point, slope and length.

      It computes the source point by usen these equations:
     -#\f$ d^2 = (x - x_1)^2 + (y - y_1)^2\f$ (Pythagoras)
     -#\f$ y - y_1 = m(x - x_1)\f$ (line equation given point and slope)

     @param[in] __src Source point
     @param[in] m slope (rise over run, dimensionless)
     @param[in] l length
  */
  Segment(Point  __src, // source point
          const Geom_Number & m, // slope
          const Geom_Number & l) // segment length
    : Geom_Object(), src(std::move(__src)), tgt(compute_tgt_point(src, m, l))
  {
    // empty
  }

  /// Builds a new segment parallel to sg and with a distance dist.
  Segment(const Segment & sg, const Geom_Number & dist)
  {
    const Segment perp = sg.mid_perpendicular(dist);

    const Point mid_point = sg.mid_point();

    const Point diff_point = mid_point - perp.get_src_point();

    src = sg.get_src_point() + diff_point;
    tgt = sg.get_tgt_point() + diff_point;
  }

  [[nodiscard]] double slope() const
  {
    return compute_slope();
  }

  /// Compute the counterclockwise wise angle respect another segment s.
  [[nodiscard]] double counterclockwise_angle_with(const Segment & s) const
  {
    const Geom_Number x1 = tgt.x - src.x;
    const Geom_Number x2 = s.tgt.x - s.src.x;
    const Geom_Number y1 = tgt.y - src.y;
    const Geom_Number y2 = s.tgt.y - s.src.y;
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
    return pitag(tgt.x - src.x, tgt.y - src.y);
  }

  /// Return true if @p p is colinear with this segment.
  [[nodiscard]] bool is_colinear_with(const Point & p) const
  {
    return p.is_colinear_with(src, tgt);
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
    const Geom_Number x = (src.get_x() + tgt.get_x()) / 2;
    const Geom_Number y = (src.get_y() + tgt.get_y()) / 2;

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
    if (src == tgt)
      return *this;

    // Special-case horizontal segments so we can return a vertical segment.
    if (src.get_y() == tgt.get_y())
      return {Point(p.get_x(), src.get_y()), p};

    // Likewise for vertical segments.
    if (src.get_x() == tgt.get_x())
      return {Point(src.get_x(), p.get_y()), p};

    const auto m1 = Geom_Number(slope());

    // calculate the slope of the new segment
    const Geom_Number m2 = -Geom_Number(1.0) / m1;

    // Find the intersection point between both lines.
    const Geom_Number & x1 = src.get_x();
    const Geom_Number & y1 = src.get_y();
    const Geom_Number & x2 = p.get_x();
    const Geom_Number & y2 = p.get_y();

    const Geom_Number x = (y2 - y1 + m1 * x1 - m2 * x2) / (m1 - m2);
    const Geom_Number y = m1 * (x - x1) + y1;

    // The perpendicular segment goes from the intersection to p.
    return {Point(x, y), p};
  }

  /// Return the perpendicular chord of length @c 2*dist through the midpoint.
  [[nodiscard]] Segment mid_perpendicular(const Geom_Number & dist) const
  {
    // Translate target to the origin and convert to polar coordinates.
    const Polar_Point tgt_polar(tgt - src);

    const Geom_Number& arc_tgt_src = tgt_polar.get_theta();

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
    const Point p1(Point(polar_perp_pt_l) + src);
    const Point p2(Point(polar_perp_pt_r) + src);

    // Return the segment in counterclockwise order with respect to @c this.
    if (p1.is_to_right_from(*this))
      return {p1, p2};
    return {p2, p1};
  }

  /// Return true if the segments intersect at interior points only.
  [[nodiscard]] bool intersects_properly_with(const Segment & s) const
  {
    // Reject degenerate colinear configurations first.
    if (src.is_colinear_with(s) or tgt.is_colinear_with(s) or
        s.src.is_colinear_with(*this) or s.tgt.is_colinear_with(*this))
      return false;

    // There is a proper intersection when each segment straddles the other.
    return (src.is_to_left_from(s) xor tgt.is_to_left_from(s)) and
           s.src.is_to_left_from(*this) xor s.tgt.is_to_left_from(*this);
  }

  /// Return true if @p p lies on this segment.
  [[nodiscard]] bool contains_to(const Point & p) const
  {
    return p.is_between(src, tgt);
  }

  /// Return true if @p s lies entirely inside this segment.
  [[nodiscard]] bool contains_to(const Segment & s) const
  {
    return (s.get_src_point().is_between(src, tgt) and
            s.get_tgt_point().is_between(src, tgt));
  }

  /// Return true if @p s intersects this segment (including endpoints).
  [[nodiscard]] bool intersects_with(const Segment & s) const
  {
    if (this->intersects_properly_with(s))
      return true;

    return (this->contains_to(s.src) or this->contains_to(s.tgt) or
            s.contains_to(this->src) or s.contains_to(this->tgt));
  }

  [[nodiscard]] inline bool intersects_with(const Triangle & t) const;

  [[nodiscard]] inline bool intersects_with(const Ellipse & e) const;

  /// Return true if @p s has the same slope.
  [[nodiscard]] bool is_parallel_with(const Segment & s) const
  {
    return slope() == s.slope();
  }

  /// Compute intersection point solving point-slope equations for both lines.
  [[nodiscard]] Point intersection_with(const Segment & s) const
  {
    ah_domain_error_if(this->is_parallel_with(s)) << "Segments are parallels";

    const Geom_Number & x1 = this->src.x;
    const Geom_Number & y1 = this->src.y;

    const Geom_Number & x2 = s.src.x;
    const Geom_Number & y2 = s.src.y;

    const Geom_Number & m1 = this->slope();
    const Geom_Number & m2 = s.slope();

    const Geom_Number x = (y2 - y1 + m1 * x1 - m2 * x2) / (m1 - m2);

    const Geom_Number y = m1 * (x - x1) + y1;

    return {x, y};
  }

  // TODO: consider polar-based sense calculation to avoid branching.

  /// Cardinal directions associated with a segment.
  enum Sense { E, NE, N, NW, W, S, SW, SE };

  [[nodiscard]] Sense sense() const
  {
    if (src.x < tgt.x) // headed east?
      {
        if (src.y < tgt.y) // and north?
          return NE;
        if (src.y > tgt.y) // and south?
          return SE;
        return E;
      }

    if (src.x > tgt.x) // headed west?
      {
        if (src.y < tgt.y) // and north?
          return NW;
        if (src.y > tgt.y) // and south?
          return SW;
        return W;
      }

    // Vertical segment: if target is above source, heading north; else south.
    return src.y < tgt.y ? N : S;
  }

  /// Extend the segment by @p __dist from the source endpoint.
  void enlarge_src(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(src, m, -__dist);

    if (const Segment s(p, src); s.size() < __dist) // fallback to extending forward
      p = compute_tgt_point(src, m, __dist);

    src = p;
  }

  /// Extend the segment by @p __dist from the target endpoint.
  void enlarge_tgt(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(tgt, m, __dist);

    if (const Segment s(tgt, p); s.size() < __dist)
      p = compute_tgt_point(tgt, m, -__dist);

    tgt = p;
  }

  /// Return "(src)(tgt)" textual form.
  [[nodiscard]] std::string to_string() const
  {
    return src.to_string() + tgt.to_string();
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

    const Polar_Point ptgt(tgt - src); // target in polar coordinates

    // move by angle radians, convert back, and translate to source frame
    tgt = Polar_Point(ptgt.get_r(), ptgt.get_theta() + angle);
    tgt = tgt + src;
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
  return this->is_colinear_with(s.src, s.tgt);
}

// Return true if this point is to the left of segment s.
inline bool Point::is_to_left_from(const Segment & s) const
{
  return this->is_to_left_from(s.src, s.tgt);
}

// Return true if this point is to the right of segment s.
inline bool Point::is_to_right_from(const Segment & s) const
{
  return this->is_to_right_from(s.src, s.tgt);
}

// Return true if the sequence (this, segment) is clockwise.
inline bool Point::is_clockwise_with(const Segment & s) const
{
  return this->is_clockwise_with(s.src, s.tgt);
}


// Return the squared Euclidean distance to that.
inline Geom_Number Point::distance_squared_to(const Point & that) const
{
  const Geom_Number dx = this->x - that.x;
  const Geom_Number dy = this->y - that.y;
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

  Point p1, p2, p3;

  Geom_Number __area;

public:
  Triangle(const Point & __p1, const Point & __p2, const Point & __p3)
    : p1(__p1), p2(__p2), p3(__p3)
  {
    __area = area_of_parallelogram(p1, p2, p3) / 2;

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are colinears";
  }

  Triangle(Point  p, const Segment & s)
    : p1(std::move(p)), p2(s.src), p3(s.tgt)
  {
    __area = area_of_parallelogram(p1, p2, p3) / 2;

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are collinear";
  }

  Triangle(const Segment & s, const Point & p)
    : p1(s.src), p2(s.tgt), p3(p)
  {
    __area = area_of_parallelogram(p1, p2, p3) / 2;

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are collinear";
  }

  Geom_Number area() const
  {
    return abs(__area);
  }

  /// Returns true if the triangle vertices are ordered clockwise.
  [[nodiscard]] bool is_clockwise() const
  {
    return __area >= 0;
  }

  [[nodiscard]] const Point &highest_point() const
  {
    const Point & max = p1.y > p2.y ? p1 : p2;

    return p3.y > max.y ? p3 : max;
  }

  [[nodiscard]] const Point &lowest_point() const
  {
    const Point & min = p1.y < p2.y ? p1 : p2;

    return p3.y < min.y ? p3 : min;
  }

  [[nodiscard]] const Point &leftmost_point() const
  {
    const Point & min = p1.x < p2.x ? p1 : p2;

    return p3.x < min.x ? p3 : min;
  }

  [[nodiscard]] const Point &rightmost_point() const
  {
    const Point & max = p1.x > p2.x ? p1 : p2;

    return p3.x > max.x ? p3 : max;
  }

  [[nodiscard]] const Point &get_p1() const { return p1; }

  [[nodiscard]] const Point &get_p2() const { return p2; }

  [[nodiscard]] const Point &get_p3() const { return p3; }

  /// Returns true if point @p p lies inside this triangle.
  [[nodiscard]] bool contains_to(const Point & p) const
  {
    const bool s = p.is_to_left_from(p1, p2);

    if (p.is_to_left_from(p2, p3) != s)
      return false;

    if (p.is_to_left_from(p3, p1) != s)
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
  Geom_Number xmin, ymin;
  Geom_Number xmax, ymax;

public:
  [[nodiscard]] const Geom_Number &get_xmin() const { return xmin; }

  [[nodiscard]] const Geom_Number &get_ymin() const { return ymin; }

  [[nodiscard]] const Geom_Number &get_xmax() const { return xmax; }

  [[nodiscard]] const Geom_Number &get_ymax() const { return ymax; }

  Rectangle() : xmin(0), ymin(0), xmax(0), ymax(0)
  {
    // empty
  }

  Rectangle(const Geom_Number & __xmin, const Geom_Number & __ymin,
            const Geom_Number & __xmax, const Geom_Number & __ymax)
    : xmin(__xmin), ymin(__ymin), xmax(__xmax), ymax(__ymax)
  {
    ah_range_error_if(xmax < xmin || ymax < ymin) << "Invalid rectangle";
  }

  void set_rect(const Geom_Number & __xmin, const Geom_Number & __ymin,
                const Geom_Number & __xmax, const Geom_Number & __ymax)
  {
    ah_range_error_if(__xmax < __xmin || __ymax < __ymin)
      << "Invalid rectangle";

    xmin = __xmin;
    ymin = __ymin;
    xmax = __xmax;
    ymax = __ymax;
  }

  [[nodiscard]] Geom_Number width() const { return xmax - xmin; }

  [[nodiscard]] Geom_Number height() const { return ymax - ymin; }

  // does this axis-aligned rectangle intersect that one?
  [[nodiscard]] bool intersects(const Rectangle & that) const
  {
    return this->xmax >= that.xmin and this->ymax >= that.ymin and
           that.xmax >= this->xmin and that.ymax >= this->ymin;
  }

  [[nodiscard]] Geom_Number distance_squared_to(const Point & p) const
  {
    Geom_Number dx = 0.0, dy = 0.0;
    if (p.get_x() < xmin)
      dx = p.get_x() - xmin;
    else if (p.get_x() > xmax)
      dx = p.get_x() - xmax;

    if (p.get_y() < ymin)
      dy = p.get_y() - ymin;
    else if (p.get_y() > ymax)
      dy = p.get_y() - ymax;

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
    return p.get_x() >= xmin and p.get_x() <= xmax and
           p.get_y() >= ymin and p.get_y() <= ymax;
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

  Represents an axis-aligned ellipse centered at @p center with horizontal
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

  Point center; // ellipse center

  Geom_Number hr; // horizontal radius (parameter a)
  Geom_Number vr; // vertical radius (parameter b)

public:
  Ellipse(Point  __center,
          const Geom_Number & __hr,
          const Geom_Number & __vr)
    : center(std::move(__center)), hr(__hr), vr(__vr)
  {
    // empty
  }

  Ellipse(const Ellipse & e) = default;


  Ellipse() = default;

  [[nodiscard]] const Point &get_center() const { return center; }

  [[nodiscard]] const Geom_Number &get_hradius() const { return hr; }

  [[nodiscard]] const Geom_Number &get_vradius() const { return vr; }

  static bool is_clockwise() { return false; }

  [[nodiscard]] Point highest_point() const
  {
    return {center.get_x(), center.get_y() + vr};
  }

  [[nodiscard]] Point lowest_point() const
  {
    return {center.get_x(), center.get_y() - vr};
  }

  [[nodiscard]] Point leftmost_point() const
  {
    return {center.get_x() - hr, center.get_y()};
  }

  [[nodiscard]] Point rightmost_point() const
  {
    return {center.get_x() + hr, center.get_y()};
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
        s1 = Segment(center + Point(-hr, vr), center + Point(hr, vr));
        s2 = Segment(center + Point(-hr, -vr), center + Point(hr, -vr));

        return;
      }

    const Geom_Number product = hr * hr * m * m + vr * vr;

    // intersection with the axis if the ellipse were centered at the origin
    const Geom_Number y1 = square_root(product);

    const Geom_Number x1 = -y1 / m;

    // when the ellipse is centered at (0,0), the tangent points are
    // (0, y1) and (0, -y1). For an ellipse centered at @p center we
    // translate them along the line (0,0)--center.

    const Segment t1 = Segment(center + Point(x1, 0), center + Point(0, y1));

    const Segment t2 = Segment(center + Point(-x1, 0), center + Point(0, -y1));

    // tangent segments may be longer than required. For each one pick the
    // endpoint closest to the center.

    // decide tangent length using the largest radius.
    const Geom_Number tangent_size = hr > vr ? hr : vr;

    { // compute distances from center to tangent endpoints
      const Geom_Number dsrc = center.distance_with(t1.get_src_point());
      const Geom_Number dtgt = center.distance_with(t1.get_tgt_point());

      if (dsrc < dtgt) // choose point nearest to center
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
      const Geom_Number dsrc = center.distance_with(t2.get_src_point());
      const Geom_Number dtgt = center.distance_with(t2.get_tgt_point());

      if (dsrc < dtgt) // choose point nearest to center
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
    Geom_Number x2 = (p.get_x() - center.get_x());
    x2 = x2 * x2;

    Geom_Number y2 = (p.get_y() - center.get_y());
    y2 = y2 * y2;

    return x2 / (hr * hr) + y2 / (vr * vr);
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

    const Geom_Number & a = hr;
    const Geom_Number & b = vr;

    const Geom_Number a2 = a * a;
    const Geom_Number b2 = b * b;

    const Geom_Number ab = a * b;

    // Segment translated to coordinate system with origin at (xc, yc).
    const Segment sg_new(sg.get_src_point() - center,
                         sg.get_tgt_point() - center);

    const Point pr = sg_new.get_tgt_point();

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

    const Point src = Point(x1, y1) + center;
    const Point tgt = Point(x2, y2) + center;

    return Segment(src, tgt);
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

  size_t __len = 0;
  for (int i = 0; true; /* empty */)
    {
      switch (ptr[i])
        {
        case '\\':
          // Skip all characters that compose the LaTeX command.
          for (++i; isalnum(ptr[i]) and ptr[i] != '\0'; /* nothing */)
            ++i;
          ++__len;
          break;

        case '$':
        case '{':
        case '}':
        case '\n':
          ++i;
          break;

        case '\0':
          return __len;

        default:
          ++__len;
          ++i;
          break;
        }
    }
}

class Text : public Geom_Object
{
  Point p;

  std::string str;

  size_t __len;

public:
  static const double font_width_in_points;

  static const double font_height_in_points;

  Text(const Point & __p, const std::string & __str)
    : p(__p), str(__str), __len(aproximate_string_size(__str))
  {
    // empty
  }

  Text()
  { /* empty */
  }

  const size_t &len() const { return __len; }

  const Point &get_point() const
  {
    return p;
  }

  const std::string &get_str() const { return str; }

  Point highest_point() const
  {
    return p;
  }

  Point lowest_point() const
  {
    return p;
  }

  Point leftmost_point() const
  {
    return p;
  }

  Point rightmost_point() const
  {
    return p;
  }
};

inline Geom_Number
area_of_parallelogram(const Point & a, const Point & b, const Point & c)
{
  return ((b.get_x() - a.get_x()) * (c.get_y() - a.get_y()) -
          (c.get_x() - a.get_x()) * (b.get_y() - a.get_y()));
}


# endif // POINT_H
