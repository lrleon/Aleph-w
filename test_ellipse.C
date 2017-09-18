
# include <gsl/gsl_rng.h>
# include <gsl/gsl_randist.h>
# include <iostream>
# include <limits>
# include <eepicgeom.H>

double v_size = 190; /* longitud vertical de picture */

using namespace std;

int main()
{
  Eepic_Plane plane(2000, 2000);

  Ellipse el(Point(800, 800), 100, 100);

  put_in_plane(plane, el);

  Segment sg(Point(-80, -80), Point(400, 400));

  put_in_plane(plane, sg);

  Segment inter = el.intersection_with(sg);

  put_in_plane(plane, Arrow(inter));

  const Segment sg_new(sg.get_src_point() + el.get_center(), 
		       sg.get_tgt_point() + el.get_center());

  put_in_plane(plane, sg_new);

  put_in_plane(plane, Text(el.get_center(), el.get_center().to_string()));
  put_in_plane(plane, 
	       Text(inter.get_src_point(), inter.get_src_point().to_string()));
  put_in_plane(plane, 
	       Text(inter.get_tgt_point(), inter.get_tgt_point().to_string()));

  ofstream output1("test-1.eepic", ios::trunc);

  plane.put_cartesian_axis();

  plane.draw(output1);
}
