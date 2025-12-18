
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

# include <gmpfrxx.h>

typedef mpq_class Geom_Number;

inline double geom_number_to_double(const Geom_Number & n)
{
  return n.get_d();
}

inline std::ostream & operator << (std::ostream & o, const Geom_Number & n)
{
  o << n.get_d();
  return o;
}

// TODO: rotación de figuras especiales como la elipse mediante
// rotación del eje cartesiano


const double PI = 3.1415926535897932384626433832795028841971693993751;
const double PI_2 = PI/2;
const double PI_4 = PI_2/2.0;


class Point;
class Polar_Point;
class Segment;
class Triangle;
class Ellipse;


/// Compute the area of parallelogram defined by vectors a->b and b->c
inline Geom_Number 
area_of_parallelogram(const Point & a, const Point & b, const Point & c);


/// Return the euclidean distance. 
inline Geom_Number pitag(const Geom_Number & x, const Geom_Number & y)
{
  return hypot(mpfr_class(x), mpfr_class(y));
}

inline Geom_Number arctan(const Geom_Number & m)
{
  return atan(mpfr_class(m));
}

inline Geom_Number arctan2(const Geom_Number & m, const Geom_Number & n)
{
  return atan2(mpfr_class(m), mpfr_class(n));
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
  //  Geom_Object(const Geom_Object & ) { /* empty */ }

  Geom_Object() { /* empty */ }

  virtual ~Geom_Object() { /* empty */ }
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

  Point() : Geom_Object(), x(0), y(0) { /* empty */ }

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

  bool operator == (const Point & point) const
  {
    return x == point.x and y == point.y;
  }

  bool operator != (const Point & point) const
  {
    return not (*this == point);
  }

  Point operator + (const Point & p) const
  {
    return Point(x + p.x, y + p.y);
  }

  Point & operator += (const Point & p)
  {
    x += p.x;
    y += p.y;

    return *this;
  }

  Point operator - (const Point & p) const
  {
    return Point(x - p.x, y - p.y);
  }

  Point & operator -= (const Point & p)
  {
    x -= p.x;
    y -= p.y;

    return *this;
  }

  const Geom_Number & get_x() const /// Returns x value
  {
    return x;
  }

  const Geom_Number & get_y() const /// Returns y value
  {
    return y;
  }

  /// Returns true if this is colinear with p1 and p2.
  bool is_colinear_with(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(*this, p1, p2) == 0;
  }

  /// Returns true if this is colinear with segment s.
  inline bool is_colinear_with(const Segment & s) const;

  /// Return true if this is to left from points p1 and p2
  bool is_to_left_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) > 0;
  }

  /// Return true if this is to right from points p1 and p2
  bool is_to_right_from(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(p1, p2, *this) < 0;
  }

  /// Return true if this is to left from (or on) points p1 and p2
  bool is_to_left_on_from(const Point & p1, const Point & p2) const
  {
    return not is_to_right_from(p1, p2);
  }

  /// Return true if this is to right from (or on) points p1 and p2
  bool is_to_right_on_from(const Point & p1, const Point & p2) const
  {
    return not is_to_left_from(p1, p2);
  }

  /// Returns true if the sequence this-p1-p2 is clockwise.
  bool is_clockwise_with(const Point & p1, const Point & p2) const
  {
    return area_of_parallelogram(*this, p1, p2) < 0;
  }

  inline bool is_to_left_from(const Segment & s) const;

  inline bool is_to_right_from(const Segment & s) const;

  inline bool is_clockwise_with(const Segment & s) const;

  /// Returns true if this is between p1 and p2.
  bool is_between(const Point & p1, const Point & p2) const
  {
    if (not this->is_colinear_with(p1, p2))
      return false;

    if (p1.get_x() == p2.get_x())
      return ((p1.get_y() <= this->get_y()) and (this->get_y() <= p2.get_y()))
        or ((p1.get_y() >= this->get_y()) and (this->get_y() >= p2.get_y()));  

    return ((p1.get_x() <= this->get_x()) and (this->get_x() <= p2.get_x()))
      or ((p1.get_x() >= this->get_x()) and (this->get_x() >= p2.get_x()));
  }

  /// Return the nearest point (to this) between p1 and p2.
  const Point & nearest_point(const Point & p1, const Point & p2) const 
  {
    return this->distance_with(p1) < this->distance_with(p2) ? p1 : p2;
  }

  /// Returns true if this is inside of segment s.
  inline bool is_inside(const Segment & s) const;

  /// Returns true if this is inside of the ellipse e.
  inline bool is_inside(const Ellipse & e) const;

  /// retorna true si this intercepta con la elipse e
  inline bool intersects_with(const Ellipse & e) const;

  /// Return a string representation of this.
  std::string to_string() const
  {
    return "(" + std::to_string(geom_number_to_double(x)) + "," +
      std::to_string(geom_number_to_double(y)) + ")";
  }

  /// String cast operator.
  operator std::string () const { return to_string(); }

  /// Returns the square distance between this y that
  inline Geom_Number distance_squared_to(const Point & that) const;

  /// Returns the euclidean distance between this and p.
  inline Geom_Number distance_with(const Point & p) const;

  const Point & highest_point() const { return *this; }

  const Point & lowest_point() const { return *this; }

  const Point & leftmost_point() const { return *this; }

  const Point & rightmost_point() const { return *this; }
};

extern const Point NullPoint;


/** Convert to polar coordinates a point in the cartesian plane.
    
    This class is very useful for problems where you need to use angles
    (rotations by example).
    
    @ingroup Geometry.
*/
class Polar_Point : public Geom_Object
{
  friend class Point;
  
  Geom_Number r;
  Geom_Number theta;
  
public:

  /// Returns the magnitude
  const Geom_Number & get_r() const { return r; }

  /// Returns the angle. 
  const Geom_Number & get_theta() const { return theta; }

  Polar_Point(const Geom_Number & __r, const Geom_Number & __theta)
    : r(__r), theta(__theta)
  {
    // empty
  }

  /// Constructor a partir de un punto cartesiano. El origen es (0,0)
  Polar_Point(const Point & p) : Geom_Object(p)
  {
    const Geom_Number & x = p.x;
    const Geom_Number & y = p.y;

    r = pitag(x, y); // radio se determima por Pitágoras

        // ahora determinamos el angulo según signos

    if (x == 0) // ¿ningún angulo?
      {
	theta = y >= 0 ? PI/2 : 3*PI/2;

	return;
      }

    if (y == 0) // ¡PI/2 (90 grados)?
      {
	theta = x >= 0 ? 0 : PI;

	return;
      }

    theta = arctan(y/x);

    if (x > 0 and y > 0) // ¿1er cuadrante?
      return;

    if (x < 0) // ¿2do o 3er cuadrante?
      r = -r;
  }

  /// Cuadrantes 
  enum Quadrant { First, Second, Third, Fourth };

  /// Retorna el cuadrante donde se encuentra el punto
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

  /// Convierte a string
  std::string to_string() const
  {
    return "[" + std::to_string(geom_number_to_double(r)) + "," +
      std::to_string(geom_number_to_double(theta)) + "]";
  }

  Polar_Point() : r(0), theta(0) { /* empty */ }
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

  const Point & highest_point() const 
  {
    return  src.y > tgt.y ? src : tgt;
  }

  const Point & lowest_point() const 
  {
    return  src.y < tgt.y ? src : tgt;
  }

  const Point & leftmost_point() const 
  {
    return  src.x < tgt.x ? src : tgt;
  }

  const Point & rightmost_point() const 
  {
    return  src.x > tgt.x ? src : tgt;
  }

  const Point & get_src_point() const { return src; }

  const Point & get_tgt_point() const { return tgt; }

  Segment() { /* empty */ }

  // Segment(const Segment & s) 
  //   : Geom_Object(), src(s.src), tgt(s.tgt)
  // {
  //   // empty
  // }

  Segment(const Point & __src, const Point & __tgt)
    : Geom_Object(), src(__src), tgt(__tgt)
  {
    // empty
  }

private:

  /** Computes the target point of a segment given the source point, slope and
      length.
      
      It uses the solution of this equations:

         d^2 = (tx-sy)^2 + (ty-sy)^2  (Pitágoras)

         ty - sy = m(tx-sx)           (ecuación de recta)

	 Return the point placed to the east (right).
  */
      static  
  Point compute_tgt_point(const Point &       __src, // punto de origen
			  const Geom_Number & m,     // pendiente
			  const Geom_Number & d)     // longitud segmento
  {
    const Geom_Number den2 = 1 + m*m;

    const Geom_Number den = square_root(den2);

    const Geom_Number x = __src.x + d/den;
    
    const Geom_Number y = __src.y + d*m/den;

    return Point(x, y);
  }

public:

  /** Builds a new segment given source point, slope and length.

      It computes the source point by usen these equations:
     -#\f$ d^2 = (x - x1)^2 + (y - y1)^2\f$ (Pitágoras)
     -#\f$ y - y1 = m*(x - x1)\f$ (ecuación de recta dada punto y
     pendiente)

     @param[in] __src Source point
     @param[in] m slope (in radians)
     @param[in] l length
  */
  Segment(const Point &       __src, // punto de origen
	  const Geom_Number & m,     // pendiente
	  const Geom_Number & l)     // longitud de la recta
    : Geom_Object(), src(__src), tgt(compute_tgt_point(src, m, l))
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

  double slope() const
  {
    return compute_slope();
  }
  
  /// Compute the counterclock wise angle respect another segment s.
  double counterclockwise_angle_with(const Segment & s) const
  {
    Geom_Number x1 = tgt.x - src.x;
    Geom_Number x2 = s.tgt.x - s.src.x;
    Geom_Number y1 = tgt.y - src.y;
    Geom_Number y2 = s.tgt.y - s.src.y;
    Geom_Number dot = x1 * x2 + y1 * y2;
    Geom_Number det = x1 * y2 - y1 * x2;

    Geom_Number angle = arctan2(det, dot);

    if (angle == 0)
      return 0;

    if (angle < 0)
      return -angle.get_d();
    
    return 2 * PI - angle.get_d();
  }

  /// Compute the counterclock angle of this respect x-axis.
  double counterclockwise_angle() const
  {
    Segment x_axis(Point(0, 0), Point(1, 0));
    return counterclockwise_angle_with(x_axis);
  }

      // retorna la longitud del segmento; es decir la distancia
      // euclidiana entre los puntos origen y destino
  Geom_Number size() const
  {
    return pitag(tgt.x - src.x, tgt.y - src.y);
  }

      // retorna true si p es colineal al segmento this
  bool is_colinear_with(const Point & p) const
  {
    return p.is_colinear_with(src, tgt);
  }

      // retorna true si segmento this está a la izquierda de punto p
  bool is_to_left_from(const Point & p) const
  {
    return p.is_to_right_from(*this);
  }

      // retorna true si segmento this está a la derecha del punto p
  bool is_to_right_from(const Point & p) const
  {
    return p.is_to_left_from(*this);
  }

      // retorna el punto medio del segmento this
  Point mid_point() const
  {
    const Geom_Number x = (src.get_x() + tgt.get_x()) / 2;
    const Geom_Number y = (src.get_y() + tgt.get_y()) / 2;

    return Point(x, y);
  }


    // retorna el punto más cercano al punto p entre y los extremos del
    // segmento this
  const Point & nearest_point(const Point & p) const
  {
    return p.nearest_point(get_src_point(), get_tgt_point());
  }

  // Construye un segmento perpendicular a this que pase por el punto p.
  Segment get_perpendicular(const Point & p) const
  {
    if (src == tgt)
      return *this;

    /* Si la pendiente es cero, construyo un segmento vertical que comience
       en la componente x de p y componente y de cualquier extremo del segmento
       y que termine en p.
    */
    if (src.get_y() == tgt.get_y())
      return Segment(Point(p.get_x(), src.get_y()), p);

    // Si this es vertical, entonces construyo un segmento horizontal.
    if (src.get_x() == tgt.get_x())
      return Segment(Point(src.get_x(), p.get_y()), p);

    Geom_Number m1 = Geom_Number(slope());

    // Calculo la pendiente del nuevo segmento
    Geom_Number m2 = -Geom_Number(1.0) / m1;

    /* Con esta información calculo el punto de intersección de las rectas
       formadas por ambos segmentos.
    */
    const Geom_Number & x1 = src.get_x();
    const Geom_Number & y1 = src.get_y();
    const Geom_Number & x2 = p.get_x();
    const Geom_Number & y2 = p.get_y();

    Geom_Number x = (y2 - y1 + m1 * x1 - m2 * x2) / (m1 - m2);
    Geom_Number y = m1 * (x - x1) + y1;

    // El nuevo segmento comienza en la intersección y termina en p
    return Segment(Point(x, y), p);
  }

      // retorna el segmento perpendicular que cruza por el punto medio
      // del segmento this de longitud 2*dist. Los puntos origen y
      // destino del segmento resultante conforman los punto
      // perpendiculares a distancia dist del centro del segmento this
  Segment mid_perpendicular(const Geom_Number & dist) const
  {
        // llevamos punto destino del segmento al origen y luego lo
        // transformamos a coordenadas polares
    const Polar_Point tgt_polar(tgt - src);
    
        // arco del segmento this respecto a la horizontal
    const Geom_Number arc_tgt_src = tgt_polar.get_theta();

        // arco del punto perpendicular entre segmentos this y
        // src--perp, donde perp es el punto resultado
    Geom_Number arc_perp_pt = arctan(dist/(tgt_polar.get_r()/2));

    const Geom_Number mperp = dist/(tgt_polar.get_r()/2);

        // distancia entre src y perp
    Geom_Number perp_r = pitag(dist, tgt_polar.get_r()/2);

    if (tgt_polar.get_r() < 0) 
      perp_r = - perp_r; // si el radio es negativo ==> el radio del
			 // punto medio también lo es

    if (tgt_polar.get_theta() < 0)
      arc_perp_pt = - arc_perp_pt; // si el angulo es negativo ==> el
				   // radio del punto medio también lo es 

        // punto perpendicular a la izquierda del segmento this y
        // respecto a src (en coordenada polar)
    const Polar_Point polar_perp_pt_l(perp_r, arc_tgt_src + arc_perp_pt);

        // punto perpendicular a la derecha del segmento this y respecto
        // a src (en coordenada polar)
    const Polar_Point polar_perp_pt_r(perp_r, arc_tgt_src - arc_perp_pt);

        // determine los puntos en coordenadas rectangulares
    const Point p1(Point(polar_perp_pt_l) + src);
    const Point p2(Point(polar_perp_pt_r) + src);
    
        // segmento resultado debe ir en sentido antihorario respecto al
        // segmento this
    if (p1.is_to_right_from(*this))
	return Segment(p1, p2);
    else
      return Segment(p2, p1);
  }

      // retorna true si hay intersección propia entre los segmentos.
      //
      // Una intersección propia es cuando el punto de intersección está
      // contenido en los segmentos
  bool intersects_properly_with(const Segment & s) const
  {
        // verifica las 4 combinaciones posibles de colinealidad
    if (src.is_colinear_with(s) or tgt.is_colinear_with(s) or
	s.src.is_colinear_with(*this) or s.tgt.is_colinear_with(*this))
      return false;

        // Hay intersección si, para cada segmento, un punto está a la
        // izquierda y el otro a la derecha 
    return ((src.is_to_left_from(s) xor tgt.is_to_left_from(s)) and
	    (s.src.is_to_left_from(*this) xor s.tgt.is_to_left_from(*this)));
  }

      // retorna true si p está contenido en el segmento this
  bool contains_to(const Point & p) const
  {
    return p.is_between(src, tgt);
  }

      // retorna true si s está contenido en el segmento this
  bool contains_to(const Segment & s) const
  {
    return (s.get_src_point().is_between(src, tgt) and 
	    s.get_tgt_point().is_between(src, tgt));
  }

      // retorna true si s intersecta con segmento this
  bool intersects_with(const Segment & s) const
  {
    if (this->intersects_properly_with(s))
      return true;

        // si no hay intersección propia ==> verificamos si alguno de los
        // puntos del segmento está contenido en el otro

    return (this->contains_to(s.src) or this->contains_to(s.tgt) or
	    s.contains_to(this->src) or s.contains_to(this->tgt));
  }

      // retorna true si segmento this intersecta con triangulo t
  inline bool intersects_with(const Triangle & t) const;

      // retorna true si segmento this intersecta con elipse e
  inline bool intersects_with(const Ellipse & e) const;

      // retorna true si segmento this es paralelo a segmento s
  bool is_parallel_with(const Segment & s) const
  {
    return slope() == s.slope();
  }

  /*
    Calcula la intersección de dos segmentos 

    Se se sirve de las ecuaciones punto y pendiente

    y - y1 = m1 (x - x1)      ((x1,y1) = this->src , m1 = this->slope())
    y - y2 = m2 (x - x2)      ((x1,y1) = s->src , m2 = s_slope())
  */
  Point intersection_with(const Segment & s) const
  {
    ah_domain_error_if(this->is_parallel_with(s)) << "Segments are parallels";
    
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

  // TODO: este cálculo debe pasar a coordenadas polares

      // sentidos cardinales de dirección de un segmento
  enum Sense { E, NE, N, NW, W, S, SW, SE };

  Sense sense() const
  {
    if (src.x < tgt.x) // ¿está hacia el este?
      {
	if (src.y < tgt.y) // ¿está al norte?
	  return NE; 
	else if (src.y > tgt.y) // ¿está al sur?
	  return SE;
	else 
	  return E;
      }

    if (src.x > tgt.x) // ¿está hacia el oeste?
      {
	if (src.y < tgt.y) // ¿está al norte?
	  return NW;
	else if (src.y > tgt.y) // ¿está al sur?
	  return SW;
	else return W;
      }

        // en este punto el segmento es con certitud vertical

    return src.y > tgt.y ? N : S;
  }

      // aumenta la distancia del segmento en __dist desde el punto origen
  void enlarge_src(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(src, m, - __dist);

    const Segment s(p, src);

    if (s.size() < __dist)
      p = compute_tgt_point(src, m, __dist);

    src = p;
  }

      // aumenta la distancia del segmento en __dist desde el punto destino
  void enlarge_tgt(const Geom_Number & __dist)
  {
    const double m = slope();

    Point p = compute_tgt_point(tgt, m, __dist);

    Segment s(tgt, p);

    if (s.size() < __dist)
      p = compute_tgt_point(tgt, m, - __dist);

    tgt = p;
  }

      // construye un string de la forma "(src)(tgt)"
  std::string to_string() const
  {
    return src.to_string() + tgt.to_string();
  }

      // convierte el segmento a un string de la forma "(src)(tgt)"
  operator std::string () const
  {
    return to_string();
  }

      // gira el segmento en ángulo angle alrededor del punto origen
      // manteniendo la misma longitud del segmento; lo que implica que
      // el punto destino tgt cambia
  /*
    Procedimiento

    1) Normalizar el segmento al origen ==> tgt - src

    2) Tranformar tgt a coordenada polar ptgt

    3) El nuevo ptgt es la coordenada polar del punto con angulo sumado
       en angle; es decir ptgt Polar_Point(r, theta + angle)

    4) Llevar ptgt a coordenadas rectangulares tgt

    5) El resultado es tgt + src
   */
  void rotate(const double & angle) 
  {
    if (angle == 0)
      return;

    const Polar_Point ptgt(tgt - src); // tgt en coordenadas polares

        // lo desplazamos en angle radianes
    tgt = Polar_Point(ptgt.get_r(), ptgt.get_theta() + angle);

    tgt = tgt + src; // lo regresamos al punto de referencia src (el polo)
  }

      // retorna el segmento intersección con el triangulo t (si existe)
  inline Segment intersection_with(const Triangle & t) const; 

    // retorna el segmento intersección con la elipse e (si existe)  
  inline Segment intersection_with(const Ellipse & e) const;
};

    // retorna true si this está contenido en el segmento this
inline bool Point::is_inside(const Segment & s) const
{
  return s.contains_to(*this);
}

    // retorna true si el punto this es colineal con el segmento s
inline bool Point::is_colinear_with(const Segment & s) const
{
  return this->is_colinear_with(s.src, s.tgt);
}

    // retorna true si el punto this está a la izquierda del segmento s
inline bool Point::is_to_left_from(const Segment & s) const
{
  return this->is_to_left_from(s.src, s.tgt);
}

    // retorna true si el punto this está a la derecha del segmento s
inline bool Point::is_to_right_from(const Segment & s) const
{
  return this->is_to_right_from(s.src, s.tgt);
}

    // retorna true si la secuencia this--s está en sentido horario
inline bool Point::is_clockwise_with(const Segment & s) const
{
  return this->is_clockwise_with(s.src, s.tgt);
}



    // retorna la distancia euclidiana entre el punto this y el punto p
inline Geom_Number Point::distance_squared_to(const Point & that) const
{
  Geom_Number dx = this->x - that.x;
  Geom_Number dy = this->y - that.y;
  return dx*dx + dy*dy;
}


    // retorna la distancia euclidiana entre el punto this y el punto p
inline Geom_Number Point::distance_with(const Point & p) const
{
  const Segment seg(*this, p);

  return seg.size();
}


/*****************************************************************

		      Clase fundamental triangulo

  La idea de esta clase es servir como objeto de uso básico en los
  algoritmos de triangulización de polígonos
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

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are colinears";
  }

  Triangle(const Point & p, const Segment & s)
    : p1(p), p2(s.src), p3(s.tgt)
  {
    __area = area_of_parallelogram(p1, p2, p3)/2;

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are colinears";
  }

  Triangle(const Segment & s, const Point & p)
    : p1(s.src), p2(s.tgt), p3(p)
  {
    __area = area_of_parallelogram(p1, p2, p3)/2;

    ah_domain_error_if(__area == 0)
      << "The three points of triangle are colinears";
  }

  Geom_Number area() const
  {
    return abs(__area);
  }

      // retorna true si los vértices del triangulo están en sentido horario
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

      // retorna true si el punto p está contenido dentro del triángulo
  bool contains_to(const Point & p) const
  {
    const bool s = p.is_to_left_from(p1, p2);

    if (p.is_to_left_from(p2, p3) != s)
      return false;

    if (p.is_to_left_from(p3, p1) != s)
      return false;

    return true;
  }

      // retorna el segmento "intersección" del triangulo con el segmento s
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
    // empty 
  }

  Rectangle(const Geom_Number & __xmin, const Geom_Number & __ymin, 
	    const Geom_Number & __xmax, const Geom_Number & __ymax) 
    : xmin(__xmin), ymin(__ymin), xmax(__xmax), ymax(__ymax)
  {
    ah_range_error_if(xmax < xmin || ymax < ymin) << "Invalid rectangle";
  }
    
  void set_rect(const Geom_Number & xmin, const Geom_Number & ymin, 
		const Geom_Number & xmax, const Geom_Number & ymax) 
  {
    new (this) Rectangle(xmin, ymin, xmax, ymax);
  }

  Geom_Number width()  { return xmax - xmin; }

  Geom_Number height() { return ymax - ymin; }

  // does this axis-aligned rectangle intersect that one?
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

  // distance from p to closest point on this axis-aligned rectangle
  Geom_Number distance_to(const Point & p) 
  {
    return sqrt(mpfr_class(distance_squared_to(p)));
  }
    
  // does this axis-aligned rectangle contain p?
  bool contains(const Point & p) const
  {
    return p.get_x() >= xmin and p.get_x() <= xmax and 
      p.get_y() >= ymin and p.get_y() <= ymax;
  }


};

    // retorna true si el segmento this intersecta con uno o dos lados del
    // triangulo t
inline bool Segment::intersects_with(const Triangle & t) const
{
  return (this->intersects_with(Segment(t.get_p1(), t.get_p2())) or
	  this->intersects_with(Segment(t.get_p2(), t.get_p3())) or
	  this->intersects_with(Segment(t.get_p3(), t.get_p1())));
}


    // retotrna el segmento resultante de la intersección del segmento this
    // con el triangulo t. Note que si un punto del segmento this está
    // dentro del triangulo, entonces la intersección es un punto ==>
    // este método podría usarse para determinar si un punto está o no
    // dentro del triangulo
inline Segment Segment::intersection_with(const Triangle & t) const
{
  ah_domain_error_if(not this->intersects_with(t))
    << "segment does not intersects with triangle";

  Point p[2]; 

  int i = 0;

  try 
    {    // revisa si hay intersección con el lado p1-p2
      p[i] = this->intersection_with(Segment(t.get_p1(), t.get_p2()));

      if (t.contains_to(p[i]))
	++i; // intersección detectada
    }
  catch (std::domain_error&)
    {
      // No hay intersección con el segmento anterior. Proseguimos ...
    }

  try
    {    // revisa si hay intersección con el lado p2-p3
      p[i] = this->intersection_with(Segment(t.get_p2(), t.get_p3()));

      if (t.contains_to(p[i]))
	++i;// intersección detectada
    }
  catch (std::domain_error&)
    {
      // No hay intersección con el segmento anterior. Proseguimos ...
    }

  if (i == 2) // si ya tenemos dos puntos de intersección ==> ya no
	      // queda más que hacer sino retornar el segmento
    return Segment(p[0], p[1]);

  try
    {    // revisa si hay intersección con el lado p3-p1
      p[i] = this->intersection_with(Segment(t.get_p3(), t.get_p1()));
    }
  catch (std::domain_error&)
    {
      throw; // algo serio debe ocurrir si hay excepción aquí. Esto es
	     // un bug pues previamente se preguntó 
    }
  
      // el resultado depende de la cantidad de puntos de intersección que
      // tengamos 
  return i == 1 ? Segment(p[0], p[0]) : Segment(p[0], p[1]);
}


/*****************************************************************

			Clase fundamental Elipse

 */
class Ellipse : public Geom_Object
{
  friend class Point;

  /*
    Se asume la siguiente ecuación en el centro (xc, yc):

                                       2           2
                               (y - yc)    (x - xc)
			       --------- + --------- = 1
			          2           2
				vr          hr

  */

  Point center; // punto centro

  Geom_Number hr; // radio horizontal (parámetro a) 
  Geom_Number vr; // radio vertical (parámetro b) 
 
public:

  Ellipse(const Point &       __center, 
	  const Geom_Number & __hr, 
	  const Geom_Number & __vr)
    : center(__center), hr(__hr), vr(__vr)
  {
    // empty
  }

  Ellipse(const Ellipse & e)
    : Geom_Object(e), center(e.center), hr(e.hr), vr(e.vr)
  {
    // empty
  }

  Ellipse() { /* empty */ }

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

  /* Calcula las tagentes a la elipse this con pendiente m
     
     Se calcula según ecuación;

         y = m x + sqrt(a^2 m^2 + b^2)

     que es la ecuación de las tangentes de la elipse con centro (0,0).

     La ecuacióon anterior es resultante de igualar la ecuación
     simplificada de la elipse en (0,0) y la recta con tangente m. Es
     decir, substituir y = mx + y0 en

                               2     2
                              y     x
			     --- + --- = 1
			       2      2
                             vr     hr

     ATENCIÓN: posibles errores de precisión debido a la irracionalidad

     s1 y s2 son las tangente y m es la pendiente
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

        // este es corte de la tangente con la abscisa si la elipse
        // estuviese centrada en (0,0) 
    const Geom_Number y1= square_root(product);

    const Geom_Number x1 = -y1/m;
    
        // si la elipse está centrada en (0,0) entonces los puntos de las
        // tangentes son (0, y1) y (0, -y1). Los puntos reales, respecto a
        // la elipse centrada en center se calculan según la recta
        // conformada por (0,0)--center

    Segment t1 = Segment(center + Point(x1, 0), center + Point(0, y1));

    Segment t2 = Segment(center + Point(-x1, 0), center + Point(0, -y1));

        // estos segmentos son tangentes a la elipse, pero, según su
        // pendiente, éstos podrían ser demasiado grandes. Por tanto,
        // para cada segmento, escogeremos el punto que está más cercano
        // al centro de la elipse

        // decide el tamaño de la tangente en función del mayor radio
    const Geom_Number tangent_size = hr > vr ? hr : vr;

    {    // calcula distancias desde el centro hasta los puntos extremo
	 // de la tangente t1 
      const Geom_Number dsrc = center.distance_with(t1.get_src_point());
      const Geom_Number dtgt = center.distance_with(t1.get_tgt_point());

      if (dsrc < dtgt) // selecciona el punto más cercano al centro
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
    
    {    // calcula distancias desde el centro hasta los puntos extremo
	 // de la tangente t1 
      const Geom_Number dsrc = center.distance_with(t2.get_src_point());
      const Geom_Number dtgt = center.distance_with(t2.get_tgt_point());

      if (dsrc < dtgt) // selecciona el punto más cercano al centro
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

      // retorna true si s intersecta con la elipse
  bool intersects_with(const Segment & s) const
  {
    Segment tg1;
    Segment tg2;

    compute_tangents(tg1, tg2, s.slope()); // calcula las dos tangentes

        // existe intersección si s está entre las tangentes de la
        // elipse que son paralelas a s
    return (s.is_to_left_from(tg1.get_src_point()) xor 
	    s.is_to_left_from(tg2.get_tgt_point()));
  }

private:

      // Calcula el valor de:
      //
      //     (p.x - xc)^2 + (p.y - yc)^2
      //     ------------   ------------ = 1
      //         a^2            b^2
      //
      // lo que permitirá determinar si un punto está o no circunscrito
      // por la elipse
  Geom_Number compute_radius(const Point & p) const
  {
    Geom_Number x2 = (p.get_x() - center.get_x());
    x2 = x2*x2;

    Geom_Number y2 = (p.get_y() - center.get_y());
    y2 = y2*y2;
    
    return x2/(hr*hr) + y2/(vr*vr);
  }

public:

      // retotrna true si el punto p está contenido dentro de la elipse this
  bool contains_to(const Point & p) const
  {
    return compute_radius(p) <= 1;
  }

      // retotrna true si el punto p pertenece exactamente a la curvade
      // la elipse this 
  bool intersects_with(const Point & p) const
  {
    return compute_radius(p) == 1;
  }

  /*
    Rutinas de intersección con segmento. Para ello, se resuelve el
    sistema de ecuaciones planteado por la ecuación de la elipse 

                             2    2
                            y    x
           eq1              -- + -- = 1
                             2    2
                            b    a

    y la ecuación de la recta del segmento:

                eq2              y - yr = m*(x - xr)

    Donde (xr,yr) es un punto de la recta transformada al plano del centro
    de la elipse

    Lo cual arroja los siguientes puntos (segun maxima
    solve([eq1,eq2],[x,y]);) 

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

     Nótese que se presume que el centro de la elipse está en (0,0)

     Para calcular la intersección, la recta se desplaza
     proporcionalmente con la distancia del centro de la elipse. Luego
     se calculan los puntos de intersección según las ecuaciones
     anteriores. Finalmente, el segmento resultante se desplaza a la
     posición real de la elipse

     BUG: hay un error. estudiar código generado por graphpic para.
          el detalle parece aparecer cuando la recta pasa por centro de
          la elipse  

	  Los puntos intersección con diferentes puntos de la recta, dan
	  diferentes y deberían dar iguales ¿por qué?
  */
  Segment intersection_with(const Segment & sg) const
  {

    ah_domain_error_if(not intersects_with(sg)) << "there is no intersection";

    const Geom_Number & a  = hr;
    const Geom_Number & b  = vr;

    const Geom_Number a2  = a*a;
    const Geom_Number b2  = b*b;

    const Geom_Number ab  = a*b;

        // segmento desplazado al eje de coordenadas con origen en (xc, yc)
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

        // hechas la cuentas principales, calculamos los valores

    const Geom_Number x1 = - (ab_root + sumx) / a2m2_plus_b2;

    const Geom_Number y1 = - (ab_m_root - sumy) / a2m2_plus_b2;

    const Geom_Number x2 = (ab_root - sumx) / a2m2_plus_b2;

    const Geom_Number y2 = (ab_m_root + sumy) / a2m2_plus_b2;

        // como los resultados son para la elipse en (0,0), reajustamos los
        // puntos de intersección al centro real de la elipse

    const Point src = Point(x1, y1) + center;
    const Point tgt = Point(x2, y2) + center;

    return Segment(src, tgt);
  }
};

    // retorna true si el punto this está contenido dentro de la elipse e
inline bool Point::is_inside(const Ellipse & e) const
{
  return e.contains_to(*this);
}

    // retorna true si el punto this intersecta exactamente con la elipse e
inline bool Point::intersects_with(const Ellipse & e) const
{
  return e.intersects_with(*this);
}

    // retorna true si el segmento this intersecta a la elipse
inline bool Segment::intersects_with(const Ellipse & e) const
{
  return e.intersects_with(*this);
}

    // retorna el segmento resultante de los dos puntos de intersección del
    // segmento this con la elipse e
inline Segment Segment::intersection_with(const Ellipse & e) const
{
  return e.intersection_with(*this);
}

/*****************************************************************

		   Clase fundamental cadena de texto

   Utilizada para escribir cadenas en el plano. 

   No usamos offsets porque, en apariencia, desde este primer estadio de
   diseño, un offset puede especificarse por el invocante desplazando el
   punto
*/

  /* Esta rutina calcula un estimado en cantidad de caracteres imprimibles
     según que la cadena sea para LateX. Por ejemplo, no se contabilizan '\'
     '$' '{' '}' etc.
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
	      // salte todos los caracteres que conforman el comando LateX
	  for (++i; isalnum(ptr[i]) and ptr[i] != '\0'; /* nothing */)  
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
    // empty
  }

  Text() { /* empty */ }

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
