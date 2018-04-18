
/*
  This file is part of Aleph-w system

  Copyright (c) 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009, 2010,
                2011, 2012, 2013, 2014
  Leandro Rabindranath León
  All rights reserved.

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are
  met: 

  1. Redistributions of source code must retain the above copyright 
     notice, this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright 
     notice, this list of conditions and the following disclaimer in the 
     documentation and/or other materials provided with the distribution.

  3. All advertising materials mentioning features or use of this software
     must display the following acknowledgement:

     Copyright (c) 2002-2014 Leandro Rabindranath León. See details of 
     licence.     

     This product includes software developed by the Hewlett-Packard
     Company, Free Software Foundation and Silicon Graphics Computer
     Systems, Inc. 

  4. Neither the name of the ULA nor the names of its contributors may
     be used to endorse or promote products derived from this software
     without specific prior written permission. 

THIS SOFTWARE IS PROVIDED BY Leandro Rabindranath León ''AS IS'' AND ANY
EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
DISCLAIMED. IN NO EVENT SHALL <COPYRIGHT HOLDER> BE LIABLE FOR ANY
DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
(INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
(INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.

  Aleph-w is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
  or FITNESS FOR A PARTICULAR PURPOSE.

  I request users of this software to return to 

  Leandro Rabindranath Leon
  CEMISID 
  Ed La Hechicera 
  3er piso, ala sur
  Facultad de Ingenieria 
  Universidad de Los Andes 
  Merida - REPÚBLICA BOLIVARIANA DE VENEZUELA    or

  leandro.r.leon@gmail.com

  any improvements or extensions that they make and grant me the rights
  to redistribute these changes.  
*/

/**
  @file ah_stdc++_utils.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef POINT_H
# define POINT_H

# include <limits>

# include <iomanip>
# include <string>

# include <ahAssert.H>
# include <ahUtils.H>

# include <gmpfrxx.h> 

# include <math.h>
# include <autosprintf.h>


typedef mpq_class Geom_Number;

inline double geom_number_to_double(const Geom_Number & n)
{
  return n.get_d();
}


const double PI = 3.1415926535897932384626433832795028841971693993751;
const double PI_2 = PI/2;
const double PI_4 = PI_2/2.0;


class Point;
class Polar_Point;
class Segment;
class Triangle;
class Ellipse;

/** 
  @brief Calculate the area of the parallelogram 
  defined by the vectors a-->b y b-->c 
*/
inline Geom_Number 
area_of_parallelogram(const Point & a, const Point & b, const Point & c);

/** 
  @brief Returns the Euclidean distance
  (Pythagoras theorem)
*/
inline Geom_Number pitag(const Geom_Number & x, const Geom_Number & y)
{
  return hypot(mpfr_class(x), mpfr_class(y));
}

inline Geom_Number arctan(const Geom_Number & m)
{
  return atan(mpfr_class(m));
}

inline Geom_Number sinus(const Geom_Number & x)
{
  return sin(mpfr_class(x));
}

inline Geom_Number cosinus(const Geom_Number & x)
{
  return cos(mpfr_class(x));
}

inline Geom_Number square_root(const Geom_Number & x)
{
  return sqrt(mpfr_class(x));
}

struct Geom_Object 
{ 
  Geom_Object(const Geom_Object & ) { /* Empty */ }

  Geom_Object() { /* Empty */ }

  virtual ~Geom_Object() { /* Empty */ }
};

/** 
  @brief Rectangular point in the plane.
	
	@details Fundamental class that defines a point on a coordinate plane,
  could be cartesian or rectangular. 

	@ingroup Geometria
*/
class Point : public Geom_Object
{
  friend class Segment;
  friend class Triangle;
  friend class Polar_Point;

  Geom_Number x;
  Geom_Number y;

public:
  
  Point() : Geom_Object(), x(0), y(0) { /* Empty */ } 
  
/** 
  @brief Builder from the coordinates __x e __y. 
*/
  Point(const Geom_Number & __x, const Geom_Number & __y) 
    : Geom_Object(), x(__x), y(__y)
  {
    /* Empty */
  }

/** 
  @brief Build copy of the point p 
*/
  Point(const Point & p) : Geom_Object(*this), x(p.x), y(p.y)
  {
    /* Empty */
  }

/** 
  @brief Builder from a point in polar coordinates
*/
  inline Point(const Polar_Point & pp);

/** 
  @brief Compare two points to see if they match
*/
  bool operator == (const Point & point) const 
  {
    return x == point.x and y == point.y; 
  }
  
/** 
  @brief Compare two points to see if they don't match 
*/
  bool operator != (const Point & point) const 
  {
    return not (*this == point);
  }

/** 
  @brief Sum of points is defined as the sum of each of its coordinates
  
  @details Useful to change points to a new reference plane
*/
  Point operator + (const Point & p) const
  {
    return Point(x + p.x, y + p.y);
  }

/** 
  @brief Sum of points is defined as the sum of each of its coordinates

  @details Useful to change points to a new reference plane
*/
  Point & operator += (const Point & p)
  {
    x += p.x;
    y += p.y;
    
    return *this;
  }

/** 
  @brief Subtracting points. 
  
  @details Useful to invest result of the sum 
*/
  Point operator - (const Point & p) const
  {
    return Point(x - p.x, y - p.y);
  }

/** 
  @brief Subtracting points 
  
  @details Useful to invest result of the sum
*/
  Point & operator -= (const Point & p)
  {
    x -= p.x;
    y -= p.y;
    
    return *this;
  }

/** 
  @brief Returns the x coordinate value 
*/
  const Geom_Number & get_x() const 
  {
    return x; 
  }

/** 
  @brief Returns the y coordinate value 
*/
  const Geom_Number & get_y() const 
  { 
    return y; 
  }

/** 
  @brief Return true if this, p1 y p2 are collinear 
*/
  bool is_colinear_with(const Point & p1, const Point & p2) const 
  {
    return area_of_parallelogram(*this, p1, p2) == 0;
  }

/** 
  @brief Returns true if point this is collinear with the segment s
*/
  inline bool is_colinear_with(const Segment & s) const;

/** 
  @brief Returns true if point this is to the left of the segment formed by p1 and p2
*/
  bool is_to_left_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) > 0;
  }

/** 
  @brief Returns true if point this is to the left of the segment formed by p1 and p2 
*/
  bool is_to_right_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) < 0;
  }

/** 
  @brief Returns true if the sequence this-P1-P2 is clockwise 
*/
  bool is_clockwise_with(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(*this, p1, p2) < 0;
  }

/** 
  @brief Returns true if p is to the left of the segment s
*/
  inline bool is_to_left_from(const Segment & s) const;

/** 
  @brief Returns true if p is to the right of segment s
*/
  inline bool is_to_right_from(const Segment & s) const;

/** 
  @brief Returns true if the sequence p-s is clockwise
*/
  inline bool is_clockwise_with(const Segment & s) const;

/** 
  @brief Returns true if this rate is contained within the segment p1 - p2
*/
  bool is_between(const Point & p1, const Point & p2) const
  {
    if (not this->is_colinear_with(p1, p2))
      return false;

    /* As the points are collinear, the test can
    refer to a coordinate */

    /** Is a vertical segment of infinite slope?
    Yes? ==> We consider the y coordinates  
    Then We consider the x coordinates */
    if (p1.x == p2.x) 
      { 
	if (this->x != p1.x)
	  return false;

	return this->y >= p1.y and this->y <= p2.y;
      }

    return  this->x >= p1.x and this->x <= p2.x;


/** 
  @brief Returns the closest to the point this between p1 and p2
*/
  const Point & nearest_point(const Point & p1, const Point & p2) const 
  {
    return this->distance_with(p1) < this->distance_with(p2) ? p1 : p2;
  }

/** 
  @brief Returns true if point this is contained in the segment s 
*/
  inline bool is_inside(const Segment & s) const;

/** 
  @brief Returns true if point this is contained in the e ellipse 
*/
  inline bool is_inside(const Ellipse & e) const;

/** 
  @brief Returns true if this intersects with the e ellipse
*/
  inline bool intersects_with(const Ellipse & e) const;

/** 
  @brief Converts the point to a string of the form "(x, y)"
*/
  std::string to_string() const
  {
    return gnu::autosprintf("(%.1f,%.1f)", geom_number_to_double(x),
			    geom_number_to_double(y));
  }

/** 
  @brief Converting a string of the form "(x, y)" 
*/
  operator std::string () const { return to_string(); }

/** 
  @brief Returns the square distance between this and that
*/
  inline Geom_Number distance_squared_to(const Point & that) const;

/** 
  @brief Returns the euclidean distance between this and p point
*/
  inline Geom_Number distance_with(const Point & p) const;

  const Point & highest_point() const { return *this; }

  const Point & lowest_point() const { return *this; }

  const Point & leftmost_point() const { return *this; }

  const Point & rightmost_point() const { return *this; }
};

extern const Point NullPoint;

/**
  @brief Converts polar coordinates to a point in the cartesian plane
  (or rectangular).

  @details Useful for problems which have to be used angles 
  (rotation, for example)
  
  @ingroup Geometria
*/
class Polar_Point : public Geom_Object
{
  friend class Point;

  Geom_Number r;
  Geom_Number theta;

public:

/** 
  @brief Returns the magnitude 
*/
  const Geom_Number & get_r() const { return r; }

/** 
  @brief Returns the angle
*/
  const Geom_Number & get_theta() const { return theta; }

  Polar_Point(const Geom_Number & __r, const Geom_Number & __theta)
    : r(__r), theta(__theta)
  {
    /* Empty */
  }

/** 
  @brief Builder from a Cartesian point; 
  The origin (0,0) Polar_Point(const Point & p) : Geom_Object(p) 
*/
  {
    const Geom_Number & x = p.x;
    const Geom_Number & y = p.y;

    r = pitag(x, y); /* The radius is determined by Pythagoras theorem */

    /* We now determine the angle at signs */

    if (x == 0) /* No angle? */
      {
	theta = y >= 0 ? PI/2 : 3*PI/2;

	return;
      }

    if (y == 0) /* ¡PI/2 (90 degrees)? */
      {
	theta = x >= 0 ? 0 : PI;

	return;
      }

    theta = arctan(y/x);

    if (x > 0 and y > 0) /* ¿1st quadrant? */
      return;

    if (x < 0) /* ¿2nd o 3rd quadrants? */
      r = -r;
  }

/** 
  @brief Enum the quadrants 
*/
  enum Quadrant { First, Second, Third, Fourth };

/** 
  @brief Returns the quadrant where the point is located
*/
  Quadrant get_quadrant() const
  {
    if (r > 0 and theta > 0)
      return First;

    if (r > 0)
      return Fourth;

    if (theta > 0)
      return Third;

    return Second;
  }

/** 
  @brief Converts to string
*/
  std::string to_string() const
  {
    return gnu::autosprintf("[%.1f,%.1f]", geom_number_to_double(r),
			    geom_number_to_double(theta));
  }

  Polar_Point() : r(0), theta(0) { /* Empty */ }
};


    inline 
Point::Point(const Polar_Point & pp)
  : x(pp.r * cosinus(pp.theta)), y(pp.r * sinus(pp.theta))
{
  /* Empty */
}


/** 
  @brief Class key segment defined by two points

  @ingroup Geometria
*/
class Segment : public Geom_Object
{
  friend class Point;
  friend class Triangle;

  Point src, tgt;

  double compute_slope() const
  {
    if (tgt.x == src.x)
      {
	if  (src.y < tgt.y )
	  return std::numeric_limits<double>::max();
	else
	  return - std::numeric_limits<double>::max();
      }

    const Geom_Number __slope = (tgt.y - src.y) / (tgt.x - src.x);

    return __slope.get_d();
  }

public:
  
  bool operator == (const Segment & s) const
  {
    return src == s.src and tgt == s.tgt;
  }

  bool operator != (const Segment & s) const
  {
    return not (*this == s);
  }

/** 
  @brief Returns the point of the segment farther north 
*/
  const Point & highest_point() const 
  {
    return  src.y > tgt.y ? src : tgt;
  }

/** 
  @brief Returns the point of the segment farther south 
*/
  const Point & lowest_point() const 
  {
    return  src.y < tgt.y ? src : tgt;
  }

/** 
  @brief Returns the point of the segment farther west
*/
  const Point & leftmost_point() const 
  {
    return  src.x < tgt.x ? src : tgt;
  }

/** 
  @brief Returns the point of the segment farther east
*/
  const Point & rightmost_point() const 
  {
    return  src.x > tgt.x ? src : tgt;
  }

/** 
  @brief Returns the start point of the segment
*/
  const Point & get_src_point() const { return src; }

/** 
  @brief Returns the end point of the segment
*/
  const Point & get_tgt_point() const { return tgt; }

  Segment() { /* Empty */ }

  Segment(const Segment & s) 
    : Geom_Object(), src(s.src), tgt(s.tgt)
  {
    /* Empty */
  }

/**
  @brief Builder essential given the origin and destination points
*/
  Segment(const Point & __src, const Point & __tgt)
    : Geom_Object(), src(__src), tgt(__tgt)
  {
    /* Empty */
  }

private:

    /* 
    Given a point of origin, the slope m and the distance d calculated
    the destination point

     Use the solution of the following equations

         d^2 = (tx-sy)^2 + (ty-sy)^2  (Pythagoras)

         ty - sy = m(tx-sx)           (Equation)

     Returns the point located to the east (right) 
     */

      static  
  Point compute_tgt_point(const Point &       __src, /* Point of origin */
			  const Geom_Number & m,     /* Slope */
			  const Geom_Number & d)     /* Length */
  {
    const Geom_Number den2 = 1 + m*m;

    const Geom_Number den = square_root(den2);

    const Geom_Number x = __src.x + d/den;
    
    const Geom_Number y = __src.y + d*m/den;

    return Point(x, y);
  }

public:

/** 
  @brief Build a point

  @details Calculates the point of origin according to the following pair of equations:
   -#\f$ d^2 = (x - x1)^2 + (y - y1)^2\f$ (Pythagoras)
   -#\f$ y - y1 = m*(x - x1)\f$ (Ecuation given line and slope)

   @param[in] __src Segment origin point
   @param[in] m Slope in radians
   @param[in] d Segment length
*/
  Segment(const Point &       __src, /* Point of origin */
	  const Geom_Number & m,     /* Slope */
	  const Geom_Number & d)     /* Length of line */
    : Geom_Object(), src(__src), tgt(compute_tgt_point(src, m, d))
  {
    /* Empty */
  }

/** 
  @brief Build a new segment parallel to sg and distance dist
*/
  Segment(const Segment & sg, const Geom_Number & dist)
  {
    const Segment perp = sg.mid_perpendicular(dist);

    const Point mid_point = sg.mid_point();

    const Point diff_point = mid_point - perp.get_src_point();

    src = sg.get_src_point() + diff_point;
    tgt = sg.get_tgt_point() + diff_point;
  }

  double slope() const
  {
    return compute_slope();
  }

/** 
  @brief Returns the length of the segment, ie, the euclidean distance
  between origin and destination points
*/
  Geom_Number size() const
  {
    return pitag(tgt.x - src.x, tgt.y - src.y);
  }

/** 
  @brief Returns true if p is collinear at this segment 
*/
  bool is_colinear_with(const Point & p) const
  {
    return p.is_colinear_with(src, tgt);
  }

/** 
  @brief Returns true if point this is segment to the left of point p
*/
  bool is_to_left_from(const Point & p) const
  {
    return p.is_to_right_from(*this);
  }

/** 
  @brief Returns true if point this is segment to the right of the point p 
*/
  bool is_to_right_from(const Point & p) const
  {
    return p.is_to_left_from(*this);
  }

/** 
  @brief Returns the midpoint of segment this
*/
  Point mid_point() const
  {
    const Geom_Number x = (src.get_x() + tgt.get_x()) / 2;
    const Geom_Number y = (src.get_y() + tgt.get_y()) / 2;

    return Point(x, y);
  }

/
/** 
  @brief Returns the closest point between the point p and
  the ends of the this segment 
*/
  const Point & nearest_point(const Point & p) const
  {
    return p.nearest_point(get_src_point(), get_tgt_point());
  }

/** 
  @brief Returns the perpendicular segment that crosses the midpoint
  segment this of length 2 * dist. 

  @details The origin and destination points of the segment resulting form the point
  dist distance perpendicular to the center of the segment this
*/
  Segment mid_perpendicular(const Geom_Number & dist) const
  {
/** 
  @brief Point to the origin destination segment and
  then transformed to polar coordinates
*/
    const Polar_Point tgt_polar(tgt - src);
    
/** 
  @brief Arc of segment this to the horizontal
*/
    const Geom_Number arc_tgt_src = tgt_polar.get_theta();

/** 
  @brief Point's arc and perpendicular between segments esta
  src - perp, which is the point perp result
*/
    Geom_Number arc_perp_pt = arctan(dist/(tgt_polar.get_r()/2));

    const Geom_Number mperp = dist/(tgt_polar.get_r()/2);

/** 
  @brief Distance between src and perp
*/
    Geom_Number perp_r = pitag(dist, tgt_polar.get_r()/2);

    /** The radius is negative?
    Yes? ==> the radius of the midpoint so is */
    if (tgt_polar.get_r() < 0) 
      perp_r = - perp_r; // 

    /** The angle is negative?
    Yes? ==> the radius of the midpoint so is */
    if (tgt_polar.get_theta() < 0)
      arc_perp_pt = - arc_perp_pt;

/** 
  @brief Point perpendicular to the left of the segment this
  regarding src (polar coordinate)
*/
    const Polar_Point polar_perp_pt_l(perp_r, arc_tgt_src + arc_perp_pt);

/** 
  @brief Punto perpendicular a la derecha del segmento this y 
  respecto a src (en coordenada polar) 
*/
    const Polar_Point polar_perp_pt_r(perp_r, arc_tgt_src - arc_perp_pt);

/** 
  @brief Determines the points in rectangular coordinates 

  @details 
  - const Point p1
  - const Point p2
  .
*/
    const Point p1(Point(polar_perp_pt_l) + src);
    const Point p2(Point(polar_perp_pt_r) + src);
    
    /* Segment result should go counterclockwise relative to the segment this */
    if (p1.is_to_right_from(*this))
	return Segment(p1, p2);
    else
      return Segment(p2, p1);
  }

/**
  @brief Return true if there own intersection between segments

  @details A self intersection point is when the intersection is
  content segments
*/
  bool intersects_properly_with(const Segment & s) const
  {
    /* Check the 4 possible combinations of collinearity */
    if (src.is_colinear_with(s) or tgt.is_colinear_with(s) or
	s.src.is_colinear_with(*this) or s.tgt.is_colinear_with(*this))
      return false;

    /* There intersection if, for each segment, a point is to
    left and one on the right */
    return ((src.is_to_left_from(s) xor tgt.is_to_left_from(s)) and
	    (s.src.is_to_left_from(*this) xor s.tgt.is_to_left_from(*this)));
  }

/** 
  @brief Retorna true if p is contained in segment this 
*/
  bool contains_to(const Point & p) const
  {
    return p.is_between(src, tgt);
  }

/** 
  @brief Retorna true if s is contained in segment this 
*/
  bool contains_to(const Segment & s) const
  {
    return (s.get_src_point().is_between(src, tgt) and 
	    s.get_tgt_point().is_between(src, tgt));
  }

/** 
  @brief Retorna true if s intersects segment this 
*/
  bool intersects_with(const Segment & s) const
  {

    /** There own intersection?
    No? ==> Check if any of the segment points is contained in another */
    if (this->intersects_properly_with(s))
      return true;

    return (this->contains_to(s.src) or this->contains_to(s.tgt) or
	    s.contains_to(this->src) or s.contains_to(this->tgt));
  }

/**
  @brief Retorna true if segment this intersects triangle t 
*/
  inline bool intersects_with(const Triangle & t) const;

/** 
  @brief Return true if segment this intersects ellipse e 
*/
  inline bool intersects_with(const Ellipse & e) const;

/** 
  @brief Return true if segment this is parallel to segment s 
*/
  bool is_parallel_with(const Segment & s) const
  {
    return slope() == s.slope();
  }

    /*
      Calculate the intersection of two segments
    
      It uses equations and slope

      y - y1 = m1 (x - x1)      ((x1,y1) = this->src , m1 = this->slope())
      y - y2 = m2 (x - x2)      ((x1,y1) = s->src , m2 = s_slope()) 
    */
  Point intersection_with(const Segment & s) const
  {
    if (this->is_parallel_with(s))
      throw std::domain_error("Segments are parallels");
    
    const Geom_Number & x1 = this->src.x;
    const Geom_Number & y1 = this->src.y;

    const Geom_Number & x2 = s.src.x;
    const Geom_Number & y2 = s.src.y;

    const Geom_Number & m1 = this->slope();
    const Geom_Number & m2 = s.slope();

    const Geom_Number x = (y2 - y1 + m1*x1 - m2*x2) / (m1 - m2);

    const Geom_Number y = m1*(x - x1) + y1;

    return Point(x, y);
  }

    /* TODO: this calculation must pass to polar coordinates */

/** 
  @brief Cardinal directions of a segment address 
*/
  enum Sense { E, NE, N, NW, W, S, SW, SE };

  Sense sense() const
  {
    if (src.x < tgt.x) /* Is in east?*/
      {
	if (src.y < tgt.y) /* Is in north? */
	  return NE; 
	else if (src.y > tgt.y) /* Is in south? */
	  return SE;
	else 
	  return E;
      }

    if (src.x > tgt.x) /* ¿Is in west? */
      {
	if (src.y < tgt.y) /* Is in north? */
	  return NW;
	else if (src.y > tgt.y) /* Is in south? */
	  return SW;
	else return W;
      }

    /* At this point the vertical segment is certaint */

    return src.y > tgt.y ? N : S;
  }

/** 
  @brief Increases in segment distance from the origin point __dist 
*/
  void enlarge_src(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(src, m, - __dist);

    const Segment s(p, src);

    if (s.size() < __dist)
      p = compute_tgt_point(src, m, __dist);

    src = p;
  }

/** 
  @brief Increases the distance of the segment from the destination point __dist 
*/
  void enlarge_tgt(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(tgt, m, __dist);

    Segment s(tgt, p);

    if (s.size() < __dist)
      p = compute_tgt_point(tgt, m, - __dist);

    tgt = p;
  }

/** 
  @brief Builds a string of the form "(src)(tgt)" 
*/
  std::string to_string() const
  {
    return src.to_string() + tgt.to_string();
  }

/** 
  @brief Segment becomes a string of the form "(src)(tgt)" 
*/
  operator std::string () const
  {
    return to_string();
  }

/** 
  @brief Turn the segment angle angle around origin point maintaining the same length segment; 
  implying that the destination point tgt changes
    
  @details Procedure

    -# Normalize the source segment ==> tgt - src

    -# Tranformar tgt to polar coordinate ptgt

    -# The new ptgt is the polar coordinate angle point added in angle; 
    ie, ptgt Polar_Point (r, theta + angle)

    -# Take a rectangular coordinate ptgt tgt

    -# The result is tgt + src
*/

  void rotate(const double & angle) 
  {
    if (angle == 0)
      return;

/** 
  @brief Means tgt in polar coordinates
*/
    const Polar_Point ptgt(tgt - src); 

    /* The move in radians angle */        
    tgt = Polar_Point(ptgt.get_r(), ptgt.get_theta() + angle);

    /* The benchmark returned to the src (the pole) */
    tgt = tgt + src; 
  }

/** 
  @brief Returns the intersection with the triangle t segment (if exist) 
*/
  inline Segment intersection_with(const Triangle & t) const; 

/** 
  @brief Returns the intersection with the ellipse segment and (if exist) 
*/
  inline Segment intersection_with(const Ellipse & e) const;
};

/** 
  @brief Returns true if point this is contained in the segment this 
*/
inline bool Point::is_inside(const Segment & s) const
{
  return s.contains_to(*this);
}

/** 
  @brief Returns true if point this is collinear with the segment s
*/
inline bool Point::is_colinear_with(const Segment & s) const
{
  return this->is_colinear_with(s.src, s.tgt);
}

/** 
  @brief Returns true if is point this at the left of the segment s
*/
inline bool Point::is_to_left_from(const Segment & s) const
{
  return this->is_to_left_from(s.src, s.tgt);
}

/** 
  @brief Returns true if is point this at the right of segment s
*/
inline bool Point::is_to_right_from(const Segment & s) const
{
  return this->is_to_right_from(s.src, s.tgt);
}

/** 
  @brief Returns true if sequence this--s is clockwise 
*/
inline bool Point::is_clockwise_with(const Segment & s) const
{
  return this->is_clockwise_with(s.src, s.tgt);
}


/** 
  @brief Returns the euclidean distance between point this and point p 
*/
inline Geom_Number Point::distance_squared_to(const Point & that) const
{
  Geom_Number dx = this->x - that.x;
  Geom_Number dy = this->y - that.y;
  return dx*dx + dy*dy;
}


/** 
  @brief Returns the Euclidean distance between point this and point p
*/
inline Geom_Number Point::distance_with(const Point & p) const
{
  const Segment seg(*this, p);

  return seg.size();
}

/**
	@brief Fundamental class triangle

  @details The idea of this class is to serve as an object of basic use in
  polygon triangulation algorithms
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
    __area = area_of_parallelogram(p1, p2, p3)/2;

    if (__area == 0)
      throw std::domain_error("The three points of triangle are colinears");
  }

  Triangle(const Point & p, const Segment & s)
    : p1(p), p2(s.src), p3(s.tgt)
  {
    __area = area_of_parallelogram(p1, p2, p3)/2;

    if (__area == 0)
      throw std::domain_error("The three points of triangle are colinears");
  }

  Triangle(const Segment & s, const Point & p)
    : p1(s.src), p2(s.tgt), p3(p)
  {
    __area = area_of_parallelogram(p1, p2, p3)/2;

    if (__area == 0)
      throw std::domain_error("The three points of triangle are colinears");
  }

  Geom_Number area() const
  {
    return abs(__area);
  }

/** 
  @brief Returns true if the triangle is clockwise
*/
  bool is_clockwise() const
  {
    return __area >= 0;
  }

  const Point & highest_point() const
  {
    const Point & max = p1.y > p2.y ? p1 : p2;

    return p3.y > max.y ? p3 : max;
  }

  const Point & lowest_point() const
  {
    const Point & min = p1.y < p2.y ? p1 : p2;

    return p3.y < min.y ? p3 : min;
  }

  const Point & leftmost_point() const
  {
    const Point & min = p1.x < p2.x ? p1 : p2;

    return p3.x < min.x ? p3 : min;
  }

  const Point & rightmost_point() const
  {
    const Point & max = p1.x > p2.x ? p1 : p2;

    return p3.x > max.x ? p3 : max;
  }

  const Point & get_p1() const { return p1; }

  const Point & get_p2() const { return p2; }

  const Point & get_p3() const { return p3; }

/** 
  @brief Returns true if the point p is contained within the triangle 
*/
  bool contains_to(const Point & p) const
  {
    const bool s = p.is_to_left_from(p1, p2);

    if (p.is_to_left_from(p2, p3) != s)
      return false;

    if (p.is_to_left_from(p3, p1) != s)
      return false;

    return true;
  }

/** 
  @brief Returns the "intersection" of the triangle segment with the segment s 
*/
  Segment intersection_wih(const Segment & s) const
  {
    return s.intersection_with(*this);
  }
};

class Rectangle
{
  Geom_Number xmin, ymin;
  Geom_Number xmax, ymax;

public:

  const Geom_Number & get_xmin() const { return xmin; }

  const Geom_Number & get_ymin() const { return ymin; }

  const Geom_Number & get_xmax() const { return xmax; }

  const Geom_Number & get_ymax() const { return ymax; }

  Rectangle() : xmin(0), ymin(0), xmax(0), ymax(0)
  {
    /* Empty */
  }

  Rectangle(const Geom_Number & __xmin, const Geom_Number & __ymin, 
	    const Geom_Number & __xmax, const Geom_Number & __ymax) 
    : xmin(__xmin), ymin(__ymin), xmax(__xmax), ymax(__ymax)
  {
    if (xmax < xmin || ymax < ymin) 
      throw std::range_error("Invalid rectangle");
  }
    
  void set_rect(const Geom_Number & xmin, const Geom_Number & ymin, 
		const Geom_Number & xmax, const Geom_Number & ymax) 
  {
    new (this) Rectangle(xmin, ymin, xmax, ymax);
  }

  Geom_Number width()  { return xmax - xmin; }

  Geom_Number height() { return ymax - ymin; }

/** 
  @brief Question: Does this axis-aligned rectangle intersect that one? 
*/
  bool intersects(const Rectangle & that) 
  {
    return this->xmax >= that.xmin and this->ymax >= that.ymin and
      that.xmax >= this->xmin and that.ymax >= this->ymin;
  }

  Geom_Number distance_squared_to(const Point & p) 
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
      
    return dx*dx + dy*dy;
  }

/** 
  @brief Distance from p to closest point on This axis-aligned rectangle
*/
  Geom_Number distance_to(const Point & p) 
  {
    return sqrt(mpfr_class(distance_squared_to(p)));
  }
    
    /* This axis-aligned rectangle contains p? */
  bool contains(const Point & p) const
  {
    return p.get_x() >= xmin and p.get_x() <= xmax and 
      p.get_y() >= ymin and p.get_y() <= ymax;
  }


};

/** 
  @brief Returns true if the this segment intersects
  no or two sides of the triangle t
*/
inline bool Segment::intersects_with(const Triangle & t) const
{
  return (this->intersects_with(Segment(t.get_p1(), t.get_p2())) or
	  this->intersects_with(Segment(t.get_p2(), t.get_p3())) or
	  this->intersects_with(Segment(t.get_p3(), t.get_p1())));
}

/** 
  @brief Return segment resulting from the intersection of
  esta segment with the triangle t.

  @note Note that if a segment is esta point inside the triangle,
  then the intersection is a point ==> this method could be used to
  determine whether a point is inside the triangle
*/
inline Segment Segment::intersection_with(const Triangle & t) const
{
  if (not this->intersects_with(t))
    throw std::domain_error("segment does not intersects with triangle");

  Point p[2]; 

  int i = 0;

  try 
    {    
    /* Checks for intersection with the side P1-P2 */
      p[i] = this->intersection_with(Segment(t.get_p1(), t.get_p2()));

      if (t.contains_to(p[i]))
	++i; /* Detected intersection */
    }
  catch (std::domain_error)
    {
    /* No intersection with the anterior segment. We continue ... */
    }

  try
    {    
    /* Checks for intersection with the side p2-p3 */
      p[i] = this->intersection_with(Segment(t.get_p2(), t.get_p3()));

      if (t.contains_to(p[i]))
	++i;/* Detected intersection */
    }
  catch (std::domain_error)
    {
     /* No intersection with the anterior segment. We continue ... */
    }
    /* There are two insertion points?
    Yes? ==> there is no more to do but return the segment */
  if (i == 2)
    return Segment(p[0], p[1]);

  try
    {    
      /* Checks for intersection with the side p3-p1 */
      p[i] = this->intersection_with(Segment(t.get_p3(), t.get_p1()));
    }
  catch (std::domain_error)
    {
      throw;  /* Something serious must happen if no exception here.
              This is a bug because previously he wondered*/
    }
  
    /* The result depends on the number of points of intersection we have 
    return i == 1 ? Segment(p[0], p[0]) : Segment(p[0], p[1]); */
}


/**
  @brief Ellipse fundamental class 
*/
class Ellipse : public Geom_Object
{
  friend class Point;

  /* The following equation is assumed in the center (xc, yc):
                     2           2
             (y - yc)    (x - xc)
			       --------- + --------- = 1
			          2           2
				        vr          hr                          */

  Point center; /* Punto centro */

  Geom_Number hr; /* Horizontal radius (parameter a)  */
  Geom_Number vr; /* Vertical radius (parameter b)*/
 
public:

  Ellipse(const Point &       __center, 
	  const Geom_Number & __hr, 
	  const Geom_Number & __vr)
    : center(__center), hr(__hr), vr(__vr)
  {
    /* Empty */
  }

  Ellipse(const Ellipse & e)
    : Geom_Object(e), center(e.center), hr(e.hr), vr(e.vr)
  {
    /* Empty */
  }

  Ellipse() { /* Empty */ }

  const Point & get_center() const { return center; }

  const Geom_Number & get_hradius() const { return hr; }

  const Geom_Number & get_vradius() const { return vr; }

  bool is_clockwise() const { return false; }

  Point highest_point() const
  {
    return Point(center.get_x(), center.get_y() + vr);
  }

  Point lowest_point() const
  {
    return Point(center.get_x(), center.get_y() - vr);
  }

  Point leftmost_point() const
  {
    return Point(center.get_x() - hr, center.get_y());
  }

  Point rightmost_point() const
  {
    return Point(center.get_x() + hr, center.get_y());
  }

    /* 
      Tagentes calculates the ellipse esta with slope m
     
      It is calculated according to equation:

          y = m x + sqrt (a ^ 2 m ^ 2 + b ^ 2)

      which it is the equation of the tangent of the ellipse with center (0,0).

      The above is resulting equation equalize the equation
      simplified ellipse at (0,0) and the line tangent m. 
      It is, ie, replace y = mx + y0 in

             2     2
            y     x
			     --- + --- = 1
			      2      2
            vr     hr


    ATTENTION: precision errors due to irrationality
               s1 and s2 are the tangent and m is the slope 
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

    const Geom_Number product = hr*hr*m*m + vr*vr;

    /* This is cutting the tangent with the abscissa if
    were ellipse centered at (0,0) */
    const Geom_Number y1= square_root(product);

    const Geom_Number x1 = -y1/m;
    
    /* If the ellipse is centered at (0,0) then the points of
    tangents are (0, y1) and (0, -Y1). 
    Actual points regarding ellipse centered center are calculated 
    according to the straight consisting of (0,0) - center */

    Segment t1 = Segment(center + Point(x1, 0), center + Point(0, y1));

    Segment t2 = Segment(center + Point(-x1, 0), center + Point(0, -y1));

    /* These segments are tangent to the ellipse, but,
    by slope, they may be too large.
    Therefore, for each segment, we will choose the point
    it is closer to the center of the ellipse */

    /* Decides the size of the tangent function of the larger radius */
    const Geom_Number tangent_size = hr > vr ? hr : vr;

    {
    /* Calculates distances from the center to the
    end points of the tangent t1 */ 
      const Geom_Number dsrc = center.distance_with(t1.get_src_point());
      const Geom_Number dtgt = center.distance_with(t1.get_tgt_point());

      if (dsrc < dtgt) /* Select the closest point to the center */
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
    
    {    
    /* Calculates distances from the center to the
    end points of the tangent t1 */
      const Geom_Number dsrc = center.distance_with(t2.get_src_point());
      const Geom_Number dtgt = center.distance_with(t2.get_tgt_point());

      if (dsrc < dtgt) /* Select the closest point to the center */
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

/** 
  @brief Returns true if s intersects with the ellipse
*/
  bool intersects_with(const Segment & s) const
  {
    Segment tg1;
    Segment tg2;

    compute_tangents(tg1, tg2, s.slope()); /* Calculate the two tangents */

    /* There intersection if s is between the tangents of the
    ellipse that are parallel to s */
    return (s.is_to_left_from(tg1.get_src_point()) xor 
	    s.is_to_left_from(tg2.get_tgt_point()));
  }

private:

    /* Calculate the value of:
    
         (p.x - xc)^2 + (p.y - yc)^2
         ------------   ------------ = 1
             a^2            b^2
    

    which will determine whether a point is or
    not circumscribed by the ellipse */
  Geom_Number compute_radius(const Point & p) const
  {
    Geom_Number x2 = (p.get_x() - center.get_x());
    x2 = x2*x2;

    Geom_Number y2 = (p.get_y() - center.get_y());
    y2 = y2*y2;
    
    return x2/(hr*hr) + y2/(vr*vr);
  }

public:

/** 
  @brief Returns true if the point p is contained within the ellipse this
*/
  bool contains_to(const Point & p) const
  {
    return compute_radius(p) <= 1;
  }

/** 
  @brief Retotrna true if the point p belongs exactly
  the curve of the ellipse this
*/
  bool intersects_with(const Point & p) const
  {
    return compute_radius(p) == 1;
  }

    /* Segment intersection routines. To do this, it resolves the
    equation system proposed by the equation of the ellipse

                             2    2
                            y    x
           eq1              -- + -- = 1
                             2    2
                            b    a

    and the equation of the line segment:

            eq2              y - yr = m*(x - xr)


    Where (xr, yr) is a point on the line transformed the plane of the center Ellipse

    Which yields the following (according max solve ([eq1, eq2], [x, y]))

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

    Note that it is assumed that the center of the ellipse is at (0,0)

    To calculate the intersection, the straight moving
    proportionally with the distance from the center of the ellipse. Then
    the intersection points are calculated according to equations
    above. Finally, the resulting segment is moved to the
    actual position of the ellipse

    BUG: there is an error, graphpic study generated code for
    detail seems to appear when the line passes through downtown
    ellipse

    The intersection points with different points on the line, give
    different and should give equal why? 
*/
  Segment intersection_with(const Segment & sg) const
  {

    if (not intersects_with(sg))
      throw std::domain_error("there is no intersection");

    const Geom_Number & a  = hr;
    const Geom_Number & b  = vr;

    const Geom_Number a2  = a*a;
    const Geom_Number b2  = b*b;

    const Geom_Number ab  = a*b;

    /* Segment shifted to a coordinate axis originating from (xc, yc) */
    const Segment sg_new(sg.get_src_point() - center, 
			 sg.get_tgt_point() - center);

    const Point pr = sg_new.get_tgt_point();

    const Geom_Number & xr = pr.get_x();
    const Geom_Number & yr = pr.get_y();

    const Geom_Number m = sg_new.slope();

    assert(m == sg.slope());

    const Geom_Number m2 = m*m;

    const Geom_Number yr2 = yr*yr;

    const Geom_Number xr2 = xr*xr;

    assert(m2 >= 0 and yr2 >= 0 and xr2 >= 0);

    const Geom_Number a2m2_plus_b2 = a2*m2 + b2;

    Geom_Number ab_root = -yr2 + 2*m*xr*yr -m2*xr2 + a2m2_plus_b2;
    ab_root = ab*square_root(ab_root);

    const Geom_Number ab_m_root = m*ab_root;

    const Geom_Number yr_minus_m_xr = yr - m*xr;

    const Geom_Number sumx = a2*m*yr_minus_m_xr;

    const Geom_Number sumy = b2*yr_minus_m_xr;

    /* Made the main accounts, we calculate values */

    const Geom_Number x1 = - (ab_root + sumx) / a2m2_plus_b2;

    const Geom_Number y1 = - (ab_m_root - sumy) / a2m2_plus_b2;

    const Geom_Number x2 = (ab_root - sumx) / a2m2_plus_b2;

    const Geom_Number y2 = (ab_m_root + sumy) / a2m2_plus_b2;

    /* The results are for the ellipse at (0,0), readjust the
    intersection points to the real center of the ellipse */

    const Point src = Point(x1, y1) + center;
    const Point tgt = Point(x2, y2) + center;

    return Segment(src, tgt);
  }
};

/** 
  @brief Returns true if point this is contained within the ellipse  e
*/ 
inline bool Point::is_inside(const Ellipse & e) const
{
  return e.contains_to(*this);
}

/** 
  @brief Returns true if point this exactly intersects the ellipse e 
*/
inline bool Point::intersects_with(const Ellipse & e) const
{
  return e.intersects_with(*this);
}

/** 
  @brief Returns true if point this segment intersects the ellipse
*/
inline bool Segment::intersects_with(const Ellipse & e) const
{
  return e.intersects_with(*this);
}

/** 
  @brief Returns the segment resulting from the intersection
  between point this and segment of ellipse e 
*/
inline Segment Segment::intersection_with(const Ellipse & e) const
{
  return e.intersection_with(*this);
}

    
/* 
  @brief Fundamental class string

  @details Used to write strings in the background.

  @note No use offsets because, apparently, from this first stage
  design, an offset can be specified by the calling moving the
  point 
*/

/** 
  @brief This routine calculates an estimated amount of printable characters as
  that the chain is to latex. 

  @example For example, do not count '\' '$' '{' '}' etc. 
*/
inline size_t aproximate_string_size(const std::string & str)
{
  const char * ptr = str.c_str();
 
  size_t __len = 0;
  for (int i = 0; true; /* empty */)
    {
      switch (ptr[i])
	{
	case '\\':
	  
    /** Skip all the characters that make the command LateX */
	  for (++i; isalnum(ptr[i]) and ptr[i] != '\0'; /* Nothing */)  
	    ++i;
	  ++__len; 
	  break;
	  
	case '$': case '{': case '}': case '\n': 
	  ++i;
	  break;

	case '\0':
	  return __len;

	default:
	  ++__len; ++i;
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
    /* Empty */
  }

  Text() { /* Empty */ }

  const size_t & len() const { return __len; }

  const Point & get_point() const
  {
    return p;
  }

  const std::string & get_str() const { return str; }

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
