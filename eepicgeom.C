
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


# include "eepicgeom.H"

const double Eepic_Plane::default_r = 20;
const double Eepic_Plane::normal_thickness = 20;
const double Eepic_Plane::default_shade_thickness = 200;
const double Eepic_Plane::dotgap = 65; 
const double Eepic_Plane::arrow_width_in_mm = 0.5;
const double Eepic_Plane::arrow_lenght_in_mm = 1.5;

// Instantiations of the put_in_plane helper function for each drawable type.
IMPL_PUT_IN_PLANE(Point);
IMPL_PUT_IN_PLANE(Segment);
IMPL_PUT_IN_PLANE(Text);
IMPL_PUT_IN_PLANE(Arrow);
IMPL_PUT_IN_PLANE(Thick_Segment);
IMPL_PUT_IN_PLANE(Thick_Dash_Segment);
IMPL_PUT_IN_PLANE(Thick_Arrow);
IMPL_PUT_IN_PLANE(Dotted_Segment);
IMPL_PUT_IN_PLANE(Dash_Segment);
IMPL_PUT_IN_PLANE(Dotted_Arrow);
IMPL_PUT_IN_PLANE(Dash_Arrow);
IMPL_PUT_IN_PLANE(Thick_Dash_Arrow);
IMPL_PUT_IN_PLANE(Triangle);
IMPL_PUT_IN_PLANE(Ellipse);
IMPL_PUT_IN_PLANE(Shade_Ellipse);
IMPL_PUT_IN_PLANE(Thick_Ellipse);
IMPL_PUT_IN_PLANE(Left_Text);
IMPL_PUT_IN_PLANE(Right_Text);
IMPL_PUT_IN_PLANE(Center_Text);
IMPL_PUT_IN_PLANE(Polygon);

// with point circles at vertices
IMPL_PUT_IN_PLANE(Polygon_With_Points); 

// with point circles at vertices and arrows on sides
IMPL_PUT_IN_PLANE(Polygon_With_Arrows); 

// with dotted sides
IMPL_PUT_IN_PLANE(Dotted_Polygon);

// with dashed sides
IMPL_PUT_IN_PLANE(Dash_Polygon);

// with dotted sides and circles at vertices
IMPL_PUT_IN_PLANE(Dotted_Polygon_With_Points); 

// with dashed sides and circles at vertices
IMPL_PUT_IN_PLANE(Dash_Polygon_With_Points);

// with dashed sides and an arrow on the last segment
IMPL_PUT_IN_PLANE(Dash_Polygon_With_Arrow);

// with shaded (gray) interior
IMPL_PUT_IN_PLANE(Shade_Polygon); 

// with shaded interior and circles at vertices
IMPL_PUT_IN_PLANE(Shade_Polygon_With_Points);

// with shaded interior and arrows for sides
IMPL_PUT_IN_PLANE(Shade_Polygon_With_Arrows);

// with shaded interior and vertex coordinate labels
IMPL_PUT_IN_PLANE(Shade_Polygon_With_Text_Points);

// with shaded interior and vertex numbers
IMPL_PUT_IN_PLANE(Shade_Polygon_With_Vertex_Numbers);

// Spline curve through polygon vertices
IMPL_PUT_IN_PLANE(Spline);

// Dashed spline curve through polygon vertices
IMPL_PUT_IN_PLANE(Dash_Spline);

// Spline curve with an arrow at the end
IMPL_PUT_IN_PLANE(Spline_Arrow);

// Dashed spline curve with an arrow at the end
IMPL_PUT_IN_PLANE(Dash_Spline_Arrow);

// with point circles at vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Polygon_With_Points); 

// with point circles at vertices and arrows on sides (thick)
IMPL_PUT_IN_PLANE(Thick_Polygon_With_Arrows); 

// with dotted sides (thick)
IMPL_PUT_IN_PLANE(Thick_Dotted_Polygon);

// with dashed sides (thick)
IMPL_PUT_IN_PLANE(Thick_Dash_Polygon);

// with dotted sides and circles at vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Dotted_Polygon_With_Points); 

// with dashed sides and circles at vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Dash_Polygon_With_Points);

// with dashed sides and arrow heads (thick)
IMPL_PUT_IN_PLANE(Thick_Dash_Polygon_With_Arrow);

// with shaded interior (thick)
IMPL_PUT_IN_PLANE(Thick_Shade_Polygon); 

// with shaded interior and circles at vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Shade_Polygon_With_Points);

// with shaded interior and arrows for sides (thick)
IMPL_PUT_IN_PLANE(Thick_Shade_Polygon_With_Arrows);

// with shaded interior and vertex coordinate labels (thick)
IMPL_PUT_IN_PLANE(Thick_Shade_Polygon_With_Text_Points);

// with shaded interior and vertex numbers (thick)
IMPL_PUT_IN_PLANE(Thick_Shade_Polygon_With_Vertex_Numbers);

// Spline curve through polygon vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Spline);

// Dashed spline curve through polygon vertices (thick)
IMPL_PUT_IN_PLANE(Thick_Dash_Spline);

// Spline curve with an arrow at the end (thick)
IMPL_PUT_IN_PLANE(Thick_Spline_Arrow);

// Dashed spline curve with an arrow at the end (thick)
IMPL_PUT_IN_PLANE(Thick_Dash_Spline_Arrow);


IMPL_PUT_IN_PLANE(Regular_Polygon_With_Points);
IMPL_PUT_IN_PLANE(Dotted_Regular_Polygon);
IMPL_PUT_IN_PLANE(Dash_Regular_Polygon);
IMPL_PUT_IN_PLANE(Dotted_Regular_Polygon_With_Points);
IMPL_PUT_IN_PLANE(Dash_Regular_Polygon_With_Points);
IMPL_PUT_IN_PLANE(Shade_Regular_Polygon);
IMPL_PUT_IN_PLANE(Shade_Regular_Polygon_With_Points);
IMPL_PUT_IN_PLANE(Shade_Regular_Polygon_With_Arrows);
IMPL_PUT_IN_PLANE(Shade_Regular_Polygon_With_Text_Points);
IMPL_PUT_IN_PLANE(Shade_Regular_Polygon_With_Vertex_Numbers);

/**
 * @brief Draws the wrapped Point object.
 */
void Eepic_Point::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Point & p = static_cast<const Point&>(*geom_object_ptr);

  plane->draw_point(p, output);
}

/**
 * @brief Draws the wrapped Segment object.
 */
void Eepic_Segment::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object with thick lines.
 */
void Eepic_Thick_Segment::draw(Eepic_Plane * plane, std::ostream & output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a thick, dashed line.
 */
void Eepic_Thick_Dash_Segment::draw(Eepic_Plane * plane, 
				    std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_dash_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as an arrow.
 */
void Eepic_Arrow::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_arrow(sg, output);
}

/**
 * @brief Draws the Cartesian axes on the plane, if enabled.
 */
void Eepic_Plane::draw_cartesian_axis(std::ostream & output)
{
  if (not with_cartesian_axis)
    return;

  Segment x_axis(Point(__leftmost.get_x(), 0), Point(__rightmost.get_x(), 0)); 
  draw_arrow(x_axis, output);

  Segment y_axis(Point(0, __lowest.get_y()), Point(0, __highest.get_y())); 
  draw_arrow(y_axis, output);
}

/**
 * @brief Draws the wrapped Segment object as a thick arrow.
 */
void Eepic_Thick_Arrow::draw(Eepic_Plane * plane, std::ostream & output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_arrow(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a dotted line.
 */
void Eepic_Dotted_Segment::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_dotted_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a dashed line.
 */
void Eepic_Dash_Segment::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_dash_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a dotted arrow.
 */
void Eepic_Dotted_Arrow::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_arrow_dotted_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a dashed arrow.
 */
void Eepic_Dash_Arrow::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_arrow_dash_segment(sg, output);
}

/**
 * @brief Draws the wrapped Segment object as a thick, dashed arrow.
 */
void Eepic_Thick_Dash_Arrow::draw(Eepic_Plane * plane, 
				  std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  const Segment & sg = static_cast<const Segment&>(*geom_object_ptr);

  plane->draw_arrow_dash_segment(sg, output);
}

/**
 * @brief Draws the wrapped Triangle object.
 */
void Eepic_Triangle::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Triangle & t = static_cast<const Triangle&>(*geom_object_ptr);

  const double p1_x = plane->x_geom_number_to_eepic(t.get_p1().get_x());
  const double p1_y = plane->y_geom_number_to_eepic(t.get_p1().get_y());
  const double p2_x = plane->x_geom_number_to_eepic(t.get_p2().get_x());
  const double p2_y = plane->y_geom_number_to_eepic(t.get_p2().get_y());
  const double p3_x = plane->x_geom_number_to_eepic(t.get_p3().get_x());
  const double p3_y = plane->y_geom_number_to_eepic(t.get_p3().get_y());

  output << "    %% Triangle from " << t.get_p1().to_string() 
	 << " to " << t.get_p2().to_string() 
	 << " to " << t.get_p3().to_string() << std::endl
	 << "    %% mapped in this plane to (" << p1_x << "," << p1_y 
	 << ") to (" << p2_x << "," << p2_y << ") to (" 
	 << p3_x << "," << p3_y << ")" << std::endl
	 << "\\path(" << p1_x << "," << p1_y 
	 << ")(" << p2_x << "," << p2_y << ")("
	 << p3_x << "," << p3_y << ")(" 
	 << p1_x << "," << p1_y << ")" << std::endl
	 << std::endl;
}

/**
 * @brief Draws the wrapped Ellipse object.
 */
void Eepic_Ellipse::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Ellipse & e = static_cast<const Ellipse&>(*geom_object_ptr);
    
  plane->draw_ellipse(e, output);
}

/**
 * @brief Draws the wrapped Ellipse object with a shaded interior.
 */
void Eepic_Shade_Ellipse::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Ellipse & e = static_cast<const Ellipse&>(*geom_object_ptr);
    
  output << "\\shade" << std::endl;

  plane->draw_ellipse(e, output);
}

/**
 * @brief Draws the wrapped Ellipse object with thick lines.
 */
void Eepic_Thick_Ellipse::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Ellipse & e = static_cast<const Ellipse&>(*geom_object_ptr);
    
  output << "\\Thicklines" << std::endl;

  plane->draw_ellipse(e, output);
}

/**
 * @brief Draws the wrapped Text object.
 */
void Eepic_Text::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Text & t = static_cast<const Text &>(*geom_object_ptr);	

  plane->draw_text(t, output, 0, 0.5);
}

/**
 * @brief Draws the wrapped Text object, left-aligned.
 */
void Eepic_Left_Text::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Text & t = static_cast<const Text &>(*geom_object_ptr);	

  plane->draw_text(t, output, 0, 0.5);
}

/**
 * @brief Draws the wrapped Text object, right-aligned.
 */
void Eepic_Right_Text::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Text & t = static_cast<const Text &>(*geom_object_ptr);	

  plane->draw_text(t, output, 1.2, 0.5);
}

/**
 * @brief Draws the wrapped Text object, center-aligned.
 */
void Eepic_Center_Text::draw(Eepic_Plane * plane, std::ostream & output) const
{
  const Text & t = static_cast<const Text &>(*geom_object_ptr);	

  plane->draw_text(t, output, 0.5, 0.5);
}

/**
 * @brief Draws the wrapped Polygon object.
 */
void Eepic_Polygon::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_segment);
}

// Implementations of how each of the styled polygons are drawn.

/**
 * @brief Draws the polygon's edges and adds circles at its vertices.
 */
void Eepic_Polygon_With_Points::draw(Eepic_Plane * plane, 
				     std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a shaded interior and arrows on its edges.
 */
void Eepic_Polygon_With_Arrows::draw(Eepic_Plane * plane, 
				     std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon_with_arrows(poly, output);
}

/**
 * @brief Draws the polygon with dotted edges.
 */
void Eepic_Dotted_Polygon::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
}

/**
 * @brief Draws the polygon with dashed edges.
 */
void Eepic_Dash_Polygon::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
}

/**
 * @brief Draws the polygon with dotted edges and circles at its vertices.
 */
void Eepic_Dotted_Polygon_With_Points::draw(Eepic_Plane * plane, 
					    std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with dashed edges and circles at its vertices.
 */
void Eepic_Dash_Polygon_With_Points::draw(Eepic_Plane * plane, 
					  std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with dashed edges and an arrow on the last segment.
 */
void Eepic_Dash_Polygon_With_Arrow::draw(Eepic_Plane * plane, 
					 std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
  Segment sg = poly.get_last_segment();
  plane->draw_arrow_extreme(sg, output);
}

/**
 * @brief Draws the polygon with a shaded interior.
 */
void Eepic_Shade_Polygon::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a shaded interior and circles at its vertices.
 */
void Eepic_Shade_Polygon_With_Points::draw(Eepic_Plane * plane, 
					   std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon(poly, output);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a shaded interior and arrows on its edges.
 */
void Eepic_Shade_Polygon_With_Arrows::draw(Eepic_Plane * plane, 
					   std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon(poly, output);
  plane->draw_arrows_in_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a shaded interior and text labels for its vertices.
 */
void Eepic_Shade_Polygon_With_Text_Points::draw(Eepic_Plane * plane, 
						std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon(poly, output);
  plane->draw_points_text_in_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a shaded interior and index numbers for its vertices.
 */
void Eepic_Shade_Polygon_With_Vertex_Numbers::draw(Eepic_Plane * plane, 
						   std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_closed_polygon(poly, output);
  plane->draw_vertex_numbers_in_polygon(poly, output);
}

/**
 * @brief Draws a spline curve through the polygon's vertices.
 */
void Eepic_Spline::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_spline(poly, output);
}

/**
 * @brief Draws a dashed spline curve through the polygon's vertices.
 */
void Eepic_Dash_Spline::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_dash_spline(poly, output);
}

/**
 * @brief Draws a spline curve with an arrow at the end.
 */
void Eepic_Spline_Arrow::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_spline_arrow(poly, output);
}

/**
 * @brief Draws a dashed spline curve with an arrow at the end.
 */
void Eepic_Dash_Spline_Arrow::draw(Eepic_Plane * plane, 
				   std::ostream &    output) const
{
  Polygon & poly = (Polygon&)(*geom_object_ptr);

  plane->draw_dash_spline_arrow(poly, output);
}

// Exactly the same methods as above, but with the addition of \Thicklines

/**
 * @brief Draws the polygon with thick edges and circles at its vertices.
 */
void Eepic_Thick_Polygon_With_Points::draw(Eepic_Plane * plane, 
					   std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick shaded interior and arrows on its edges.
 */
void Eepic_Thick_Polygon_With_Arrows::draw(Eepic_Plane * plane, 
					   std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon_with_arrows(poly, output);
}

/**
 * @brief Draws the polygon with thick, dotted edges.
 */
void Eepic_Thick_Dotted_Polygon::draw(Eepic_Plane * plane, 
				      std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
}

/**
 * @brief Draws the polygon with thick, dashed edges.
 */
void Eepic_Thick_Dash_Polygon::draw(Eepic_Plane * plane, 
				    std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
}

/**
 * @brief Draws the polygon with thick, dotted edges and circles at its vertices.
 */
void Eepic_Thick_Dotted_Polygon_With_Points::draw(Eepic_Plane * plane, 
						  std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with thick, dashed edges and an arrow on the last segment.
 */
void Eepic_Thick_Dash_Polygon_With_Arrow::draw(Eepic_Plane * plane, 
					       std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
  Segment sg = poly.get_last_segment();
  plane->draw_arrow_extreme(sg, output);
}

/**
 * @brief Draws the polygon with thick, dashed edges and circles at its vertices.
 */
void Eepic_Thick_Dash_Polygon_With_Points::draw(Eepic_Plane * plane, 
						std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick, shaded interior.
 */
void Eepic_Thick_Shade_Polygon::draw(Eepic_Plane * plane, 
				     std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick, shaded interior and circles at its vertices.
 */
void Eepic_Thick_Shade_Polygon_With_Points::draw(Eepic_Plane * plane, 
						 std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick, shaded interior and arrows on its edges.
 */
void Eepic_Thick_Shade_Polygon_With_Arrows::draw(Eepic_Plane * plane, 
						 std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_arrows_in_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick, shaded interior and text labels for its vertices.
 */
void Eepic_Thick_Shade_Polygon_With_Text_Points::draw(Eepic_Plane * plane, 
						 std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_points_text_in_polygon(poly, output);
}

/**
 * @brief Draws the polygon with a thick, shaded interior and index numbers for its vertices.
 */
void Eepic_Thick_Shade_Polygon_With_Vertex_Numbers::draw(Eepic_Plane * plane, 
						    std::ostream & output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_vertex_numbers_in_polygon(poly, output);
}

/**
 * @brief Draws a thick spline curve through the polygon's vertices.
 */
void Eepic_Thick_Spline::draw(Eepic_Plane * plane, std::ostream & output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_spline(poly, output);
}

/**
 * @brief Draws a thick, dashed spline curve through the polygon's vertices.
 */
void Eepic_Thick_Dash_Spline::draw(Eepic_Plane * plane, 
				   std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_dash_spline(poly, output);
}

/**
 * @brief Draws a thick spline curve with an arrow at the end.
 */
void Eepic_Thick_Spline_Arrow::draw(Eepic_Plane * plane, 
				    std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_spline_arrow(poly, output);
}

/**
 * @brief Draws a thick, dashed spline curve with an arrow at the end.
 */
void Eepic_Thick_Dash_Spline_Arrow::draw(Eepic_Plane * plane, 
					 std::ostream &    output) const
{
  output << "\\Thicklines" << std::endl
	 << std::endl;

  Polygon & poly = (Polygon&)(*geom_object_ptr);
  plane->draw_dash_spline_arrow(poly, output);
}

/**
 * @brief Draws the wrapped Regular_Polygon object.
 */
void Eepic_Regular_Polygon::draw(Eepic_Plane * plane, std::ostream & output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);

  plane->draw_polygon(poly, output, &Eepic_Plane::draw_segment);
}

/**
 * @brief Draws the regular polygon's edges and adds circles at its vertices.
 */
void Eepic_Regular_Polygon_With_Points::draw(Eepic_Plane * plane, 
					     std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with dotted edges.
 */
void Eepic_Dotted_Regular_Polygon::draw(Eepic_Plane * plane, 
					std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
}

/**
 * @brief Draws the regular polygon with dashed edges.
 */
void Eepic_Dash_Regular_Polygon::draw(Eepic_Plane * plane, 
				      std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
}

/**
 * @brief Draws the regular polygon with dotted edges and circles at its vertices.
 */
void Eepic_Dotted_Regular_Polygon_With_Points::draw(Eepic_Plane * plane, 
						    std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dotted_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with dashed edges and circles at its vertices.
 */
void Eepic_Dash_Regular_Polygon_With_Points::draw(Eepic_Plane * plane, 
						  std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_polygon(poly, output, &Eepic_Plane::draw_dash_segment);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with a shaded interior.
 */
void Eepic_Shade_Regular_Polygon::draw(Eepic_Plane * plane, 
				       std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with a shaded interior and circles at its vertices.
 */
void Eepic_Shade_Regular_Polygon_With_Points::draw(Eepic_Plane * plane, 
						   std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_dots_from_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with a shaded interior and arrows on its edges.
 */
void Eepic_Shade_Regular_Polygon_With_Arrows::draw(Eepic_Plane * plane, 
						   std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_arrows_in_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with a shaded interior and text labels for its vertices.
 */
void Eepic_Shade_Regular_Polygon_With_Text_Points::draw(Eepic_Plane * plane, 
						   std::ostream &    output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_points_text_in_polygon(poly, output);
}

/**
 * @brief Draws the regular polygon with a shaded interior and index numbers for its vertices.
 */
void Eepic_Shade_Regular_Polygon_With_Vertex_Numbers::draw
    (Eepic_Plane * plane, std::ostream & output) const
{
  Regular_Polygon & poly = (Regular_Polygon&)(*geom_object_ptr);
  plane->draw_closed_polygon(poly, output);
  plane->draw_vertex_numbers_in_polygon(poly, output);
}
