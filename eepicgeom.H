
/*
                          Aleph_w

  Data structures & Algorithms
  version 2.0.0b
  https://github.com/lrleon/Aleph-w

  This file is part of Aleph-w library

  Copyright (c) 2002-2026 Leandro Rabindranath Leon

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


/** @file eepicgeom.H
 *  @brief EEPIC/LaTeX geometric drawing utilities.
 *
 *  This file provides classes for generating EEPIC (Extended EPIC) code
 *  for LaTeX documents. EEPIC is a macro package that extends the standard
 *  LaTeX picture environment with additional drawing primitives.
 *
 *  The main components are:
 *  - @ref Eepic_Plane A 2D canvas that holds geometric objects and generates
 *    the LaTeX picture environment
 *  - @ref Eepic_Geom_Object Abstract base class for drawable geometric objects
 *  - Various Eepic_* wrapper classes for Points, Segments, Polygons, etc.
 *
 *  @section eepicgeom_usage Usage Example
 *  @code
 *  Eepic_Plane plane(500, 500);  // 500x500 point canvas
 *  Point p1(0, 0), p2(100, 100);
 *  Segment seg(p1, p2);
 *  put_in_plane(plane, seg);
 *
 *  std::ofstream file("output.tex");
 *  plane.draw(file);
 *  @endcode
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */

# ifndef EEPICGEOM_H
# define EEPICGEOM_H


# include <fstream>
# include <tpl_dynDlist.H>
# include <tpl_sort_utils.H>
# include <point.H>
# include <polygon.H>
# include <ah-errors.H>

/** @brief Global flag to enable tiny font size for keys/labels.
 *
 *  When set to true, text labels will be rendered using LaTeX's \\tiny
 *  font size modifier. Useful for dense diagrams with many labels.
 */
extern bool tiny_keys;

// TODO: dotted polygon curves

class Eepic_Plane;


/** @brief Abstract base class for EEPIC-drawable geometric objects.
 *
 *  This class serves as the polymorphic base for all geometric objects that
 *  can be rendered in EEPIC format. Each derived class wraps a concrete
 *  geometric object (Point, Segment, Polygon, etc.) and provides:
 *  - A draw() method that generates the EEPIC LaTeX code
 *  - Methods to query extreme points for bounding box calculations
 *  - A clone() method for polymorphic copying
 *
 *  @section memory_management Memory Management
 *  The wrapped geometric object can be owned by this class (to_delete=true)
 *  or externally managed (to_delete=false). When owned, the destructor
 *  will delete the wrapped object.
 *
 *  @section subclassing Creating New Drawable Types
 *  Use the DEFINE_EEPIC_CLASS macro to create new drawable types:
 *  @code
 *  DEFINE_EEPIC_CLASS(MyGeomType);
 *  @endcode
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */
class Eepic_Geom_Object
{
  Eepic_Geom_Object();

  Eepic_Geom_Object(const Eepic_Geom_Object &);

protected:
  /** @brief Pointer to the wrapped geometric object.
   *
   *  This pointer is const-qualified to prevent accidental modification.
   *  Memory management depends on the to_delete flag.
   */
  const Geom_Object *const geom_object_ptr;

  /** @brief Flag indicating ownership of geom_object_ptr.
   *
   *  When true, the destructor will delete geom_object_ptr.
   *  When false, the caller is responsible for memory management.
   */
  bool to_delete;

public:
  /** @brief Construct an EEPIC wrapper for a geometric object.
   *
   *  @param ptr Pointer to the geometric object to wrap
   *  @param __to_delete If true, this object takes ownership and will
   *                     delete ptr in the destructor
   */
  Eepic_Geom_Object(Geom_Object *ptr, const bool & __to_delete = false)
    : geom_object_ptr(ptr), to_delete(__to_delete)
  {
    // empty
  }

  /** @brief Virtual destructor.
   *
   *  Deletes geom_object_ptr only if to_delete is true.
   */
  virtual ~Eepic_Geom_Object()
  {
    if (to_delete)
      delete geom_object_ptr;
  }

  /** @brief Generate EEPIC code for this object.
   *
   *  Derived classes must override this method to produce the appropriate
   *  EEPIC LaTeX commands for their geometric type.
   *
   *  @param plane The Eepic_Plane providing coordinate transformation
   *  @param output The output stream to write EEPIC code to
   *
   *  @throws std::domain_error if geom_object_ptr is null
   *  @throws std::logic_error if called on the base class (not overridden)
   */
  virtual void draw(Eepic_Plane *plane, std::ostream & output) const
  {
    ah_domain_error_if(geom_object_ptr == nullptr)
      << "Eepic_Geom_Object is null";

    ah_logic_error() << "Call to draw() method from abstract class";
  }

  /** @brief Create a polymorphic copy of this object.
   *
   *  Derived classes must implement this to return a new instance of
   *  their specific type.
   *
   *  @return A new heap-allocated copy of this object
   */
  virtual Eepic_Geom_Object * clone() const = 0;

  /** @brief Get the highest point (maximum y-coordinate) of this object.
   *  @return The point with the maximum y-coordinate
   */
  virtual Point highest_point() const = 0;

  /** @brief Get the lowest point (minimum y-coordinate) of this object.
   *  @return The point with the minimum y-coordinate
   */
  virtual Point lowest_point() const = 0;

  /** @brief Get the leftmost point (minimum x-coordinate) of this object.
   *  @return The point with the minimum x-coordinate
   */
  virtual Point leftmost_point() const = 0;

  /** @brief Get the rightmost point (maximum x-coordinate) of this object.
   *  @return The point with the maximum x-coordinate
   */
  virtual Point rightmost_point() const = 0;
};


/** @brief 2D canvas for generating EEPIC/LaTeX picture environments.
 *
 *  Eepic_Plane serves as a container for geometric objects and handles the
 *  transformation from geometric coordinates to EEPIC picture coordinates.
 *  It generates a complete LaTeX picture environment that can be included
 *  in any LaTeX document with the eepic package.
 *
 *  @section coordinate_system Coordinate System
 *  The plane maintains two coordinate systems:
 *  - **Geometric coordinates**: The original coordinates of the objects
 *  - **EEPIC coordinates**: Normalized coordinates for the picture environment
 *
 *  The transformation between these systems handles:
 *  - Scaling to fit the specified width and height
 *  - Optional aspect ratio preservation (squarize mode)
 *  - Offset adjustments for margins
 *
 *  @section drawing_methods Drawing Methods
 *  The class provides primitive drawing methods for:
 *  - Points (draw_point)
 *  - Segments in various styles (draw_segment, draw_dotted_segment, etc.)
 *  - Arrows (draw_arrow, draw_arrow_extreme)
 *  - Ellipses (draw_ellipse)
 *  - Polygons (draw_polygon, draw_closed_polygon, etc.)
 *  - Splines (draw_spline, draw_dash_spline)
 *  - Text (draw_text, draw_point_text)
 *
 *  @section example Example Usage
 *  @code
 *  // Create a 500x500 point canvas
 *  Eepic_Plane plane(500, 500);
 *
 *  // Add geometric objects
 *  Point p1(0, 0), p2(100, 100);
 *  Segment seg(p1, p2);
 *  put_in_plane(plane, seg);
 *
 *  // Generate EEPIC output
 *  std::ofstream file("diagram.tex");
 *  plane.draw(file);
 *  @endcode
 *
 *  @ingroup Geometry
 *  @author Leandro Rabindranath León
 */
class Eepic_Plane
{
  // Picture environment frame defined at construction time
  double wide; ///< Width in resolution points
  double height; ///< Height in resolution points

  // Final frame dimensions based on user's scaling choice
  double real_wide = 0; ///< Actual width after scaling
  double real_height = 0; ///< Actual height after scaling

  double xoffset; ///< Additional horizontal margin
  double yoffset; ///< Additional vertical margin

  Geom_Number geom_wide; ///< Width from leftmost to rightmost point
  Geom_Number geom_height; ///< Height from lowest to highest point

  DynDlist<Eepic_Geom_Object *> list; ///< List of geometric objects

  Geom_Number geom_x_min; ///< Minimum x value in geometric coordinates
  Geom_Number geom_y_min; ///< Minimum y value in geometric coordinates

  // NOTE: These must be initialized to zero as they are used in
  // self-referential calculations
  double x_min; ///< Minimum x value in plane coordinates
  double y_min; ///< Minimum y value in plane coordinates

  // Extreme points for computing object dimensions
  Point __leftmost; ///< Point with minimum x-coordinate
  Point __rightmost; ///< Point with maximum x-coordinate
  Point __highest; ///< Point with maximum y-coordinate
  Point __lowest; ///< Point with minimum y-coordinate

  bool with_cartesian_axis; ///< Flag to draw Cartesian axes

  /// Default radius for drawing points (also used for text height)
  static const double default_r;

  double r; ///< Actual radius in real points for drawing a point

  // Parameters that control the LaTeX picture environment
  double resolution; ///< Resolution in millimeters (default: 0.05mm)

  std::string fill_type; ///< Fill type for shapes (e.g., "black")
  std::string shade_type; ///< Shading type for segments

  static const double normal_thickness; ///< Default line thickness
  static const double default_shade_thickness; ///< Default shading thickness

  double shade_thickness; ///< Current shading thickness

  static const double dotgap; ///< Gap between dots in dotted lines

  /** Rescales a coordinate value `p` from a geometric scale `geom_max` into
      the plane scale `max`.

      Formula: result = p * max / geom_max
  */
  double geom_number_to_plane(const Geom_Number & p,
                              const Geom_Number & geom_max,
                              const double & max) const
  {
    const Geom_Number ratio = p / geom_max;

    double result = geom_number_to_double(ratio);

    result = max * result;

    return result;
  }

  /** Points must be normalized to the plane size (in picture coordinates).
      These routines normalize a coordinate with respect to the origin
      (x or y axis).
  */

  /// Normalize to x-axis origin.
  double xpic(const double & x) const
  {
    return x - x_min;
  }

  /// Normalize to y-axis origin.
  double ypic(const double & y) const
  {
    return y - y_min;
  }

public:
  /// Convert a geometric x proportion into plane units using `wide`.
  double h_geom_number_to_eepic(const Geom_Number & x) const
  {
    return geom_number_to_plane(x, geom_wide, wide);
  }

  /// Convert a geometric y proportion into plane units using `height`.
  double v_geom_number_to_eepic(const Geom_Number & y) const
  {
    return geom_number_to_plane(y, geom_height, height);
  }

  /// Convert a geometric x coordinate into plane x coordinate.
  double x_geom_number_to_eepic(const Geom_Number & x) const
  {
    return xpic(geom_number_to_plane(x, geom_wide, wide));
  }

  /// Convert a geometric y coordinate into plane y coordinate.
  double y_geom_number_to_eepic(const Geom_Number & y) const
  {
    const double result = geom_number_to_plane(y, geom_height, height);

    return ypic(result);
  }

  /// Linear scan over all objects to compute extreme points.
  void compute_extreme_points()
  {
    // Scan all objects searching extreme coordinates (left, right, top, bottom).
    DynDlist<Eepic_Geom_Object *>::Iterator it(list);

    Eepic_Geom_Object *curr = it.get_curr();

    __lowest = curr->lowest_point();
    __highest = curr->highest_point();
    __leftmost = curr->leftmost_point();
    __rightmost = curr->rightmost_point();

    for (it.next(); it.has_curr(); it.next_ne())
      {
        curr = it.get_curr();

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

  /// Scale the EEPIC plane in real points (zoom in/out).
  void zoom(const double & factor)
  {
    ah_domain_error_if(factor <= 0) << "zoom factor must be greater than zero";

    height *= factor;
    wide *= factor;
  }

private:
  /// Compute extreme coordinates and derive width/height of all objects.
  /// If `squarize` is true, rescaling preserves aspect ratio (no distortion).
  void compute_geom_plane(const bool & squarize)
  {
    compute_extreme_points();

    // Compute minimal bounding box from contained figures.
    geom_height = __highest.get_y() - __lowest.get_y();
    geom_wide = __rightmost.get_x() - __leftmost.get_x();

    // Compute minimal x/y.
    geom_x_min = geom_number_to_double(__leftmost.get_x());
    geom_y_min = geom_number_to_double(__lowest.get_y());

    // Shift so that min point maps to plane origin (0,0).
    x_min = h_geom_number_to_eepic(geom_x_min);
    y_min = v_geom_number_to_eepic(geom_y_min);

    if (squarize) // Preserve aspect ratio (avoid distorting figures).
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

  void draw_cartesian_axis(std::ostream & output);

public:
  const Point &leftmost() const { return __leftmost; }

  const Point &rightmost() const { return __rightmost; }

  const Point &highest() const { return __highest; }

  const Point &lowest() const { return __lowest; }

  /// Given point p, returns "(x,y)" in normalized plane coordinates.
  std::string point_string(const Point & p)
  {
    const double x = x_geom_number_to_eepic(p.get_x());
    const double y = y_geom_number_to_eepic(p.get_y());
    return "(" + std::to_string(x) + "," + std::to_string(y) + ")";
  }

  /**
      Emits a complete LaTeX `picture` environment containing the geometric objects.

      If `squarize == false`, objects are scaled independently in x/y to fit exactly
      in the plane defined at construction time.

      If `squarize == true`, aspect ratio is preserved (no distortion) but the
      available space may be used less efficiently.

  */
  void draw(std::ostream & output, const bool & squarize = true)
  {
    compute_geom_plane(squarize);

    // Set floating point output format.
    output << std::fixed << std::setprecision(6);

    // Emit header and open picture environment.
    output << "\\setlength{\\unitlength}{" << resolution << "mm}" << std::endl
        << std::endl
        << "\\filltype{" << fill_type << "}" << std::endl
        << std::endl
        << "% leftmost point is " << __leftmost.to_string() << " --> "
        << point_string(__leftmost) << std::endl
        << "% rightmost point is " << __rightmost.to_string() << " --> "
        << point_string(__rightmost) << std::endl
        << "% highest point is " << __highest.to_string() << " --> "
        << point_string(__highest) << std::endl
        << "% lowest point is " << __lowest.to_string() << " --> "
        << point_string(__lowest) << std::endl
        << "% " << list.size() << " geometric objects were put in the plane"
        << std::endl
        << "\\begin{picture}(" << real_wide << "," << real_height << ")"
        << "(" << xoffset << "," << yoffset << ")" << std::endl
        << std::endl;

    draw_cartesian_axis(output); // Only if enabled.

    // Draw each object.
    for (DynDlist<Eepic_Geom_Object *>::Iterator it(list);
         it.has_curr(); it.next_ne())
      {
        Eepic_Geom_Object *curr = it.get_curr();

        // Reset to default thickness each time.
        output << "\\thinlines" << std::endl
            << std::endl;

        curr->draw(this, output);
      }

    // Close picture environment.
    output << "\\end{picture}" << std::endl
        << std::endl;
  }

  /** Plane accessors. */
  /// Picture width (in EEPIC units).
  const double &get_wide() const { return wide; }

  /// Picture height (in EEPIC units).
  const double &get_height() const { return height; }

  /// Y offset of the LaTeX picture environment.
  const double &get_yoffset() const { return yoffset; }

  /// X offset of the LaTeX picture environment.
  const double &get_xoffset() const { return xoffset; }

  /// Radius used to draw points (also used as text baseline offset).
  const double &get_r() const { return r; }

  /** Construct a plane with a fixed picture frame.
   *
   *  @param __wide Picture width.
   *  @param __height Picture height.
   *  @param __xoffset X offset for the LaTeX picture environment.
   *  @param __yoffset Y offset for the LaTeX picture environment.
   */
  Eepic_Plane(const double & __wide,
              const double & __height,
              const double & __xoffset = 0.0,
              const double & __yoffset = 0.0)
    : wide(__wide), height(__height),
      xoffset(__xoffset), yoffset(__yoffset),
      x_min(0), y_min(0), with_cartesian_axis(false),
      r(default_r), resolution(0.05), shade_thickness(default_shade_thickness)
  {
    // empty
  }

  /// Destructor: deletes all stored Eepic_Geom_Object clones.
  ~Eepic_Plane()
  {
    while (not list.is_empty())
      delete list.remove_first_ne();
  }

  /// Enable drawing Cartesian axes when calling draw().
  void put_cartesian_axis()
  {
    with_cartesian_axis = true;
  }

  /** Design note: why Eepic_Plane stores Eepic_Geom_Object wrappers.
   *
   *  Eepic_Plane stores objects of type Eepic_Geom_Object. However, the goal is
   *  to let users work directly with objects derived from Geom_Object.
   *
   *  One approach would be to make Geom_Object highly polymorphic (virtual clone,
   *  virtual draw, virtual extreme points). This header avoids that for two reasons:
   *
   *  1. Geom_Object would become "coarse-grained": even small types like Point
   *     would pay the cost of a larger vtable (at least ~7 virtual methods).
   *  2. The required polymorphism depends on the rendering backend: drawing in
   *     EEPIC is not the same as drawing on a GUI canvas. Baking the rendering
   *     API into Geom_Object would not be general.
   *
   *  For that reason, this header defines an external helper:
   *  put_in_plane(plane, geom_obj), which:
   *  - wraps/clones geom_obj into its corresponding Eepic_* wrapper
   *  - inserts the wrapper into the plane
   *
   *  For implementation simplicity, the insertion primitive is public, but its
   *  intended usage is indirect (via put_in_plane()).
   */
  /** Insert an already-wrapped EEPIC drawable into the plane.
   *
   *  This method clones the input object (polymorphic copy via clone())
   *  and stores it in the plane. Typical usage is through the helper
   *  `put_in_plane(plane, geom_obj)` which builds the right wrapper type.
   *
   *  @param __eepic_geom_obj An Eepic_Geom_Object wrapper to clone and store.
   */
  void put(const Eepic_Geom_Object & __eepic_geom_obj)
  {
    Eepic_Geom_Object *eepic_geom_obj = __eepic_geom_obj.clone();

    list.append(eepic_geom_obj);
  }

  /// Set the output resolution in millimeters (unitlength = res mm).
  void set_resolution(const double & res)
  {
    resolution = res;
  }

  /// Return the current output resolution in millimeters.
  const double &get_resolution() const { return resolution; }

  /// Set the fill type used by LaTeX eepic (e.g., "black").
  void set_fill_type(const std::string & ftype)
  {
    fill_type = ftype;
  }

  /// Set the thickness used for shaded primitives.
  void set_shade_thickness(const double & __thickness)
  {
    shade_thickness = __thickness;
  }

  /// Return the current shading thickness.
  double get_shade_thickness() const { return shade_thickness; }

  /// Return the dot gap used for dotted/dashed lines.
  double get_dotgap() const { return dotgap; }

  /** EEPIC "primitive drawing" methods.
   *
   *  The draw() method of each Eepic_* wrapper may call one (or more) of these
   *  primitives. They are also callable by users (at their own risk).
   *
   *  This family of primitives assumes that the extreme points required to map
   *  geometric coordinates into EEPIC coordinates were already computed by
   *  compute_geom_plane().
   */

  /// Emit EEPIC code to draw point `p` into `output`.
  void draw_point(const Point & p, std::ostream & output)
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

  // Compute source/target EEPIC coordinates for a segment (macro for brevity).
# define COMPUTE_SEGMENT_EEPIC_COORDENATES()                            \
  const double src_x = x_geom_number_to_eepic(sg.get_src_point().get_x()); \
                                                                        \
  const double src_y = y_geom_number_to_eepic(sg.get_src_point().get_y()); \
                                                                        \
  const double tgt_x = x_geom_number_to_eepic(sg.get_tgt_point().get_x()); \
                                                                        \
  const double tgt_y = y_geom_number_to_eepic(sg.get_tgt_point().get_y());

  // Draw the segment `sg` to `output` (macro uses locals above).
# define DRAW_SEGMENT(sg, output)                                       \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
  output << "    %% Segment from " << sg.get_src_point().to_string()    \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\path(" << src_x << "," << src_y                                 \
  << ")(" << tgt_x << "," << tgt_y << ")" << std::endl                  \
  << std::endl;
  /// Emit EEPIC code to draw segment `sg`.
  void draw_segment(const Segment & sg, std::ostream & output)
  {
    DRAW_SEGMENT(sg, output);
  }

  static const double arrow_width_in_mm;

  static const double arrow_lenght_in_mm;

  // Compute and draw the two small segments that form an arrow head.
  // Uses src/tgt coordinates from COMPUTE_SEGMENT_EEPIC_COORDENATES().
  // Geometry is controlled by arrow_width_in_mm and arrow_lenght_in_mm.
# define PUT_ARROW()                                                    \
  const double arrow_width  = arrow_width_in_mm/resolution;             \
  const double arrow_lenght = arrow_lenght_in_mm/resolution;            \
                                                                        \
  const double l = sqrt(arrow_width*arrow_width + arrow_lenght*arrow_lenght); \
  const double tetha = atan2(arrow_width, arrow_lenght);                \
  const double phi = atan( fabs( (tgt_y - src_y)/(tgt_x - src_x) ) );   \
                                                                        \
  double dx1 = l*cos(phi - tetha);                                      \
  double dy1 = l*sin(phi - tetha);                                      \
                                                                        \
  double dx2 = l*sin(M_PI_2 - (phi + tetha));                           \
  double dy2 = l*cos(M_PI_2 - (phi + tetha));                           \
                                                                        \
  if (tgt_x > src_x)                                                    \
    {                                                                   \
      dx1 = -dx1;                                                       \
      dx2 = -dx2;                                                       \
    }                                                                   \
                                                                        \
  if (tgt_y > src_y)                                                    \
    {                                                                   \
      dy1 = -dy1;                                                       \
      dy2 = -dy2;                                                       \
    }                                                                   \
                                                                        \
  output << std::endl                                                        \
  << "    % Drawing of arrow's edges at (" << tgt_x                     \
  << "," << tgt_y << ")" << std::endl                                        \
  << "\\path(" << tgt_x << "," << tgt_y << ")("                         \
  << tgt_x + dx1 << "," << tgt_y + dy1 << ")" << std::endl                   \
  << "\\path(" << tgt_x << "," << tgt_y << ")("                         \
  << tgt_x + dx2 << "," << tgt_y + dy2 << ")" << std::endl << std::endl;

  // Draw segment plus arrow head.
# define DRAW_ARROW(sg, output)                 \
  DRAW_SEGMENT(sg, output);                     \
  PUT_ARROW();

  /// Emit EEPIC code to draw segment `sg` with an arrow head.
  void draw_arrow(const Segment & sg, std::ostream & output)
  {
    DRAW_ARROW(sg, output);
  }

  // Macro to draw a dotted segment.
# define DRAW_DOTTED_SEGMENT(sg, output)                                \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dotted Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dottedline{" << dotgap << "}(" << src_x                         \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;

  /// Emit EEPIC code to draw dotted segment `sg`.
  void draw_dotted_segment(const Segment & sg, std::ostream & output)
  {
    DRAW_DOTTED_SEGMENT(sg, output);
  }

  // Macro to draw a dashed segment.
# define DRAW_DASH_SEGMENT(sg, output)                                  \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dash Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dashline{" << dotgap << "}(" << src_x                           \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;

  /// Emit EEPIC code to draw dashed segment `sg`.
  void draw_dash_segment(const Segment & sg, std::ostream & output)
  {
    DRAW_DASH_SEGMENT(sg, output);
  }

  // Dotted segment with arrow head (macro).
# define DRAW_ARROW_DOTTED_SEGMENT(sg, output)                          \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dotted arrow Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dottedline{" << dotgap << "}(" << src_x                         \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;                                                         \
  PUT_ARROW();

  /// Emit EEPIC code to draw dotted segment `sg` with arrow head.
  void draw_arrow_dotted_segment(const Segment & sg, std::ostream & output)
  {
    DRAW_ARROW_DOTTED_SEGMENT(sg, output);
  }

  // Dashed segment with arrow head (macro).
# define DRAW_ARROW_DASH_SEGMENT(sg, output)                            \
  COMPUTE_SEGMENT_EEPIC_COORDENATES();                                  \
                                                                        \
  output << "    %% Dash arrow Segment from " << sg.get_src_point().to_string() \
  << " to " << sg.get_tgt_point().to_string() << std::endl              \
  << "    %% mapped in this plane to (" << src_x << "," << src_y        \
  << ") to (" << tgt_x << "," << tgt_y << ")" << std::endl              \
  << "\\dashline{" << dotgap << "}(" << src_x                           \
  << "," << src_y << ")(" << tgt_x << "," << tgt_y << ")" << std::endl  \
  << std::endl;                                                         \
  PUT_ARROW();

  /// Emit EEPIC code to draw dashed segment `sg` with arrow head.
  void draw_arrow_dash_segment(const Segment & sg, std::ostream & output)
  {
    DRAW_ARROW_DASH_SEGMENT(sg, output);
  }

  /// Emit EEPIC code to draw ellipse `e`.
  void draw_ellipse(const Ellipse & e, std::ostream & output)
  {
    const Point & center = e.get_center();

    const double x = x_geom_number_to_eepic(center.get_x());

    const double y = y_geom_number_to_eepic(center.get_y());

    const double hd = h_geom_number_to_eepic(2 * e.get_hradius());

    const double vd = v_geom_number_to_eepic(2 * e.get_vradius());

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

  // Member-function pointer type used to draw a segment.
  // Lets us implement polygon drawing generically by viewing polygon sides as segments.
  typedef void (Eepic_Plane::*Draw_Segment_Fct)(const Segment & poly,
                                                std::ostream & output);


  // Draw a polygon by iterating its sides (segments).
  // draw_sg_fct is the member-function pointer used to draw each side.
  // This is generic and is also used for regular polygons.
  template <class Poly>
  void draw_polygon(Poly & poly,
                    std::ostream & output,
                    Draw_Segment_Fct draw_sg_fct)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
      (this->*draw_sg_fct)(it.get_current_segment(), output);
  }

  // Draw polygon interior shading using a vertex path.
  // Faster than the segment-based version, but less flexible.
  template <class Poly>
  void draw_closed_polygon(Poly & poly, std::ostream & output)
  {
    output << "    % Drawing of polygon of " << poly.size()
        << " sides" << std::endl
        << "\\shade\\path";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << std::endl;
  }

  // Draw shaded polygon with dotted border.
  template <class Poly>
  void draw_dotted_closed_polygon(Poly & poly, std::ostream & output)
  {
    output << "    % Drawing of dotted line Polygon" << std::endl
        << "\\shade\\dottedline{" << dotgap << "}";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << std::endl;
  }

  // Draw only polygon vertices as points (does NOT draw sides).
  template <class Poly>
  void draw_dots_from_polygon(Poly & poly, std::ostream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      {
        Vertex & vertex = it.get_current_vertex();

        draw_point(vertex, output);
      }
  }

  // Draw only the arrow head of segment sg.
  void draw_arrow_extreme(const Segment & sg, std::ostream & output)
  { // ¡tal cual!
    COMPUTE_SEGMENT_EEPIC_COORDENATES();
    PUT_ARROW();
  }

  // Draw arrow heads for each polygon side (does NOT draw the sides).
  template <class Poly>
  void draw_arrows_in_polygon(Poly & poly, std::ostream & output)
  {
    for (typename Poly::Segment_Iterator it(poly); it.has_curr(); it.next_ne())
      {
        const Segment & sg = it.get_current_segment();

        draw_arrow_extreme(sg, output);
      }
  }

  template <class Poly>
  void draw_closed_polygon_with_arrows(Poly & poly, std::ostream & output)
  {
    draw_closed_polygon(poly, output);
    draw_arrows_in_polygon(poly, output);
  }

  // Draw a text label for point p (as "(x,y)"), slightly shifted in y.
  void draw_point_text(const Point & p, std::ostream & output)
  {
    const double x = x_geom_number_to_eepic(p.get_x());

    const double y = y_geom_number_to_eepic(p.get_y()) + r;

    const std::string str = p.to_string();

    output << "    %% Text Point " << str
        << "    %% mapped in this plane to (" << x << "," << y << ")"
        << std::endl
        << "\\put(" << x << "," << y << "){\\texttt{" << str
        << "}}" << std::endl
        << std::endl;
  }

  // Draw text labels for polygon vertices (does NOT draw sides).
  // TODO: place text inside/outside without crossing edges.
  template <class Poly>
  void draw_points_text_in_polygon(Poly & poly, std::ostream & output)
  {
    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      draw_point_text(it.get_current_vertex(), output);
  }

  // Draw vertex numbers (does NOT draw sides).
  // TODO: place numbers inside/outside without crossing edges.
  template <class Poly>
  void draw_vertex_numbers_in_polygon(Poly & poly, std::ostream & output)
  {
    int n = 0;

    for (typename Poly::Vertex_Iterator it(poly);
         it.has_curr(); it.next_ne(), ++n)
      {
        Vertex & vertex = it.get_current_vertex();

        const double x = x_geom_number_to_eepic(vertex.get_x());

        const double y = y_geom_number_to_eepic(vertex.get_y()) - r;

        output << "    %% Poligon Vertex Number " << n
            << "    %% mapped in this plane to (" << x << "," << y << ")"
            << std::endl
            << "\\put(" << x << "," << y << "){$" << n << "$}" << std::endl
            << std::endl;
      }
  }

  // Draw a spline through polygon vertices.
  template <class Poly>
  void draw_spline(Poly & poly, std::ostream & output)
  {
    output << "    % Drawing spline of " << poly.size()
        << " points" << std::endl
        << "\\spline";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << std::endl;
  }

  // Draw a piecewise-linear curve through polygon vertices.
  template <class Poly>
  void draw_dash_spline(Poly & poly, std::ostream & output)
  {
    output << "    % Drawing dash curve of " << poly.size()
        << " points" << std::endl
        << "\\closecurve";

    for (typename Poly::Vertex_Iterator it(poly); it.has_curr(); it.next_ne())
      output << point_string(it.get_current_vertex());

    if (poly.is_closed())
      output << point_string(poly.get_first_vertex()) << std::endl;
  }

  // Draw spline plus an arrow head at the last segment.
  template <class Poly>
  void draw_spline_arrow(Poly & poly, std::ostream & output)
  {
    draw_spline(poly, output); // draw curve first

    // Now draw the arrow head using the slope of the last segment.

    draw_arrow_extreme(poly.get_last_segment(), output);
  }

  // Draw dashed curve plus an arrow head at the last segment.
  template <class Poly>
  void draw_dash_spline_arrow(Poly & poly, std::ostream & output)
  {
    draw_dash_spline(poly, output); // draw curve first

    // Now draw the arrow head using the slope of the last segment.

    draw_arrow_extreme(poly.get_last_segment(), output);
  }

  void draw_text(const Text & t, std::ostream & output,
                 const double & hfactor, const double & vfactor)
  {
    const Point & p = t.get_point();

    const double str_size_in_points = t.len() * Text::font_width_in_points;

    const double x =
        x_geom_number_to_eepic(p.get_x()) - hfactor * str_size_in_points;

    const double y = (y_geom_number_to_eepic(p.get_y()) -
                      vfactor * Text::font_height_in_points);

    output << "%    std::string at " << p.to_string() << std::endl
        << "%    mapped to the plane at (" << x << "," << y << ")" << std::endl
        << "\\put(" << x << "," << y << "){\\texttt{"
        << (tiny_keys ? "\\tiny " : "") << t.get_str()
        << "}}" << std::endl
        << std::endl;
  }
};


/** Generator macros for Eepic_Geom_Object-derived wrapper types.
 *
 *  The macros below generate Eepic_* wrapper classes for underlying geometric
 *  primitives (Point, Segment, Polygon, ...). Each wrapper provides:
 *  - bounding box helpers (highest/lowest/leftmost/rightmost points)
 *  - clone() for polymorphic copying
 *  - draw() to emit EEPIC commands for that wrapper
 */

# define DEFINE_EEPIC_COPY_CTOR(Type)                                   \
  Eepic_##Type(const Type & o) : Eepic_Geom_Object(new Type(o), true) { }


/// Common methods always provided by an Eepic_Type wrapper class.
# define DEFINE_EEPIC_COMMON_METHODS(Type)                              \
  friend void put_in_plane(Eepic_Plane & plane, const Type & geom_obj); \
                                                                        \
  ~Eepic_##Type() { }                                                   \
                                                                        \
  Point highest_point() const override                                  \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->highest_point();  \
  }                                                                     \
                                                                        \
  Point lowest_point() const override                                   \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->lowest_point();   \
  }                                                                     \
                                                                        \
  Point leftmost_point() const override                                 \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->leftmost_point(); \
  }                                                                     \
                                                                        \
  Point rightmost_point() const override                                \
  {                                                                     \
    return static_cast<const Type*>(geom_object_ptr)->rightmost_point(); \
  }                                                                     \
                                                                        \
  Eepic_Geom_Object * clone() const override                            \
  {                                                                     \
    return new Eepic_##Type(static_cast<const Type&>(*geom_object_ptr)); \
  }                                                                     \
                                                                        \
  void draw(Eepic_Plane * plane, std::ostream & output) const override;


/// Common methods plus a copy-constructor that owns a deep copy of the wrapped object.
# define DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type)       \
  DEFINE_EEPIC_COPY_CTOR(Type)                          \
  DEFINE_EEPIC_COMMON_METHODS(Type)


/// Common methods, but the copy-constructor is only declared and must be implemented later.
# define DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type)    \
  DEFINE_EEPIC_COMMON_METHODS(Type)                     \
  Eepic_##Type(const Type &);


/// Declare put_in_plane(plane, geom_obj) for a given geometric type.
# define DEFINE_PUT_IN_PLANE(Type)                                      \
  extern void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)


# define IMPL_PUT_IN_PLANE(Type)                                \
  void put_in_plane(Eepic_Plane & plane, const Type & geom_obj)	\
  {                                                             \
    Eepic_##Type eepic_obj(geom_obj);                           \
                                                                \
    plane.put(eepic_obj);                                       \
  }

/** Fundamental macro to declare a class derived from Eepic_Geom_Object.
 *
 *  It generates the wrapper type Eepic_Type plus the corresponding put_in_plane()
 *  declaration for the underlying geometric type.
 */

// Typical definition of an Eepic_Type wrapper class.
// Only Eepic_Plane::put(const Eepic_Type&) needs to be implemented.
# define DEFINE_EEPIC_CLASS(Type)               \
  class Eepic_##Type : public Eepic_Geom_Object \
  {                                             \
    DEFINE_EEPIC_COMMON_WITH_COPY_CTOR(Type)    \
  };                                            \
                                                \
  DEFINE_PUT_IN_PLANE(Type)


// Typical definition of an Eepic_Type wrapper class with extra members.
// This implies defining the copy-constructor separately to initialize
// additional attributes.
# define DEFINE_EEPIC_CLASS_AND_MEMBERS(Type, ...)	\
  class Eepic_##Type : public Eepic_Geom_Object         \
  {                                                     \
    __VA_ARGS__ ;                                       \
    DEFINE_EEPIC_COMMON_WITHOUT_COPY_CTOR(Type)         \
  };                                                    \
                                                        \
  DEFINE_PUT_IN_PLANE(Type)


/** From here on, the rest of the file defines concrete Eepic_* wrapper classes
    and their specific drawing behavior.
*/

DEFINE_EEPIC_CLASS(Point);


DEFINE_EEPIC_CLASS(Segment);


/** We need several ways of drawing segments. This macro defines a new class
    derived from Segment. Its only purpose is to give a different geometric type
    name so we can attach different EEPIC drawing behaviors.
*/
# define DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)      \
  struct Name : public Segment                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Segment & s) : Segment(s)                \
    {                                                   \
                                                        \
    }                                                   \
    Name(const Point & __src, const Point & __tgt)      \
      : Segment(__src, __tgt)                           \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point &       __src,                     \
         const Geom_Number & m,                         \
         const Geom_Number & d)                         \
      : Segment(__src, m, d)                            \
    {                                                   \
                                                        \
    }                                                   \
  };

// Define a Segment-derived type named Name. Purpose: provide a distinct geometric
// type name (different from Segment) so it can have different EEPIC drawing behavior.
# define DEFINE_EEPIC_SEGMENT_CLASS(Name)       \
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)     \
  DEFINE_EEPIC_CLASS(Name);


// Same as DEFINE_EEPIC_SEGMENT_CLASS, but allows extra members to be injected
// via the macro parameter list.
# define DEFINE_EEPIC_SEGMENT_CLASS_AND_MEMBERS(Name, ...)	\
  DEFINE_CLASS_DERIVATED_FROM_SEGMENT(Name)                     \
  DEFINE_EEPIC_CLASS_AND_MEMBERS(Name, __VA_ARGS__);


/** Segment-drawing variants (types).
 *
 *  These are distinct Segment-derived types so they can have distinct EEPIC
 *  drawing behavior.
 */

DEFINE_EEPIC_SEGMENT_CLASS(Arrow)

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Segment);

DEFINE_EEPIC_SEGMENT_CLASS(Dotted_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Dash_Arrow);

DEFINE_EEPIC_SEGMENT_CLASS(Thick_Dash_Arrow);


/** Triangle wrapper. */
DEFINE_EEPIC_CLASS(Triangle);


/** Ellipse wrapper. */
DEFINE_EEPIC_CLASS(Ellipse);


// Define an Ellipse-derived type to allow different drawing behaviors.
# define DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Name)      \
  struct Name : public Ellipse                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Ellipse & p) : Ellipse(p)                \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point &       center,                    \
         const Geom_Number & hr,                        \
         const Geom_Number & vr)                        \
      : Ellipse(center, hr, vr)                         \
    {                                                   \
                                                        \
    }                                                   \
  };


DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Shade_Ellipse);

DEFINE_CLASS_DERIVATED_FROM_ELLIPSE(Thick_Ellipse);

DEFINE_EEPIC_CLASS(Shade_Ellipse);

DEFINE_EEPIC_CLASS(Thick_Ellipse);


/** Text wrappers. */

# define DEFINE_CLASS_DERIVATED_FROM_TEXT(Name)         \
  struct Name : public Text                             \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Text & t) : Text(t)                      \
    {                                                   \
                                                        \
    }                                                   \
                                                        \
    Name(const Point & p, const std::string & str)      \
      : Text(p, str)                                    \
    {                                                   \
                                                        \
    }                                                   \
  };


DEFINE_EEPIC_CLASS(Text);

DEFINE_CLASS_DERIVATED_FROM_TEXT(Left_Text);

DEFINE_CLASS_DERIVATED_FROM_TEXT(Right_Text);

DEFINE_CLASS_DERIVATED_FROM_TEXT(Center_Text);

DEFINE_EEPIC_CLASS(Left_Text);

DEFINE_EEPIC_CLASS(Right_Text);

DEFINE_EEPIC_CLASS(Center_Text);


/** Polygon wrappers and variants.
 *
 *  We define multiple Polygon-derived types so each can map to a different
 *  EEPIC drawing behavior.
 */

// The simplest polygon wrapper (default drawing behavior).
DEFINE_EEPIC_CLASS(Polygon);


// Define a Polygon-derived type to allow different drawing behaviors.
# define DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name)      \
  struct Name : public Polygon                          \
  {                                                     \
    Name() { }                                          \
                                                        \
    Name(const Polygon & p) : Polygon(p)                \
    {                                                   \
                                                        \
    }                                                   \
  };


# define DEFINE_EEPIC_POLYGON(Name)             \
  DEFINE_CLASS_DERIVATED_FROM_POLYGON(Name);    \
  DEFINE_EEPIC_CLASS(Name);


// Polygon variants.

// With point circles at vertices.
DEFINE_EEPIC_POLYGON(Polygon_With_Points);

// With point circles at vertices and arrow heads on sides.
DEFINE_EEPIC_POLYGON(Polygon_With_Arrows);

// Dotted sides.
DEFINE_EEPIC_POLYGON(Dotted_Polygon);

// Dashed sides.
DEFINE_EEPIC_POLYGON(Dash_Polygon);

// Dotted sides with point circles at vertices.
DEFINE_EEPIC_POLYGON(Dotted_Polygon_With_Points);

// Dashed sides with point circles at vertices.
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Points);

// Dashed sides with an arrow head on the last segment.
DEFINE_EEPIC_POLYGON(Dash_Polygon_With_Arrow);

// Shaded interior (gray).
DEFINE_EEPIC_POLYGON(Shade_Polygon);

// Shaded interior with point circles at vertices.
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Points);

// Shaded interior, sides drawn as arrows.
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Arrows);

// Shaded interior with vertex coordinate labels.
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Text_Points);

// Shaded interior with vertex numbers.
DEFINE_EEPIC_POLYGON(Shade_Polygon_With_Vertex_Numbers);

// Spline curve built from polygon vertices.
DEFINE_EEPIC_POLYGON(Spline);

// Piecewise-linear curve built from polygon vertices.
DEFINE_EEPIC_POLYGON(Dash_Spline);

// Spline curve with arrow head at the end.
DEFINE_EEPIC_POLYGON(Spline_Arrow);

// Piecewise-linear curve with arrow head at the end.
DEFINE_EEPIC_POLYGON(Dash_Spline_Arrow);

// With point circles at vertices.
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Points);

// With point circles at vertices.
DEFINE_EEPIC_POLYGON(Thick_Polygon_With_Arrows);

// Dotted sides.
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon);

// Dashed sides.
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon);

// Dotted sides with point circles at vertices.
DEFINE_EEPIC_POLYGON(Thick_Dotted_Polygon_With_Points);

// Dashed sides with point circles at vertices.
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Points);

// Dashed sides with arrow heads.
DEFINE_EEPIC_POLYGON(Thick_Dash_Polygon_With_Arrow);

// Shaded interior (gray).
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon);

// Shaded interior with point circles at vertices.
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Points);

// Shaded interior, sides drawn as arrows.
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Arrows);

// Shaded interior with vertex coordinate text.
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Text_Points);

// Shaded interior with vertex numbers.
DEFINE_EEPIC_POLYGON(Thick_Shade_Polygon_With_Vertex_Numbers);

// Thick spline curve built from polygon vertices.
DEFINE_EEPIC_POLYGON(Thick_Spline);

// Thick piecewise-linear curve built from polygon vertices.
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline);

// Thick spline curve with arrow head.
DEFINE_EEPIC_POLYGON(Thick_Spline_Arrow);

// Thick piecewise-linear curve with arrow head.
DEFINE_EEPIC_POLYGON(Thick_Dash_Spline_Arrow);

/** Regular polygons.
 *
 *  NOTE: In practice, because Polygon(Regular_Polygon) exists and there is a
 *  conversion from Regular_Polygon to Polygon, regular polygons could be drawn
 *  using the same Eepic_* classes used for irregular polygons.
 *
 *  The purpose of defining dedicated regular-polygon wrappers is mainly
 *  efficiency/constant-factor improvements: a regular polygon can be drawn a
 *  bit faster than a general polygon, even though the time complexity remains
 *  \(O(n)\) in both cases.
 */

// Define a new class derived from Regular_Polygon to allow different drawing behaviors.
// A new type is needed because the constructor differs.
# define DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name)      \
  struct Name : public Regular_Polygon                          \
  {                                                             \
    Name() { }                                                  \
                                                                \
    Name(const Regular_Polygon & p) : Regular_Polygon(p)        \
    {                                                           \
                                                                \
    }                                                           \
                                                                \
    Name(const Point &  c,                                      \
         const double & side_sz,                                \
         const size_t & n,                                      \
         const double & ang = 0)                                \
      : Regular_Polygon(c, side_sz, n, ang)                     \
    {                                                           \
                                                                \
    }                                                           \
  };


# define DEFINE_EEPIC_REGULAR_POLYGON(Name)             \
  DEFINE_CLASS_DERIVATED_FROM_REGULAR_POLYGON(Name);    \
  DEFINE_EEPIC_CLASS(Name);


DEFINE_EEPIC_CLASS(Regular_Polygon);

// Regular polygon variants.
DEFINE_EEPIC_REGULAR_POLYGON(Regular_Polygon_With_Points);

DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon);

DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon);

DEFINE_EEPIC_REGULAR_POLYGON(Dotted_Regular_Polygon_With_Points);

DEFINE_EEPIC_REGULAR_POLYGON(Dash_Regular_Polygon_With_Points);

DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon);

DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Points);

DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Arrows);

DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Text_Points);

DEFINE_EEPIC_REGULAR_POLYGON(Shade_Regular_Polygon_With_Vertex_Numbers);


# endif // EEPICGEOM_H
