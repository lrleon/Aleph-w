
/* Aleph-w

     / \  | | ___ _ __ | |__      __      __
    / _ \ | |/ _ \ '_ \| '_ \ ____\ \ /\ / / Data structures & Algorithms
   / ___ \| |  __/ |_) | | | |_____\ V  V /  version 1.9b
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
