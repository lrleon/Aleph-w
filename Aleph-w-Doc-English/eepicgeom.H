
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
  @file eepicgeom.H
  @date 2002 - 2014
  @author Leandro León (Source Code), Julio Criollo (Documentation)
*/


# ifndef EEPICGEOM_H
# define EEPICGEOM_H


# include <fstream>
# include <tpl_dynDlist.H>
# include <tpl_sort_utils.H>
# include <point.H>
# include <polygon.H>

extern bool tiny_keys;

    /* ALL: dotted curves polygons*/

class Eepic_Plane;


/** 
  @brief Abstract geometric object to be drawn on a flat Eeepic 
*/
class Eepic_Geom_Object
{
  Eepic_Geom_Object();

  Eepic_Geom_Object(const Eepic_Geom_Object&);

protected:

/** 
  @brief Copy the geometric object ==> it is the responsibility
  handle this kind of memory 
*/
  const Geom_Object * const geom_object_ptr; 

/** 
  @brief This false flag indicates that there must be called
  to delete geom_object_ptr; the destroyer
*/
  bool to_delete; 

public:

  Eepic_Geom_Object(Geom_Object * ptr, const bool & __to_delete = false) 
    : geom_object_ptr(ptr), to_delete(__to_delete)
  {
    /* Empty */
  }

  virtual ~Eepic_Geom_Object() 
  { 
    if (to_delete)
      delete geom_object_ptr;
  }

 virtual void draw(Eepic_Plane *, ofstream &) const
  {
    if (geom_object_ptr == NULL)
      throw std::domain_error("Eepic_Geom_Object is null");

    throw std::logic_error("Call to draw() method from abstract class");
  }

/** 
  @brief You must return an object of the derived class
*/
  virtual Eepic_Geom_Object * clone() const = 0;

  virtual Point highest_point() const = 0;

  virtual Point lowest_point() const = 0;

  virtual Point leftmost_point() const = 0;

  virtual Point rightmost_point() const = 0;
};


class Eepic_Plane
{
/** 
  @note Picture frame environment defined at construction time
   -# double wide;
   -# double height;
  .
*/
  double wide;    /** Wide at points of the resolution. */
  double height;  /** High points of the resolution. */

/** 
  @note resulting final frame according to the user decides whether or not scaling Figure
   -# double real_wide;
   -# double real_height; 
  .
*/
  double real_wide;   /** Wide at points of the resolution. */
  double real_height; /** High points of the resolution. */

  double xoffset; /** Additional wide margin. */
  double yoffset; /** Additional margin height. */

  Geom_Number geom_wide;   /** Width from the leftmost point to the rightmost. */
  Geom_Number geom_height; /** Wide from the lowest point to the highest. */

  DynDlist<Eepic_Geom_Object*> list;

  Geom_Number geom_x_min; /** Minimum value of x in real coordinates. */
  Geom_Number geom_y_min; /** Minimum value and actual coordinates. */

/**
  @note ATTENTION:
  These values must be initialized to zero because
  They are used to calculate themselves
    -# double x_min;
    -# double y_min;
   .
*/
  double x_min; /** Minimum value of x in the plane. */
  double y_min; /** Miniumm value of y in the plane. */

/** 
  @note 
  Endpoints with which to calculate the width and height
   objects (geom_x_min and geom_y_min):
  -# Point __leftmost;
  -# Point __rightmost;
  -# Point __highest;
  -# Point __lowest;
 .
*/
  Point __leftmost;
  Point __rightmost;
  Point __highest;
  Point __lowest;

/** 
  @brief It indicates that you must draw two segments
  corresponding to the Cartesian axes. 
*/
  bool with_cartesian_axis; 

/** 
  @brief Radio failure to resolve a point in the plane,
  It is also used for the height of the letters. 
*/
  static const double default_r;

  double r; /* The actual radio spots with a draw point. */

/** 
  @note Parameters that set how
   latex picture environment is defined:  -# double resolution;
  -# string fill_type;
  -# string shade_type;
  -# static const double normal_thickness;
  -# static const double default_shade_thickness;
  -# double shade_thickness;
  -# static const double dotgap;
 . 
*/
  double resolution; /**Resolution millimeters (0.05mm default) */
  string fill_type; /** Fill type (black) */
  string shade_type; /** Type of shading to segments. */
  static const double normal_thickness;
  static const double default_shade_thickness;
  double shade_thickness;
  static const double dotgap; 

/**
  @brief scaling change

  @details Performs scaling of a point p
  on a scale plan geom_max a max scale plane.

             p * max
   result = ----------
             geom_max
  @param p
  @param geom-max
  @param max

  @return escala cambiada
*/
  double geom_number_to_plane(const Geom_Number & p,
			      const Geom_Number & geom_max,
			      const double &      max) const
  { 
    const Geom_Number ratio = p / geom_max;

    double result = geom_number_to_double(ratio);

    result = max * result;
    
    return result;
  }

/** 
  @note The points should be standardized to the size in pixels.
   These routines take the coordinate and standardize the zero axis (x o y): 
  -double xpic
  -double ypic
*/

/** 
  @brief normalizes the x axis 
*/
  double xpic(const double & x) const
  {
    return x - x_min;
  }

/** 
  @brief normalizes the x axis 
*/
  double ypic(const double & y) const
  {
    return y - y_min;
  }

public:

/** 
  @brief Geom_Number calculates a ratio of x to the
  equivalent in terms as wide 
*/
  double h_geom_number_to_eepic(const Geom_Number & x) const
  {
    return geom_number_to_plane(x, geom_wide, wide);
  }

/** 
  @brief Calculate the ratio of Geom_Number and his
  equivalent height in the plane as  
*/
  double v_geom_number_to_eepic(const Geom_Number & y) const
  {
    return geom_number_to_plane(y, geom_height, height);
  }

/** 
  @brief Calculates an x coordinate their type Geom_Number
  equivalent in the plane 
*/
  double x_geom_number_to_eepic(const Geom_Number & x) const
  {
    return xpic(geom_number_to_plane(x, geom_wide, wide));
  }

/** 
  @brief Calculates a coordinate of its kind Geom_Number
  equivalent in the plane
*/
  double y_geom_number_to_eepic(const Geom_Number & y) const
  {
    const double result = geom_number_to_plane(y, geom_height, height);

    return ypic(result);
  }

/** 
  @brief Performs a linear sweep of all geometric objects
  present and calculated end point. 
*/
  void compute_extreme_points()
  {

/** 
  @brief Through all the objects in search of extreme coordinates
  the izquierad more to the right, high and low 
*/
    DynDlist<Eepic_Geom_Object*>::Iterator it(list);

    Eepic_Geom_Object * curr = it.get_current();

    __lowest    = curr->lowest_point();
    __highest   = curr->highest_point();
    __leftmost  = curr->leftmost_point();
    __rightmost = curr->rightmost_point();

    for (it.next(); it.has_current(); it.next())
      {
	curr = it.get_current();

	if (curr->lowest_point().get_y() < __lowest.get_y())
	  __lowest = curr->lowest_point();

	if (curr->highest_point().get_y() > __highest.get_y())
	  __highest = curr->highest_point();

	if (curr->leftmost_point().get_x() < __leftmost.get_x())
	  __leftmost = curr->leftmost_point();

	if (curr->rightmost_point().get_x() > __rightmost.get_x())
	  __rightmost = curr->rightmost_point();
      }
  }

/**
  @brief Increases and decreases the level eepic points.
  
  @details This is the method used for
  increase or decrease the object eepic picture

  @param factor Factor

  @throw only it works if the factor is greater than zero(0)
*/ 
  void zoom(const double & factor)
  {
    if (factor <= 0)
      throw std::domain_error("zoom factor must be greater than zero");

    height *= factor;
    wide *= factor;
  }

private:

    /* Calculate the extreme coordinates and with it the width and height
     geometric objects placed in the plane
      
     == True square parameter makes the drawing Rescala
     Adjustment for items not distorted */

  void compute_geom_plane(const bool & squarize)
  {
    compute_extreme_points();
    
    /* Calculate the minimum margins according to figures contained */
    geom_height = __highest.get_y()   - __lowest.get_y();
    geom_wide   = __rightmost.get_x() - __leftmost.get_x();

    /* Calculate the minimum x and y coordinates*/
    geom_x_min = geom_number_to_double(__leftmost.get_x()); 
    geom_y_min = geom_number_to_double(__lowest.get_y());

    /* According to the lows, we adjust the plane (0,0). */
    x_min = h_geom_number_to_eepic(geom_x_min);
    y_min = v_geom_number_to_eepic(geom_y_min);

    /*This option indicates that the plane should not deform Figures. */
    if (squarize) 
      { 
	if (geom_wide > geom_height)
	  {
	    geom_height = geom_wide;
	    real_height = y_geom_number_to_eepic(__highest.get_y());
	    real_wide = wide;
	  }
	else
	  { 
	    geom_wide = geom_height;
	    real_wide = x_geom_number_to_eepic(__rightmost.get_x());
	    real_height = height;
	  }
      }
    else
      {
	real_wide = wide;
	real_height = height;
      }

    assert(geom_height > 0 and geom_wide > 0);
  }

  void draw_cartesian_axis(ofstream & output);

public:

  const Point & leftmost() const { return __leftmost; }

  const Point & rightmost() const { return __rightmost; }

  const Point & highest() const { return __highest; }

  const Point & lowest() const { return __lowest; }

/** 
  @brief Generates a string of the form "(x, y)" with
  normalized coordinates to the plane.

  @details Since the point p generates a string of the form "(x, y)"
  with normalized coordinates to the plane.
  
  This string is used for the generation eepic own.

  @param p

  @return string of the form "(x, y)".
*/
  string point_string(const Point & p)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y());

    return gnu::autosprintf("(%lf,%lf)", x, y);
  }

/**   
  @brief generates output stream in an environment
  \begin {picture}... \end {picture} contentivo Figures
  geometric present.

  @details The squarize == false parameter means that the scale of objects
  They are transformed to fit exactly in the plane defined in
  construction time. squarize == true ensures that the figures
  are drawn without distortion but at the expense of the exploited space.
*/
  void draw(ofstream & output, const bool & squarize = true)
  {
    compute_geom_plane(squarize);

/** 
  @brief Set the output format floating point. 
*/    
    output << fixed << setprecision(6); 

/** 
  @brief Generating measures and open header. 
*/
    output << "\\setlength{\\unitlength}{" << resolution << "mm}" << endl
	   << endl
	   << "\\filltype{" << fill_type << "}" << endl
	   << endl
	   << "% leftmost point is " << __leftmost.to_string() << " --> "
	   << point_string(__leftmost) << endl
	   << "% rightmost point is " << __rightmost.to_string()  << " --> "
	   << point_string(__rightmost) << endl
	   << "% highest point is " << __highest.to_string()  << " --> "
	   << point_string(__highest) << endl
	   << "% lowest point is " << __lowest.to_string()  << " --> "
	   << point_string(__lowest) << endl
	   << "% " << list.size() << " geometric objects were put in the plane"
	   << endl
	   << "\\begin{picture}(" << real_wide << "," << real_height << ")" 
	   << "(" << xoffset << "," << yoffset << ")" << endl
	   << endl;

/** 
  @brief If the flag is true. 
*/
    draw_cartesian_axis(output); 

/** 
  @brief Through each object and draws. 
*/
    for (DynDlist<Eepic_Geom_Object*>::Iterator it(list); 
	 it.has_current(); it.next())
      {
	Eepic_Geom_Object * curr = it.get_current();
	
/** 
  @brief Reset each time the thickness default form. 
*/
	output << "\\thinlines" << std::endl 
	       << std::endl;

	curr->draw(this, output); 
      }

/** 
  @brief close header. 
*/
    output << "\\end{picture}" << std::endl
	   << std::endl;
  }

/**
  @note Observers of the plane:
  -# const double & get_wide() const { return wide; }
  -# const double & get_height() const { return height; }
  -# const double & get_yoffset() const { return yoffset; }
  -# const double & get_xoffset() const { return xoffset; }
  -# const double & get_r() const { return r; }
 .
*/
  const double & get_wide() const { return wide; }

  const double & get_height() const { return height; }

  const double & get_yoffset() const { return yoffset; }

  const double & get_xoffset() const { return xoffset; }

  const double & get_r() const { return r; }

  Eepic_Plane(const double & __wide,
	      const double & __height,
	      const double & __xoffset = 0.0,
	      const double & __yoffset = 0.0)
    : wide(__wide), height(__height), 
      xoffset(__xoffset), yoffset(__yoffset), 
      x_min(0), y_min(0), with_cartesian_axis(false),
      r(default_r), resolution(0.05), shade_thickness(default_shade_thickness)
  {
    /* Empty */
  }

  ~Eepic_Plane()
  {
    while (not list.is_empty())
      delete list.remove_first();
  }

  void put_cartesian_axis()
  {
    with_cartesian_axis = true;
  }

/** 
  @brief Eepic_Plane handles objects Eepic_Geom_Object type.
  
  @details Eepic_Plane handles objects Eepic_Geom_Object type. But nevertheless,
  It is directly to handle objects derived from Geom_Object.
  This would be possible with a highly class Geom_Object
  polymorphic clone that allows handling methods, draw and points
  virtually ends. But this approach involves two
  problems:

       1. Geom_Object comes from "coarse grain" because, for example,
       Point is highly used. This would imply a cost of at least
       7 pointers in the virtual table.

       2. The polymorphic nature varies according to the plane; in the occurrence,
       draw on eepic is not equal to draw on a gtk + canvas.

  Polymorphism in Geom_Object not guarantee generality for drawing,
  as this depends on the type of plan or fund.

  For the above reason, we define later, an external method,
  called put_in_plane (plane, geom_obj) which does:
     
       1. Geom_obj clone to its corresponding eepic_geom_obj

       2. Insert the new eepic_geom_obj in the plane

  For simplicity of implementation, this function is public, but
  Private use is (anyway) is not simple to achieve its invocation.
*/
  void put(const Eepic_Geom_Object & __eepic_geom_obj)
  {
    Eepic_Geom_Object * eepic_geom_obj = __eepic_geom_obj.clone();

    list.append(eepic_geom_obj);
  }

  void set_resolution(const double & res)
  {
    resolution = res;
  }

  const double & get_resolution() const { return resolution; }

  void set_fill_type(const string & ftype)
  {
    fill_type = ftype;
  }

  void set_shade_thickness(const double & __thickness)
  {
    shade_thickness = __thickness;
  }

  double get_shade_thickness() const { return shade_thickness; }

  double get_dotgap() const { return dotgap; }

/**
  @note definition of "primitive methods" of eepic drawn geometric objects.
  The draw () methods of each class can invoke one (s) of these methods.
  Also, the methods can be invoked by the user (at your own risk, of course)
  This family of "primitive methods" assumes that endpoints required
  to map a point to plane eepic already they have been calculated by the method
  compute_geom_plane ();
*/

/** 
  @brief Write in the stream output code for the eepic
  draw the point p
*/
  void draw_point(const Point & p, ofstream & output)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y());

    output << "    %% Point " << p.to_string()
	   << "    %% mapped in this plane to (" << x << "," << y << ")"
	   << std::endl
	   << "\\put(" << x << "," << y << "){\\ellipse{" << r
	   << "}{" << r << "}}" << std::endl
	   << std::endl;
  }

/** 
  @def General calculation coordinate origin and destination of a segment.
  The macro placed as to spare other lines so by statements
  and method invocation. 
*/
# define COMPUTE_SEGMENT_EEPIC_COORDENATES() \
  const double src_x = x_geom_number_to_eepic(sg.get_src_point().get_x()); \
\
  const double src_y = y_geom_number_to_eepic(sg.get_src_point().get_y()); \
\
  const double tgt_x = x_geom_number_to_eepic(sg.get_tgt_point().get_x()); \
\
  const double tgt_y = y_geom_number_to_eepic(sg.get_tgt_point().get_y()); 

/** @def In the spirit of previous macro
"COMPUTE_SEGMENT_EEPIC_COORDENATES ()" and because
refers to local variables defined therein,
sg this macro draws the segment on the output stream. */
# define DRAW_SEGMENT(sg, output) \
  COMPUTE_SEGMENT_EEPIC_COORDENATES(); \
  output << "    %% Segment from " << sg.get_src_point().to_string() \
	 << " to " << sg.get_tgt_point().to_string() << std::endl \
	 << "    %% mapped in this plane to (" << src_x << "," << src_y \
	 << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << "\\path(" << src_x << "," << src_y \
	 << ")(" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << std::endl; \

/** 
  @brief Write to the output stream the eepic sg code to draw the segment. \
*/
  void draw_segment(const Segment & sg, ofstream & output)
  {
    DRAW_SEGMENT(sg, output);
  }

  static const double arrow_width_in_mm;

  static const double arrow_lenght_in_mm;

/** 
  @def General calculation of the coordinates of the two lines pictorizan
  edges of the tips of an arrow and drawn in accordance with the
  origin and destination coordinates used in the macro "COMPUTE_SEGMENT_EEPIC_COORDENATES ()".
  We use a macro for the same reasons.
  And amplitus length of the edges are determined by the constants
  previous arrow_width_in_mm and arrow_lenght_in_mm. 
*/
# define PUT_ARROW() \
  const double arrow_width  = arrow_width_in_mm/resolution; \
  const double arrow_lenght = arrow_lenght_in_mm/resolution; \
\
  const double l = sqrt(arrow_width*arrow_width + arrow_lenght*arrow_lenght); \
  const double tetha = atan2(arrow_width, arrow_lenght); \
  const double phi = atan( fabs( (tgt_y - src_y)/(tgt_x - src_x) ) ); \
\
  double dx1 = l*cos(phi - tetha); \
  double dy1 = l*sin(phi - tetha); \
\
  double dx2 = l*sin(M_PI_2l - (phi + tetha)); \
  double dy2 = l*cos(M_PI_2l - (phi + tetha)); \
\
  if (tgt_x > src_x) \
    { \
      dx1 = -dx1; \
      dx2 = -dx2; \
    } \
\
  if (tgt_y > src_y) \
    { \
      dy1 = -dy1; \
      dy2 = -dy2; \
    } \
\
  output << endl \
         << "    % Drawing of arrow's edges at (" << tgt_x \
          << "," << tgt_y << ")" << endl \
	 << "\\path(" << tgt_x << "," << tgt_y << ")("  \
	 << tgt_x + dx1 << "," << tgt_y + dy1 << ")" << endl \
     	 << "\\path(" << tgt_x << "," << tgt_y << ")("  \
   	 << tgt_x + dx2 << "," << tgt_y + dy2 << ")" << endl << endl; 

/** 
  @def Macro to generate the output stream eepic code to draw
  sg segment with arrowheads.
*/
# define DRAW_ARROW(sg, output) \
  DRAW_SEGMENT(sg, output); \
  PUT_ARROW();

/** 
  @brief Write in the stream output code for the eepic
  sg draw the segment edged arrow. 
*/
  void draw_arrow(const Segment & sg, ofstream & output)
  {
    DRAW_ARROW(sg, output);
  }

/** 
  @def Macro to draw a segment puntillado. 
*/
# define DRAW_DOTTED_SEGMENT(sg, output) \
  COMPUTE_SEGMENT_EEPIC_COORDENATES(); \
\
  output << "    %% Dotted Segment from " << sg.get_src_point().to_string() \
	 << " to " << sg.get_tgt_point().to_string() << std::endl \
	 << "    %% mapped in this plane to (" << src_x << "," << src_y \
	 << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl \
         << "\\dottedline{" << dotgap << "}(" << src_x \
         << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << std::endl; 

/** 
  @brief Write in the stream output code for the eepic
  draw the segment puntillado sg. 
*/
  void draw_dotted_segment(const Segment & sg, ofstream & output)
  {
    DRAW_DOTTED_SEGMENT(sg, output);
  }

/** 
  @def Epic macro to generate code in the output setream
  sg shaped segment split line. 
*/
# define DRAW_DASH_SEGMENT(sg, output) \
  COMPUTE_SEGMENT_EEPIC_COORDENATES(); \
\
  output << "    %% Dash Segment from " << sg.get_src_point().to_string() \
	 << " to " << sg.get_tgt_point().to_string() << std::endl \
	 << "    %% mapped in this plane to (" << src_x << "," << src_y \
	 << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl \
         << "\\dashline{" << dotgap << "}(" << src_x \
         << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << std::endl; 

/** 
  @brief Write in the stream output code for the eepic
  sg draw the starting line segment.
*/
  void draw_dash_segment(const Segment & sg, ofstream & output)
  {
    DRAW_DASH_SEGMENT(sg, output);
  }

/** 
  @def Macro to draw sg puntillado segment with arrowheads. 
*/
# define DRAW_ARROW_DOTTED_SEGMENT(sg, output) \
  COMPUTE_SEGMENT_EEPIC_COORDENATES(); \
\
output << "    %% Dotted arrow Segment from " << sg.get_src_point().to_string()\
	 << " to " << sg.get_tgt_point().to_string() << std::endl \
	 << "    %% mapped in this plane to (" << src_x << "," << src_y \
	 << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl \
         << "\\dottedline{" << dotgap << "}(" << src_x \
         << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << std::endl; \
  PUT_ARROW();

/** 
  @breif Write in the stream output code for the eepic
  sg puntillado draw the segment with arrowheads. 
*/
  void draw_arrow_dotted_segment(const Segment & sg, ofstream & output)
  {
    DRAW_ARROW_DOTTED_SEGMENT(sg, output);
  }

/** 
  @def Macro to draw a line segment starting and Arrowhead. 
*/
# define DRAW_ARROW_DASH_SEGMENT(sg, output) \
  COMPUTE_SEGMENT_EEPIC_COORDENATES(); \
\
  output << "    %% Dash arrow Segment from " << sg.get_src_point().to_string()\
	 << " to " << sg.get_tgt_point().to_string() << std::endl \
	 << "    %% mapped in this plane to (" << src_x << "," << src_y \
	 << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl \
         << "\\dashline{" << dotgap << "}(" << src_x \
         << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl \
	 << std::endl; \
  PUT_ARROW();

/** 
  @brief Write in the stream output code for the eepic
  sg draw the segment with split line and arrowhead. 
*/
  void draw_arrow_dash_segment(const Segment & sg, ofstream & output)
  {
    DRAW_ARROW_DASH_SEGMENT(sg, output);
  }
/**
   @brief Write to the output stream the eepic code to draw the ellipse e. 
*/
  void draw_ellipse(const Ellipse & e, ofstream & output)
  {
    const Point & center = e.get_center();

    const double x = x_geom_number_to_eepic(center.get_x());
    
    const double y = y_geom_number_to_eepic(center.get_y());

    const double hd = h_geom_number_to_eepic(2*e.get_hradius());
    
    const double vd = v_geom_number_to_eepic(2*e.get_vradius());

    output << "    %% Ellipse with center in " << center.to_string()
	   << " with horizontal radius of " << e.get_hradius()
	   << " and vertical radius of " << e.get_vradius() << std::endl
	   << "    %% mapped in the plane at center (" << x
	   << "," << y << ") with horizontal radius of " << hd
	   << " and vertical radius of " << vd << std::endl
	   << "\\put(" << x << "," << y << "){\\ellipse{" << hd
	   << "}{" << vd << "}}" << std::endl
	   << std::endl;
  }

/** 
  @brief Defining function pointer to draw segment.
  
   @details This pointer can use a single function to draw polygons;
   Viewed as segments sides.
*/
  typedef void (Eepic_Plane::*Draw_Segment_Fct)(const Segment & poly, 
						ofstream &     output);

/** 
  @brief Write in the output stream the code to draw eepic
  polygon by the sides that comprise it.

  @details draw_sg_fct is a pointer to the function responsible for drawing a segment,
  which it is invoked whenever required to draw aside.
  
  @Note that the draw_polygonnote function is generic; indeed, it is also used
  to draw regular polygons.
*/
  template <class Poly>
  void draw_polygon(Poly &           poly, 
		    ofstream &       output,
		    Draw_Segment_Fct draw_sg_fct)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_current(); it.next())
      (this->*draw_sg_fct)(it.get_current_segment(), output);
  }
/**
  @breaf Write in the output stream the code to draw eepic
  poly shaded polygon with interior.

  @details is independent of whether or not closed. The difference from the version
  overloaded with three parameters it is that this creates a path
  vertices instead of a series of segments.

  This version is much faster than the version of three parameters;
  although less versatile (nothing is without cost; that's life).
*/
    template <class Poly>
  void draw_closed_polygon(Poly & poly, ofstream & output)
  {
    output << "    % Drawing of polygon of " << poly.size() 
	   << " sides" << endl
	   << "\\shade\\path";

    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      output << point_string(*it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

/**
  @brief Write in the output stream the code to draw eepic
  shaded polygon with its interior and side lines
  You unteadas.

  @note is independent of whether or not closed.
*/
    template <class Poly>
  void draw_dotted_closed_polygon(Poly &  poly, ofstream & output)
  {
    output << "    % Drawing of dotted line Polygon" << endl
	   << "\\shade\\dottedline{" << dotgap << "}";

    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      output << point_string(*it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

/**
  @brief Write in the output stream the code to draw eepic
  points for the polygon vertices poly points.

  @note CAUTION: Do not draw the sides; Only vertices dibula. 
*/
    template <class Poly>
  void draw_dots_from_polygon(Poly & poly, ofstream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      {
	Vertex * vertex = it.get_current_vertex();
      
	draw_point(*vertex, output);
      }
  }

/** 
  @brief Eepic generates code in the output stream to the edge of arrow segment sg. 
*/
  void draw_arrow_extreme(const Segment & sg, ofstream & output)
  {    /* ¡Tal cual! */
    COMPUTE_SEGMENT_EEPIC_COORDENATES();
    PUT_ARROW();
  }

/**
  @brief Write in the output stream the code to draw eepic
  arrowheads corresponding poly vertices of the polygon points.

  @note CAUTION: Do not draw the sides; Just draw the edges of the arrow
  what are the sides of the polygon seen as segments.
*/
      template <class Poly>
  void draw_arrows_in_polygon(Poly & poly, ofstream & output)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_current(); it.next())
      {
	const Segment & sg = it.get_current_segment();
      
	draw_arrow_extreme(sg, output);
      }
  }

    template <class Poly>
  void draw_closed_polygon_with_arrows(Poly & poly, ofstream & output)
  {
    draw_closed_polygon(poly, output);
    draw_arrows_in_polygon(poly, output);
  }

/**
  @brief Write in the output stream the code to draw eepic
  Text point corresponding to the point p.

  @details The text is of the form "(x, y)".
  Coordinate and moves slightly upward so r
  as to leave space to write the number of vertex below.
*/
  void draw_point_text(const Point & p, ofstream & output)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y()) + r;

    const string str = p.to_string();

    output << "    %% Text Point " << str
	   << "    %% mapped in this plane to (" << x << "," << y << ")"
	   << std::endl
	   << "\\put(" << x << "," << y << "){\\texttt{" << str
	   << "}}" << std::endl
	   << std::endl;
  }

/**
  @brief Write in the output stream the code to draw eepic
  texts of the vertices of the polygon poly.

  @note CAUTION: Do not draw the sides; just draw the texts of the vertices.
*/
    //    /**************************************************************** 
    //    ALL : Write the text inside or outside the polygon without
    //    cross a side ==> that the context of the sides is required ==> no
    //    could be used draw_point_text.
    //    *****************************************************************/
    template <class Poly>
  void draw_points_text_in_polygon(Poly & poly, ofstream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      draw_point_text(*it.get_current_vertex(), output);
  }

/**
  @brief Write to the output stream the code to draw eepic poly 
  vertex numbers of the polygon. 

  @note CAUTION: Do not draw the sides; Only numbers drawn vertices.
*/
      
    //    /****************************************************************
    //    ALL: Write the text inside or outside the polygon without
    //    cross a side ==> that the context of the sides is required ==> no
    //    could be used draw_point_text.
    //    *****************************************************************/
    template <class Poly>
  void draw_vertex_numbers_in_polygon(Poly & poly, ofstream & output)
  {
    int n = 0;

    for (typename Poly::Vertex_Iterator it(poly); 
	 it.has_current(); it.next(), ++n)
      {
	Vertex * vertex = it.get_current_vertex();
      
	const double x = x_geom_number_to_eepic(vertex->get_x());

	const double y = y_geom_number_to_eepic(vertex->get_y()) - r;

	output << "    %% Poligon Vertex Number " << n
	   << "    %% mapped in this plane to (" << x << "," << y << ")"
	   << std::endl
	   << "\\put(" << x << "," << y << "){$" << n << "$}" << std::endl
	   << std::endl;
      }
  }

/** 
  @brief Write to the output stream the code to draw eepic
  spline curve characterized by the points of the polygon and
  passes through its endpoints. 
*/
    template <class Poly>
  void draw_spline(Poly & poly, ofstream & output)
  {
    output << "    % Drawing spline of " << poly.size() 
	   << " points" << endl
	   << "\\spline";

    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      output << point_string(*it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

/** 
  @brief Write to the output stream the code to draw eepic
  starting line curve characterized by the points of the polygon
  and passing through the end points. 
*/
    template <class Poly>
  void draw_dash_spline(Poly & poly, ofstream & output)
  {
    output << "    % Drawing dash curve of " << poly.size() 
	   << " points" << endl
	   << "\\closecurve";

    for (typename Poly::Vertex_Iterator it(poly); it.has_current(); it.next())
      output << point_string(*it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << endl;
  }

/** 
  @brief Write in the output stream eepic code to draw the curve
  spline characterized by the points of the polygon and passing through points
  ends and whose last vertex has arrowhead. 
*/
    template <class Poly>
  void draw_spline_arrow(Poly & poly, ofstream & output)
  {
/** 
  @brief Draw the curve. 
*/
    draw_spline(poly, output);\

/** 
  @brief Drawing the edges of the arrows according to
  slope of the last segment of the polygon. 
*/ 
    draw_arrow_extreme(poly.get_last_segment(), output);
  }

/** 
  @brief Write in the stream output code for the eepic
  draw the line spline curve characterized by starting points
  polygon and passing through the end points and whose
  last vertex has Arrowhead. 
*/
    template <class Poly>
  void draw_dash_spline_arrow(Poly & poly, ofstream & output)
  {

/** 
  @brief Draw the curve
*/
    draw_dash_spline(poly, output); 

/** 
  @brief Drawing the edges of the arrows as
  the slope of the last segment of the polygon. 
*/
    draw_arrow_extreme(poly.get_last_segment(), output);
  }

  void draw_text(const Text & t, ofstream & output, 
		 const double & hfactor, const double & vfactor)
  {
    const Point & p = t.get_point();

    const double str_size_in_points = t.len()*Text::font_width_in_points;

    const double x = 
      x_geom_number_to_eepic(p.get_x()) - hfactor*str_size_in_points; 

    const double y = (y_geom_number_to_eepic(p.get_y()) - 
		      vfactor*Text::font_height_in_points); 

    output << "%    string at " << p.to_string() << endl 
	   << "%    mapped to the plane at (" << x << "," << y << ")" << endl 
	   << "\\put(" << x << "," << y << "){\\texttt{" 
	   << (tiny_keys ? "\\tiny " : "") << t.get_str() 
	   << "}}" << endl 
	   << endl;
  }
};


    //    /*****************************************************
    //    Definition generator derived objects Eepic_Geom_Object
    //    *****************************************************/

# define DEFINE_EEPIC_COPY_CTOR(Type) \
  Eepic_##Type(const Type & o) : Eepic_Geom_Object(new Type(o), true) { } 


    /* Common methods that always has a class Eepic_Type */
# define DEFINE_EEPIC_COMMON_METHODS(Type) \
friend void put_in_plane(Eepic_Plane & plane, const Type & geom_obj); \
\
~Eepic_##Type() { } \
\
virtual Point highest_point() const \
{ \
  return static_cast<const Type*>(geom_object_ptr)->highest_point(); \
} \
\
virtual Point lowest_point() const \
{ \
  return static_cast<const Type*>(geom_object_ptr)->lowest_point(); \
} \
\
virtual Point leftmost_point() const \
{ \
  return static_cast<const Type*>(geom_object_ptr)->leftmost_point(); \
} \
\
virtual Point rightmost_point() const \
{ \
  return static_cast<const Type*>(geom_object_ptr)->rightmost_point(); \
} \
\
virtual Eepic_Geom_Object * clone() const \
{ \
  return new Eepic_##Type(static_cast<const Type&>(*geom_object_ptr)); \
} \
\
virtual void draw(Eepic_Plane *, ofstream &) const;


/** 
  @def Macro to define common methods Eepic_Type more
  the copy constructor. 
*/
# define DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type) \
DEFINE_EEPIC_COPY_CTOR(Type) \
DEFINE_EEPIC_COMMON_METHODS(Type)


/** 
  @def Macro to define common methods Eepic_Type but the builder
  copy only declared ==> should be written after. 
*/
# define DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type) \
DEFINE_EEPIC_COMMON_METHODS(Type) \
Eepic_##Type(const Type &);


/** 
  @def Macro to define method of inserting an object
  A Eepic_Type Eepic_Plane. 
*/
# define DEFINE_PUT_IN_PLANE(Type) \
extern void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)


# define IMPL_PUT_IN_PLANE(Type) \
void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)	\
{ \
  Eepic_##Type eepic_obj(geom_obj); \
\
  plane.put(eepic_obj); \
} 


/** 
  @def Eepic_Type macro to define a class,
  only must implement the virtual function Eepic_Plane::put(const Eepic_Type &). 
*/
# define DEFINE_EEPIC_CLASS(Type) \
class Eepic_##Type : public Eepic_Geom_Object \
{ \
  DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type) \
};\
\
DEFINE_PUT_IN_PLANE(Type) 

/** 
  @def Eepic_Type macro to define a class with other attributes,
  this implies that you must define the copy constructor separately
  perform initialization of additional attributes. 
*/
# define DEFINE_EEPIC_CLASS_AND_MEMBERS(Type,...)	\
class Eepic_##Type : public Eepic_Geom_Object \
{ \
  __VA_ARGS__ ; 			       \
  DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type) \
};\
\
DEFINE_PUT_IN_PLANE(Type) 


    //    /************************************************************
    //    From here the only issue is to define new classes
    //    eepic according to the object and functionality desired drawn
    //    implement.
    //    *************************************************************/

DEFINE_EEPIC_CLASS(Point);


DEFINE_EEPIC_CLASS(Segment);


    //    /***********************************************
    //    We require define various ways to draw segments.    
    //    ***********************************************/

/** 
  @def Macro to define a new class derived from Segment,
  and their sole purpose is to have a new object name
  geometric order to define different behaviors drawn. 
*/
# define DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)  \
struct Name : public Segment \
{ \
  Name() { } \
\
  Name(const Segment & s) : Segment(s) \
  { \
\
  } \
  Name(const Point & __src, const Point & __tgt) \
    : Segment(__src, __tgt) \
  { \
\
  } \
\
  Name(const Point &       __src, \
       const Geom_Number & m, \
       const Geom_Number & d) \
    : Segment(__src, m, d) \
  { \
\
  } \
};

/** 
  @def Macro to define a class derived from segment name Name -
  The purpose is to distiguirlo appointment of Segment. 
*/ 
# define DEFINE_EEPIC_SEGMENT_CLASS(Name) \
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name) \
  DEFINE_EEPIC_CLASS(Name);

/** 
  @def Like "DEFINE_EEPIC_SEGMENT_CLASS (Name)"
  but allows incorporates other members in the macro parameter Members. 
*/
# define DEFINE_EEPIC_SEGMENT_CLASS_AND_MEMBERS(Name,...)	\
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name) \
  DEFINE_EEPIC_CLASS_AND_MEMBERS(Name, __VA_ARGS__);



    /* Family sorts of ways to draw a segment. */

DEFINE_EEPIC_SEGMENT_CLASS(Arrow)

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Arrow);
    

    /* Triangle. */
DEFINE_EEPIC_CLASS(Triangle);


    /* Ellipse. */
DEFINE_EEPIC_CLASS(Ellipse);


/** 
  @def Macro to define a new class derived from ellipse to define 
  different behaviors drawn.
*/
# define DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Name)  \
struct Name : public Ellipse \
{ \
  Name() { } \
\
  Name(const Ellipse & p) : Ellipse(p) \
  { \
\
  } \
\
  Name(const Point &       center, \
       const Geom_Number & hr, \
       const Geom_Number & vr) \
    : Ellipse(center, hr, vr) \
  { \
\
  } \
};


DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Shade_Ellipse);
DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Thick_Ellipse);
DEFINE_EEPIC_CLASS(Shade_Ellipse);
DEFINE_EEPIC_CLASS(Thick_Ellipse);


    /* Texto. */
# define DEFINE_CLASS_DERIVATED_FROM_TEXT(Name)  \
struct Name : public Text \
{ \
  Name() { } \
\
  Name(const Text & t) : Text(t) \
  { \
\
  } \
\
  Name(const Point & p, const std::string & str) \
    : Text(p, str) \
  { \
\
  } \
};


DEFINE_EEPIC_CLASS(Text);

DEFINE_CLASS_DERIVATED_FROM_TEXT(Left_Text);
DEFINE_CLASS_DERIVATED_FROM_TEXT(Right_Text);
DEFINE_CLASS_DERIVATED_FROM_TEXT(Center_Text);

DEFINE_EEPIC_CLASS(Left_Text);
DEFINE_EEPIC_CLASS(Right_Text);
DEFINE_EEPIC_CLASS(Center_Text);


    //    /****************************************************
    //    Now we request several ways to draw general polygons.
    //    ****************************************************/

    /* The simplest way to draw a polygon. */
DEFINE_EEPIC_CLASS(Polygon);



/** 
  @def Macro for a new class derived from polygon
  to define different behaviors drawn. 
*/
# define DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name)  \
struct Name : public Polygon \
{ \
  Name() { } \
\
  Name(const Polygon & p) : Polygon(p) \
  { \
\
  } \
};


# define DEFINE_EEPIC_POLYGON(Name) \
DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name); \
DEFINE_EEPIC_CLASS(Name);


    /* Family of polygons. */

/** 
  @brief With circles point vertices. 
*/
DEFINE_EEPIC_POLYGON(Polygon_With_Points); 

/** 
  @brief With circles point vertices. 
*/
DEFINE_EEPIC_POLYGON(Polygon_With_Arrows); 

/** 
  @brief dotted or sides. 
*/
DEFINE_EEPIC_POLYGON(Dotted_Polygon);

/** 
  @brief Sides starting line. 
*/
DEFINE_EEPIC_POLYGON(Dash_Polygon);

/** 
  @brief Dotted or circles on the sides and vertices. 
*/
DEFINE_EEPIC_POLYGON(Dotted_Polygon_With_Points); 

/** 
  @brief Sides broken lines with circles at the vertices. 
*/
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Points);

/** 
  @brief Sides tipped d broken lines eflecha last segment. 
*/
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Arrow);

/** 
  @brief The interior of the polygon is sobreado (gray). 
*/
DEFINE_EEPIC_POLYGON(Shade_Polygon); 

/** 
  @brief The interior of the polygon is sobreado (gray) with circles at the vertices. 
*/
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Points);

/** 
  @brief The interior of the polygon is sobreado (gray) and sides are arrows. 
*/
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Arrows);

/** 
  @brief Sobreado Interior (gray) and coordinates of the vertices. \
*/
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Text_Points);

/** 
  @brief Sobreado Interior (gray) and numbers of the vertices. 
*/
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Vertex_Numbers);

/** 
  @brief Spline constructed according to the points of the polygon. 
*/
DEFINE_EEPIC_POLYGON(Spline);

/** 
  @brief Spline curve constructed starting line as the points of the polygon. 
*/
DEFINE_EEPIC_POLYGON(Dash_Spline);

/** 
  @brief Spline constructed according to the points of the polygon with arrow. 
*/
DEFINE_EEPIC_POLYGON(Spline_Arrow);

/** 
  @brief Spline curve constructed according to the starting line
  polygon points with arrow. 
*/
DEFINE_EEPIC_POLYGON(Dash_Spline_Arrow);

/** 
  @brief With circles point vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Points); 

/** 
  @brief With circles point vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Arrows); 

/** 
  @brief dotted or sides. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon);

/** 
  @brief Sides starting line. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon);

/** 
  @brief Dotted or circles on the sides and vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon_With_Points); 

/** 
  @brief Sides broken lines with circles at the vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Points);

/** 
  @brief Sides broken lines with arrowheads. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Arrow);

/** 
  @brief The interior of the polygon is sobreado (gray). 
*/
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon); 

/** 
  @brief inside the polygon is sobreado (gray) with circles at the vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Points);

/** 
  @brief The interior of the polygon is sobreado (gray) and sides are arrows. 
*/
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Arrows);

/** 
  @brief Inteiror shading (gray) and coordinates of the vertices. 
*/
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Text_Points);

/** 
  @brief Interior shading (gray) and number of vertices 
*/
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Vertex_Numbers);

/** 
  @brief Spline constructed according to the points of the polygon. 
*/
DEFINE_EEPIC_POLYGON(Thick_Spline);

/** 
  @brief Spline curve starting line built according to the points of the polygon. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline);

/** 
  @brief Spline constructed according to the points of the polygon with arrow. 
*/
DEFINE_EEPIC_POLYGON(Thick_Spline_Arrow);
    
/** 
  @brief Spline curve constructed starting line as the points of the polygon with arrow. 
*/
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline_Arrow);
    
    //    /*********************************************************************  
    //    Now we do pretty much the same for regular polygons.
    //    NOTE: in fact, the definition of builder Polygon (Regular_Polygon)
    //    And conversion operator to Polygon Regular_Polygon you can draw
    //    Regular polygons using the classes to draw irregular polygons.
    //    The matter is therefore only a little efficiency: you can draw a
    //    regular polygon in constant time faster than an irregular one.
    //    In any case, the time complexity is the same O (n) for the two classes.
    //    **********************************************************************/ 

/** 
  @def Macro to define a new class derived from regular polygon
  in order to define different behaviors drawn and
  We must define a new one because the builder is different 
*/
# define DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name)  \
struct Name : public Regular_Polygon \
{ \
  Name() { } \
\
  Name(const Regular_Polygon & p) : Regular_Polygon(p) \
  { \
\
  } \
\
  Name(const Point &  c,                        \
       const double & side_sz,                  \
       const size_t & n,                        \
       const double & ang = 0)                  \
    : Regular_Polygon(c, side_sz, n, ang)	\
  { \
\
  } \
};


# define DEFINE_EEPIC_REGULAR_POLYGON(Name) \
DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name); \
DEFINE_EEPIC_CLASS(Name);

DEFINE_EEPIC_CLASS(Regular_Polygon);

    /* Definitions for regular polygons. */

/* 
  @brief 
*/
DEFINE_EEPIC_REGULAR_POLYGON(Regular_Polygon_With_Points);

/** 
  @brief dotted regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon);

/** 
  @brief lines games regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon);

/** 
  @brief Dotted at the vertices regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon_With_Points);

/** 
  @brief Lines games at the vertices regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon_With_Points);

/** 
  @brief Shading regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon);

/** 
  @brief Shading and circles at the vertices regular polygon
*/
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Points);

/** 
  @brief Shading and arrows
*/
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Arrows);

/** 
  @brief Shading and coordinates 
*/
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Text_Points);

/** 
  @brief Shading and number of vertices 
*/
DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Vertex_Numbers);

  

# endif // EEPICGEOM_H
